/* FAudio - XAudio Reimplementation for FNA
 *
 * Copyright (c) 2011-2024 Ethan Lee, Luigi Auriemma, and the MonoGame Team
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Ethan "flibitijibibo" Lee <flibitijibibo@flibitijibibo.com>
 *
 */

#include "FAudio_internal.h"

/* SECTION 0: SSE/NEON Detection */

/* The SSE/NEON detection comes from MojoAL:
 * https://hg.icculus.org/icculus/mojoAL/file/default/mojoal.c
 */

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64ec__) || defined(_M_ARM64EC)
	/* Some platforms fail to define this... */
	#ifndef __ARM_NEON__
	#define __ARM_NEON__ 1
	#endif

	/* AArch64 guarantees NEON. */
	#define NEED_SCALAR_CONVERTER_FALLBACKS 0
#elif defined(__x86_64__) || defined(_M_X64)
	/* Some platforms fail to define this... */
	#ifndef __SSE2__
	#define __SSE2__ 1
	#endif

	/* x86_64 guarantees SSE2. */
	#define NEED_SCALAR_CONVERTER_FALLBACKS 0
#elif __MACOSX__ && !defined(__POWERPC__)
	/* Some build systems may need to specify this. */
	#if !defined(__SSE2__) && !defined(__ARM_NEON__)
	#error macOS does not have SSE2/NEON? Bad compiler?
	#endif

	/* Mac OS X/Intel guarantees SSE2. */
	#define NEED_SCALAR_CONVERTER_FALLBACKS 0
#else
	/* Need plain C implementations to support all other hardware */
	#define NEED_SCALAR_CONVERTER_FALLBACKS 1
#endif

/* Our NEON paths require AArch64, don't check __ARM_NEON__ here */
#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64ec__) || defined(_M_ARM64EC)
#include <arm_neon.h>
#define HAVE_NEON_INTRINSICS 1
#endif


#ifdef __SSE2__
#include <emmintrin.h>
#define HAVE_SSE2_INTRINSICS 1
#endif

/* SECTION 1: Type Converters */

/* The SSE/NEON converters are based on SDL_audiotypecvt:
 * https://hg.libsdl.org/SDL/file/default/src/audio/SDL_audiotypecvt.c
 */

#define DIVBY128 0.0078125f
#define DIVBY32768 0.000030517578125f
#define DIVBY8388607 0.00000011920930376163766f

#if NEED_SCALAR_CONVERTER_FALLBACKS
void FAudio_INTERNAL_Convert_U8_To_F32_Scalar(
	const uint8_t *restrict src,
	float *restrict dst,
	uint32_t len
) {
	uint32_t i;
	for (i = 0; i < len; i += 1)
	{
		*dst++ = (*src++ * DIVBY128) - 1.0f;
	}
}

void FAudio_INTERNAL_Convert_S16_To_F32_Scalar(
	const int16_t *restrict src,
	float *restrict dst,
	uint32_t len
) {
	uint32_t i;
	for (i = 0; i < len; i += 1)
	{
		*dst++ = *src++ * DIVBY32768;
	}
}

void FAudio_INTERNAL_Convert_S32_To_F32_Scalar(
	const int32_t *restrict src,
	float *restrict dst,
	uint32_t len
) {
	uint32_t i;
	for (i = 0; i < len; i += 1)
	{
		*dst++ = (*src++ >> 8) * DIVBY8388607;
	}
}
#endif /* NEED_SCALAR_CONVERTER_FALLBACKS */

#if HAVE_SSE2_INTRINSICS
void FAudio_INTERNAL_Convert_U8_To_F32_SSE2(
	const uint8_t *restrict src,
	float *restrict dst,
	uint32_t len
) {
    int i;
    src += len - 1;
    dst += len - 1;

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = len; i && (((size_t) (dst-15)) & 15); --i, --src, --dst) {
        *dst = (((float) *src) * DIVBY128) - 1.0f;
    }

    src -= 15; dst -= 15;  /* adjust to read SSE blocks from the start. */
    FAudio_assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128i *mmsrc = (const __m128i *) src;
        const __m128i zero = _mm_setzero_si128();
        const __m128 divby128 = _mm_set1_ps(DIVBY128);
        const __m128 minus1 = _mm_set1_ps(-1.0f);
        while (i >= 16) {   /* 16 * 8-bit */
            const __m128i bytes = _mm_load_si128(mmsrc);  /* get 16 uint8 into an XMM register. */
            /* treat as int16, shift left to clear every other sint16, then back right with zero-extend. Now uint16. */
            const __m128i shorts1 = _mm_srli_epi16(_mm_slli_epi16(bytes, 8), 8);
            /* right-shift-zero-extend gets us uint16 with the other set of values. */
            const __m128i shorts2 = _mm_srli_epi16(bytes, 8);
            /* unpack against zero to make these int32, convert to float, multiply, add. Whew! */
            /* Note that AVX2 can do floating point multiply+add in one instruction, fwiw. SSE2 cannot. */
            const __m128 floats1 = _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(shorts1, zero)), divby128), minus1);
            const __m128 floats2 = _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(shorts2, zero)), divby128), minus1);
            const __m128 floats3 = _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi16(shorts1, zero)), divby128), minus1);
            const __m128 floats4 = _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi16(shorts2, zero)), divby128), minus1);
            /* Interleave back into correct order, store. */
            _mm_store_ps(dst, _mm_unpacklo_ps(floats1, floats2));
            _mm_store_ps(dst+4, _mm_unpackhi_ps(floats1, floats2));
            _mm_store_ps(dst+8, _mm_unpacklo_ps(floats3, floats4));
            _mm_store_ps(dst+12, _mm_unpackhi_ps(floats3, floats4));
            i -= 16; mmsrc--; dst -= 16;
        }

        src = (const uint8_t *) mmsrc;
    }

    src += 15; dst += 15;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = (((float) *src) * DIVBY128) - 1.0f;
        i--; src--; dst--;
    }
}

void FAudio_INTERNAL_Convert_S16_To_F32_SSE2(
	const int16_t *restrict src,
	float *restrict dst,
	uint32_t len
) {
    int i;
    src += len - 1;
    dst += len - 1;

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = len; i && (((size_t) (dst-7)) & 15); --i, --src, --dst) {
        *dst = ((float) *src) * DIVBY32768;
    }

    src -= 7; dst -= 7;  /* adjust to read SSE blocks from the start. */
    FAudio_assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128 divby32768 = _mm_set1_ps(DIVBY32768);
        while (i >= 8) {   /* 8 * 16-bit */
            const __m128i ints = _mm_load_si128((__m128i const *) src);  /* get 8 sint16 into an XMM register. */
            /* treat as int32, shift left to clear every other sint16, then back right with sign-extend. Now sint32. */
            const __m128i a = _mm_srai_epi32(_mm_slli_epi32(ints, 16), 16);
            /* right-shift-sign-extend gets us sint32 with the other set of values. */
            const __m128i b = _mm_srai_epi32(ints, 16);
            /* Interleave these back into the right order, convert to float, multiply, store. */
            _mm_store_ps(dst, _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi32(a, b)), divby32768));
            _mm_store_ps(dst+4, _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi32(a, b)), divby32768));
            i -= 8; src -= 8; dst -= 8;
        }
    }

    src += 7; dst += 7;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = ((float) *src) * DIVBY32768;
        i--; src--; dst--;
    }
}

void FAudio_INTERNAL_Convert_S32_To_F32_SSE2(
	const int32_t *restrict src,
	float *restrict dst,
	uint32_t len
) {
    int i;

    /* Get dst aligned to 16 bytes */
    for (i = len; i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        *dst = ((float) (*src>>8)) * DIVBY8388607;
    }

    FAudio_assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128 divby8388607 = _mm_set1_ps(DIVBY8388607);
        const __m128i *mmsrc = (const __m128i *) src;
        while (i >= 4) {   /* 4 * sint32 */
            /* shift out lowest bits so int fits in a float32. Small precision loss, but much faster. */
            _mm_store_ps(dst, _mm_mul_ps(_mm_cvtepi32_ps(_mm_srai_epi32(_mm_load_si128(mmsrc), 8)), divby8388607));
            i -= 4; mmsrc++; dst += 4;
        }
        src = (const int32_t *) mmsrc;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = ((float) (*src>>8)) * DIVBY8388607;
        i--; src++; dst++;
    }
}
#endif /* HAVE_SSE2_INTRINSICS */

#if HAVE_NEON_INTRINSICS
void FAudio_INTERNAL_Convert_U8_To_F32_NEON(
	const uint8_t *restrict src,
	float *restrict dst,
	uint32_t len
) {
    int i;
    src += len - 1;
    dst += len - 1;

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = len; i && (((size_t) (dst-15)) & 15); --i, --src, --dst) {
        *dst = (((float) *src) * DIVBY128) - 1.0f;
    }

    src -= 15; dst -= 15;  /* adjust to read NEON blocks from the start. */
    FAudio_assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const uint8_t *mmsrc = (const uint8_t *) src;
        const float32x4_t divby128 = vdupq_n_f32(DIVBY128);
        const float32x4_t negone = vdupq_n_f32(-1.0f);
        while (i >= 16) {   /* 16 * 8-bit */
            const uint8x16_t bytes = vld1q_u8(mmsrc);  /* get 16 uint8 into a NEON register. */
            const uint16x8_t uint16hi = vmovl_u8(vget_high_u8(bytes));  /* convert top 8 bytes to 8 uint16 */
            const uint16x8_t uint16lo = vmovl_u8(vget_low_u8(bytes));   /* convert bottom 8 bytes to 8 uint16 */
            /* split uint16 to two uint32, then convert to float, then multiply to normalize, subtract to adjust for sign, store. */
            vst1q_f32(dst, vmlaq_f32(negone, vcvtq_f32_u32(vmovl_u16(vget_low_u16(uint16lo))), divby128));
            vst1q_f32(dst+4, vmlaq_f32(negone, vcvtq_f32_u32(vmovl_u16(vget_high_u16(uint16lo))), divby128));
            vst1q_f32(dst+8, vmlaq_f32(negone, vcvtq_f32_u32(vmovl_u16(vget_low_u16(uint16hi))), divby128));
            vst1q_f32(dst+12, vmlaq_f32(negone, vcvtq_f32_u32(vmovl_u16(vget_high_u16(uint16hi))), divby128));
            i -= 16; mmsrc -= 16; dst -= 16;
        }

        src = (const uint8_t *) mmsrc;
    }

    src += 15; dst += 15;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = (((float) *src) * DIVBY128) - 1.0f;
        i--; src--; dst--;
    }
}

void FAudio_INTERNAL_Convert_S16_To_F32_NEON(
	const int16_t *restrict src,
	float *restrict dst,
	uint32_t len
) {
    int i;
    src += len - 1;
    dst += len - 1;

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = len; i && (((size_t) (dst-7)) & 15); --i, --src, --dst) {
        *dst = ((float) *src) * DIVBY32768;
    }

    src -= 7; dst -= 7;  /* adjust to read NEON blocks from the start. */
    FAudio_assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const float32x4_t divby32768 = vdupq_n_f32(DIVBY32768);
        while (i >= 8) {   /* 8 * 16-bit */
            const int16x8_t ints = vld1q_s16((int16_t const *) src);  /* get 8 sint16 into a NEON register. */
            /* split int16 to two int32, then convert to float, then multiply to normalize, store. */
            vst1q_f32(dst, vmulq_f32(vcvtq_f32_s32(vmovl_s16(vget_low_s16(ints))), divby32768));
            vst1q_f32(dst+4, vmulq_f32(vcvtq_f32_s32(vmovl_s16(vget_high_s16(ints))), divby32768));
            i -= 8; src -= 8; dst -= 8;
        }
    }

    src += 7; dst += 7;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = ((float) *src) * DIVBY32768;
        i--; src--; dst--;
    }
}

void FAudio_INTERNAL_Convert_S32_To_F32_NEON(
	const int32_t *restrict src,
	float *restrict dst,
	uint32_t len
) {
    int i;

    /* Get dst aligned to 16 bytes */
    for (i = len; i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        *dst = ((float) (*src>>8)) * DIVBY8388607;
    }

    FAudio_assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const float32x4_t divby8388607 = vdupq_n_f32(DIVBY8388607);
        const int32_t *mmsrc = (const int32_t *) src;
        while (i >= 4) {   /* 4 * sint32 */
            /* shift out lowest bits so int fits in a float32. Small precision loss, but much faster. */
            vst1q_f32(dst, vmulq_f32(vcvtq_f32_s32(vshrq_n_s32(vld1q_s32(mmsrc), 8)), divby8388607));
            i -= 4; mmsrc += 4; dst += 4;
        }
        src = (const int32_t *) mmsrc;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = ((float) (*src>>8)) * DIVBY8388607;
        i--; src++; dst++;
    }
}
#endif /* HAVE_NEON_INTRINSICS */

/* SECTION 2: Linear Resamplers */

void FAudio_INTERNAL_ResampleGeneric(
	float *restrict dCache,
	float *restrict resampleCache,
	uint64_t *resampleOffset,
	uint64_t resampleStep,
	uint64_t toResample,
	uint8_t channels
) {
	uint32_t i, j;
	uint64_t cur = *resampleOffset & FIXED_FRACTION_MASK;
	for (i = 0; i < toResample; i += 1)
	{
		for (j = 0; j < channels; j += 1)
		{
			/* lerp, then convert to float value */
			*resampleCache++ = (float) (
				dCache[j] +
				(dCache[j + channels] - dCache[j]) *
				FIXED_TO_DOUBLE(cur)
			);
		}

		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur >> FIXED_PRECISION) * channels;

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur &= FIXED_FRACTION_MASK;
	}
}

#if NEED_SCALAR_CONVERTER_FALLBACKS
void FAudio_INTERNAL_ResampleMono_Scalar(
	float *restrict dCache,
	float *restrict resampleCache,
	uint64_t *resampleOffset,
	uint64_t resampleStep,
	uint64_t toResample,
	uint8_t UNUSED
) {
	uint32_t i;
	uint64_t cur = *resampleOffset & FIXED_FRACTION_MASK;
	for (i = 0; i < toResample; i += 1)
	{
		/* lerp, then convert to float value */
		*resampleCache++ = (float) (
			dCache[0] +
			(dCache[1] - dCache[0]) *
			FIXED_TO_DOUBLE(cur)
		);

		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur >> FIXED_PRECISION);

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur &= FIXED_FRACTION_MASK;
	}
}

void FAudio_INTERNAL_ResampleStereo_Scalar(
	float *restrict dCache,
	float *restrict resampleCache,
	uint64_t *resampleOffset,
	uint64_t resampleStep,
	uint64_t toResample,
	uint8_t UNUSED
) {
	uint32_t i;
	uint64_t cur = *resampleOffset & FIXED_FRACTION_MASK;
	for (i = 0; i < toResample; i += 1)
	{
		/* lerp, then convert to float value */
		*resampleCache++ = (float) (
			dCache[0] +
			(dCache[2] - dCache[0]) *
			FIXED_TO_DOUBLE(cur)
		);
		*resampleCache++ = (float) (
			dCache[1] +
			(dCache[3] - dCache[1]) *
			FIXED_TO_DOUBLE(cur)
		);

		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur >> FIXED_PRECISION) * 2;

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur &= FIXED_FRACTION_MASK;
	}
}
#endif /* NEED_SCALAR_CONVERTER_FALLBACKS */

/* The SSE2 versions of the resamplers come from @8thMage! */

#if HAVE_SSE2_INTRINSICS
void FAudio_INTERNAL_ResampleMono_SSE2(
	float *restrict dCache,
	float *restrict resampleCache,
	uint64_t *resampleOffset,
	uint64_t resampleStep,
	uint64_t toResample,
	uint8_t UNUSED
) {
	uint32_t i, header, tail;
	uint64_t cur_scalar_1, cur_scalar_2, cur_scalar_3;
	float *dCache_1, *dCache_2, *dCache_3;
	uint64_t cur_scalar = *resampleOffset & FIXED_FRACTION_MASK;
	__m128 one_over_fixed_one, half, current_next_0_1, current_next_2_3,
		current, next, sub, cur_fixed, mul, res;
	__m128i cur_frac, adder_frac, adder_frac_loop;

	/* This is the header, the Dest needs to be aligned to 16B */
	header = (16 - ((size_t) resampleCache) % 16) / 4;
	if (header == 4)
	{
		header = 0;
	}
	for (i = 0; i < header; i += 1)
	{
		/* lerp, then convert to float value */
		*resampleCache++ = (float) (
			dCache[0] +
			(dCache[1] - dCache[0]) *
			FIXED_TO_FLOAT(cur_scalar)
		);

		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur_scalar += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur_scalar >> FIXED_PRECISION);

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur_scalar &= FIXED_FRACTION_MASK;
	}

	toResample -= header;

	/* initialising the varius cur
	 * cur_frac is the fractional part of cur with 4 samples. as the
	 * fractional part is 32 bit unsigned value, it can be just added
	 * and the modulu operation for keeping the fractional part will be implicit.
	 * the 0.5 is for converting signed values to float (no unsigned convert),
	 * the 0.5 is added later.
	 */
	cur_frac = _mm_set1_epi32(
		(uint32_t) (cur_scalar & FIXED_FRACTION_MASK) - DOUBLE_TO_FIXED(0.5)
	);
	adder_frac = _mm_setr_epi32(
		0,
		(uint32_t) (resampleStep & FIXED_FRACTION_MASK),
		(uint32_t) ((resampleStep * 2) & FIXED_FRACTION_MASK),
		(uint32_t) ((resampleStep * 3) & FIXED_FRACTION_MASK)
	);
	cur_frac = _mm_add_epi32(cur_frac, adder_frac);

	/* The various cur_scalar is for the different samples
	 * (1, 2, 3 compared to original cur_scalar = 0)
	 */
	cur_scalar_1 = cur_scalar + resampleStep;
	cur_scalar_2 = cur_scalar + resampleStep * 2;
	cur_scalar_3 = cur_scalar + resampleStep * 3;
	dCache_1 = dCache + (cur_scalar_1 >> FIXED_PRECISION);
	dCache_2 = dCache + (cur_scalar_2 >> FIXED_PRECISION);
	dCache_3 = dCache + (cur_scalar_3 >> FIXED_PRECISION);
	cur_scalar &= FIXED_FRACTION_MASK;
	cur_scalar_1 &= FIXED_FRACTION_MASK;
	cur_scalar_2 &= FIXED_FRACTION_MASK;
	cur_scalar_3 &= FIXED_FRACTION_MASK;

	/* FIXME: These should be _mm_undefined_ps! */
	current_next_0_1 = _mm_setzero_ps();
	current_next_2_3 = _mm_setzero_ps();

	/* Constants */
	one_over_fixed_one = _mm_set1_ps(1.0f / FIXED_ONE);
	half = _mm_set1_ps(0.5f);
	adder_frac_loop = _mm_set1_epi32(
		(uint32_t) ((resampleStep * 4) & FIXED_FRACTION_MASK)
	);

	tail = toResample % 4;
	for (i = 0; i < toResample - tail; i += 4, resampleCache += 4)
	{
		/* current next holds 2 pairs of the sample and the sample + 1
		 * after that need to seperate them.
		 */

		current_next_0_1 = _mm_loadl_pi(current_next_0_1, (__m64*) dCache);
		current_next_0_1 = _mm_loadh_pi(current_next_0_1, (__m64*) dCache_1);
		current_next_2_3 = _mm_loadl_pi(current_next_2_3, (__m64*) dCache_2);
		current_next_2_3 = _mm_loadh_pi(current_next_2_3, (__m64*) dCache_3);

		/* Unpack them to have seperate current and next in 2 vectors. */
		current = _mm_shuffle_ps(current_next_0_1, current_next_2_3, 0x88); /* 0b1000 */
		next = _mm_shuffle_ps(current_next_0_1, current_next_2_3, 0xdd); /* 0b1101 */

		sub = _mm_sub_ps(next, current);

		/* Convert the fractional part to float and then mul to get the fractions out.
		 * then add back the 0.5 we subtracted before.
		 */
		cur_fixed = _mm_add_ps(
			_mm_mul_ps(
				_mm_cvtepi32_ps(cur_frac),
				one_over_fixed_one
			),
			half
		);
		mul = _mm_mul_ps(sub, cur_fixed);
		res = _mm_add_ps(current, mul);

		/* Store back */
		_mm_store_ps(resampleCache, res);

		/* Update dCaches for next iteration */
		cur_scalar += resampleStep * 4;
		cur_scalar_1 += resampleStep * 4;
		cur_scalar_2 += resampleStep * 4;
		cur_scalar_3 += resampleStep * 4;
		dCache = dCache + (cur_scalar >> FIXED_PRECISION);
		dCache_1 = dCache_1 + (cur_scalar_1 >> FIXED_PRECISION);
		dCache_2 = dCache_2 + (cur_scalar_2 >> FIXED_PRECISION);
		dCache_3 = dCache_3 + (cur_scalar_3 >> FIXED_PRECISION);
		cur_scalar &= FIXED_FRACTION_MASK;
		cur_scalar_1 &= FIXED_FRACTION_MASK;
		cur_scalar_2 &= FIXED_FRACTION_MASK;
		cur_scalar_3 &= FIXED_FRACTION_MASK;

		cur_frac = _mm_add_epi32(cur_frac, adder_frac_loop);
	}
	*resampleOffset += resampleStep * (toResample - tail);

	/* This is the tail. */
	for (i = 0; i < tail; i += 1)
	{
		/* lerp, then convert to float value */
		*resampleCache++ = (float) (
			dCache[0] +
			(dCache[1] - dCache[0]) *
			FIXED_TO_FLOAT(cur_scalar)
		);
		
		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur_scalar += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur_scalar >> FIXED_PRECISION);

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur_scalar &= FIXED_FRACTION_MASK;
	}
}

void FAudio_INTERNAL_ResampleStereo_SSE2(
	float *restrict dCache,
	float *restrict resampleCache,
	uint64_t *resampleOffset,
	uint64_t resampleStep,
	uint64_t toResample,
	uint8_t UNUSED
) {
	uint32_t i, header, tail;
	uint64_t cur_scalar, cur_scalar_1;
	float *dCache_1;
	__m128 one_over_fixed_one, half, current_next_1, current_next_2,
		current, next, sub, cur_fixed, mul, res;
	__m128i cur_frac, adder_frac, adder_frac_loop;

	/* This is the header, the Dest needs to be aligned to 16B */
	header = (16 - ((size_t) resampleCache) % 16) / 8;
	if (header == 2)
	{
		header = 0;
	}
	cur_scalar = *resampleOffset & FIXED_FRACTION_MASK;
	for (i = 0; i < header; i += 2)
	{
		/* lerp, then convert to float value */
		*resampleCache++ = (float) (
			dCache[0] +
			(dCache[2] - dCache[0]) *
			FIXED_TO_FLOAT(cur_scalar)
		);
		*resampleCache++ = (float) (
			dCache[1] +
			(dCache[3] - dCache[1]) *
			FIXED_TO_FLOAT(cur_scalar)
		);

		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur_scalar += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur_scalar >> FIXED_PRECISION) * 2;

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur_scalar &= FIXED_FRACTION_MASK;
	}

	toResample -= header;

	/* initialising the varius cur.
	 * cur_frac holds the fractional part of cur.
	 * to avoid duplication please see the mono part for a thorough
	 * explanation.
	 */
	cur_frac = _mm_set1_epi32(
		(uint32_t) (cur_scalar & FIXED_FRACTION_MASK) - DOUBLE_TO_FIXED(0.5)
	);
	adder_frac = _mm_setr_epi32(
		0,
		0,
		(uint32_t) (resampleStep & FIXED_FRACTION_MASK),
		(uint32_t) (resampleStep & FIXED_FRACTION_MASK)
	);
	cur_frac = _mm_add_epi32(cur_frac, adder_frac);

	/* dCache_1 is the pointer for dcache in the next resample pos. */
	cur_scalar_1 = cur_scalar + resampleStep;
	dCache_1 = dCache + (cur_scalar_1 >> FIXED_PRECISION) * 2;
	cur_scalar_1 &= FIXED_FRACTION_MASK;

	one_over_fixed_one = _mm_set1_ps(1.0f / FIXED_ONE);
	half = _mm_set1_ps(0.5f);
	adder_frac_loop = _mm_set1_epi32(
		(uint32_t) ((resampleStep * 2) & FIXED_FRACTION_MASK)
	);

	tail = toResample % 2;
	for (i = 0; i < toResample - tail; i += 2, resampleCache += 4)
	{
		/* Current_next_1 and current_next_2 each holds 4 src
		 * sample points for getting 4 dest resample point at the end.
		 * current_next_1 holds:
		 * (current_ch_1, current_ch_2, next_ch_1, next_ch_2)
		 * for the first resample position, while current_next_2 holds
		 * the same for the 2nd resample position
		 */
		current_next_1 = _mm_loadu_ps(dCache); /* A1B1A2B2 */
		current_next_2 = _mm_loadu_ps(dCache_1); /* A3B3A4B4 */

		/* Unpack them to get the current and the next in seperate vectors. */
		current = _mm_castpd_ps(
			_mm_unpacklo_pd(
				_mm_castps_pd(current_next_1),
				_mm_castps_pd(current_next_2)
			)
		);
		next = _mm_castpd_ps(
			_mm_unpackhi_pd(
				_mm_castps_pd(current_next_1),
				_mm_castps_pd(current_next_2)
			)
		);

		sub = _mm_sub_ps(next, current);

		/* Adding the 0.5 back.
		 * See mono explanation for more elaborate explanation.
		 */
		cur_fixed = _mm_add_ps(
			_mm_mul_ps(
				_mm_cvtepi32_ps(cur_frac),
				one_over_fixed_one
			),
			half
		);
		mul = _mm_mul_ps(sub, cur_fixed);
		res = _mm_add_ps(current, mul);

		/* Store the results */
		_mm_store_ps(resampleCache, res);

		/* Update dCaches for next iteration */
		cur_scalar += resampleStep * 2;
		cur_scalar_1 += resampleStep * 2;
		dCache = dCache + (cur_scalar >> FIXED_PRECISION) * 2;
		dCache_1 = dCache_1 + (cur_scalar_1 >> FIXED_PRECISION) * 2;
		cur_scalar &= FIXED_FRACTION_MASK;
		cur_scalar_1 &= FIXED_FRACTION_MASK;

		cur_frac = _mm_add_epi32(cur_frac, adder_frac_loop);
	}
	*resampleOffset += resampleStep * (toResample - tail);

	/* This is the tail. */
	for (i = 0; i < tail; i += 1)
	{
		/* lerp, then convert to float value */
		*resampleCache++ = (float) (
			dCache[0] +
			(dCache[2] - dCache[0]) *
			FIXED_TO_FLOAT(cur_scalar)
		);
		*resampleCache++ = (float) (
			dCache[1] +
			(dCache[3] - dCache[1]) *
			FIXED_TO_FLOAT(cur_scalar)
		);

		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur_scalar += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur_scalar >> FIXED_PRECISION) * 2;

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur_scalar &= FIXED_FRACTION_MASK;
	}
}
#endif /* HAVE_SSE2_INTRINSICS */

#if HAVE_NEON_INTRINSICS
void FAudio_INTERNAL_ResampleMono_NEON(
	float *restrict dCache,
	float *restrict resampleCache,
	uint64_t *resampleOffset,
	uint64_t resampleStep,
	uint64_t toResample,
	uint8_t UNUSED
) {
	uint32_t i, header, tail;
	uint64_t cur_scalar_1, cur_scalar_2, cur_scalar_3;
	float *dCache_1, *dCache_2, *dCache_3;
	uint64_t cur_scalar = *resampleOffset & FIXED_FRACTION_MASK;
	float32x4_t one_over_fixed_one, half, current_next_0_1, current_next_2_3,
		current, next, sub, cur_fixed, mul, res;
	int32x4_t cur_frac, adder_frac, adder_frac_loop;

	/* This is the header, the Dest needs to be aligned to 16B */
	header = (16 - ((size_t) resampleCache) % 16) / 4;
	if (header == 4)
	{
		header = 0;
	}
	for (i = 0; i < header; i += 1)
	{
		/* lerp, then convert to float value */
		*resampleCache++ = (float) (
			dCache[0] +
			(dCache[1] - dCache[0]) *
			FIXED_TO_FLOAT(cur_scalar)
		);

		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur_scalar += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur_scalar >> FIXED_PRECISION);

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur_scalar &= FIXED_FRACTION_MASK;
	}

	toResample -= header;

	/* initialising the varius cur
	 * cur_frac is the fractional part of cur with 4 samples. as the
	 * fractional part is 32 bit unsigned value, it can be just added
	 * and the modulu operation for keeping the fractional part will be implicit.
	 * the 0.5 is for converting signed values to float (no unsigned convert),
	 * the 0.5 is added later.
	 */
	cur_frac = vdupq_n_s32(
		(uint32_t) (cur_scalar & FIXED_FRACTION_MASK) - DOUBLE_TO_FIXED(0.5)
	);
	ALIGN(int32_t, 16) data[4] =
	{
		0,
		(uint32_t) (resampleStep & FIXED_FRACTION_MASK),
		(uint32_t) ((resampleStep * 2) & FIXED_FRACTION_MASK),
		(uint32_t) ((resampleStep * 3) & FIXED_FRACTION_MASK)
	};
	adder_frac = vld1q_s32(data);
	cur_frac = vaddq_s32(cur_frac, adder_frac);

	/* The various cur_scalar is for the different samples
	 * (1, 2, 3 compared to original cur_scalar = 0)
	 */
	cur_scalar_1 = cur_scalar + resampleStep;
	cur_scalar_2 = cur_scalar + resampleStep * 2;
	cur_scalar_3 = cur_scalar + resampleStep * 3;
	dCache_1 = dCache + (cur_scalar_1 >> FIXED_PRECISION);
	dCache_2 = dCache + (cur_scalar_2 >> FIXED_PRECISION);
	dCache_3 = dCache + (cur_scalar_3 >> FIXED_PRECISION);
	cur_scalar &= FIXED_FRACTION_MASK;
	cur_scalar_1 &= FIXED_FRACTION_MASK;
	cur_scalar_2 &= FIXED_FRACTION_MASK;
	cur_scalar_3 &= FIXED_FRACTION_MASK;

	/* Constants */
	one_over_fixed_one = vdupq_n_f32(1.0f / FIXED_ONE);
	half = vdupq_n_f32(0.5f);
	adder_frac_loop = vdupq_n_s32(
		(uint32_t) ((resampleStep * 4) & FIXED_FRACTION_MASK)
	);

	tail = toResample % 4;
	for (i = 0; i < toResample - tail; i += 4, resampleCache += 4)
	{
		/* current next holds 2 pairs of the sample and the sample + 1
		 * after that need to separate them.
		 */
		current_next_0_1 = vcombine_f32(
			vld1_f32(dCache),
			vld1_f32(dCache_1)
		);
		current_next_2_3 = vcombine_f32(
			vld1_f32(dCache_2),
			vld1_f32(dCache_3)
		);

		/* Unpack them to have seperate current and next in 2 vectors. */
		current = vuzp1q_f32(current_next_0_1, current_next_2_3);
		next = vuzp2q_f32(current_next_0_1, current_next_2_3);

		sub = vsubq_f32(next, current);

		/* Convert the fractional part to float and then mul to get the fractions out.
		 * then add back the 0.5 we subtracted before.
		 */
		cur_fixed = vaddq_f32(
			vmulq_f32(
				vcvtq_f32_s32(cur_frac),
				one_over_fixed_one
			),
			half
		);
		mul = vmulq_f32(sub, cur_fixed);
		res = vaddq_f32(current, mul);

		/* Store back */
		vst1q_f32(resampleCache, res);

		/* Update dCaches for next iteration */
		cur_scalar += resampleStep * 4;
		cur_scalar_1 += resampleStep * 4;
		cur_scalar_2 += resampleStep * 4;
		cur_scalar_3 += resampleStep * 4;
		dCache = dCache + (cur_scalar >> FIXED_PRECISION);
		dCache_1 = dCache_1 + (cur_scalar_1 >> FIXED_PRECISION);
		dCache_2 = dCache_2 + (cur_scalar_2 >> FIXED_PRECISION);
		dCache_3 = dCache_3 + (cur_scalar_3 >> FIXED_PRECISION);
		cur_scalar &= FIXED_FRACTION_MASK;
		cur_scalar_1 &= FIXED_FRACTION_MASK;
		cur_scalar_2 &= FIXED_FRACTION_MASK;
		cur_scalar_3 &= FIXED_FRACTION_MASK;

		cur_frac = vaddq_s32(cur_frac, adder_frac_loop);
	}
	*resampleOffset += resampleStep * (toResample - tail);

	/* This is the tail. */
	for (i = 0; i < tail; i += 1)
	{
		/* lerp, then convert to float value */
		*resampleCache++ = (float) (
			dCache[0] +
			(dCache[1] - dCache[0]) *
			FIXED_TO_FLOAT(cur_scalar)
		);

		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur_scalar += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur_scalar >> FIXED_PRECISION);

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur_scalar &= FIXED_FRACTION_MASK;
	}
}

void FAudio_INTERNAL_ResampleStereo_NEON(
	float *restrict dCache,
	float *restrict resampleCache,
	uint64_t *resampleOffset,
	uint64_t resampleStep,
	uint64_t toResample,
	uint8_t channels
) {
	uint32_t i, header, tail;
	uint64_t cur_scalar, cur_scalar_1;
	float *dCache_1;
	float32x4_t one_over_fixed_one, half, current, next, sub, cur_fixed, mul, res;
	int32x4_t cur_frac, adder_frac, adder_frac_loop;

	/* This is the header, the Dest needs to be aligned to 16B */
	header = (16 - ((size_t) resampleCache) % 16) / 8;
	if (header == 2)
	{
		header = 0;
	}
	cur_scalar = *resampleOffset & FIXED_FRACTION_MASK;
	for (i = 0; i < header; i += 2)
	{
		/* lerp, then convert to float value */
		*resampleCache++ = (float) (
			dCache[0] +
			(dCache[2] - dCache[0]) *
			FIXED_TO_FLOAT(cur_scalar)
		);
		*resampleCache++ = (float) (
			dCache[1] +
			(dCache[3] - dCache[1]) *
			FIXED_TO_FLOAT(cur_scalar)
		);

		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur_scalar += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur_scalar >> FIXED_PRECISION) * 2;

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur_scalar &= FIXED_FRACTION_MASK;
	}

	toResample -= header;

	/* initialising the varius cur.
	 * cur_frac holds the fractional part of cur.
	 * to avoid duplication please see the mono part for a thorough
	 * explanation.
	 */
	cur_frac = vdupq_n_s32(
		(uint32_t) (cur_scalar & FIXED_FRACTION_MASK) - DOUBLE_TO_FIXED(0.5)
	);
	ALIGN(int32_t, 16) data[4] =
	{
		0,
		0,
		(uint32_t) (resampleStep & FIXED_FRACTION_MASK),
		(uint32_t) (resampleStep & FIXED_FRACTION_MASK)
	};
	adder_frac = vld1q_s32(data);
	cur_frac = vaddq_s32(cur_frac, adder_frac);

	/* dCache_1 is the pointer for dcache in the next resample pos. */
	cur_scalar_1 = cur_scalar + resampleStep;
	dCache_1 = dCache + (cur_scalar_1 >> FIXED_PRECISION) * 2;
	cur_scalar_1 &= FIXED_FRACTION_MASK;

	one_over_fixed_one = vdupq_n_f32(1.0f / FIXED_ONE);
	half = vdupq_n_f32(0.5f);
	adder_frac_loop = vdupq_n_s32(
		(uint32_t) ((resampleStep * 2) & FIXED_FRACTION_MASK)
	);

	tail = toResample % 2;
	for (i = 0; i < toResample - tail; i += 2, resampleCache += 4)
	{
		/* Current_next_1 and current_next_2 each holds 4 src
		 * sample points for getting 4 dest resample point at the end.
		 * current_next_1 holds:
		 * (current_ch_1, current_ch_2, next_ch_1, next_ch_2)
		 * for the first resample position, while current_next_2 holds
		 * the same for the 2nd resample position
		 */
		current = vcombine_f32(
			vld1_f32(dCache), /* A1B1 */
			vld1_f32(dCache_1) /* A3B3 */
		);
		next = vcombine_f32(
			vld1_f32(dCache + 2), /* A2B2 */
			vld1_f32(dCache_1 + 2) /* A4B4 */
		);

		sub = vsubq_f32(next, current);

		/* Adding the 0.5 back.
		 * See mono explanation for more elaborate explanation.
		 */
		cur_fixed = vaddq_f32(
			vmulq_f32(
				vcvtq_f32_s32(cur_frac),
				one_over_fixed_one
			),
			half
		);
		mul = vmulq_f32(sub, cur_fixed);
		res = vaddq_f32(current, mul);

		/* Store the results */
		vst1q_f32(resampleCache, res);

		/* Update dCaches for next iteration */
		cur_scalar += resampleStep * 2;
		cur_scalar_1 += resampleStep * 2;
		dCache = dCache + (cur_scalar >> FIXED_PRECISION) * 2;
		dCache_1 = dCache_1 + (cur_scalar_1 >> FIXED_PRECISION) * 2;
		cur_scalar &= FIXED_FRACTION_MASK;
		cur_scalar_1 &= FIXED_FRACTION_MASK;

		cur_frac = vaddq_s32(cur_frac, adder_frac_loop);
	}
	*resampleOffset += resampleStep * (toResample - tail);

	/* This is the tail. */
	for (i = 0; i < tail; i += 1)
	{
		/* lerp, then convert to float value */
		*resampleCache++ = (float) (
			dCache[0] +
			(dCache[2] - dCache[0]) *
			FIXED_TO_FLOAT(cur_scalar)
		);
		*resampleCache++ = (float) (
			dCache[1] +
			(dCache[3] - dCache[1]) *
			FIXED_TO_FLOAT(cur_scalar)
		);

		/* Increment fraction offset by the stepping value */
		*resampleOffset += resampleStep;
		cur_scalar += resampleStep;

		/* Only increment the sample offset by integer values.
		 * Sometimes this will be 0 until cur accumulates
		 * enough steps, especially for "slow" rates.
		 */
		dCache += (cur_scalar >> FIXED_PRECISION) * 2;

		/* Now that any integer has been added, drop it.
		 * The offset pointer will preserve the total.
		 */
		cur_scalar &= FIXED_FRACTION_MASK;
	}
}
#endif /* HAVE_NEON_INTRINSICS */

/* SECTION 3: Amplifiers */

#if NEED_SCALAR_CONVERTER_FALLBACKS
void FAudio_INTERNAL_Amplify_Scalar(
	float* output,
	uint32_t totalSamples,
	float volume
) {
	uint32_t i;
	for (i = 0; i < totalSamples; i += 1)
	{
		output[i] *= volume;
	}
}
#endif /* NEED_SCALAR_CONVERTER_FALLBACKS */

/* The SSE2 version of the amplifier comes from @8thMage! */

#if HAVE_SSE2_INTRINSICS
void FAudio_INTERNAL_Amplify_SSE2(
	float* output,
	uint32_t totalSamples,
	float volume
) {
	uint32_t i;
	uint32_t header = (16 - (((size_t) output) % 16)) / 4;
	uint32_t tail = (totalSamples - header) % 4;
	__m128 volumeVec, outVec;
	if (header == 4)
	{
		header = 0;
	}
	if (tail == 4)
	{
		tail = 0;
	}

	for (i = 0; i < header; i += 1)
	{
		output[i] *= volume;
	}

	volumeVec = _mm_set1_ps(volume);
	for (i = header; i < totalSamples - tail; i += 4)
	{
		outVec = _mm_load_ps(output + i);
		outVec = _mm_mul_ps(outVec, volumeVec);
		_mm_store_ps(output + i, outVec);
	}

	for (i = totalSamples - tail; i < totalSamples; i += 1)
	{
		output[i] *= volume;
	}
}
#endif /* HAVE_SSE2_INTRINSICS */

#if HAVE_NEON_INTRINSICS
void FAudio_INTERNAL_Amplify_NEON(
	float* output,
	uint32_t totalSamples,
	float volume
) {
	uint32_t i;
	uint32_t header = (16 - (((size_t) output) % 16)) / 4;
	uint32_t tail = (totalSamples - header) % 4;
	float32x4_t volumeVec, outVec;
	if (header == 4)
	{
		header = 0;
	}
	if (tail == 4)
	{
		tail = 0;
	}

	for (i = 0; i < header; i += 1)
	{
		output[i] *= volume;
	}

	volumeVec = vdupq_n_f32(volume);
	for (i = header; i < totalSamples - tail; i += 4)
	{
		outVec = vld1q_f32(output + i);
		outVec = vmulq_f32(outVec, volumeVec);
		vst1q_f32(output + i, outVec);
	}

	for (i = totalSamples - tail; i < totalSamples; i += 1)
	{
		output[i] *= volume;
	}
}
#endif /* HAVE_NEON_INTRINSICS */

/* SECTION 4: Mixer Functions */

void FAudio_INTERNAL_Mix_Generic_Scalar(
	uint32_t toMix,
	uint32_t srcChans,
	uint32_t dstChans,
	float *restrict src,
	float *restrict dst,
	float *restrict coefficients
) {
	uint32_t i, co, ci;
	for (i = 0; i < toMix; i += 1, src += srcChans, dst += dstChans)
	for (co = 0; co < dstChans; co += 1)
	{
		for (ci = 0; ci < srcChans; ci += 1)
		{
			dst[co] += (
				src[ci] *
				coefficients[co * srcChans + ci]
			);
		}
	}
}

#if HAVE_SSE2_INTRINSICS
/* SSE horizontal add by Peter Cordes, CC-BY-SA.
 * From https://stackoverflow.com/a/35270026 */
static inline float FAudio_simd_hadd(__m128 v)
{
	__m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
	__m128 sums = _mm_add_ps(v, shuf);
	shuf = _mm_movehl_ps(shuf, sums);
	sums = _mm_add_ss(sums, shuf);
	return _mm_cvtss_f32(sums);
}

void FAudio_INTERNAL_Mix_Generic_SSE2(
	uint32_t toMix,
	uint32_t srcChans,
	uint32_t dstChans,
	float *restrict src,
	float *restrict dst,
	float *restrict coefficients
) {
	uint32_t i, co, ci;
	for (i = 0; i < toMix; i += 1, src += srcChans, dst += dstChans)
	for (co = 0; co < dstChans; co += 1)
	{
		for (ci = 0; srcChans - ci >= 4; ci += 4)
		{
			/* do SIMD */
			const __m128 vols = _mm_loadu_ps(&coefficients[co * srcChans + ci]);
			const __m128 dat = _mm_loadu_ps(&src[ci]);
			dst[co] += FAudio_simd_hadd(_mm_mul_ps(dat, vols));
		}

		for (; ci < srcChans; ci += 1)
		{
			/* do scalar */
			dst[co] += (
				src[ci] *
				coefficients[co * srcChans + ci]
			);
		}
	}
}
#endif /* HAVE_SSE2_INTRINSICS */

void FAudio_INTERNAL_Mix_1in_1out_Scalar(
	uint32_t toMix,
	uint32_t UNUSED1,
	uint32_t UNUSED2,
	float *restrict src,
	float *restrict dst,
	float *restrict coefficients
) {
	uint32_t i;
	for (i = 0; i < toMix; i += 1, src += 1, dst += 1)
	{
		/* Base source data, combined with the coefficients */
		dst[0] += src[0] * coefficients[0];
	}
}

void FAudio_INTERNAL_Mix_1in_2out_Scalar(
	uint32_t toMix,
	uint32_t UNUSED1,
	uint32_t UNUSED2,
	float *restrict src,
	float *restrict dst,
	float *restrict coefficients
) {
	uint32_t i;
	for (i = 0; i < toMix; i += 1, src += 1, dst += 2)
	{
		dst[0] += src[0] * coefficients[0];
		dst[1] += src[0] * coefficients[1];
	}
}

void FAudio_INTERNAL_Mix_1in_6out_Scalar(
	uint32_t toMix,
	uint32_t UNUSED1,
	uint32_t UNUSED2,
	float *restrict src,
	float *restrict dst,
	float *restrict coefficients
) {
	uint32_t i;
	for (i = 0; i < toMix; i += 1, src += 1, dst += 6)
	{
		dst[0] += src[0] * coefficients[0];
		dst[1] += src[0] * coefficients[1];
		dst[2] += src[0] * coefficients[2];
		dst[3] += src[0] * coefficients[3];
		dst[4] += src[0] * coefficients[4];
		dst[5] += src[0] * coefficients[5];
	}
}

void FAudio_INTERNAL_Mix_1in_8out_Scalar(
	uint32_t toMix,
	uint32_t UNUSED1,
	uint32_t UNUSED2,
	float *restrict src,
	float *restrict dst,
	float *restrict coefficients
) {
	uint32_t i;
	for (i = 0; i < toMix; i += 1, src += 1, dst += 8)
	{
		dst[0] += src[0] * coefficients[0];
		dst[1] += src[0] * coefficients[1];
		dst[2] += src[0] * coefficients[2];
		dst[3] += src[0] * coefficients[3];
		dst[4] += src[0] * coefficients[4];
		dst[5] += src[0] * coefficients[5];
		dst[6] += src[0] * coefficients[6];
		dst[7] += src[0] * coefficients[7];
	}
}

void FAudio_INTERNAL_Mix_2in_1out_Scalar(
	uint32_t toMix,
	uint32_t UNUSED1,
	uint32_t UNUSED2,
	float *restrict src,
	float *restrict dst,
	float *restrict coefficients
) {
	uint32_t i;
	for (i = 0; i < toMix; i += 1, src += 2, dst += 1)
	{
		/* Base source data, combined with the coefficients */
		dst[0] += (
			(src[0] * coefficients[0]) +
			(src[1] * coefficients[1])
		);
	}
}

void FAudio_INTERNAL_Mix_2in_2out_Scalar(
	uint32_t toMix,
	uint32_t UNUSED1,
	uint32_t UNUSED2,
	float *restrict src,
	float *restrict dst,
	float *restrict coefficients
) {
	uint32_t i;
	for (i = 0; i < toMix; i += 1, src += 2, dst += 2)
	{
		dst[0] += (
			(src[0] * coefficients[0]) +
			(src[1] * coefficients[1])
		);
		dst[1] += (
			(src[0] * coefficients[2]) +
			(src[1] * coefficients[3])
		);
	}
}

void FAudio_INTERNAL_Mix_2in_6out_Scalar(
	uint32_t toMix,
	uint32_t UNUSED1,
	uint32_t UNUSED2,
	float *restrict src,
	float *restrict dst,
	float *restrict coefficients
) {
	uint32_t i;
	for (i = 0; i < toMix; i += 1, src += 2, dst += 6)
	{
		dst[0] += (
			(src[0] * coefficients[0]) +
			(src[1] * coefficients[1])
		);
		dst[1] += (
			(src[0] * coefficients[2]) +
			(src[1] * coefficients[3])
		);
		dst[2] += (
			(src[0] * coefficients[4]) +
			(src[1] * coefficients[5])
		);
		dst[3] += (
			(src[0] * coefficients[6]) +
			(src[1] * coefficients[7])
		);
		dst[4] += (
			(src[0] * coefficients[8]) +
			(src[1] * coefficients[9])
		);
		dst[5] += (
			(src[0] * coefficients[10]) +
			(src[1] * coefficients[11])
		);
	}
}

void FAudio_INTERNAL_Mix_2in_8out_Scalar(
	uint32_t toMix,
	uint32_t UNUSED1,
	uint32_t UNUSED2,
	float *restrict src,
	float *restrict dst,
	float *restrict coefficients
) {
	uint32_t i;
	for (i = 0; i < toMix; i += 1, src += 2, dst += 8)
	{
		dst[0] += (
			(src[0] * coefficients[0]) +
			(src[1] * coefficients[1])
		);
		dst[1] += (
			(src[0] * coefficients[2]) +
			(src[1] * coefficients[3])
		);
		dst[2] += (
			(src[0] * coefficients[4]) +
			(src[1] * coefficients[5])
		);
		dst[3] += (
			(src[0] * coefficients[6]) +
			(src[1] * coefficients[7])
		);
		dst[4] += (
			(src[0] * coefficients[8]) +
			(src[1] * coefficients[9])
		);
		dst[5] += (
			(src[0] * coefficients[10]) +
			(src[1] * coefficients[11])
		);
		dst[6] += (
			(src[0] * coefficients[12]) +
			(src[1] * coefficients[13])
		);
		dst[7] += (
			(src[0] * coefficients[14]) +
			(src[1] * coefficients[15])
		);
	}
}

/* SECTION 5: InitSIMDFunctions. Assigns based on SSE2/NEON support. */

void (*FAudio_INTERNAL_Convert_U8_To_F32)(
	const uint8_t *restrict src,
	float *restrict dst,
	uint32_t len
);
void (*FAudio_INTERNAL_Convert_S16_To_F32)(
	const int16_t *restrict src,
	float *restrict dst,
	uint32_t len
);
void (*FAudio_INTERNAL_Convert_S32_To_F32)(
	const int32_t *restrict src,
	float *restrict dst,
	uint32_t len
);

FAudioResampleCallback FAudio_INTERNAL_ResampleMono;
FAudioResampleCallback FAudio_INTERNAL_ResampleStereo;

void (*FAudio_INTERNAL_Amplify)(
	float *output,
	uint32_t totalSamples,
	float volume
);

FAudioMixCallback FAudio_INTERNAL_Mix_Generic;

void FAudio_INTERNAL_InitSIMDFunctions(uint8_t hasSSE2, uint8_t hasNEON)
{
#if HAVE_SSE2_INTRINSICS
	if (hasSSE2)
	{
		FAudio_INTERNAL_Convert_U8_To_F32 = FAudio_INTERNAL_Convert_U8_To_F32_SSE2;
		FAudio_INTERNAL_Convert_S16_To_F32 = FAudio_INTERNAL_Convert_S16_To_F32_SSE2;
		FAudio_INTERNAL_Convert_S32_To_F32 = FAudio_INTERNAL_Convert_S32_To_F32_SSE2;
		FAudio_INTERNAL_ResampleMono = FAudio_INTERNAL_ResampleMono_SSE2;
		FAudio_INTERNAL_ResampleStereo = FAudio_INTERNAL_ResampleStereo_SSE2;
		FAudio_INTERNAL_Amplify = FAudio_INTERNAL_Amplify_SSE2;
		FAudio_INTERNAL_Mix_Generic = FAudio_INTERNAL_Mix_Generic_SSE2;
		return;
	}
#endif
#if HAVE_NEON_INTRINSICS
	if (hasNEON)
	{
		FAudio_INTERNAL_Convert_U8_To_F32 = FAudio_INTERNAL_Convert_U8_To_F32_NEON;
		FAudio_INTERNAL_Convert_S16_To_F32 = FAudio_INTERNAL_Convert_S16_To_F32_NEON;
		FAudio_INTERNAL_Convert_S32_To_F32 = FAudio_INTERNAL_Convert_S32_To_F32_NEON;
		FAudio_INTERNAL_ResampleMono = FAudio_INTERNAL_ResampleMono_NEON;
		FAudio_INTERNAL_ResampleStereo = FAudio_INTERNAL_ResampleStereo_NEON;
		FAudio_INTERNAL_Amplify = FAudio_INTERNAL_Amplify_NEON;
		FAudio_INTERNAL_Mix_Generic = FAudio_INTERNAL_Mix_Generic_Scalar;
		return;
	}
#endif
#if NEED_SCALAR_CONVERTER_FALLBACKS
	FAudio_INTERNAL_Convert_U8_To_F32 = FAudio_INTERNAL_Convert_U8_To_F32_Scalar;
	FAudio_INTERNAL_Convert_S16_To_F32 = FAudio_INTERNAL_Convert_S16_To_F32_Scalar;
	FAudio_INTERNAL_Convert_S32_To_F32 = FAudio_INTERNAL_Convert_S32_To_F32_Scalar;
	FAudio_INTERNAL_ResampleMono = FAudio_INTERNAL_ResampleMono_Scalar;
	FAudio_INTERNAL_ResampleStereo = FAudio_INTERNAL_ResampleStereo_Scalar;
	FAudio_INTERNAL_Amplify = FAudio_INTERNAL_Amplify_Scalar;
	FAudio_INTERNAL_Mix_Generic = FAudio_INTERNAL_Mix_Generic_Scalar;
#else
	FAudio_assert(0 && "Need converter functions!");
#endif
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
