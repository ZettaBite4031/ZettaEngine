#pragma once
#include "CommonHeaders.h"
#include "MathTypes.h"

namespace Zetta::Math {
	template<typename T>
	constexpr T clamp(T v, T min, T max) {
		return (v < min) ? min : (v > max) ? max : v;
	}

	template<u32 bits>
	constexpr u32 PackUnitFloat(f32 f) {
		static_assert(bits <= sizeof(u32) * 8);
		assert(f >= 0.f && f <= 1.f);
		constexpr f32 intervals{ (f32)((1ui32 << bits) - 1) };
		return (u32)(intervals * f + 0.5f);
	}

	template<u32 bits>
	constexpr f32 UnpackUnitFloat(u32 i) {
		static_assert(bits <= sizeof(u32) * 8);
		assert(i < (1ui32 << bits));
		constexpr f32 intervals{ (f32)((1ui32 << bits) - 1) };
		return (f32)i / intervals;
	}

	template<u32 bits>
	constexpr u32 PackFloat(f32 f, f32 min, f32 max) {
		assert(min < max);
		assert(f >= min && f <= max);
		const f32 distance{ (f - min) / (max - min) };
		return PackUnitFloat<bits>(distance);
	}

	template<u32 bits>
	constexpr f32 UnpackFloat(u32 i, f32 min, f32 max) {
		assert(min < max);
		return UnpackUnitFloat<bits>(i) * (max - min) + min;
	}

	template<u64 alignment>
	constexpr u64 AlignSizeUp(u64 size) {
		static_assert(alignment, "Alignment must be non-zero");
		constexpr u32 mask{ alignment - 1 };
		static_assert(!(alignment & mask), "Alignment should be a power of two");
		return ((size + mask) & ~mask);
	}

	template<u64 alignment>
	constexpr u64 AlignSizeDown(u64 size) {
		static_assert(alignment, "Alignment must be non-zero");
		constexpr u32 mask{ alignment - 1 };
		static_assert(!(alignment & mask), "Alignment should be a power of two");
		return (size & ~mask);
	}
}