#pragma once
#include "CommonHeaders.h"

namespace Zetta::Content {
	struct AssetType {
		enum Type : u32 {
			Unknown = 0,
			Animation,
			Audio,
			Material,
			Mesh,
			Skeleton,
			Texture,

			count
		};
	};

	struct TextureFlags {
		enum Flags : u32 {
			IS_HDR = 0x01,
			HAS_ALPHA = 0x02,
			IS_PREMULTIPLIED_ALPHA = 0x04,
			IS_IMPORTED_AS_NORMAL_MAP = 0x08,
			IS_CUBE_MAP = 0x10,
			IS_VOLUME_MAP = 0x20,
		};
	};

	typedef struct CompiledShader {
		static constexpr u32 hash_length{ 16 };
		constexpr u64 ByteCodeSize() const { return _byte_code_size; };
		constexpr const u8* const Hash() const { return &_hash[0]; }
		constexpr const u8* const ByteCode() const { return &_byte_code; }
		constexpr const u64 Size() const { return sizeof(_byte_code_size) + hash_length + _byte_code_size; }
		constexpr static u64 Size(u64 size) { return sizeof(_byte_code_size) + hash_length + size; }
	private:
		u64 _byte_code_size;
		u8 _hash[hash_length];
		u8 _byte_code;
	} const* pCompiledShader;

	struct LODOffset {
		u16 offset;
		u16 count;
	};

	ID::ID_Type CreateResource(const void* const data, AssetType::Type type);
	void DestroyResource(ID::ID_Type id, AssetType::Type type);

	ID::ID_Type AddShaderGroup(const u8** shaders, u32 shader_count, const u32* const keys);
	void RemoveShaderGroup(ID::ID_Type id);
	pCompiledShader GetShader(ID::ID_Type id, u32 key);

	void GetSubmeshGPU_IDs(ID::ID_Type geometry_content_id, u32 id_count, ID::ID_Type* const gpu_ids);
	void GetLODOffsets(const ID::ID_Type* const geometry_ids, const f32* const thresholds, u32 id_count, util::vector<LODOffset>& offsets);
}