#include "D3D12Surface.h"
#include "D3D12Core.h"

namespace Zetta::Graphics::D3D12 {
	namespace {
		constexpr DXGI_FORMAT ToNonSRGB(DXGI_FORMAT format) {
			if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) return DXGI_FORMAT_R8G8B8A8_UNORM;

			return format;
		}
	}

	void D3D12Surface::CreateSwapchain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue, DXGI_FORMAT format) {
		assert(factory && cmd_queue);
		Release();

		if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &_allow_tearing, sizeof(u32))) && _allow_tearing) 
			_present_flags = DXGI_PRESENT_ALLOW_TEARING;
		
		_format = format;

		DXGI_SWAP_CHAIN_DESC1 desc{};
		desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		desc.BufferCount = BufferCount;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Flags = _allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
		desc.Format = ToNonSRGB(format);
		desc.Height = _window.Height();
		desc.Width = _window.Width();
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Stereo = false;

		IDXGISwapChain1* swapchain;
		HWND hwnd{ (HWND)_window.Handle() };
		DXCall(factory->CreateSwapChainForHwnd(cmd_queue, hwnd, &desc, nullptr, nullptr, &swapchain));
		DXCall(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
		DXCall(swapchain->QueryInterface(IID_PPV_ARGS(&_swapchain)));
		Core::Release(swapchain);

		_curent_bb_index = _swapchain->GetCurrentBackBufferIndex();
		for (u32 i{ 0 }; i < BufferCount; i++) 
			_render_target_data[i].rtv = Core::RTV_Heap().Alloc();
		
		Finalize();
	}

	void D3D12Surface::Finalize() {
		for (u32 i{ 0 }; i < BufferCount; i++) {
			RenderTargetData& data{ _render_target_data[i] };
			assert(!data.resource);
			DXCall(_swapchain->GetBuffer(i, IID_PPV_ARGS(&data.resource)));
			D3D12_RENDER_TARGET_VIEW_DESC desc{};
			desc.Format = _format;
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			Core::Device()->CreateRenderTargetView(data.resource, &desc, data.rtv.cpu);
		}
		DXGI_SWAP_CHAIN_DESC desc{};
		DXCall(_swapchain->GetDesc(&desc));
		const u32 width{ desc.BufferDesc.Width };
		const u32 height{ desc.BufferDesc.Height };
		assert(_window.Width() == width && _window.Height() == height);

		_viewport.TopLeftX = 0.f;
		_viewport.TopLeftY = 0.f;
		_viewport.Width = (float)width;
		_viewport.Height = (float)height;
		_viewport.MinDepth = 0.f;
		_viewport.MaxDepth = 1.f;

		_scissor_rect = { 0, 0, (s32)width, (s32)height };
	}

	void D3D12Surface::Present() const {
		assert(_swapchain);
		DXCall(_swapchain->Present(0, _present_flags));
		_curent_bb_index = _swapchain->GetCurrentBackBufferIndex();

	}

	void D3D12Surface::Resize() {
		assert(_swapchain);
		for (u32 i{ 0 }; i < BufferCount; i++) 
			Core::Release(_render_target_data[i].resource);

		const u32 flags{ _allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0ul };
		DXCall(_swapchain->ResizeBuffers(BufferCount, 0, 0, DXGI_FORMAT_UNKNOWN, flags));
		_curent_bb_index = _swapchain->GetCurrentBackBufferIndex();

		Finalize();

		DEBUG_OP(OutputDebugString(L"::D3D12 Surface Resized\n"));
	}

	void D3D12Surface::Release() {
		for (u32 i{ 0 }; i < BufferCount; i++) {
			RenderTargetData& data{ _render_target_data[i] };
			Core::Release(data.resource);
			Core::RTV_Heap().Free(data.rtv);
		}

		Core::Release(_swapchain);
	}
}