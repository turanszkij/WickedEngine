#ifndef WICKEDENGINE_COMMONINCLUDE_H
#define WICKEDENGINE_COMMONINCLUDE_H

// This is a helper include file pasted into all engine headers, try to keep it minimal!
// Do not include engine features in this file!

#include <cstdint>
#include <type_traits>
#include <cstring>

#define arraysize(a) (sizeof(a) / sizeof(a[0]))

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

// Simple common math helpers:

template <typename T>
constexpr T sqr(T x) { return x * x; }

template <typename T>
constexpr T clamp(T x, T a, T b)
{
	return x < a ? a : (x > b ? b : x);
}

template <typename T>
constexpr T saturate(T x)
{
	return clamp(x, T(0), T(1));
}

template <typename T>
constexpr T frac(T x)
{
	T f = x - T(int(x));
	f = f < 0 ? (1 + f) : f;
	return f;
}

template <typename T>
constexpr float lerp(T x, T y, T a)
{
	return x * (1 - a) + y * a;
}

template <typename T>
constexpr T inverse_lerp(T value1, T value2, T pos)
{
	return value2 == value1 ? T(0) : ((pos - value1) / (value2 - value1));
}

template <typename T>
constexpr T smoothstep(T edge0, T edge1, T x)
{
	const T t = saturate((x - edge0) / (edge1 - edge0));
	return t * t * (T(3) - T(2) * t);
}

template <typename float4, typename float2>
constexpr float bilinear(float4 gather, float2 pixel_frac)
{
	const float top_row = lerp(gather.w, gather.z, pixel_frac.x);
	const float bottom_row = lerp(gather.x, gather.y, pixel_frac.x);
	return lerp(top_row, bottom_row, pixel_frac.y);
}

template<typename T>
constexpr T align(T value, T alignment)
{
	return ((value + alignment - T(1)) / alignment) * alignment;
}

template<typename T>
constexpr bool is_aligned(T value, T alignment)
{
	return value == align(value, alignment);
}

// CPU intrinsics:
#if defined(_WIN32)
// Windows, Xbox:
#include <intrin.h>
inline long AtomicAnd(volatile long* ptr, long mask)
{
	return _InterlockedAnd(ptr, mask);
}
inline long long AtomicAnd(volatile long long* ptr, long long mask)
{
	return _InterlockedAnd64(ptr, mask);
}
inline long AtomicOr(volatile long* ptr, long mask)
{
	return _InterlockedOr(ptr, mask);
}
inline long long AtomicOr(volatile long long* ptr, long long mask)
{
	return _InterlockedOr64(ptr, mask);
}
inline long AtomicXor(volatile long* ptr, long mask)
{
	return _InterlockedXor(ptr, mask);
}
inline long long AtomicXor(volatile long long* ptr, long long mask)
{
	return _InterlockedXor64(ptr, mask);
}
inline long AtomicAdd(volatile long* ptr, long val)
{
	return _InterlockedExchangeAdd(ptr, val);
}
inline long long AtomicAdd(volatile long long* ptr, long long val)
{
	return _InterlockedExchangeAdd64(ptr, val);
}
inline unsigned int countbits(unsigned int value)
{
	return __popcnt(value);
}
inline unsigned long long countbits(unsigned long long value)
{
	return __popcnt64(value);
}
inline unsigned long firstbithigh(unsigned long value)
{
	unsigned long bit_index;
	if (_BitScanReverse(&bit_index, value))
	{
		return 31ul - bit_index;
	}
	return 0;
}
inline unsigned long firstbithigh(unsigned long long value)
{
	unsigned long bit_index;
	if (_BitScanReverse64(&bit_index, value))
	{
		return 31ull - bit_index;
	}
	return 0;
}
inline unsigned long firstbitlow(unsigned long value)
{
	unsigned long bit_index;
	if (_BitScanForward(&bit_index, value))
	{
		return bit_index;
	}
	return 0;
}
inline unsigned long firstbitlow(unsigned long long value)
{
	unsigned long bit_index;
	if (_BitScanForward64(&bit_index, value))
	{
		return bit_index;
	}
	return 0;
}
#else
// Linux, PlayStation:
inline long AtomicAnd(volatile long* ptr, long mask)
{
	return __atomic_fetch_and(ptr, mask, __ATOMIC_SEQ_CST);
}
inline long long AtomicAnd(volatile long long* ptr, long long mask)
{
	return __atomic_fetch_and(ptr, mask, __ATOMIC_SEQ_CST);
}
inline long AtomicOr(volatile long* ptr, long mask)
{
	return __atomic_fetch_or(ptr, mask, __ATOMIC_SEQ_CST);
}
inline long long AtomicOr(volatile long long* ptr, long long mask)
{
	return __atomic_fetch_or(ptr, mask, __ATOMIC_SEQ_CST);
}
inline long AtomicXor(volatile long* ptr, long mask)
{
	return __atomic_fetch_xor(ptr, mask, __ATOMIC_SEQ_CST);
}
inline long long AtomicXor(volatile long long* ptr, long long mask)
{
	return __atomic_fetch_xor(ptr, mask, __ATOMIC_SEQ_CST);
}
inline long AtomicAdd(volatile long* ptr, long val)
{
	return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST);
}
inline long long AtomicAdd(volatile long long* ptr, long long val)
{
	return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST);
}
inline unsigned int countbits(unsigned int value)
{
	return __builtin_popcount(value);
}
inline unsigned long long countbits(unsigned long value)
{
	return __builtin_popcountl(value);
}
inline unsigned long long countbits(unsigned long long value)
{
	return __builtin_popcountll(value);
}
inline unsigned long firstbithigh(unsigned int value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_clz(value);
}
inline unsigned long firstbithigh(unsigned long value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_clzl(value);
}
inline unsigned long firstbithigh(unsigned long long value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_clzll(value);
}
inline unsigned long firstbitlow(unsigned int value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_ctz(value);
}
inline unsigned long firstbitlow(unsigned long value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_ctzl(value);
}
inline unsigned long firstbitlow(unsigned long long value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_ctzll(value);
}
#endif // _WIN32

inline long AtomicLoad(const volatile long* ptr)
{
	return AtomicOr((volatile long*)ptr, 0);
}
inline long long AtomicLoad(const volatile long long* ptr)
{
	return AtomicOr((volatile long long*)ptr, 0);
}

#include <emmintrin.h> // SSE
#include <immintrin.h> // AVX2
#include <zmmintrin.h> // AVX512

// AVX support can be enabled optionally:
#define ENABLE_AVX2
//#define ENABLE_AVX512

// Streaming memcpy for bypassing cache:
inline void memcpy_stream(void* dst, const void* src, size_t size)
{
#ifdef ENABLE_AVX512
	if (is_aligned(size, sizeof(__m512i)))
	{
		__m512i* _dst = (__m512i*)dst;
		const __m512i* _src = (const __m512i*)src;
		size_t remain = size / sizeof(__m512i);
		while (remain--)
		{
			_mm512_stream_si512(_dst++, _mm512_load_si512(_src++));
		}
		return;
	}
#endif // ENABLE_AVX512

#ifdef ENABLE_AVX2
	if (is_aligned(size, sizeof(__m256i)))
	{
		__m256i* _dst = (__m256i*)dst;
		const __m256i* _src = (const __m256i*)src;
		size_t remain = size / sizeof(__m256i);
		while (remain--)
		{
			_mm256_stream_si256(_dst++, _mm256_load_si256(_src++));
		}
		return;
	}
#endif // ENABLE_AVX2

	if (is_aligned(size, sizeof(__m128i)))
	{
		__m128i* _dst = (__m128i*)dst;
		const __m128i* _src = (const __m128i*)src;
		size_t remain = size / sizeof(__m128i);
		while (remain--)
		{
			_mm_stream_si128(_dst++, _mm_load_si128(_src++));
		}
		return;
	}

	if (is_aligned(size, sizeof(int)))
	{
		int* _dst = (int*)dst;
		const int* _src = (const int*)src;
		size_t remain = size / sizeof(int);
		while (remain--)
		{
			_mm_stream_si32(_dst++, *_src);
			_src++;
		}
		return;
	}

	// Fallback to memcpy:
	std::memcpy(dst, src, size);
}

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
constexpr typename std::enable_if<enable_bitmask_operators<E>::enable, E>::type operator~(E rhs)
{
	typedef typename std::underlying_type<E>::type underlying;
	rhs = static_cast<E>(
		~static_cast<underlying>(rhs));
	return rhs;
}
template<typename E>
constexpr bool has_flag(E lhs, E rhs)
{
	return (lhs & rhs) == rhs;
}

#endif //WICKEDENGINE_COMMONINCLUDE_H
