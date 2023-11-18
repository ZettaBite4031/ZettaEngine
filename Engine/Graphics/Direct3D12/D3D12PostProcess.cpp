#include "D3D12PostProcess.h"
#include "D3D12Core.h"
#include "D3D12Shaders.h"
#include "D3D12Surface.h"
#include "D3D12GPass.h"


namespace Zetta::Graphics::D3D12::FX {
	namespace {
		struct FXRootParamIndices {
			enum : u32 {
				RootConstants,
				DescriptorTable,

				count
			};
		};

		ID3D12RootSignature* fx_root_sig{ nullptr };
		ID3D12PipelineState* fx_pso{ nullptr };

		bool CreateFXPSOandRootSignature() {
			assert(!fx_root_sig && !fx_pso);
			D3DX::D3D12DescriptorRange range{
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND, 0, 0,
				D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
			};

			using idx = FXRootParamIndices;
			D3DX::D3D12RootParameter parameters[idx::count]{};
			parameters[idx::RootConstants].AsConstants(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
			parameters[idx::DescriptorTable].AsDescriptorTable(D3D12_SHADER_VISIBILITY_PIXEL, &range, 1);
			const D3DX::D3D12RootSignatureDesc root_sig{ &parameters[0], _countof(parameters) };
			fx_root_sig = root_sig.Create();
			assert(fx_root_sig);
			NAME_D3D12_OBJECT(fx_root_sig, L"Post-Processing FX Root Signature");

			// Create FX PSO
			struct {
				D3DX::D3D12PipelineStateSubobjectRootSignature root_sig{ fx_root_sig };
				D3DX::D3D12PipelineStateSubobjectVS vs{ Shaders::GetEngineShader(Shaders::EngineShader::FullscreenTriangleVS) };
				D3DX::D3D12PipelineStateSubobjectPS ps{ Shaders::GetEngineShader(Shaders::EngineShader::PostProcessPS) };
				D3DX::D3D12PipelineStateSubobjectPrimitiveTopology primitive_topology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
				D3DX::D3D12PipelineStateSubobjectRenderTargetFormats render_target_formats;
				D3DX::D3D12PipelineStateSubobjectRasterizer rasterizer{ D3DX::RasterizerState.no_cull };
			} stream;

			D3D12_RT_FORMAT_ARRAY rtf_array{};
			rtf_array.NumRenderTargets = 1;
			rtf_array.RTFormats[0] = D3D12Surface::DefaultBackBufferFormat;

			stream.render_target_formats = rtf_array;

			fx_pso = D3DX::CreatePipelineState(&stream, sizeof(stream));
			NAME_D3D12_OBJECT(fx_pso, L"Post-Processing FX Pipeline State Object");

			return fx_root_sig && fx_pso;
		}
	}

	bool Initialize() {
		return CreateFXPSOandRootSignature();
	}

	void Shutdown() {
		Core::Release(fx_root_sig);
		Core::Release(fx_pso);
	}

	void PostProcessing(ID3D12GraphicsCommandList* cmd_list, D3D12_CPU_DESCRIPTOR_HANDLE target_rtv) {
		cmd_list->SetGraphicsRootSignature(fx_root_sig);
		cmd_list->SetPipelineState(fx_pso);

		using idx = FXRootParamIndices;
		cmd_list->SetGraphicsRoot32BitConstant(idx::RootConstants, GPass::MainBuffer().SRV().index, 0);
		cmd_list->SetGraphicsRootDescriptorTable(idx::DescriptorTable, Core::SRV_Heap().GPU_Start());

		cmd_list->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd_list->OMSetRenderTargets(1, &target_rtv, 1, nullptr);
		cmd_list->DrawInstanced(3, 1, 0, 0);
	}
}