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

		// Backwards compatibility implementation for XMVectorSin() without intrinsics, but strictly matching with SSE version:
		inline float fmadd_compat(float a, float b, float c) noexcept
		{
			float t = a * b;
			return t + c;
		}
		inline float fnmadd_compat(float a, float b, float c) noexcept
		{
			float t = a * b;
			return c - t;
		}
		inline float modangles_compat(float angle) noexcept
		{
			const float ReciprocalTwoPi = 0.159154943f;
			const float TwoPi = 6.283185307f;
			float vResult = angle * ReciprocalTwoPi;
			vResult = std::nearbyint(vResult);
			return fnmadd_compat(vResult, TwoPi, angle);
		}
		inline float compute_sin(float x) noexcept
		{
			constexpr float Pi = 3.141592654f;
			constexpr float HalfPi = 1.570796327f;
			constexpr float One = 1.0f;
			constexpr float Coeff1 = -0.16666667f;
			constexpr float Coeff2 = 0.0083333310f;
			constexpr float Coeff3 = -0.00019840874f;
			constexpr float Coeff4 = 2.7525562e-06f;
			constexpr float Coeff5 = -2.3889859e-08f;
			x = modangles_compat(x);
			float c = std::copysign(Pi, x);
			float absx = std::abs(x);
			float rflx = c - x;
			bool comp = (absx <= HalfPi);
			x = comp ? x : rflx;
			float x2 = x * x;
			float result = fmadd_compat(Coeff5, x2, Coeff4);
			result = fmadd_compat(result, x2, Coeff3);
			result = fmadd_compat(result, x2, Coeff2);
			result = fmadd_compat(result, x2, Coeff1);
			result = fmadd_compat(result, x2, One);
			result *= x;
			return result;
		}
		inline XMFLOAT2 sin(XMFLOAT2 p) noexcept
		{
			XMFLOAT2 ret;
			ret.x = compute_sin(p.x);
			ret.y = compute_sin(p.y);
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
