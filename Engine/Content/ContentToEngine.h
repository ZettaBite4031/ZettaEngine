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

	typedef struct CompiledShader {
		static constexpr u32 hash_length{ 16 };
		constexpr u64 ByteCodeSize() const { return _byte_code_size; };
		constexpr const u8* const Hash() const { return &_hash[0]; }
		constexpr const u8* const ByteCode() const { return &_byte_code; }
	private:
		u64 _byte_code_size;
		u8 _hash[hash_length];
		u8 _byte_code;
	} const* pCompiledShader;

	ID::ID_Type CreateResource(const void* const data, AssetType::Type type);
	void DestroyResource(ID::ID_Type id, AssetType::Type type);

	ID::ID_Type AddShader(const u8* data);
	void RemoveShader(ID::ID_Type id);
	pCompiledShader GetShader(ID::ID_Type id);
}