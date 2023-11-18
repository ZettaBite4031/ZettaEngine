#pragma once
#include "D3D12CommonHeaders.h"
#include "D3D12Resources.h"

namespace Zetta::Graphics::D3D12 {
	class D3D12Surface {
	public:
		constexpr static u32 BufferCount{ 3 };
		constexpr static DXGI_FORMAT DefaultBackBufferFormat{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB};
		explicit D3D12Surface(Platform::Window window)
			: _window{ window }
		{
			assert(_window.Handle());
		}

#if USE_STL_VECTOR
		DISABLE_COPY(D3D12Surface);
		constexpr D3D12Surface(D3D12Surface&& o)
			: _swapchain{ o._swapchain }, _window{ o._window }, _curent_bb_index{ o._curent_bb_index },
			_viewport{ o._viewport }, _scissor_rect{ o._scissor_rect }, _allow_tearing{o._allow_tearing}, _present_flags{o._present_flags}
		{
			for (u32 i{ 0 }; i < BufferCount; i++) {
				_render_target_data[i].resource = o._render_target_data[i].resource;
				_render_target_data[i].rtv = o._render_target_data[i].rtv;
			}

			o.Reset();
		}

		constexpr D3D12Surface& operator=(D3D12Surface&& o){
			assert(this != &o);
			if (this != &o) {
				Release();
				Move(o);
			}
			return *this;
		}
#endif
		~D3D12Surface() { Release(); }

		void CreateSwapchain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue, DXGI_FORMAT format = DefaultBackBufferFormat);
		void Present() const;
		void Resize();

		constexpr u32 Width() const { return (u32)_viewport.Width; }
		constexpr u32 Height() const { return (u32)_viewport.Height; }
		constexpr ID3D12Resource* const BackBuffer() const { return _render_target_data[_curent_bb_index].resource; }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE RTV() const { return _render_target_data[_curent_bb_index].rtv.cpu; }
		constexpr const D3D12_VIEWPORT& Viewport() const { return _viewport; }
		constexpr const D3D12_RECT& ScissorRect() const { return _scissor_rect; }

	private:
		void Finalize();
		void Release();

#if USE_STL_VECTOR
		constexpr void Move(D3D12Surface& o) {
			_swapchain = o._swapchain;
			for (u32 i{ 0 }; i < BufferCount; i++)
				_render_target_data[i] = o._render_target_data[i];
			_window = o._window;
			_curent_bb_index = o._curent_bb_index;
			_viewport = o._viewport;
			_scissor_rect = o._scissor_rect;
			_present_flags = o._present_flags;
			_allow_tearing = o._allow_tearing;
		}

		constexpr void Reset() {
			_swapchain = nullptr;
			for (u32 i{ 0 }; i < BufferCount; i++)
				_render_target_data[i] = {};
			
			_window = {};
			_curent_bb_index = 0;
			_viewport = {};
			_scissor_rect = {};
			_present_flags = 0;
			_allow_tearing = 0;
		}
#endif

		struct RenderTargetData {
			ID3D12Resource* resource{ nullptr };
			DescriptorHandle rtv;
		};

		IDXGISwapChain4* _swapchain{ nullptr };
		RenderTargetData _render_target_data[BufferCount]{};
		Platform::Window _window{};
		DXGI_FORMAT _format{ DefaultBackBufferFormat };
		mutable u32 _curent_bb_index{ 0 };
		u32 _allow_tearing{ 0 };
		u32 _present_flags{ 0 };
		D3D12_VIEWPORT _viewport{};
		D3D12_RECT _scissor_rect{};
	};


}