#ifndef WI_OCEAN_HF
#define WI_OCEAN_HF
#include "globals.hlsli"

inline bool ocean_has_exclusions(ShaderOcean ocean)
{
	return ocean.exclusion_count != 0 && ocean.exclusion_buffer >= 0;
}

inline float ocean_height_at(ShaderOcean ocean, float2 xz)
{
	Texture2D displacementmap = bindless_textures[descriptor_index(ocean.texture_displacementmap)];
	return ocean.water_height + displacementmap.SampleLevel(sampler_linear_wrap, xz * ocean.patch_size_rcp, 0).z;
}

inline float3 ocean_exclusion_transform(ShaderOceanExclusion volume, float4 P)
{
	return float3(dot(volume.transform_x, P), dot(volume.transform_y, P), dot(volume.transform_z, P));
}

inline float2 ocean_exclusion_polygon_point(ByteAddressBuffer buffer, ShaderOceanExclusion volume, uint i)
{
	return buffer.Load<float2>(volume.polygon_offset + i * sizeof(float2));
}

inline bool ocean_exclusion_polygon_contains(ByteAddressBuffer buffer, ShaderOceanExclusion volume, float2 P)
{
	// Same half-open even-odd rule as OceanExclusionComponent::ContainsLocal().
	bool inside = false;
	float2 a = ocean_exclusion_polygon_point(buffer, volume, volume.polygon_count - 1);
	[loop]
	for (uint i = 0; i < volume.polygon_count; ++i)
	{
		const float2 b = ocean_exclusion_polygon_point(buffer, volume, i);
		if ((a.y > P.y) != (b.y > P.y) && P.x < (b.x - a.x) * (P.y - a.y) / (b.y - a.y) + a.x)
		{
			inside = !inside;
		}
		a = b;
	}
	return inside;
}

// Membership is independent of sea level: surface clipping and medium queries use the same mask.
inline bool ocean_exclusion_check(float3 P)
{
	const ShaderOcean ocean = GetWeather().ocean;
	[branch]
	if (!ocean_has_exclusions(ocean))
	{
		return false;
	}
	ByteAddressBuffer buffer = bindless_buffers[descriptor_index(ocean.exclusion_buffer)];
	[loop]
	for (uint i = 0; i < ocean.exclusion_count; ++i)
	{
		const ShaderOceanExclusion volume = buffer.Load<ShaderOceanExclusion>(i * sizeof(ShaderOceanExclusion));
		if (any(P < volume.aabb_min) || any(P > volume.aabb_max))
		{
			continue;
		}
		const float3 L = ocean_exclusion_transform(volume, float4(P, 1));
		if (abs(L.y) > 1)
		{
			continue;
		}
		switch (volume.shape)
		{
		case OCEAN_EXCLUSION_SHAPE_BOX:
			if (abs(L.x) <= 1 && abs(L.z) <= 1)
			{
				return true;
			}
			break;
		case OCEAN_EXCLUSION_SHAPE_SPHERE:
			if (dot(L, L) <= 1)
			{
				return true;
			}
			break;
		case OCEAN_EXCLUSION_SHAPE_CYLINDER:
			if (dot(L.xz, L.xz) <= 1)
			{
				return true;
			}
			break;
		case OCEAN_EXCLUSION_SHAPE_POLYGON:
			if (ocean_exclusion_polygon_contains(buffer, volume, L.xz))
			{
				return true;
			}
			break;
		}
	}
	return false;
}

inline bool ocean_exclusion_clip_slab(float O, float D, float lo, float hi, inout float2 interval)
{
	if (D == 0)
	{
		return O >= lo && O <= hi;
	}
	const float a = (lo - O) / D;
	const float b = (hi - O) / D;
	interval = float2(max(interval.x, min(a, b)), min(interval.y, max(a, b)));
	return interval.x < interval.y;
}

// A unit sphere in 3D, or a unit disk extruded along Y if cylinder == true.
inline bool ocean_exclusion_clip_radius(float3 O, float3 D, inout float2 interval)
{
	const float d2 = dot(D, D);
	if (d2 == 0)
	{
		return dot(O, O) <= 1;
	}
	const float t = -dot(O, D) / d2;
	const float3 closest = O + D * t;
	const float r2 = (1 - dot(closest, closest)) / d2;
	if (r2 <= 0)
	{
		return false;
	}
	const float r = sqrt(r2);
	interval = float2(max(interval.x, t - r), min(interval.y, t + r));
	return interval.x < interval.y;
}

inline float ocean_exclusion_cross2(float2 a, float2 b)
{
	return a.x * b.y - a.y * b.x;
}

// Find the first interval inside a (possibly concave) outline on O + t * D.
inline bool ocean_exclusion_polygon_interval(ByteAddressBuffer buffer, ShaderOceanExclusion volume, float2 O, float2 D, inout float2 interval)
{
	if (all(D == 0))
	{
		return ocean_exclusion_polygon_contains(buffer, volume, O);
	}
	const float2 last = ocean_exclusion_polygon_point(buffer, volume, volume.polygon_count - 1) - O;
	float cursor = interval.x;
	[loop]
	while (cursor < interval.y)
	{
		bool inside = false;
		float next = interval.y;
		float2 a = last;
		float side_a = ocean_exclusion_cross2(D, a);
		[loop]
		for (uint i = 0; i < volume.polygon_count; ++i)
		{
			const float2 b = ocean_exclusion_polygon_point(buffer, volume, i) - O;
			const float side_b = ocean_exclusion_cross2(D, b);
			if ((side_a > 0) != (side_b > 0))
			{
				const float t = ocean_exclusion_cross2(a, b) / (side_b - side_a);
				if (t > cursor)
				{
					// Count all crossings ahead (even outside the clipped interval) to
					// determine parity just after cursor. Tangencies don't flip parity.
					inside = !inside;
					next = min(next, t);
				}
			}
			a = b;
			side_a = side_b;
		}
		if (inside)
		{
			interval = float2(cursor, next);
			return true;
		}
		cursor = next;
	}
	return false;
}

inline bool ocean_exclusion_interval(ByteAddressBuffer buffer, ShaderOceanExclusion volume, float3 O, float3 D, inout float2 interval)
{
	// Even rotated/parented shapes first get a cheap world-space slab test.
	if (!ocean_exclusion_clip_slab(O.x, D.x, volume.aabb_min.x, volume.aabb_max.x, interval))
	{
		return false;
	}
	if (!ocean_exclusion_clip_slab(O.y, D.y, volume.aabb_min.y, volume.aabb_max.y, interval))
	{
		return false;
	}
	if (!ocean_exclusion_clip_slab(O.z, D.z, volume.aabb_min.z, volume.aabb_max.z, interval))
	{
		return false;
	}
	const float3 L = ocean_exclusion_transform(volume, float4(O, 1));
	const float3 V = ocean_exclusion_transform(volume, float4(D, 0));
	if (!ocean_exclusion_clip_slab(L.y, V.y, -1, 1, interval))
	{
		return false;
	}
	switch (volume.shape)
	{
	case OCEAN_EXCLUSION_SHAPE_BOX:
		if (!ocean_exclusion_clip_slab(L.x, V.x, -1, 1, interval))
		{
			return false;
		}
		return ocean_exclusion_clip_slab(L.z, V.z, -1, 1, interval);
	case OCEAN_EXCLUSION_SHAPE_SPHERE:
		return ocean_exclusion_clip_radius(L, V, interval);
	case OCEAN_EXCLUSION_SHAPE_CYLINDER:
		return ocean_exclusion_clip_radius(float3(L.x, 0, L.z), float3(V.x, 0, V.z), interval);
	case OCEAN_EXCLUSION_SHAPE_POLYGON:
		return ocean_exclusion_polygon_interval(buffer, volume, L.xz, V.xz, interval);
	}
	return false;
}

// Fraction of a below-sea-level segment occupied by ocean, subtracting the UNION
// of dry intervals. Advancing to the next dry exit handles overlaps, nesting and
// multiple spans of a concave prism without a per-pixel intersection array or a sampling step.
inline float ocean_unexcluded_fraction(float3 O, float3 end)
{
	const ShaderOcean ocean = GetWeather().ocean;
	[branch]
	if (!ocean_has_exclusions(ocean))
	{
		return 1;
	}
	ByteAddressBuffer buffer = bindless_buffers[descriptor_index(ocean.exclusion_buffer)];
	const float3 D = end - O;
	float water = 0;
	float cursor = 0;
	[loop]
	while (cursor < 1)
	{
		float2 dry = 1;
		[loop]
		for (uint i = 0; i < ocean.exclusion_count; ++i)
		{
			const ShaderOceanExclusion volume = buffer.Load<ShaderOceanExclusion>(i * sizeof(ShaderOceanExclusion));
			float2 interval = float2(cursor, 1);
			if (ocean_exclusion_interval(buffer, volume, O, D, interval))
			{
				if (interval.x < dry.x || (interval.x == dry.x && interval.y > dry.y))
				{
					dry = interval;
				}
			}
		}
		water += dry.x - cursor;
		cursor = dry.y;
	}
	return saturate(water);
}

// A dry pocket below an intact surface is still sheltered from rain above it.
inline bool ocean_blocks_rain(float3 P, float height)
{
	if (!(P.y < height))
	{
		return false;
	}
	if (!ocean_exclusion_check(P))
	{
		return true;
	}
	const float3 surface = float3(P.x, height, P.z);
	if (!ocean_exclusion_check(surface))
	{
		return true;
	}
	// Two dry endpoints aren't enough if there is water between them.
	return ocean_unexcluded_fraction(P, surface) > 1e-5;
}

#endif // WI_OCEAN_HF
