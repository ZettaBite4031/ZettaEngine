#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12::Content {

	bool Initialize();
	void Shutdown();

	namespace Submesh{
		struct ViewsCache {
			D3D12_GPU_VIRTUAL_ADDRESS* const	position_buffers;
			D3D12_GPU_VIRTUAL_ADDRESS* const	element_buffers;
			D3D12_INDEX_BUFFER_VIEW* const		index_buffer_views;
			D3D_PRIMITIVE_TOPOLOGY* const		primitive_topologies;
			u32* const							element_types;
		};

		ID::ID_Type Add(const u8*& data);
		void Remove(ID::ID_Type id);

		void GetViews(const ID::ID_Type* const gpu_ids, u32 id_count, const ViewsCache& cache);
	}

	namespace Texture {
		ID::ID_Type Add(const u8* const);
		void Remove(ID::ID_Type);
		void GetDescriptorIndices(const ID::ID_Type* const texture_ids, u32 id_count, u32* const indices);
	}

	namespace Material {
		struct MaterialsCache {
			ID3D12RootSignature** const root_signatures;
			MaterialType::Type* const material_types;
		};

		ID::ID_Type Add(const MaterialInitInfo info);
		void Remove(ID::ID_Type id);
		void GetMaterials(const ID::ID_Type* const mat_ids, u32 mat_count, const MaterialsCache& cache);
	}

	namespace RenderItem {
		struct ItemsCache {
			ID::ID_Type* const entity_ids;
			ID::ID_Type* const submesh_gpu_ids;
			ID::ID_Type* const mat_ids;
			ID3D12PipelineState** const psos;
			ID3D12PipelineState** const depth_psos;
		};

		ID::ID_Type Add(ID::ID_Type entity_id, ID::ID_Type geometry_content_id, u32 mat_count, const ID::ID_Type* const mat_ids);
		void Remove(ID::ID_Type id);
		void GetD3D12RenderItemsIDs(const FrameInfo& info, util::vector<ID::ID_Type>& render_item_ids);
		void GetItems(const ID::ID_Type* const d3d12_render_item_ids, u32 id_count, const ItemsCache& cache);
	}
}