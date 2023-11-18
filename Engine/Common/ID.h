#pragma once

#include "CommonHeaders.h"

namespace Zetta::ID {
	using ID_Type = u32;
	namespace Detail {
		constexpr u32 Generation_Bits{ 10 };
		constexpr u32 Index_Bits{ sizeof(ID_Type) * 8 - Generation_Bits };
		constexpr ID_Type Index_Mask{ (ID_Type{1} << Index_Bits) - 1 };
		constexpr ID_Type Generation_Mask{ (ID_Type{1} << Generation_Bits) - 1 };
	}

	constexpr ID_Type Invalid_ID{ ID_Type(-1) };
	constexpr u32 Minimum_Deleted_Elements{ 1024 };

	using Generation_Type = std::conditional_t<Detail::Generation_Bits <= 16, std::conditional_t<Detail::Generation_Bits <= 8, u8, u16>, u32>;
	static_assert(sizeof(Generation_Type) * 8 >= Detail::Generation_Bits);

	constexpr bool IsValid(ID_Type id) {
		return id != Invalid_ID;
	}

	constexpr ID_Type Index(ID_Type id) {
		ID_Type index{ id & Detail::Index_Mask };
		assert(index != Detail::Index_Mask);
		return index;
	}

	constexpr ID_Type Generation(ID_Type id) {
		return (id >> Detail::Index_Bits) & Detail::Generation_Mask;
	}

	constexpr ID_Type NewGeneration(ID_Type id) {
		const ID_Type generation{ ID::Generation(id) + 1 };
		assert(generation < (((u64)1 << Detail::Generation_Bits) - 1));
		return Index(id) | (generation << Detail::Index_Bits);
	}

#if _DEBUG
	namespace Detail {
		struct ID_Base {
			constexpr explicit ID_Base(ID_Type id) : _id{ id } {}
			constexpr operator ID_Type() const { return _id; }

		private:
			ID_Type _id;
		};
	}
#define DEFINE_TYPED_ID(name)											\
		struct name final : ID::Detail::ID_Base {						\
			constexpr explicit name(ID::ID_Type id)						\
				: ID_Base{ id } {}										\
			constexpr name() : ID_Base{ 0 } {}							\
		};

#else
#define DEFINE_TYPED_ID(name) using name = ID::ID_Type;
#endif
}
