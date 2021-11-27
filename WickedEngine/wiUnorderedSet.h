#ifndef WI_UNORDERED_SET_REPLACEMENT
#define WI_UNORDERED_SET_REPLACEMENT
// This file is used to allow replacement of std::unordered_set

#ifndef WI_UNORDERED_SET_TYPE
#define WI_UNORDERED_SET_TYPE 1
#endif // WI_UNORDERED_SET_TYPE

#if WI_UNORDERED_SET_TYPE == 1
#include "Utility/flat_hash_map.hpp"
#else
#include <unordered_set>
#endif // WI_UNORDERED_SET_TYPE

namespace wi
{
	template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T> >
#if WI_UNORDERED_SET_TYPE == 1
	using unordered_set = ska::flat_hash_set<T, H, E, A>;
#else
	using unordered_set = std::unordered_set<T, H, E, A>;
#endif // WI_UNORDERED_SET_TYPE
}

#endif // WI_UNORDERED_SET_REPLACEMENT
