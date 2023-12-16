#include "ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "Utilities/IOStream.h"

namespace Zetta::Content {
	namespace {
		class GeometryHierarchyStream {
		public:
			DISABLE_COPY_AND_MOVE(GeometryHierarchyStream);
			GeometryHierarchyStream(u8* const buffer, u32 lods = u32_invalid_id)
				: _buffer{ buffer } {
				assert(buffer && lods);
				if (lods != u32_invalid_id)
					*((u32*)buffer) = lods;
				_lod_count = *((u32*)buffer);
				_thresholds = (f32*)(&buffer[sizeof(u32)]);
				_lod_offsets = (LODOffset*)(&_thresholds[_lod_count]);
				_gpu_ids = (ID::ID_Type*)(&_lod_offsets[_lod_count]);
			}

			void GPU_IDs(u32 lod, ID::ID_Type*& ids, u32& count) {
				assert(lod < _lod_count);
				ids = &_gpu_ids[_lod_offsets[lod].offset];
				count = _lod_offsets[lod].count;
			}

			u32 LODFromThreshold(f32 threshold) {
				assert(threshold > 0);

				if (_lod_count == 1) return 0;

				for (u32 i{ _lod_count - 1 }; i > 0; i--)
					if (_thresholds[i] <= threshold) return i;

				assert(false); // This should NEVER be executed
				return 0;
			}

			[[nodiscard]] constexpr u32 LODCount() const { return _lod_count; }
			[[nodiscard]] constexpr f32* Thresholds() const { return _thresholds; }
			[[nodiscard]] constexpr LODOffset* LODOffsets() const { return _lod_offsets; }
			[[nodiscard]] constexpr ID::ID_Type* GPU_IDs() const { return _gpu_ids; }

		private:
			u8* const _buffer;
			f32* _thresholds;
			LODOffset* _lod_offsets;
			ID::ID_Type* _gpu_ids;
			u32 _lod_count;
		};
		
		// NOTE: Required for maintained compatibility with STL vector.
		struct NoexceptMap {
			std::unordered_map<u32, std::unique_ptr<u8[]>> map;

			NoexceptMap() = default;
			NoexceptMap(const NoexceptMap&) = default;
			NoexceptMap(NoexceptMap&&) noexcept = default;
			NoexceptMap& operator=(const NoexceptMap&) = default;
			NoexceptMap& operator=(NoexceptMap&&) noexcept = default;
		};

		constexpr uintptr_t single_mesh_marker{ (uintptr_t)0x01 };
		util::FreeList<u8*> geometry_hierarchies;
		std::mutex geometry_mutex;

		util::FreeList<NoexceptMap> shader_groups;
		std::mutex shader_mutex;

		u32 GetGeometryHierarchyBufferSize(const void* const data) {
			assert(data);
			util::BlobStreamReader blob{ (const u8*)data };
			const u32 lod_count{ blob.read<u32>() };
			assert(lod_count);
			constexpr u32 su32{ sizeof(u32) };
			// add the size of lod_count, thresholds, and lod_offsets to the size of the hierarchy
			u32 size{ su32 + (sizeof(f32) + sizeof(LODOffset)) * lod_count };

			for (u32 i{ 0 }; i < lod_count; i++) {
				// skip the threshold
				blob.skip(sizeof(f32));
				// add size of gpu_ids (sizeof(ID::ID_Type) * submesh_count)
				size += sizeof(ID::ID_Type) * blob.read<u32>();
				// skip submesh data and go to the next LOD
				blob.skip(blob.read<u32>());
			}

			return size;
		}
	
		ID::ID_Type CreateMeshHierarchy(const void* const data) {
			assert(data);
			const u32 size{ GetGeometryHierarchyBufferSize(data) };
			u8* const hierarchy_buffer{ (u8* const)malloc(size) };

			util::BlobStreamReader blob{ (const u8*)data };
			const u32 lod_count{ blob.read<u32>() };
			assert(lod_count);
			GeometryHierarchyStream stream{ hierarchy_buffer, lod_count };
			u32 submesh_idx{ 0 };
			ID::ID_Type* const gpu_ids{ stream.GPU_IDs() };

			for (u32 i{ 0 }; i < lod_count; i++) {
				stream.Thresholds()[i] = blob.read<f32>();
				const u32 id_count{ blob.read<u32>() };
				assert(id_count < (1 << 16));
				stream.LODOffsets()[i] = { (u16)submesh_idx, (u16)id_count };
				blob.skip(sizeof(u32)); // skip over sizeof(submeshes)
				for (u32 j{ 0 }; j < id_count; j++) {
					const u8* at{ blob.Position() };
					gpu_ids[submesh_idx++] = Graphics::AddSubmesh(at);
					blob.skip((u32)(at - blob.Position()));
					assert(submesh_idx < (1 << 16));
				}
			}

			assert([&]() {
				f32 previous_threshold{ stream.Thresholds()[0] };
				for (u32 i{ 1 }; i < lod_count; i++) {
					f32 threshold_ = stream.Thresholds()[i];
					if (threshold_ <= previous_threshold) return false;
					previous_threshold = stream.Thresholds()[i];
				}
				return true;
				}());

			static_assert(alignof(void*) > 2, "At least one significant bit for single mesh marker is required.");
			std::lock_guard lock{ geometry_mutex };
			return geometry_hierarchies.Add(hierarchy_buffer);
		}

		ID::ID_Type CreateSingleMesh(const void* const data) {
			assert(data);
			util::BlobStreamReader blob{ (const u8*)data };
			blob.skip(sizeof(u32) + sizeof(f32) + sizeof(u32) + sizeof(u32));
			const u8* at{ blob.Position() };
			const ID::ID_Type gpu_id{ Graphics::AddSubmesh(at) };

			static_assert(sizeof(uintptr_t) > sizeof(ID::ID_Type));
			constexpr u8 shift_bits{ (sizeof(uintptr_t) - sizeof(ID::ID_Type)) << 3 };
			u8* const fake_ptr{ (u8* const)((((uintptr_t)gpu_id) << shift_bits) | single_mesh_marker) };
			std::lock_guard lock{ geometry_mutex };
			return geometry_hierarchies.Add(fake_ptr);
		}

		bool IsSingleMesh(const void* const data) {
			assert(data);
			util::BlobStreamReader blob{ (const u8*)data };
			const u32 lod_count{ blob.read<u32>() };
			assert(lod_count);
			if (lod_count > 1) return false;

			blob.skip(sizeof(f32));
			const u32 submesh_count{ blob.read<u32>() };
			assert(submesh_count);
			return submesh_count == 1;
		}

		constexpr ID::ID_Type GPU_IDFromFakePointer(u8* const ptr) {
			assert((uintptr_t)ptr & single_mesh_marker);
			static_assert(sizeof(uintptr_t) > sizeof(ID::ID_Type));
			constexpr u8 shift_bits{ (sizeof(uintptr_t) - sizeof(ID::ID_Type)) << 3 };
			return (((uintptr_t)ptr) >> shift_bits) & (uintptr_t)ID::Invalid_ID;
		}

		//
		// NOTE: Expects 'data' to contain:
		// struct {
		//     u32 lod_count,
		//     struct {
		//         f32 lod_threshold,
		//         u32 submesh_count,
		//         u32 size_of_submeshes,
		//         struct {
		//             u32 element_size, u32 vertex_count,
		//             u32 index_count, u32 elements_type, u32 primitive_toplogy,
		//             u8 positions[sizeof(f32) * 3 vertex_count],
		//			   u8 elements[sizeof(element_size) * vertex_count],
		//             u8 indices[index_size * index_count]
		//         } submeshes[submesh_count]
		//     } mesh_lods[lod_count]
		// } geometry;
		// 
		//	Output Format
		// 
		// if Geometry has more than one LOD or submesh:
		// struct {
		//     u32 lod_count,
		//     f32 tresholds[lod_count],
		//	   struct { 
		//         u16 offset,
		//         u16 count
		//     } lod_offsets[lod_count],
		//     ID::ID_Type gpu_ids[total_number_of_submeshes]
		// } geometry_hierarchies
		// 
		// If geometry has a single LOD and submesh:
		// (gpu_id << 32) | 0x01
		//
		ID::ID_Type CreateGeometryResource(const void* const data) {
			assert(data);
			return IsSingleMesh(data) ? CreateSingleMesh(data) : CreateMeshHierarchy(data);
		}

		void DestroyGeometryResource(ID::ID_Type id) {
			std::lock_guard lock{ geometry_mutex };
			u8* const pointer{ geometry_hierarchies[id] };
			if ((uintptr_t)pointer & single_mesh_marker) 
				Graphics::RemoveSubmesh(GPU_IDFromFakePointer(pointer));
			else {
				GeometryHierarchyStream stream{ pointer };
				const u32 lod_count{ stream.LODCount() };
				u32 id_index{ 0 };
				for (u32 lod{ 0 }; lod < lod_count; lod++)
					for (u32 i{ 0 }; i < stream.LODOffsets()[lod].count; i++)
						Graphics::RemoveSubmesh(stream.GPU_IDs()[id_index++]);

				free(pointer);
			}

			geometry_hierarchies.Remove(id);
		}

		// NOTE: Expects data to contain
		// struct {
		//     MaterialType::Type type,
		//     u32 texture_count,
		//     ID::ID_Type shader_ids[ShaderType::count],
		//     ID::ID_Type* texture_ids
		// } MaterialInitInfo;
		//
		ID::ID_Type CreateMaterialResource(const void* const data) {
			assert(data);
			return Graphics::AddMaterial(*(const Graphics::MaterialInitInfo* const)data);
		}

		void DestroyMaterialResource(ID::ID_Type id) {
			Graphics::RemoveMaterial(id);
		}
	}

	ID::ID_Type CreateResource(const void* const data, AssetType::Type type) {
		assert(data);
		ID::ID_Type id{ ID::Invalid_ID };

		switch (type)
		{
		case AssetType::Unknown: break;
		case AssetType::Animation: break;
		case AssetType::Audio: break;
		case AssetType::Material: id = CreateMaterialResource(data); break;
		case AssetType::Mesh: id = CreateGeometryResource(data); break;
		case AssetType::Skeleton: break;
		case AssetType::Texture: break;
		}

		assert(ID::IsValid(id));
		return id;
	}

	void DestroyResource(ID::ID_Type id, AssetType::Type type) {
		assert(ID::IsValid(id));
		switch (type)
		{
		case AssetType::Unknown: break;
		case AssetType::Animation: break;
		case AssetType::Audio: break;
		case AssetType::Material: DestroyMaterialResource(id);  break;
		case AssetType::Mesh: DestroyGeometryResource(id); break;
		case AssetType::Skeleton: break;
		case AssetType::Texture: break;
		default:
			assert(false);
			break;
		}
	}

	ID::ID_Type AddShaderGroup(const u8** shaders, u32 shader_count, const u32* const keys) {
		assert(shaders && shader_count && keys);
		NoexceptMap group;
		for (u32 i{ 0 }; i < shader_count; i++) {
			assert(shaders[i]);
			const pCompiledShader shader_ptr{ (const pCompiledShader)shaders[i] };
			const u64 size{ shader_ptr->Size() };
			std::unique_ptr<u8[]> shader{ std::make_unique<u8[]>(size) };
			memcpy(shader.get(), shaders[i], size);
			group.map[keys[i]] = std::move(shader);
		}
		std::lock_guard lock{ shader_mutex };
		return shader_groups.Add(std::move(group));
	}

	void RemoveShaderGroup(ID::ID_Type id) {
		assert(ID::IsValid(id));
		std::lock_guard lock{ shader_mutex };

		shader_groups[id].map.clear();
		shader_groups.Remove(id);
	}

	pCompiledShader GetShader(ID::ID_Type id, u32 shader_key) {
		assert(ID::IsValid(id));
		std::lock_guard lock{ shader_mutex };

		for (const auto& [key, value] : shader_groups[id].map)
			if (key == shader_key)
				return (const pCompiledShader)value.get();

		assert(false); // Code path should never reach here
		return nullptr;
	}

	void GetSubmeshGPU_IDs(ID::ID_Type geometry_content_id, u32 id_count, ID::ID_Type* const gpu_ids) {
		std::lock_guard lock{ geometry_mutex };
		u8* const ptr{ geometry_hierarchies[geometry_content_id] };
		if ((uintptr_t)ptr & single_mesh_marker) {
			assert(id_count == 1);
			*gpu_ids = GPU_IDFromFakePointer(ptr);
		}
		else {
			GeometryHierarchyStream stream{ ptr };

			assert([&]() {
				const u32 lod_count{ stream.LODCount() };
				const LODOffset lod_offset{ stream.LODOffsets()[lod_count - 1] };
				const u32 gpu_id_count{ (u32)lod_offset.offset + (u32)lod_offset.count };
				return gpu_id_count == id_count;
				}());

			memcpy(gpu_ids, stream.GPU_IDs(), sizeof(ID::ID_Type) * id_count);
		}

	}

	void GetLODOffsets(const ID::ID_Type* const geometry_ids, const f32* const thresholds, u32 id_count, util::vector<LODOffset>& offsets) {
		assert(geometry_ids && thresholds && id_count);
		assert(offsets.empty());

		std::lock_guard lock{ geometry_mutex };

		for (u32 i{ 0 }; i < id_count; i++) {
			u8* const ptr{ geometry_hierarchies[geometry_ids[i]] };
			if ((uintptr_t)ptr & single_mesh_marker) {
				offsets.emplace_back(LODOffset{ 0, 1 });
			}
			else {
				GeometryHierarchyStream stream{ ptr };
				const u32 lod{ stream.LODFromThreshold(thresholds[i]) };
				offsets.emplace_back(stream.LODOffsets()[lod]);
			}
		}
	}
}