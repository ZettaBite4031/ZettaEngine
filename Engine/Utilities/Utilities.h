#pragma once

#define USE_STL_VECTOR 0
#define USE_STL_DEQUE 1

#if USE_STL_VECTOR
#include <vector>
#include <algorithm>
namespace Zetta::util {
	template<typename T>
	using vector = std::vector<T>;

	template<typename T>
	void EraseUnordered(T& v, size_t idx) {
		if (v.size() > 1) {
			std::iter_swap(v.begin() + idx, v.end() - 1);
			v.pop_back();
		}
		else v.clear();
	}
}
#else
#include "vector.h"

namespace Zetta::util {
	template<typename T>
	void EraseUnordered(T& v, size_t i) {
		v.EraseUnordered(i);
	}
}
#endif

#if USE_STL_DEQUE
#include <deque>
namespace Zetta::util {
	template<typename T>
	using deque = std::deque<T>;
}
#endif

#include "FreeList.h"
