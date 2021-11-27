#pragma once

#ifndef WI_UNORDERED_CONTAINER_TYPE
#define WI_UNORDERED_CONTAINER_TYPE 1
#endif // WI_UNORDERED_CONTAINER_TYPE

#if WI_UNORDERED_CONTAINER_TYPE == 1
#include "Utility/flat_hash_map.hpp"
#else
#include <unordered_map>
#include <unordered_set>
#endif // WI_UNORDERED_CONTAINER_TYPE

namespace wi
{
	// Replacements for unordered_map:
	template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<const K, V> > >
#if WI_UNORDERED_CONTAINER_TYPE == 1
	using unordered_map = ska::flat_hash_map<K, V, H, E, A>;
#else
	using unordered_map = std::unordered_map<K, V, H, E, A>;
#endif // WI_UNORDERED_CONTAINER_TYPE

	// Replacements for unordered_set:
	template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T> >
#if WI_UNORDERED_CONTAINER_TYPE == 1
	using unordered_set = ska::flat_hash_set<T, H, E, A>;
#else
	using unordered_set = std::unordered_set<T, H, E, A>;
#endif // WI_UNORDERED_CONTAINER_TYPE

}
