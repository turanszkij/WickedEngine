/**
 * Apple Silicon NEON shim layer for missing legacy intrinsics and signature
 * mismatch definitions expected by some third-party code. This file is
 * force-included (see CMakeLists.txt) only on macOS arm64 builds.
 *
 * Provided shims:
 *  - vtbl2_u8  : overload wrapper (signed table) forwarding to intrinsic /
 *    vector lookup
 *  - vtbl4_u8  : AArch64 vector implementation using vqtbl2q_u8 (32-byte table
 *    lookup)
 *  - vacleq_f32 / vacle_f32 : comparison (<=) wrappers mapping to vcleq_f32 /
 *    vcle_f32
 *  - vld4_f32_ex : mapped to standard vld4_f32 (flags ignored)
 *  - __prefetch : mapped to __builtin_prefetch
 */

#pragma once

#if defined(__APPLE__) && defined(__ARM_NEON)

#include <arm_neon.h>
#include <stdint.h>

// Helper macro to avoid multiple definitions if this header is accidentally
// included twice without force include mechanism
#ifndef WICKED_NEON_SHIMS_INCLUDED
#define WICKED_NEON_SHIMS_INCLUDED 1

// --------------------------------------------------------------------------------------
// Reinterpret helper overloads: DirectXMath (ARM32 era) sometimes uses the
// non 'q' intrinsic names with 128-bit vectors. Provide 128-bit overloads so
// width is preserved (no-op bit reinterpret).
#ifdef __cplusplus
static inline uint32x4_t  vreinterpret_u32_s32(  int32x4_t v) { return ( uint32x4_t )v; }
static inline int32x4_t   vreinterpret_s32_u32( uint32x4_t v) { return (  int32x4_t )v; }
static inline uint8x16_t  vreinterpret_u8_s8(    int8x16_t v) { return ( uint8x16_t )v; }
static inline int8x16_t   vreinterpret_s8_u8(   uint8x16_t v) { return (  int8x16_t )v; }
static inline uint16x8_t  vreinterpret_u16_s16(  int16x8_t v) { return ( uint16x8_t )v; }
static inline int16x8_t   vreinterpret_s16_u16( uint16x8_t v) { return (  int16x8_t )v; }
static inline uint64x2_t  vreinterpret_u64_s64(  int64x2_t v) { return ( uint64x2_t )v; }
static inline int64x2_t   vreinterpret_s64_u64( uint64x2_t v) { return (  int64x2_t )v; }
static inline float32x4_t vreinterpret_f32_u32( uint32x4_t v) { return (float32x4_t )v; }
static inline uint32x4_t  vreinterpret_u32_f32(float32x4_t v) { return ( uint32x4_t )v; }
#endif // __cplusplus
// --------------------------------------------------------------------------------------

// Explicit comparison wrappers used by DirectXMath for <= (naming quirk):
#ifndef WICKED_HAVE_VACLEQ_F32
static inline uint32x4_t vacleq_f32(float32x4_t a, float32x4_t b)
{
	return vcleq_f32(a,b);
}
#endif // WICKED_HAVE_VACLEQ_F32

#ifndef WICKED_HAVE_VACLE_F32
static inline uint32x2_t vacle_f32(float32x2_t a, float32x2_t b)
{
	return vcle_f32(a,b);
}
#endif // WICKED_HAVE_VACLE_F32

// --------------------------------------------------------------------------------------
// vzip_u8 / vzip_u16 signed aggregate wrappers: DirectXMath assigns the result
// of vzip_u8/vzip_u16 into int8x8x2_t immediately. The canonical intrinsics
// return unsigned aggregate types, which causes a conversion error. We provide
// macro wrappers that preserve original intrinsics under wicked_orig_* names
// and redefine vzip_u8/vzip_u16 to produce signed aggregates, matching legacy
// expectations without changing call sites.
#ifndef WICKED_VZIP_SHIMS
#define WICKED_VZIP_SHIMS 1
static inline uint8x8x2_t wicked_orig_vzip_u8(uint8x8_t a, uint8x8_t b)
{
	return vzip_u8(a,b);
}
static inline uint16x4x2_t wicked_orig_vzip_u16(uint16x4_t a, uint16x4_t b)
{
	return vzip_u16(a,b);
}

static inline int8x8x2_t wicked_vzip_u8_signed(uint8x8_t a, uint8x8_t b)
{
	uint8x8x2_t u = wicked_orig_vzip_u8(a,b);
	int8x8x2_t r;

	r.val[0] = vreinterpret_s8_u8(u.val[0]);
	r.val[1] = vreinterpret_s8_u8(u.val[1]);

	return r;
}

static inline int8x8x2_t wicked_vzip_u16_signed(int8x8_t a, int8x8_t b)
{
	uint16x4_t ua = vreinterpret_u16_s8(a);
	uint16x4_t ub = vreinterpret_u16_s8(b);
	uint16x4x2_t u = wicked_orig_vzip_u16(ua, ub);
	int8x8x2_t r;

	r.val[0] = vreinterpret_s8_u16(u.val[0]);
	r.val[1] = vreinterpret_s8_u16(u.val[1]);

	return r;
}

// Redefine the names expected by DirectXMath to the signed-returning versions.
#undef vzip_u8
#undef vzip_u16
#define vzip_u8(a,b)  wicked_vzip_u8_signed((a),(b))
#define vzip_u16(a,b) wicked_vzip_u16_signed((a),(b))
#endif // WICKED_VZIP_SHIMS

/**
 * vtbl2_u8 shim (signed aggregate overload)
 *
 * Purpose:
 *   Adapt legacy DirectXMath usage that builds an int8x8x2_t table aggregate
 *   and passes it (sometimes with a byte index already in uint8x8_t form) to
 *   vtbl2_u8. The AArch64 intrinsic expects a uint8x8x2_t. This overload
 *   reinterprets to the unsigned aggregate and forwards without redefining
 *   the intrinsic symbol.
 *
 * Semantics:
 *   Concatenate: T = table.val[0] || table.val[1] (16 bytes)
 *   For each lane i of index (0..7):
 *       result[i] = (index[i] < 16) ? T[index[i]] : 0
 *   Out-of-range lookups (>=16) are zeroed by the hardware intrinsic.
 *
 * Parameters:
 *   table (int8x8x2_t): Signed 2×8-byte table aggregate to adapt.
 *   index (uint8x8_t): 8 unsigned byte indices selecting from the 16-byte table.
 *
 * Return:
 *   uint8x8_t – 8 lookup result bytes.
 */
#ifdef __cplusplus
static inline uint8x8_t vtbl2_u8(const int8x8x2_t table, const uint8x8_t index)
{
	uint8x8x2_t t;
	t.val[0] = vreinterpret_u8_s8(table.val[0]);
	t.val[1] = vreinterpret_u8_s8(table.val[1]);

	return vtbl2_u8(t, index); // Dispatches to unsigned intrinsic version
}
#endif // __cplusplus

/**
 * vtbl4_u8 shim (AArch64 implementation using vqtbl2q_u8)
 *
 * Purpose:
 *   Emulate legacy ARMv7 NEON vtbl4_u8 (4×8-byte tables -> 32-byte lookup) for
 *   DirectXMath code building an int8x8x4_t aggregate, using a single Armv8
 *   table instruction (vqtbl2q_u8) for efficiency.
 *
 * Semantics:
 *   Concatenate: T = table.val[0] || table.val[1] || table.val[2] || table.val[3]
 *   For each lane i of index (0..7):
 *       result[i] = (index[i] < 32) ? T[index[i]] : 0
 *   Hardware zeroes lanes with indices >= 32.
 *
 * Parameters:
 *   table (int8x8x4_t): Signed 4×8-byte table aggregate (reinterpreted to unsigned).
 *   index (uint8x8_t): 8 unsigned byte indices selecting from the 32-byte table.
 *
 * Return:
 *   uint8x8_t – 8 lookup result bytes.
 */
static inline uint8x8_t vtbl4_u8(const int8x8x4_t table, const uint8x8_t index)
{
	// Availability note: relies on vqtbl2q_u8 which should be present on modern
	// AppleClang AArch64 toolchains. If a build error reports the intrinsic is
	// missing, introduce a feature test and provide a dual vqtbl1q_u8 fallback
	// (two 16-byte lookups, blend + explicit zero for idx>=32).
	// Combine into two 16-byte vectors, then build a 32-byte table set
	uint8x16_t t0 = vcombine_u8(vreinterpret_u8_s8(
		table.val[0]),
		vreinterpret_u8_s8(table.val[1])
	);
	uint8x16_t t1 = vcombine_u8(vreinterpret_u8_s8(
		table.val[2]),
		vreinterpret_u8_s8(table.val[3])
	);
	uint8x16x2_t tables = { t0, t1 };
	// Form 16-lane index (upper half zeroed)
	uint8x16_t idx16 = vcombine_u8(index, vdup_n_u8(0));
	// Single 32-byte table lookup, lanes with idx>=32 automatically zero
	uint8x16_t r16 = vqtbl2q_u8(tables, idx16);

	return vget_low_u8(r16);
}

/**
 * vld4_f32_ex replacement: load 4-interleaved float32x2 vectors; ignore flags
 * parameter.
 */
#ifndef WICKED_HAVE_VLD4_F32_EX
static inline float32x2x4_t vld4_f32_ex(const float* ptr, int /*flags*/)
{
	return vld4_f32(ptr);
}
#endif

/**
 * Provide __prefetch mapping if missing (MSVC style) to builtin for clang/Apple
 * Prefetch shim (both macro and inline fallback)
 */
#undef __prefetch
#define __prefetch(addr) __builtin_prefetch((addr))
static inline void __prefetch_inline(const void* p){ __builtin_prefetch(p); }

#endif // WICKED_NEON_SHIMS_INCLUDED
#endif // defined(__APPLE__) && defined(__ARM_NEON)
