#include "D3D12GPass.h"
#include "D3D12Core.h"
#include "D3D12Shaders.h"

namespace Zetta::Graphics::D3D12::GPass {
	namespace {
		struct GPassRootParamIndices {
			enum : u32 {
				RootConstants,

				count
			};
		};

		constexpr DXGI_FORMAT main_buffer_format{ DXGI_FORMAT_R16G16B16A16_FLOAT };
		constexpr DXGI_FORMAT depth_buffer_format{ DXGI_FORMAT_D32_FLOAT };
		constexpr Math::u32v2 initial_dimensions{ 100, 100 };
		D3D12RenderTexture gpass_main_buffer{};
		D3D12DepthBuffer gpass_depth_buffer{};
		Math::u32v2 dimensions{ initial_dimensions };

		ID3D12RootSignature* gpass_root_sig{ nullptr };
		ID3D12PipelineState* gpass_pso{ nullptr };

#if _DEBUG
		constexpr f32 clear_value[4]{ 0.5f, 0.5f, 0.5f, 1.0f };
#else
		constexpr f32 clear_value[4]{};
#endif

		bool CreateBuffers(Math::u32v2 size) {

			assert(size.x && size.y);
			gpass_main_buffer.Release();
			gpass_depth_buffer.Release();

			D3D12_RESOURCE_DESC desc{};
			desc.Alignment = 0; // NOTE: 0 is the same as 64KB (or 4MB for MSAA)
			desc.DepthOrArraySize = 1;
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			desc.Format = main_buffer_format;
			desc.Height = size.y;
			desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			desc.MipLevels = 0; // make space for all mip levels
			desc.SampleDesc = { 1, 0 };
			desc.Width = size.x;

			// create the main buffer
			{
				D3D12TextureInitInfo info{};
				info.desc = &desc;
				info.initial_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				info.clear_value.Format = desc.Format;
				memcpy(&info.clear_value.Color, &clear_value[0], sizeof(clear_value));
				gpass_main_buffer = D3D12RenderTexture{ info };
			}

			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			desc.Format = depth_buffer_format;
			desc.MipLevels = 1;
			
			// Create depth buffer
			{
				D3D12TextureInitInfo info{};
				info.desc = &desc;
				info.initial_state = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
				info.clear_value.Format = desc.Format;
				info.clear_value.DepthStencil.Depth = 0.f;
				info.clear_value.DepthStencil.Stencil = 0;

				gpass_depth_buffer = D3D12DepthBuffer{ info };
			}

			NAME_D3D12_OBJECT(gpass_main_buffer.Resource(), L"GPass Main Buffer");
			NAME_D3D12_OBJECT(gpass_depth_buffer.Resource(), L"GPass Depth Buffer");

			return gpass_main_buffer.Resource() && gpass_depth_buffer.Resource();

			return true;
		}
	}

	bool CreateGPassPSOandRootSignature() {
		assert(!gpass_root_sig && !gpass_pso);

		// Create GPass root signature
		using idx = GPassRootParamIndices;
		D3DX::D3D12RootParameter parameter[idx::count]{};
		parameter[idx::RootConstants].AsConstants(3, D3D12_SHADER_VISIBILITY_PIXEL, 1);
		const D3DX::D3D12RootSignatureDesc root_signature{ &parameter[0], idx::count };
		gpass_root_sig = root_signature.Create();
		assert(gpass_root_sig);
		NAME_D3D12_OBJECT(gpass_root_sig, L"GPass Root Signature");

		// Create GPass PSO
		struct {
			D3DX::D3D12PipelineStateSubobjectRootSignature root_signature{ gpass_root_sig };
			D3DX::D3D12PipelineStateSubobjectVS vs{ Shaders::GetEngineShader(Shaders::EngineShader::FullscreenTriangleVS) };
			D3DX::D3D12PipelineStateSubobjectPS fs{ Shaders::GetEngineShader(Shaders::EngineShader::FillColorPS) };
			D3DX::D3D12PipelineStateSubobjectPrimitiveTopology primitive_topology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
			D3DX::D3D12PipelineStateSubobjectRenderTargetFormats render_target_formats;
			D3DX::D3D12PipelineStateSubobjectDepthStencilFormats depth_stencil_formats{ depth_buffer_format };
			D3DX::D3D12PipelineStateSubobjectRasterizer rasterizer{ D3DX::RasterizerState.no_cull };
			D3DX::D3D12PipelineStateSubobjectDepthStencil1 depth{ D3DX::DepthState.disabled };
			
		} stream; 

		D3D12_RT_FORMAT_ARRAY rtf_array{};
		rtf_array.NumRenderTargets = 1;
		rtf_array.RTFormats[0] = main_buffer_format;

		stream.render_target_formats = rtf_array;

		gpass_pso = D3DX::CreatePipelineState(&stream, sizeof(stream));
		NAME_D3D12_OBJECT(gpass_pso, L"GPass Pipeline State Object");

		return gpass_root_sig && gpass_pso;
	}

	bool Initialize() {
		return CreateBuffers(initial_dimensions) &&
			CreateGPassPSOandRootSignature();
	}

	void Shutdown() {
		gpass_main_buffer.Release();
		gpass_depth_buffer.Release();
		dimensions = initial_dimensions;

		Core::Release(gpass_root_sig);
		Core::Release(gpass_pso);
	}


	[[nodiscard]] const D3D12RenderTexture& MainBuffer() { return gpass_main_buffer; }

	[[nodiscard]] const D3D12DepthBuffer& DepthBuffer() { return gpass_depth_buffer; }

	void SetSize(Math::u32v2 size) {
		Math::u32v2& d{ dimensions };
		if (size.x > d.x || size.y > d.y) {
			d = { std::max(size.x, d.x), std::max(size.y, d.y) };
			CreateBuffers(d);
		}
	}

	void DepthPrepass(ID3D12GraphicsCommandList* cmd_list, const D3D12FrameInfo& info) {

	}

	void Render(ID3D12GraphicsCommandList* cmd_list, const D3D12FrameInfo& frame_info) {
		cmd_list->SetGraphicsRootSignature(gpass_root_sig);
		cmd_list->SetPipelineState(gpass_pso);

		static u32 frame{ 0 };
		struct {
			f32 width;
			f32 height;
			u32 frame;
		} constants { (f32)frame_info.surface_width, (f32)frame_info.surface_height, frame++ };

		using idx = GPassRootParamIndices;
		cmd_list->SetGraphicsRoot32BitConstants(idx::RootConstants, 3, &constants, 0);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd_list->DrawInstanced(3, 1, 0, 0);
	}

	void AddDepthPrepassTransitions(D3DX::D3D12ResourceBarrier& barriers) {
		barriers.Add(gpass_depth_buffer.Resource(),
			D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

	void AddGPassTransitions(D3DX::D3D12ResourceBarrier& barriers) {
		barriers.Add(gpass_main_buffer.Resource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		barriers.Add(gpass_depth_buffer.Resource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	void AddPostProcessingTransitions(D3DX::D3D12ResourceBarrier& barriers) {
		barriers.Add(gpass_main_buffer.Resource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	void SetDepthPrepassRenderTargets(ID3D12GraphicsCommandList* cmd_list) {
		const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ gpass_depth_buffer.DSV() };
		cmd_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.f, 0, 0, nullptr);
		cmd_list->OMSetRenderTargets(0, nullptr, 0, &dsv);
	}

	void SetGPassRenderTargets(ID3D12GraphicsCommandList* cmd_list) {
		const D3D12_CPU_DESCRIPTOR_HANDLE rtv{ gpass_main_buffer.RTV(0) };
		const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ gpass_depth_buffer.DSV() };

		cmd_list->ClearRenderTargetView(rtv, clear_value, 0, nullptr);
		cmd_list->OMSetRenderTargets(1, &rtv, 0, &dsv);
	}
}