#ifndef WI_VECTOR_REPLACEMENT
#define WI_VECTOR_REPLACEMENT
// This file is used to allow replacement of std::vector

#include <vector>

namespace wi
{
	template<typename T>
	using vector = std::vector<T>;
}

#endif // WI_VECTOR_REPLACEMENT
