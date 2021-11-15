#ifndef WICKEDENGINE_COMMONINCLUDE_H
#define WICKEDENGINE_COMMONINCLUDE_H

// This is a helper include file pasted into all engine headers try to keep it minimal!
// Do not include engine features in this file!

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <type_traits>

#if __has_include("DirectXMath.h")
// In this case, DirectXMath is coming from Windows SDK.
//	It is better to use this on Windows as some Windows libraries could depend on the same 
//	DirectXMath headers
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#else
// In this case, DirectXMath is coming from supplied source code
//	On platforms that don't have Windows SDK, the source code for DirectXMath is provided
//	as part of the engine utilities
#include "Utility/DirectXMath.h"
#include "Utility/DirectXPackedVector.h"
#include "Utility/DirectXCollision.h"
#endif

using namespace DirectX;
using namespace DirectX::PackedVector;
static const XMFLOAT4X4 IDENTITYMATRIX = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

#define arraysize(a) (sizeof(a) / sizeof(a[0]))
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define ALIGN_16 void* operator new(size_t i){return _mm_malloc(i, 16);} void operator delete(void* p){_mm_free(p);}

// Enable enum flags:
//	https://www.justsoftwaresolutions.co.uk/cplusplus/using-enum-classes-as-bitfields.html
template<typename E>
struct enable_bitmask_operators {
	static constexpr bool enable = false;
};
template<typename E>
constexpr typename std::enable_if<enable_bitmask_operators<E>::enable, E>::type operator|(E lhs, E rhs)
{
	typedef typename std::underlying_type<E>::type underlying;
	return static_cast<E>(
		static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}
template<typename E>
constexpr typename std::enable_if<enable_bitmask_operators<E>::enable, E&>::type operator|=(E& lhs, E rhs)
{
	typedef typename std::underlying_type<E>::type underlying;
	lhs = static_cast<E>(
		static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
	return lhs;
}
template<typename E>
constexpr typename std::enable_if<enable_bitmask_operators<E>::enable, E>::type operator&(E lhs, E rhs)
{
	typedef typename std::underlying_type<E>::type underlying;
	return static_cast<E>(
		static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}
template<typename E>
constexpr typename std::enable_if<enable_bitmask_operators<E>::enable, E&>::type operator&=(E& lhs, E rhs)
{
	typedef typename std::underlying_type<E>::type underlying;
	lhs = static_cast<E>(
		static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
	return lhs;
}
template<typename E>
constexpr bool has(E lhs, E rhs)
{
	return (lhs & rhs) == rhs;
}

// convert enum to underlying value (this is part of the standard library in C++23)
template <typename T>
constexpr auto to_underlying(T e) noexcept
	-> std::enable_if_t<std::is_enum<T>::value, std::underlying_type_t<T>>
{
	return static_cast<std::underlying_type_t<T>>(e);
}

#endif //WICKEDENGINE_COMMONINCLUDE_H
