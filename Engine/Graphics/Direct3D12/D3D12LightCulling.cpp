#include "D3D12LightCulling.h"
#include "D3D12Core.h"
#include "Shaders/SharedTypes.h"
#include "D3D12Shaders.h"
#include "D3D12Light.h"
#include "D3D12Camera.h"
#include "D3D12GPass.h"

namespace Zetta::Graphics::D3D12::DeLight {
	namespace {
		struct LightCullingRootParameter {
			enum Parameter : u32 {
				GlobalShaderData,
				Constants,
				FrustumsOutOrIndexCounter,
				FrustumsIn,
				CullingInfo,
				BoundingSpheres,
				LightGridOpaque,
				LightIndexListOpaque,

				count
			};
		};

		struct CullingParameters {
			D3D12Buffer								frustums;
			D3D12Buffer								light_grid_and_index_list;
			UAV_ClearableBuffer						light_index_counter;
			HLSL::LightCullingDispatchParameters	grid_frustums_dispatch_params{};
			HLSL::LightCullingDispatchParameters	light_culling_dispatch_params{};
			u32										frustum_count{ 0 };
			u32										view_width{ 0 };
			u32										view_height{ 0 };
			f32										camera_fov{ 0.f };
			D3D12_GPU_VIRTUAL_ADDRESS				light_index_list_opaque_buffer{ 0 };
			// NOTE: Initialize has_lights with `true` so that the culling shader
			//	     is run at least once in order the clear the buffer
			bool									has_lights{ true };
		};

		struct LightCuller {
			CullingParameters cullers[FrameBufferCount]{};
		};

		constexpr u32 max_lights_per_tile{ 256 };

		ID3D12RootSignature* light_culling_root_signature{ nullptr };
		ID3D12PipelineState* grid_frustum_pso{ nullptr };
		ID3D12PipelineState* light_culling_pso{ nullptr };
		util::FreeList<LightCuller> light_cullers;

		bool CreateRootSignature() {
			assert(!light_culling_root_signature);

			using param = LightCullingRootParameter;
			D3DX::D3D12RootParameter params[param::count]{};
			params[param::GlobalShaderData].AsCBV(D3D12_SHADER_VISIBILITY_ALL, 0);
			params[param::Constants].AsCBV(D3D12_SHADER_VISIBILITY_ALL, 1);
			params[param::FrustumsOutOrIndexCounter].AsUAV(D3D12_SHADER_VISIBILITY_ALL, 0);
			params[param::FrustumsIn].AsSRV(D3D12_SHADER_VISIBILITY_ALL, 0);
			params[param::CullingInfo].AsSRV(D3D12_SHADER_VISIBILITY_ALL, 1);
			params[param::BoundingSpheres].AsSRV(D3D12_SHADER_VISIBILITY_ALL, 2);
			params[param::LightGridOpaque].AsUAV(D3D12_SHADER_VISIBILITY_ALL, 1);
			params[param::LightIndexListOpaque].AsUAV(D3D12_SHADER_VISIBILITY_ALL, 3);

			light_culling_root_signature = D3DX::D3D12RootSignatureDesc{ &params[0], _countof(params) }.Create();
			NAME_D3D12_OBJECT(light_culling_root_signature, L"Light Culling Root Signature");
			return light_culling_root_signature != nullptr;
		}

		bool CreatePSOs() {
			{
				assert(!grid_frustum_pso);
				struct {
					D3DX::D3D12PipelineStateSubobjectRootSignature root_signature{ light_culling_root_signature };
					D3DX::D3D12PipelineStateSubobjectCS cs{ Shaders::GetEngineShader(Shaders::EngineShader::GridFrustumCS) };
				} stream;
				DXCall(D3DX::CreatePipelineState(&stream, sizeof(stream), &grid_frustum_pso));
				NAME_D3D12_OBJECT(grid_frustum_pso, L"Grid Frustum PSO");
			}
			{
				assert(!light_culling_pso);
				struct {
					D3DX::D3D12PipelineStateSubobjectRootSignature root_signature{ light_culling_root_signature };
					D3DX::D3D12PipelineStateSubobjectCS cs{ Shaders::GetEngineShader(Shaders::EngineShader::CullLightsCS) };
				} stream;
				DXCall(D3DX::CreatePipelineState(&stream, sizeof(stream), &light_culling_pso));
				NAME_D3D12_OBJECT(light_culling_pso, L"Light Culling PSO");
			}
			return grid_frustum_pso != nullptr && light_culling_pso != nullptr;
		}

		void ResizeBuffers(CullingParameters& culler) {
			const u32 frustum_count{ culler.frustum_count };

			const u32 frustums_buffer_size{ sizeof(HLSL::Frustum) * frustum_count };
			const u32 light_grid_buffer_size{ (u32)Math::AlignSizeUp<sizeof(Math::v4)>(sizeof(Math::u32v2) * frustum_count) };
			const u32 light_index_list_buffer_size{ (u32)Math::AlignSizeUp<sizeof(Math::v4)>(sizeof(u32) * max_lights_per_tile * frustum_count) };
			const u32 light_grid_and_index_list_buffer_size{ light_grid_buffer_size + light_index_list_buffer_size };

			D3D12BufferInitInfo info{};
			info.alignment = sizeof(Math::v4);
			info.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			if (frustums_buffer_size > culler.frustums.Size()) {
				info.size = frustums_buffer_size;
				culler.frustums = D3D12Buffer{ info, false };
				NAME_D3D12_OBJECT_INDEXED(culler.frustums.Buffer(), frustum_count, L"Light Grid Frustums Buffer - count");
			}

			if (light_grid_and_index_list_buffer_size > culler.light_grid_and_index_list.Size()) {
				info.size = light_grid_and_index_list_buffer_size;
				culler.light_grid_and_index_list = D3D12Buffer{ info, false };

				const D3D12_GPU_VIRTUAL_ADDRESS light_grid_opaque_buffer{ culler.light_grid_and_index_list.GPU_Address() };
				culler.light_index_list_opaque_buffer = light_grid_opaque_buffer + light_grid_buffer_size;
				NAME_D3D12_OBJECT_INDEXED(culler.light_grid_and_index_list.Buffer(), light_grid_and_index_list_buffer_size,
					L"Light Grid and Index List Buffer - size");

				if (!culler.light_index_counter.Buffer()) {
					info = UAV_ClearableBuffer::GetDefaultInitInfo(1);
					culler.light_index_counter = UAV_ClearableBuffer{ info };
					NAME_D3D12_OBJECT_INDEXED(culler.light_index_counter.Buffer(), Core::CurrentFrameIndex(), L"Light Index Counter Buffer");
				}
			}
		}

		void Resize(CullingParameters& culler) {
			constexpr u32 tile_size{ light_culling_tile_size };
			assert(culler.view_width >= tile_size && culler.view_height >= tile_size);
			const Math::u32v2 tile_count{
				(u32)Math::AlignSizeUp<tile_size>(culler.view_width) / tile_size,
				(u32)Math::AlignSizeUp<tile_size>(culler.view_height) / tile_size
			};
			culler.frustum_count = tile_count.x * tile_count.y;

			// Grid frustum dispatch params 
			{
				HLSL::LightCullingDispatchParameters& params{ culler.grid_frustums_dispatch_params };
				params.NumThreads = tile_count;
				params.NumThreadGroups.x = (u32)Math::AlignSizeUp<tile_size>(tile_count.x) / tile_size;
				params.NumThreadGroups.y = (u32)Math::AlignSizeUp<tile_size>(tile_count.y) / tile_size;
			}
			// Light culling dispatch params
			{
				HLSL::LightCullingDispatchParameters& params{ culler.light_culling_dispatch_params };
				params.NumThreads.x = tile_count.x * tile_size;
				params.NumThreads.y = tile_count.y * tile_size;
				params.NumThreadGroups = tile_count;
			}

			ResizeBuffers(culler);
		}

		void CalculateGridFrustums(
								CullingParameters& culler, ID3D12GraphicsCommandList* const cmd_list,
								const D3D12FrameInfo& d3d12_info, D3DX::D3D12ResourceBarrier& barriers) {
			ConstantBuffer& cbuffer{ Core::CBuffer() };
			HLSL::LightCullingDispatchParameters* const buffer{ cbuffer.alloc<HLSL::LightCullingDispatchParameters>() };
			const HLSL::LightCullingDispatchParameters& params{ culler.grid_frustums_dispatch_params };
			memcpy(buffer, &params, sizeof(HLSL::LightCullingDispatchParameters));

			// Make frustums buffer writable
			barriers.Add(culler.frustums.Buffer(), 
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			barriers.Apply(cmd_list);

			using param = LightCullingRootParameter;
			cmd_list->SetComputeRootSignature(light_culling_root_signature);
			cmd_list->SetPipelineState(grid_frustum_pso);
			cmd_list->SetComputeRootConstantBufferView(param::GlobalShaderData, d3d12_info.global_shader_data);
			cmd_list->SetComputeRootConstantBufferView(param::Constants, cbuffer.GPU_Address(buffer));
			cmd_list->SetComputeRootUnorderedAccessView(param::FrustumsOutOrIndexCounter, culler.frustums.GPU_Address());
			cmd_list->Dispatch(params.NumThreadGroups.x, params.NumThreadGroups.y, 1);

			// Make frustums buffer readonly
			// NOTE: CullLights() will apply this transition
			barriers.Add(culler.frustums.Buffer(), 
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		void _declspec(noinline) ResizeAndCalculateGridFrustums(
								CullingParameters& culler, ID3D12GraphicsCommandList* const cmd_list, 
								const D3D12FrameInfo& d3d12_info, D3DX::D3D12ResourceBarrier& barriers) {
			culler.camera_fov = d3d12_info.camera->FOV();
			culler.view_width = d3d12_info.surface_width;
			culler.view_height = d3d12_info.surface_height;

			Resize(culler);
			CalculateGridFrustums(culler, cmd_list, d3d12_info, barriers);
		}
	}

	bool Initialize() {
		return CreateRootSignature() && CreatePSOs() && Light::Initialize();
	}

	void Shutdown() {
		Light::Shutdown();
		assert(light_culling_root_signature && grid_frustum_pso && light_culling_pso);
		Core::DeferredRelease(light_culling_root_signature);
		Core::DeferredRelease(grid_frustum_pso);
		Core::DeferredRelease(light_culling_pso);
	}

	ID::ID_Type AddCuller() {
		return light_cullers.Add();
	}

	void RemoveCuller(ID::ID_Type id) {
		assert(ID::IsValid(id));
		light_cullers.Remove(id);
	}

	void CullLights(ID3D12GraphicsCommandList* const cmd_list, const D3D12FrameInfo& d3d12_info, D3DX::D3D12ResourceBarrier& barriers) {
		const ID::ID_Type id{ d3d12_info.light_culling_id };
		assert(ID::IsValid(id));
		CullingParameters& culler{ light_cullers[id].cullers[d3d12_info.frame_index] };

		if (d3d12_info.surface_width != culler.view_width || d3d12_info.surface_height != culler.view_height || !Math::IsEqual(d3d12_info.camera->FOV(), culler.camera_fov)) {
			ResizeAndCalculateGridFrustums(culler, cmd_list, d3d12_info, barriers);
		}

		HLSL::LightCullingDispatchParameters& params{ culler.light_culling_dispatch_params };
		params.NumLights = Light::CullableLightCount(d3d12_info.info->light_set_key);
		params.DepthBufferSRV_Index = GPass::DepthBuffer().SRV().index;

		//NOTE: culler.has_lights is updated after this statement so the light culling shader
		//		will run once to clear the buffers when there's no lgiths.
		if (!params.NumLights && !culler.has_lights) return;

		culler.has_lights = params.NumLights > 0;

		ConstantBuffer& cbuffer{ Core::CBuffer() };
		HLSL::LightCullingDispatchParameters* const buffer{ cbuffer.alloc<HLSL::LightCullingDispatchParameters>() };
		memcpy(buffer, &params, sizeof(HLSL::LightCullingDispatchParameters));

		// Make light grid and index buffers writable
		barriers.Add(culler.light_grid_and_index_list.Buffer(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		barriers.Apply(cmd_list);

		const Math::u32v4 clear_value{ 0, 0, 0, 0 };
		culler.light_index_counter.ClearUAV(cmd_list, &clear_value.x);

		cmd_list->SetComputeRootSignature(light_culling_root_signature);
		cmd_list->SetPipelineState(light_culling_pso);
		using param = LightCullingRootParameter;
		cmd_list->SetComputeRootConstantBufferView(param::GlobalShaderData, d3d12_info.global_shader_data);
		cmd_list->SetComputeRootConstantBufferView(param::Constants, cbuffer.GPU_Address(buffer));
		cmd_list->SetComputeRootUnorderedAccessView(param::FrustumsOutOrIndexCounter, culler.light_index_counter.GPU_Address());
		cmd_list->SetComputeRootShaderResourceView(param::FrustumsIn, culler.frustums.GPU_Address());
		cmd_list->SetComputeRootShaderResourceView(param::CullingInfo, Light::CullingInfoBuffer(d3d12_info.frame_index));
		cmd_list->SetComputeRootShaderResourceView(param::BoundingSpheres, Light::BoundingSphereBuffer(d3d12_info.frame_index));
		cmd_list->SetComputeRootUnorderedAccessView(param::LightGridOpaque, culler.light_grid_and_index_list.GPU_Address());
		cmd_list->SetComputeRootUnorderedAccessView(param::LightIndexListOpaque, culler.light_index_list_opaque_buffer);

		cmd_list->Dispatch(params.NumThreadGroups.x, params.NumThreadGroups.y, 1);

		// Make light grid and index buffers writable
		// NOTE: This transition will be applied by the caller of the function.
		barriers.Add(culler.light_grid_and_index_list.Buffer(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	//TODO: Temporary for light culling visualization. Remove Later.
	D3D12_GPU_VIRTUAL_ADDRESS Frustums(ID::ID_Type light_culling_id, u32 frame_idx) {
		assert(frame_idx < FrameBufferCount && ID::IsValid(light_culling_id));
		return light_cullers[light_culling_id].cullers[frame_idx].frustums.GPU_Address();
	}

	D3D12_GPU_VIRTUAL_ADDRESS LightGridOpaque(ID::ID_Type light_culling_id, u32 frame_idx) {
		assert(frame_idx < FrameBufferCount && ID::IsValid(light_culling_id));
		return light_cullers[light_culling_id].cullers[frame_idx].light_grid_and_index_list.GPU_Address();
	}

	D3D12_GPU_VIRTUAL_ADDRESS LightIndexListOpaque(ID::ID_Type light_culling_id, u32 frame_idx) {
		assert(frame_idx < FrameBufferCount && ID::IsValid(light_culling_id));
		return light_cullers[light_culling_id].cullers[frame_idx].light_index_list_opaque_buffer;
	}
}