#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12::FX {
	bool Initialize();
	void Shutdown();

	void PostProcessing(ID3D12GraphicsCommandList* cmd_list, D3D12_CPU_DESCRIPTOR_HANDLE target_rtv);
}