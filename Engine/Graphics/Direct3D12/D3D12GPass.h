#pragma once

#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12 {
	struct D3D12FrameInfo;
}

namespace Zetta::Graphics::D3D12::GPass {

	constexpr DXGI_FORMAT main_buffer_format{ DXGI_FORMAT_R16G16B16A16_FLOAT };
	constexpr DXGI_FORMAT depth_buffer_format{ DXGI_FORMAT_D32_FLOAT };

	struct OpaqueRootParameter {
		enum Parameter : u32 {
			GlobalShaderData,
			PerObjectData,
			PositionBuffer,
			ElementBuffer,
			SRV_Indices,
			DirectionLights,
			CullableLights,
			LightGrid,
			LightIndexList,

			count
		};
	};

	bool Initialize();
	void Shutdown();

	[[nodiscard]] const D3D12RenderTexture& MainBuffer();
	[[nodiscard]] const D3D12DepthBuffer& DepthBuffer();

	// NOTE: call this every frame before rendering anything in gpass
	void SetSize(Math::u32v2 size);
	void DepthPrepass(ID3D12GraphicsCommandList* cmd_list, const D3D12FrameInfo& info);
	void Render(ID3D12GraphicsCommandList* cmd_list, const D3D12FrameInfo& info);

	void AddDepthPrepassTransitions(D3DX::D3D12ResourceBarrier& barriers);
	void AddGPassTransitions(D3DX::D3D12ResourceBarrier& barriers);
	void AddPostProcessingTransitions(D3DX::D3D12ResourceBarrier& barriers);

	void SetDepthPrepassRenderTargets(ID3D12GraphicsCommandList* cmd_list);
	void SetGPassRenderTargets(ID3D12GraphicsCommandList* cmd_list);
}
