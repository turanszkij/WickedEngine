#ifndef WI_VECTOR_REPLACEMENT
#define WI_VECTOR_REPLACEMENT
// This file is used to allow replacement of std::vector

#include <vector>

namespace wi
{
	template<typename T, typename A = std::allocator<T>>
	using vector = std::vector<T, A>;
}

#endif // WI_VECTOR_REPLACEMENT
