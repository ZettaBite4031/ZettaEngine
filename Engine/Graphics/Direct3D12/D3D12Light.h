#pragma once

#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12 {
	struct D3D12FrameInfo;
}

namespace Zetta::Graphics::D3D12::Light {
	bool Initialize();
	void Shutdown();

	void CreateLightSet(u64);
	void RemoveLightSet(u64);
	Graphics::Light Create(LightInitInfo info);
	void Remove(LightID id, u64 light_set_key);

	void SetParameter(LightID id, u64 light_set_key, LightParameter::Parameter param, const void* const data, u32 size);
	void GetParameter(LightID id, u64 light_set_key, LightParameter::Parameter param, void* const data, u32 size);

	void UpdateLightBuffers(const D3D12FrameInfo& d3d12_info);
	D3D12_GPU_VIRTUAL_ADDRESS NoncullableLightBuffer(u32 frame_idx);
	D3D12_GPU_VIRTUAL_ADDRESS CullableLightBuffer(u32 frame_idx);
	D3D12_GPU_VIRTUAL_ADDRESS CullingInfoBuffer(u32 frame_idx); 
	D3D12_GPU_VIRTUAL_ADDRESS BoundingSphereBuffer(u32 frame_idx);
	u32 NoncullableLightCount(u64 light_set_key);
	u32 CullableLightCount(u64 light_set_key);


}