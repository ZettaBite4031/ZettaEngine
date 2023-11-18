#include "D3D12Core.h"
#include "D3D12Surface.h"
#include "D3D12Shaders.h"
#include "D3D12GPass.h"
#include "D3D12PostProcess.h"

using namespace Microsoft::WRL;

namespace Zetta::Graphics::D3D12::Core {
	namespace {
		class D3D12Command {
		public:
			D3D12Command() = default;
			DISABLE_COPY_AND_MOVE(D3D12Command);
			explicit D3D12Command(ID3D12Device* const device, D3D12_COMMAND_LIST_TYPE type) {
				HRESULT hr{ S_OK };
				D3D12_COMMAND_QUEUE_DESC desc{};
				desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
				desc.NodeMask = 0;
				desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
				desc.Type = type;
				DXCall(hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_cmd_queue)));
				if (FAILED(hr)) goto _error;
				NAME_D3D12_OBJECT(_cmd_queue,
					type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
					L"GFX Command Queue" :
					type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
					L"Compute Command Queue" : L"Command Queue");

				for (u32 i{ 0 }; i < FrameBufferCount; i++) {
					CommandFrame& frame{ _cmd_frames[i] };
					DXCall(hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&frame.cmd_alloc)));
					if (FAILED(hr)) goto _error;
					NAME_D3D12_OBJECT_INDEXED(frame.cmd_alloc, i,
						type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
						L"GFX Command Allocator" :
						type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
						L"Compute Command Allocator" : L"Command Allocator");
				}

				DXCall(hr = device->CreateCommandList(0, type, _cmd_frames[0].cmd_alloc, nullptr, IID_PPV_ARGS(&_cmd_list)));
				if (FAILED(hr)) goto _error;
				DXCall(_cmd_list->Close());
				NAME_D3D12_OBJECT(_cmd_queue,
					type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
					L"GFX Command List" :
					type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
					L"Compute Command List" : L"Command List");

				DXCall(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
				if (FAILED(hr)) goto _error;
				NAME_D3D12_OBJECT(_fence, L"D3D12 Fence");

				_fence_event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
				assert(_fence_event);

				return;

			_error:
				Release();
			}

			~D3D12Command() {
				assert(!_cmd_queue && !_cmd_list && !_fence);
			}

			void BeginFrame() {
				CommandFrame& frame{ _cmd_frames[_frame_index] };
				frame.Wait(_fence_event, _fence);
				DXCall(frame.cmd_alloc->Reset());
				DXCall(_cmd_list->Reset(frame.cmd_alloc, nullptr));

			}

			void EndFrame(const D3D12Surface& surface) {
				DXCall(_cmd_list->Close());
				ID3D12CommandList* const cmd_lists[]{ _cmd_list };
				_cmd_queue->ExecuteCommandLists(_countof(cmd_lists), &cmd_lists[0]);

				surface.Present();

				u64& fence_value{ _fence_value };
				fence_value++;
				CommandFrame& frame{ _cmd_frames[_frame_index] };
				frame.fence_value = fence_value;
				_cmd_queue->Signal(_fence, fence_value);

				_frame_index = (_frame_index + 1) % FrameBufferCount;
			}

			void Flush() {
				for (u32 i{ 0 }; i < FrameBufferCount; i++)
					_cmd_frames[i].Wait(_fence_event, _fence);
				_frame_index = 0;
			}

			void Release() {
				Flush();
				Core::Release(_fence);
				_fence_value = 0;

				CloseHandle(_fence_event);
				_fence_event = nullptr;

				Core::Release(_cmd_queue);
				Core::Release(_cmd_list);

				for (u32 i{ 0 }; i < FrameBufferCount; i++)
					_cmd_frames[i].Release();
			}

			constexpr ID3D12CommandQueue* const CommandQueue() const { return _cmd_queue; }
			constexpr ID3D12GraphicsCommandList* const CommandList() const { return _cmd_list; }
			constexpr u32 FrameIndex() const { return _frame_index; }

		private:
			struct CommandFrame {
				ID3D12CommandAllocator* cmd_alloc{ nullptr };
				u64 fence_value{ 0 };

				void Wait(HANDLE fence_event, ID3D12Fence1* fence) {
					assert(fence && fence_event);

					if (fence->GetCompletedValue() < fence_value) {
						DXCall(fence->SetEventOnCompletion(fence_value, fence_event));
						WaitForSingleObject(fence_event, INFINITE);
					}
				}

				void Release() {
					Core::Release(cmd_alloc);
					fence_value = 0;
				}
			};

			ID3D12CommandQueue* _cmd_queue{ nullptr };
			ID3D12GraphicsCommandList* _cmd_list{ nullptr };
			ID3D12Fence1* _fence{ nullptr };
			u64 _fence_value{ 0 };
			CommandFrame _cmd_frames[FrameBufferCount]{};
			HANDLE _fence_event{ nullptr };
			u32 _frame_index{ 0 };
		};

		using SurfaceCollection = util::FreeList<D3D12Surface>;

		ID3D12Device* main_device{ nullptr };
		IDXGIFactory7* dxgi_factory{ nullptr };
		D3D12Command		gfx_command;
		SurfaceCollection	surfaces;
		D3DX::D3D12ResourceBarrier resource_barrier{};

		DescriptorHeap		rtv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
		DescriptorHeap		dsv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
		DescriptorHeap		srv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
		DescriptorHeap		uav_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };

		util::vector<IUnknown*> deferred_releases[FrameBufferCount]{};
		u32					deferred_releases_flag[FrameBufferCount]{};
		std::mutex			deferred_release_mutex{};

		constexpr D3D_FEATURE_LEVEL minimum_feature_level{ D3D_FEATURE_LEVEL_11_0 };

		bool FailedInit() {
			Shutdown();
			return false;
		}

		IDXGIAdapter4* DetermineMainAdapter() {
			IDXGIAdapter4* adapter{ nullptr };

			// get adapters in descending order of performance
			for (u32 i{ 0 };
				dxgi_factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
				++i)
			{
				// pick the first adapter that supports the minimum feature level.
				if (SUCCEEDED(D3D12CreateDevice(adapter, minimum_feature_level, __uuidof(ID3D12Device), nullptr)))
				{
					return adapter;
				}
				Release(adapter);
			}

			return nullptr;
		}

		D3D_FEATURE_LEVEL GetMaxFeatureLevel(IDXGIAdapter4* adapter) {
			constexpr D3D_FEATURE_LEVEL feature_levels[4]{
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_12_0,
				D3D_FEATURE_LEVEL_12_1,
			};

			D3D12_FEATURE_DATA_FEATURE_LEVELS feature_level_info{};
			feature_level_info.NumFeatureLevels = _countof(feature_levels);
			feature_level_info.pFeatureLevelsRequested = feature_levels;

			ComPtr<ID3D12Device> device;
			DXCall(D3D12CreateDevice(adapter, minimum_feature_level, IID_PPV_ARGS(&device)));
			DXCall(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_level_info, sizeof(feature_level_info)));
			return feature_level_info.MaxSupportedFeatureLevel;
		}

		void __declspec(noinline) ProcessDeferredReleases(u32 frame_idx) {
			std::lock_guard lock{ deferred_release_mutex };
			deferred_releases_flag[frame_idx] = 0;

			rtv_desc_heap.ProcessDeferredFree(frame_idx);
			dsv_desc_heap.ProcessDeferredFree(frame_idx);
			srv_desc_heap.ProcessDeferredFree(frame_idx);
			uav_desc_heap.ProcessDeferredFree(frame_idx);

			util::vector<IUnknown*>& resources{ deferred_releases[frame_idx] };
			if (!resources.empty()) {
				for (auto& resource : resources) Release(resource);
				resources.clear();
			}
		}
	}

	namespace Detail {
		void DeferredRelease(IUnknown* resource) {
			const u32 frame_index{ CurrentFrameIndex() };
			std::lock_guard lock{ deferred_release_mutex };
			deferred_releases[frame_index].push_back(resource);
			SetDeferredReleasesFlag();
		}
	}

	bool Initialize() {
		if (main_device) Shutdown();

		u32 dxgi_factory_flags{ 0 };
#ifdef _DEBUG
		{
			ComPtr<ID3D12Debug3> debug_interface;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)))) {
				debug_interface->EnableDebugLayer();
#if 0
#pragma warning("WARNING: GPU based validation enabled. Expect considerable performance hits.")
				debug_interface->SetEnableGPUBasedValidation(1);
#endif
			}
			else OutputDebugStringA("WARNING: D3D12 Debug Interface is unavailable.");
			dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif
		HRESULT hr{ S_OK };
		DXCall(hr = CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));
		if (FAILED(hr)) return FailedInit();

		ComPtr<IDXGIAdapter4> main_adapter;
		main_adapter.Attach(DetermineMainAdapter());
		if (!main_adapter) return FailedInit();

		D3D_FEATURE_LEVEL max_feature_level{ GetMaxFeatureLevel(main_adapter.Get()) };
		assert(max_feature_level >= minimum_feature_level);
		if (max_feature_level < minimum_feature_level) return FailedInit();

		DXCall(hr = D3D12CreateDevice(main_adapter.Get(), max_feature_level, IID_PPV_ARGS(&main_device)));
		if (FAILED(hr)) return FailedInit();

		bool res{ true };
		res &= rtv_desc_heap.Initialize(512, false);
		res &= dsv_desc_heap.Initialize(512, false);
		res &= srv_desc_heap.Initialize(4096, true);
		res &= uav_desc_heap.Initialize(512, false);
		if (!res) return FailedInit();

#ifdef _DEBUG
		{
			ComPtr<ID3D12InfoQueue> info_queue;
			DXCall(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		}
#endif

		new (&gfx_command) D3D12Command(main_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		if (!gfx_command.CommandQueue()) return FailedInit();

		if (!(Shaders::Initialize() && GPass::Initialize() && FX::Initialize())) return FailedInit();

		NAME_D3D12_OBJECT(main_device, L"Main D3D12 Device");
		NAME_D3D12_OBJECT(rtv_desc_heap.Heap(), L"RTV Descriptor Heap");
		NAME_D3D12_OBJECT(dsv_desc_heap.Heap(), L"DSV Descriptor Heap");
		NAME_D3D12_OBJECT(srv_desc_heap.Heap(), L"SRV Descriptor Heap");
		NAME_D3D12_OBJECT(uav_desc_heap.Heap(), L"UAV Descriptor Heap");

		return true;
	}

	void Shutdown() {
		gfx_command.Release();

		// NOTE: ProcessDeferredReleases is not called at the end because
		//		 Some Resources (such as swap chains) can't be released
		//		 Before their depending resources are released
		for (u32 i{ 0 }; i < FrameBufferCount; i++) ProcessDeferredReleases(i);

		FX::Shutdown();
		GPass::Shutdown();
		Shaders::Shutdown();

		Release(dxgi_factory);

		rtv_desc_heap.ProcessDeferredFree(0);
		dsv_desc_heap.ProcessDeferredFree(0);
		srv_desc_heap.ProcessDeferredFree(0);
		uav_desc_heap.ProcessDeferredFree(0);

		rtv_desc_heap.Release();
		dsv_desc_heap.Release();
		srv_desc_heap.Release();
		uav_desc_heap.Release();

		// NOTE: Some types only use deferred release for their resources during
		//		 Shutdown, reset, or clear. To finally release those resources
		//		 ProcessDeferredReleases is called once more.
		ProcessDeferredReleases(0);

#ifdef _DEBUG
		{
			{
				ComPtr<ID3D12InfoQueue> info_queue;
				DXCall(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));
				info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
				info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
				info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
			}
			ComPtr<ID3D12DebugDevice2> debug_device;
			DXCall(main_device->QueryInterface(IID_PPV_ARGS(&debug_device)));
			Release(main_device);
			DXCall(debug_device->ReportLiveDeviceObjects(
				D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
		}
#endif

		Release(main_device);
	}

	DescriptorHeap& RTV_Heap() { return rtv_desc_heap; }
	DescriptorHeap& DSV_Heap() { return dsv_desc_heap; }
	DescriptorHeap& SRV_Heap() { return srv_desc_heap; }
	DescriptorHeap& UAV_Heap() { return uav_desc_heap; }

	ID3D12Device* const Device() { return main_device; }
	u32 CurrentFrameIndex() { return gfx_command.FrameIndex(); }
	void SetDeferredReleasesFlag() { deferred_releases_flag[CurrentFrameIndex()] = 1; }

	Surface CreateSurface(Platform::Window window) {

		SurfaceID id{ surfaces.Add(window) };
		surfaces[id].CreateSwapchain(dxgi_factory, gfx_command.CommandQueue());
		return Surface{ id };
	}

	void RemoveSurface(SurfaceID id) {
		gfx_command.Flush();
		surfaces.Remove(id);
	}

	void ResizeSurface(SurfaceID id, u32, u32) {
		gfx_command.Flush();
		surfaces[id].Resize();
	}

	u32 SurfaceWidth(SurfaceID id) {
		return surfaces[id].Width();
	}

	u32 SurfaceHeight(SurfaceID id) {
		return surfaces[id].Height();
	}

	void RenderSurface(SurfaceID id) {
		gfx_command.BeginFrame();
		ID3D12GraphicsCommandList* cmd_list{ gfx_command.CommandList() };

		const u32 frame_idx{ CurrentFrameIndex() };
		if (deferred_releases_flag[frame_idx]) ProcessDeferredReleases(frame_idx);

		const D3D12Surface& surface{ surfaces[id] };
		ID3D12Resource* const current_back_buffer{ surface.BackBuffer() };


		D3D12FrameInfo info {
			surface.Width(),
			surface.Height()
		};

		GPass::SetSize({ info.surface_width, info.surface_height });
		D3DX::D3D12ResourceBarrier& barriers{ resource_barrier };

		ID3D12DescriptorHeap* const heaps[]{ srv_desc_heap.Heap() };
		cmd_list->SetDescriptorHeaps(1, &heaps[0]);

		cmd_list->RSSetViewports(1, &surface.Viewport());
		cmd_list->RSSetScissorRects(1, &surface.ScissorRect());

		barriers.Add(current_back_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
		GPass::AddDepthPrepassTransitions(barriers);
		barriers.Apply(cmd_list);
		GPass::SetDepthPrepassRenderTargets(cmd_list);
		GPass::DepthPrepass(cmd_list, info);

		GPass::AddGPassTransitions(barriers);
		barriers.Apply(cmd_list);
		GPass::SetGPassRenderTargets(cmd_list);
		GPass::Render(cmd_list, info);

		barriers.Add(current_back_buffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

		GPass::AddPostProcessingTransitions(barriers);
		barriers.Apply(cmd_list);
		FX::PostProcessing(cmd_list, surface.RTV());

		D3DX::TransitionResource(cmd_list, current_back_buffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		gfx_command.EndFrame(surface);

	}

}