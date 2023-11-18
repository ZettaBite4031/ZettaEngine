#pragma once
#include "CommonHeaders.h"

namespace Zetta::util {
#if USE_STL_VECTOR
#pragma message("WARNING: Using util::FreeList with std::vector will result in duplicate calls to class constructor")
#endif

	template<typename T>
	class FreeList {
		static_assert(sizeof(T) >= sizeof(u32));
	public:
		FreeList() = default;
		explicit FreeList(u32 count) {
			_array.reserve(count);
		}
		~FreeList() { 
			assert(!_size); 
#if USE_STL_VECTOR
			memset(_array.data(), 0x0, _array.size() * sizeof(T));
#endif
		}

		template<class... params>
		constexpr u32 Add(params&&... p) {
			u32 id{ u32_invalid_id };
			if (_next_free_index == u32_invalid_id) {
				id = (u32)_array.size();
				_array.emplace_back(std::forward<params>(p)...);
			}
			else {
				id = _next_free_index;
				_next_free_index = *(const u32* const)std::addressof(_array[id]);
				new (std::addressof(_array[id])) T(std::forward<params>(p)...);
			}
			_size++;
			return id;
		}

		constexpr void Remove(u32 id) {
			assert(id < _array.size() && !AlreadyRemoved(id));
			T& item{ _array[id] };
			item.~T();
			DEBUG_OP(memset(std::addressof(_array[id]), 0xCC, sizeof(T)));
			*(u32* const)std::addressof(_array[id]) = _next_free_index;
			_next_free_index = id;
			_size--;
		}

		constexpr u32 size() const { return _size; }
		constexpr u32 capacity() const { return _array.size(); }
		constexpr u32 empty() const { return _size == 0; }

		[[nodiscard]] constexpr T& operator[](u32 id) {
			assert(id < _array.size() && !AlreadyRemoved(id));
			return _array[id];
		}

		[[nodiscard]] constexpr const T& operator[](u32 id) const {
			assert(id < _array.size() && !AlreadyRemoved(id));
			return _array[id];
		}

	private:
		constexpr bool AlreadyRemoved(u32 id) {
			if constexpr (sizeof(T) > sizeof(u32)) {
				u32 i{ sizeof(u32) };
				const u8* const p{ (const u8* const)std::addressof(_array[id]) };
				while ((p[i] == 0xCC) && (i < sizeof(T))) i++;
				return i == sizeof(T);
			}
			else {
				return true;
			}
		}
#if USE_STL_VECTOR
		util::vector<T> _array;
#else
		util::vector<T, false> _array;
#endif
		u32 _next_free_index{ u32_invalid_id };
		u32 _size{ 0 };
	};
}