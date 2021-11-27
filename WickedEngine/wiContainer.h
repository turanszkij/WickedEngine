#pragma once

#ifndef WI_UNORDERED_MAP_TYPE
#define WI_UNORDERED_MAP_TYPE 1
#endif // WI_UNORDERED_MAP_TYPE

#if WI_UNORDERED_MAP_TYPE == 1
#include "Utility/flat_hash_map.hpp"
#elif WI_UNORDERED_MAP_TYPE == 2
#include "Utility/bytell_hash_map.hpp"
#elif WI_UNORDERED_MAP_TYPE == 3
#include "Utility/unordered_map.hpp"
#else
#include <unordered_map>
#include <unordered_set>
#endif // WI_UNORDERED_MAP_TYPE

#include <vector>

namespace wiContainer
{
	// Replacements for unordered_map:
	template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<const K, V> > >
#if WI_UNORDERED_MAP_TYPE == 1
	using unordered_map = ska::flat_hash_map<K, V, H, E, A>;
#elif WI_UNORDERED_MAP_TYPE == 2
	using unordered_map = ska::bytell_hash_map<K, V, H, E, A>;
#elif WI_UNORDERED_MAP_TYPE == 3
	using unordered_map = ska::unordered_map<K, V, H, E, A>;
#else
	using unordered_map = std::unordered_map<K, V, H, E, A>;
#endif // WI_UNORDERED_MAP_TYPE

	// Replacements for unordered_set:
	template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T> >
#if WI_UNORDERED_MAP_TYPE == 1
	using unordered_set = ska::flat_hash_set<T, H, E, A>;
#elif WI_UNORDERED_MAP_TYPE == 2
	using unordered_set = ska::bytell_hash_set<T, H, E, A>;
#elif WI_UNORDERED_MAP_TYPE == 3
	using unordered_set = ska::unordered_set<T, H, E, A>;
#else
	using unordered_set = std::unordered_set<T, H, E, A>;
#endif // WI_UNORDERED_MAP_TYPE

	// Replacements for vector:
	template <class T, class A = std::allocator<T>>
	using vector = std::vector<T, A>;

}
