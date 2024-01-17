#include "D3D12GPass.h"
#include "D3D12Core.h"
#include "D3D12Shaders.h"
#include "D3D12Camera.h"
#include "D3D12Content.h"
#include "D3D12Light.h"
#include "D3D12LightCulling.h"

#include "Shaders/SharedTypes.h"
#include "Components/Entity.h"
#include "Components/Transform.h"

namespace Zetta::Graphics::D3D12::GPass {
	namespace {		
		constexpr Math::u32v2 initial_dimensions{ 100, 100 };
		D3D12RenderTexture gpass_main_buffer{};
		D3D12DepthBuffer gpass_depth_buffer{};
		Math::u32v2 dimensions{ initial_dimensions };

#if _DEBUG
		constexpr f32 clear_value[4]{ 0.5f, 0.5f, 0.5f, 1.0f };
#else
		constexpr f32 clear_value[4]{};
#endif

		struct GPassCache {
			util::vector<ID::ID_Type> d3d12_render_item_ids;

			// NOTE: When adding new arrays, update resize and struct size
			ID::ID_Type*				entity_ids{ nullptr };
			ID::ID_Type*				submesh_ids{ nullptr };
			ID::ID_Type*				material_ids{ nullptr };
			ID3D12PipelineState**		gpass_pipeline_states{ nullptr };
			ID3D12PipelineState**		depth_pipeline_states{ nullptr };
			ID3D12RootSignature**		root_signatures{ nullptr };
			MaterialType::Type*			material_types{ nullptr };
			D3D12_GPU_VIRTUAL_ADDRESS*	position_buffers{ nullptr };
			D3D12_GPU_VIRTUAL_ADDRESS*	element_buffers{ nullptr };
			D3D12_INDEX_BUFFER_VIEW*	index_buffer_views{ nullptr };
			D3D_PRIMITIVE_TOPOLOGY*		primitive_topologies{ nullptr };
			u32*						elements_types{ nullptr };
			D3D12_GPU_VIRTUAL_ADDRESS*	per_object_data{ nullptr };
			
			constexpr Content::RenderItem::ItemsCache ItemsCache() const {
				return {
					entity_ids,
					submesh_ids,
					material_ids,
					gpass_pipeline_states,
					depth_pipeline_states
				};
			}

			constexpr Content::Submesh::ViewsCache ViewsCache() const {
				return {
					position_buffers,
					element_buffers,
					index_buffer_views,
					primitive_topologies,
					elements_types
				};
			}

			constexpr Content::Material::MaterialsCache MaterialsCache() const {
				return {
					root_signatures,
					material_types
				};
			}

			constexpr u32 Size() const {
				return (u32)d3d12_render_item_ids.size();
			}

			constexpr void Clear() {
				d3d12_render_item_ids.clear();
			}

			constexpr void resize() {
				const u64 items_count{ d3d12_render_item_ids.size() };
				const u64 new_buffer_size{ items_count * struct_size };
				const u64 old_buffer_size{ _buffer.size() };
				if (new_buffer_size > old_buffer_size)
					_buffer.resize(new_buffer_size);

				if (new_buffer_size != old_buffer_size) {
					entity_ids = (ID::ID_Type*)_buffer.data();
					submesh_ids = (ID::ID_Type*)(&entity_ids[items_count]);
					material_ids = (ID::ID_Type*)(&submesh_ids[items_count]);
					gpass_pipeline_states = (ID3D12PipelineState**)(&material_ids[items_count]);
					depth_pipeline_states = (ID3D12PipelineState**)(&gpass_pipeline_states[items_count]);
					root_signatures = (ID3D12RootSignature**)(&depth_pipeline_states[items_count]);
					material_types = (MaterialType::Type*)(&root_signatures[items_count]);
					position_buffers = (D3D12_GPU_VIRTUAL_ADDRESS*)(&material_types[items_count]);
					element_buffers = (D3D12_GPU_VIRTUAL_ADDRESS*)(&position_buffers[items_count]);
					index_buffer_views = (D3D12_INDEX_BUFFER_VIEW*)(&element_buffers[items_count]);
					primitive_topologies = (D3D_PRIMITIVE_TOPOLOGY*)(&index_buffer_views[items_count]);
					elements_types = (u32*)(&primitive_topologies[items_count]);
					per_object_data = (D3D12_GPU_VIRTUAL_ADDRESS*)(&elements_types[items_count]);
				}
			}

		private:
			constexpr static u32 struct_size{
				sizeof(ID::ID_Type)+					// entity_ids
				sizeof(ID::ID_Type) +					// submesh_ids
				sizeof(ID::ID_Type) +					// material_ids
				sizeof(ID3D12PipelineState*) +			// gpass_pipeline_states
				sizeof(ID3D12PipelineState*) +			// depth_pipeline_states
				sizeof(ID3D12RootSignature*) +			// root_signatures
				sizeof(MaterialType::Type) +			// material_types
				sizeof(D3D12_GPU_VIRTUAL_ADDRESS) +		// position_buffers
				sizeof(D3D12_GPU_VIRTUAL_ADDRESS) +		// element_buffers
				sizeof(D3D12_INDEX_BUFFER_VIEW) +		// index_buffer_views
				sizeof(D3D_PRIMITIVE_TOPOLOGY) +		// primitive_topologies
				sizeof(u32) +							// elements_types
				sizeof(D3D12_GPU_VIRTUAL_ADDRESS)		// per_object_data
			};

			util::vector<u8> _buffer;
		} frame_cache;

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
		}

		void FillPerObjectData(const D3D12FrameInfo& d3d12_info) {
			const GPassCache& cache{ frame_cache };
			const u32 render_items_count{ (u32)cache.Size() };
			ID::ID_Type current_entity_id{ ID::Invalid_ID };
			HLSL::PerObjectData* current_data_ptr{ nullptr };
			ConstantBuffer& cbuffer{ Core::CBuffer() };

			using namespace DirectX;
			for (u32 i{ 0 }; i < render_items_count; i++) {
				if (current_entity_id != cache.entity_ids[i]) {
					current_entity_id = cache.entity_ids[i];
					HLSL::PerObjectData data{};
					Transform::GetTransformMatrices(GameEntity::EntityID{ current_entity_id }, data.World, data.InvWorld);
					XMMATRIX world{ XMLoadFloat4x4(&data.World) };
					XMMATRIX wvp{ XMMatrixMultiply(world, d3d12_info.camera->ViewProjection()) };
					XMStoreFloat4x4(&data.WorldViewProjection, wvp);

					current_data_ptr = cbuffer.alloc<HLSL::PerObjectData>();
					memcpy(current_data_ptr, &data, sizeof(HLSL::PerObjectData));
				}

				assert(current_data_ptr);
				cache.per_object_data[i] = cbuffer.GPU_Address(current_data_ptr);
			}
		}

		void SetRootParameters(ID3D12GraphicsCommandList* const cmd_list, u32 cache_index) {
			GPassCache& cache{ frame_cache };
			assert(cache_index < cache.Size());

			const MaterialType::Type mat_type{ cache.material_types[cache_index] };
			switch (mat_type)
			{
			case MaterialType::Opaque:
			{
				using params = OpaqueRootParameter;
				cmd_list->SetGraphicsRootShaderResourceView(params::PositionBuffer, cache.position_buffers[cache_index]);
				cmd_list->SetGraphicsRootShaderResourceView(params::ElementBuffer, cache.element_buffers[cache_index]);
				cmd_list->SetGraphicsRootConstantBufferView(params::PerObjectData, cache.per_object_data[cache_index]);
			}				
			break;
			}
		}

		void PrepareRenderFrame(const D3D12FrameInfo& d3d12_info) {
			assert(d3d12_info.info && d3d12_info.camera);
			assert(d3d12_info.info->render_item_ids && d3d12_info.info->render_item_count);
			GPassCache& cache{ frame_cache };
			cache.Clear();

			using namespace Content;
			RenderItem::GetD3D12RenderItemsIDs(*d3d12_info.info, cache.d3d12_render_item_ids);
			cache.resize();
			const u32 items_count{ cache.Size()};
			const RenderItem::ItemsCache items_cache{ cache.ItemsCache() };
			RenderItem::GetItems(cache.d3d12_render_item_ids.data(), items_count, items_cache);

			const Submesh::ViewsCache views_cache{ cache.ViewsCache() };
			Submesh::GetViews(items_cache.submesh_gpu_ids, items_count, views_cache);

			const Material::MaterialsCache materials_cache{ cache.MaterialsCache() };
			Material::GetMaterials(items_cache.mat_ids, items_count, materials_cache);

			FillPerObjectData(d3d12_info);
		}
	}

	bool Initialize() {
		return CreateBuffers(initial_dimensions);
	}

	void Shutdown() {
		gpass_main_buffer.Release();
		gpass_depth_buffer.Release();
		dimensions = initial_dimensions;
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

	void DepthPrepass(ID3D12GraphicsCommandList* cmd_list, const D3D12FrameInfo& d3d12_info) {
		PrepareRenderFrame(d3d12_info);

		const GPassCache& cache{ frame_cache };
		const u32 items_count{ cache.Size() };

		ID3D12RootSignature* current_root_signature{ nullptr };
		ID3D12PipelineState* current_pipeline_state{ nullptr };

		for (u32 i{ 0 }; i < items_count; i++) {
			if (current_root_signature != cache.root_signatures[i]) {
				current_root_signature = cache.root_signatures[i];
				cmd_list->SetGraphicsRootSignature(current_root_signature);
				cmd_list->SetGraphicsRootConstantBufferView(OpaqueRootParameter::GlobalShaderData, d3d12_info.global_shader_data);
			}

			if (current_pipeline_state != cache.depth_pipeline_states[i]) {
				current_pipeline_state = cache.depth_pipeline_states[i];
				cmd_list->SetPipelineState(current_pipeline_state);
			}

			SetRootParameters(cmd_list, i);

			const D3D12_INDEX_BUFFER_VIEW& ibv{ cache.index_buffer_views[i] };
			const u32 index_count{ ibv.SizeInBytes >> (ibv.Format == DXGI_FORMAT_R16_UINT ? 1 : 2) };

			cmd_list->IASetIndexBuffer(&ibv);
			cmd_list->IASetPrimitiveTopology(cache.primitive_topologies[i]);
			cmd_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
		}
	}

	void Render(ID3D12GraphicsCommandList* cmd_list, const D3D12FrameInfo& d3d12_info) {
		const GPassCache& cache{ frame_cache };
		const u32 items_count{ cache.Size() };
		const u32 frame_idx{ d3d12_info.frame_index };
		const ID::ID_Type light_culling_id{ d3d12_info.light_culling_id };

		ID3D12RootSignature* current_root_signature{ nullptr };
		ID3D12PipelineState* current_pipeline_state{ nullptr };

		for (u32 i{ 0 }; i < items_count; i++) {
			if (current_root_signature != cache.root_signatures[i]) {
				using idx = OpaqueRootParameter;
				current_root_signature = cache.root_signatures[i];
				cmd_list->SetGraphicsRootSignature(current_root_signature);
				cmd_list->SetGraphicsRootConstantBufferView(idx::GlobalShaderData, d3d12_info.global_shader_data);
				cmd_list->SetGraphicsRootShaderResourceView(idx::DirectionLights, Light::NoncullableLightBuffer(frame_idx));
				cmd_list->SetGraphicsRootShaderResourceView(idx::CullableLights, Light::CullableLightBuffer(frame_idx));
				cmd_list->SetGraphicsRootShaderResourceView(idx::LightGrid, DeLight::LightGridOpaque(light_culling_id, frame_idx));
				cmd_list->SetGraphicsRootShaderResourceView(idx::LightIndexList, DeLight::LightIndexListOpaque(light_culling_id, frame_idx));
			}

			if (current_pipeline_state != cache.gpass_pipeline_states[i]) {
				current_pipeline_state = cache.gpass_pipeline_states[i];
				cmd_list->SetPipelineState(current_pipeline_state);
			}

			SetRootParameters(cmd_list, i);

			const D3D12_INDEX_BUFFER_VIEW& ibv{ cache.index_buffer_views[i] };
			const u32 index_count{ ibv.SizeInBytes >> (ibv.Format == DXGI_FORMAT_R16_UINT ? 1 : 2) };

			cmd_list->IASetIndexBuffer(&ibv);
			cmd_list->IASetPrimitiveTopology(cache.primitive_topologies[i]);
			cmd_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
		}
	}

	void AddDepthPrepassTransitions(D3DX::D3D12ResourceBarrier& barriers) {
		barriers.Add(gpass_main_buffer.Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
		barriers.Add(gpass_depth_buffer.Resource(),
			D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

	void AddGPassTransitions(D3DX::D3D12ResourceBarrier& barriers) {
		barriers.Add(gpass_main_buffer.Resource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
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