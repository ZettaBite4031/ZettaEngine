#include "D3D12Content.h"
#include "D3D12Core.h"
#include "D3D12Helpers.h"
#include "Utilities//IOStream.h"
#include "Content/ContentToEngine.h"

namespace Zetta::Graphics::D3D12::Content {
	namespace {
		struct SubmeshView {
			D3D12_VERTEX_BUFFER_VIEW position_buffer_view{};
			D3D12_VERTEX_BUFFER_VIEW element_buffer_view{};
			D3D12_INDEX_BUFFER_VIEW index_buffer_view{};
			u32 elements_type{};
			D3D_PRIMITIVE_TOPOLOGY primitive_topology{};
		};

		util::FreeList<ID3D12Resource*> submesh_buffers{};
		util::FreeList<SubmeshView> submesh_views{};
		std::mutex submesh_mutex{};

		D3D_PRIMITIVE_TOPOLOGY GetD3DPrimitiveTopology(PrimitiveTopology::Type type) {
			assert(type < PrimitiveTopology::count);

			switch (type)
			{
			case PrimitiveTopology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			case PrimitiveTopology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case PrimitiveTopology::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case PrimitiveTopology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case PrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			}

			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}
	
	namespace Submesh {
		ID::ID_Type Add(const u8*& data) {
			util::BlobStreamReader blob{ (const u8*)data };
			const u32 element_size{ blob.read<u32>() };
			const u32 vertex_count{ blob.read<u32>() };
			const u32 index_count{ blob.read<u32>() };
			const u32 elements_type{ blob.read<u32>() };
			const u32 primitive_topology{ blob.read<u32>() };
			const u32 index_size{ (vertex_count < (1 << 16)) ? sizeof(u16) : sizeof(u32) };

			const u32 position_buffer_size{ sizeof(Math::v3) * vertex_count };
			const u32 element_buffer_size{ element_size * vertex_count };
			const u32 index_buffer_size{ index_size * index_count };

			constexpr u32 alignment{ D3D12_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE };
			const u32 aligned_position_buffer_size{ (u32)Math::AlignSizeUp<alignment>(position_buffer_size) };
			const u32 aligned_element_buffer_size{ (u32)Math::AlignSizeUp<alignment>(element_buffer_size) };
			const u32 total_buffer_size{ aligned_position_buffer_size + aligned_element_buffer_size + index_buffer_size };

			ID3D12Resource* resource{ D3DX::CreateBuffer(blob.Position(), total_buffer_size) };

			blob.skip(total_buffer_size);
			data = blob.Position();

			SubmeshView view{};
			view.position_buffer_view.BufferLocation = resource->GetGPUVirtualAddress();
			view.position_buffer_view.SizeInBytes = position_buffer_size;
			view.position_buffer_view.StrideInBytes = sizeof(Math::v3);

			if (element_size) {
				view.element_buffer_view.BufferLocation = resource->GetGPUVirtualAddress() + aligned_position_buffer_size;
				view.element_buffer_view.SizeInBytes = element_buffer_size;
				view.element_buffer_view.StrideInBytes = sizeof(Math::v3);
			}

			view.index_buffer_view.BufferLocation = resource->GetGPUVirtualAddress() + aligned_position_buffer_size + aligned_element_buffer_size;
			view.index_buffer_view.SizeInBytes = index_buffer_size;
			view.index_buffer_view.Format = (index_size == sizeof(u16)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

			view.elements_type = elements_type;
			view.primitive_topology = GetD3DPrimitiveTopology((PrimitiveTopology::Type)primitive_topology);

			std::lock_guard lock{ submesh_mutex };
			submesh_buffers.Add(resource);
			return submesh_views.Add(view);
		}

		void Remove(ID::ID_Type id) {
			std::lock_guard lock{ submesh_mutex };
			submesh_views.Remove(id);

			Core::DeferredRelease(submesh_buffers[id]);
			submesh_buffers.Remove(id);
		}
	}
}