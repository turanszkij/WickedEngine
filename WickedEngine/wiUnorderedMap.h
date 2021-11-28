#ifndef WI_UNORDERED_MAP_REPLACEMENT
#define WI_UNORDERED_MAP_REPLACEMENT
// This file is used to allow replacement of std::unordered_map

#ifndef WI_UNORDERED_MAP_TYPE
#define WI_UNORDERED_MAP_TYPE 1
#endif // WI_UNORDERED_MAP_TYPE

#if WI_UNORDERED_MAP_TYPE == 1
#include "Utility/flat_hash_map.hpp"
#else
#include <unordered_map>
#endif // WI_UNORDERED_MAP_TYPE

namespace wi
{
	template<typename K, typename V>
#if WI_UNORDERED_MAP_TYPE == 1
	using unordered_map = ska::flat_hash_map<K, V>;
#else
	using unordered_map = std::unordered_map<K, V>;
#endif // WI_UNORDERED_MAP_TYPE
}

#endif // WI_UNORDERED_MAP_REPLACEMENT
