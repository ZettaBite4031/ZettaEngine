#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12 {
	struct D3D12FrameInfo {
		u32 surface_width{};
		u32 surface_height{};
	};
}

namespace Zetta::Graphics::D3D12::Core {
	bool Initialize();
	void Shutdown();

	template<typename T>
	constexpr void Release(T*& resource) {
		if (resource) {
			resource->Release();
			resource = nullptr;
		}
	}

	namespace Detail {
		void DeferredRelease(IUnknown* resource);
	}

	template<typename T>
	constexpr void DeferredRelease(T*& resource) {
		if (resource) {
			Detail::DeferredRelease(resource);
			resource = nullptr;
		}
	}

	ID3D12Device* const Device();
	DescriptorHeap& RTV_Heap();
	DescriptorHeap& DSV_Heap();
	DescriptorHeap& SRV_Heap();
	DescriptorHeap& UAV_Heap();

	u32 CurrentFrameIndex();
	void SetDeferredReleasesFlag();	

	Surface CreateSurface(Platform::Window);
	void RemoveSurface(SurfaceID);
	void ResizeSurface(SurfaceID, u32, u32);
	u32 SurfaceWidth(SurfaceID);
	u32 SurfaceHeight(SurfaceID);
	void RenderSurface(SurfaceID);

}