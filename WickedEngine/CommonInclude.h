#ifndef WICKEDENGINE_COMMONINCLUDE_H
#define WICKEDENGINE_COMMONINCLUDE_H

// This is a helper include file pasted into all engine headers, try to keep it minimal!
// Do not include engine features in this file!

#include <cfloat>
#include <cstdint>
#include <type_traits>

#define arraysize(a) (sizeof(a) / sizeof(a[0]))

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

// Simple common math helpers:

template<typename T>
constexpr T align(T value, T alignment)
{
	return value + (alignment - value) % alignment;
}

template <typename T>
constexpr T sqr(T x) { return x * x; }

template <typename T>
constexpr T pow4(T x) { return x * x * x * x; }

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

// Stack allocated string utility:
template <unsigned capacity = 256>
struct StackString
{
	char chars[capacity] = {};
	unsigned cnt = 0;
	static_assert(capacity > 1);
	constexpr operator const char* () const { return chars; }
	constexpr const char* const c_str() const { return chars; }
	constexpr void push_back(const char* str) { while (*str != 0 && (cnt < (capacity - 1))) { chars[cnt++] = *str; str++; } }
	constexpr unsigned size() const { return capacity; }
	constexpr unsigned length() const { return cnt; }
	constexpr bool empty() const { return cnt == 0; }
};

// Stack allocated simplified vector container:
template<typename T, unsigned count>
struct StackVector
{
	T items[count] = {};
	unsigned last = 0;
	constexpr unsigned size() const { return last; }
	constexpr bool empty() const { return last == 0; }
	constexpr void push_back(const T& item) { items[last++] = item; }
	constexpr void push_back(T&& item) { items[last++] = static_cast<T&&>(item); }
	constexpr void pop_back() { if (!empty()) items[--last] = {}; }
	constexpr void clear() { for (unsigned i = 0; i < count; ++i) items[i] = {}; last = 0; }
	constexpr T* begin() { return items; }
	constexpr T* end() { return items + last; }
	constexpr const T& back() const { return items[last - 1]; }
	constexpr T& back() { return items[0]; }
	constexpr const T& front() const { return items[last - 1]; }
	constexpr T& front() { return items[0]; }
	constexpr const T& operator[](unsigned index) const { return items[index]; }
	constexpr T& operator[](unsigned index) { return items[index]; }
};

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
inline unsigned long countbits(unsigned long value)
{
	return (unsigned long)__popcnt64((unsigned long long)value);
}
inline unsigned long long countbits(unsigned long long value)
{
	return __popcnt64(value);
}
inline unsigned int firstbithigh(unsigned int value)
{
	unsigned long bit_index;
	if (_BitScanReverse(&bit_index, (unsigned long)value))
	{
		return 31u - (unsigned int)bit_index;
	}
	return 0;
}
inline unsigned long firstbithigh(unsigned long value)
{
	unsigned long bit_index;
	if (_BitScanReverse64(&bit_index, (unsigned long long)value))
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
		return 63ull - bit_index;
	}
	return 0;
}
inline unsigned int firstbitlow(unsigned int value)
{
	unsigned long bit_index;
	if (_BitScanForward(&bit_index, value))
	{
		return (unsigned int)bit_index;
	}
	return 0;
}
inline unsigned long firstbitlow(unsigned long value)
{
	unsigned long bit_index;
	if (_BitScanForward64(&bit_index, (unsigned long long)value))
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
constexpr unsigned int countbits(unsigned int value)
{
	return __builtin_popcount(value);
}
constexpr unsigned long long countbits(unsigned long value)
{
	return __builtin_popcountl(value);
}
constexpr unsigned long long countbits(unsigned long long value)
{
	return __builtin_popcountll(value);
}
constexpr unsigned long firstbithigh(unsigned int value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_clz(value);
}
constexpr unsigned long firstbithigh(unsigned long value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_clzl(value);
}
constexpr unsigned long firstbithigh(unsigned long long value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_clzll(value);
}
constexpr unsigned long firstbitlow(unsigned int value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_ctz(value);
}
constexpr unsigned long firstbitlow(unsigned long value)
{
	if (value == 0)
	{
		return 0;
	}
	return __builtin_ctzl(value);
}
constexpr unsigned long firstbitlow(unsigned long long value)
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

// Extract file name from a path at compile-time
constexpr const char* relative_path(const char* path)
{
	const char* startPosition = path;
	for (const char* currentCharacter = path; *currentCharacter != '\0'; ++currentCharacter)
	{
		if (*currentCharacter == '\\' || *currentCharacter == '/')
		{
			startPosition = currentCharacter;
		}
	}

	if (startPosition != path)
	{
		++startPosition;
	}

	return startPosition;
}

// Extract function name from a string at compile-time
constexpr auto extract_function_name(const char* str)
{
	StackString ret;
	ret.push_back(str);
	return ret;
}

#endif //WICKEDENGINE_COMMONINCLUDE_H
