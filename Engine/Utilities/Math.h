#pragma once
#include "CommonHeaders.h"
#include "MathTypes.h"

namespace Zetta::Math {
	constexpr bool IsEqual(f32 a, f32 b, f32 e = EPSILON) {
		f32 d{ a - b };
		if (d < 0.f) d = -d;
		return d < e;
	}

	template<typename T>
	[[nodiscard]] constexpr T clamp(T v, T min, T max) {
		return (v < min) ? min : (v > max) ? max : v;
	}

	template<u32 bits>
	[[nodiscard]] constexpr u32 PackUnitFloat(f32 f) {
		static_assert(bits <= sizeof(u32) * 8);
		assert(f >= 0.f && f <= 1.f);
		constexpr f32 intervals{ (f32)(((u32)1 << bits) - 1) };
		return (u32)(intervals * f + 0.5f);
	}

	template<u32 bits>
	[[nodiscard]] constexpr f32 UnpackUnitFloat(u32 i) {
		static_assert(bits <= sizeof(u32) * 8);
		assert(i < (1ui32 << bits));
		constexpr f32 intervals{ (f32)(((u32)1 << bits) - 1) };
		return (f32)i / intervals;
	}

	template<u32 bits>
	[[nodiscard]] constexpr u32 PackFloat(f32 f, f32 min, f32 max) {
		assert(min < max);
		assert(f >= min && f <= max);
		const f32 distance{ (f - min) / (max - min) };
		return PackUnitFloat<bits>(distance);
	}

	template<u32 bits>
	[[nodiscard]] constexpr f32 UnpackFloat(u32 i, f32 min, f32 max) {
		assert(min < max);
		return UnpackUnitFloat<bits>(i) * (max - min) + min;
	}

	template<u64 alignment>
	[[nodiscard]] constexpr u64 AlignSizeUp(u64 size) {
		static_assert(alignment, "Alignment must be non-zero");
		constexpr u64 mask{ alignment - 1 };
		static_assert(!(alignment & mask), "Alignment should be a power of two");
		return ((size + mask) & ~mask);
	}

	template<u64 alignment>
	[[nodiscard]] constexpr u64 AlignSizeDown(u64 size) {
		static_assert(alignment, "Alignment must be non-zero");
		constexpr u64 mask{ alignment - 1 };
		static_assert(!(alignment & mask), "Alignment should be a power of two");
		return (size & ~mask);
	}
	
	[[nodiscard]] constexpr u64 AlignSizeUp(u64 size, u64 alignment) {
		assert(alignment && "Alignment must be non-zero");
		const u64 mask{ alignment - 1 };
		assert(!(alignment & mask) && "Alignment should be a power of two");
		return ((size + mask) & ~mask);
	}

	[[nodiscard]] constexpr u64 AlignSizeDown(u64 size, u64 alignment) {
		assert(alignment && "Alignment must be non-zero");
		const u64 mask{ alignment - 1 };
		assert(!(alignment & mask) && "Alignment should be a power of two");
		return (size & ~mask);
	}

	[[nodiscard]] constexpr u64 calc_crc32_u64(const u8* const data, u64 size) {
		assert(size >= sizeof(u64));
		u64 crc{ 0 };
		const u8* at{ data };
		const u8* const end{ data + AlignSizeDown<sizeof(u64)>(size) };
		while (at < end) {
			crc = _mm_crc32_u64(crc, *((const u64*)at));
			at += sizeof(u64);
		}
		return crc;
	}
}