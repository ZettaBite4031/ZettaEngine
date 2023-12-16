#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12 {
	struct D3D12FrameInfo;
}

namespace Zetta::Graphics::D3D12::FX {
	bool Initialize();
	void Shutdown();

	void PostProcessing(ID3D12GraphicsCommandList* cmd_list, const D3D12FrameInfo& info, D3D12_CPU_DESCRIPTOR_HANDLE target_rtv);
}