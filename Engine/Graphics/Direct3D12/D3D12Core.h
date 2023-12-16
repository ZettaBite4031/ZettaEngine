#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12 {
	namespace Camera { class D3D12Camera; }

	struct D3D12FrameInfo {
		const FrameInfo* info;
		Camera::D3D12Camera* camera;
		D3D12_GPU_VIRTUAL_ADDRESS global_shader_data;
		u32 surface_width{0};
		u32 surface_height{0};
		ID::ID_Type light_culling_id{ ID::Invalid_ID };
		u32 frame_index{0};
		f32 delta_time{0};
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

	[[nodiscard]] ID3D12Device* const Device();
	[[nodiscard]] DescriptorHeap& RTV_Heap();
	[[nodiscard]] DescriptorHeap& DSV_Heap();
	[[nodiscard]] DescriptorHeap& SRV_Heap();
	[[nodiscard]] DescriptorHeap& UAV_Heap();
	[[nodiscard]] ConstantBuffer& CBuffer();

	[[nodiscard]] u32 CurrentFrameIndex();
	void SetDeferredReleasesFlag();	

	[[nodiscard]] Surface CreateSurface(Platform::Window);
	void RemoveSurface(SurfaceID);
	void ResizeSurface(SurfaceID, u32, u32);
	[[nodiscard]] u32 SurfaceWidth(SurfaceID);
	[[nodiscard]] u32 SurfaceHeight(SurfaceID);
	void RenderSurface(SurfaceID, FrameInfo);

}