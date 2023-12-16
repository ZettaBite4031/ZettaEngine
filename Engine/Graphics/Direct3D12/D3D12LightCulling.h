#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12 {
	struct D3D12FrameInfo;
}

namespace Zetta::Graphics::D3D12::DeLight {
	constexpr u32 light_culling_tile_size{ 16 };

	bool Initialize();
	void Shutdown();

	[[nodiscard]] ID::ID_Type AddCuller();
	void RemoveCuller(ID::ID_Type id);

	void CullLights(ID3D12GraphicsCommandList* const cmd_list, const D3D12FrameInfo& d3d12_info, D3DX::D3D12ResourceBarrier& barriers);

	//TODO: Temporary for light culling visualization. Remove Later.
	D3D12_GPU_VIRTUAL_ADDRESS Frustums(ID::ID_Type light_culling_id, u32 frame_idx);
	D3D12_GPU_VIRTUAL_ADDRESS LightGridOpaque(ID::ID_Type light_culling_id, u32 frame_idx);
	D3D12_GPU_VIRTUAL_ADDRESS LightIndexListOpaque(ID::ID_Type light_culling_id, u32 frame_idx);
}