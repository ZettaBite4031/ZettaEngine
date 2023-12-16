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
			_viewport{ o._viewport }, _scissor_rect{ o._scissor_rect }, _allow_tearing{o._allow_tearing}, _present_flags{o._present_flags},
			_light_culling_id{ o._light_culling_id }
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

		void CreateSwapchain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue);
		void Present() const;
		void Resize();

		[[nodiscard]] constexpr u32 Width() const { return (u32)_viewport.Width; }
		[[nodiscard]] constexpr u32 Height() const { return (u32)_viewport.Height; }
		[[nodiscard]] constexpr ID3D12Resource* const BackBuffer() const { return _render_target_data[_curent_bb_index].resource; }
		[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE RTV() const { return _render_target_data[_curent_bb_index].rtv.cpu; }
		[[nodiscard]] constexpr const D3D12_VIEWPORT& Viewport() const { return _viewport; }
		[[nodiscard]] constexpr const D3D12_RECT& ScissorRect() const { return _scissor_rect; }
		[[nodiscard]] constexpr ID::ID_Type LightCullingID() const { return _light_culling_id; }

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
			_light_culling_id = o._light_culling_id;

			o.Reset();
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
			_light_culling_id = ID::Invalid_ID;
		}
#endif

		struct RenderTargetData {
			ID3D12Resource* resource{ nullptr };
			DescriptorHandle rtv;
		};


		// NOTE: When adding new member variables, *DO NOT* forget to update the move constructor
		IDXGISwapChain4* _swapchain{ nullptr };
		RenderTargetData _render_target_data[BufferCount]{};
		Platform::Window _window{};
		mutable u32 _curent_bb_index{ 0 };
		u32 _allow_tearing{ 0 };
		u32 _present_flags{ 0 };
		D3D12_VIEWPORT _viewport{};
		D3D12_RECT _scissor_rect{};
		ID::ID_Type _light_culling_id{ ID::Invalid_ID };
	};


}