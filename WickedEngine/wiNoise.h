#pragma once
#include "CommonInclude.h"
#include "wiMath.h"
#include "wiArchive.h"
#include "wiRandom.h"

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
		inline XMVECTOR fract(XMVECTOR p)
		{
			return p - XMVectorFloor(p);
		}
		inline XMVECTOR hash(XMVECTOR p)
		{
			p = XMVectorSet(
				XMVectorGetX(XMVector2Dot(p, XMVectorSet(127.1f, 311.7f, 0, 0))),
				XMVectorGetX(XMVector2Dot(p, XMVectorSet(269.5f, 183.3f, 0, 0))),
				0,
				0
			);
			return fract(XMVectorSin(p) * 18.5453f);
		}
		struct Result
		{
			float distance;
			float cell_id;
		};
		inline Result compute(float x, float y, float seed)
		{
			Result result = {};

			XMVECTOR p = XMVectorSet(x, y, 0, 0);
			XMVECTOR n = XMVectorFloor(p);
			XMVECTOR f = fract(p);

			XMVECTOR m = XMVectorSet(8, 0, 0, 0);
			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					XMVECTOR g = XMVectorSet(float(i), float(j), 0, 0);
					XMVECTOR o = hash(n + g);
					//XMVECTOR r = g - f + o;
					XMVECTOR r = g - f + (XMVectorReplicate(0.5f) + 0.5f * XMVectorSin(seed * o));
					float d = XMVectorGetX(XMVector2Dot(r, r));
					if (d < XMVectorGetX(m))
					{
						m = XMVectorSet(d, XMVectorGetX(o), XMVectorGetY(o), 0);
					}
				}
			}

			result.distance = XMVectorGetX(XMVectorSqrt(m));
			result.cell_id = XMVectorGetY(m) + XMVectorGetZ(m);

			return result;
		}
	};
}
