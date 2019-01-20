// This code is in the public domain -- castanyo@yahoo.es
#include "stdafx.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include <memory>
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#undef min
#undef max
#include "xatlas.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define XA_STR(x) #x
#define XA_XSTR(x) XA_STR(x)

#ifndef XA_ASSERT
#define XA_ASSERT(exp) if (!(exp)) { XA_PRINT(0, "\rASSERT: %s %s %d\n", XA_XSTR(exp), __FILE__, __LINE__); }
#endif

#ifndef XA_DEBUG_ASSERT
#define XA_DEBUG_ASSERT(exp) assert(exp)
#endif

#ifndef XA_PRINT
#define XA_PRINT(flags, ...) \
	if (xatlas::internal::s_print && (flags == 0 || (xatlas::internal::s_printFlags & flags) != 0)) \
		xatlas::internal::s_print(__VA_ARGS__);
#endif

#define XA_EPSILON          (0.0001f)
#define XA_NORMAL_EPSILON   (0.001f)

namespace xatlas {
namespace internal {

static int s_printFlags = 0;
static PrintFunc s_print = printf;

static int align(int x, int a)
{
	return (x + a - 1) & ~(a - 1);
}

static bool isAligned(int x, int a)
{
	return (x & (a - 1)) == 0;
}

/// Return the maximum of the three arguments.
template <typename T>
static T max3(const T &a, const T &b, const T &c)
{
	return std::max(a, std::max(b, c));
}

/// Return the maximum of the three arguments.
template <typename T>
static T min3(const T &a, const T &b, const T &c)
{
	return std::min(a, std::min(b, c));
}

/// Clamp between two values.
template <typename T>
static T clamp(const T &x, const T &a, const T &b)
{
	return std::min(std::max(x, a), b);
}

// Robust floating point comparisons:
// http://realtimecollisiondetection.net/blog/?p=89
static bool equal(const float f0, const float f1, const float epsilon = XA_EPSILON)
{
	//return fabs(f0-f1) <= epsilon;
	return fabs(f0 - f1) <= epsilon * max3(1.0f, fabsf(f0), fabsf(f1));
}

static int ftoi_ceil(float val)
{
	return (int)ceilf(val);
}

static int ftoi_round(float f)
{
	return int(floorf(f + 0.5f));
}

static bool isZero(const float f, const float epsilon = XA_EPSILON)
{
	return fabs(f) <= epsilon;
}

static float square(float f)
{
	return f * f;
}

/** Return the next power of two.
* @see http://graphics.stanford.edu/~seander/bithacks.html
* @warning Behaviour for 0 is undefined.
* @note isPowerOfTwo(x) == true -> nextPowerOfTwo(x) == x
* @note nextPowerOfTwo(x) = 2 << log2(x-1)
*/
static uint32_t nextPowerOfTwo(uint32_t x)
{
	XA_DEBUG_ASSERT( x != 0 );
	// On modern CPUs this is supposed to be as fast as using the bsr instruction.
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

static uint32_t sdbmHash(const void *data_in, uint32_t size, uint32_t h = 5381)
{
	const uint8_t *data = (const uint8_t *) data_in;
	uint32_t i = 0;
	while (i < size) {
		h = (h << 16) + (h << 6) - h + (uint32_t ) data[i++];
	}
	return h;
}

template <typename T>
static uint32_t hash(const T &t, uint32_t h = 5381)
{
	return sdbmHash(&t, sizeof(T), h);
}

// Functors for hash table:
template <typename Key> struct Hash
{
	uint32_t operator()(const Key &k) const { return hash(k); }
};

template <typename Key> struct Equal
{
	bool operator()(const Key &k0, const Key &k1) const { return k0 == k1; }
};

class Vector2
{
public:
	typedef Vector2 const &Arg;

	Vector2() {}
	explicit Vector2(float f) : x(f), y(f) {}
	Vector2(float x, float y): x(x), y(y) {}
	Vector2(Vector2::Arg v) : x(v.x), y(v.y) {}

	const Vector2 &operator=(Vector2::Arg v)
	{
		x = v.x;
		y = v.y;
		return
			*this;
	}
	const float *ptr() const { return &x; }

	void set(float _x, float _y)
	{
		x = _x;
		y = _y;
	}

	Vector2 operator-() const
	{
		return Vector2(-x, -y);
	}

	void operator+=(Vector2::Arg v)
	{
		x += v.x;
		y += v.y;
	}

	void operator-=(Vector2::Arg v)
	{
		x -= v.x;
		y -= v.y;
	}

	void operator*=(float s)
	{
		x *= s;
		y *= s;
	}

	void operator*=(Vector2::Arg v)
	{
		x *= v.x;
		y *= v.y;
	}

	friend bool operator==(Vector2::Arg a, Vector2::Arg b)
	{
		return a.x == b.x && a.y == b.y;
	}

	friend bool operator!=(Vector2::Arg a, Vector2::Arg b)
	{
		return a.x != b.x || a.y != b.y;
	}

	union
	{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
		struct
		{
			float x, y;
		};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		float component[2];
	};
};

static Vector2 operator+(Vector2::Arg a, Vector2::Arg b)
{
	return Vector2(a.x + b.x, a.y + b.y);
}

static Vector2 operator-(Vector2::Arg a, Vector2::Arg b)
{
	return Vector2(a.x - b.x, a.y - b.y);
}

static Vector2 operator*(Vector2::Arg v, float s)
{
	return Vector2(v.x * s, v.y * s);
}

static Vector2 operator*(Vector2::Arg v1, Vector2::Arg v2)
{
	return Vector2(v1.x * v2.x, v1.y * v2.y);
}

static Vector2 lerp(Vector2::Arg v1, Vector2::Arg v2, float t)
{
	const float s = 1.0f - t;
	return Vector2(v1.x * s + t * v2.x, v1.y * s + t * v2.y);
}

static float dot(Vector2::Arg a, Vector2::Arg b)
{
	return a.x * b.x + a.y * b.y;
}

static float lengthSquared(Vector2::Arg v)
{
	return v.x * v.x + v.y * v.y;
}

static float length(Vector2::Arg v)
{
	return sqrtf(lengthSquared(v));
}

#ifdef _DEBUG
static bool isNormalized(Vector2::Arg v, float epsilon = XA_NORMAL_EPSILON)
{
	return equal(length(v), 1, epsilon);
}
#endif

static Vector2 normalize(Vector2::Arg v, float epsilon = XA_EPSILON)
{
	float l = length(v);
	XA_DEBUG_ASSERT(!isZero(l, epsilon));
#ifdef NDEBUG
	epsilon = epsilon; // silence unused parameter warning
#endif
	Vector2 n = v * (1.0f / l);
	XA_DEBUG_ASSERT(isNormalized(n));
	return n;
}

static bool equal(Vector2::Arg v1, Vector2::Arg v2, float epsilon = XA_EPSILON)
{
	return equal(v1.x, v2.x, epsilon) && equal(v1.y, v2.y, epsilon);
}

static Vector2 min(Vector2::Arg a, Vector2::Arg b)
{
	return Vector2(std::min(a.x, b.x), std::min(a.y, b.y));
}

static Vector2 max(Vector2::Arg a, Vector2::Arg b)
{
	return Vector2(std::max(a.x, b.x), std::max(a.y, b.y));
}

static bool isFinite(Vector2::Arg v)
{
	return std::isfinite(v.x) && std::isfinite(v.y);
}

// Note, this is the area scaled by 2!
static float triangleArea(Vector2::Arg v0, Vector2::Arg v1)
{
	return (v0.x * v1.y - v0.y * v1.x); // * 0.5f;
}

static float triangleArea(Vector2::Arg a, Vector2::Arg b, Vector2::Arg c)
{
	// IC: While it may be appealing to use the following expression:
	//return (c.x * a.y + a.x * b.y + b.x * c.y - b.x * a.y - c.x * b.y - a.x * c.y); // * 0.5f;
	// That's actually a terrible idea. Small triangles far from the origin can end up producing fairly large floating point
	// numbers and the results becomes very unstable and dependent on the order of the factors.
	// Instead, it's preferable to subtract the vertices first, and multiply the resulting small values together. The result
	// in this case is always much more accurate (as long as the triangle is small) and less dependent of the location of
	// the triangle.
	//return ((a.x - c.x) * (b.y - c.y) - (a.y - c.y) * (b.x - c.x)); // * 0.5f;
	return triangleArea(a - c, b - c);
}

class Vector3
{
public:
	typedef Vector3 const &Arg;

	Vector3() {}
	explicit Vector3(float f) : x(f), y(f), z(f) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	Vector3(Vector2::Arg v, float z) : x(v.x), y(v.y), z(z) {}
	Vector3(Vector3::Arg v) : x(v.x), y(v.y), z(v.z) {}

	const Vector3 &operator=(Vector3::Arg v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	Vector2 xy() const
	{
		return Vector2(x, y);
	}

	const float *ptr() const { return &x; }

	void set(float _x, float _y, float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}

	Vector3 operator-() const
	{
		return Vector3(-x, -y, -z);
	}

	void operator+=(Vector3::Arg v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
	}

	void operator-=(Vector3::Arg v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
	}

	void operator*=(float s)
	{
		x *= s;
		y *= s;
		z *= s;
	}

	void operator/=(float s)
	{
		float is = 1.0f / s;
		x *= is;
		y *= is;
		z *= is;
	}

	void operator*=(Vector3::Arg v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
	}

	void operator/=(Vector3::Arg v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
	}

	friend bool operator==(Vector3::Arg a, Vector3::Arg b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	friend bool operator!=(Vector3::Arg a, Vector3::Arg b)
	{
		return a.x != b.x || a.y != b.y || a.z != b.z;
	}

	union
	{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
		struct
		{
			float x, y, z;
		};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		float component[3];
	};
};

static Vector3 add(Vector3::Arg a, Vector3::Arg b)
{
	return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static Vector3 operator+(Vector3::Arg a, Vector3::Arg b)
{
	return add(a, b);
}

static Vector3 sub(Vector3::Arg a, Vector3::Arg b)
{
	return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static Vector3 operator-(Vector3::Arg a, Vector3::Arg b)
{
	return sub(a, b);
}

static Vector3 cross(Vector3::Arg a, Vector3::Arg b)
{
	return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

static Vector3 operator*(Vector3::Arg v, float s)
{
	return Vector3(v.x * s, v.y * s, v.z * s);
}

static Vector3 operator*(float s, Vector3::Arg v)
{
	return Vector3(v.x * s, v.y * s, v.z * s);
}

static Vector3 operator/(Vector3::Arg v, float s)
{
	return v * (1.0f / s);
}

static Vector3 lerp(Vector3::Arg v1, Vector3::Arg v2, float t)
{
	const float s = 1.0f - t;
	return Vector3(v1.x * s + t * v2.x, v1.y * s + t * v2.y, v1.z * s + t * v2.z);
}

static float dot(Vector3::Arg a, Vector3::Arg b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float lengthSquared(Vector3::Arg v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

static float length(Vector3::Arg v)
{
	return sqrtf(lengthSquared(v));
}

static bool isNormalized(Vector3::Arg v, float epsilon = XA_NORMAL_EPSILON)
{
	return equal(length(v), 1, epsilon);
}

static Vector3 normalize(Vector3::Arg v, float epsilon = XA_EPSILON)
{
	float l = length(v);
	XA_DEBUG_ASSERT(!isZero(l, epsilon));
#ifdef NDEBUG
	epsilon = epsilon; // silence unused parameter warning
#endif
	Vector3 n = v * (1.0f / l);
	XA_DEBUG_ASSERT(isNormalized(n));
	return n;
}

static Vector3 normalizeSafe(Vector3::Arg v, Vector3::Arg fallback, float epsilon = XA_EPSILON)
{
	float l = length(v);
	if (isZero(l, epsilon)) {
		return fallback;
	}
	return v * (1.0f / l);
}

#ifdef _DEBUG
bool isFinite(Vector3::Arg v)
{
	return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
#endif

template <typename T>
static void construct_range(T * ptr, uint32_t new_size, uint32_t old_size) {
	for (uint32_t i = old_size; i < new_size; i++) {
		new(ptr+i) T; // placement new
	}
}

template <typename T>
static void construct_range(T * ptr, uint32_t new_size, uint32_t old_size, const T & elem) {
	for (uint32_t i = old_size; i < new_size; i++) {
		new(ptr+i) T(elem); // placement new
	}
}

template <typename T>
static void construct_range(T * ptr, uint32_t new_size, uint32_t old_size, const T * src) {
	for (uint32_t i = old_size; i < new_size; i++) {
		new(ptr+i) T(src[i]); // placement new
	}
}

template <typename T>
static void destroy_range(T * ptr, uint32_t new_size, uint32_t old_size) {
	for (uint32_t i = new_size; i < old_size; i++) {
		(ptr+i)->~T(); // Explicit call to the destructor
	}
}

/**
* Replacement for std::vector that is easier to debug and provides
* some nice foreach enumerators. 
*/
template<typename T>
class Array {
public:
	typedef uint32_t size_type;

	Array() : m_buffer(NULL), m_capacity(0), m_size(0) {}

	Array(const Array & a) : m_buffer(NULL), m_capacity(0), m_size(0) {
		copy(a.m_buffer, a.m_size);
	}

	Array(const T * ptr, uint32_t num) : m_buffer(NULL), m_capacity(0), m_size(0) {
		copy(ptr, num);
	}

	explicit Array(uint32_t capacity) : m_buffer(NULL), m_capacity(0), m_size(0) {
		setArrayCapacity(capacity);
	}

	~Array() {
		clear();
		free(m_buffer);
	}

	const T & operator[]( uint32_t index ) const
	{
		XA_DEBUG_ASSERT(index < m_size);
		return m_buffer[index];
	}
	
	const T & at( uint32_t index ) const
	{
		XA_DEBUG_ASSERT(index < m_size);
		return m_buffer[index];
	}

	T & operator[] ( uint32_t index )
	{
		XA_DEBUG_ASSERT(index < m_size);
		return m_buffer[index];
	}

	T & at( uint32_t index )
	{
		XA_DEBUG_ASSERT(index < m_size);
		return m_buffer[index];
	}

	uint32_t size() const { return m_size; }
	uint32_t capacity() const { return m_capacity; }
	const T * data() const { return m_buffer; }
	T * data() { return m_buffer; }
	T * begin() { return m_buffer; }
	T * end() { return m_buffer + m_size; }
	const T * begin() const { return m_buffer; }
	const T * end() const { return m_buffer + m_size; }
	bool isEmpty() const { return m_size == 0; }

	void push_back( const T & val )
	{
		XA_DEBUG_ASSERT(&val < m_buffer || &val >= m_buffer+m_size);
		uint32_t old_size = m_size;
		uint32_t new_size = m_size + 1;
		setArraySize(new_size);
		construct_range(m_buffer, new_size, old_size, val);
	}

	void pop_back()
	{
		XA_DEBUG_ASSERT( m_size > 0 );
		resize( m_size - 1 );
	}

	const T & back() const
	{
		XA_DEBUG_ASSERT( m_size > 0 );
		return m_buffer[m_size-1];
	}

	T & back()
	{
		XA_DEBUG_ASSERT( m_size > 0 );
		return m_buffer[m_size-1];
	}

	const T & front() const
	{
		XA_DEBUG_ASSERT( m_size > 0 );
		return m_buffer[0];
	}

	T & front()
	{
		XA_DEBUG_ASSERT( m_size > 0 );
		return m_buffer[0];
	}

	// Remove the element at the given index. This is an expensive operation!
	void removeAt(uint32_t index)
	{
		XA_DEBUG_ASSERT(index >= 0 && index < m_size);
		if (m_size == 1) {
			clear();
		}
		else {
			m_buffer[index].~T();
			memmove(m_buffer+index, m_buffer+index+1, sizeof(T) * (m_size - 1 - index));
			m_size--;
		}
	}

	// Insert the given element at the given index shifting all the elements up.
	void insertAt(uint32_t index, const T & val = T())
	{
		XA_DEBUG_ASSERT( index >= 0 && index <= m_size );
		setArraySize(m_size + 1);
		if (index < m_size - 1) {
			memmove(m_buffer+index+1, m_buffer+index, sizeof(T) * (m_size - 1 - index));
		}
		// Copy-construct into the newly opened slot.
		new(m_buffer+index) T(val);
	}

	void append(const Array<T> & other)
	{
		append(other.m_buffer, other.m_size);
	}

	void append(const T other[], uint32_t count)
	{
		if (count > 0) {
			const uint32_t old_size = m_size;
			setArraySize(m_size + count);
			for (uint32_t i = 0; i < count; i++ ) {
				new(m_buffer + old_size + i) T(other[i]);
			}
		}
	}

	void resize(uint32_t new_size)
	{
		uint32_t old_size = m_size;
		// Destruct old elements (if we're shrinking).
		destroy_range(m_buffer, new_size, old_size);
		setArraySize(new_size);
		// Call default constructors
		construct_range(m_buffer, new_size, old_size);
	}

	void resize(uint32_t new_size, const T & elem)
	{
		XA_DEBUG_ASSERT(&elem < m_buffer || &elem > m_buffer+m_size);
		uint32_t old_size = m_size;
		// Destruct old elements (if we're shrinking).
		destroy_range(m_buffer, new_size, old_size);
		setArraySize(new_size);
		// Call copy constructors
		construct_range(m_buffer, new_size, old_size, elem);
	}

	void clear()
	{
		// Destruct old elements
		destroy_range(m_buffer, 0, m_size);
		m_size = 0;
	}

	void reserve(uint32_t desired_size)
	{
		if (desired_size > m_capacity) {
			setArrayCapacity(desired_size);
		}
	}

	void copy(const T * data, uint32_t count)
	{
		destroy_range(m_buffer, 0, m_size);
		setArraySize(count);
		construct_range(m_buffer, count, 0, data);
	}

	Array<T> & operator=( const Array<T> & a )
	{
		copy(a.m_buffer, a.m_size);
		return *this;
	}

	friend void swap(Array<T> & a, Array<T> & b)
	{
		std::swap(a.m_buffer, b.m_buffer);
		std::swap(a.m_capacity, b.m_capacity);
		std::swap(a.m_size, b.m_size);
	}

protected:
	void setArraySize(uint32_t new_size)
	{
		m_size = new_size;
		if (new_size > m_capacity) {
			uint32_t new_buffer_size;
			if (m_capacity == 0) {
				// first allocation is exact
				new_buffer_size = new_size;
			}
			else {
				// following allocations grow array by 25%
				new_buffer_size = new_size + (new_size >> 2);
			}
			setArrayCapacity( new_buffer_size );
		}
	}
	void setArrayCapacity(uint32_t new_capacity)
	{
		XA_DEBUG_ASSERT(new_capacity >= m_size);
		if (new_capacity == 0) {
			// free the buffer.
			if (m_buffer != NULL) {
				free(m_buffer);
				m_buffer = NULL;
			}
		}
		else {
			// realloc the buffer
			m_buffer = (T *)realloc(m_buffer, sizeof(T) * new_capacity);
		}
		m_capacity = new_capacity;
	}

	T * m_buffer;
	uint32_t m_capacity;
	uint32_t m_size;
};

/// Basis class to compute tangent space basis, ortogonalizations and to
/// transform vectors from one space to another.
class Basis
{
public:
	/// Create a null basis.
	Basis() : tangent(0, 0, 0), bitangent(0, 0, 0), normal(0, 0, 0) {}

	void buildFrameForDirection(Vector3::Arg d, float angle = 0)
	{
		XA_ASSERT(isNormalized(d));
		normal = d;
		// Choose minimum axis.
		if (fabsf(normal.x) < fabsf(normal.y) && fabsf(normal.x) < fabsf(normal.z)) {
			tangent = Vector3(1, 0, 0);
		} else if (fabsf(normal.y) < fabsf(normal.z)) {
			tangent = Vector3(0, 1, 0);
		} else {
			tangent = Vector3(0, 0, 1);
		}
		// Ortogonalize
		tangent -= normal * dot(normal, tangent);
		tangent = normalize(tangent);
		bitangent = cross(normal, tangent);
		// Rotate frame around normal according to angle.
		if (angle != 0.0f) {
			float c = cosf(angle);
			float s = sinf(angle);
			Vector3 tmp = c * tangent - s * bitangent;
			bitangent = s * tangent + c * bitangent;
			tangent = tmp;
		}
	}

	Vector3 tangent;
	Vector3 bitangent;
	Vector3 normal;
};

// Simple bit array.
class BitArray
{
public:
	BitArray() : m_size(0) {}
	BitArray(uint32_t sz)
	{
		resize(sz);
	}

	uint32_t size() const
	{
		return m_size;
	}

	void clear()
	{
		resize(0);
	}

	void resize(uint32_t new_size)
	{
		m_size = new_size;
		m_wordArray.resize( (m_size + 31) >> 5 );
	}

	/// Get bit.
	bool bitAt(uint32_t b) const
	{
		XA_DEBUG_ASSERT( b < m_size );
		return (m_wordArray[b >> 5] & (1 << (b & 31))) != 0;
	}

	// Set a bit.
	void setBitAt(uint32_t idx)
	{
		XA_DEBUG_ASSERT(idx < m_size);
		m_wordArray[idx >> 5] |=  (1 << (idx & 31));
	}

	// Toggle a bit.
	void toggleBitAt(uint32_t idx)
	{
		XA_DEBUG_ASSERT(idx < m_size);
		m_wordArray[idx >> 5] ^= (1 << (idx & 31));
	}

	// Set a bit to the given value. @@ Rename modifyBitAt?
	void setBitAt(uint32_t idx, bool b)
	{
		XA_DEBUG_ASSERT(idx < m_size);
		m_wordArray[idx >> 5] = setBits(m_wordArray[idx >> 5], 1 << (idx & 31), b);
		XA_DEBUG_ASSERT(bitAt(idx) == b);
	}

	// Clear all the bits.
	void clearAll()
	{
		memset(m_wordArray.data(), 0, m_wordArray.size() * sizeof(uint32_t ));
	}

	// Set all the bits.
	void setAll()
	{
		memset(m_wordArray.data(), 0xFF, m_wordArray.size() * sizeof(uint32_t ));
	}

private:
	// See "Conditionally set or clear bits without branching" at http://graphics.stanford.edu/~seander/bithacks.html
	uint32_t setBits(uint32_t w, uint32_t m, bool b)
	{
		return (w & ~m) | (-int(b) & m);
	}

	// Number of bits stored.
	uint32_t m_size;

	// Array of bits.
	Array<uint32_t> m_wordArray;
};

/// Bit map. This should probably be called BitImage.
class BitMap
{
public:
	BitMap() : m_width(0), m_height(0) {}
	BitMap(uint32_t w, uint32_t h) : m_width(w), m_height(h), m_bitArray(w * h) {}

	uint32_t width() const
	{
		return m_width;
	}
	uint32_t height() const
	{
		return m_height;
	}

	void resize(uint32_t w, uint32_t h, bool initValue)
	{
		BitArray tmp(w * h);
		if (initValue) tmp.setAll();
		else tmp.clearAll();
		// @@ Copying one bit at a time. This could be much faster.
		for (uint32_t y = 0; y < m_height; y++) {
			for (uint32_t x = 0; x < m_width; x++) {
				//tmp.setBitAt(y*w + x, bitAt(x, y));
				if (bitAt(x, y) != initValue) tmp.toggleBitAt(y * w + x);
			}
		}
		std::swap(m_bitArray, tmp);
		m_width = w;
		m_height = h;
	}

	bool bitAt(uint32_t x, uint32_t y) const
	{
		XA_DEBUG_ASSERT(x < m_width && y < m_height);
		return m_bitArray.bitAt(y * m_width + x);
	}

	void setBitAt(uint32_t x, uint32_t y)
	{
		XA_DEBUG_ASSERT(x < m_width && y < m_height);
		m_bitArray.setBitAt(y * m_width + x);
	}

	void clearAll()
	{
		m_bitArray.clearAll();
	}

private:
	uint32_t m_width;
	uint32_t m_height;
	BitArray m_bitArray;
};

class Fit
{
public:
	static Vector3 computeCentroid(int n, const Vector3 * points)
	{
		Vector3 centroid(0.0f);
		for (int i = 0; i < n; i++) {
			centroid += points[i];
		}
		centroid /= float(n);
		return centroid;
	}

	static Vector3 computeCovariance(int n, const Vector3 * points, float * covariance)
	{
		// compute the centroid
		Vector3 centroid = computeCentroid(n, points);
		// compute covariance matrix
		for (int i = 0; i < 6; i++) {
			covariance[i] = 0.0f;
		}
		for (int i = 0; i < n; i++) {
			Vector3 v = points[i] - centroid;
			covariance[0] += v.x * v.x;
			covariance[1] += v.x * v.y;
			covariance[2] += v.x * v.z;
			covariance[3] += v.y * v.y;
			covariance[4] += v.y * v.z;
			covariance[5] += v.z * v.z;
		}
		return centroid;
	}

	static bool isPlanar(int n, const Vector3 *points, float epsilon = XA_EPSILON)
	{
		// compute the centroid and covariance
		float matrix[6];
		computeCovariance(n, points, matrix);
		float eigenValues[3];
		Vector3 eigenVectors[3];
		if (!eigenSolveSymmetric3(matrix, eigenValues, eigenVectors)) {
			return false;
		}
		return eigenValues[2] < epsilon;
	}

	// Tridiagonal solver from Charles Bloom.
	// Householder transforms followed by QL decomposition.
	// Seems to be based on the code from Numerical Recipes in C.
	static bool eigenSolveSymmetric3(const float matrix[6], float eigenValues[3], Vector3 eigenVectors[3])
	{
		XA_DEBUG_ASSERT(matrix != NULL && eigenValues != NULL && eigenVectors != NULL);
		float subd[3];
		float diag[3];
		float work[3][3];
		work[0][0] = matrix[0];
		work[0][1] = work[1][0] = matrix[1];
		work[0][2] = work[2][0] = matrix[2];
		work[1][1] = matrix[3];
		work[1][2] = work[2][1] = matrix[4];
		work[2][2] = matrix[5];
		EigenSolver3_Tridiagonal(work, diag, subd);
		if (!EigenSolver3_QLAlgorithm(work, diag, subd)) {
			for (int i = 0; i < 3; i++) {
				eigenValues[i] = 0;
				eigenVectors[i] = Vector3(0);
			}
			return false;
		}
		for (int i = 0; i < 3; i++) {
			eigenValues[i] = (float)diag[i];
		}
		// eigenvectors are the columns; make them the rows :
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				eigenVectors[j].component[i] = (float) work[i][j];
			}
		}
		// shuffle to sort by singular value :
		if (eigenValues[2] > eigenValues[0] && eigenValues[2] > eigenValues[1]) {
			std::swap(eigenValues[0], eigenValues[2]);
			std::swap(eigenVectors[0], eigenVectors[2]);
		}
		if (eigenValues[1] > eigenValues[0]) {
			std::swap(eigenValues[0], eigenValues[1]);
			std::swap(eigenVectors[0], eigenVectors[1]);
		}
		if (eigenValues[2] > eigenValues[1]) {
			std::swap(eigenValues[1], eigenValues[2]);
			std::swap(eigenVectors[1], eigenVectors[2]);
		}
		XA_DEBUG_ASSERT(eigenValues[0] >= eigenValues[1] && eigenValues[0] >= eigenValues[2]);
		XA_DEBUG_ASSERT(eigenValues[1] >= eigenValues[2]);
		return true;
	}

private:
	static void EigenSolver3_Tridiagonal(float mat[3][3], float *diag, float *subd)
	{
		// Householder reduction T = Q^t M Q
		//   Input:
		//     mat, symmetric 3x3 matrix M
		//   Output:
		//     mat, orthogonal matrix Q
		//     diag, diagonal entries of T
		//     subd, subdiagonal entries of T (T is symmetric)
		const float epsilon = 1e-08f;
		float a = mat[0][0];
		float b = mat[0][1];
		float c = mat[0][2];
		float d = mat[1][1];
		float e = mat[1][2];
		float f = mat[2][2];
		diag[0] = a;
		subd[2] = 0.f;
		if (fabsf(c) >= epsilon) {
			const float ell = sqrtf(b * b + c * c);
			b /= ell;
			c /= ell;
			const float q = 2 * b * e + c * (f - d);
			diag[1] = d + c * q;
			diag[2] = f - c * q;
			subd[0] = ell;
			subd[1] = e - b * q;
			mat[0][0] = 1;
			mat[0][1] = 0;
			mat[0][2] = 0;
			mat[1][0] = 0;
			mat[1][1] = b;
			mat[1][2] = c;
			mat[2][0] = 0;
			mat[2][1] = c;
			mat[2][2] = -b;
		} else {
			diag[1] = d;
			diag[2] = f;
			subd[0] = b;
			subd[1] = e;
			mat[0][0] = 1;
			mat[0][1] = 0;
			mat[0][2] = 0;
			mat[1][0] = 0;
			mat[1][1] = 1;
			mat[1][2] = 0;
			mat[2][0] = 0;
			mat[2][1] = 0;
			mat[2][2] = 1;
		}
	}

	static bool EigenSolver3_QLAlgorithm(float mat[3][3], float *diag, float *subd)
	{
		// QL iteration with implicit shifting to reduce matrix from tridiagonal
		// to diagonal
		const int maxiter = 32;
		for (int ell = 0; ell < 3; ell++) {
			int iter;
			for (iter = 0; iter < maxiter; iter++) {
				int m;
				for (m = ell; m <= 1; m++) {
					float dd = fabsf(diag[m]) + fabsf(diag[m + 1]);
					if ( fabsf(subd[m]) + dd == dd )
						break;
				}
				if ( m == ell )
					break;
				float g = (diag[ell + 1] - diag[ell]) / (2 * subd[ell]);
				float r = sqrtf(g * g + 1);
				if ( g < 0 )
					g = diag[m] - diag[ell] + subd[ell] / (g - r);
				else
					g = diag[m] - diag[ell] + subd[ell] / (g + r);
				float s = 1, c = 1, p = 0;
				for (int i = m - 1; i >= ell; i--) {
					float f = s * subd[i], b = c * subd[i];
					if ( fabsf(f) >= fabsf(g) ) {
						c = g / f;
						r = sqrtf(c * c + 1);
						subd[i + 1] = f * r;
						c *= (s = 1 / r);
					} else {
						s = f / g;
						r = sqrtf(s * s + 1);
						subd[i + 1] = g * r;
						s *= (c = 1 / r);
					}
					g = diag[i + 1] - p;
					r = (diag[i] - g) * s + 2 * b * c;
					p = s * r;
					diag[i + 1] = g + p;
					g = c * r - b;
					for (int k = 0; k < 3; k++) {
						f = mat[k][i + 1];
						mat[k][i + 1] = s * mat[k][i] + c * f;
						mat[k][i] = c * mat[k][i] - s * f;
					}
				}
				diag[ell] -= p;
				subd[ell] = g;
				subd[m] = 0;
			}
			if ( iter == maxiter )
				// should not get here under normal circumstances
				return false;
		}
		return true;
	}
};

/// Fixed size vector class.
class FullVector
{
public:
	FullVector(uint32_t dim) { m_array.resize(dim); }
	FullVector(const FullVector &v) : m_array(v.m_array) {}

	const FullVector &operator=(const FullVector &v)
	{
		XA_ASSERT(dimension() == v.dimension());
		m_array = v.m_array;
		return *this;
	}

	uint32_t dimension() const { return m_array.size(); }
	const float &operator[]( uint32_t index ) const { return m_array[index]; }
	float &operator[] ( uint32_t index ) { return m_array[index]; }

	void fill(float f)
	{
		const uint32_t dim = dimension();
		for (uint32_t i = 0; i < dim; i++) {
			m_array[i] = f;
		}
	}

	void operator+=(const FullVector &v)
	{
		XA_DEBUG_ASSERT(dimension() == v.dimension());
		const uint32_t dim = dimension();
		for (uint32_t i = 0; i < dim; i++) {
			m_array[i] += v.m_array[i];
		}
	}

	void operator-=(const FullVector &v)
	{
		XA_DEBUG_ASSERT(dimension() == v.dimension());
		const uint32_t dim = dimension();
		for (uint32_t i = 0; i < dim; i++) {
			m_array[i] -= v.m_array[i];
		}
	}

	void operator*=(const FullVector &v)
	{
		XA_DEBUG_ASSERT(dimension() == v.dimension());
		const uint32_t dim = dimension();
		for (uint32_t i = 0; i < dim; i++) {
			m_array[i] *= v.m_array[i];
		}
	}

	void operator+=(float f)
	{
		const uint32_t dim = dimension();
		for (uint32_t i = 0; i < dim; i++) {
			m_array[i] += f;
		}
	}

	void operator-=(float f)
	{
		const uint32_t dim = dimension();
		for (uint32_t i = 0; i < dim; i++) {
			m_array[i] -= f;
		}
	}

	void operator*=(float f)
	{
		const uint32_t dim = dimension();
		for (uint32_t i = 0; i < dim; i++) {
			m_array[i] *= f;
		}
	}

private:
	Array<float> m_array;
};

/** Thatcher Ulrich's hash table.
*
* Hash table, linear probing, internal chaining.  One
* interesting/nice thing about this implementation is that the table
* itself is a flat chunk of memory containing no pointers, only
* relative indices.  If the key and value types of the hash contain
* no pointers, then the hash can be serialized using raw IO.  Could
* come in handy.
*
* Never shrinks, unless you explicitly clear() it.  Expands on
* demand, though.  For best results, if you know roughly how big your
* table will be, default it to that size when you create it.
*/
template<typename T, typename U, typename H = Hash<T>, typename E = Equal<T> >
class HashMap
{
public:
	HashMap() : entry_count(0), size_mask(-1), table(NULL) { }
	explicit HashMap(int size_hint) : entry_count(0), size_mask(-1), table(NULL) { setCapacity(size_hint); }
	~HashMap() { clear(); }
	
	// Add a new value to the hash table, under the specified key.
	void add(const T& key, const U& value)
	{
		XA_ASSERT(findIndex(key) == -1);
		checkExpand();
		XA_ASSERT(table != NULL);
		entry_count++;
		const uint32_t hash_value = compute_hash(key);
		const int index = hash_value & size_mask;
		Entry * natural_entry = &(entry(index));
		if (natural_entry->isEmpty())
		{
			// Put the new entry in.
			new (natural_entry) Entry(key, value, -1, hash_value);
		} 
		else if (natural_entry->isTombstone()) {
			// Put the new entry in, without disturbing the rest of the chain.
			int next_in_chain = natural_entry->next_in_chain;
			new (natural_entry) Entry(key, value, next_in_chain, hash_value);
		}
		else
		{
			// Find a blank spot.
			int	blank_index = index;
			for (int search_count = 0; ; search_count++)
			{
				blank_index = (blank_index + 1) & size_mask;
				if (entry(blank_index).isEmpty()) break;	// found it
				if (entry(blank_index).isTombstone()) {
					blank_index = removeTombstone(blank_index);
					break;
				}
				XA_ASSERT(search_count < this->size_mask);
			}
			Entry * blank_entry = &entry(blank_index);

			if (int(natural_entry->hash_value & size_mask) == index)
			{
				// Collision.  Link into this chain.
				// Move existing list head.
				new (blank_entry) Entry(*natural_entry);	// placement new, copy ctor
															// Put the new info in the natural entry.
				natural_entry->key = key;
				natural_entry->value = value;
				natural_entry->next_in_chain = blank_index;
				natural_entry->hash_value = hash_value;
			}
			else
			{
				// Existing entry does not naturally
				// belong in this slot.  Existing
				// entry must be moved.
				// Find natural location of collided element (i.e. root of chain)
				int	collided_index = natural_entry->hash_value & size_mask;
				for (int search_count = 0; ; search_count++)
				{
					Entry * e = &entry(collided_index);
					if (e->next_in_chain == index)
					{
						// Here's where we need to splice.
						new (blank_entry) Entry(*natural_entry);
						e->next_in_chain = blank_index;
						break;
					}
					collided_index = e->next_in_chain;
					XA_ASSERT(collided_index >= 0 && collided_index <= size_mask);
					XA_ASSERT(search_count <= size_mask);
				}
				// Put the new data in the natural entry.
				natural_entry->key = key;
				natural_entry->value = value;
				natural_entry->hash_value = hash_value;
				natural_entry->next_in_chain = -1;
			}
		}
	}

	// Remove the first value under the specified key.
	bool remove(const T& key)
	{
		if (table == NULL)
			return false;
		int	index = findIndex(key);
		if (index < 0)
			return false;
		Entry * pos = &entry(index);
		int natural_index = (int) (pos->hash_value & size_mask);
		if (index != natural_index) {
			// We're not the head of our chain, so we can
			// be spliced out of it.
			// Iterate up the chain, and splice out when
			// we get to m_index.
			Entry* e = &entry(natural_index);
			while (e->next_in_chain != index) {
				XA_DEBUG_ASSERT(e->isEndOfChain() == false);
				e = &entry(e->next_in_chain);
			}
			if (e->isTombstone() && pos->isEndOfChain()) {
				// Tombstone has nothing else to point
				// to, so mark it empty.
				e->next_in_chain = -2;
			} else {
				e->next_in_chain = pos->next_in_chain;
			}
			pos->clear();
		}
		else if (pos->isEndOfChain() == false) {
			// We're the head of our chain, and there are
			// additional elements.
			//
			// We need to put a tombstone here.
			//
			// We can't clear the element, because the
			// rest of the elements in the chain must be
			// linked to this position.
			//
			// We can't move any of the succeeding
			// elements in the chain (i.e. to fill this
			// entry), because we don't want to invalidate
			// any other existing iterators.
			pos->makeTombstone();
		} else {
			// We're the head of the chain, but we're the
			// only member of the chain.
			pos->clear();
		}
		entry_count--;
		return true;
	}

	// Remove all entries from the hash table.
	void clear()
	{
		if (table != NULL)
		{
			// Delete the entries.
			for (int i = 0, n = size_mask; i <= n; i++)
			{
				Entry * e = &entry(i);
				if (e->isEmpty() == false && e->isTombstone() == false)
					e->clear();
			}
			free(table);
			table = NULL;
			entry_count = 0;
			size_mask = -1;
		}
	}

	bool isEmpty() const
	{
		return table == NULL || entry_count == 0;
	}

	// Retrieve the value under the given key.
	// - If there's no value under the key, then return false and leave *value alone.
	// - If there is a value, return true, and set *value to the entry's value.
	// - If value == NULL, return true or false according to the presence of the key, but don't touch *value.
	bool get(const T& key, U* value = NULL, T* other_key = NULL) const
	{
		int	index = findIndex(key);
		if (index >= 0)
		{
			if (value != NULL) {
				*value = entry(index).value;	// take care with side-effects!
			}
			if (other_key != NULL) {
				*other_key = entry(index).key;
			}
			return true;
		}
		return false;
	}

	int	size() const
	{
		return entry_count;
	}

	int	count() const
	{
		return entry_count;
	}

	int	capacity() const
	{
		return size_mask+1;
	}

	// Hint the bucket count to >= n.
	void resize(int n)
	{
		// Not really sure what this means in relation to
		// STLport's hash_map... they say they "increase the
		// bucket count to at least n" -- but does that mean
		// their real capacity after resize(n) is more like
		// n*2 (since they do linked-list chaining within
		// buckets?).
		setCapacity(n);
	}

	// Behaves much like std::pair.
	struct Entry
	{
		int	next_in_chain;	// internal chaining for collisions
		uint32_t hash_value;	// avoids recomputing.  Worthwhile?
		T key;
		U value;

		Entry() : next_in_chain(-2) {}
		Entry(const Entry& e) : next_in_chain(e.next_in_chain), hash_value(e.hash_value), key(e.key), value(e.value) {}
		Entry(const T& k, const U& v, int next, int hash) : next_in_chain(next), hash_value(hash), key(k), value(v) {}
		bool isEmpty() const { return next_in_chain == -2; }
		bool isEndOfChain() const { return next_in_chain == -1; }
		bool isTombstone() const { return hash_value == TOMBSTONE_HASH; }

		void clear() {
			key.~T();	// placement delete
			value.~U();	// placement delete
			next_in_chain = -2;
			hash_value = ~TOMBSTONE_HASH;
		}

		void makeTombstone() {
			key.~T();
			value.~U();
			hash_value = TOMBSTONE_HASH;
		}
	};

	// HashMap enumerator.
	typedef int PseudoIndex;
	PseudoIndex start() const { PseudoIndex i = 0; findNext(i); return i; }
	bool isDone(const PseudoIndex & i) const { XA_DEBUG_ASSERT(i <= size_mask+1); return i == size_mask+1; };
	void advance(PseudoIndex & i) const { XA_DEBUG_ASSERT(i <= size_mask+1); i++; findNext(i); }

	Entry & operator[](const PseudoIndex & i) {
		Entry & e = entry(i);
		XA_DEBUG_ASSERT(e.isTombstone() == false);
		return e;
	}

	const Entry & operator[](const PseudoIndex & i) const {
		const Entry & e = entry(i);
		XA_DEBUG_ASSERT(e.isTombstone() == false);
		return e;
	}

	friend void swap(HashMap<T, U, H, E> & a, HashMap<T, U, H, E> & b)
	{
		std::swap(a.entry_count, b.entry_count);
		std::swap(a.size_mask, b.size_mask);
		std::swap(a.table, b.table);
	}

private:
	// Size the hash so that it can comfortably contain the given number of elements.  If the hash already contains more
	// elements than new_size, then this may be a no-op.
	void setCapacity(int new_size)
	{
		int	new_raw_size = (new_size * 3) / 2;
		if (new_raw_size < size()) { return; }
		setRawCapacity(new_raw_size);
	}

	// Resize the hash table to fit one more entry.  Often this doesn't involve any action.
	void checkExpand()
	{
		if (table == NULL) {
			// Initial creation of table.  Make a minimum-sized table.
			setRawCapacity(16);
		} 
		else if (entry_count * 3 > (size_mask + 1) * 2) {
			// Table is more than 2/3rds full.  Expand.
			setRawCapacity(entry_count * 2);
		}
	}

	static const uint32_t TOMBSTONE_HASH = (uint32_t) -1;

	uint32_t compute_hash(const T& key) const
	{
		H hash;
		uint32_t hash_value = hash(key);
		if (hash_value == TOMBSTONE_HASH) {
			hash_value ^= 0x8000;
		}
		return hash_value;
	}

	// Find the index of the matching entry. If no match, then return -1.
	int	findIndex(const T& key) const
	{
		if (table == NULL) return -1;
		E equal;
		uint32_t hash_value = compute_hash(key);
		int	index = hash_value & size_mask;
		const Entry * e = &entry(index);
		if (e->isEmpty()) return -1;
		if (e->isTombstone() == false && int(e->hash_value & size_mask) != index) {
			// occupied by a collider
			return -1;
		}
		for (;;)
		{
			XA_ASSERT(e->isTombstone() || (e->hash_value & size_mask) == (hash_value & size_mask));
			if (e->hash_value == hash_value && equal(e->key, key))
			{
				// Found it.
				return index;
			}
			XA_DEBUG_ASSERT(e->isTombstone() || !equal(e->key, key));   // keys are equal, but hash differs!
																	 // Keep looking through the chain.
			index = e->next_in_chain;
			if (index == -1) break;	// end of chain
			XA_ASSERT(index >= 0 && index <= size_mask);
			e = &entry(index);
			XA_ASSERT(e->isEmpty() == false || e->isTombstone());
		}
		return -1;
	}

	// Return the index of the newly cleared element.
	int removeTombstone(int index)
	{
		Entry* e = &entry(index);
		XA_ASSERT(e->isTombstone());
		XA_ASSERT(!e->isEndOfChain());
		// Move the next element of the chain into the
		// tombstone slot, and return the vacated element.
		int new_blank_index = e->next_in_chain;
		Entry* new_blank = &entry(new_blank_index);
		new (e) Entry(*new_blank);
		new_blank->clear();
		return new_blank_index;
	}

	// Helpers.
	Entry & entry(int index)
	{
		XA_DEBUG_ASSERT(table != NULL);
		XA_DEBUG_ASSERT(index >= 0 && index <= size_mask);
		return table[index];
	}

	const Entry & entry(int index) const
	{
		XA_DEBUG_ASSERT(table != NULL);
		XA_DEBUG_ASSERT(index >= 0 && index <= size_mask);
		return table[index];
	}

	// Resize the hash table to the given size (Rehash the contents of the current table).  The arg is the number of
	// hash table entries, not the number of elements we should actually contain (which will be less than this).
	void setRawCapacity(int new_size)
	{
		if (new_size <= 0) {
			// Special case.
			clear();
			return;
		}
		// Force new_size to be a power of two.
		new_size = nextPowerOfTwo(uint32_t(new_size));
		HashMap<T, U, H, E> new_hash;
		new_hash.table = (Entry *)malloc(sizeof(Entry) * new_size);
		XA_DEBUG_ASSERT(new_hash.table != NULL);
		new_hash.entry_count = 0;
		new_hash.size_mask = new_size - 1;
		for (int i = 0; i < new_size; i++)
			new_hash.entry(i).next_in_chain = -2;	// mark empty
		// Copy stuff to new_hash
		if (table != NULL)
		{
			for (int i = 0, n = size_mask; i <= n; i++)
			{
				Entry * e = &entry(i);
				if (e->isEmpty() == false && e->isTombstone() == false)
				{
					// Insert old entry into new hash.
					new_hash.add(e->key, e->value);
					e->clear();	// placement delete of old element
				}
			}
			// Delete our old data buffer.
			free(table);
		}
		// Steal new_hash's data.
		entry_count = new_hash.entry_count;
		size_mask = new_hash.size_mask;
		table = new_hash.table;
		new_hash.entry_count = 0;
		new_hash.size_mask = -1;
		new_hash.table = NULL;
	}

	// Move the enumerator to the next valid element.
	void findNext(PseudoIndex & i) const
	{
		while (i <= size_mask) {
			const Entry & e = entry(i);
			if (e.isEmpty() == false && e.isTombstone() == false) {
				break;
			}
			i++;
		}
	}

	int	entry_count;
	int	size_mask;
	Entry * table;
};

namespace halfedge {
class Face;
class Vertex;

class Edge
{
public:
	uint32_t id;
	Edge *next;
	Edge *prev;	// This is not strictly half-edge, but makes algorithms easier and faster.
	Edge *pair;
	Vertex *vertex;
	Face *face;

	// Default constructor.
	Edge(uint32_t id) : id(id), next(NULL), prev(NULL), pair(NULL), vertex(NULL), face(NULL) {}

	// Vertex queries.
	const Vertex *from() const
	{
		return vertex;
	}

	Vertex *from()
	{
		return vertex;
	}

	const Vertex *to() const
	{
		return pair->vertex;    // This used to be 'next->vertex', but that changed often when the connectivity of the mesh changes.
	}

	Vertex *to()
	{
		return pair->vertex;
	}

	// Edge queries.
	void setNext(Edge *e)
	{
		next = e;
		if (e != NULL) e->prev = this;
	}
	void setPrev(Edge *e)
	{
		prev = e;
		if (e != NULL) e->next = this;
	}

	// @@ It would be more simple to only check m_pair == NULL
	// Face queries.
	bool isBoundary() const
	{
		return !(face && pair && pair->face);
	}

	// @@ This is not exactly accurate, we should compare the texture coordinates...
	bool isSeam() const
	{
		return vertex != pair->next->vertex || next->vertex != pair->vertex;
	}

	bool isNormalSeam() const;
	bool isTextureSeam() const;

	bool isValid() const
	{
		// null face is OK.
		if (next == NULL || prev == NULL || pair == NULL || vertex == NULL) return false;
		if (next->prev != this) return false;
		if (prev->next != this) return false;
		if (pair->pair != this) return false;
		return true;
	}

	float length() const;

	// Return angle between this edge and the previous one.
	float angle() const;
};

class Vertex
{
public:
	uint32_t id;
	Edge *edge;
	Vertex *next;
	Vertex *prev;
	Vector3 pos;
	Vector3 nor;
	Vector2 tex;

	Vertex(uint32_t id) : id(id), edge(NULL), pos(0.0f), nor(0.0f), tex(0.0f)
	{
		next = this;
		prev = this;
	}

	// Set first edge of all colocals.
	void setEdge(Edge *e)
	{
		for (VertexIterator it(colocals()); !it.isDone(); it.advance()) {
			it.current()->edge = e;
		}
	}

	// Update position of all colocals.
	void setPos(const Vector3 &p)
	{
		for (VertexIterator it(colocals()); !it.isDone(); it.advance()) {
			it.current()->pos = p;
		}
	}

	bool isFirstColocal() const
	{
		return firstColocal() == this;
	}

	const Vertex *firstColocal() const
	{
		uint32_t firstId = id;
		const Vertex *vertex = this;
		for (ConstVertexIterator it(colocals()); !it.isDone(); it.advance()) {
			if (it.current()->id < firstId) {
				firstId = vertex->id;
				vertex = it.current();
			}
		}
		return vertex;
	}

	Vertex *firstColocal()
	{
		Vertex *vertex = this;
		uint32_t firstId = id;
		for (VertexIterator it(colocals()); !it.isDone(); it.advance()) {
			if (it.current()->id < firstId) {
				firstId = vertex->id;
				vertex = it.current();
			}
		}
		return vertex;
	}

	bool isColocal(const Vertex *v) const
	{
		if (this == v) return true;
		if (pos != v->pos) return false;
		for (ConstVertexIterator it(colocals()); !it.isDone(); it.advance()) {
			if (v == it.current()) {
				return true;
			}
		}
		return false;
	}

	void linkColocal(Vertex *v)
	{
		next->prev = v;
		v->next = next;
		next = v;
		v->prev = this;
	}
	void unlinkColocal()
	{
		next->prev = prev;
		prev->next = next;
		next = this;
		prev = this;
	}

	// @@ Note: This only works if linkBoundary has been called.
	bool isBoundary() const
	{
		return (edge && !edge->face);
	}

	// Iterator that visits the edges around this vertex in counterclockwise order.
	class EdgeIterator //: public Iterator<Edge *>
	{
	public:
		EdgeIterator(Edge *e) : m_end(NULL), m_current(e) { }

		virtual void advance()
		{
			if (m_end == NULL) m_end = m_current;
			m_current = m_current->pair->next;
			//m_current = m_current->prev->pair;
		}

		virtual bool isDone() const
		{
			return m_end == m_current;
		}
		virtual Edge *current() const
		{
			return m_current;
		}
		Vertex *vertex() const
		{
			return m_current->vertex;
		}

	private:
		Edge *m_end;
		Edge *m_current;
	};

	EdgeIterator edges()
	{
		return EdgeIterator(edge);
	}
	EdgeIterator edges(Edge *e)
	{
		return EdgeIterator(e);
	}

	// Iterator that visits the edges around this vertex in counterclockwise order.
	class ConstEdgeIterator //: public Iterator<Edge *>
	{
	public:
		ConstEdgeIterator() : m_end(NULL), m_current(NULL) {}
		ConstEdgeIterator(const Edge *e) : m_end(NULL), m_current(e) { }
		ConstEdgeIterator(EdgeIterator it) : m_end(NULL), m_current(it.current()) { }

		virtual void advance()
		{
			if (m_end == NULL) m_end = m_current;
			m_current = m_current->pair->next;
			//m_current = m_current->prev->pair;
		}

		virtual bool isDone() const
		{
			return m_end == m_current;
		}
		virtual const Edge *current() const
		{
			return m_current;
		}
		const Vertex *vertex() const
		{
			return m_current->to();
		}

	private:
		const Edge *m_end;
		const Edge *m_current;
	};

	ConstEdgeIterator edges() const
	{
		return ConstEdgeIterator(edge);
	}
	ConstEdgeIterator edges(const Edge *e) const
	{
		return ConstEdgeIterator(e);
	}

	// Iterator that visits all the colocal vertices.
	class VertexIterator //: public Iterator<Edge *>
	{
	public:
		VertexIterator(Vertex *v) : m_end(NULL), m_current(v) { }

		virtual void advance()
		{
			if (m_end == NULL) m_end = m_current;
			m_current = m_current->next;
		}

		virtual bool isDone() const
		{
			return m_end == m_current;
		}
		virtual Vertex *current() const
		{
			return m_current;
		}

	private:
		Vertex *m_end;
		Vertex *m_current;
	};

	VertexIterator colocals()
	{
		return VertexIterator(this);
	}

	// Iterator that visits all the colocal vertices.
	class ConstVertexIterator //: public Iterator<Edge *>
	{
	public:
		ConstVertexIterator(const Vertex *v) : m_end(NULL), m_current(v) { }

		virtual void advance()
		{
			if (m_end == NULL) m_end = m_current;
			m_current = m_current->next;
		}

		virtual bool isDone() const
		{
			return m_end == m_current;
		}
		virtual const Vertex *current() const
		{
			return m_current;
		}

	private:
		const Vertex *m_end;
		const Vertex *m_current;
	};

	ConstVertexIterator colocals() const
	{
		return ConstVertexIterator(this);
	}
};

bool Edge::isNormalSeam() const
{
	return (vertex->nor != pair->next->vertex->nor || next->vertex->nor != pair->vertex->nor);
}

bool Edge::isTextureSeam() const
{
	return (vertex->tex != pair->next->vertex->tex || next->vertex->tex != pair->vertex->tex);
}

float Edge::length() const
{
	return internal::length(to()->pos - from()->pos);
}

float Edge::angle() const
{
	Vector3 p = vertex->pos;
	Vector3 a = prev->vertex->pos;
	Vector3 b = next->vertex->pos;
	Vector3 v0 = a - p;
	Vector3 v1 = b - p;
	return acosf(dot(v0, v1) / (internal::length(v0) * internal::length(v1)));
}

struct FaceFlags
{
	enum
	{
		Ignore = 1<<0
	};
};

class Face
{
public:
	uint32_t id;
	uint16_t group;
	Edge *edge;
	uint32_t flags;

	Face(uint32_t id) : id(id), group(uint16_t(~0)), edge(NULL), flags(0) {}

	float area() const
	{
		float area = 0;
		const Vector3 &v0 = edge->from()->pos;
		for (ConstEdgeIterator it(edges(edge->next)); it.current() != edge->prev; it.advance()) {
			const Edge *e = it.current();
			const Vector3 &v1 = e->vertex->pos;
			const Vector3 &v2 = e->next->vertex->pos;
			area += length(cross(v1 - v0, v2 - v0));
		}
		return area * 0.5f;
	}

	float parametricArea() const
	{
		float area = 0;
		const Vector2 &v0 = edge->from()->tex;
		for (ConstEdgeIterator it(edges(edge->next)); it.current() != edge->prev; it.advance()) {
			const Edge *e = it.current();
			const Vector2 &v1 = e->vertex->tex;
			const Vector2 &v2 = e->next->vertex->tex;
			area += triangleArea(v0, v1, v2);
		}
		return area * 0.5f;
	}

	Vector3 normal() const
	{
		Vector3 n(0);
		const Vertex *vertex0 = NULL;
		for (ConstEdgeIterator it(edges()); !it.isDone(); it.advance()) {
			const Edge *e = it.current();
			XA_ASSERT(e != NULL);
			if (vertex0 == NULL) {
				vertex0 = e->vertex;
			} else if (e->next->vertex != vertex0) {
				const halfedge::Vertex *vertex1 = e->from();
				const halfedge::Vertex *vertex2 = e->to();
				const Vector3 &p0 = vertex0->pos;
				const Vector3 &p1 = vertex1->pos;
				const Vector3 &p2 = vertex2->pos;
				Vector3 v10 = p1 - p0;
				Vector3 v20 = p2 - p0;
				n += cross(v10, v20);
			}
		}
		return normalizeSafe(n, Vector3(0, 0, 1), 0.0f);
	}

	Vector3 centroid() const
	{
		Vector3 sum(0.0f);
		uint32_t count = 0;
		for (ConstEdgeIterator it(edges()); !it.isDone(); it.advance()) {
			const Edge *e = it.current();
			sum += e->from()->pos;
			count++;
		}
		return sum / float(count);
	}

	// Unnormalized face normal assuming it's a triangle.
	Vector3 triangleNormal() const
	{
		Vector3 p0 = edge->vertex->pos;
		Vector3 p1 = edge->next->vertex->pos;
		Vector3 p2 = edge->next->next->vertex->pos;
		Vector3 e0 = p2 - p0;
		Vector3 e1 = p1 - p0;
		return normalizeSafe(cross(e0, e1), Vector3(0), 0.0f);
	}

	Vector3 triangleNormalAreaScaled() const
	{
		Vector3 p0 = edge->vertex->pos;
		Vector3 p1 = edge->next->vertex->pos;
		Vector3 p2 = edge->next->next->vertex->pos;
		Vector3 e0 = p2 - p0;
		Vector3 e1 = p1 - p0;
		return cross(e0, e1);
	}

	// Average of the edge midpoints weighted by the edge length.
	// I want a point inside the triangle, but closer to the cirumcenter.
	Vector3 triangleCenter() const
	{
		Vector3 p0 = edge->vertex->pos;
		Vector3 p1 = edge->next->vertex->pos;
		Vector3 p2 = edge->next->next->vertex->pos;
		float l0 = length(p1 - p0);
		float l1 = length(p2 - p1);
		float l2 = length(p0 - p2);
		Vector3 m0 = (p0 + p1) * l0 / (l0 + l1 + l2);
		Vector3 m1 = (p1 + p2) * l1 / (l0 + l1 + l2);
		Vector3 m2 = (p2 + p0) * l2 / (l0 + l1 + l2);
		return m0 + m1 + m2;
	}

	bool isValid() const
	{
		uint32_t count = 0;
		for (ConstEdgeIterator it(edges()); !it.isDone(); it.advance()) {
			const Edge *e = it.current();
			if (e->face != this) return false;
			if (!e->isValid()) return false;
			if (!e->pair->isValid()) return false;
			count++;
		}
		if (count < 3) return false;
		return true;
	}

	bool contains(const Edge *e) const
	{
		for (ConstEdgeIterator it(edges()); !it.isDone(); it.advance()) {
			if (it.current() == e) return true;
		}
		return false;
	}

	uint32_t edgeCount() const
	{
		uint32_t count = 0;
		for (ConstEdgeIterator it(edges()); !it.isDone(); it.advance()) {
			++count;
		}
		return count;
	}

	// The iterator that visits the edges of this face in clockwise order.
	class EdgeIterator //: public Iterator<Edge *>
	{
	public:
		EdgeIterator(Edge *e) : m_end(NULL), m_current(e) { }

		virtual void advance()
		{
			if (m_end == NULL) m_end = m_current;
			m_current = m_current->next;
		}

		virtual bool isDone() const
		{
			return m_end == m_current;
		}
		virtual Edge *current() const
		{
			return m_current;
		}
		Vertex *vertex() const
		{
			return m_current->vertex;
		}

	private:
		Edge *m_end;
		Edge *m_current;
	};

	EdgeIterator edges()
	{
		return EdgeIterator(edge);
	}
	EdgeIterator edges(Edge *e)
	{
		XA_DEBUG_ASSERT(contains(e));
		return EdgeIterator(e);
	}

	// The iterator that visits the edges of this face in clockwise order.
	class ConstEdgeIterator //: public Iterator<const Edge *>
	{
	public:
		ConstEdgeIterator() : m_end(NULL), m_current(NULL) { }
		ConstEdgeIterator(const Edge *e) : m_end(NULL), m_current(e) { }
		ConstEdgeIterator(const EdgeIterator &it) : m_end(NULL), m_current(it.current()) { }

		virtual void advance()
		{
			if (m_end == NULL) m_end = m_current;
			m_current = m_current->next;
		}

		virtual bool isDone() const
		{
			return m_end == m_current;
		}
		virtual const Edge *current() const
		{
			return m_current;
		}
		const Vertex *vertex() const
		{
			return m_current->vertex;
		}

	private:
		const Edge *m_end;
		const Edge *m_current;
	};

	ConstEdgeIterator edges() const
	{
		return ConstEdgeIterator(edge);
	}
	ConstEdgeIterator edges(const Edge *e) const
	{
		XA_DEBUG_ASSERT(contains(e));
		return ConstEdgeIterator(e);
	}
};

/// Simple half edge mesh designed for dynamic mesh manipulation.
class Mesh
{
public:
	Mesh(uint32_t id = 0) : m_id(id), m_colocalVertexCount(0) {}

	Mesh(const Mesh *mesh)
	{
		// Copy mesh vertices.
		const uint32_t vertexCount = mesh->vertexCount();
		m_vertexArray.resize(vertexCount);
		for (uint32_t v = 0; v < vertexCount; v++) {
			const Vertex *vertex = mesh->vertexAt(v);
			XA_DEBUG_ASSERT(vertex->id == v);
			m_vertexArray[v] = new Vertex(v);
			m_vertexArray[v]->pos = vertex->pos;
			m_vertexArray[v]->nor = vertex->nor;
			m_vertexArray[v]->tex = vertex->tex;
		}
		m_colocalVertexCount = vertexCount;
		// Copy mesh faces.
		const uint32_t faceCount = mesh->faceCount();
		Array<uint32_t> indexArray;
		indexArray.reserve(3);
		for (uint32_t f = 0; f < faceCount; f++) {
			const Face *face = mesh->faceAt(f);
			Face::ConstEdgeIterator it(face->edges());
			for (; !it.isDone(); it.advance()) {
				const Vertex *vertex = it.current()->from();
				indexArray.push_back(vertex->id);
			}
			addFace(indexArray);
			indexArray.clear();
		}
	}

	~Mesh()
	{
		clear();
	}

	void clear()
	{
		for (size_t i = 0; i < m_vertexArray.size(); i++)
			delete m_vertexArray[(uint32_t)i];
		m_vertexArray.clear();
		for (EdgeMap::PseudoIndex it = m_edgeMap.start(); !m_edgeMap.isDone(it); m_edgeMap.advance(it))
			delete m_edgeMap[it].value;
		m_edgeArray.clear();
		m_edgeMap.clear();
		for (size_t i = 0; i < m_faceArray.size(); i++)
			delete m_faceArray[(uint32_t)i];
		m_faceArray.clear();
	}

	Vertex *addVertex(const Vector3 &pos)
	{
		XA_DEBUG_ASSERT(isFinite(pos));
		Vertex *v = new Vertex(m_vertexArray.size());
		v->pos = pos;
		m_vertexArray.push_back(v);
		return v;
	}

	/// Link colocal vertices based on geometric location only.
	void linkColocals()
	{
		XA_PRINT(PrintFlags::MeshCreation, "--- Linking colocals:\n");
		const uint32_t vertexCount = this->vertexCount();
		HashMap<Vector3, Vertex *, Hash<Vector3>, Equal<Vector3> > vertexMap(vertexCount);
		for (uint32_t v = 0; v < vertexCount; v++) {
			Vertex *vertex = vertexAt(v);
			Vertex *colocal;
			if (vertexMap.get(vertex->pos, &colocal))
				colocal->linkColocal(vertex);
			else
				vertexMap.add(vertex->pos, vertex);
		}
		m_colocalVertexCount = vertexMap.count();
		XA_PRINT(PrintFlags::MeshCreation, "---   %d vertex positions.\n", m_colocalVertexCount);
		// @@ Remove duplicated vertices? or just leave them as colocals?
	}

	void linkColocalsWithCanonicalMap(const Array<uint32_t> &canonicalMap)
	{
		XA_PRINT(PrintFlags::MeshCreation, "--- Linking colocals:\n");
		uint32_t vertexMapSize = 0;
		for (uint32_t i = 0; i < canonicalMap.size(); i++) {
			vertexMapSize = std::max(vertexMapSize, canonicalMap[i] + 1);
		}
		Array<Vertex *> vertexMap;
		vertexMap.resize(vertexMapSize, NULL);
		m_colocalVertexCount = 0;
		const uint32_t vertexCount = this->vertexCount();
		for (uint32_t v = 0; v < vertexCount; v++) {
			Vertex *vertex = vertexAt(v);
			Vertex *colocal = vertexMap[canonicalMap[v]];
			if (colocal != NULL) {
				XA_DEBUG_ASSERT(vertex->pos == colocal->pos);
				colocal->linkColocal(vertex);
			} else {
				vertexMap[canonicalMap[v]] = vertex;
				m_colocalVertexCount++;
			}
		}
		XA_PRINT(PrintFlags::MeshCreation, "---   %d vertex positions.\n", m_colocalVertexCount);
	}

	Face *addFace()
	{
		Face *f = new Face(m_faceArray.size());
		m_faceArray.push_back(f);
		return f;
	}

	Face *addFace(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t flags = 0)
	{
		uint32_t indexArray[3];
		indexArray[0] = v0;
		indexArray[1] = v1;
		indexArray[2] = v2;
		return addFace(indexArray, 3, 0, 3, flags);
	}

	Face *addFace(const Array<uint32_t> &indexArray, uint32_t flags = 0)
	{
		return addFace(indexArray, 0, indexArray.size(), flags);
	}

	Face *addFace(const Array<uint32_t> &indexArray, uint32_t first, uint32_t num, uint32_t flags = 0)
	{
		return addFace(indexArray.data(), (uint32_t)indexArray.size(), first, num, flags);
	}

	Face *addFace(const uint32_t *indexArray, uint32_t indexCount, uint32_t first, uint32_t num, uint32_t flags = 0)
	{
		XA_DEBUG_ASSERT(first < indexCount);
		XA_DEBUG_ASSERT(num <= indexCount - first);
#ifdef NDEBUG
		indexCount = indexCount; // silence unused parameter warning
#endif
		XA_DEBUG_ASSERT(num > 2);
		Face *f = new Face(m_faceArray.size());
		for (uint32_t j = num - 1, i = 0; i < num; j = i++) {
			const uint32_t edgeIndex0 = indexArray[first + j];
			const uint32_t edgeIndex1 = indexArray[first + i];
			// Make sure edge has not been added yet.
			Edge *edge = findEdge(edgeIndex0, edgeIndex1);
			// We ignore edges that don't have an adjacent face yet, since this face could become the edge's face.
			if (edge && edge->face) {
				// Unlink colocals so this edge can be added.
				Vertex *v0 = vertexAt(edgeIndex0);
				Vertex *v1 = vertexAt(edgeIndex1);
				v0->unlinkColocal();
				v1->unlinkColocal();
				edge = findEdge(edgeIndex0, edgeIndex1);
				if (edge && edge->face)
					XA_PRINT(PrintFlags::MeshWarnings, "Mesh %d duplicate edge: index %d, index %d\n", m_id, edgeIndex0, edgeIndex1);
			}
		}
		// We also have to make sure the face does not have any duplicate edge!
		for (uint32_t i = 0; i < num; i++) {
			int i0 = indexArray[first + i + 0];
			int i1 = indexArray[first + (i + 1) % num];
			for (uint32_t j = i + 1; j < num; j++) {
				int j0 = indexArray[first + j + 0];
				int j1 = indexArray[first + (j + 1) % num];
				if (i0 == j0 && i1 == j1)
					XA_PRINT(PrintFlags::MeshWarnings, "Mesh %d duplicate face edge: index %d, index %d\n", m_id, i0, i1);
			}
		}
		Edge *firstEdge = NULL;
		Edge *last = NULL;
		Edge *current = NULL;
		for (uint32_t i = 0; i < num - 1; i++) {
			current = addEdge(indexArray[first + i], indexArray[first + i + 1]);
			XA_ASSERT(current != NULL && current->face == NULL);
			current->face = f;
			if (last != NULL) last->setNext(current);
			else firstEdge = current;
			last = current;
		}
		current = addEdge(indexArray[first + num - 1], indexArray[first]);
		XA_ASSERT(current != NULL && current->face == NULL);
		current->face = f;
		last->setNext(current);
		current->setNext(firstEdge);
		f->edge = firstEdge;
		f->flags = flags;
		m_faceArray.push_back(f);
		return f;
	}

	// These functions disconnect the given element from the mesh and delete it.

	// @@ We must always disconnect edge pairs simultaneously.
	void disconnect(Edge *edge)
	{
		XA_DEBUG_ASSERT(edge != NULL);
		// Remove from edge list.
		if ((edge->id & 1) == 0) {
			XA_DEBUG_ASSERT(m_edgeArray[edge->id / 2] == edge);
			m_edgeArray[edge->id / 2] = NULL;
		}
		// Remove edge from map. @@ Store map key inside edge?
		XA_DEBUG_ASSERT(edge->from() != NULL && edge->to() != NULL);
		bool removed = m_edgeMap.remove(Key(edge->from()->id, edge->to()->id));
		XA_DEBUG_ASSERT(removed == true);
#ifdef NDEBUG
		removed = removed; // silence unused parameter warning
#endif
		// Disconnect from vertex.
		if (edge->vertex != NULL) {
			if (edge->vertex->edge == edge) {
				if (edge->prev && edge->prev->pair) {
					edge->vertex->edge = edge->prev->pair;
				} else if (edge->pair && edge->pair->next) {
					edge->vertex->edge = edge->pair->next;
				} else {
					edge->vertex->edge = NULL;
					// @@ Remove disconnected vertex?
				}
			}
		}
		// Disconnect from face.
		if (edge->face != NULL) {
			if (edge->face->edge == edge) {
				if (edge->next != NULL && edge->next != edge) {
					edge->face->edge = edge->next;
				} else if (edge->prev != NULL && edge->prev != edge) {
					edge->face->edge = edge->prev;
				} else {
					edge->face->edge = NULL;
					// @@ Remove disconnected face?
				}
			}
		}
		// Disconnect from previous.
		if (edge->prev) {
			if (edge->prev->next == edge) {
				edge->prev->setNext(NULL);
			}
			//edge->setPrev(NULL);
		}
		// Disconnect from next.
		if (edge->next) {
			if (edge->next->prev == edge) {
				edge->next->setPrev(NULL);
			}
			//edge->setNext(NULL);
		}
	}

	void remove(Edge *edge)
	{
		XA_DEBUG_ASSERT(edge != NULL);
		disconnect(edge);
		delete edge;
	}

	void remove(Vertex *vertex)
	{
		XA_DEBUG_ASSERT(vertex != NULL);
		// Remove from vertex list.
		m_vertexArray[vertex->id] = NULL;
		// Disconnect from colocals.
		vertex->unlinkColocal();
		// Disconnect from edges.
		if (vertex->edge != NULL) {
			// @@ Removing a connected vertex is asking for trouble...
			if (vertex->edge->vertex == vertex) {
				// @@ Connect edge to a colocal?
				vertex->edge->vertex = NULL;
			}
			vertex->setEdge(NULL);
		}
		delete vertex;
	}

	void remove(Face *face)
	{
		XA_DEBUG_ASSERT(face != NULL);
		// Remove from face list.
		m_faceArray[face->id] = NULL;
		// Disconnect from edges.
		if (face->edge != NULL) {
			XA_DEBUG_ASSERT(face->edge->face == face);
			face->edge->face = NULL;
			face->edge = NULL;
		}
		delete face;
	}

	// Triangulate in place.
	void triangulate()
	{
		bool all_triangles = true;
		const uint32_t faceCount = m_faceArray.size();
		for (uint32_t f = 0; f < faceCount; f++) {
			Face *face = m_faceArray[f];
			if (face->edgeCount() != 3) {
				all_triangles = false;
				break;
			}
		}
		if (all_triangles) {
			return;
		}
		// Do not touch vertices, but rebuild edges and faces.
		Array<Edge *> edgeArray;
		Array<Face *> faceArray;
		std::swap(edgeArray, m_edgeArray);
		std::swap(faceArray, m_faceArray);
		m_edgeMap.clear();
		for (uint32_t f = 0; f < faceCount; f++) {
			Face *face = faceArray[f];
			// Trivial fan-like triangulation.
			const uint32_t v0 = face->edge->vertex->id;
			uint32_t v2, v1 = (uint32_t)-1;
			for (Face::EdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
				Edge *edge = it.current();
				v2 = edge->to()->id;
				if (v2 == v0) break;
				if (v1 != (uint32_t)-1) addFace(v0, v1, v2);
				v1 = v2;
			}
		}
		XA_DEBUG_ASSERT(m_faceArray.size() > faceCount); // triangle count > face count
		linkBoundary();
		for (size_t i = 0; i < edgeArray.size(); i++)
			delete edgeArray[(uint32_t)i];
		for (size_t i = 0; i < faceArray.size(); i++)
			delete faceArray[(uint32_t)i];
	}

	/// Link boundary edges once the mesh has been created.
	void linkBoundary()
	{
		XA_PRINT(PrintFlags::MeshProcessing, "--- Linking boundaries:\n");
		int num = 0;
		// Create boundary edges.
		uint32_t edgeCount = this->edgeCount();
		for (uint32_t e = 0; e < edgeCount; e++) {
			Edge *edge = edgeAt(e);
			if (edge != NULL && edge->pair == NULL) {
				if (edge->face && edge->face->flags & FaceFlags::Ignore)
					continue;
				Edge *pair = new Edge(edge->id + 1);
				uint32_t i = edge->from()->id;
				uint32_t j = edge->next->from()->id;
				pair->vertex = m_vertexArray[j];
				Key key(j, i);
				if (!m_edgeMap.get(key))
					m_edgeMap.add(key, pair);
				edge->pair = pair;
				pair->pair = edge;
				num++;
			}
		}
		// Link boundary edges.
		for (uint32_t e = 0; e < edgeCount; e++) {
			Edge *edge = edgeAt(e);
			if (edge != NULL) {
				if (edge->face && edge->face->flags & FaceFlags::Ignore)
					continue;
				if (edge->pair->face == NULL)
					linkBoundaryEdge(edge->pair);
			}
		}
		XA_PRINT(PrintFlags::MeshProcessing, "---   %d boundary edges.\n", num);
	}

	/*
	Fixing T-junctions.

	- Find T-junctions. Find  vertices that are on an edge.
		- This test is approximate.
		- Insert edges on a spatial index to speedup queries.
		- Consider only open edges, that is edges that have no pairs.
		- Consider only vertices on boundaries.
	- Close T-junction.
		- Split edge.

	*/
	bool splitBoundaryEdges() // Returns true if any split was made.
	{
		Array<Vertex *> boundaryVertices;
		for (uint32_t i = 0; i < m_vertexArray.size(); i++) {
			Vertex *v = m_vertexArray[i];
			if (v->isBoundary()) {
				boundaryVertices.push_back(v);
			}
		}
		XA_PRINT(PrintFlags::MeshProcessing, "Fixing T-junctions:\n");
		int splitCount = 0;
		for (uint32_t v = 0; v < boundaryVertices.size(); v++) {
			Vertex *vertex = boundaryVertices[v];
			Vector3 x0 = vertex->pos;
			// Find edges that this vertex overlaps with.
			for (uint32_t e = 0; e < m_edgeArray.size(); e++) {
				Edge *edge = m_edgeArray[e];
				if (edge != NULL && edge->isBoundary()) {
					if (edge->from() == vertex || edge->to() == vertex) {
						continue;
					}
					Vector3 x1 = edge->from()->pos;
					Vector3 x2 = edge->to()->pos;
					Vector3 v01 = x0 - x1;
					Vector3 v21 = x2 - x1;
					float l = length(v21);
					float d = length(cross(v01, v21)) / l;
					if (isZero(d)) {
						float t = dot(v01, v21) / (l * l);
						if (t > 0.0f + XA_EPSILON && t < 1.0f - XA_EPSILON) {
							XA_DEBUG_ASSERT(lerp(x1, x2, t) == x0);
							Vertex *splitVertex = splitBoundaryEdge(edge, t, x0);
							vertex->linkColocal(splitVertex);   // @@ Should we do this here?
							splitCount++;
						}
					}
				}
			}
		}
		XA_PRINT(PrintFlags::MeshProcessing, " - %d edges split.\n", splitCount);
		XA_DEBUG_ASSERT(isValid());
		return splitCount != 0;
	}

	// Vertices
	uint32_t vertexCount() const
	{
		return m_vertexArray.size();
	}

	const Vertex *vertexAt(int i) const
	{
		return m_vertexArray[i];
	}

	Vertex *vertexAt(int i)
	{
		return m_vertexArray[i];
	}

	uint32_t colocalVertexCount() const
	{
		return m_colocalVertexCount;
	}

	// Faces
	uint32_t faceCount() const
	{
		return m_faceArray.size();
	}

	const Face *faceAt(int i) const
	{
		return m_faceArray[i];
	}

	Face *faceAt(int i)
	{
		return m_faceArray[i];
	}

	// Edges
	uint32_t edgeCount() const
	{
		return m_edgeArray.size();
	}

	const Edge *edgeAt(int i) const
	{
		return m_edgeArray[i];
	}

	Edge *edgeAt(int i)
	{
		return m_edgeArray[i];
	}

	class ConstVertexIterator;

	class VertexIterator
	{
		friend class ConstVertexIterator;
	public:
		VertexIterator(Mesh *mesh) : m_mesh(mesh), m_current(0) { }

		virtual void advance()
		{
			m_current++;
		}
		virtual bool isDone() const
		{
			return m_current == m_mesh->vertexCount();
		}
		virtual Vertex *current() const
		{
			return m_mesh->vertexAt(m_current);
		}

	private:
		halfedge::Mesh *m_mesh;
		uint32_t m_current;
	};

	VertexIterator vertices()
	{
		return VertexIterator(this);
	}

	class ConstVertexIterator
	{
	public:
		ConstVertexIterator(const Mesh *mesh) : m_mesh(mesh), m_current(0) { }
		ConstVertexIterator(class VertexIterator &it) : m_mesh(it.m_mesh), m_current(it.m_current) { }

		virtual void advance()
		{
			m_current++;
		}
		virtual bool isDone() const
		{
			return m_current == m_mesh->vertexCount();
		}
		virtual const Vertex *current() const
		{
			return m_mesh->vertexAt(m_current);
		}

	private:
		const halfedge::Mesh *m_mesh;
		uint32_t m_current;
	};

	ConstVertexIterator vertices() const
	{
		return ConstVertexIterator(this);
	}

	class ConstFaceIterator;

	class FaceIterator
	{
		friend class ConstFaceIterator;
	public:
		FaceIterator(Mesh *mesh) : m_mesh(mesh), m_current(0) { }

		virtual void advance()
		{
			m_current++;
		}
		virtual bool isDone() const
		{
			return m_current == m_mesh->faceCount();
		}
		virtual Face *current() const
		{
			return m_mesh->faceAt(m_current);
		}

	private:
		halfedge::Mesh *m_mesh;
		uint32_t m_current;
	};

	FaceIterator faces()
	{
		return FaceIterator(this);
	}

	class ConstFaceIterator
	{
	public:
		ConstFaceIterator(const Mesh *mesh) : m_mesh(mesh), m_current(0) { }
		ConstFaceIterator(const FaceIterator &it) : m_mesh(it.m_mesh), m_current(it.m_current) { }

		virtual void advance()
		{
			m_current++;
		}
		virtual bool isDone() const
		{
			return m_current == m_mesh->faceCount();
		}
		virtual const Face *current() const
		{
			return m_mesh->faceAt(m_current);
		}

	private:
		const halfedge::Mesh *m_mesh;
		uint32_t m_current;
	};

	ConstFaceIterator faces() const
	{
		return ConstFaceIterator(this);
	}

	class ConstEdgeIterator;

	class EdgeIterator
	{
		friend class ConstEdgeIterator;
	public:
		EdgeIterator(Mesh *mesh) : m_mesh(mesh), m_current(0) { }

		virtual void advance()
		{
			m_current++;
		}
		virtual bool isDone() const
		{
			return m_current == m_mesh->edgeCount();
		}
		virtual Edge *current() const
		{
			return m_mesh->edgeAt(m_current);
		}

	private:
		halfedge::Mesh *m_mesh;
		uint32_t m_current;
	};

	EdgeIterator edges()
	{
		return EdgeIterator(this);
	}

	class ConstEdgeIterator
	{
	public:
		ConstEdgeIterator(const Mesh *mesh) : m_mesh(mesh), m_current(0) { }
		ConstEdgeIterator(const EdgeIterator &it) : m_mesh(it.m_mesh), m_current(it.m_current) { }

		virtual void advance()
		{
			m_current++;
		}
		virtual bool isDone() const
		{
			return m_current == m_mesh->edgeCount();
		}
		virtual const Edge *current() const
		{
			return m_mesh->edgeAt(m_current);
		}

	private:
		const halfedge::Mesh *m_mesh;
		uint32_t m_current;
	};

	ConstEdgeIterator edges() const
	{
		return ConstEdgeIterator(this);
	}

	// @@ Add half-edge iterator.

	bool isValid() const
	{
		// Make sure all edges are valid.
		const uint32_t edgeCount = m_edgeArray.size();
		for (uint32_t e = 0; e < edgeCount; e++) {
			Edge *edge = m_edgeArray[e];
			if (edge != NULL) {
				if (edge->id != 2 * e) {
					return false;
				}
				if (!edge->isValid()) {
					return false;
				}
				if (edge->pair->id != 2 * e + 1) {
					return false;
				}
				if (!edge->pair->isValid()) {
					return false;
				}
			}
		}
		// @@ Make sure all faces are valid.
		// @@ Make sure all vertices are valid.
		return true;
	}

private:
	Edge *addEdge(uint32_t i, uint32_t j)
	{
		// Add new edge.
		// Lookup pair.
		Edge *edge = NULL;
		Edge *pair = findEdge(j, i);
		if (pair != NULL) {
			// Create edge with same id.
			edge = new Edge(pair->id + 1);
			// Link edge pairs.
			edge->pair = pair;
			pair->pair = edge;
			// @@ I'm not sure this is necessary!
			pair->vertex->setEdge(pair);
		} else {
			// Create edge.
			edge = new Edge(2 * m_edgeArray.size());
			// Add only unpaired edges.
			m_edgeArray.push_back(edge);
		}
		edge->vertex = m_vertexArray[i];
		Key key(i, j);
		if (!m_edgeMap.get(key))
			m_edgeMap.add(key, edge);
		// Face and Next are set by addFace.
		return edge;
	}

	/// Find edge, test all colocals.
	Edge *findEdge(uint32_t i, uint32_t j) const
	{
		const Vertex *v0 = vertexAt(i);
		const Vertex *v1 = vertexAt(j);
		// Test all colocal pairs.
		for (Vertex::ConstVertexIterator it0(v0->colocals()); !it0.isDone(); it0.advance()) {
			for (Vertex::ConstVertexIterator it1(v1->colocals()); !it1.isDone(); it1.advance()) {
				Key key(it0.current()->id, it1.current()->id);
				Edge *edge = NULL;
				if (m_edgeMap.get(key, &edge))
					return edge;
			}
		}
		return NULL;
	}

	/// Link this boundary edge.
	void linkBoundaryEdge(Edge *edge)
	{
		XA_ASSERT(edge->face == NULL);
		// Make sure next pointer has not been set. @@ We want to be able to relink boundary edges after mesh changes.
		Edge *next = edge;
		while (next->pair->face != NULL) {
			if (next->pair->face->flags & FaceFlags::Ignore)
				break;
			// Get pair prev
			Edge *e = next->pair->next;
			while (e->next != next->pair) {
				e = e->next;
			}
			next = e;
		}
		edge->setNext(next->pair);
		// Adjust vertex edge, so that it's the boundary edge. (required for isBoundary())
		if (edge->vertex->edge != edge) {
			// Multiple boundaries in the same edge.
			edge->vertex->edge = edge;
		}
	}

	Vertex *splitBoundaryEdge(Edge *edge, float t, const Vector3 &pos)
	{
		/*
		  We want to go from this configuration:

				+   +
				|   ^
		   edge |<->|  pair
				v   |
				+   +

		  To this one:

				+   +
				|   ^
			 e0 |<->| p0
				v   |
		 vertex +   +
				|   ^
			 e1 |<->| p1
				v   |
				+   +

		*/
		Edge *pair = edge->pair;
		// Make sure boundaries are linked.
		XA_DEBUG_ASSERT(pair != NULL);
		// Make sure edge is a boundary edge.
		XA_DEBUG_ASSERT(pair->face == NULL);
		// Add new vertex.
		Vertex *vertex = addVertex(pos);
		vertex->nor = lerp(edge->from()->nor, edge->to()->nor, t);
		vertex->tex = lerp(edge->from()->tex, edge->to()->tex, t);
		disconnect(edge);
		disconnect(pair);
		// Add edges.
		Edge *e0 = addEdge(edge->from()->id, vertex->id);
		Edge *p0 = addEdge(vertex->id, pair->to()->id);
		Edge *e1 = addEdge(vertex->id, edge->to()->id);
		Edge *p1 = addEdge(pair->from()->id, vertex->id);
		// Link edges.
		e0->setNext(e1);
		p1->setNext(p0);
		e0->setPrev(edge->prev);
		e1->setNext(edge->next);
		p1->setPrev(pair->prev);
		p0->setNext(pair->next);
		XA_DEBUG_ASSERT(e0->next == e1);
		XA_DEBUG_ASSERT(e1->prev == e0);
		XA_DEBUG_ASSERT(p1->next == p0);
		XA_DEBUG_ASSERT(p0->prev == p1);
		XA_DEBUG_ASSERT(p0->pair == e0);
		XA_DEBUG_ASSERT(e0->pair == p0);
		XA_DEBUG_ASSERT(p1->pair == e1);
		XA_DEBUG_ASSERT(e1->pair == p1);
		// Link faces.
		e0->face = edge->face;
		e1->face = edge->face;
		// Link vertices.
		edge->from()->setEdge(e0);
		vertex->setEdge(e1);
		delete edge;
		delete pair;
		return vertex;
	}

private:
	uint32_t m_id;
	Array<Vertex *> m_vertexArray;
	Array<Edge *> m_edgeArray;
	Array<Face *> m_faceArray;

	struct Key
	{
		Key() {}
		Key(const Key &k) : p0(k.p0), p1(k.p1) {}
		Key(uint32_t v0, uint32_t v1) : p0(v0), p1(v1) {}
		void operator=(const Key &k)
		{
			p0 = k.p0;
			p1 = k.p1;
		}
		bool operator==(const Key &k) const
		{
			return p0 == k.p0 && p1 == k.p1;
		}

		uint32_t p0;
		uint32_t p1;
	};

	friend struct Hash<Mesh::Key>;
	typedef HashMap<Key, Edge *, Hash<Key>, Equal<Key> > EdgeMap;
	EdgeMap m_edgeMap;
	uint32_t m_colocalVertexCount;
};

class MeshTopology
{
public:
	MeshTopology(const Mesh *mesh)
	{
		buildTopologyInfo(mesh);
	}

	/// Determine if the mesh is connected.
	bool isConnected() const
	{
		return m_connectedCount == 1;
	}

	/// Determine if the mesh is closed. (Each edge is shared by two faces)
	bool isClosed() const
	{
		return m_boundaryCount == 0;
	}

	/// Return true if the mesh has the topology of a disk.
	bool isDisk() const
	{
		return isConnected() && m_boundaryCount == 1/* && m_eulerNumber == 1*/;
	}

private:
	void buildTopologyInfo(const Mesh *mesh)
	{
		const uint32_t vertexCount = mesh->colocalVertexCount();
		const uint32_t faceCount = mesh->faceCount();
		const uint32_t edgeCount = mesh->edgeCount();
		XA_PRINT(PrintFlags::ComputingCharts, "--- Building mesh topology:\n" );
		Array<uint32_t> stack(faceCount);
		BitArray bitFlags(faceCount);
		bitFlags.clearAll();
		// Compute connectivity.
		XA_PRINT(PrintFlags::ComputingCharts, "---   Computing connectivity.\n" );
		m_connectedCount = 0;
		for (uint32_t f = 0; f < faceCount; f++ ) {
			if ( bitFlags.bitAt(f) == false ) {
				m_connectedCount++;
				stack.push_back( f );
				while ( !stack.isEmpty() ) {
					const uint32_t top = stack.back();
					XA_ASSERT(top != uint32_t(~0));
					stack.pop_back();
					if ( bitFlags.bitAt(top) == false ) {
						bitFlags.setBitAt(top);
						const Face *face = mesh->faceAt(top);
						const Edge *firstEdge = face->edge;
						const Edge *edge = firstEdge;
						do {
							const Face *neighborFace = edge->pair->face;
							if (neighborFace != NULL) {
								stack.push_back(neighborFace->id);
							}
							edge = edge->next;
						} while (edge != firstEdge);
					}
				}
			}
		}
		XA_ASSERT(stack.isEmpty());
		XA_PRINT(PrintFlags::ComputingCharts, "---   %d connected components.\n", m_connectedCount );
		// Count boundary loops.
		XA_PRINT(PrintFlags::ComputingCharts, "---   Counting boundary loops.\n" );
		m_boundaryCount = 0;
		bitFlags.resize(edgeCount);
		bitFlags.clearAll();
		// Don't forget to link the boundary otherwise this won't work.
		for (uint32_t e = 0; e < edgeCount; e++) {
			const Edge *startEdge = mesh->edgeAt(e);
			if (startEdge != NULL && startEdge->isBoundary() && bitFlags.bitAt(e) == false) {
				XA_DEBUG_ASSERT(startEdge->face != NULL);
				XA_DEBUG_ASSERT(startEdge->pair->face == NULL);
				startEdge = startEdge->pair;
				m_boundaryCount++;
				const Edge *edge = startEdge;
				do {
					bitFlags.setBitAt(edge->id / 2);
					edge = edge->next;
				} while (startEdge != edge);
			}
		}
		XA_PRINT(PrintFlags::ComputingCharts, "---   %d boundary loops found.\n", m_boundaryCount );
		// Compute euler number.
		m_eulerNumber = vertexCount - edgeCount + faceCount;
		XA_PRINT(PrintFlags::ComputingCharts, "---   Euler number: %d.\n", m_eulerNumber);
		// Compute genus. (only valid on closed connected surfaces)
		m_genus = -1;
		if ( isClosed() && isConnected() ) {
			m_genus = (2 - m_eulerNumber) / 2;
			XA_PRINT(PrintFlags::ComputingCharts, "---   Genus: %d.\n", m_genus);
		}
	}

private:
	///< Number of boundary loops.
	int m_boundaryCount;

	///< Number of connected components.
	int m_connectedCount;

	///< Euler number.
	int m_eulerNumber;

	/// Mesh genus.
	int m_genus;
};

static float computeSurfaceArea(const halfedge::Mesh *mesh)
{
	float area = 0;
	for (halfedge::Mesh::ConstFaceIterator it(mesh->faces()); !it.isDone(); it.advance()) {
		const halfedge::Face *face = it.current();
		area += face->area();
	}
	XA_DEBUG_ASSERT(area >= 0);
	return area;
}

static float computeParametricArea(const halfedge::Mesh *mesh)
{
	float area = 0;
	for (halfedge::Mesh::ConstFaceIterator it(mesh->faces()); !it.isDone(); it.advance()) {
		const halfedge::Face *face = it.current();
		area += face->parametricArea();
	}
	return area;
}

static uint32_t countMeshTriangles(const Mesh *mesh)
{
	const uint32_t faceCount = mesh->faceCount();
	uint32_t triangleCount = 0;
	for (uint32_t f = 0; f < faceCount; f++) {
		const Face *face = mesh->faceAt(f);
		uint32_t edgeCount = face->edgeCount();
		XA_DEBUG_ASSERT(edgeCount > 2);
		triangleCount += edgeCount - 2;
	}
	return triangleCount;
}

static Mesh *unifyVertices(const Mesh *inputMesh)
{
	Mesh *mesh = new Mesh;
	// Only add the first colocal.
	const uint32_t vertexCount = inputMesh->vertexCount();
	for (uint32_t v = 0; v < vertexCount; v++) {
		const Vertex *vertex = inputMesh->vertexAt(v);
		if (vertex->isFirstColocal()) {
			mesh->addVertex(vertex->pos);
		}
	}
	Array<uint32_t> indexArray;
	// Add new faces pointing to first colocals.
	uint32_t faceCount = inputMesh->faceCount();
	for (uint32_t f = 0; f < faceCount; f++) {
		const Face *face = inputMesh->faceAt(f);
		indexArray.clear();
		for (Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
			const Edge *edge = it.current();
			const Vertex *vertex = edge->vertex->firstColocal();
			indexArray.push_back(vertex->id);
		}
		mesh->addFace(indexArray, face->flags);
	}
	mesh->linkBoundary();
	return mesh;
}

static bool pointInTriangle(const Vector2 &p, const Vector2 &a, const Vector2 &b, const Vector2 &c)
{
	return triangleArea(a, b, p) >= 0.00001f && triangleArea(b, c, p) >= 0.00001f && triangleArea(c, a, p) >= 0.00001f;
}

// This is doing a simple ear-clipping algorithm that skips invalid triangles. Ideally, we should
// also sort the ears by angle, start with the ones that have the smallest angle and proceed in order.
static Mesh *triangulate(const Mesh *inputMesh)
{
	Mesh *mesh = new Mesh;
	// Add all vertices.
	const uint32_t vertexCount = inputMesh->vertexCount();
	for (uint32_t v = 0; v < vertexCount; v++) {
		const Vertex *vertex = inputMesh->vertexAt(v);
		mesh->addVertex(vertex->pos);
	}
	Array<int> polygonVertices;
	Array<float> polygonAngles;
	Array<Vector2> polygonPoints;
	const uint32_t faceCount = inputMesh->faceCount();
	for (uint32_t f = 0; f < faceCount; f++) {
		const Face *face = inputMesh->faceAt(f);
		XA_DEBUG_ASSERT(face != NULL);
		const uint32_t edgeCount = face->edgeCount();
		XA_DEBUG_ASSERT(edgeCount >= 3);
		polygonVertices.clear();
		polygonVertices.reserve(edgeCount);
		if (edgeCount == 3) {
			// Simple case for triangles.
			for (Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
				const Edge *edge = it.current();
				const Vertex *vertex = edge->vertex;
				polygonVertices.push_back(vertex->id);
			}
			int v0 = polygonVertices[0];
			int v1 = polygonVertices[1];
			int v2 = polygonVertices[2];
			mesh->addFace(v0, v1, v2);
		} else {
			// Build 2D polygon projecting vertices onto normal plane.
			// Faces are not necesarily planar, this is for example the case, when the face comes from filling a hole. In such cases
			// it's much better to use the best fit plane.
			const Vector3 fn = face->normal();
			Basis basis;
			basis.buildFrameForDirection(fn);
			polygonPoints.clear();
			polygonPoints.reserve(edgeCount);
			polygonAngles.clear();
			polygonAngles.reserve(edgeCount);
			for (Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
				const Edge *edge = it.current();
				const Vertex *vertex = edge->vertex;
				polygonVertices.push_back(vertex->id);
				Vector2 p;
				p.x = dot(basis.tangent, vertex->pos);
				p.y = dot(basis.bitangent, vertex->pos);
				polygonPoints.push_back(p);
			}
			polygonAngles.resize(edgeCount);
			while (polygonVertices.size() > 2) {
				uint32_t size = polygonVertices.size();
				// Update polygon angles. @@ Update only those that have changed.
				float minAngle = float(2 * M_PI);
				uint32_t bestEar = 0; // Use first one if none of them is valid.
				bool bestIsValid = false;
				for (uint32_t i = 0; i < size; i++) {
					uint32_t i0 = i;
					uint32_t i1 = (i + 1) % size; // Use Sean's polygon interation trick.
					uint32_t i2 = (i + 2) % size;
					Vector2 p0 = polygonPoints[i0];
					Vector2 p1 = polygonPoints[i1];
					Vector2 p2 = polygonPoints[i2];
					float d = clamp(dot(p0 - p1, p2 - p1) / (length(p0 - p1) * length(p2 - p1)), -1.0f, 1.0f);
					float angle = acosf(d);
					float area = triangleArea(p0, p1, p2);
					if (area < 0.0f) angle = float(2.0f * M_PI - angle);
					polygonAngles[i1] = angle;
					if (angle < minAngle || !bestIsValid) {
						// Make sure this is a valid ear, if not, skip this point.
						bool valid = true;
						for (uint32_t j = 0; j < size; j++) {
							if (j == i0 || j == i1 || j == i2) continue;
							Vector2 p = polygonPoints[j];
							if (pointInTriangle(p, p0, p1, p2)) {
								valid = false;
								break;
							}
						}
						if (valid || !bestIsValid) {
							minAngle = angle;
							bestEar = i1;
							bestIsValid = valid;
						}
					}
				}
				if (!(face->flags & FaceFlags::Ignore))
				{
					XA_DEBUG_ASSERT(minAngle <= 2 * M_PI);
				}
				// Clip best ear:
				uint32_t i0 = (bestEar + size - 1) % size;
				uint32_t i1 = (bestEar + 0) % size;
				uint32_t i2 = (bestEar + 1) % size;
				int v0 = polygonVertices[i0];
				int v1 = polygonVertices[i1];
				int v2 = polygonVertices[i2];
				mesh->addFace(v0, v1, v2);
				polygonVertices.removeAt(i1);
				polygonPoints.removeAt(i1);
				polygonAngles.removeAt(i1);
			}
		}
	}
	mesh->linkBoundary();
	return mesh;
}

} //  namespace halfedge

/// Mersenne twister random number generator.
class MTRand
{
public:
	enum time_e { Time };
	enum { N = 624 };       // length of state vector
	enum { M = 397 };

	/// Constructor that uses the current time as the seed.
	MTRand( time_e )
	{
		seed((uint32_t )time(NULL));
	}

	/// Constructor that uses the given seed.
	MTRand( uint32_t s = 0 )
	{
		seed(s);
	}

	/// Provide a new seed.
	void seed( uint32_t s )
	{
		initialize(s);
		reload();
	}

	/// Get a random number between 0 - 65536.
	uint32_t get()
	{
		// Pull a 32-bit integer from the generator state
		// Every other access function simply transforms the numbers extracted here
		if ( left == 0 ) {
			reload();
		}
		left--;
		uint32_t s1;
		s1 = *next++;
		s1 ^= (s1 >> 11);
		s1 ^= (s1 <<  7) & 0x9d2c5680U;
		s1 ^= (s1 << 15) & 0xefc60000U;
		return ( s1 ^ (s1 >> 18) );
	};

	/// Get a random number on [0, max] interval.
	uint32_t getRange( uint32_t max )
	{
		if (max == 0) return 0;
		if (max == UINT32_MAX) return get();
		const uint32_t np2 = nextPowerOfTwo( max + 1 ); // @@ This fails if max == UINT32_MAX
		const uint32_t mask = np2 - 1;
		uint32_t n;
		do {
			n = get() & mask;
		} while ( n > max );
		return n;
	}

private:
	void initialize( uint32_t seed )
	{
		// Initialize generator state with seed
		// See Knuth TAOCP Vol 2, 3rd Ed, p.106 for multiplier.
		// In previous versions, most significant bits (MSBs) of the seed affect
		// only MSBs of the state array.  Modified 9 Jan 2002 by Makoto Matsumoto.
		uint32_t *s = state;
		uint32_t *r = state;
		int i = 1;
		*s++ = seed & 0xffffffffUL;
		for ( ; i < N; ++i ) {
			*s++ = ( 1812433253UL * ( *r ^ (*r >> 30) ) + i ) & 0xffffffffUL;
			r++;
		}
	}

	void reload()
	{
		// Generate N new values in state
		// Made clearer and faster by Matthew Bellew (matthew.bellew@home.com)
		uint32_t *p = state;
		int i;
		for ( i = N - M; i--; ++p )
			*p = twist( p[M], p[0], p[1] );
		for ( i = M; --i; ++p )
			*p = twist( p[M - N], p[0], p[1] );
		*p = twist( p[M - N], p[0], state[0] );
		left = N, next = state;
	}

	uint32_t hiBit( uint32_t u ) const
	{
		return u & 0x80000000U;
	}
	uint32_t loBit( uint32_t u ) const
	{
		return u & 0x00000001U;
	}
	uint32_t loBits( uint32_t u ) const
	{
		return u & 0x7fffffffU;
	}
	uint32_t mixBits( uint32_t u, uint32_t v ) const
	{
		return hiBit(u) | loBits(v);
	}
	uint32_t twist( uint32_t m, uint32_t s0, uint32_t s1 ) const
	{
		return m ^ (mixBits(s0, s1) >> 1) ^ ((~loBit(s1) + 1) & 0x9908b0dfU);
	}

	uint32_t state[N];	// internal state
	uint32_t *next;	// next value to get from state
	int left;		// number of values left before reload needed
};

// Based on Pierre Terdiman's and Michael Herf's source code.
// http://www.codercorner.com/RadixSortRevisited.htm
// http://www.stereopsis.com/radix.html
class RadixSort
{
public:
	RadixSort() : m_size(0), m_ranks(NULL), m_ranks2(NULL), m_validRanks(false) {}

	~RadixSort()
	{
		// Release everything
		free(m_ranks2);
		free(m_ranks);
	}

	RadixSort &sort(const float *input, uint32_t count)
	{
		if (input == NULL || count == 0) return *this;
		// Resize lists if needed
		if (count != m_size) {
			if (count > m_size) {
				m_ranks2 = (uint32_t *)realloc(m_ranks2, sizeof(uint32_t ) * count);
				m_ranks = (uint32_t *)realloc(m_ranks, sizeof(uint32_t ) * count);
			}
			m_size = count;
			m_validRanks = false;
		}
		if (count < 32) {
			insertionSort(input, count);
		} else {
			// @@ Avoid touching the input multiple times.
			for (uint32_t i = 0; i < count; i++) {
				FloatFlip((uint32_t &)input[i]);
			}
			radixSort<uint32_t>((const uint32_t *)input, count);
			for (uint32_t i = 0; i < count; i++) {
				IFloatFlip((uint32_t &)input[i]);
			}
		}
		return *this;
	}

	RadixSort &sort(const Array<float> &input)
	{
		return sort(input.data(), input.size());
	}

	// Access to results. m_ranks is a list of indices in sorted order, i.e. in the order you may further process your data
	const uint32_t *ranks() const
	{
		XA_DEBUG_ASSERT(m_validRanks);
		return m_ranks;
	}

	uint32_t *ranks()
	{
		XA_DEBUG_ASSERT(m_validRanks);
		return m_ranks;
	}

private:
	uint32_t m_size;
	uint32_t *m_ranks;
	uint32_t *m_ranks2;
	bool m_validRanks;

	void FloatFlip(uint32_t &f)
	{
		int32_t mask = (int32_t(f) >> 31) | 0x80000000; // Warren Hunt, Manchor Ko.
		f ^= mask;
	}

	void IFloatFlip(uint32_t &f)
	{
		uint32_t mask = ((f >> 31) - 1) | 0x80000000; // Michael Herf.
		f ^= mask;
	}

	template<typename T>
	void createHistograms(const T *buffer, uint32_t count, uint32_t *histogram)
	{
		const uint32_t bucketCount = sizeof(T); // (8 * sizeof(T)) / log2(radix)
		// Init bucket pointers.
		uint32_t *h[bucketCount];
		for (uint32_t i = 0; i < bucketCount; i++) {
			h[i] = histogram + 256 * i;
		}
		// Clear histograms.
		memset(histogram, 0, 256 * bucketCount * sizeof(uint32_t ));
		// @@ Add support for signed integers.
		// Build histograms.
		const uint8_t *p = (const uint8_t *)buffer;  // @@ Does this break aliasing rules?
		const uint8_t *pe = p + count * sizeof(T);
		while (p != pe) {
			h[0][*p++]++, h[1][*p++]++, h[2][*p++]++, h[3][*p++]++;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)
#endif
			if (bucketCount == 8) h[4][*p++]++, h[5][*p++]++, h[6][*p++]++, h[7][*p++]++;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
		}
	}

	template <typename T> void insertionSort(const T *input, uint32_t count)
	{
		if (!m_validRanks) {
			m_ranks[0] = 0;
			for (uint32_t i = 1; i != count; ++i) {
				int rank = m_ranks[i] = i;
				uint32_t j = i;
				while (j != 0 && input[rank] < input[m_ranks[j - 1]]) {
					m_ranks[j] = m_ranks[j - 1];
					--j;
				}
				if (i != j) {
					m_ranks[j] = rank;
				}
			}
			m_validRanks = true;
		} else {
			for (uint32_t i = 1; i != count; ++i) {
				int rank = m_ranks[i];
				uint32_t j = i;
				while (j != 0 && input[rank] < input[m_ranks[j - 1]]) {
					m_ranks[j] = m_ranks[j - 1];
					--j;
				}
				if (i != j) {
					m_ranks[j] = rank;
				}
			}
		}
	}

	template <typename T> void radixSort(const T *input, uint32_t count)
	{
		const uint32_t P = sizeof(T); // pass count
		// Allocate histograms & offsets on the stack
		uint32_t histogram[256 * P];
		uint32_t *link[256];
		createHistograms(input, count, histogram);
		// Radix sort, j is the pass number (0=LSB, P=MSB)
		for (uint32_t j = 0; j < P; j++) {
			// Pointer to this bucket.
			const uint32_t *h = &histogram[j * 256];
			const uint8_t *inputBytes = (const uint8_t *)input; // @@ Is this aliasing legal?
			inputBytes += j;
			if (h[inputBytes[0]] == count) {
				// Skip this pass, all values are the same.
				continue;
			}
			// Create offsets
			link[0] = m_ranks2;
			for (uint32_t i = 1; i < 256; i++) link[i] = link[i - 1] + h[i - 1];
			// Perform Radix Sort
			if (!m_validRanks) {
				for (uint32_t i = 0; i < count; i++) {
					*link[inputBytes[i * P]]++ = i;
				}
				m_validRanks = true;
			} else {
				for (uint32_t i = 0; i < count; i++) {
					const uint32_t idx = m_ranks[i];
					*link[inputBytes[idx * P]]++ = idx;
				}
			}
			// Swap pointers for next pass. Valid indices - the most recent ones - are in m_ranks after the swap.
			std::swap(m_ranks, m_ranks2);
		}
		// All values were equal, generate linear ranks.
		if (!m_validRanks) {
			for (uint32_t i = 0; i < count; i++) {
				m_ranks[i] = i;
			}
			m_validRanks = true;
		}
	}
};

namespace raster {
class ClippedTriangle
{
public:
	ClippedTriangle(Vector2::Arg a, Vector2::Arg b, Vector2::Arg c)
	{
		m_numVertices = 3;
		m_activeVertexBuffer = 0;
		m_verticesA[0] = a;
		m_verticesA[1] = b;
		m_verticesA[2] = c;
		m_vertexBuffers[0] = m_verticesA;
		m_vertexBuffers[1] = m_verticesB;
	}

	uint32_t vertexCount()
	{
		return m_numVertices;
	}

	const Vector2 *vertices()
	{
		return m_vertexBuffers[m_activeVertexBuffer];
	}

	void clipHorizontalPlane(float offset, float clipdirection)
	{
		Vector2 *v  = m_vertexBuffers[m_activeVertexBuffer];
		m_activeVertexBuffer ^= 1;
		Vector2 *v2 = m_vertexBuffers[m_activeVertexBuffer];
		v[m_numVertices] = v[0];
		float dy2,   dy1 = offset - v[0].y;
		int   dy2in, dy1in = clipdirection * dy1 >= 0;
		uint32_t  p = 0;
		for (uint32_t k = 0; k < m_numVertices; k++) {
			dy2   = offset - v[k + 1].y;
			dy2in = clipdirection * dy2 >= 0;
			if (dy1in) v2[p++] = v[k];
			if ( dy1in + dy2in == 1 ) { // not both in/out
				float dx = v[k + 1].x - v[k].x;
				float dy = v[k + 1].y - v[k].y;
				v2[p++] = Vector2(v[k].x + dy1 * (dx / dy), offset);
			}
			dy1 = dy2;
			dy1in = dy2in;
		}
		m_numVertices = p;
		//for (uint32_t k=0; k<m_numVertices; k++) printf("(%f, %f)\n", v2[k].x, v2[k].y); printf("\n");
	}

	void clipVerticalPlane(float offset, float clipdirection )
	{
		Vector2 *v  = m_vertexBuffers[m_activeVertexBuffer];
		m_activeVertexBuffer ^= 1;
		Vector2 *v2 = m_vertexBuffers[m_activeVertexBuffer];
		v[m_numVertices] = v[0];
		float dx2,   dx1   = offset - v[0].x;
		int   dx2in, dx1in = clipdirection * dx1 >= 0;
		uint32_t  p = 0;
		for (uint32_t k = 0; k < m_numVertices; k++) {
			dx2 = offset - v[k + 1].x;
			dx2in = clipdirection * dx2 >= 0;
			if (dx1in) v2[p++] = v[k];
			if ( dx1in + dx2in == 1 ) { // not both in/out
				float dx = v[k + 1].x - v[k].x;
				float dy = v[k + 1].y - v[k].y;
				v2[p++] = Vector2(offset, v[k].y + dx1 * (dy / dx));
			}
			dx1 = dx2;
			dx1in = dx2in;
		}
		m_numVertices = p;
	}

	void computeAreaCentroid()
	{
		Vector2 *v  = m_vertexBuffers[m_activeVertexBuffer];
		v[m_numVertices] = v[0];
		m_area = 0;
		float centroidx = 0, centroidy = 0;
		for (uint32_t k = 0; k < m_numVertices; k++) {
			// http://local.wasp.uwa.edu.au/~pbourke/geometry/polyarea/
			float f = v[k].x * v[k + 1].y - v[k + 1].x * v[k].y;
			m_area += f;
			centroidx += f * (v[k].x + v[k + 1].x);
			centroidy += f * (v[k].y + v[k + 1].y);
		}
		m_area = 0.5f * fabsf(m_area);
		if (m_area == 0) {
			m_centroid = Vector2(0.0f);
		} else {
			m_centroid = Vector2(centroidx / (6 * m_area), centroidy / (6 * m_area));
		}
	}

	void clipAABox(float x0, float y0, float x1, float y1)
	{
		clipVerticalPlane  ( x0, -1);
		clipHorizontalPlane( y0, -1);
		clipVerticalPlane  ( x1,  1);
		clipHorizontalPlane( y1,  1);
		computeAreaCentroid();
	}

	Vector2 centroid()
	{
		return m_centroid;
	}

	float area()
	{
		return m_area;
	}

private:
	Vector2 m_verticesA[7 + 1];
	Vector2 m_verticesB[7 + 1];
	Vector2 *m_vertexBuffers[2];
	uint32_t    m_numVertices;
	uint32_t    m_activeVertexBuffer;
	float   m_area;
	Vector2 m_centroid;
};

/// A callback to sample the environment. Return false to terminate rasterization.
typedef bool (* SamplingCallback)(void *param, int x, int y, Vector3::Arg bar, Vector3::Arg dx, Vector3::Arg dy, float coverage);

/// A triangle for rasterization.
struct Triangle
{
	Triangle(Vector2::Arg v0, Vector2::Arg v1, Vector2::Arg v2, Vector3::Arg t0, Vector3::Arg t1, Vector3::Arg t2)
	{
		// Init vertices.
		this->v1 = v0;
		this->v2 = v2;
		this->v3 = v1;
		// Set barycentric coordinates.
		this->t1 = t0;
		this->t2 = t2;
		this->t3 = t1;
		// make sure every triangle is front facing.
		flipBackface();
		// Compute deltas.
		valid = computeDeltas();
		computeUnitInwardNormals();
	}

	/// Compute texture space deltas.
	/// This method takes two edge vectors that form a basis, determines the
	/// coordinates of the canonic vectors in that basis, and computes the
	/// texture gradient that corresponds to those vectors.
	bool computeDeltas()
	{
		Vector2 e0 = v3 - v1;
		Vector2 e1 = v2 - v1;
		Vector3 de0 = t3 - t1;
		Vector3 de1 = t2 - t1;
		float denom = 1.0f / (e0.y * e1.x - e1.y * e0.x);
		if (!std::isfinite(denom)) {
			return false;
		}
		float lambda1 = - e1.y * denom;
		float lambda2 = e0.y * denom;
		float lambda3 = e1.x * denom;
		float lambda4 = - e0.x * denom;
		dx = de0 * lambda1 + de1 * lambda2;
		dy = de0 * lambda3 + de1 * lambda4;
		return true;
	}

	bool draw(const Vector2 &extents, bool enableScissors, SamplingCallback cb, void *param)
	{
		// 28.4 fixed-point coordinates
		const int Y1 = ftoi_round(16.0f * v1.y);
		const int Y2 = ftoi_round(16.0f * v2.y);
		const int Y3 = ftoi_round(16.0f * v3.y);
		const int X1 = ftoi_round(16.0f * v1.x);
		const int X2 = ftoi_round(16.0f * v2.x);
		const int X3 = ftoi_round(16.0f * v3.x);
		// Deltas
		const int DX12 = X1 - X2;
		const int DX23 = X2 - X3;
		const int DX31 = X3 - X1;
		const int DY12 = Y1 - Y2;
		const int DY23 = Y2 - Y3;
		const int DY31 = Y3 - Y1;
		// Fixed-point deltas
		const int FDX12 = DX12 << 4;
		const int FDX23 = DX23 << 4;
		const int FDX31 = DX31 << 4;
		const int FDY12 = DY12 << 4;
		const int FDY23 = DY23 << 4;
		const int FDY31 = DY31 << 4;
		int minx, miny, maxx, maxy;
		if (enableScissors) {
			int frustumX0 =  0 << 4;
			int frustumY0 =  0 << 4;
			int frustumX1 =  (int)extents.x << 4;
			int frustumY1 =  (int)extents.y << 4;
			// Bounding rectangle
			minx = (std::max(min3(X1, X2, X3), frustumX0) + 0xF) >> 4;
			miny = (std::max(min3(Y1, Y2, Y3), frustumY0) + 0xF) >> 4;
			maxx = (std::min(max3(X1, X2, X3), frustumX1) + 0xF) >> 4;
			maxy = (std::min(max3(Y1, Y2, Y3), frustumY1) + 0xF) >> 4;
		} else {
			// Bounding rectangle
			minx = (min3(X1, X2, X3) + 0xF) >> 4;
			miny = (min3(Y1, Y2, Y3) + 0xF) >> 4;
			maxx = (max3(X1, X2, X3) + 0xF) >> 4;
			maxy = (max3(Y1, Y2, Y3) + 0xF) >> 4;
		}
		// Block size, standard 8x8 (must be power of two)
		const int q = 8;
		// @@ This won't work when minx,miny are negative. This code path is not used. Leaving as is for now.
		XA_ASSERT(minx >= 0);
		XA_ASSERT(miny >= 0);
		// Start in corner of 8x8 block
		minx &= ~(q - 1);
		miny &= ~(q - 1);
		// Half-edge constants
		int C1 = DY12 * X1 - DX12 * Y1;
		int C2 = DY23 * X2 - DX23 * Y2;
		int C3 = DY31 * X3 - DX31 * Y3;
		// Correct for fill convention
		if (DY12 < 0 || (DY12 == 0 && DX12 > 0)) C1++;
		if (DY23 < 0 || (DY23 == 0 && DX23 > 0)) C2++;
		if (DY31 < 0 || (DY31 == 0 && DX31 > 0)) C3++;
		// Loop through blocks
		for (int y = miny; y < maxy; y += q) {
			for (int x = minx; x < maxx; x += q) {
				// Corners of block
				int x0 = x << 4;
				int x1 = (x + q - 1) << 4;
				int y0 = y << 4;
				int y1 = (y + q - 1) << 4;
				// Evaluate half-space functions
				bool a00 = C1 + DX12 * y0 - DY12 * x0 > 0;
				bool a10 = C1 + DX12 * y0 - DY12 * x1 > 0;
				bool a01 = C1 + DX12 * y1 - DY12 * x0 > 0;
				bool a11 = C1 + DX12 * y1 - DY12 * x1 > 0;
				int a = (a00 << 0) | (a10 << 1) | (a01 << 2) | (a11 << 3);
				bool b00 = C2 + DX23 * y0 - DY23 * x0 > 0;
				bool b10 = C2 + DX23 * y0 - DY23 * x1 > 0;
				bool b01 = C2 + DX23 * y1 - DY23 * x0 > 0;
				bool b11 = C2 + DX23 * y1 - DY23 * x1 > 0;
				int b = (b00 << 0) | (b10 << 1) | (b01 << 2) | (b11 << 3);
				bool c00 = C3 + DX31 * y0 - DY31 * x0 > 0;
				bool c10 = C3 + DX31 * y0 - DY31 * x1 > 0;
				bool c01 = C3 + DX31 * y1 - DY31 * x0 > 0;
				bool c11 = C3 + DX31 * y1 - DY31 * x1 > 0;
				int c = (c00 << 0) | (c10 << 1) | (c01 << 2) | (c11 << 3);
				// Skip block when outside an edge
				if (a == 0x0 || b == 0x0 || c == 0x0) continue;
				// Accept whole block when totally covered
				if (a == 0xF && b == 0xF && c == 0xF) {
					Vector3 texRow = t1 + dy * (y0 - v1.y) + dx * (x0 - v1.x);
					for (int iy = y; iy < y + q; iy++) {
						Vector3 tex = texRow;
						for (int ix = x; ix < x + q; ix++) {
							//Vector3 tex = t1 + dx * (ix - v1.x) + dy * (iy - v1.y);
							if (!cb(param, ix, iy, tex, dx, dy, 1.0)) {
								// early out.
								return false;
							}
							tex += dx;
						}
						texRow += dy;
					}
				} else { // Partially covered block
					int CY1 = C1 + DX12 * y0 - DY12 * x0;
					int CY2 = C2 + DX23 * y0 - DY23 * x0;
					int CY3 = C3 + DX31 * y0 - DY31 * x0;
					Vector3 texRow = t1 + dy * (y0 - v1.y) + dx * (x0 - v1.x);
					for (int iy = y; iy < y + q; iy++) {
						int CX1 = CY1;
						int CX2 = CY2;
						int CX3 = CY3;
						Vector3 tex = texRow;
						for (int ix = x; ix < x + q; ix++) {
							if (CX1 > 0 && CX2 > 0 && CX3 > 0) {
								if (!cb(param, ix, iy, tex, dx, dy, 1.0)) {
									// early out.
									return false;
								}
							}
							CX1 -= FDY12;
							CX2 -= FDY23;
							CX3 -= FDY31;
							tex += dx;
						}
						CY1 += FDX12;
						CY2 += FDX23;
						CY3 += FDX31;
						texRow += dy;
					}
				}
			}
		}
		return true;
	}

	// extents has to be multiple of BK_SIZE!!
	bool drawAA(const Vector2 &extents, bool enableScissors, SamplingCallback cb, void *param)
	{
		const float PX_INSIDE = 1.0f/sqrt(2.0f);
		const float PX_OUTSIDE = -1.0f/sqrt(2.0f);
		const float BK_SIZE = 8;
		const float BK_INSIDE = sqrt(BK_SIZE*BK_SIZE/2.0f);
		const float BK_OUTSIDE = -sqrt(BK_SIZE*BK_SIZE/2.0f);

		float minx, miny, maxx, maxy;
		if (enableScissors) {
			// Bounding rectangle
			minx = floorf(std::max(min3(v1.x, v2.x, v3.x), 0.0f));
			miny = floorf(std::max(min3(v1.y, v2.y, v3.y), 0.0f));
			maxx = ceilf( std::min(max3(v1.x, v2.x, v3.x), extents.x - 1.0f));
			maxy = ceilf( std::min(max3(v1.y, v2.y, v3.y), extents.y - 1.0f));
		} else {
			// Bounding rectangle
			minx = floorf(min3(v1.x, v2.x, v3.x));
			miny = floorf(min3(v1.y, v2.y, v3.y));
			maxx = ceilf( max3(v1.x, v2.x, v3.x));
			maxy = ceilf( max3(v1.y, v2.y, v3.y));
		}
		// There's no reason to align the blocks to the viewport, instead we align them to the origin of the triangle bounds.
		minx = floorf(minx);
		miny = floorf(miny);
		//minx = (float)(((int)minx) & (~((int)BK_SIZE - 1))); // align to blocksize (we don't need to worry about blocks partially out of viewport)
		//miny = (float)(((int)miny) & (~((int)BK_SIZE - 1)));
		minx += 0.5;
		miny += 0.5; // sampling at texel centers!
		maxx += 0.5;
		maxy += 0.5;
		// Half-edge constants
		float C1 = n1.x * (-v1.x) + n1.y * (-v1.y);
		float C2 = n2.x * (-v2.x) + n2.y * (-v2.y);
		float C3 = n3.x * (-v3.x) + n3.y * (-v3.y);
		// Loop through blocks
		for (float y0 = miny; y0 <= maxy; y0 += BK_SIZE) {
			for (float x0 = minx; x0 <= maxx; x0 += BK_SIZE) {
				// Corners of block
				float xc = (x0 + (BK_SIZE - 1) / 2.0f);
				float yc = (y0 + (BK_SIZE - 1) / 2.0f);
				// Evaluate half-space functions
				float aC = C1 + n1.x * xc + n1.y * yc;
				float bC = C2 + n2.x * xc + n2.y * yc;
				float cC = C3 + n3.x * xc + n3.y * yc;
				// Skip block when outside an edge
				if ( (aC <= BK_OUTSIDE) || (bC <= BK_OUTSIDE) || (cC <= BK_OUTSIDE) ) continue;
				// Accept whole block when totally covered
				if ( (aC >= BK_INSIDE) && (bC >= BK_INSIDE) && (cC >= BK_INSIDE) ) {
					Vector3 texRow = t1 + dy * (y0 - v1.y) + dx * (x0 - v1.x);
					for (float y = y0; y < y0 + BK_SIZE; y++) {
						Vector3 tex = texRow;
						for (float x = x0; x < x0 + BK_SIZE; x++) {
							if (!cb(param, (int)x, (int)y, tex, dx, dy, 1.0f)) {
								return false;
							}
							tex += dx;
						}
						texRow += dy;
					}
				} else { // Partially covered block
					float CY1 = C1 + n1.x * x0 + n1.y * y0;
					float CY2 = C2 + n2.x * x0 + n2.y * y0;
					float CY3 = C3 + n3.x * x0 + n3.y * y0;
					Vector3 texRow = t1 + dy * (y0 - v1.y) + dx * (x0 - v1.x);
					for (float y = y0; y < y0 + BK_SIZE; y++) { // @@ This is not clipping to scissor rectangle correctly.
						float CX1 = CY1;
						float CX2 = CY2;
						float CX3 = CY3;
						Vector3 tex = texRow;
						for (float x = x0; x < x0 + BK_SIZE; x++) { // @@ This is not clipping to scissor rectangle correctly.
							if (CX1 >= PX_INSIDE && CX2 >= PX_INSIDE && CX3 >= PX_INSIDE) {
								// pixel completely covered
								Vector3 tex2 = t1 + dx * (x - v1.x) + dy * (y - v1.y);
								if (!cb(param, (int)x, (int)y, tex2, dx, dy, 1.0f)) {
									return false;
								}
							} else if ((CX1 >= PX_OUTSIDE) && (CX2 >= PX_OUTSIDE) && (CX3 >= PX_OUTSIDE)) {
								// triangle partially covers pixel. do clipping.
								ClippedTriangle ct(v1 - Vector2(x, y), v2 - Vector2(x, y), v3 - Vector2(x, y));
								ct.clipAABox(-0.5, -0.5, 0.5, 0.5);
								Vector2 centroid = ct.centroid();
								float area = ct.area();
								if (area > 0.0f) {
									Vector3 texCent = tex - dx * centroid.x - dy * centroid.y;
									//XA_ASSERT(texCent.x >= -0.1f && texCent.x <= 1.1f); // @@ Centroid is not very exact...
									//XA_ASSERT(texCent.y >= -0.1f && texCent.y <= 1.1f);
									//XA_ASSERT(texCent.z >= -0.1f && texCent.z <= 1.1f);
									//Vector3 texCent2 = t1 + dx * (x - v1.x) + dy * (y - v1.y);
									if (!cb(param, (int)x, (int)y, texCent, dx, dy, area)) {
										return false;
									}
								}
							}
							CX1 += n1.x;
							CX2 += n2.x;
							CX3 += n3.x;
							tex += dx;
						}
						CY1 += n1.y;
						CY2 += n2.y;
						CY3 += n3.y;
						texRow += dy;
					}
				}
			}
		}
		return true;
	}

	void flipBackface()
	{
		// check if triangle is backfacing, if so, swap two vertices
		if ( ((v3.x - v1.x) * (v2.y - v1.y) - (v3.y - v1.y) * (v2.x - v1.x)) < 0 ) {
			Vector2 hv = v1;
			v1 = v2;
			v2 = hv; // swap pos
			Vector3 ht = t1;
			t1 = t2;
			t2 = ht; // swap tex
		}
	}

	// compute unit inward normals for each edge.
	void computeUnitInwardNormals()
	{
		n1 = v1 - v2;
		n1 = Vector2(-n1.y, n1.x);
		n1 = n1 * (1.0f / sqrtf(n1.x * n1.x + n1.y * n1.y));
		n2 = v2 - v3;
		n2 = Vector2(-n2.y, n2.x);
		n2 = n2 * (1.0f / sqrtf(n2.x * n2.x + n2.y * n2.y));
		n3 = v3 - v1;
		n3 = Vector2(-n3.y, n3.x);
		n3 = n3 * (1.0f / sqrtf(n3.x * n3.x + n3.y * n3.y));
	}

	// Vertices.
	Vector2 v1, v2, v3;
	Vector2 n1, n2, n3; // unit inward normals
	Vector3 t1, t2, t3;

	// Deltas.
	Vector3 dx, dy;

	float sign;
	bool valid;
};

enum Mode
{
	Mode_Nearest,
	Mode_Antialiased
};

// Process the given triangle. Returns false if rasterization was interrupted by the callback.
static bool drawTriangle(Mode mode, Vector2::Arg extents, bool enableScissors, const Vector2 v[3], SamplingCallback cb, void *param)
{
	Triangle tri(v[0], v[1], v[2], Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1));
	// @@ It would be nice to have a conservative drawing mode that enlarges the triangle extents by one texel and is able to handle degenerate triangles.
	// @@ Maybe the simplest thing to do would be raster triangle edges.
	if (tri.valid) {
		if (mode == Mode_Antialiased) {
			return tri.drawAA(extents, enableScissors, cb, param);
		}
		if (mode == Mode_Nearest) {
			return tri.draw(extents, enableScissors, cb, param);
		}
	}
	return true;
}

} // namespace raster

// Full and sparse vector and matrix classes. BLAS subset.
// Pseudo-BLAS interface.
namespace sparse {
enum Transpose
{
	NoTransposed = 0,
	Transposed = 1
};

/**
* Sparse matrix class. The matrix is assumed to be sparse and to have
* very few non-zero elements, for this reason it's stored in indexed
* format. To multiply column vectors efficiently, the matrix stores
* the elements in indexed-column order, there is a list of indexed
* elements for each row of the matrix. As with the FullVector the
* dimension of the matrix is constant.
**/
class Matrix
{
public:
	// An element of the sparse array.
	struct Coefficient
	{
		uint32_t x;  // column
		float v; // value
	};

	Matrix(uint32_t d) : m_width(d) { m_array.resize(d); }
	Matrix(uint32_t w, uint32_t h) : m_width(w) { m_array.resize(h); }
	Matrix(const Matrix &m) : m_width(m.m_width) { m_array = m.m_array; }

	const Matrix &operator=(const Matrix &m)
	{
		XA_ASSERT(width() == m.width());
		XA_ASSERT(height() == m.height());
		m_array = m.m_array;
		return *this;
	}

	uint32_t width() const { return m_width; }
	uint32_t height() const { return m_array.size(); }
	bool isSquare() const { return width() == height(); }

	// x is column, y is row
	float getCoefficient(uint32_t x, uint32_t y) const
	{
		XA_DEBUG_ASSERT( x < width() );
		XA_DEBUG_ASSERT( y < height() );
		const uint32_t count = m_array[y].size();
		for (uint32_t i = 0; i < count; i++) {
			if (m_array[y][i].x == x) return m_array[y][i].v;
		}
		return 0.0f;
	}

	void setCoefficient(uint32_t x, uint32_t y, float f)
	{
		XA_DEBUG_ASSERT( x < width() );
		XA_DEBUG_ASSERT( y < height() );
		const uint32_t count = m_array[y].size();
		for (uint32_t i = 0; i < count; i++) {
			if (m_array[y][i].x == x) {
				m_array[y][i].v = f;
				return;
			}
		}
		if (f != 0.0f) {
			Coefficient c = { x, f };
			m_array[y].push_back( c );
		}
	}

	float dotRow(uint32_t y, const FullVector &v) const
	{
		XA_DEBUG_ASSERT( y < height() );
		const uint32_t count = m_array[y].size();
		float sum = 0;
		for (uint32_t i = 0; i < count; i++) {
			sum += m_array[y][i].v * v[m_array[y][i].x];
		}
		return sum;
	}

	void madRow(uint32_t y, float alpha, FullVector &v) const
	{
		XA_DEBUG_ASSERT(y < height());
		const uint32_t count = m_array[y].size();
		for (uint32_t i = 0; i < count; i++) {
			v[m_array[y][i].x] += alpha * m_array[y][i].v;
		}
	}

	void clearRow(uint32_t y)
	{
		XA_DEBUG_ASSERT( y < height() );
		m_array[y].clear();
	}

	void scaleRow(uint32_t y, float f)
	{
		XA_DEBUG_ASSERT( y < height() );
		const uint32_t count = m_array[y].size();
		for (uint32_t i = 0; i < count; i++) {
			m_array[y][i].v *= f;
		}
	}

	const Array<Coefficient> &getRow(uint32_t y) const { return m_array[y]; }

private:
	/// Number of columns.
	const uint32_t m_width;

	/// Array of matrix elements.
	Array< Array<Coefficient> > m_array;
};

// y = a * x + y
static void saxpy(float a, const FullVector &x, FullVector &y)
{
	XA_DEBUG_ASSERT(x.dimension() == y.dimension());
	const uint32_t dim = x.dimension();
	for (uint32_t i = 0; i < dim; i++) {
		y[i] += a * x[i];
	}
}

static void copy(const FullVector &x, FullVector &y)
{
	XA_DEBUG_ASSERT(x.dimension() == y.dimension());
	const uint32_t dim = x.dimension();
	for (uint32_t i = 0; i < dim; i++) {
		y[i] = x[i];
	}
}

static void scal(float a, FullVector &x)
{
	const uint32_t dim = x.dimension();
	for (uint32_t i = 0; i < dim; i++) {
		x[i] *= a;
	}
}

static float dot(const FullVector &x, const FullVector &y)
{
	XA_DEBUG_ASSERT(x.dimension() == y.dimension());
	const uint32_t dim = x.dimension();
	float sum = 0;
	for (uint32_t i = 0; i < dim; i++) {
		sum += x[i] * y[i];
	}
	return sum;
}

static void mult(Transpose TM, const Matrix &M, const FullVector &x, FullVector &y)
{
	uint32_t w = M.width();
	uint32_t h = M.height();
	if (TM == Transposed) {
		XA_DEBUG_ASSERT( h == x.dimension() );
		XA_DEBUG_ASSERT( w == y.dimension() );
		y.fill(0.0f);
		for (uint32_t i = 0; i < h; i++) {
			M.madRow(i, x[i], y);
		}
	} else {
		XA_DEBUG_ASSERT( w == x.dimension() );
		XA_DEBUG_ASSERT( h == y.dimension() );
		for (uint32_t i = 0; i < h; i++) {
			y[i] = M.dotRow(i, x);
		}
	}
#ifdef NDEBUG
	w = w; // silence unused parameter warning
	h = h;
#endif
}

// y = M * x
static void mult(const Matrix &M, const FullVector &x, FullVector &y)
{
	mult(NoTransposed, M, x, y);
}

static void sgemv(float alpha, Transpose TA, const Matrix &A, const FullVector &x, float beta, FullVector &y)
{
	uint32_t w = A.width();
	uint32_t h = A.height();
	if (TA == Transposed) {
		XA_DEBUG_ASSERT( h == x.dimension() );
		XA_DEBUG_ASSERT( w == y.dimension() );
		for (uint32_t i = 0; i < h; i++) {
			A.madRow(i, alpha * x[i], y);
		}
	} else {
		XA_DEBUG_ASSERT( w == x.dimension() );
		XA_DEBUG_ASSERT( h == y.dimension() );
		for (uint32_t i = 0; i < h; i++) {
			y[i] = alpha * A.dotRow(i, x) + beta * y[i];
		}
	}
#ifdef NDEBUG
	w = w; // silence unused parameter warning
	h = h;
#endif
}

// y = alpha*A*x + beta*y
static void sgemv(float alpha, const Matrix &A, const FullVector &x, float beta, FullVector &y)
{
	sgemv(alpha, NoTransposed, A, x, beta, y);
}

// dot y-row of A by x-column of B
static float dotRowColumn(int y, const Matrix &A, int x, const Matrix &B)
{
	const Array<Matrix::Coefficient> &row = A.getRow(y);
	const uint32_t count = row.size();
	float sum = 0.0f;
	for (uint32_t i = 0; i < count; i++) {
		const Matrix::Coefficient &c = row[i];
		sum += c.v * B.getCoefficient(x, c.x);
	}
	return sum;
}

// dot y-row of A by x-row of B
static float dotRowRow(int y, const Matrix &A, int x, const Matrix &B)
{
	const Array<Matrix::Coefficient> &row = A.getRow(y);
	const uint32_t count = row.size();
	float sum = 0.0f;
	for (uint32_t i = 0; i < count; i++) {
		const Matrix::Coefficient &c = row[i];
		sum += c.v * B.getCoefficient(c.x, x);
	}
	return sum;
}

// dot y-column of A by x-column of B
static float dotColumnColumn(int y, const Matrix &A, int x, const Matrix &B)
{
	XA_DEBUG_ASSERT(A.height() == B.height());
	const uint32_t h = A.height();
	float sum = 0.0f;
	for (uint32_t i = 0; i < h; i++) {
		sum += A.getCoefficient(y, i) * B.getCoefficient(x, i);
	}
	return sum;
}

static void transpose(const Matrix &A, Matrix &B)
{
	XA_DEBUG_ASSERT(A.width() == B.height());
	XA_DEBUG_ASSERT(B.width() == A.height());
	const uint32_t w = A.width();
	for (uint32_t x = 0; x < w; x++) {
		B.clearRow(x);
	}
	const uint32_t h = A.height();
	for (uint32_t y = 0; y < h; y++) {
		const Array<Matrix::Coefficient> &row = A.getRow(y);
		const uint32_t count = row.size();
		for (uint32_t i = 0; i < count; i++) {
			const Matrix::Coefficient &c = row[i];
			XA_DEBUG_ASSERT(c.x < w);
			B.setCoefficient(y, c.x, c.v);
		}
	}
}

static void sgemm(float alpha, Transpose TA, const Matrix &A, Transpose TB, const Matrix &B, float beta, Matrix &C)
{
	const uint32_t w = C.width();
	const uint32_t h = C.height();
	uint32_t aw = (TA == NoTransposed) ? A.width() : A.height();
	uint32_t ah = (TA == NoTransposed) ? A.height() : A.width();
	uint32_t bw = (TB == NoTransposed) ? B.width() : B.height();
	uint32_t bh = (TB == NoTransposed) ? B.height() : B.width();
	XA_DEBUG_ASSERT(aw == bh);
	XA_DEBUG_ASSERT(bw == ah);
	XA_DEBUG_ASSERT(w == bw);
	XA_DEBUG_ASSERT(h == ah);
#ifdef NDEBUG
	aw = aw; // silence unused parameter warning
	ah = ah;
	bw = bw;
	bh = bh;
#endif
	for (uint32_t y = 0; y < h; y++) {
		for (uint32_t x = 0; x < w; x++) {
			float c = beta * C.getCoefficient(x, y);
			if (TA == NoTransposed && TB == NoTransposed) {
				// dot y-row of A by x-column of B.
				c += alpha * dotRowColumn(y, A, x, B);
			} else if (TA == Transposed && TB == Transposed) {
				// dot y-column of A by x-row of B.
				c += alpha * dotRowColumn(x, B, y, A);
			} else if (TA == Transposed && TB == NoTransposed) {
				// dot y-column of A by x-column of B.
				c += alpha * dotColumnColumn(y, A, x, B);
			} else if (TA == NoTransposed && TB == Transposed) {
				// dot y-row of A by x-row of B.
				c += alpha * dotRowRow(y, A, x, B);
			}
			C.setCoefficient(x, y, c);
		}
	}
}

static void mult(Transpose TA, const Matrix &A, Transpose TB, const Matrix &B, Matrix &C)
{
	sgemm(1.0f, TA, A, TB, B, 0.0f, C);
}

// C = A * B
static void mult(const Matrix &A, const Matrix &B, Matrix &C)
{
	mult(NoTransposed, A, NoTransposed, B, C);
}

} // namespace sparse

class JacobiPreconditioner
{
public:
	JacobiPreconditioner(const sparse::Matrix &M, bool symmetric) : m_inverseDiagonal(M.width())
	{
		XA_ASSERT(M.isSquare());
		for (uint32_t x = 0; x < M.width(); x++) {
			float elem = M.getCoefficient(x, x);
			//XA_DEBUG_ASSERT( elem != 0.0f ); // This can be zero in the presence of zero area triangles.
			if (symmetric) {
				m_inverseDiagonal[x] = (elem != 0) ? 1.0f / sqrtf(fabsf(elem)) : 1.0f;
			} else {
				m_inverseDiagonal[x] = (elem != 0) ? 1.0f / elem : 1.0f;
			}
		}
	}

	void apply(const FullVector &x, FullVector &y) const
	{
		XA_DEBUG_ASSERT(x.dimension() == m_inverseDiagonal.dimension());
		XA_DEBUG_ASSERT(y.dimension() == m_inverseDiagonal.dimension());
		// @@ Wrap vector component-wise product into a separate function.
		const uint32_t D = x.dimension();
		for (uint32_t i = 0; i < D; i++) {
			y[i] = m_inverseDiagonal[i] * x[i];
		}
	}

private:
	FullVector m_inverseDiagonal;
};

// Linear solvers.
class Solver
{
public:
	// Solve the symmetric system: At·A·x = At·b
	static bool LeastSquaresSolver(const sparse::Matrix &A, const FullVector &b, FullVector &x, float epsilon = 1e-5f)
	{
		XA_DEBUG_ASSERT(A.width() == x.dimension());
		XA_DEBUG_ASSERT(A.height() == b.dimension());
		XA_DEBUG_ASSERT(A.height() >= A.width()); // @@ If height == width we could solve it directly...
		const uint32_t D = A.width();
		sparse::Matrix At(A.height(), A.width());
		sparse::transpose(A, At);
		FullVector Atb(D);
		sparse::mult(At, b, Atb);
		sparse::Matrix AtA(D);
		sparse::mult(At, A, AtA);
		return SymmetricSolver(AtA, Atb, x, epsilon);
	}

	// See section 10.4.3 in: Mesh Parameterization: Theory and Practice, Siggraph Course Notes, August 2007
	static bool LeastSquaresSolver(const sparse::Matrix &A, const FullVector &b, FullVector &x, const uint32_t *lockedParameters, uint32_t lockedCount, float epsilon = 1e-5f)
	{
		XA_DEBUG_ASSERT(A.width() == x.dimension());
		XA_DEBUG_ASSERT(A.height() == b.dimension());
		XA_DEBUG_ASSERT(A.height() >= A.width() - lockedCount);
		// @@ This is not the most efficient way of building a system with reduced degrees of freedom. It would be faster to do it on the fly.
		const uint32_t D = A.width() - lockedCount;
		XA_DEBUG_ASSERT(D > 0);
		// Compute: b - Al * xl
		FullVector b_Alxl(b);
		for (uint32_t y = 0; y < A.height(); y++) {
			const uint32_t count = A.getRow(y).size();
			for (uint32_t e = 0; e < count; e++) {
				uint32_t column = A.getRow(y)[e].x;
				bool isFree = true;
				for (uint32_t i = 0; i < lockedCount; i++) {
					isFree &= (lockedParameters[i] != column);
				}
				if (!isFree) {
					b_Alxl[y] -= x[column] * A.getRow(y)[e].v;
				}
			}
		}
		// Remove locked columns from A.
		sparse::Matrix Af(D, A.height());
		for (uint32_t y = 0; y < A.height(); y++) {
			const uint32_t count = A.getRow(y).size();
			for (uint32_t e = 0; e < count; e++) {
				uint32_t column = A.getRow(y)[e].x;
				uint32_t ix = column;
				bool isFree = true;
				for (uint32_t i = 0; i < lockedCount; i++) {
					isFree &= (lockedParameters[i] != column);
					if (column > lockedParameters[i]) ix--; // shift columns
				}
				if (isFree) {
					Af.setCoefficient(ix, y, A.getRow(y)[e].v);
				}
			}
		}
		// Remove elements from x
		FullVector xf(D);
		for (uint32_t i = 0, j = 0; i < A.width(); i++) {
			bool isFree = true;
			for (uint32_t l = 0; l < lockedCount; l++) {
				isFree &= (lockedParameters[l] != i);
			}
			if (isFree) {
				xf[j++] = x[i];
			}
		}
		// Solve reduced system.
		bool result = LeastSquaresSolver(Af, b_Alxl, xf, epsilon);
		// Copy results back to x.
		for (uint32_t i = 0, j = 0; i < A.width(); i++) {
			bool isFree = true;
			for (uint32_t l = 0; l < lockedCount; l++) {
				isFree &= (lockedParameters[l] != i);
			}
			if (isFree) {
				x[i] = xf[j++];
			}
		}
		return result;
	}

private:
	/**
	* Compute the solution of the sparse linear system Ab=x using the Conjugate
	* Gradient method.
	*
	* Solving sparse linear systems:
	* (1)		A·x = b
	*
	* The conjugate gradient algorithm solves (1) only in the case that A is
	* symmetric and positive definite. It is based on the idea of minimizing the
	* function
	*
	* (2)		f(x) = 1/2·x·A·x - b·x
	*
	* This function is minimized when its gradient
	*
	* (3)		df = A·x - b
	*
	* is zero, which is equivalent to (1). The minimization is carried out by
	* generating a succession of search directions p.k and improved minimizers x.k.
	* At each stage a quantity alfa.k is found that minimizes f(x.k + alfa.k·p.k),
	* and x.k+1 is set equal to the new point x.k + alfa.k·p.k. The p.k and x.k are
	* built up in such a way that x.k+1 is also the minimizer of f over the whole
	* vector space of directions already taken, {p.1, p.2, . . . , p.k}. After N
	* iterations you arrive at the minimizer over the entire vector space, i.e., the
	* solution to (1).
	*
	* For a really good explanation of the method see:
	*
	* "An Introduction to the Conjugate Gradient Method Without the Agonizing Pain",
	* Jonhathan Richard Shewchuk.
	*
	**/
	static bool ConjugateGradientSolver(const sparse::Matrix &A, const FullVector &b, FullVector &x, float epsilon)
	{
		XA_DEBUG_ASSERT( A.isSquare() );
		XA_DEBUG_ASSERT( A.width() == b.dimension() );
		XA_DEBUG_ASSERT( A.width() == x.dimension() );
		int i = 0;
		const int D = A.width();
		const int i_max = 4 * D;   // Convergence should be linear, but in some cases, it's not.
		FullVector r(D);   // residual
		FullVector p(D);   // search direction
		FullVector q(D);   //
		float delta_0;
		float delta_old;
		float delta_new;
		float alpha;
		float beta;
		// r = b - A·x;
		sparse::copy(b, r);
		sparse::sgemv(-1, A, x, 1, r);
		// p = r;
		sparse::copy(r, p);
		delta_new = sparse::dot( r, r );
		delta_0 = delta_new;
		while (i < i_max && delta_new > epsilon * epsilon * delta_0) {
			i++;
			// q = A·p
			mult(A, p, q);
			// alpha = delta_new / p·q
			alpha = delta_new / sparse::dot( p, q );
			// x = alfa·p + x
			sparse::saxpy(alpha, p, x);
			if ((i & 31) == 0) { // recompute r after 32 steps
				// r = b - A·x
				sparse::copy(b, r);
				sparse::sgemv(-1, A, x, 1, r);
			} else {
				// r = r - alpha·q
				sparse::saxpy(-alpha, q, r);
			}
			delta_old = delta_new;
			delta_new = sparse::dot( r, r );
			beta = delta_new / delta_old;
			// p = beta·p + r
			sparse::scal(beta, p);
			sparse::saxpy(1, r, p);
		}
		return delta_new <= epsilon * epsilon * delta_0;
	}


	// Conjugate gradient with preconditioner.
	static bool ConjugateGradientSolver(const JacobiPreconditioner &preconditioner, const sparse::Matrix &A, const FullVector &b, FullVector &x, float epsilon)
	{
		XA_DEBUG_ASSERT( A.isSquare() );
		XA_DEBUG_ASSERT( A.width() == b.dimension() );
		XA_DEBUG_ASSERT( A.width() == x.dimension() );
		int i = 0;
		const int D = A.width();
		const int i_max = 4 * D;   // Convergence should be linear, but in some cases, it's not.
		FullVector r(D);    // residual
		FullVector p(D);    // search direction
		FullVector q(D);    //
		FullVector s(D);    // preconditioned
		float delta_0;
		float delta_old;
		float delta_new;
		float alpha;
		float beta;
		// r = b - A·x
		sparse::copy(b, r);
		sparse::sgemv(-1, A, x, 1, r);
		// p = M^-1 · r
		preconditioner.apply(r, p);
		delta_new = sparse::dot(r, p);
		delta_0 = delta_new;
		while (i < i_max && delta_new > epsilon * epsilon * delta_0) {
			i++;
			// q = A·p
			mult(A, p, q);
			// alpha = delta_new / p·q
			alpha = delta_new / sparse::dot(p, q);
			// x = alfa·p + x
			sparse::saxpy(alpha, p, x);
			if ((i & 31) == 0) { // recompute r after 32 steps
				// r = b - A·x
				sparse::copy(b, r);
				sparse::sgemv(-1, A, x, 1, r);
			} else {
				// r = r - alfa·q
				sparse::saxpy(-alpha, q, r);
			}
			// s = M^-1 · r
			preconditioner.apply(r, s);
			delta_old = delta_new;
			delta_new = sparse::dot( r, s );
			beta = delta_new / delta_old;
			// p = s + beta·p
			sparse::scal(beta, p);
			sparse::saxpy(1, s, p);
		}
		return delta_new <= epsilon * epsilon * delta_0;
	}

	static bool SymmetricSolver(const sparse::Matrix &A, const FullVector &b, FullVector &x, float epsilon = 1e-5f)
	{
		XA_DEBUG_ASSERT(A.height() == A.width());
		XA_DEBUG_ASSERT(A.height() == b.dimension());
		XA_DEBUG_ASSERT(b.dimension() == x.dimension());
		JacobiPreconditioner jacobi(A, true);
		return ConjugateGradientSolver(jacobi, A, b, x, epsilon);
	}
};

namespace param {
class Atlas;
class Chart;

// Fast sweep in 3 directions
static bool findApproximateDiameterVertices(halfedge::Mesh *mesh, halfedge::Vertex **a, halfedge::Vertex **b)
{
	XA_DEBUG_ASSERT(mesh != NULL);
	XA_DEBUG_ASSERT(a != NULL);
	XA_DEBUG_ASSERT(b != NULL);
	const uint32_t vertexCount = mesh->vertexCount();
	halfedge::Vertex *minVertex[3];
	halfedge::Vertex *maxVertex[3];
	minVertex[0] = minVertex[1] = minVertex[2] = NULL;
	maxVertex[0] = maxVertex[1] = maxVertex[2] = NULL;
	for (uint32_t v = 1; v < vertexCount; v++) {
		halfedge::Vertex *vertex = mesh->vertexAt(v);
		XA_DEBUG_ASSERT(vertex != NULL);
		if (vertex->isBoundary()) {
			minVertex[0] = minVertex[1] = minVertex[2] = vertex;
			maxVertex[0] = maxVertex[1] = maxVertex[2] = vertex;
			break;
		}
	}
	if (minVertex[0] == NULL) {
		// Input mesh has not boundaries.
		return false;
	}
	for (uint32_t v = 1; v < vertexCount; v++) {
		halfedge::Vertex *vertex = mesh->vertexAt(v);
		XA_DEBUG_ASSERT(vertex != NULL);
		if (!vertex->isBoundary()) {
			// Skip interior vertices.
			continue;
		}
		if (vertex->pos.x < minVertex[0]->pos.x) minVertex[0] = vertex;
		else if (vertex->pos.x > maxVertex[0]->pos.x) maxVertex[0] = vertex;
		if (vertex->pos.y < minVertex[1]->pos.y) minVertex[1] = vertex;
		else if (vertex->pos.y > maxVertex[1]->pos.y) maxVertex[1] = vertex;
		if (vertex->pos.z < minVertex[2]->pos.z) minVertex[2] = vertex;
		else if (vertex->pos.z > maxVertex[2]->pos.z) maxVertex[2] = vertex;
	}
	float lengths[3];
	for (int i = 0; i < 3; i++) {
		lengths[i] = length(minVertex[i]->pos - maxVertex[i]->pos);
	}
	if (lengths[0] > lengths[1] && lengths[0] > lengths[2]) {
		*a = minVertex[0];
		*b = maxVertex[0];
	} else if (lengths[1] > lengths[2]) {
		*a = minVertex[1];
		*b = maxVertex[1];
	} else {
		*a = minVertex[2];
		*b = maxVertex[2];
	}
	return true;
}

// Conformal relations from Brecht Van Lommel (based on ABF):

static float vec_angle_cos(Vector3::Arg v1, Vector3::Arg v2, Vector3::Arg v3)
{
	Vector3 d1 = v1 - v2;
	Vector3 d2 = v3 - v2;
	return clamp(dot(d1, d2) / (length(d1) * length(d2)), -1.0f, 1.0f);
}

static float vec_angle(Vector3::Arg v1, Vector3::Arg v2, Vector3::Arg v3)
{
	float dot = vec_angle_cos(v1, v2, v3);
	return acosf(dot);
}

static void triangle_angles(Vector3::Arg v1, Vector3::Arg v2, Vector3::Arg v3, float *a1, float *a2, float *a3)
{
	*a1 = vec_angle(v3, v1, v2);
	*a2 = vec_angle(v1, v2, v3);
	*a3 = float(M_PI - *a2 - *a1);
}

static void setup_abf_relations(sparse::Matrix &A, int row, const halfedge::Vertex *v0, const halfedge::Vertex *v1, const halfedge::Vertex *v2)
{
	int id0 = v0->id;
	int id1 = v1->id;
	int id2 = v2->id;
	Vector3 p0 = v0->pos;
	Vector3 p1 = v1->pos;
	Vector3 p2 = v2->pos;
	// @@ IC: Wouldn't it be more accurate to return cos and compute 1-cos^2?
	// It does indeed seem to be a little bit more robust.
	// @@ Need to revisit this more carefully!
	float a0, a1, a2;
	triangle_angles(p0, p1, p2, &a0, &a1, &a2);
	float s0 = sinf(a0);
	float s1 = sinf(a1);
	float s2 = sinf(a2);
	if (s1 > s0 && s1 > s2) {
		std::swap(s1, s2);
		std::swap(s0, s1);
		std::swap(a1, a2);
		std::swap(a0, a1);
		std::swap(id1, id2);
		std::swap(id0, id1);
	} else if (s0 > s1 && s0 > s2) {
		std::swap(s0, s2);
		std::swap(s0, s1);
		std::swap(a0, a2);
		std::swap(a0, a1);
		std::swap(id0, id2);
		std::swap(id0, id1);
	}
	float c0 = cosf(a0);
	float ratio = (s2 == 0.0f) ? 1.0f : s1 / s2;
	float cosine = c0 * ratio;
	float sine = s0 * ratio;
	// Note  : 2*id + 0 --> u
	//         2*id + 1 --> v
	int u0_id = 2 * id0 + 0;
	int v0_id = 2 * id0 + 1;
	int u1_id = 2 * id1 + 0;
	int v1_id = 2 * id1 + 1;
	int u2_id = 2 * id2 + 0;
	int v2_id = 2 * id2 + 1;
	// Real part
	A.setCoefficient(u0_id, 2 * row + 0, cosine - 1.0f);
	A.setCoefficient(v0_id, 2 * row + 0, -sine);
	A.setCoefficient(u1_id, 2 * row + 0, -cosine);
	A.setCoefficient(v1_id, 2 * row + 0, sine);
	A.setCoefficient(u2_id, 2 * row + 0, 1);
	// Imaginary part
	A.setCoefficient(u0_id, 2 * row + 1, sine);
	A.setCoefficient(v0_id, 2 * row + 1, cosine - 1.0f);
	A.setCoefficient(u1_id, 2 * row + 1, -sine);
	A.setCoefficient(v1_id, 2 * row + 1, -cosine);
	A.setCoefficient(v2_id, 2 * row + 1, 1);
}

static bool computeLeastSquaresConformalMap(halfedge::Mesh *mesh)
{
	XA_DEBUG_ASSERT(mesh != NULL);
	// For this to work properly, mesh should not have colocals that have the same
	// attributes, unless you want the vertices to actually have different texcoords.
	const uint32_t vertexCount = mesh->vertexCount();
	const uint32_t D = 2 * vertexCount;
	const uint32_t N = 2 * halfedge::countMeshTriangles(mesh);
	// N is the number of equations (one per triangle)
	// D is the number of variables (one per vertex; there are 2 pinned vertices).
	if (N < D - 4) {
		return false;
	}
	sparse::Matrix A(D, N);
	FullVector b(N);
	FullVector x(D);
	// Fill b:
	b.fill(0.0f);
	// Fill x:
	halfedge::Vertex *v0;
	halfedge::Vertex *v1;
	if (!findApproximateDiameterVertices(mesh, &v0, &v1)) {
		// Mesh has no boundaries.
		return false;
	}
	if (v0->tex == v1->tex) {
		// LSCM expects an existing parameterization.
		return false;
	}
	for (uint32_t v = 0; v < vertexCount; v++) {
		halfedge::Vertex *vertex = mesh->vertexAt(v);
		XA_DEBUG_ASSERT(vertex != NULL);
		// Initial solution.
		x[2 * v + 0] = vertex->tex.x;
		x[2 * v + 1] = vertex->tex.y;
	}
	// Fill A:
	const uint32_t faceCount = mesh->faceCount();
	for (uint32_t f = 0, t = 0; f < faceCount; f++) {
		const halfedge::Face *face = mesh->faceAt(f);
		XA_DEBUG_ASSERT(face != NULL);
		XA_DEBUG_ASSERT(face->edgeCount() == 3);
		const halfedge::Vertex *vertex0 = NULL;
		for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
			const halfedge::Edge *edge = it.current();
			XA_ASSERT(edge != NULL);
			if (vertex0 == NULL) {
				vertex0 = edge->vertex;
			} else if (edge->next->vertex != vertex0) {
				const halfedge::Vertex *vertex1 = edge->from();
				const halfedge::Vertex *vertex2 = edge->to();
				setup_abf_relations(A, t, vertex0, vertex1, vertex2);
				//setup_conformal_map_relations(A, t, vertex0, vertex1, vertex2);
				t++;
			}
		}
	}
	const uint32_t lockedParameters[] = {
		2 * v0->id + 0,
		2 * v0->id + 1,
		2 * v1->id + 0,
		2 * v1->id + 1
	};
	// Solve
	Solver::LeastSquaresSolver(A, b, x, lockedParameters, 4, 0.000001f);
	// Map x back to texcoords:
	for (uint32_t v = 0; v < vertexCount; v++) {
		halfedge::Vertex *vertex = mesh->vertexAt(v);
		XA_DEBUG_ASSERT(vertex != NULL);
		vertex->tex = Vector2(x[2 * v + 0], x[2 * v + 1]);
	}
	return true;
}

static bool computeOrthogonalProjectionMap(halfedge::Mesh *mesh)
{
	Vector3 axis[2];
	uint32_t vertexCount = mesh->vertexCount();
	Array<Vector3> points(vertexCount);
	points.resize(vertexCount);
	for (uint32_t i = 0; i < vertexCount; i++) {
		points[i] = mesh->vertexAt(i)->pos;
	}
	// Avoid redundant computations.
	float matrix[6];
	Fit::computeCovariance(vertexCount, points.data(), matrix);
	if (matrix[0] == 0 && matrix[3] == 0 && matrix[5] == 0) {
		return false;
	}
	float eigenValues[3];
	Vector3 eigenVectors[3];
	if (!Fit::eigenSolveSymmetric3(matrix, eigenValues, eigenVectors)) {
		return false;
	}
	axis[0] = normalize(eigenVectors[0]);
	axis[1] = normalize(eigenVectors[1]);
	// Project vertices to plane.
	for (halfedge::Mesh::VertexIterator it(mesh->vertices()); !it.isDone(); it.advance()) {
		halfedge::Vertex *vertex = it.current();
		vertex->tex.x = dot(axis[0], vertex->pos);
		vertex->tex.y = dot(axis[1], vertex->pos);
	}
	return true;
}

static void computeSingleFaceMap(halfedge::Mesh *mesh)
{
	XA_DEBUG_ASSERT(mesh != NULL);
	XA_DEBUG_ASSERT(mesh->faceCount() == 1);
	halfedge::Face *face = mesh->faceAt(0);
	XA_ASSERT(face != NULL);
	Vector3 p0 = face->edge->from()->pos;
	Vector3 p1 = face->edge->to()->pos;
	Vector3 X = normalizeSafe(p1 - p0, Vector3(0.0f), 0.0f);
	Vector3 Z = face->normal();
	Vector3 Y = normalizeSafe(cross(Z, X), Vector3(0.0f), 0.0f);
	uint32_t i = 0;
	for (halfedge::Face::EdgeIterator it(face->edges()); !it.isDone(); it.advance(), i++) {
		halfedge::Vertex *vertex = it.vertex();
		XA_ASSERT(vertex != NULL);
		if (i == 0) {
			vertex->tex = Vector2(0);
		} else {
			Vector3 pn = vertex->pos;
			float xn = dot((pn - p0), X);
			float yn = dot((pn - p0), Y);
			vertex->tex = Vector2(xn, yn);
		}
	}
}

// Dummy implementation of a priority queue using sort at insertion.
// - Insertion is o(n)
// - Smallest element goes at the end, so that popping it is o(1).
// - Resorting is n*log(n)
// @@ Number of elements in the queue is usually small, and we'd have to rebalance often. I'm not sure it's worth implementing a heap.
// @@ Searcing at removal would remove the need for sorting when priorities change.
struct PriorityQueue
{
	PriorityQueue(uint32_t size = UINT_MAX) : maxSize(size) {}

	void push(float priority, uint32_t face)
	{
		uint32_t i = 0;
		const uint32_t count = pairs.size();
		for (; i < count; i++) {
			if (pairs[i].priority > priority) break;
		}
		Pair p = { priority, face };
		pairs.insertAt(i, p);
		if (pairs.size() > maxSize)
			pairs.removeAt(0);
	}

	// push face out of order, to be sorted later.
	void push(uint32_t face)
	{
		Pair p = { 0.0f, face };
		pairs.push_back(p);
	}

	uint32_t pop()
	{
		uint32_t f = pairs.back().face;
		pairs.pop_back();
		return f;
	}

	void sort()
	{
		//sort(pairs); // @@ My intro sort appears to be much slower than it should!
		std::sort(pairs.begin(), pairs.end());
	}

	void clear()
	{
		pairs.clear();
	}

	uint32_t count() const
	{
		return pairs.size();
	}

	float firstPriority() const
	{
		return pairs.back().priority;
	}

	const uint32_t maxSize;

	struct Pair
	{
		bool operator <(const Pair &p) const
		{
			return priority > p.priority;    // !! Sort in inverse priority order!
		}

		float priority;
		uint32_t face;
	};

	Array<Pair> pairs;
};

struct ChartBuildData
{
	ChartBuildData(int id) : id(id)
	{
		planeNormal = Vector3(0);
		centroid = Vector3(0);
		coneAxis = Vector3(0);
		coneAngle = 0;
		area = 0;
		boundaryLength = 0;
		normalSum = Vector3(0);
		centroidSum = Vector3(0);
	}

	int id;

	// Proxy info:
	Vector3 planeNormal;
	Vector3 centroid;
	Vector3 coneAxis;
	float coneAngle;

	float area;
	float boundaryLength;
	Vector3 normalSum;
	Vector3 centroidSum;

	Array<uint32_t> seeds;  // @@ These could be a pointers to the halfedge faces directly.
	Array<uint32_t> faces;
	PriorityQueue candidates;
};

struct AtlasBuilder
{
	AtlasBuilder(const halfedge::Mesh *m, const CharterOptions &options) : mesh(m), facesLeft(m->faceCount()), m_options(options)
	{
		const uint32_t faceCount = m->faceCount();
		faceChartArray.resize(faceCount, -1);
		faceCandidateArray.resize(faceCount, (uint32_t)-1);
		// @@ Floyd for the whole mesh is too slow. We could compute floyd progressively per patch as the patch grows. We need a better solution to compute most central faces.
		//computeShortestPaths();
		// Precompute edge lengths and face areas.
		uint32_t edgeCount = m->edgeCount();
		edgeLengths.resize(edgeCount);
		for (uint32_t i = 0; i < edgeCount; i++) {
			uint32_t id = m->edgeAt(i)->id;
			XA_DEBUG_ASSERT(id / 2 == i);
#ifdef NDEBUG
			id = id; // silence unused parameter warning
#endif
			const halfedge::Edge *edge = m->edgeAt(i);
			if (edge->face->flags & halfedge::FaceFlags::Ignore)
				edgeLengths[i] = 0;
			else
				edgeLengths[i] = edge->length();
		}
		faceAreas.resize(faceCount);
		for (uint32_t i = 0; i < faceCount; i++) {
			const halfedge::Face *face = m->faceAt(i);
			if (face->flags & halfedge::FaceFlags::Ignore)
				faceAreas[i] = 0;
			else
				faceAreas[i] = face->area();
		}
	}

	~AtlasBuilder()
	{
		const uint32_t chartCount = chartArray.size();
		for (uint32_t i = 0; i < chartCount; i++) {
			delete chartArray[i];
		}
	}

	void markUnchartedFaces(const Array<uint32_t> &unchartedFaces)
	{
		const uint32_t unchartedFaceCount = unchartedFaces.size();
		for (uint32_t i = 0; i < unchartedFaceCount; i++) {
			uint32_t f = unchartedFaces[i];
			faceChartArray[f] = -2;
			//faceCandidateArray[f] = -2; // @@ ?
			removeCandidate(f);
		}
		XA_DEBUG_ASSERT(facesLeft >= unchartedFaceCount);
		facesLeft -= unchartedFaceCount;
	}

	void computeShortestPaths()
	{
		const uint32_t faceCount = mesh->faceCount();
		shortestPaths.resize(faceCount * faceCount, FLT_MAX);
		// Fill edges:
		for (uint32_t i = 0; i < faceCount; i++) {
			shortestPaths[i * faceCount + i] = 0.0f;
			const halfedge::Face *face_i = mesh->faceAt(i);
			Vector3 centroid_i = face_i->centroid();
			for (halfedge::Face::ConstEdgeIterator it(face_i->edges()); !it.isDone(); it.advance()) {
				const halfedge::Edge *edge = it.current();
				if (!edge->isBoundary()) {
					const halfedge::Face *face_j = edge->pair->face;
					uint32_t j = face_j->id;
					Vector3 centroid_j = face_j->centroid();
					shortestPaths[i * faceCount + j] = shortestPaths[j * faceCount + i] = length(centroid_i - centroid_j);
				}
			}
		}
		// Use Floyd-Warshall algorithm to compute all paths:
		for (uint32_t k = 0; k < faceCount; k++) {
			for (uint32_t i = 0; i < faceCount; i++) {
				for (uint32_t j = 0; j < faceCount; j++) {
					shortestPaths[i * faceCount + j] = std::min(shortestPaths[i * faceCount + j], shortestPaths[i * faceCount + k] + shortestPaths[k * faceCount + j]);
				}
			}
		}
	}

	void placeSeeds(float threshold, uint32_t maxSeedCount)
	{
		// Instead of using a predefiened number of seeds:
		// - Add seeds one by one, growing chart until a certain treshold.
		// - Undo charts and restart growing process.
		// @@ How can we give preference to faces far from sharp features as in the LSCM paper?
		//   - those points can be found using a simple flood filling algorithm.
		//   - how do we weight the probabilities?
		for (uint32_t i = 0; i < maxSeedCount; i++) {
			if (facesLeft == 0) {
				// No faces left, stop creating seeds.
				break;
			}
			createRandomChart(threshold);
		}
	}

	void createRandomChart(float threshold)
	{
		ChartBuildData *chart = new ChartBuildData(chartArray.size());
		chartArray.push_back(chart);
		// Pick random face that is not used by any chart yet.
		uint32_t randomFaceIdx = rand.getRange(facesLeft - 1);
		uint32_t i = 0;
		for (uint32_t f = 0; f != randomFaceIdx; f++, i++) {
			while (faceChartArray[i] != -1) i++;
		}
		while (faceChartArray[i] != -1) i++;
		chart->seeds.push_back(i);
		addFaceToChart(chart, i, true);
		// Grow the chart as much as possible within the given threshold.
		growChart(chart, threshold * 0.5f, facesLeft);
		//growCharts(threshold - threshold * 0.75f / chartCount(), facesLeft);
	}

	void addFaceToChart(ChartBuildData *chart, uint32_t f, bool recomputeProxy = false)
	{
		// Add face to chart.
		chart->faces.push_back(f);
		XA_DEBUG_ASSERT(faceChartArray[f] == -1);
		faceChartArray[f] = chart->id;
		facesLeft--;
		// Update area and boundary length.
		chart->area = evaluateChartArea(chart, f);
		chart->boundaryLength = evaluateBoundaryLength(chart, f);
		chart->normalSum = evaluateChartNormalSum(chart, f);
		chart->centroidSum = evaluateChartCentroidSum(chart, f);
		if (recomputeProxy) {
			// Update proxy and candidate's priorities.
			updateProxy(chart);
		}
		// Update candidates.
		removeCandidate(f);
		updateCandidates(chart, f);
		updatePriorities(chart);
	}

	// Returns true if any of the charts can grow more.
	bool growCharts(float threshold, uint32_t faceCount)
	{
		// Using one global list.
		faceCount = std::min(faceCount, facesLeft);
		for (uint32_t i = 0; i < faceCount; i++) {
			const Candidate &candidate = getBestCandidate();
			if (candidate.metric > threshold) {
				return false; // Can't grow more.
			}
			addFaceToChart(candidate.chart, candidate.face);
		}
		return facesLeft != 0; // Can continue growing.
	}

	bool growChart(ChartBuildData *chart, float threshold, uint32_t faceCount)
	{
		// Try to add faceCount faces within threshold to chart.
		for (uint32_t i = 0; i < faceCount; ) {
			if (chart->candidates.count() == 0 || chart->candidates.firstPriority() > threshold) {
				return false;
			}
			uint32_t f = chart->candidates.pop();
			if (faceChartArray[f] == -1) {
				addFaceToChart(chart, f);
				i++;
			}
		}
		if (chart->candidates.count() == 0 || chart->candidates.firstPriority() > threshold) {
			return false;
		}
		return true;
	}

	void resetCharts()
	{
		const uint32_t faceCount = mesh->faceCount();
		for (uint32_t i = 0; i < faceCount; i++) {
			faceChartArray[i] = -1;
			faceCandidateArray[i] = (uint32_t)-1;
		}
		facesLeft = faceCount;
		candidateArray.clear();
		const uint32_t chartCount = chartArray.size();
		for (uint32_t i = 0; i < chartCount; i++) {
			ChartBuildData *chart = chartArray[i];
			const uint32_t seed = chart->seeds.back();
			chart->area = 0.0f;
			chart->boundaryLength = 0.0f;
			chart->normalSum = Vector3(0);
			chart->centroidSum = Vector3(0);
			chart->faces.clear();
			chart->candidates.clear();
			addFaceToChart(chart, seed);
		}
	}

	void updateCandidates(ChartBuildData *chart, uint32_t f)
	{
		const halfedge::Face *face = mesh->faceAt(f);
		// Traverse neighboring faces, add the ones that do not belong to any chart yet.
		for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
			const halfedge::Edge *edge = it.current()->pair;
			if (!edge->isBoundary()) {
				uint32_t faceId = edge->face->id;
				if (faceChartArray[faceId] == -1) {
					chart->candidates.push(faceId);
				}
			}
		}
	}

	void updateProxies()
	{
		const uint32_t chartCount = chartArray.size();
		for (uint32_t i = 0; i < chartCount; i++) {
			updateProxy(chartArray[i]);
		}
	}

	void updateProxy(ChartBuildData *chart)
	{
		//#pragma message(NV_FILE_LINE "TODO: Use best fit plane instead of average normal.")
		chart->planeNormal = normalizeSafe(chart->normalSum, Vector3(0), 0.0f);
		chart->centroid = chart->centroidSum / float(chart->faces.size());
	}

	bool relocateSeeds()
	{
		bool anySeedChanged = false;
		const uint32_t chartCount = chartArray.size();
		for (uint32_t i = 0; i < chartCount; i++) {
			if (relocateSeed(chartArray[i])) {
				anySeedChanged = true;
			}
		}
		return anySeedChanged;
	}

	bool relocateSeed(ChartBuildData *chart)
	{
		Vector3 centroid = computeChartCentroid(chart);
		const uint32_t N = 10;  // @@ Hardcoded to 10?
		PriorityQueue bestTriangles(N);
		// Find the first N triangles that fit the proxy best.
		const uint32_t faceCount = chart->faces.size();
		for (uint32_t i = 0; i < faceCount; i++) {
			float priority = evaluateProxyFitMetric(chart, chart->faces[i]);
			bestTriangles.push(priority, chart->faces[i]);
		}
		// Of those, choose the most central triangle.
		uint32_t mostCentral;
		float maxDistance = -1;
		const uint32_t bestCount = bestTriangles.count();
		for (uint32_t i = 0; i < bestCount; i++) {
			const halfedge::Face *face = mesh->faceAt(bestTriangles.pairs[i].face);
			Vector3 faceCentroid = face->triangleCenter();
			float distance = length(centroid - faceCentroid);
			if (distance > maxDistance) {
				maxDistance = distance;
				mostCentral = bestTriangles.pairs[i].face;
			}
		}
		XA_DEBUG_ASSERT(maxDistance >= 0);
		// In order to prevent k-means cyles we record all the previously chosen seeds.
		uint32_t index = (uint32_t)(std::find(chart->seeds.begin(), chart->seeds.end(), mostCentral) - chart->seeds.begin());
		if (index < chart->seeds.size()) {
			// Move new seed to the end of the seed array.
			uint32_t last = chart->seeds.size() - 1;
			std::swap(chart->seeds[index], chart->seeds[last]);
			return false;
		} else {
			// Append new seed.
			chart->seeds.push_back(mostCentral);
			return true;
		}
	}

	void updatePriorities(ChartBuildData *chart)
	{
		// Re-evaluate candidate priorities.
		uint32_t candidateCount = chart->candidates.count();
		for (uint32_t i = 0; i < candidateCount; i++) {
			chart->candidates.pairs[i].priority = evaluatePriority(chart, chart->candidates.pairs[i].face);
			if (faceChartArray[chart->candidates.pairs[i].face] == -1) {
				updateCandidate(chart, chart->candidates.pairs[i].face, chart->candidates.pairs[i].priority);
			}
		}
		// Sort candidates.
		chart->candidates.sort();
	}

	// Evaluate combined metric.
	float evaluatePriority(ChartBuildData *chart, uint32_t face)
	{
		// Estimate boundary length and area:
		float newBoundaryLength = evaluateBoundaryLength(chart, face);
		float newChartArea = evaluateChartArea(chart, face);
		float F = evaluateProxyFitMetric(chart, face);
		float C = evaluateRoundnessMetric(chart, face, newBoundaryLength, newChartArea);
		float P = evaluateStraightnessMetric(chart, face);
		// Penalize faces that cross seams, reward faces that close seams or reach boundaries.
		float N = evaluateNormalSeamMetric(chart, face);
		float T = evaluateTextureSeamMetric(chart, face);
		//float R = evaluateCompletenessMetric(chart, face);
		//float D = evaluateDihedralAngleMetric(chart, face);
		// @@ Add a metric based on local dihedral angle.
		// @@ Tweaking the normal and texture seam metrics.
		// - Cause more impedance. Never cross 90 degree edges.
		// -
		float cost = float(
			m_options.proxyFitMetricWeight * F +
			m_options.roundnessMetricWeight * C +
			m_options.straightnessMetricWeight * P +
			m_options.normalSeamMetricWeight * N +
			m_options.textureSeamMetricWeight * T);
		// Enforce limits strictly:
		if (newChartArea > m_options.maxChartArea) cost = FLT_MAX;
		if (newBoundaryLength > m_options.maxBoundaryLength) cost = FLT_MAX;
		// Make sure normal seams are fully respected:
		if (m_options.normalSeamMetricWeight >= 1000 && N != 0) cost = FLT_MAX;
		XA_ASSERT(std::isfinite(cost));
		return cost;
	}

	// Returns a value in [0-1].
	float evaluateProxyFitMetric(ChartBuildData *chart, uint32_t f)
	{
		const halfedge::Face *face = mesh->faceAt(f);
		Vector3 faceNormal = face->triangleNormal();
		// Use plane fitting metric for now:
		return 1 - dot(faceNormal, chart->planeNormal); // @@ normal deviations should be weighted by face area
	}

	float evaluateRoundnessMetric(ChartBuildData *chart, uint32_t /*face*/, float newBoundaryLength, float newChartArea)
	{
		float roundness = square(chart->boundaryLength) / chart->area;
		float newRoundness = square(newBoundaryLength) / newChartArea;
		if (newRoundness > roundness) {
			return square(newBoundaryLength) / float(newChartArea * 4 * M_PI);
		} else {
			// Offer no impedance to faces that improve roundness.
			return 0;
		}
	}

	float evaluateStraightnessMetric(ChartBuildData *chart, uint32_t f)
	{
		float l_out = 0.0f;
		float l_in = 0.0f;
		const halfedge::Face *face = mesh->faceAt(f);
		if (face->flags & halfedge::FaceFlags::Ignore)
			return 1.0f;
		for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
			const halfedge::Edge *edge = it.current();
			float l = edgeLengths[edge->id / 2];
			if (edge->isBoundary()) {
				l_out += l;
			} else {
				uint32_t neighborFaceId = edge->pair->face->id;
				if (faceChartArray[neighborFaceId] != chart->id) {
					l_out += l;
				} else {
					l_in += l;
				}
			}
		}
		XA_DEBUG_ASSERT(l_in != 0.0f); // Candidate face must be adjacent to chart. @@ This is not true if the input mesh has zero-length edges.
		float ratio = (l_out - l_in) / (l_out + l_in);
		return std::min(ratio, 0.0f); // Only use the straightness metric to close gaps.
	}

	float evaluateNormalSeamMetric(ChartBuildData *chart, uint32_t f)
	{
		float seamFactor = 0.0f;
		float totalLength = 0.0f;
		const halfedge::Face *face = mesh->faceAt(f);
		for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
			const halfedge::Edge *edge = it.current();
			if (edge->isBoundary()) {
				continue;
			}
			const uint32_t neighborFaceId = edge->pair->face->id;
			if (faceChartArray[neighborFaceId] != chart->id) {
				continue;
			}
			//float l = edge->length();
			float l = edgeLengths[edge->id / 2];
			totalLength += l;
			if (!edge->isSeam()) {
				continue;
			}
			// Make sure it's a normal seam.
			if (edge->isNormalSeam()) {
				float d0 = clamp(dot(edge->vertex->nor, edge->pair->next->vertex->nor), 0.0f, 1.0f);
				float d1 = clamp(dot(edge->next->vertex->nor, edge->pair->vertex->nor), 0.0f, 1.0f);
				l *= 1 - (d0 + d1) * 0.5f;
				seamFactor += l;
			}
		}
		if (seamFactor == 0) return 0.0f;
		return seamFactor / totalLength;
	}

	float evaluateTextureSeamMetric(ChartBuildData *chart, uint32_t f)
	{
		float seamLength = 0.0f;
		float totalLength = 0.0f;
		const halfedge::Face *face = mesh->faceAt(f);
		for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
			const halfedge::Edge *edge = it.current();
			if (edge->isBoundary()) {
				continue;
			}
			const uint32_t neighborFaceId = edge->pair->face->id;
			if (faceChartArray[neighborFaceId] != chart->id) {
				continue;
			}
			//float l = edge->length();
			float l = edgeLengths[edge->id / 2];
			totalLength += l;
			if (!edge->isSeam()) {
				continue;
			}
			// Make sure it's a texture seam.
			if (edge->isTextureSeam()) {
				seamLength += l;
			}
		}
		if (seamLength == 0.0f) {
			return 0.0f; // Avoid division by zero.
		}
		return seamLength / totalLength;
	}

	float evaluateChartArea(ChartBuildData *chart, uint32_t f)
	{
		const halfedge::Face *face = mesh->faceAt(f);
		return chart->area + faceAreas[face->id];
	}

	float evaluateBoundaryLength(ChartBuildData *chart, uint32_t f)
	{
		float boundaryLength = chart->boundaryLength;
		// Add new edges, subtract edges shared with the chart.
		const halfedge::Face *face = mesh->faceAt(f);
		for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
			const halfedge::Edge *edge = it.current();
			//float edgeLength = edge->length();
			float edgeLength = edgeLengths[edge->id / 2];
			if (edge->isBoundary()) {
				boundaryLength += edgeLength;
			} else {
				uint32_t neighborFaceId = edge->pair->face->id;
				if (faceChartArray[neighborFaceId] != chart->id) {
					boundaryLength += edgeLength;
				} else {
					boundaryLength -= edgeLength;
				}
			}
		}
		return std::max(0.0f, boundaryLength);  // @@ Hack!
	}

	Vector3 evaluateChartNormalSum(ChartBuildData *chart, uint32_t f)
	{
		const halfedge::Face *face = mesh->faceAt(f);
		return chart->normalSum + face->triangleNormalAreaScaled();
	}

	Vector3 evaluateChartCentroidSum(ChartBuildData *chart, uint32_t f)
	{
		const halfedge::Face *face = mesh->faceAt(f);
		return chart->centroidSum + face->centroid();
	}

	Vector3 computeChartCentroid(const ChartBuildData *chart)
	{
		Vector3 centroid(0);
		const uint32_t faceCount = chart->faces.size();
		for (uint32_t i = 0; i < faceCount; i++) {
			const halfedge::Face *face = mesh->faceAt(chart->faces[i]);
			centroid += face->triangleCenter();
		}
		return centroid / float(faceCount);
	}

	void fillHoles(float threshold)
	{
		while (facesLeft > 0)
			createRandomChart(threshold);
	}

	void mergeCharts()
	{
		Array<float> sharedBoundaryLengths;
		const uint32_t chartCount = chartArray.size();
		for (int c = chartCount - 1; c >= 0; c--) {
			sharedBoundaryLengths.clear();
			sharedBoundaryLengths.resize(chartCount, 0.0f);
			ChartBuildData *chart = chartArray[c];
			float externalBoundary = 0.0f;
			const uint32_t faceCount = chart->faces.size();
			for (uint32_t i = 0; i < faceCount; i++) {
				uint32_t f = chart->faces[i];
				const halfedge::Face *face = mesh->faceAt(f);
				for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
					const halfedge::Edge *edge = it.current();
					//float l = edge->length();
					float l = edgeLengths[edge->id / 2];
					if (edge->isBoundary()) {
						externalBoundary += l;
					} else {
						uint32_t neighborFace = edge->pair->face->id;
						int neighborChart = faceChartArray[neighborFace];
						if (neighborChart != c) {
							if ((edge->isSeam() && (edge->isNormalSeam() || edge->isTextureSeam())) || neighborChart == -2) {
								externalBoundary += l;
							} else {
								sharedBoundaryLengths[neighborChart] += l;
							}
						}
					}
				}
			}
			for (int cc = chartCount - 1; cc >= 0; cc--) {
				if (cc == c)
					continue;
				ChartBuildData *chart2 = chartArray[cc];
				if (chart2 == NULL)
					continue;
				if (sharedBoundaryLengths[cc] > 0.8 * std::max(0.0f, chart->boundaryLength - externalBoundary)) {
					// Try to avoid degenerate configurations.
					if (chart2->boundaryLength > sharedBoundaryLengths[cc]) {
						if (dot(chart2->planeNormal, chart->planeNormal) > -0.25) {
							mergeChart(chart2, chart, sharedBoundaryLengths[cc]);
							delete chart;
							chartArray[c] = NULL;
							break;
						}
					}
				}
				if (sharedBoundaryLengths[cc] > 0.20 * std::max(0.0f, chart->boundaryLength - externalBoundary)) {
					// Compare proxies.
					if (dot(chart2->planeNormal, chart->planeNormal) > 0) {
						mergeChart(chart2, chart, sharedBoundaryLengths[cc]);
						delete chart;
						chartArray[c] = NULL;
						break;
					}
				}
			}
		}
		// Remove deleted charts.
		for (int c = 0; c < int32_t(chartArray.size()); /*do not increment if removed*/) {
			if (chartArray[c] == NULL) {
				chartArray.removeAt(c);
				// Update faceChartArray.
				const uint32_t faceCount = faceChartArray.size();
				for (uint32_t i = 0; i < faceCount; i++) {
					XA_DEBUG_ASSERT (faceChartArray[i] != -1);
					XA_DEBUG_ASSERT (faceChartArray[i] != c);
					XA_DEBUG_ASSERT (faceChartArray[i] <= int32_t(chartArray.size()));
					if (faceChartArray[i] > c) {
						faceChartArray[i]--;
					}
				}
			} else {
				chartArray[c]->id = c;
				c++;
			}
		}
	}

	// @@ Cleanup.
	struct Candidate {
		uint32_t face;
		ChartBuildData *chart;
		float metric;
	};

	// @@ Get N best candidates in one pass.
	const Candidate &getBestCandidate() const
	{
		uint32_t best = 0;
		float bestCandidateMetric = FLT_MAX;
		const uint32_t candidateCount = candidateArray.size();
		XA_ASSERT(candidateCount > 0);
		for (uint32_t i = 0; i < candidateCount; i++) {
			const Candidate &candidate = candidateArray[i];
			if (candidate.metric < bestCandidateMetric) {
				bestCandidateMetric = candidate.metric;
				best = i;
			}
		}
		return candidateArray[best];
	}

	void removeCandidate(uint32_t f)
	{
		int c = faceCandidateArray[f];
		if (c != -1) {
			faceCandidateArray[f] = (uint32_t)-1;
			if (c == int(candidateArray.size() - 1)) {
				candidateArray.pop_back();
			} else {
				// Replace with last.
				candidateArray[c] = candidateArray[candidateArray.size() - 1];
				candidateArray.pop_back();
				faceCandidateArray[candidateArray[c].face] = c;
			}
		}
	}

	void updateCandidate(ChartBuildData *chart, uint32_t f, float metric)
	{
		if (faceCandidateArray[f] == (uint32_t)-1) {
			const uint32_t index = candidateArray.size();
			faceCandidateArray[f] = index;
			candidateArray.resize(index + 1);
			candidateArray[index].face = f;
			candidateArray[index].chart = chart;
			candidateArray[index].metric = metric;
		} else {
			int c = faceCandidateArray[f];
			XA_DEBUG_ASSERT(c != -1);
			Candidate &candidate = candidateArray[c];
			XA_DEBUG_ASSERT(candidate.face == f);
			if (metric < candidate.metric || chart == candidate.chart) {
				candidate.metric = metric;
				candidate.chart = chart;
			}
		}
	}

	void mergeChart(ChartBuildData *owner, ChartBuildData *chart, float sharedBoundaryLength)
	{
		const uint32_t faceCount = chart->faces.size();
		for (uint32_t i = 0; i < faceCount; i++) {
			uint32_t f = chart->faces[i];
			XA_DEBUG_ASSERT(faceChartArray[f] == chart->id);
			faceChartArray[f] = owner->id;
			owner->faces.push_back(f);
		}
		// Update adjacencies?
		owner->area += chart->area;
		owner->boundaryLength += chart->boundaryLength - sharedBoundaryLength;
		owner->normalSum += chart->normalSum;
		owner->centroidSum += chart->centroidSum;
		updateProxy(owner);
	}


	uint32_t chartCount() const { return chartArray.size(); }
	const Array<uint32_t> &chartFaces(uint32_t i) const { return chartArray[i]->faces; }

	const halfedge::Mesh *mesh;
	uint32_t facesLeft;
	Array<int> faceChartArray;
	Array<ChartBuildData *> chartArray;
	Array<float> shortestPaths;
	Array<float> edgeLengths;
	Array<float> faceAreas;
	Array<Candidate> candidateArray; //
	Array<uint32_t> faceCandidateArray; // Map face index to candidate index.
	MTRand rand;

private:
	CharterOptions m_options;
};

/// A chart is a connected set of faces with a certain topology (usually a disk).
class Chart
{
public:
	Chart() : scale(1.0f), blockAligned(true), m_chartMesh(NULL), m_unifiedMesh(NULL), m_isDisk(false), m_isVertexMapped(false) {}
	~Chart()
	{
		delete m_chartMesh;
		delete m_unifiedMesh;
	}

	void build(const halfedge::Mesh *originalMesh, const Array<uint32_t> &faceArray)
	{
		// Copy face indices.
		m_faceArray = faceArray;
		const uint32_t meshVertexCount = originalMesh->vertexCount();
		if (m_chartMesh)
			delete m_chartMesh;
		m_chartMesh = new halfedge::Mesh();
		if (m_unifiedMesh)
			delete m_unifiedMesh;
		m_unifiedMesh = new halfedge::Mesh();
		Array<uint32_t> chartMeshIndices;
		chartMeshIndices.resize(meshVertexCount, (uint32_t)~0);
		Array<uint32_t> unifiedMeshIndices;
		unifiedMeshIndices.resize(meshVertexCount, (uint32_t)~0);
		// Add vertices.
		const uint32_t faceCount = faceArray.size();
		for (uint32_t f = 0; f < faceCount; f++) {
			const halfedge::Face *face = originalMesh->faceAt(faceArray[f]);
			XA_DEBUG_ASSERT(face != NULL);
			for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
				const halfedge::Vertex *vertex = it.current()->vertex;
				const halfedge::Vertex *unifiedVertex = vertex->firstColocal();
				if (unifiedMeshIndices[unifiedVertex->id] == (uint32_t)~0) {
					unifiedMeshIndices[unifiedVertex->id] = m_unifiedMesh->vertexCount();
					XA_DEBUG_ASSERT(vertex->pos == unifiedVertex->pos);
					m_unifiedMesh->addVertex(vertex->pos);
				}
				if (chartMeshIndices[vertex->id] == (uint32_t)~0) {
					chartMeshIndices[vertex->id] = m_chartMesh->vertexCount();
					m_chartToOriginalMap.push_back(vertex->id);
					m_chartToUnifiedMap.push_back(unifiedMeshIndices[unifiedVertex->id]);
					halfedge::Vertex *v = m_chartMesh->addVertex(vertex->pos);
					v->nor = vertex->nor;
					v->tex = vertex->tex;
				}
			}
		}
		// This is ignoring the canonical map:
		// - Is it really necessary to link colocals?
		m_chartMesh->linkColocals();
		//m_unifiedMesh->linkColocals();  // Not strictly necessary, no colocals in the unified mesh. # Wrong.
		// This check is not valid anymore, if the original mesh vertices were linked with a canonical map, then it might have
		// some colocal vertices that were unlinked. So, the unified mesh might have some duplicate vertices, because firstColocal()
		// is not guaranteed to return the same vertex for two colocal vertices.
		//XA_ASSERT(m_chartMesh->colocalVertexCount() == m_unifiedMesh->vertexCount());
		// Is that OK? What happens in meshes were that happens? Does anything break? Apparently not...
		Array<uint32_t> faceIndices;
		faceIndices.reserve(7);
		// Add faces.
		for (uint32_t f = 0; f < faceCount; f++) {
			const halfedge::Face *face = originalMesh->faceAt(faceArray[f]);
			XA_DEBUG_ASSERT(face != NULL);
			faceIndices.clear();
			for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
				const halfedge::Vertex *vertex = it.current()->vertex;
				XA_DEBUG_ASSERT(vertex != NULL);
				faceIndices.push_back(chartMeshIndices[vertex->id]);
			}
			m_chartMesh->addFace(faceIndices, face->flags);
			faceIndices.clear();
			for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
				const halfedge::Vertex *vertex = it.current()->vertex;
				XA_DEBUG_ASSERT(vertex != NULL);
				vertex = vertex->firstColocal();
				faceIndices.push_back(unifiedMeshIndices[vertex->id]);
			}
			m_unifiedMesh->addFace(faceIndices, face->flags);
		}
		m_chartMesh->linkBoundary();
		m_unifiedMesh->linkBoundary();
		//exportMesh(m_unifiedMesh.ptr(), "debug_input.obj");
		if (m_unifiedMesh->splitBoundaryEdges()) {
			halfedge::Mesh *newUnifiedMesh = halfedge::unifyVertices(m_unifiedMesh);
			delete m_unifiedMesh;
			m_unifiedMesh = newUnifiedMesh;
		}
		//exportMesh(m_unifiedMesh.ptr(), "debug_split.obj");
		// Closing the holes is not always the best solution and does not fix all the problems.
		// We need to do some analysis of the holes and the genus to:
		// - Find cuts that reduce genus.
		// - Find cuts to connect holes.
		// - Use minimal spanning trees or seamster.
		if (!closeHoles()) {
			/*static int pieceCount = 0;
			StringBuilder fileName;
			fileName.format("debug_hole_%d.obj", pieceCount++);
			exportMesh(m_unifiedMesh.ptr(), fileName.str());*/
		}
		halfedge::Mesh *newUnifiedMesh = halfedge::triangulate(m_unifiedMesh);
		delete m_unifiedMesh;
		m_unifiedMesh = newUnifiedMesh;
		//exportMesh(m_unifiedMesh.ptr(), "debug_triangulated.obj");
		// Analyze chart topology.
		halfedge::MeshTopology topology(m_unifiedMesh);
		m_isDisk = topology.isDisk();
	}

	void buildVertexMap(const halfedge::Mesh *originalMesh)
	{
		XA_ASSERT(m_chartMesh == NULL && m_unifiedMesh == NULL);
		m_isVertexMapped = true;
		// Build face indices.
		m_faceArray.clear();
		const uint32_t meshFaceCount = originalMesh->faceCount();
		for (uint32_t f = 0; f < meshFaceCount; f++) {
			const halfedge::Face *face = originalMesh->faceAt(f);
			if ((face->flags & halfedge::FaceFlags::Ignore) != 0)
				m_faceArray.push_back(f);
		}
		const uint32_t faceCount = m_faceArray.size();
		if (faceCount == 0) {
			return;
		}
		// @@ The chartMesh construction is basically the same as with regular charts, don't duplicate!
		const uint32_t meshVertexCount = originalMesh->vertexCount();
		if (m_chartMesh)
			delete m_chartMesh;
		m_chartMesh = new halfedge::Mesh();
		Array<uint32_t> chartMeshIndices;
		chartMeshIndices.resize(meshVertexCount, (uint32_t)~0);
		// Vertex map mesh only has disconnected vertices.
		for (uint32_t f = 0; f < faceCount; f++) {
			const halfedge::Face *face = originalMesh->faceAt(m_faceArray[f]);
			XA_DEBUG_ASSERT(face != NULL);
			for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
				const halfedge::Vertex *vertex = it.current()->vertex;
				if (chartMeshIndices[vertex->id] == (uint32_t)~0) {
					chartMeshIndices[vertex->id] = m_chartMesh->vertexCount();
					m_chartToOriginalMap.push_back(vertex->id);
					halfedge::Vertex *v = m_chartMesh->addVertex(vertex->pos);
					v->nor = vertex->nor;
					v->tex = vertex->tex; // @@ Not necessary.
				}
			}
		}
		// @@ Link colocals using the original mesh canonical map? Build canonical map on the fly? Do we need to link colocals at all for this?
		//m_chartMesh->linkColocals();
		Array<uint32_t> faceIndices;
		faceIndices.reserve(7);
		// Add faces.
		for (uint32_t f = 0; f < faceCount; f++) {
			const halfedge::Face *face = originalMesh->faceAt(m_faceArray[f]);
			XA_DEBUG_ASSERT(face != NULL);
			faceIndices.clear();
			for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
				const halfedge::Vertex *vertex = it.current()->vertex;
				XA_DEBUG_ASSERT(vertex != NULL);
				XA_DEBUG_ASSERT(chartMeshIndices[vertex->id] != (uint32_t)~0);
				faceIndices.push_back(chartMeshIndices[vertex->id]);
			}
			halfedge::Face *new_face = m_chartMesh->addFace(faceIndices);
			XA_DEBUG_ASSERT(new_face != NULL);
#ifdef NDEBUG
			new_face = new_face; // silence unused parameter warning
#endif
		}
	}

	bool closeHoles()
	{
		XA_DEBUG_ASSERT(!m_isVertexMapped);
		Array<halfedge::Edge *> boundaryEdges;
		getBoundaryEdges(m_unifiedMesh, boundaryEdges);
		uint32_t boundaryCount = boundaryEdges.size();
		if (boundaryCount <= 1) {
			// Nothing to close.
			return true;
		}
		// Compute lengths and areas.
		Array<float> boundaryLengths;
		for (uint32_t i = 0; i < boundaryCount; i++) {
			const halfedge::Edge *startEdge = boundaryEdges[i];
			XA_ASSERT(startEdge->face == NULL);
			//float boundaryEdgeCount = 0;
			float boundaryLength = 0.0f;
			//Vector3 boundaryCentroid(zero);
			const halfedge::Edge *edge = startEdge;
			do {
				Vector3 t0 = edge->from()->pos;
				Vector3 t1 = edge->to()->pos;
				//boundaryEdgeCount++;
				boundaryLength += length(t1 - t0);
				//boundaryCentroid += edge->vertex()->pos;
				edge = edge->next;
			} while (edge != startEdge);
			boundaryLengths.push_back(boundaryLength);
			//boundaryCentroids.append(boundaryCentroid / boundaryEdgeCount);
		}
		// Find disk boundary.
		uint32_t diskBoundary = 0;
		float maxLength = boundaryLengths[0];
		for (uint32_t i = 1; i < boundaryCount; i++) {
			if (boundaryLengths[i] > maxLength) {
				maxLength = boundaryLengths[i];
				diskBoundary = i;
			}
		}
		// Close holes.
		for (uint32_t i = 0; i < boundaryCount; i++) {
			if (diskBoundary == i) {
				// Skip disk boundary.
				continue;
			}
			halfedge::Edge *startEdge = boundaryEdges[i];
			XA_DEBUG_ASSERT(startEdge != NULL);
			XA_DEBUG_ASSERT(startEdge->face == NULL);
			Array<halfedge::Vertex *> vertexLoop;
			Array<halfedge::Edge *> edgeLoop;
			halfedge::Edge *edge = startEdge;
			do {
				halfedge::Vertex *vertex = edge->next->vertex;  // edge->to()
				uint32_t j;
				for (j = 0; j < vertexLoop.size(); j++) {
					if (vertex->isColocal(vertexLoop[j])) {
						break;
					}
				}
				bool isCrossing = (j != vertexLoop.size());
				if (isCrossing) {
					halfedge::Edge *prev = edgeLoop[j];     // Previous edge before the loop.
					halfedge::Edge *next = edge->next;    // Next edge after the loop.
					XA_DEBUG_ASSERT(prev->to()->isColocal(next->from()));
					// Close loop.
					edgeLoop.push_back(edge);
					closeLoop(j + 1, edgeLoop);
					// Link boundary loop.
					prev->setNext(next);
					vertex->setEdge(next);
					// Start over again.
					vertexLoop.clear();
					edgeLoop.clear();
					edge = startEdge;
					vertex = edge->to();
				}
				vertexLoop.push_back(vertex);
				edgeLoop.push_back(edge);
				edge = edge->next;
			} while (edge != startEdge);
			closeLoop(0, edgeLoop);
		}
		getBoundaryEdges(m_unifiedMesh, boundaryEdges);
		boundaryCount = boundaryEdges.size();
		XA_DEBUG_ASSERT(boundaryCount == 1);
		return boundaryCount == 1;
	}

	bool isDisk() const
	{
		return m_isDisk;
	}
	bool isVertexMapped() const
	{
		return m_isVertexMapped;
	}

	uint32_t vertexCount() const
	{
		return m_chartMesh->vertexCount();
	}
	uint32_t colocalVertexCount() const
	{
		return m_unifiedMesh->vertexCount();
	}

	uint32_t faceCount() const
	{
		return m_faceArray.size();
	}
	uint32_t faceAt(uint32_t i) const
	{
		return m_faceArray[i];
	}

	const halfedge::Mesh *chartMesh() const
	{
		return m_chartMesh;
	}
	halfedge::Mesh *chartMesh()
	{
		return m_chartMesh;
	}
	const halfedge::Mesh *unifiedMesh() const
	{
		return m_unifiedMesh;
	}
	halfedge::Mesh *unifiedMesh()
	{
		return m_unifiedMesh;
	}

	//uint32_t vertexIndex(uint32_t i) const { return m_vertexIndexArray[i]; }

	uint32_t mapChartVertexToOriginalVertex(uint32_t i) const
	{
		return m_chartToOriginalMap[i];
	}
	uint32_t mapChartVertexToUnifiedVertex(uint32_t i) const
	{
		return m_chartToUnifiedMap[i];
	}

	const Array<uint32_t> &faceArray() const
	{
		return m_faceArray;
	}

	// Transfer parameterization from unified mesh to chart mesh.
	void transferParameterization()
	{
		XA_DEBUG_ASSERT(!m_isVertexMapped);
		uint32_t vertexCount = m_chartMesh->vertexCount();
		for (uint32_t v = 0; v < vertexCount; v++) {
			halfedge::Vertex *vertex = m_chartMesh->vertexAt(v);
			halfedge::Vertex *unifiedVertex = m_unifiedMesh->vertexAt(mapChartVertexToUnifiedVertex(v));
			vertex->tex = unifiedVertex->tex;
		}
	}

	float computeSurfaceArea() const
	{
		return halfedge::computeSurfaceArea(m_chartMesh) * scale;
	}

	float computeParametricArea() const
	{
		// This only makes sense in parameterized meshes.
		XA_DEBUG_ASSERT(m_isDisk);
		XA_DEBUG_ASSERT(!m_isVertexMapped);
		return halfedge::computeParametricArea(m_chartMesh);
	}

	Vector2 computeParametricBounds() const
	{
		// This only makes sense in parameterized meshes.
		XA_DEBUG_ASSERT(m_isDisk);
		XA_DEBUG_ASSERT(!m_isVertexMapped);
		Vector2 minCorner(FLT_MAX, FLT_MAX);
		Vector2 maxCorner(-FLT_MAX, -FLT_MAX);
		uint32_t vertexCount = m_chartMesh->vertexCount();
		for (uint32_t v = 0; v < vertexCount; v++) {
			halfedge::Vertex *vertex = m_chartMesh->vertexAt(v);
			minCorner = min(minCorner, vertex->tex);
			maxCorner = max(maxCorner, vertex->tex);
		}
		return (maxCorner - minCorner) * 0.5f;
	}

	float scale;
	uint32_t vertexMapWidth;
	uint32_t vertexMapHeight;
	bool blockAligned;

private:
	bool closeLoop(uint32_t start, const Array<halfedge::Edge *> &loop)
	{
		const uint32_t vertexCount = loop.size() - start;
		XA_DEBUG_ASSERT(vertexCount >= 3);
		if (vertexCount < 3) return false;
		XA_DEBUG_ASSERT(loop[start]->vertex->isColocal(loop[start + vertexCount - 1]->to()));
		// If the hole is planar, then we add a single face that will be properly triangulated later.
		// If the hole is not planar, we add a triangle fan with a vertex at the hole centroid.
		// This is still a bit of a hack. There surely are better hole filling algorithms out there.
		Array<Vector3> points;
		points.resize(vertexCount);
		for (uint32_t i = 0; i < vertexCount; i++) {
			points[i] = loop[start + i]->vertex->pos;
		}
		bool isPlanar = Fit::isPlanar(vertexCount, points.data());
		if (isPlanar) {
			// Add face and connect edges.
			halfedge::Face *face = m_unifiedMesh->addFace();
			for (uint32_t i = 0; i < vertexCount; i++) {
				halfedge::Edge *edge = loop[start + i];
				edge->face = face;
				edge->setNext(loop[start + (i + 1) % vertexCount]);
			}
			face->edge = loop[start];
			XA_DEBUG_ASSERT(face->isValid());
		} else {
			// If the polygon is not planar, we just cross our fingers, and hope this will work:
			// Compute boundary centroid:
			Vector3 centroidPos(0);
			for (uint32_t i = 0; i < vertexCount; i++) {
				centroidPos += points[i];
			}
			centroidPos *= (1.0f / vertexCount);
			halfedge::Vertex *centroid = m_unifiedMesh->addVertex(centroidPos);
			// Add one pair of edges for each boundary vertex.
			for (uint32_t j = vertexCount - 1, i = 0; i < vertexCount; j = i++) {
				halfedge::Face *face = m_unifiedMesh->addFace(centroid->id, loop[start + j]->vertex->id, loop[start + i]->vertex->id);
				XA_DEBUG_ASSERT(face != NULL);
#ifdef NDEBUG
				face = face; // silence unused parameter warning
#endif
			}
		}
		return true;
	}

	static void getBoundaryEdges(halfedge::Mesh *mesh, Array<halfedge::Edge *> &boundaryEdges)
	{
		XA_DEBUG_ASSERT(mesh != NULL);
		const uint32_t edgeCount = mesh->edgeCount();
		BitArray bitFlags(edgeCount);
		bitFlags.clearAll();
		boundaryEdges.clear();
		// Search for boundary edges. Mark all the edges that belong to the same boundary.
		for (uint32_t e = 0; e < edgeCount; e++) {
			halfedge::Edge *startEdge = mesh->edgeAt(e);
			if (startEdge != NULL && startEdge->isBoundary() && bitFlags.bitAt(e) == false) {
				XA_DEBUG_ASSERT(startEdge->face != NULL);
				XA_DEBUG_ASSERT(startEdge->pair->face == NULL);
				startEdge = startEdge->pair;
				const halfedge::Edge *edge = startEdge;
				do {
					XA_DEBUG_ASSERT(edge->face == NULL);
					XA_DEBUG_ASSERT(bitFlags.bitAt(edge->id / 2) == false);
					bitFlags.setBitAt(edge->id / 2);
					edge = edge->next;
				} while (startEdge != edge);
				boundaryEdges.push_back(startEdge);
			}
		}
	}

	// Chart mesh.
	halfedge::Mesh *m_chartMesh;
	halfedge::Mesh *m_unifiedMesh;
	bool m_isDisk;
	bool m_isVertexMapped;
	
	// List of faces of the original mesh that belong to this chart.
	Array<uint32_t> m_faceArray;

	// Map vertices of the chart mesh to vertices of the original mesh.
	Array<uint32_t> m_chartToOriginalMap;

	Array<uint32_t> m_chartToUnifiedMap;
};

// Estimate quality of existing parameterization.
class ParameterizationQuality
{
public:
	ParameterizationQuality()
	{
		m_totalTriangleCount = 0;
		m_flippedTriangleCount = 0;
		m_zeroAreaTriangleCount = 0;
		m_parametricArea = 0.0f;
		m_geometricArea = 0.0f;
		m_stretchMetric = 0.0f;
		m_maxStretchMetric = 0.0f;
		m_conformalMetric = 0.0f;
		m_authalicMetric = 0.0f;
	}

	ParameterizationQuality(const halfedge::Mesh *mesh)
	{
		XA_DEBUG_ASSERT(mesh != NULL);
		m_totalTriangleCount = 0;
		m_flippedTriangleCount = 0;
		m_zeroAreaTriangleCount = 0;
		m_parametricArea = 0.0f;
		m_geometricArea = 0.0f;
		m_stretchMetric = 0.0f;
		m_maxStretchMetric = 0.0f;
		m_conformalMetric = 0.0f;
		m_authalicMetric = 0.0f;
		const uint32_t faceCount = mesh->faceCount();
		for (uint32_t f = 0; f < faceCount; f++) {
			const halfedge::Face *face = mesh->faceAt(f);
			const halfedge::Vertex *vertex0 = NULL;
			Vector3 p[3];
			Vector2 t[3];
			for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
				const halfedge::Edge *edge = it.current();
				if (vertex0 == NULL) {
					vertex0 = edge->vertex;
					p[0] = vertex0->pos;
					t[0] = vertex0->tex;
				} else if (edge->to() != vertex0) {
					p[1] = edge->from()->pos;
					p[2] = edge->to()->pos;
					t[1] = edge->from()->tex;
					t[2] = edge->to()->tex;
					processTriangle(p, t);
				}
			}
		}
		if (m_flippedTriangleCount + m_zeroAreaTriangleCount == faceCount) {
			// If all triangles are flipped, then none is.
			m_flippedTriangleCount = 0;
		}
		XA_DEBUG_ASSERT(std::isfinite(m_parametricArea) && m_parametricArea >= 0);
		XA_DEBUG_ASSERT(std::isfinite(m_geometricArea) && m_geometricArea >= 0);
		XA_DEBUG_ASSERT(std::isfinite(m_stretchMetric));
		XA_DEBUG_ASSERT(std::isfinite(m_maxStretchMetric));
		XA_DEBUG_ASSERT(std::isfinite(m_conformalMetric));
		XA_DEBUG_ASSERT(std::isfinite(m_authalicMetric));
	}

	bool isValid() const
	{
		return m_flippedTriangleCount == 0; // @@ Does not test for self-overlaps.
	}

	float rmsStretchMetric() const
	{
		if (m_geometricArea == 0) return 0.0f;
		float normFactor = sqrtf(m_parametricArea / m_geometricArea);
		return sqrtf(m_stretchMetric / m_geometricArea) * normFactor;
	}

	float maxStretchMetric() const
	{
		if (m_geometricArea == 0) return 0.0f;
		float normFactor = sqrtf(m_parametricArea / m_geometricArea);
		return m_maxStretchMetric * normFactor;
	}

	float rmsConformalMetric() const
	{
		if (m_geometricArea == 0) return 0.0f;
		return sqrtf(m_conformalMetric / m_geometricArea);
	}

	float maxAuthalicMetric() const
	{
		if (m_geometricArea == 0) return 0.0f;
		return sqrtf(m_authalicMetric / m_geometricArea);
	}

	void operator+=(const ParameterizationQuality &pq)
	{
		m_totalTriangleCount += pq.m_totalTriangleCount;
		m_flippedTriangleCount += pq.m_flippedTriangleCount;
		m_zeroAreaTriangleCount += pq.m_zeroAreaTriangleCount;
		m_parametricArea += pq.m_parametricArea;
		m_geometricArea += pq.m_geometricArea;
		m_stretchMetric += pq.m_stretchMetric;
		m_maxStretchMetric = std::max(m_maxStretchMetric, pq.m_maxStretchMetric);
		m_conformalMetric += pq.m_conformalMetric;
		m_authalicMetric += pq.m_authalicMetric;
	}

private:
	void processTriangle(Vector3 q[3], Vector2 p[3])
	{
		m_totalTriangleCount++;
		// Evaluate texture stretch metric. See:
		// - "Texture Mapping Progressive Meshes", Sander, Snyder, Gortler & Hoppe
		// - "Mesh Parameterization: Theory and Practice", Siggraph'07 Course Notes, Hormann, Levy & Sheffer.
		float t1 = p[0].x;
		float s1 = p[0].y;
		float t2 = p[1].x;
		float s2 = p[1].y;
		float t3 = p[2].x;
		float s3 = p[2].y;
		float geometricArea = length(cross(q[1] - q[0], q[2] - q[0])) / 2;
		float parametricArea = ((s2 - s1) * (t3 - t1) - (s3 - s1) * (t2 - t1)) / 2;
		if (isZero(parametricArea)) {
			m_zeroAreaTriangleCount++;
			return;
		}
		Vector3 Ss = (q[0] * (t2 - t3) + q[1] * (t3 - t1) + q[2] * (t1 - t2)) / (2 * parametricArea);
		Vector3 St = (q[0] * (s3 - s2) + q[1] * (s1 - s3) + q[2] * (s2 - s1)) / (2 * parametricArea);
		float a = dot(Ss, Ss); // E
		float b = dot(Ss, St); // F
		float c = dot(St, St); // G
		// Compute eigen-values of the first fundamental form:
		float sigma1 = sqrtf(0.5f * std::max(0.0f, a + c - sqrtf(square(a - c) + 4 * square(b)))); // gamma uppercase, min eigenvalue.
		float sigma2 = sqrtf(0.5f * std::max(0.0f, a + c + sqrtf(square(a - c) + 4 * square(b)))); // gamma lowercase, max eigenvalue.
		XA_ASSERT(sigma2 > sigma1 || equal(sigma1, sigma2));
		// isometric: sigma1 = sigma2 = 1
		// conformal: sigma1 / sigma2 = 1
		// authalic: sigma1 * sigma2 = 1
		float rmsStretch = sqrtf((a + c) * 0.5f);
		float rmsStretch2 = sqrtf((square(sigma1) + square(sigma2)) * 0.5f);
		XA_DEBUG_ASSERT(equal(rmsStretch, rmsStretch2, 0.01f));
#ifdef NDEBUG
		rmsStretch2 = rmsStretch2; // silence unused parameter warning
#endif
		if (parametricArea < 0.0f) {
			// Count flipped triangles.
			m_flippedTriangleCount++;
			parametricArea = fabsf(parametricArea);
		}
		m_stretchMetric += square(rmsStretch) * geometricArea;
		m_maxStretchMetric = std::max(m_maxStretchMetric, sigma2);
		if (!isZero(sigma1, 0.000001f)) {
			// sigma1 is zero when geometricArea is zero.
			m_conformalMetric += (sigma2 / sigma1) * geometricArea;
		}
		m_authalicMetric += (sigma1 * sigma2) * geometricArea;
		// Accumulate total areas.
		m_geometricArea += geometricArea;
		m_parametricArea += parametricArea;
		//triangleConformalEnergy(q, p);
	}

	uint32_t m_totalTriangleCount;
	uint32_t m_flippedTriangleCount;
	uint32_t m_zeroAreaTriangleCount;
	float m_parametricArea;
	float m_geometricArea;
	float m_stretchMetric;
	float m_maxStretchMetric;
	float m_conformalMetric;
	float m_authalicMetric;
};

// Set of charts corresponding to a single mesh.
class MeshCharts
{
public:
	MeshCharts(const halfedge::Mesh *mesh) : m_mesh(mesh) {}

	~MeshCharts()
	{
		for (size_t i = 0; i < m_chartArray.size(); i++)
			delete m_chartArray[(uint32_t)i];
	}

	uint32_t chartCount() const
	{
		return m_chartArray.size();
	}
	uint32_t vertexCount () const
	{
		return m_totalVertexCount;
	}

	const Chart *chartAt(uint32_t i) const
	{
		return m_chartArray[i];
	}
	Chart *chartAt(uint32_t i)
	{
		return m_chartArray[i];
	}

	// Extract the charts of the input mesh.
	void extractCharts()
	{
		const uint32_t faceCount = m_mesh->faceCount();
		int first = 0;
		Array<uint32_t> queue;
		queue.reserve(faceCount);
		BitArray bitFlags(faceCount);
		bitFlags.clearAll();
		for (uint32_t f = 0; f < faceCount; f++) {
			if (bitFlags.bitAt(f) == false) {
				// Start new patch. Reset queue.
				first = 0;
				queue.clear();
				queue.push_back(f);
				bitFlags.setBitAt(f);
				while (first != (int)queue.size()) {
					const halfedge::Face *face = m_mesh->faceAt(queue[first]);
					// Visit face neighbors of queue[first]
					for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
						const halfedge::Edge *edge = it.current();
						XA_DEBUG_ASSERT(edge->pair != NULL);
						if (!edge->isBoundary() && /*!edge->isSeam()*/
								//!(edge->from()->tex() != edge->pair()->to()->tex() || edge->to()->tex() != edge->pair()->from()->tex()))
								!(edge->from() != edge->pair->to() || edge->to() != edge->pair->from())) { // Preserve existing seams (not just texture seams).
							const halfedge::Face *neighborFace = edge->pair->face;
							XA_DEBUG_ASSERT(neighborFace != NULL);
							if (bitFlags.bitAt(neighborFace->id) == false) {
								queue.push_back(neighborFace->id);
								bitFlags.setBitAt(neighborFace->id);
							}
						}
					}
					first++;
				}
				Chart *chart = new Chart();
				chart->build(m_mesh, queue);
				m_chartArray.push_back(chart);
			}
		}
	}

	/*
	Compute charts using a simple segmentation algorithm.

	LSCM:
	- identify sharp features using local dihedral angles.
	- identify seed faces farthest from sharp features.
	- grow charts from these seeds.

	MCGIM:
	- phase 1: chart growth
	  - grow all charts simultaneously using dijkstra search on the dual graph of the mesh.
	  - graph edges are weighted based on planarity metric.
	  - metric uses distance to global chart normal.
	  - terminate when all faces have been assigned.
	- phase 2: seed computation:
	  - place new seed of the chart at the most interior face.
	  - most interior is evaluated using distance metric only.

	- method repeates the two phases, until the location of the seeds does not change.
	  - cycles are detected by recording all the previous seeds and chartification terminates.

	D-Charts:

	- Uniaxial conic metric:
	  - N_c = axis of the generalized cone that best fits the chart. (cone can a be cylinder or a plane).
	  - omega_c = angle between the face normals and the axis.
	  - Fitting error between chart C and tringle t: F(c,t) = (N_c*n_t - cos(omega_c))^2

	- Compactness metrics:
	  - Roundness:
		- C(c,t) = pi * D(S_c,t)^2 / A_c
		- S_c = chart seed.
		- D(S_c,t) = length of the shortest path inside the chart betwen S_c and t.
		- A_c = chart area.
	  - Straightness:
		- P(c,t) = l_out(c,t) / l_in(c,t)
		- l_out(c,t) = lenght of the edges not shared between C and t.
		- l_in(c,t) = lenght of the edges shared between C and t.

	- Combined metric:
	  - Cost(c,t) = F(c,t)^alpha + C(c,t)^beta + P(c,t)^gamma
	  - alpha = 1, beta = 0.7, gamma = 0.5

	Our basic approach:
	- Just one iteration of k-means?
	- Avoid dijkstra by greedily growing charts until a threshold is met. Increase threshold and repeat until no faces left.
	- If distortion metric is too high, split chart, add two seeds.
	- If chart size is low, try removing chart.

	Postprocess:
	- If topology is not disk:
	  - Fill holes, if new faces fit proxy.
	  - Find best cut, otherwise.
	- After parameterization:
	  - If boundary self-intersects:
		- cut chart along the closest two diametral boundary vertices, repeat parametrization.
		- what if the overlap is on an appendix? How do we find that out and cut appropiately?
		  - emphasize roundness metrics to prevent those cases.
	  - If interior self-overlaps: preserve boundary parameterization and use mean-value map.
	*/
	void computeCharts(const CharterOptions &options)
	{
		Chart *vertexMap = new Chart();
		vertexMap->buildVertexMap(m_mesh);
		if (vertexMap->faceCount() == 0) {
			delete vertexMap;
			vertexMap = NULL;
		}
		AtlasBuilder builder(m_mesh, options);
		if (vertexMap != NULL) {
			// Mark faces that do not need to be charted.
			builder.markUnchartedFaces(vertexMap->faceArray());
			m_chartArray.push_back(vertexMap);
		}
		if (builder.facesLeft != 0) {
			// This seems a reasonable estimate.
			uint32_t maxSeedCount = std::max(6U, builder.facesLeft);
			// Create initial charts greedely.
			XA_PRINT(PrintFlags::ComputingCharts, "### Placing seeds\n");
			builder.placeSeeds(options.maxThreshold, maxSeedCount);
			XA_PRINT(PrintFlags::ComputingCharts, "###   Placed %d seeds (max = %d)\n", builder.chartCount(), maxSeedCount);
			builder.updateProxies();
			builder.mergeCharts();
	#if 1
			XA_PRINT(PrintFlags::ComputingCharts, "### Relocating seeds\n");
			builder.relocateSeeds();
			XA_PRINT(PrintFlags::ComputingCharts, "### Reset charts\n");
			builder.resetCharts();
			if (vertexMap != NULL) {
				builder.markUnchartedFaces(vertexMap->faceArray());
			}
			XA_PRINT(PrintFlags::ComputingCharts, "### Growing charts\n");
			// Restart process growing charts in parallel.
			uint32_t iteration = 0;
			while (true) {
				if (!builder.growCharts(options.maxThreshold, options.growFaceCount)) {
					XA_PRINT(PrintFlags::ComputingCharts, "### Can't grow anymore\n");
					// If charts cannot grow more: fill holes, merge charts, relocate seeds and start new iteration.
					XA_PRINT(PrintFlags::ComputingCharts, "### Filling holes\n");
					builder.fillHoles(options.maxThreshold);
					XA_PRINT(PrintFlags::ComputingCharts, "###   Using %d charts now\n", builder.chartCount());
					builder.updateProxies();
					XA_PRINT(PrintFlags::ComputingCharts, "### Merging charts\n");
					builder.mergeCharts();
					XA_PRINT(PrintFlags::ComputingCharts, "###   Using %d charts now\n", builder.chartCount());
					XA_PRINT(PrintFlags::ComputingCharts, "### Reseeding\n");
					if (!builder.relocateSeeds()) {
						XA_PRINT(PrintFlags::ComputingCharts, "### Cannot relocate seeds anymore\n");
						// Done!
						break;
					}
					if (iteration == options.maxIterations) {
						XA_PRINT(PrintFlags::ComputingCharts, "### Reached iteration limit\n");
						break;
					}
					iteration++;
					XA_PRINT(PrintFlags::ComputingCharts, "### Reset charts\n");
					builder.resetCharts();
					if (vertexMap != NULL) {
						builder.markUnchartedFaces(vertexMap->faceArray());
					}
					XA_PRINT(PrintFlags::ComputingCharts, "### Growing charts\n");
				}
			};
	#endif
			// Make sure no holes are left!
			XA_DEBUG_ASSERT(builder.facesLeft == 0);
			const uint32_t chartCount = builder.chartArray.size();
			for (uint32_t i = 0; i < chartCount; i++) {
				Chart *chart = new Chart();
				m_chartArray.push_back(chart);
				chart->build(m_mesh, builder.chartFaces(i));
			}
		}
		const uint32_t chartCount = m_chartArray.size();
		// Build face indices.
		m_faceChart.resize(m_mesh->faceCount());
		m_faceIndex.resize(m_mesh->faceCount());
		for (uint32_t i = 0; i < chartCount; i++) {
			const Chart *chart = m_chartArray[i];
			const uint32_t faceCount = chart->faceCount();
			for (uint32_t f = 0; f < faceCount; f++) {
				uint32_t idx = chart->faceAt(f);
				m_faceChart[idx] = i;
				m_faceIndex[idx] = f;
			}
		}
		// Build an exclusive prefix sum of the chart vertex counts.
		m_chartVertexCountPrefixSum.resize(chartCount);
		if (chartCount > 0) {
			m_chartVertexCountPrefixSum[0] = 0;
			for (uint32_t i = 1; i < chartCount; i++) {
				const Chart *chart = m_chartArray[i - 1];
				m_chartVertexCountPrefixSum[i] = m_chartVertexCountPrefixSum[i - 1] + chart->vertexCount();
			}
			m_totalVertexCount = m_chartVertexCountPrefixSum[chartCount - 1] + m_chartArray[chartCount - 1]->vertexCount();
		} else {
			m_totalVertexCount = 0;
		}
	}

	void parameterizeCharts()
	{
		ParameterizationQuality globalParameterizationQuality;
		// Parameterize the charts.
		uint32_t diskCount = 0;
		const uint32_t chartCount = m_chartArray.size();
		for (uint32_t i = 0; i < chartCount; i++)
		{
			Chart *chart = m_chartArray[i];

			bool isValid = false;

			if (chart->isVertexMapped())
			{
				continue;
			}

			if (chart->isDisk())
			{
				diskCount++;
				ParameterizationQuality chartParameterizationQuality;
				if (chart->faceCount() == 1) {
					computeSingleFaceMap(chart->unifiedMesh());
					chartParameterizationQuality = ParameterizationQuality(chart->unifiedMesh());
				} else {
					computeOrthogonalProjectionMap(chart->unifiedMesh());
					ParameterizationQuality orthogonalQuality(chart->unifiedMesh());
					computeLeastSquaresConformalMap(chart->unifiedMesh());
					ParameterizationQuality lscmQuality(chart->unifiedMesh());
					chartParameterizationQuality = lscmQuality;
				}
				isValid = chartParameterizationQuality.isValid();
				if (!isValid) {
					XA_PRINT(PrintFlags::ParametizingCharts, "*** Invalid parameterization.\n");
				}
				// @@ Check that parameterization quality is above a certain threshold.
				// @@ Detect boundary self-intersections.
				globalParameterizationQuality += chartParameterizationQuality;
			}

			// Transfer parameterization from unified mesh to chart mesh.
			chart->transferParameterization();

		}
		XA_PRINT(PrintFlags::ParametizingCharts, "  Parameterized %d/%d charts.\n", diskCount, chartCount);
		XA_PRINT(PrintFlags::ParametizingCharts, "  RMS stretch metric: %f\n", globalParameterizationQuality.rmsStretchMetric());
		XA_PRINT(PrintFlags::ParametizingCharts, "  MAX stretch metric: %f\n", globalParameterizationQuality.maxStretchMetric());
		XA_PRINT(PrintFlags::ParametizingCharts, "  RMS conformal metric: %f\n", globalParameterizationQuality.rmsConformalMetric());
		XA_PRINT(PrintFlags::ParametizingCharts, "  RMS authalic metric: %f\n", globalParameterizationQuality.maxAuthalicMetric());
	}

	uint32_t faceChartAt(uint32_t i) const
	{
		return m_faceChart[i];
	}
	uint32_t faceIndexWithinChartAt(uint32_t i) const
	{
		return m_faceIndex[i];
	}

	uint32_t vertexCountBeforeChartAt(uint32_t i) const
	{
		return m_chartVertexCountPrefixSum[i];
	}

private:

	const halfedge::Mesh *m_mesh;

	Array<Chart *> m_chartArray;

	Array<uint32_t> m_chartVertexCountPrefixSum;
	uint32_t m_totalVertexCount;

	Array<uint32_t> m_faceChart; // the chart of every face of the input mesh.
	Array<uint32_t> m_faceIndex; // the index within the chart for every face of the input mesh.
};

/// An atlas is a set of charts.
class Atlas
{
public:
	~Atlas()
	{
		for (size_t i = 0; i < m_meshChartsArray.size(); i++)
			delete m_meshChartsArray[(uint32_t)i];
	}

	uint32_t meshCount() const
	{
		return m_meshChartsArray.size();
	}

	const MeshCharts *meshAt(uint32_t i) const
	{
		return m_meshChartsArray[i];
	}

	MeshCharts *meshAt(uint32_t i)
	{
		return m_meshChartsArray[i];
	}

	uint32_t chartCount() const
	{
		uint32_t count = 0;
		for (uint32_t c = 0; c < m_meshChartsArray.size(); c++) {
			count += m_meshChartsArray[c]->chartCount();
		}
		return count;
	}

	const Chart *chartAt(uint32_t i) const
	{
		for (uint32_t c = 0; c < m_meshChartsArray.size(); c++) {
			uint32_t count = m_meshChartsArray[c]->chartCount();
			if (i < count) {
				return m_meshChartsArray[c]->chartAt(i);
			}
			i -= count;
		}
		return NULL;
	}

	Chart *chartAt(uint32_t i)
	{
		for (uint32_t c = 0; c < m_meshChartsArray.size(); c++) {
			uint32_t count = m_meshChartsArray[c]->chartCount();
			if (i < count) {
				return m_meshChartsArray[c]->chartAt(i);
			}
			i -= count;
		}
		return NULL;
	}

	// Add mesh charts and takes ownership.
	// Extract the charts and add to this atlas.
	void addMeshCharts(MeshCharts *meshCharts)
	{
		m_meshChartsArray.push_back(meshCharts);
	}

	void extractCharts(const halfedge::Mesh *mesh)
	{
		MeshCharts *meshCharts = new MeshCharts(mesh);
		meshCharts->extractCharts();
		addMeshCharts(meshCharts);
	}

	void computeCharts(const halfedge::Mesh *mesh, const CharterOptions &options)
	{
		MeshCharts *meshCharts = new MeshCharts(mesh);
		meshCharts->computeCharts(options);
		addMeshCharts(meshCharts);
	}

	void parameterizeCharts(ProgressCallback progressCallback, void *progressCallbackUserData)
	{
		int progress = 0;
		if (progressCallback)
			progressCallback(ProgressCategory::ParametizingCharts, 0, progressCallbackUserData);
		for (uint32_t i = 0; i < m_meshChartsArray.size(); i++) {
			m_meshChartsArray[i]->parameterizeCharts();
			if (progressCallback)
			{
				const int newProgress = int((i + 1) / (float)m_meshChartsArray.size() * 100.0f);
				if (newProgress != progress)
				{
					progress = newProgress;
					progressCallback(ProgressCategory::ParametizingCharts, progress, progressCallbackUserData);
				}
			}
		}
		if (progressCallback && progress != 100)
			progressCallback(ProgressCategory::ParametizingCharts, 100, progressCallbackUserData);
	}

	void saveOriginalChartUvs()
	{
		m_originalChartUvs.resize(chartCount());
		for (uint32_t i = 0; i < chartCount(); i++) {
			const halfedge::Mesh *mesh = chartAt(i)->chartMesh();
			m_originalChartUvs[i].resize(mesh->vertexCount());
			for (uint32_t j = 0; j < mesh->vertexCount(); j++)
				m_originalChartUvs[i][j] = mesh->vertexAt(j)->tex;
		}
	}

	void restoreOriginalChartUvs()
	{
		for (uint32_t i = 0; i < chartCount(); i++) {
			halfedge::Mesh *mesh = chartAt(i)->chartMesh();
			for (uint32_t j = 0; j < mesh->vertexCount(); j++)
				mesh->vertexAt(j)->tex = m_originalChartUvs[i][j];
		}
	}

private:
	Array<MeshCharts *> m_meshChartsArray;
	Array<Array<Vector2> > m_originalChartUvs;
};

struct AtlasPacker
{
	AtlasPacker(Atlas *atlas) : m_atlas(atlas), m_width(0), m_height(0)
	{
		m_atlas->restoreOriginalChartUvs();
	}

	uint32_t getWidth() const { return m_width; }
	uint32_t getHeight() const { return m_height; }

	// Pack charts in the smallest possible rectangle.
	void packCharts(const PackerOptions &options)
	{
		const uint32_t chartCount = m_atlas->chartCount();
		XA_PRINT(PrintFlags::PackingCharts, "   %u charts\n", chartCount);
		if (chartCount == 0) return;
		float texelsPerUnit = 1;
		if (options.method == PackMethod::TexelArea)
			texelsPerUnit = options.texelArea;
		for (int iteration = 0;; iteration++) {
			XA_PRINT(PrintFlags::PackingCharts, "   Iteration %d\n", iteration);
			m_rand = MTRand();
			Array<float> chartOrderArray;
			chartOrderArray.resize(chartCount);
			Array<Vector2> chartExtents;
			chartExtents.resize(chartCount);
			float meshArea = 0;
			for (uint32_t c = 0; c < chartCount; c++) {
				Chart *chart = m_atlas->chartAt(c);
				if (chart->isVertexMapped() || !chart->isDisk()) {
					chartOrderArray[c] = 0;
					// Skip non-disks.
					continue;
				}
				Vector2 extents(0.0f);
				// Compute surface area to sort charts.
				float chartArea = chart->computeSurfaceArea();
				meshArea += chartArea;
				//chartOrderArray[c] = chartArea;
				// Compute chart scale
				float parametricArea = fabsf(chart->computeParametricArea());    // @@ There doesn't seem to be anything preventing parametric area to be negative.
				if (parametricArea < XA_EPSILON) {
					// When the parametric area is too small we use a rough approximation to prevent divisions by very small numbers.
					Vector2 bounds = chart->computeParametricBounds();
					parametricArea = bounds.x * bounds.y;
				}
				float scale = (chartArea / parametricArea) * texelsPerUnit;
				if (parametricArea == 0) { // < XA_EPSILON)
					scale = 0;
				}
				XA_ASSERT(std::isfinite(scale));
				// Compute bounding box of chart.
				Vector2 majorAxis, minorAxis, origin, end;
				computeBoundingBox(chart, &majorAxis, &minorAxis, &origin, &end);
				XA_ASSERT(isFinite(majorAxis) && isFinite(minorAxis) && isFinite(origin));
				// Sort charts by perimeter. @@ This is sometimes producing somewhat unexpected results. Is this right?
				//chartOrderArray[c] = ((end.x - origin.x) + (end.y - origin.y)) * scale;
				// Translate, rotate and scale vertices. Compute extents.
				halfedge::Mesh *mesh = chart->chartMesh();
				const uint32_t vertexCount = mesh->vertexCount();
				for (uint32_t i = 0; i < vertexCount; i++) {
					halfedge::Vertex *vertex = mesh->vertexAt(i);
					//Vector2 t = vertex->tex - origin;
					Vector2 tmp;
					tmp.x = dot(vertex->tex, majorAxis);
					tmp.y = dot(vertex->tex, minorAxis);
					tmp -= origin;
					tmp *= scale;
					if (tmp.x < 0 || tmp.y < 0) {
						XA_PRINT(PrintFlags::PackingCharts, "tmp: %f %f\n", tmp.x, tmp.y);
						XA_PRINT(PrintFlags::PackingCharts, "scale: %f\n", scale);
						XA_PRINT(PrintFlags::PackingCharts, "origin: %f %f\n", origin.x, origin.y);
						XA_PRINT(PrintFlags::PackingCharts, "majorAxis: %f %f\n", majorAxis.x, majorAxis.y);
						XA_PRINT(PrintFlags::PackingCharts, "minorAxis: %f %f\n", minorAxis.x, minorAxis.y);
						XA_DEBUG_ASSERT(false);
					}
					//XA_ASSERT(tmp.x >= 0 && tmp.y >= 0);
					vertex->tex = tmp;
					XA_ASSERT(std::isfinite(vertex->tex.x) && std::isfinite(vertex->tex.y));
					extents = max(extents, tmp);
				}
				XA_DEBUG_ASSERT(extents.x >= 0 && extents.y >= 0);
				// Limit chart size.
				if (extents.x > 1024 || extents.y > 1024) {
					float limit = std::max(extents.x, extents.y);
					scale = 1024 / (limit + 1);
					for (uint32_t i = 0; i < vertexCount; i++) {
						halfedge::Vertex *vertex = mesh->vertexAt(i);
						vertex->tex *= scale;
					}
					extents *= scale;
					XA_DEBUG_ASSERT(extents.x <= 1024 && extents.y <= 1024);
				}
				// Scale the charts to use the entire texel area available. So, if the width is 0.1 we could scale it to 1 without increasing the lightmap usage and making a better
				// use of it. In many cases this also improves the look of the seams, since vertices on the chart boundaries have more chances of being aligned with the texel centers.
				float scale_x = 1.0f;
				float scale_y = 1.0f;
				float divide_x = 1.0f;
				float divide_y = 1.0f;
				if (extents.x > 0) {
					int cw = ftoi_ceil(extents.x);
					if (options.blockAlign && chart->blockAligned) {
						// Align all chart extents to 4x4 blocks, but taking padding into account.
						if (options.conservative) {
							cw = align(cw + 2, 4) - 2;
						} else {
							cw = align(cw + 1, 4) - 1;
						}
					}
					scale_x = (float(cw) - XA_EPSILON);
					divide_x = extents.x;
					extents.x = float(cw);
				}
				if (extents.y > 0) {
					int ch = ftoi_ceil(extents.y);
					if (options.blockAlign && chart->blockAligned) {
						// Align all chart extents to 4x4 blocks, but taking padding into account.
						if (options.conservative) {
							ch = align(ch + 2, 4) - 2;
						} else {
							ch = align(ch + 1, 4) - 1;
						}
					}
					scale_y = (float(ch) - XA_EPSILON);
					divide_y = extents.y;
					extents.y = float(ch);
				}
				for (uint32_t v = 0; v < vertexCount; v++) {
					halfedge::Vertex *vertex = mesh->vertexAt(v);
					vertex->tex.x /= divide_x;
					vertex->tex.y /= divide_y;
					vertex->tex.x *= scale_x;
					vertex->tex.y *= scale_y;
					XA_ASSERT(std::isfinite(vertex->tex.x) && std::isfinite(vertex->tex.y));
				}
				chartExtents[c] = extents;
				// Sort charts by perimeter.
				chartOrderArray[c] = extents.x + extents.y;
			}
			// @@ We can try to improve compression of small charts by sorting them by proximity like we do with vertex samples.
			// @@ How to do that? One idea: compute chart centroid, insert into grid, compute morton index of the cell, sort based on morton index.
			// @@ We would sort by morton index, first, then quantize the chart sizes, so that all small charts have the same size, and sort by size preserving the morton order.
			//XA_PRINT("Sorting charts.\n");
			// Sort charts by area.
			m_radix = RadixSort();
			m_radix.sort(chartOrderArray);
			const uint32_t *ranks = m_radix.ranks();
			// First iteration - guess texelsPerUnit.
			if (options.method != PackMethod::TexelArea && iteration == 0) {
				// Estimate size of the map based on the mesh surface area and given texel scale.
				const float texelCount = std::max(1.0f, meshArea * square(texelsPerUnit) / 0.75f); // Assume 75% utilization.
				texelsPerUnit = sqrt((options.resolution * options.resolution) / texelCount);
				XA_PRINT(PrintFlags::PackingCharts, "      Estimating texelsPerUnit as %g\n", texelsPerUnit);
				m_atlas->restoreOriginalChartUvs();
				continue;
			}
			// Init bit map.
			m_bitmap.clearAll();
			m_bitmap.resize(options.resolution, options.resolution, false);
			int w = 0;
			int h = 0;
			// Add sorted charts to bitmap.
			XA_PRINT(PrintFlags::PackingCharts, "      Rasterizing charts\n");
			for (uint32_t i = 0; i < chartCount; i++) {
				uint32_t c = ranks[chartCount - i - 1]; // largest chart first
				Chart *chart = m_atlas->chartAt(c);
				if (chart->isVertexMapped() || !chart->isDisk()) continue;
				//float scale_x = 1;
				//float scale_y = 1;
				BitMap chart_bitmap;
				// @@ Add special cases for dot and line charts. @@ Lightmap rasterizer also needs to handle these special cases.
				// @@ We could also have a special case for chart quads. If the quad surface <= 4 texels, align vertices with texel centers and do not add padding. May be very useful for foliage.
				// @@ In general we could reduce the padding of all charts by one texel by using a rasterizer that takes into account the 2-texel footprint of the tent bilinear filter. For example,
				// if we have a chart that is less than 1 texel wide currently we add one texel to the left and one texel to the right creating a 3-texel-wide bitmap. However, if we know that the
				// chart is only 1 texel wide we could align it so that it only touches the footprint of two texels:
				//      |   |      <- Touches texels 0, 1 and 2.
				//    |   |        <- Only touches texels 0 and 1.
				// \   \ / \ /   /
				//  \   X   X   /
				//   \ / \ / \ /
				//    V   V   V
				//    0   1   2
				if (options.conservative) {
					// Init all bits to 0.
					chart_bitmap.resize(ftoi_ceil(chartExtents[c].x) + 1 + options.padding, ftoi_ceil(chartExtents[c].y) + 1 + options.padding, /*initValue=*/false);  // + 2 to add padding on both sides.
					// Rasterize chart and dilate.
					drawChartBitmapDilate(chart, &chart_bitmap, options.padding);
				} else {
					// Init all bits to 0.
					chart_bitmap.resize(ftoi_ceil(chartExtents[c].x) + 1, ftoi_ceil(chartExtents[c].y) + 1, /*initValue=*/false);  // Add half a texels on each side.
					// Rasterize chart and dilate.
					drawChartBitmap(chart, &chart_bitmap, Vector2(1), Vector2(0.5));
				}
				int best_x = 0, best_y = 0;
				int best_cw = 0, best_ch = 0;   // Includes padding now.
				int best_r = 0;
				findChartLocation(options.quality, &chart_bitmap, chartExtents[c], w, h, &best_x, &best_y, &best_cw, &best_ch, &best_r, chart->blockAligned);
				/*if (w < best_x + best_cw || h < best_y + best_ch)
				{
					XA_PRINT("Resize extents to (%d, %d).\n", best_x + best_cw, best_y + best_ch);
				}*/
				// Update parametric extents.
				w = std::max(w, best_x + best_cw);
				h = std::max(h, best_y + best_ch);
				w = align(w, 4);
				h = align(h, 4);
				// Resize bitmap if necessary.
				if (uint32_t(w) > m_bitmap.width() || uint32_t(h) > m_bitmap.height()) {
					//XA_PRINT("Resize bitmap (%d, %d).\n", nextPowerOfTwo(w), nextPowerOfTwo(h));
					m_bitmap.resize(nextPowerOfTwo(uint32_t(w)), nextPowerOfTwo(uint32_t(h)), false);
				}
				//XA_PRINT("Add chart at (%d, %d).\n", best_x, best_y);
				addChart(&chart_bitmap, w, h, best_x, best_y, best_r);
				//float best_angle = 2 * M_PI * best_r;
				// Translate and rotate chart texture coordinates.
				halfedge::Mesh *mesh = chart->chartMesh();
				const uint32_t vertexCount = mesh->vertexCount();
				for (uint32_t v = 0; v < vertexCount; v++) {
					halfedge::Vertex *vertex = mesh->vertexAt(v);
					Vector2 t = vertex->tex;
					if (best_r) std::swap(t.x, t.y);
					//vertex->tex.x = best_x + t.x * cosf(best_angle) - t.y * sinf(best_angle);
					//vertex->tex.y = best_y + t.x * sinf(best_angle) + t.y * cosf(best_angle);
					vertex->tex.x = best_x + t.x + 0.5f;
					vertex->tex.y = best_y + t.y + 0.5f;
					XA_ASSERT(vertex->tex.x >= 0 && vertex->tex.y >= 0);
					XA_ASSERT(std::isfinite(vertex->tex.x) && std::isfinite(vertex->tex.y));
				}
			}
			//w -= padding - 1; // Leave one pixel border!
			//h -= padding - 1;
			m_width = std::max(0, w);
			m_height = std::max(0, h);
			XA_ASSERT(isAligned(m_width, 4));
			XA_ASSERT(isAligned(m_height, 4));
			XA_PRINT(PrintFlags::PackingCharts, "      %dx%d resolution\n", m_width, m_height);
			if (options.method == PackMethod::ExactResolution) {
				const uint32_t largestDimension = std::max(m_width, m_height);
				texelsPerUnit *= sqrt((options.resolution * options.resolution) / (float)(largestDimension * largestDimension));
				// Reduce texelsPerUnit by 10% after a few iterations.
				const int firstAdjustmentIteration = 8;
				if (iteration >= firstAdjustmentIteration)
					texelsPerUnit *= 1.0f - (0.1f * (iteration - firstAdjustmentIteration + 1));
				XA_PRINT(PrintFlags::PackingCharts, "      Estimating texelsPerUnit as %g\n", texelsPerUnit);
				if (iteration > 1 && m_width <= options.resolution && m_height <= options.resolution) {
					m_width = m_height = options.resolution;
					return;
				}
				// Give up after too many iterations.
				if (iteration >= 16)
					return;
				m_atlas->restoreOriginalChartUvs();
			} else {
				return;
			}
		}
	}

	float computeAtlasUtilization() const
	{
		const uint32_t w = m_width;
		const uint32_t h = m_height;
		XA_DEBUG_ASSERT(w <= m_bitmap.width());
		XA_DEBUG_ASSERT(h <= m_bitmap.height());
		uint32_t count = 0;
		for (uint32_t y = 0; y < h; y++) {
			for (uint32_t x = 0; x < w; x++) {
				count += m_bitmap.bitAt(x, y);
			}
		}
		return float(count) / (w * h);
	}

private:
	// IC: Brute force is slow, and random may take too much time to converge. We start inserting large charts in a small atlas. Using brute force is lame, because most of the space
	// is occupied at this point. At the end we have many small charts and a large atlas with sparse holes. Finding those holes randomly is slow. A better approach would be to
	// start stacking large charts as if they were tetris pieces. Once charts get small try to place them randomly. It may be interesting to try a intermediate strategy, first try
	// along one axis and then try exhaustively along that axis.
	void findChartLocation(int quality, const BitMap *bitmap, Vector2::Arg extents, int w, int h, int *best_x, int *best_y, int *best_w, int *best_h, int *best_r, bool blockAligned)
	{
		int attempts = 256;
		if (quality == 1) attempts = 4096;
		if (quality == 2) attempts = 2048;
		if (quality == 3) attempts = 1024;
		if (quality == 4) attempts = 512;
		if (quality == 0 || w * h < attempts) {
			findChartLocation_bruteForce(bitmap, extents, w, h, best_x, best_y, best_w, best_h, best_r, blockAligned);
		} else {
			findChartLocation_random(bitmap, extents, w, h, best_x, best_y, best_w, best_h, best_r, attempts, blockAligned);
		}
	}

	void findChartLocation_bruteForce(const BitMap *bitmap, Vector2::Arg /*extents*/, int w, int h, int *best_x, int *best_y, int *best_w, int *best_h, int *best_r, bool blockAligned)
	{
		const int BLOCK_SIZE = 4;
		int best_metric = INT_MAX;
		int step_size = blockAligned ? BLOCK_SIZE : 1;
		// Try two different orientations.
		for (int r = 0; r < 2; r++) {
			int cw = bitmap->width();
			int ch = bitmap->height();
			if (r & 1) std::swap(cw, ch);
			for (int y = 0; y <= h + 1; y += step_size) { // + 1 to extend atlas in case atlas full.
				for (int x = 0; x <= w + 1; x += step_size) { // + 1 not really necessary here.
					// Early out.
					int area = std::max(w, x + cw) * std::max(h, y + ch);
					//int perimeter = max(w, x+cw) + max(h, y+ch);
					int extents = std::max(std::max(w, x + cw), std::max(h, y + ch));
					int metric = extents * extents + area;
					if (metric > best_metric) {
						continue;
					}
					if (metric == best_metric && std::max(x, y) >= std::max(*best_x, *best_y)) {
						// If metric is the same, pick the one closest to the origin.
						continue;
					}
					if (canAddChart(bitmap, w, h, x, y, r)) {
						best_metric = metric;
						*best_x = x;
						*best_y = y;
						*best_w = cw;
						*best_h = ch;
						*best_r = r;
						if (area == w * h) {
							// Chart is completely inside, do not look at any other location.
							goto done;
						}
					}
				}
			}
		}
	done:
		XA_DEBUG_ASSERT (best_metric != INT_MAX);
	}

	void findChartLocation_random(const BitMap *bitmap, Vector2::Arg /*extents*/, int w, int h, int *best_x, int *best_y, int *best_w, int *best_h, int *best_r, int minTrialCount, bool blockAligned)
	{
		const int BLOCK_SIZE = 4;
		int best_metric = INT_MAX;
		for (int i = 0; i < minTrialCount || best_metric == INT_MAX; i++) {
			int r = m_rand.getRange(1);
			int x = m_rand.getRange(w + 1); // + 1 to extend atlas in case atlas full. We may want to use a higher number to increase probability of extending atlas.
			int y = m_rand.getRange(h + 1); // + 1 to extend atlas in case atlas full.
			if (blockAligned) {
				x = align(x, BLOCK_SIZE);
				y = align(y, BLOCK_SIZE);
			}
			int cw = bitmap->width();
			int ch = bitmap->height();
			if (r & 1) std::swap(cw, ch);
			// Early out.
			int area = std::max(w, x + cw) * std::max(h, y + ch);
			//int perimeter = max(w, x+cw) + max(h, y+ch);
			int extents = std::max(std::max(w, x + cw), std::max(h, y + ch));
			int metric = extents * extents + area;
			if (metric > best_metric) {
				continue;
			}
			if (metric == best_metric && std::min(x, y) > std::min(*best_x, *best_y)) {
				// If metric is the same, pick the one closest to the origin.
				continue;
			}
			if (canAddChart(bitmap, w, h, x, y, r)) {
				best_metric = metric;
				*best_x = x;
				*best_y = y;
				*best_w = cw;
				*best_h = ch;
				*best_r = r;
				if (area == w * h) {
					// Chart is completely inside, do not look at any other location.
					break;
				}
			}
		}
	}

	void drawChartBitmapDilate(const Chart *chart, BitMap *bitmap, int padding)
	{
		const int w = bitmap->width();
		const int h = bitmap->height();
		const Vector2 extents = Vector2(float(w), float(h));
		// Rasterize chart faces, check that all bits are not set.
		const uint32_t faceCount = chart->faceCount();
		for (uint32_t f = 0; f < faceCount; f++) {
			const halfedge::Face *face = chart->chartMesh()->faceAt(f);
			Vector2 vertices[3];
			uint32_t edgeCount = 0;
			for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
				if (edgeCount < 3)
					vertices[edgeCount] = it.vertex()->tex + Vector2(0.5) + Vector2(float(padding), float(padding));
				edgeCount++;
			}
			XA_DEBUG_ASSERT(edgeCount == 3);
			raster::drawTriangle(raster::Mode_Antialiased, extents, true, vertices, AtlasPacker::setBitsCallback, bitmap);
		}
		// Expand chart by padding pixels. (dilation)
		BitMap tmp(w, h);
		for (int i = 0; i < padding; i++) {
			tmp.clearAll();
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					bool b = bitmap->bitAt(x, y);
					if (!b) {
						if (x > 0) {
							b |= bitmap->bitAt(x - 1, y);
							if (y > 0) b |= bitmap->bitAt(x - 1, y - 1);
							if (y < h - 1) b |= bitmap->bitAt(x - 1, y + 1);
						}
						if (y > 0) b |= bitmap->bitAt(x, y - 1);
						if (y < h - 1) b |= bitmap->bitAt(x, y + 1);
						if (x < w - 1) {
							b |= bitmap->bitAt(x + 1, y);
							if (y > 0) b |= bitmap->bitAt(x + 1, y - 1);
							if (y < h - 1) b |= bitmap->bitAt(x + 1, y + 1);
						}
					}
					if (b) tmp.setBitAt(x, y);
				}
			}
			std::swap(tmp, *bitmap);
		}
	}

	void drawChartBitmap(const Chart *chart, BitMap *bitmap, const Vector2 &scale, const Vector2 &offset)
	{
		const int w = bitmap->width();
		const int h = bitmap->height();
		const Vector2 extents = Vector2(float(w), float(h));
		static const Vector2 pad[4] = {
			Vector2(-0.5, -0.5),
			Vector2(0.5, -0.5),
			Vector2(-0.5, 0.5),
			Vector2(0.5, 0.5)
		};
		// Rasterize 4 times to add proper padding.
		for (int i = 0; i < 4; i++) {
			// Rasterize chart faces, check that all bits are not set.
			const uint32_t faceCount = chart->chartMesh()->faceCount();
			for (uint32_t f = 0; f < faceCount; f++) {
				const halfedge::Face *face = chart->chartMesh()->faceAt(f);
				Vector2 vertices[3];
				uint32_t edgeCount = 0;
				for (halfedge::Face::ConstEdgeIterator it(face->edges()); !it.isDone(); it.advance()) {
					if (edgeCount < 3) {
						vertices[edgeCount] = it.vertex()->tex * scale + offset + pad[i];
						XA_ASSERT(ftoi_ceil(vertices[edgeCount].x) >= 0);
						XA_ASSERT(ftoi_ceil(vertices[edgeCount].y) >= 0);
						XA_ASSERT(ftoi_ceil(vertices[edgeCount].x) <= w);
						XA_ASSERT(ftoi_ceil(vertices[edgeCount].y) <= h);
					}
					edgeCount++;
				}
				XA_ASSERT(edgeCount == 3);
				raster::drawTriangle(raster::Mode_Antialiased, extents, /*enableScissors=*/true, vertices, AtlasPacker::setBitsCallback, bitmap);
			}
		}
		// Expand chart by padding pixels. (dilation)
		BitMap tmp(w, h);
		tmp.clearAll();
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				bool b = bitmap->bitAt(x, y);
				if (!b) {
					if (x > 0) {
						b |= bitmap->bitAt(x - 1, y);
						if (y > 0) b |= bitmap->bitAt(x - 1, y - 1);
						if (y < h - 1) b |= bitmap->bitAt(x - 1, y + 1);
					}
					if (y > 0) b |= bitmap->bitAt(x, y - 1);
					if (y < h - 1) b |= bitmap->bitAt(x, y + 1);
					if (x < w - 1) {
						b |= bitmap->bitAt(x + 1, y);
						if (y > 0) b |= bitmap->bitAt(x + 1, y - 1);
						if (y < h - 1) b |= bitmap->bitAt(x + 1, y + 1);
					}
				}
				if (b) tmp.setBitAt(x, y);
			}
		}
		std::swap(tmp, *bitmap);
	}

	bool canAddChart(const BitMap *bitmap, int atlas_w, int atlas_h, int offset_x, int offset_y, int r)
	{
		XA_DEBUG_ASSERT(r == 0 || r == 1);
		// Check whether the two bitmaps overlap.
		const int w = bitmap->width();
		const int h = bitmap->height();
		if (r == 0) {
			for (int y = 0; y < h; y++) {
				int yy = y + offset_y;
				if (yy >= 0) {
					for (int x = 0; x < w; x++) {
						int xx = x + offset_x;
						if (xx >= 0) {
							if (bitmap->bitAt(x, y)) {
								if (xx < atlas_w && yy < atlas_h) {
									if (m_bitmap.bitAt(xx, yy)) return false;
								}
							}
						}
					}
				}
			}
		} else if (r == 1) {
			for (int y = 0; y < h; y++) {
				int xx = y + offset_x;
				if (xx >= 0) {
					for (int x = 0; x < w; x++) {
						int yy = x + offset_y;
						if (yy >= 0) {
							if (bitmap->bitAt(x, y)) {
								if (xx < atlas_w && yy < atlas_h) {
									if (m_bitmap.bitAt(xx, yy)) return false;
								}
							}
						}
					}
				}
			}
		}
		return true;
	}

	void addChart(const BitMap *bitmap, int atlas_w, int atlas_h, int offset_x, int offset_y, int r)
	{
		XA_DEBUG_ASSERT(r == 0 || r == 1);
		// Check whether the two bitmaps overlap.
		const int w = bitmap->width();
		const int h = bitmap->height();
		if (r == 0) {
			for (int y = 0; y < h; y++) {
				int yy = y + offset_y;
				if (yy >= 0) {
					for (int x = 0; x < w; x++) {
						int xx = x + offset_x;
						if (xx >= 0) {
							if (bitmap->bitAt(x, y)) {
								if (xx < atlas_w && yy < atlas_h) {
									XA_DEBUG_ASSERT(m_bitmap.bitAt(xx, yy) == false);
									m_bitmap.setBitAt(xx, yy);
								}
							}
						}
					}
				}
			}
		} else if (r == 1) {
			for (int y = 0; y < h; y++) {
				int xx = y + offset_x;
				if (xx >= 0) {
					for (int x = 0; x < w; x++) {
						int yy = x + offset_y;
						if (yy >= 0) {
							if (bitmap->bitAt(x, y)) {
								if (xx < atlas_w && yy < atlas_h) {
									XA_DEBUG_ASSERT(m_bitmap.bitAt(xx, yy) == false);
									m_bitmap.setBitAt(xx, yy);
								}
							}
						}
					}
				}
			}
		}
	}

	static bool setBitsCallback(void *param, int x, int y, Vector3::Arg, Vector3::Arg, Vector3::Arg, float area)
	{
		BitMap *bitmap = (BitMap * )param;
		if (area > 0.0) {
			bitmap->setBitAt(x, y);
		}
		return true;
	}

	// Compute the convex hull using Graham Scan.
	static void convexHull(const Array<Vector2> &input, Array<Vector2> &output, float epsilon)
	{
		const uint32_t inputCount = input.size();
		Array<float> coords;
		coords.resize(inputCount);
		for (uint32_t i = 0; i < inputCount; i++) {
			coords[i] = input[i].x;
		}
		RadixSort radix;
		radix.sort(coords);
		const uint32_t *ranks = radix.ranks();
		Array<Vector2> top;
		top.reserve(inputCount);
		Array<Vector2> bottom;
		bottom.reserve(inputCount);
		Vector2 P = input[ranks[0]];
		Vector2 Q = input[ranks[inputCount - 1]];
		float topy = std::max(P.y, Q.y);
		float boty = std::min(P.y, Q.y);
		for (uint32_t i = 0; i < inputCount; i++) {
			Vector2 p = input[ranks[i]];
			if (p.y >= boty) top.push_back(p);
		}
		for (uint32_t i = 0; i < inputCount; i++) {
			Vector2 p = input[ranks[inputCount - 1 - i]];
			if (p.y <= topy) bottom.push_back(p);
		}
		// Filter top list.
		output.clear();
		output.push_back(top[0]);
		output.push_back(top[1]);
		for (uint32_t i = 2; i < top.size(); ) {
			Vector2 a = output[output.size() - 2];
			Vector2 b = output[output.size() - 1];
			Vector2 c = top[i];
			float area = triangleArea(a, b, c);
			if (area >= -epsilon) {
				output.pop_back();
			}
			if (area < -epsilon || output.size() == 1) {
				output.push_back(c);
				i++;
			}
		}
		uint32_t top_count = output.size();
		output.push_back(bottom[1]);
		// Filter bottom list.
		for (uint32_t i = 2; i < bottom.size(); ) {
			Vector2 a = output[output.size() - 2];
			Vector2 b = output[output.size() - 1];
			Vector2 c = bottom[i];
			float area = triangleArea(a, b, c);
			if (area >= -epsilon) {
				output.pop_back();
			}
			if (area < -epsilon || output.size() == top_count) {
				output.push_back(c);
				i++;
			}
		}
		// Remove duplicate element.
		XA_DEBUG_ASSERT(output.front() == output.back());
		output.pop_back();
	}

	// This should compute convex hull and use rotating calipers to find the best box. Currently it uses a brute force method.
	static void computeBoundingBox(const Chart *chart, Vector2 *majorAxis, Vector2 *minorAxis, Vector2 *minCorner, Vector2 *maxCorner)
	{
		// Compute list of boundary points.
		Array<Vector2> points;
		points.reserve(16);
		const halfedge::Mesh *mesh = chart->chartMesh();
		const uint32_t vertexCount = mesh->vertexCount();
		for (uint32_t i = 0; i < vertexCount; i++) {
			const halfedge::Vertex *vertex = mesh->vertexAt(i);
			if (vertex->isBoundary()) {
				points.push_back(vertex->tex);
			}
		}
		XA_DEBUG_ASSERT(points.size() > 0);
		Array<Vector2> hull;
		convexHull(points, hull, 0.00001f);
		// @@ Ideally I should use rotating calipers to find the best box. Using brute force for now.
		float best_area = FLT_MAX;
		Vector2 best_min(0);
		Vector2 best_max(0);
		Vector2 best_axis(0);
		const uint32_t hullCount = hull.size();
		for (uint32_t i = 0, j = hullCount - 1; i < hullCount; j = i, i++) {
			if (equal(hull[i], hull[j])) {
				continue;
			}
			Vector2 axis = normalize(hull[i] - hull[j], 0.0f);
			XA_DEBUG_ASSERT(isFinite(axis));
			// Compute bounding box.
			Vector2 box_min(FLT_MAX, FLT_MAX);
			Vector2 box_max(-FLT_MAX, -FLT_MAX);
			for (uint32_t v = 0; v < hullCount; v++) {
				Vector2 point = hull[v];
				float x = dot(axis, point);
				if (x < box_min.x) box_min.x = x;
				if (x > box_max.x) box_max.x = x;
				float y = dot(Vector2(-axis.y, axis.x), point);
				if (y < box_min.y) box_min.y = y;
				if (y > box_max.y) box_max.y = y;
			}
			// Compute box area.
			float area = (box_max.x - box_min.x) * (box_max.y - box_min.y);
			if (area < best_area) {
				best_area = area;
				best_min = box_min;
				best_max = box_max;
				best_axis = axis;
			}
		}
		// Consider all points, not only boundary points, in case the input chart is malformed.
		for (uint32_t i = 0; i < vertexCount; i++) {
			const halfedge::Vertex *vertex = mesh->vertexAt(i);
			Vector2 point = vertex->tex;
			float x = dot(best_axis, point);
			if (x < best_min.x) best_min.x = x;
			if (x > best_max.x) best_max.x = x;
			float y = dot(Vector2(-best_axis.y, best_axis.x), point);
			if (y < best_min.y) best_min.y = y;
			if (y > best_max.y) best_max.y = y;
		}
		*majorAxis = best_axis;
		*minorAxis = Vector2(-best_axis.y, best_axis.x);
		*minCorner = best_min;
		*maxCorner = best_max;
	}

	Atlas *m_atlas;
	BitMap m_bitmap;
	RadixSort m_radix;
	uint32_t m_width;
	uint32_t m_height;
	MTRand m_rand;
};

} // namespace param
} // namespace internal

struct Atlas
{
	Atlas() : chartCount(0), width(0), height(0), outputMeshes(NULL) {}
	internal::param::Atlas atlas;
	uint32_t chartCount; // Excluding vertex mapped charts.
	internal::Array<internal::halfedge::Mesh *> heMeshes;
	uint32_t width;
	uint32_t height;
	OutputMesh **outputMeshes;
};

void SetPrint(int flags, PrintFunc print)
{
	internal::s_printFlags = flags;
	internal::s_print = print ? print : printf;
}

Atlas *Create()
{
	Atlas *atlas = new Atlas();
	return atlas;
}

static void DestroyOutputMeshes(Atlas *atlas)
{
	if (!atlas->outputMeshes)
		return;
	for (int i = 0; i < (int)atlas->heMeshes.size(); i++) {
		OutputMesh *outputMesh = atlas->outputMeshes[i];
		for (uint32_t j = 0; j < outputMesh->chartCount; j++)
			delete [] outputMesh->chartArray[j].indexArray;
		delete [] outputMesh->chartArray;
		delete [] outputMesh->vertexArray;
		delete [] outputMesh->indexArray;
		delete outputMesh;
	}
	delete [] atlas->outputMeshes;
	atlas->outputMeshes = NULL;
}

void Destroy(Atlas *atlas)
{
	XA_DEBUG_ASSERT(atlas);
	for (int i = 0; i < (int)atlas->heMeshes.size(); i++)
		delete atlas->heMeshes[i];
	DestroyOutputMeshes(atlas);
	delete atlas;
}

static internal::Vector3 DecodePosition(const InputMesh &mesh, uint32_t index)
{
	XA_DEBUG_ASSERT(mesh.vertexPositionData);
	XA_DEBUG_ASSERT(mesh.vertexPositionStride > 0);
	return *((const internal::Vector3 *)&((const uint8_t *)mesh.vertexPositionData)[mesh.vertexPositionStride * index]);
}

static internal::Vector3 DecodeNormal(const InputMesh &mesh, uint32_t index)
{
	XA_DEBUG_ASSERT(mesh.vertexNormalData);
	XA_DEBUG_ASSERT(mesh.vertexNormalStride > 0);
	return *((const internal::Vector3 *)&((const uint8_t *)mesh.vertexNormalData)[mesh.vertexNormalStride * index]);
}

static internal::Vector2 DecodeUv(const InputMesh &mesh, uint32_t index)
{
	XA_DEBUG_ASSERT(mesh.vertexUvData);
	XA_DEBUG_ASSERT(mesh.vertexUvStride > 0);
	return *((const internal::Vector2 *)&((const uint8_t *)mesh.vertexUvData)[mesh.vertexUvStride * index]);
}

static uint32_t DecodeIndex(IndexFormat::Enum format, const void *indexData, uint32_t i)
{
	XA_DEBUG_ASSERT(indexData);
	if (format == IndexFormat::UInt16)
		return (uint32_t)((const uint16_t *)indexData)[i];
	return ((const uint32_t *)indexData)[i];
}

static float EdgeLength(internal::Vector3 pos1, internal::Vector3 pos2)
{
	return internal::length(pos2 - pos1);
}

AddMeshError::Enum AddMesh(Atlas *atlas, const InputMesh &mesh, bool useColocalVertices)
{
	XA_DEBUG_ASSERT(atlas);
	XA_DEBUG_ASSERT(mesh.vertexCount > 0);
	XA_DEBUG_ASSERT(mesh.indexCount > 0);
	XA_PRINT(PrintFlags::MeshCreation, "Adding mesh %d: %u vertices, %u triangles\n", atlas->heMeshes.size(), mesh.vertexCount, mesh.indexCount / 3);
	// Expecting triangle faces.
	if ((mesh.indexCount % 3) != 0)
		return AddMeshError::InvalidIndexCount;
	// Check if any index is out of range.
	for (uint32_t j = 0; j < mesh.indexCount; j++) {
		const uint32_t index = DecodeIndex(mesh.indexFormat, mesh.indexData, j);
		if (index >= mesh.vertexCount)
			return AddMeshError::IndexOutOfRange;
	}
	// Build half edge mesh.
	internal::halfedge::Mesh *heMesh = new internal::halfedge::Mesh(atlas->heMeshes.size());
	internal::Array<uint32_t> canonicalMap;
	canonicalMap.reserve(mesh.vertexCount);
	for (uint32_t i = 0; i < mesh.vertexCount; i++) {
		internal::halfedge::Vertex *vertex = heMesh->addVertex(DecodePosition(mesh, i));
		if (mesh.vertexNormalData)
			vertex->nor = DecodeNormal(mesh, i);
		if (mesh.vertexUvData)
			vertex->tex = DecodeUv(mesh, i);
		// Link colocals. You probably want to do this more efficiently! Sort by one axis or use a hash or grid.
		uint32_t firstColocal = i;
		if (useColocalVertices) {
			for (uint32_t j = 0; j < i; j++) {
				if (vertex->pos != DecodePosition(mesh, j))
					continue;
#if 0
				if (mesh.vertexNormalData && vertex->nor != DecodeNormal(mesh, j))
					continue;
				if (mesh.vertexUvData && vertex->tex != DecodeUv(mesh, j))
					continue;
#endif
				firstColocal = j;
				break;
			}
		}
		canonicalMap.push_back(firstColocal);
	}
	heMesh->linkColocalsWithCanonicalMap(canonicalMap);
	for (uint32_t i = 0; i < mesh.indexCount / 3; i++) {
		uint32_t tri[3];
		for (int j = 0; j < 3; j++)
			tri[j] = DecodeIndex(mesh.indexFormat, mesh.indexData, i * 3 + j);
		uint32_t faceFlags = 0;
		// Check for degenerate or zero length edges.
		for (int j = 0; j < 3; j++) {
			const uint32_t edges[6] = { 0, 1, 1, 2, 2, 0 };
			const uint32_t index1 = tri[edges[j * 2 + 0]];
			const uint32_t index2 = tri[edges[j * 2 + 1]];
			if (index1 == index2) {
				faceFlags |= internal::halfedge::FaceFlags::Ignore;
				XA_PRINT(PrintFlags::MeshWarnings, "Mesh %d degenerate edge: index %d, index %d\n", (int)atlas->heMeshes.size(), index1, index2);
				break;
			}
			const internal::Vector3 pos1 = DecodePosition(mesh, index1);
			const internal::Vector3 pos2 = DecodePosition(mesh, index2);
			if (EdgeLength(pos1, pos2) <= 0.0f) {
				faceFlags |= internal::halfedge::FaceFlags::Ignore;
				XA_PRINT(PrintFlags::MeshWarnings, "Mesh %d zero length edge: index %d position (%g %g %g), index %d position (%g %g %g)\n", (int)atlas->heMeshes.size(), index1, pos1.x, pos1.y, pos1.z, index2, pos2.x, pos2.y, pos2.z);
				break;
			}
		}
		// Check for zero area faces. Don't bother if a degenerate or zero length edge was already detected.
		if (!(faceFlags & internal::halfedge::FaceFlags::Ignore))
		{
			const internal::Vector3 a = DecodePosition(mesh, tri[0]);
			const internal::Vector3 b = DecodePosition(mesh, tri[1]);
			const internal::Vector3 c = DecodePosition(mesh, tri[2]);
			const float area = internal::length(internal::cross(b - a, c - a)) * 0.5f;
			if (area <= 0.0f)
			{
				faceFlags |= internal::halfedge::FaceFlags::Ignore;
				XA_PRINT(PrintFlags::MeshWarnings, "Mesh %d zero area face: %d, indices (%d %d %d)\n", (int)atlas->heMeshes.size(), i, tri[0], tri[1], tri[2]);
			}
		}
		internal::halfedge::Face *face = heMesh->addFace(tri[0], tri[1], tri[2], faceFlags);
		XA_DEBUG_ASSERT(face);
		if (mesh.faceIgnoreData && mesh.faceIgnoreData[i])
			face->flags |= internal::halfedge::FaceFlags::Ignore;
	}
	heMesh->linkBoundary();
	atlas->heMeshes.push_back(heMesh);
	return AddMeshError::Success;
}

void GenerateCharts(Atlas *atlas, CharterOptions charterOptions, ProgressCallback progressCallback, void *progressCallbackUserData)
{
	XA_DEBUG_ASSERT(atlas);
	if (atlas->heMeshes.isEmpty())
		return;
	atlas->chartCount = 0;
	atlas->width = 0;
	atlas->height = 0;
	DestroyOutputMeshes(atlas);
	// Chart meshes.
	XA_PRINT(PrintFlags::ComputingCharts, "Computing charts\n");
	int progress = 0;
	if (progressCallback)
		progressCallback(ProgressCategory::ComputingCharts, 0, progressCallbackUserData);
	for (int i = 0; i < (int)atlas->heMeshes.size(); i++)
	{
		atlas->atlas.computeCharts(atlas->heMeshes[i], charterOptions);
		if (progressCallback)
		{
			const int newProgress = int((i + 1) / (float)atlas->heMeshes.size() * 100.0f);
			if (newProgress != progress)
			{
				progress = newProgress;
				progressCallback(ProgressCategory::ComputingCharts, progress, progressCallbackUserData);
			}
		}
	}
	if (progressCallback && progress != 100)
		progressCallback(ProgressCategory::ComputingCharts, 0, progressCallbackUserData);
	XA_PRINT(PrintFlags::ParametizingCharts, "Parameterizing charts\n");
	atlas->atlas.parameterizeCharts(progressCallback, progressCallbackUserData);
	atlas->atlas.saveOriginalChartUvs();
	// Count charts.
	for (int i = 0; i < (int)atlas->heMeshes.size(); i++) {
		const internal::param::MeshCharts *charts = atlas->atlas.meshAt(i);
		for (uint32_t j = 0; j < charts->chartCount(); j++) {
			if (!charts->chartAt(j)->isVertexMapped())
				atlas->chartCount++;
		}
	}
}

void PackCharts(Atlas *atlas, PackerOptions packerOptions, ProgressCallback progressCallback, void *progressCallbackUserData)
{
	XA_DEBUG_ASSERT(atlas);
	XA_DEBUG_ASSERT(packerOptions.texelArea > 0);
	atlas->width = 0;
	atlas->height = 0;
	DestroyOutputMeshes(atlas);
	if (atlas->chartCount <= 0)
		return;
	XA_PRINT(PrintFlags::PackingCharts, "Packing charts\n");
	internal::param::AtlasPacker packer(&atlas->atlas);
	if (progressCallback)
		progressCallback(ProgressCategory::PackingCharts, 0, progressCallbackUserData);
	packer.packCharts(packerOptions);
	if (progressCallback)
		progressCallback(ProgressCategory::PackingCharts, 100, progressCallbackUserData);
	//float utilization = return packer.computeAtlasUtilization();
	atlas->width = packer.getWidth();
	atlas->height = packer.getHeight();
	XA_PRINT(PrintFlags::BuildingOutputMeshes, "Building output meshes\n");
	int progress = 0;
	if (progressCallback)
		progressCallback(ProgressCategory::BuildingOutputMeshes, 0, progressCallbackUserData);
	atlas->outputMeshes = new OutputMesh*[atlas->heMeshes.size()];
	for (int i = 0; i < (int)atlas->heMeshes.size(); i++) {
		const internal::halfedge::Mesh *heMesh = atlas->heMeshes[i];
		OutputMesh *outputMesh = atlas->outputMeshes[i] = new OutputMesh;
		const internal::param::MeshCharts *charts = atlas->atlas.meshAt(i);
		// Vertices.
		outputMesh->vertexCount = charts->vertexCount();
		outputMesh->vertexArray = new OutputVertex[outputMesh->vertexCount];
		for (uint32_t j = 0; j < charts->chartCount(); j++) {
			const internal::param::Chart *chart = charts->chartAt(j);
			const uint32_t vertexOffset = charts->vertexCountBeforeChartAt(j);
			for (uint32_t v = 0; v < chart->vertexCount(); v++) {
				OutputVertex &output_vertex = outputMesh->vertexArray[vertexOffset + v];
				output_vertex.xref = chart->mapChartVertexToOriginalVertex(v);
				internal::Vector2 uv = chart->chartMesh()->vertexAt(v)->tex;
				output_vertex.uv[0] = std::max(0.0f, uv.x);
				output_vertex.uv[1] = std::max(0.0f, uv.y);
			}
		}
		// Indices.
		outputMesh->indexCount = heMesh->faceCount() * 3;
		outputMesh->indexArray = new uint32_t[outputMesh->indexCount];
		for (uint32_t f = 0; f < heMesh->faceCount(); f++) {
			const uint32_t c = charts->faceChartAt(f);
			const uint32_t fi = charts->faceIndexWithinChartAt(f);
			const uint32_t vertexOffset = charts->vertexCountBeforeChartAt(c);
			const internal::param::Chart *chart = charts->chartAt(c);
			XA_DEBUG_ASSERT(fi < chart->chartMesh()->faceCount());
			XA_DEBUG_ASSERT(chart->faceAt(fi) == f);
			const internal::halfedge::Face *face = chart->chartMesh()->faceAt(fi);
			const internal::halfedge::Edge *edge = face->edge;
			outputMesh->indexArray[3 * f + 0] = vertexOffset + edge->vertex->id;
			outputMesh->indexArray[3 * f + 1] = vertexOffset + edge->next->vertex->id;
			outputMesh->indexArray[3 * f + 2] = vertexOffset + edge->next->next->vertex->id;
		}
		// Charts.
		// Ignore vertex mapped charts.
		outputMesh->chartCount = 0;
		for (uint32_t j = 0; j < charts->chartCount(); j++) {
			const internal::param::Chart *chart = charts->chartAt(j);
			if (!chart->isVertexMapped())
				outputMesh->chartCount++;
		}
		outputMesh->chartArray = new OutputChart[outputMesh->chartCount];
		uint32_t chartIndex = 0;
		for (uint32_t j = 0; j < charts->chartCount(); j++) {
			const internal::param::Chart *chart = charts->chartAt(j);
			if (chart->isVertexMapped())
				continue;
			OutputChart *outputChart = &outputMesh->chartArray[chartIndex];
			const uint32_t vertexOffset = charts->vertexCountBeforeChartAt(j);
			const internal::halfedge::Mesh *mesh = chart->chartMesh();
			outputChart->indexCount = mesh->faceCount() * 3;
			outputChart->indexArray = new uint32_t[outputChart->indexCount];
			for (uint32_t k = 0; k < mesh->faceCount(); k++) {
				const internal::halfedge::Face *face = mesh->faceAt(k);
				const internal::halfedge::Edge *edge = face->edge;
				outputChart->indexArray[3 * k + 0] = vertexOffset + edge->vertex->id;
				outputChart->indexArray[3 * k + 1] = vertexOffset + edge->next->vertex->id;
				outputChart->indexArray[3 * k + 2] = vertexOffset + edge->next->next->vertex->id;
			}
			chartIndex++;
		}
		XA_PRINT(PrintFlags::BuildingOutputMeshes, "   mesh %d: %u vertices, %u triangles, %u charts\n", i, outputMesh->vertexCount, outputMesh->indexCount / 3, outputMesh->chartCount);
		if (progressCallback)
		{
			const int newProgress = int((i + 1) / (float)atlas->heMeshes.size() * 100.0f);
			if (newProgress != progress)
			{
				progress = newProgress;
				progressCallback(ProgressCategory::BuildingOutputMeshes, progress, progressCallbackUserData);
			}
		}
	}
	if (progressCallback && progress != 100)
		progressCallback(ProgressCategory::BuildingOutputMeshes, 0, progressCallbackUserData);
}

uint32_t GetWidth(const Atlas *atlas)
{
	XA_ASSERT(atlas);
	return atlas->width;
}

uint32_t GetHeight(const Atlas *atlas)
{
	XA_ASSERT(atlas);
	return atlas->height;
}

uint32_t GetNumCharts(const Atlas *atlas)
{
	XA_ASSERT(atlas);
	return atlas->chartCount;
}

const OutputMesh * const *GetOutputMeshes(const Atlas *atlas)
{
	XA_ASSERT(atlas);
	return atlas->outputMeshes;
}

const char *StringForEnum(AddMeshError::Enum error)
{
	if (error == AddMeshError::IndexOutOfRange)
		return "Index out of range";
	if (error == AddMeshError::InvalidIndexCount)
		return "Invalid index count";
	return "Success";
}

const char *StringForEnum(ProgressCategory::Enum category)
{
	if (category == ProgressCategory::ComputingCharts)
		return "Computing charts";
	if (category == ProgressCategory::ParametizingCharts)
		return "Parametizing charts";
	if (category == ProgressCategory::PackingCharts)
		return "Packing charts";
	if (category == ProgressCategory::BuildingOutputMeshes)
		return "Building output meshes";
	return "";
}

} // namespace xatlas
