#pragma once
#include "CommonInclude.h"
#include "wiArchive.h"
#include "wiRandom.h"
#include "wiMath.h"

// Note: these should be implemented independently of math library optimizations to be cross platform deterministic!
//	Otherwise the terrain generation might be different across platforms

namespace wi::noise
{
	// Based on: https://github.com/Reputeless/PerlinNoise
	struct Perlin
	{
		uint8_t state[256] = {};

		void init(uint32_t seed)
		{
			wi::random::RNG rng(seed);
			for (int i = 0; i < arraysize(state); ++i)
			{
				state[i] = uint8_t(rng.next_uint());
			}
		}
		constexpr float fade(float t) const
		{
			return t * t * t * (t * (t * 6 - 15) + 10);
		}
		constexpr float lerp(float a, float b, float t) const
		{
			return (a + (b - a) * t);
		}
		constexpr float grad(uint8_t hash, float x, float y, float z) const
		{
			const uint8_t h = hash & 15;
			const float u = h < 8 ? x : y;
			const float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
			return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
		}
		// returns noise in range [-1, 1]
		inline float compute(float x, float y, float z) const
		{
			const float _x = std::floor(x);
			const float _y = std::floor(y);
			const float _z = std::floor(z);

			const int ix = int(_x) & 255;
			const int iy = int(_y) & 255;
			const int iz = int(_z) & 255;

			const float fx = (x - _x);
			const float fy = (y - _y);
			const float fz = (z - _z);

			const float u = fade(fx);
			const float v = fade(fy);
			const float w = fade(fz);

			const uint8_t A = (state[ix & 255] + iy) & 255;
			const uint8_t B = (state[(ix + 1) & 255] + iy) & 255;

			const uint8_t AA = (state[A] + iz) & 255;
			const uint8_t AB = (state[(A + 1) & 255] + iz) & 255;

			const uint8_t BA = (state[B] + iz) & 255;
			const uint8_t BB = (state[(B + 1) & 255] + iz) & 255;

			const float p0 = grad(state[AA], fx, fy, fz);
			const float p1 = grad(state[BA], fx - 1, fy, fz);
			const float p2 = grad(state[AB], fx, fy - 1, fz);
			const float p3 = grad(state[BB], fx - 1, fy - 1, fz);
			const float p4 = grad(state[(AA + 1) & 255], fx, fy, fz - 1);
			const float p5 = grad(state[(BA + 1) & 255], fx - 1, fy, fz - 1);
			const float p6 = grad(state[(AB + 1) & 255], fx, fy - 1, fz - 1);
			const float p7 = grad(state[(BB + 1) & 255], fx - 1, fy - 1, fz - 1);

			const float q0 = lerp(p0, p1, u);
			const float q1 = lerp(p2, p3, u);
			const float q2 = lerp(p4, p5, u);
			const float q3 = lerp(p6, p7, u);

			const float r0 = lerp(q0, q1, v);
			const float r1 = lerp(q2, q3, v);

			return lerp(r0, r1, w);
		}
		// returns noise in range [-1, 1]
		constexpr float compute(float x, float y, float z, int octaves, float persistence = 0.5f) const
		{
			float result = 0;
			float amplitude = 1;
			for (int i = 0; i < octaves; ++i)
			{
				result += (compute(x, y, z) * amplitude);
				x *= 2;
				y *= 2;
				z *= 2;
				amplitude *= persistence;
			}
			return result;
		}

		void Serialize(wi::Archive& archive)
		{
			if (archive.IsReadMode())
			{
				for (auto& x : state)
				{
					archive >> x;
				}
			}
			else
			{
				for (auto& x : state)
				{
					archive << x;
				}
			}
		}
	};

	// Based on: https://www.shadertoy.com/view/MslGD8
	namespace voronoi
	{
		constexpr float dot(XMFLOAT2 a, XMFLOAT2 b)
		{
			return a.x * b.x + a.y * b.y;
		}
		inline XMFLOAT2 fract(XMFLOAT2 p)
		{
			return XMFLOAT2(
				p.x - std::floor(p.x),
				p.y - std::floor(p.y)
			);
		}
		inline XMFLOAT2 floor(XMFLOAT2 p)
		{
			return XMFLOAT2(
				std::floor(p.x),
				std::floor(p.y)
			);
		}

		// Backwards compatibility implementation for XMVectorSin() without FMA instruction:
		inline XMVECTOR XM_CALLCONV FMADD_COMPAT(XMVECTOR a, XMVECTOR b, XMVECTOR c) noexcept { return _mm_add_ps(_mm_mul_ps((a), (b)), (c)); }
		inline XMVECTOR XM_CALLCONV FNMADD_COMPAT(XMVECTOR a, XMVECTOR b, XMVECTOR c) noexcept { return _mm_sub_ps((c), _mm_mul_ps((a), (b))); }
		inline XMVECTOR XM_CALLCONV XMVectorModAngles_COMPAT(FXMVECTOR Angles) noexcept
		{
			// Modulo the range of the given angles such that -XM_PI <= Angles < XM_PI
			XMVECTOR vResult = _mm_mul_ps(Angles, g_XMReciprocalTwoPi);
			// Use the inline function due to complexity for rounding
			vResult = XMVectorRound(vResult);
			return FNMADD_COMPAT(vResult, g_XMTwoPi, Angles);
		}
		inline XMFLOAT2 sin(XMFLOAT2 p)
		{
			XMVECTOR P = XMLoadFloat2(&p);
			//P = XMVectorSin(P);
			{
				// Force the value within the bounds of pi
				XMVECTOR x = XMVectorModAngles_COMPAT(P);

				// Map in [-pi/2,pi/2] with sin(y) = sin(x).
				__m128 sign = _mm_and_ps(x, g_XMNegativeZero);
				__m128 c = _mm_or_ps(g_XMPi, sign);  // pi when x >= 0, -pi when x < 0
				__m128 absx = _mm_andnot_ps(sign, x);  // |x|
				__m128 rflx = _mm_sub_ps(c, x);
				__m128 comp = _mm_cmple_ps(absx, g_XMHalfPi);
				__m128 select0 = _mm_and_ps(comp, x);
				__m128 select1 = _mm_andnot_ps(comp, rflx);
				x = _mm_or_ps(select0, select1);

				__m128 x2 = _mm_mul_ps(x, x);

				// Compute polynomial approximation
				const XMVECTOR SC1 = g_XMSinCoefficients1;
				__m128 vConstantsB = XM_PERMUTE_PS(SC1, _MM_SHUFFLE(0, 0, 0, 0));
				const XMVECTOR SC0 = g_XMSinCoefficients0;
				__m128 vConstants = XM_PERMUTE_PS(SC0, _MM_SHUFFLE(3, 3, 3, 3));
				__m128 Result = FMADD_COMPAT(vConstantsB, x2, vConstants);

				vConstants = XM_PERMUTE_PS(SC0, _MM_SHUFFLE(2, 2, 2, 2));
				Result = FMADD_COMPAT(Result, x2, vConstants);

				vConstants = XM_PERMUTE_PS(SC0, _MM_SHUFFLE(1, 1, 1, 1));
				Result = FMADD_COMPAT(Result, x2, vConstants);

				vConstants = XM_PERMUTE_PS(SC0, _MM_SHUFFLE(0, 0, 0, 0));
				Result = FMADD_COMPAT(Result, x2, vConstants);

				Result = FMADD_COMPAT(Result, x2, g_XMOne);
				Result = _mm_mul_ps(Result, x);

				P = Result;
			}
			XMFLOAT2 ret;
			XMStoreFloat2(&ret, P);
			return ret;
		}

		inline XMFLOAT2 hash(XMFLOAT2 p)
		{
			XMFLOAT2 ret = XMFLOAT2(
				dot(XMFLOAT2(p.x, p.y), XMFLOAT2(127.1f, 311.7f)),
				dot(XMFLOAT2(p.x, p.y), XMFLOAT2(269.5f, 183.3f))
			);
			ret = sin(ret);
			return fract(XMFLOAT2(ret.x * 18.5453f, ret.y * 18.5453f));
		}
		struct Result
		{
			float distance = 0;
			float cell_id = 0;
		};
		inline Result compute(float x, float y, float seed)
		{
			Result result;

			XMFLOAT2 p = XMFLOAT2(x, y);
			XMFLOAT2 n = floor(p);
			XMFLOAT2 f = fract(p);

			XMFLOAT3 m = XMFLOAT3(8, 0, 0);
			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					XMFLOAT2 g = XMFLOAT2(float(i), float(j));
					XMFLOAT2 o = hash(XMFLOAT2(n.x + g.x, n.y + g.y));
					XMFLOAT2 r;
					r.x = g.x - f.x + (0.5f + 0.5f * std::sin(seed * o.x));
					r.y = g.y - f.y + (0.5f + 0.5f * std::sin(seed * o.y));
					float d = dot(r, r);
					if (d < m.x)
					{
						m = XMFLOAT3(d, o.x, o.y);
					}
				}
			}

			result.distance = std::sqrt(m.x);
			result.cell_id = m.y + m.z;

			return result;
		}
	};
}
