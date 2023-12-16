#include "D3D12Content.h"
#include "D3D12Core.h"
#include "D3D12Helpers.h"
#include "Utilities//IOStream.h"
#include "Content/ContentToEngine.h"
#include "D3D12GPass.h"
#include "D3D12Core.h"

namespace Zetta::Graphics::D3D12::Content {
	namespace {
		struct PSO_ID {
			ID::ID_Type gpass_pso_id{ ID::Invalid_ID };
			ID::ID_Type depth_pso_id{ ID::Invalid_ID };
		};

		struct SubmeshView {
			D3D12_VERTEX_BUFFER_VIEW					position_buffer_view{};
			D3D12_VERTEX_BUFFER_VIEW					element_buffer_view{};
			D3D12_INDEX_BUFFER_VIEW						index_buffer_view{};
			u32											elements_type{};
			D3D_PRIMITIVE_TOPOLOGY						primitive_topology{};
		};

		struct D3D12RenderItem {
			ID::ID_Type entity_id;
			ID::ID_Type submesh_gpu_id;
			ID::ID_Type mat_id;
			ID::ID_Type pso_id;
			ID::ID_Type depth_pso_id;
		};

		util::FreeList<ID3D12Resource*>					submesh_buffers{};
		util::FreeList<SubmeshView>						submesh_views{};
		std::mutex										submesh_mutex{};

		util::FreeList<D3D12Texture>					textures;
		std::mutex										texture_mutex{};

		util::vector<ID3D12RootSignature*>				root_signatures;
		std::unordered_map<u64, ID::ID_Type>			mat_rs_map;
		util::FreeList<std::unique_ptr<u8[]>>			materials;
		std::mutex										material_mutex{};

		util::FreeList<D3D12RenderItem>					render_items;
		util::FreeList<std::unique_ptr<ID::ID_Type[]>>	render_item_ids;
		std::mutex										render_item_mutex{};

		util::vector<ID3D12PipelineState*>				pipeline_states;
		std::unordered_map<u64, ID::ID_Type>			pso_map;
		std::mutex										pso_mutex{};

		struct {
			util::vector<Zetta::Content::LODOffset>		lod_offsets;
			util::vector<ID::ID_Type>					geometry_ids;
		} frame_cache;

		ID::ID_Type CreateRootSignature(MaterialType::Type type, ShaderFlags::Flags flags);

		class D3D12MaterialStream {
		public:
			DISABLE_COPY_AND_MOVE(D3D12MaterialStream);
			explicit D3D12MaterialStream(u8* const material_buffer) 
				: _buffer{ material_buffer } {
				Initialize();
			}

			explicit D3D12MaterialStream(std::unique_ptr<u8[]>& material_buffer, MaterialInitInfo info) {
				assert(!material_buffer);

				u32 shader_count{ 0 };
				u32 flags{ 0 };
				for (u32 i{ 0 }; i < ShaderType::count; i++) {
					if (ID::IsValid(info.shader_ids[i])) {
						shader_count++;
						flags |= (1 << i);
					}
				}

				assert(shader_count && flags);

				const u32 buffer_size{
					sizeof(MaterialType::Type) +
					sizeof(ShaderFlags::Flags) +
					sizeof(ID::ID_Type) +
					sizeof(u32) + 
					sizeof(ID::ID_Type) * shader_count + 
					(sizeof(ID::ID_Type) + sizeof(u32)) * info.texture_count
				};

				material_buffer = std::make_unique<u8[]>(buffer_size);
				_buffer = material_buffer.get();
				u8* const buffer{ _buffer };

				*(MaterialType::Type*)buffer = info.type;
				*(ShaderFlags::Flags*)(&buffer[shader_flags_index]) = (ShaderFlags::Flags)flags;
				*(ID::ID_Type*)(&buffer[root_signature_index]) = CreateRootSignature(info.type, (ShaderFlags::Flags)flags);
				*(u32*)(&buffer[texture_count_index]) = info.texture_count;

				Initialize();

				if (info.texture_count) {
					memcpy(_texture_ids, info.texture_ids, info.texture_count * sizeof(ID::ID_Type));
					Texture::GetDescriptorIndices(_texture_ids, info.texture_count, _descriptor_indices);
				}

				u32 shader_index{ 0 };
				for (u32 i{ 0 }; i < ShaderType::count; i++) {
					if (ID::IsValid(info.shader_ids[i])) {
						_shader_ids[shader_index] = info.shader_ids[i];
						shader_index++;
					}
				}

				assert(shader_index = (u32)_mm_popcnt_u32(_shader_flags));
			}

			[[nodiscard]] constexpr u32 TextureCount() const { return _texture_count; }
			[[nodiscard]] constexpr MaterialType::Type MaterialType() const { return _type; }
			[[nodiscard]] constexpr ShaderFlags::Flags ShaderFlags() const { return _shader_flags; }
			[[nodiscard]] constexpr ID::ID_Type RootSignatureID() const { return _root_signature_id; }
			[[nodiscard]] constexpr ID::ID_Type* TextureIDs() const { return _texture_ids; }
			[[nodiscard]] constexpr u32* DescriptorIndices() const { return _descriptor_indices; }
			[[nodiscard]] constexpr ID::ID_Type* ShaderIDs() const { return _shader_ids; }

		private:
			void Initialize() {
				assert(_buffer);
				u8* const buffer{ _buffer };
				
				_type = *(MaterialType::Type*)buffer;
				_shader_flags = *(ShaderFlags::Flags*)(&buffer[shader_flags_index]);
				_root_signature_id = *(ID::ID_Type*)(&buffer[root_signature_index]);
				_texture_count = *(u32*)(&buffer[texture_count_index]);

				_shader_ids = (ID::ID_Type*)(&buffer[texture_count_index + (sizeof(u32))]);
				_texture_ids = _texture_count ? &_shader_ids[_mm_popcnt_u32(_shader_flags)] : nullptr;
				_descriptor_indices = _texture_count ? (u32*)(&_texture_ids[_texture_count]) : nullptr;
			}

			constexpr static u32 shader_flags_index{ sizeof(MaterialType::Type) };
			constexpr static u32 root_signature_index{ shader_flags_index + sizeof(ShaderFlags::Flags) };
			constexpr static u32 texture_count_index{ root_signature_index + sizeof(ID::ID_Type) };

			u8* _buffer;
			ID::ID_Type* _texture_ids;
			u32* _descriptor_indices;
			ID::ID_Type* _shader_ids;
			ID::ID_Type _root_signature_id;
			u32 _texture_count;
			MaterialType::Type _type;
			ShaderFlags::Flags _shader_flags;
		};

		constexpr D3D_PRIMITIVE_TOPOLOGY GetD3DPrimitiveTopology(PrimitiveTopology::Type type) {
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

		constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE GetD3D12PrimitiveTopologyType(D3D_PRIMITIVE_TOPOLOGY topology) {
			switch (topology)
			{
			case D3D_PRIMITIVE_TOPOLOGY_POINTLIST: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			case D3D_PRIMITIVE_TOPOLOGY_LINELIST: 
			case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST: 
			case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			}

			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		}

		constexpr D3D12_ROOT_SIGNATURE_FLAGS GetRootSignatureFlags(ShaderFlags::Flags flags) {
			D3D12_ROOT_SIGNATURE_FLAGS default_flags{ D3DX::D3D12RootSignatureDesc::default_flags };

			if (flags & ShaderFlags::Vertex) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::Hull) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::Domain) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::Geometry) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::Pixel) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::Amplification) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::Mesh) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

			return default_flags;
		}

		ID::ID_Type CreateRootSignature(MaterialType::Type type, ShaderFlags::Flags flags) {
			assert(type < MaterialType::count);
			static_assert(sizeof(type) == sizeof(u32) && sizeof(flags) == sizeof(u32));
			const u64 key{ ((u64)type << 32) | flags };
			auto pair = mat_rs_map.find(key);
			if (pair != mat_rs_map.end()) {
				assert(pair->first == key);
				return pair->second;
			}

			ID3D12RootSignature* root_sig{ nullptr };

			switch (type)
			{
			case Zetta::Graphics::MaterialType::Opaque: {
				using param = GPass::OpaqueRootParameter;
				D3DX::D3D12RootParameter params[param::count]{};

				D3D12_SHADER_VISIBILITY buffer_visibility{};
				D3D12_SHADER_VISIBILITY data_visibility{};

				if (flags & ShaderFlags::Vertex) {
					buffer_visibility = D3D12_SHADER_VISIBILITY_VERTEX;
					data_visibility = D3D12_SHADER_VISIBILITY_VERTEX;
				}
				else if (flags & ShaderFlags::Mesh) {
					buffer_visibility = D3D12_SHADER_VISIBILITY_MESH;
					data_visibility = D3D12_SHADER_VISIBILITY_MESH;
				}
				if ((flags & ShaderFlags::Hull) || (flags & ShaderFlags::Geometry) ||
					(flags & ShaderFlags::Amplification)) {
					buffer_visibility = D3D12_SHADER_VISIBILITY_ALL;
					data_visibility = D3D12_SHADER_VISIBILITY_ALL;
				}

				if ((flags & ShaderFlags::Pixel) || (flags & ShaderFlags::Compute)) {
					data_visibility = D3D12_SHADER_VISIBILITY_ALL;
				}

				params[param::GlobalShaderData].AsCBV(D3D12_SHADER_VISIBILITY_ALL, 0);
				params[param::PerObjectData].AsCBV(data_visibility, 1);
				params[param::PositionBuffer].AsSRV(buffer_visibility, 0);
				params[param::ElementBuffer].AsSRV(buffer_visibility, 1);
				params[param::SRV_Indices].AsSRV(D3D12_SHADER_VISIBILITY_PIXEL, 2); // TODO: needs to be visible to any stages that need to sample textures.
				params[param::DirectionLights].AsSRV(D3D12_SHADER_VISIBILITY_PIXEL, 3);
				params[param::CullableLights].AsSRV(D3D12_SHADER_VISIBILITY_PIXEL, 4);
				params[param::LightGrid].AsSRV(D3D12_SHADER_VISIBILITY_PIXEL, 5);
				params[param::LightIndexList].AsSRV(D3D12_SHADER_VISIBILITY_PIXEL, 6);

				root_sig = D3DX::D3D12RootSignatureDesc{ &params[0], _countof(params), GetRootSignatureFlags(flags) }.Create();

			} break;
			}

			assert(root_sig);
			const ID::ID_Type id{ (ID::ID_Type)root_signatures.size() };
			root_signatures.emplace_back(root_sig);
			mat_rs_map[key] = id;
			NAME_D3D12_OBJECT_INDEXED(root_sig, key, L"GPass Root Signature - key");

			return id;
		}

		ID::ID_Type CreatePSO_IF_NEEDED(const u8* const stream_ptr, u64 aligned_stream_size, [[maybe_unused]] bool is_depth ) {
			const u64 key{ Math::calc_crc32_u64(stream_ptr, aligned_stream_size) };
			{	// Lock scope to check if PSO already exists
				std::lock_guard lock{ pso_mutex };
				auto pair = pso_map.find(key);

				if (pair != pso_map.end()) {
					assert(pair->first == key);
					return pair->second;
				}
			}

			// Creating a new PSO is a lock-free operation
			D3DX::D3D12PipelineStateSubobjectStream* const stream{ (D3DX::D3D12PipelineStateSubobjectStream* const)stream_ptr };
			ID3D12PipelineState* pso{ D3DX::CreatePipelineState(stream, sizeof(D3DX::D3D12PipelineStateSubobjectStream)) };

			{	// Lock scope to add the new PSO pointer and id. Scoping unneedd, but added for clarity
				std::lock_guard lock{ pso_mutex };
				const ID::ID_Type id{ (u32)pipeline_states.size() };
				pipeline_states.emplace_back(pso);
				NAME_D3D12_OBJECT_INDEXED(pipeline_states.back(), key,
					is_depth ? L"Depth-only Pipeline State Object - key" : L"GPass Pipeline State Object - key");

				pso_map[key] = id;
				return id;
			}
		}

#pragma intrinsic(_BitScanForward)
		ShaderType::Type GetShaderType(u32 flag) {
			assert(flag);
			unsigned long index;
			_BitScanForward(&index, flag);
			return (ShaderType::Type)index;
		}

		PSO_ID CreatePSO_ID(ID::ID_Type mat_id, D3D12_PRIMITIVE_TOPOLOGY topology, u32 elements_type) {
			constexpr u64 aligned_stream_size{ Math::AlignSizeUp<sizeof(u64)>(sizeof(D3DX::D3D12PipelineStateSubobjectStream)) };
			u8* const stream_ptr{ (u8* const)alloca(aligned_stream_size) };
			ZeroMemory(stream_ptr, aligned_stream_size);
			new (stream_ptr) D3DX::D3D12PipelineStateSubobjectStream{};

			D3DX::D3D12PipelineStateSubobjectStream& stream{ *(D3DX::D3D12PipelineStateSubobjectStream* const)stream_ptr };

			{	// Lock materials
				std::lock_guard lock{ material_mutex };
				const D3D12MaterialStream mat{ materials[mat_id].get() };

				D3D12_RT_FORMAT_ARRAY rt_array{};
				rt_array.NumRenderTargets = 1;
				rt_array.RTFormats[0] = GPass::main_buffer_format;

				stream.RenderTargetFormats = rt_array;
				stream.RootSignature = root_signatures[mat.RootSignatureID()];
				stream.PrimitiveTopology = GetD3D12PrimitiveTopologyType(topology);
				stream.DepthStencilFormats = GPass::depth_buffer_format;
				stream.Rasterizer = D3DX::RasterizerState.backface_cull;
				stream.DepthStencil1 = D3DX::DepthState.reversed_readonly;
				stream.Blend = D3DX::BlendState.disabled;

				const ShaderFlags::Flags flags{ mat.ShaderFlags() };
				D3D12_SHADER_BYTECODE shaders[ShaderType::count]{};
				u32 shader_index{ 0 };
				for (u32 i{ 0 }; i < ShaderType::count; i++) {
					if (flags & (1 << i)) {
						const u32 key{ GetShaderType(flags & (1 << i)) == ShaderType::Vertex ? elements_type : u32_invalid_id };
						Zetta::Content::pCompiledShader shader{ Zetta::Content::GetShader(mat.ShaderIDs()[shader_index], key) };
						assert(shader);
						shaders[i].pShaderBytecode = shader->ByteCode();
						shaders[i].BytecodeLength = shader->ByteCodeSize();
						shader_index++;
					}
				}

				stream.VS = shaders[ShaderType::Vertex];
				stream.PS = shaders[ShaderType::Pixel];
				stream.DS = shaders[ShaderType::Domain];
				stream.HS = shaders[ShaderType::Hull];
				stream.GS = shaders[ShaderType::Geometry];
				stream.CS = shaders[ShaderType::Compute];
				stream.AS = shaders[ShaderType::Amplification];
				stream.MS = shaders[ShaderType::Mesh];
			}

			PSO_ID id_pair{};
			id_pair.gpass_pso_id = CreatePSO_IF_NEEDED(stream_ptr, aligned_stream_size, false);

			stream.PS = D3D12_SHADER_BYTECODE{};
			stream.DepthStencil1 = D3DX::DepthState.reversed;
			id_pair.depth_pso_id = CreatePSO_IF_NEEDED(stream_ptr, aligned_stream_size, true);

			return id_pair;
		}
	}
	
	bool Initialize() {
		return true;
	}

	void Shutdown() {
		for (auto& sig : root_signatures)
			Core::Release(sig);

		mat_rs_map.clear();
		root_signatures.clear();

		for (auto& pso : pipeline_states)
			Core::Release(pso);

		pso_map.clear();
		pipeline_states.clear();
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

		void GetViews(const ID::ID_Type* const gpu_ids, u32 id_count, const ViewsCache& cache) {
			assert(gpu_ids&& id_count);
			assert(cache.position_buffers && cache.element_buffers && cache.index_buffer_views &&
				cache.primitive_topologies && cache.element_types);

			std::lock_guard lock{ submesh_mutex };
			for (u32 i{ 0 }; i < id_count; i++) {
				const SubmeshView& view{ submesh_views[gpu_ids[i]] };
				cache.position_buffers[i] = view.position_buffer_view.BufferLocation;
				cache.element_buffers[i] = view.element_buffer_view.BufferLocation;
				cache.index_buffer_views[i] = view.index_buffer_view;
				cache.primitive_topologies[i] = view.primitive_topology;
				cache.element_types[i] = view.elements_type;
			}
		}
	}

	namespace Texture {
		void GetDescriptorIndices(const ID::ID_Type* const texture_ids, u32 id_count, u32* const indices) {
			assert(texture_ids && id_count && indices);
			std::lock_guard lock{ texture_mutex };
			for (u32 i{ 0 }; i < id_count; i++)
				indices[i] = textures[i].SRV().index;
		}
	}

	namespace Material {
		void GetMaterials(const ID::ID_Type* const mat_ids, u32 mat_count, const MaterialsCache& cache) {
			assert(mat_ids && mat_count);
			assert(cache.root_signatures && cache.material_types);
			std::lock_guard lock{ material_mutex };

			for (u32 i{ 0 }; i < mat_count; i++) {
				const D3D12MaterialStream stream{ materials[mat_ids[i]].get() };
				cache.root_signatures[i] = root_signatures[stream.RootSignatureID()];
				cache.material_types[i] = stream.MaterialType();
			}
		}

		//
		// Output format:
		// 
		// struct {
		//     MaterialType::Type type,
		//     ShaderFlags::flags flags,
		//     ID::ID_Type root_signature_id,
		//     u32 texture_count,
		//     ID::ID_Type shader_ids[shader_count],
		//     ID::ID_Type texture_ids[texture_count],
		//     u32* descriptor_indices[texture_count],
		// } D3D12Material;
		//
		ID::ID_Type Add(const MaterialInitInfo info) {
			std::unique_ptr<u8[]> buffer;
			std::lock_guard lock{ material_mutex };

			D3D12MaterialStream stream{ buffer, info };

			assert(buffer);
			return materials.Add(std::move(buffer));
		}

		void Remove(ID::ID_Type id) {
			std::lock_guard lock{ material_mutex };
			materials.Remove(id);
		}
	}

	namespace RenderItem {

		// Creates an array of ID::ID_Types as a buffer
		// buffer[0] = geometry_content_id
		// buffer[1 .. n] = render_item_ids (n is the number of low-level render item ids which must also equal the number of submesh/material ids)
		// buffer[n+1] = ID::Invalid_ID (marks the end of the array)
		//
		ID::ID_Type Add(ID::ID_Type entity_id, ID::ID_Type geometry_content_id, u32 mat_count, const ID::ID_Type* const mat_ids) {
			assert(ID::IsValid(entity_id) && ID::IsValid(geometry_content_id));
			assert(mat_count && mat_ids);

			ID::ID_Type* const gpu_ids{ (ID::ID_Type* const)alloca(mat_count * sizeof(ID::ID_Type)) };
			Zetta::Content::GetSubmeshGPU_IDs(geometry_content_id, mat_count, gpu_ids);

			Submesh::ViewsCache views_cache{
				(D3D12_GPU_VIRTUAL_ADDRESS* const)alloca(mat_count * sizeof(D3D12_GPU_VIRTUAL_ADDRESS)),
				(D3D12_GPU_VIRTUAL_ADDRESS* const)alloca(mat_count * sizeof(D3D12_GPU_VIRTUAL_ADDRESS)),
				(D3D12_INDEX_BUFFER_VIEW* const)alloca(mat_count * sizeof(D3D12_INDEX_BUFFER_VIEW)),
				(D3D_PRIMITIVE_TOPOLOGY* const)alloca(mat_count * sizeof(D3D_PRIMITIVE_TOPOLOGY)),
				(u32* const)alloca(mat_count * sizeof(u32)),
			};

			Submesh::GetViews(gpu_ids, mat_count, views_cache);

			// NOTE: The list of IDs starts with geometry id and ends with an invalid id to mark the end of the list.
			std::unique_ptr<ID::ID_Type[]> items{ std::make_unique<ID::ID_Type[]>(sizeof(ID::ID_Type) * (1 + (u64)mat_count + 1)) };

			items[0] = geometry_content_id;
			ID::ID_Type* const item_ids{ &items[1] };

			std::lock_guard lock{ render_item_mutex };
			for (u32 i{ 0 }; i < mat_count; i++) {
				D3D12RenderItem item{};
				item.entity_id = entity_id;
				item.submesh_gpu_id = gpu_ids[i];
				item.mat_id = mat_ids[i];
				PSO_ID id_pair{ CreatePSO_ID(item.mat_id, views_cache.primitive_topologies[i], views_cache.element_types[i]) };
				item.pso_id = id_pair.gpass_pso_id;
				item.depth_pso_id = id_pair.depth_pso_id;

				assert(ID::IsValid(item.submesh_gpu_id) && ID::IsValid(item.mat_id));
				item_ids[i] = render_items.Add(item);
			}
			item_ids[mat_count] = ID::Invalid_ID;

			return render_item_ids.Add(std::move(items));
		}

		void Remove(ID::ID_Type id) {
			std::lock_guard lock{ render_item_mutex };
			const ID::ID_Type* const item_ids{ &render_item_ids[id][1] };

			for (u32 i{ 0 }; item_ids[i] != ID::Invalid_ID; i++)
				render_items.Remove(item_ids[i]);

			render_item_ids.Remove(id);
		}

		void GetD3D12RenderItemsIDs(const FrameInfo& info, util::vector<ID::ID_Type>& d3d12_render_item_ids) {
			assert(info.render_item_ids && info.thresholds && info.render_item_count);
			assert(d3d12_render_item_ids.empty());

			frame_cache.lod_offsets.clear();
			frame_cache.geometry_ids.clear();
			const u32 count{ info.render_item_count };

			std::lock_guard lock{ render_item_mutex };

			for (u32 i{ 0 }; i < count; i++) {
				const ID::ID_Type* const buffer{ render_item_ids[info.render_item_ids[i]].get() };
				frame_cache.geometry_ids.emplace_back(buffer[0]);
			}
			
			Zetta::Content::GetLODOffsets(frame_cache.geometry_ids.data(), info.thresholds, count, frame_cache.lod_offsets);
			assert(frame_cache.lod_offsets.size() == count);

			u32 d3d12_render_item_count{ 0 };
			for (u32 i{ 0 }; i < count; i++) 
				d3d12_render_item_count += frame_cache.lod_offsets[i].count;
			
			assert(d3d12_render_item_count);
			d3d12_render_item_ids.resize(d3d12_render_item_count);

			u32 item_index{ 0 };
			for (u32 i{ 0 }; i < count; i++) {
				const ID::ID_Type* const item_ids{ &render_item_ids[info.render_item_ids[i]][1] };
				const Zetta::Content::LODOffset& lod_offset{ frame_cache.lod_offsets[i] };
				memcpy(&d3d12_render_item_ids[item_index], &item_ids[lod_offset.offset], sizeof(ID::ID_Type) * lod_offset.count);
				item_index += lod_offset.count;
				assert(item_index <= d3d12_render_item_count);
			}
			assert(item_index <= d3d12_render_item_count);
		}

		void GetItems(const ID::ID_Type* const d3d12_render_item_ids, u32 id_count, const ItemsCache& cache) {
			assert(d3d12_render_item_ids && id_count);
			assert(cache.entity_ids && cache.submesh_gpu_ids && cache.mat_ids &&
				cache.psos && cache.depth_psos);

			std::lock_guard lock1{ render_item_mutex };
			std::lock_guard lock2{ pso_mutex };

			for (u32 i{ 0 }; i < id_count; i++) {
				const D3D12RenderItem& item{ render_items[d3d12_render_item_ids[i]] };
				cache.entity_ids[i] = item.entity_id;
				cache.submesh_gpu_ids[i] = item.submesh_gpu_id;
				cache.mat_ids[i] = item.mat_id;
				cache.psos[i] = pipeline_states[item.pso_id];
				cache.depth_psos[i] = pipeline_states[item.depth_pso_id];
			}
		}
	}
}