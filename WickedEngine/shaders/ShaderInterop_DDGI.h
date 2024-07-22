#ifndef WI_SHADERINTEROP_DDGI_H
#define WI_SHADERINTEROP_DDGI_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

static const uint DDGI_MAX_RAYCOUNT = 512; // affects global ray buffer size
static const uint DDGI_COLOR_RESOLUTION = 6; // this should not be modified, border update code is fixed
static const uint DDGI_COLOR_TEXELS = 1 + DDGI_COLOR_RESOLUTION + 1; // with border. NOTE: this must be 4x4 block aligned for BC6!
static const uint DDGI_DEPTH_RESOLUTION = 16; // this should not be modified, border update code is fixed
static const uint DDGI_DEPTH_TEXELS = 1 + DDGI_DEPTH_RESOLUTION + 1; // with border
static const float DDGI_KEEP_DISTANCE = 0.1f; // how much distance should probes keep from surfaces
static const uint DDGI_RAY_BUCKET_COUNT = 4; // ray count per bucket

#define DDGI_LINEAR_BLENDING

struct DDGIPushConstants
{
	uint instanceInclusionMask;
	uint frameIndex;
	uint rayCount;
	float blendSpeed;
};

struct DDGIRayData
{
	float3 direction;
	float depth;
	float4 radiance;
};
struct DDGIRayDataPacked
{
	uint4 data;

#ifndef __cplusplus
	inline void store(DDGIRayData rayData)
	{
		data.xy = pack_half4(float4(rayData.direction, rayData.depth));
		data.zw = pack_half4(rayData.radiance);
	}
	inline DDGIRayData load()
	{
		DDGIRayData rayData;
		float4 unpk = unpack_half4(data.xy);
		rayData.direction = unpk.xyz;
		rayData.depth = unpk.w;
		rayData.radiance = unpack_half4(data.zw);
		return rayData;
	}
#endif // __cplusplus
};

struct DDGIVarianceData
{
	float3 mean;
	float3 shortMean;
	float vbbr;
	float3 variance;
	float inconsistency;
};
struct DDGIVarianceDataPacked
{
	uint4 data;

#ifndef __cplusplus
	inline void store(DDGIVarianceData varianceData)
	{
		data.x = PackRGBE(varianceData.mean);
		data.y = PackRGBE(varianceData.shortMean);
		data.z = PackRGBE(varianceData.variance);
		data.w = pack_half2(float2(varianceData.vbbr, varianceData.inconsistency));
	}
	inline DDGIVarianceData load()
	{
		DDGIVarianceData varianceData;
		varianceData.mean = UnpackRGBE(data.x);
		varianceData.shortMean = UnpackRGBE(data.y);
		varianceData.variance = UnpackRGBE(data.z);
		float2 other = unpack_half2(data.w);
		varianceData.vbbr = other.x;
		varianceData.inconsistency = other.y;
		return varianceData;
	}
#endif // __cplusplus
};

#ifndef __cplusplus
inline float3 ddgi_cellsize()
{
	return GetScene().ddgi.cell_size;
}
inline float3 ddgi_cellsize_rcp()
{
	return GetScene().ddgi.cell_size_rcp;
}
inline float ddgi_max_distance()
{
	return GetScene().ddgi.max_distance;
}
inline uint3 ddgi_base_probe_coord(float3 P)
{
	float3 normalized_pos = (P - GetScene().ddgi.grid_min) * GetScene().ddgi.grid_extents_rcp;
	return floor(normalized_pos * (GetScene().ddgi.grid_dimensions - 1));
}
inline uint3 ddgi_probe_coord(uint probeIndex)
{
	return unflatten3D(probeIndex, GetScene().ddgi.grid_dimensions);
}
inline uint ddgi_probe_index(uint3 probeCoord)
{
	return flatten3D(probeCoord, GetScene().ddgi.grid_dimensions);
}
inline uint3 ddgi_probe_offset_pixel(uint3 probeCoord)
{
	return probeCoord.xzy;
}
inline float3 ddgi_probe_position_rest(uint3 probeCoord)
{
	return GetScene().ddgi.grid_min + probeCoord * ddgi_cellsize();
}
inline float3 ddgi_probe_position(uint3 probeCoord)
{
	float3 pos = ddgi_probe_position_rest(probeCoord);
	[branch]
	if (GetScene().ddgi.offset_texture >= 0)
	{
		float3 offset = bindless_textures3D[GetScene().ddgi.offset_texture][ddgi_probe_offset_pixel(probeCoord)].xyz;
		offset = (offset - 0.5) * ddgi_cellsize();
		pos += offset;
	}
	return pos;
}
inline uint2 ddgi_probe_color_pixel(uint3 probeCoord)
{
	return probeCoord.xz * DDGI_COLOR_TEXELS + uint2(probeCoord.y * GetScene().ddgi.grid_dimensions.x * DDGI_COLOR_TEXELS, 0) + 1;
}
inline float2 ddgi_probe_color_uv(uint3 probeCoord, float3 direction)
{
	float2 pixel = ddgi_probe_color_pixel(probeCoord);
	pixel += (encode_oct(normalize(direction)) * 0.5 + 0.5) * DDGI_COLOR_RESOLUTION;
	return pixel * GetScene().ddgi.color_texture_resolution_rcp;
}
inline uint2 ddgi_probe_depth_pixel(uint3 probeCoord)
{
	return probeCoord.xz * DDGI_DEPTH_TEXELS + uint2(probeCoord.y * GetScene().ddgi.grid_dimensions.x * DDGI_DEPTH_TEXELS, 0) + 1;
}
inline float2 ddgi_probe_depth_uv(uint3 probeCoord, float3 direction)
{
	float2 pixel = ddgi_probe_depth_pixel(probeCoord);
	pixel += (encode_oct(normalize(direction)) * 0.5 + 0.5) * DDGI_DEPTH_RESOLUTION;
	return pixel * GetScene().ddgi.depth_texture_resolution_rcp;
}


// Based on: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_common.glsl
half3 ddgi_sample_irradiance(float3 P, half3 N)
{
	uint3 base_grid_coord = ddgi_base_probe_coord(P);
	float3 base_probe_pos = ddgi_probe_position(base_grid_coord);

	half3 sum_irradiance = 0;
	half sum_weight = 0;

	// alpha is how far from the floor(currentVertex) position. on [0, 1] for each axis.
	half3 alpha = saturate((P - base_probe_pos) * ddgi_cellsize_rcp());

	// Iterate over adjacent probe cage
	for (uint i = 0; i < 8; ++i)
	{
		// Compute the offset grid coord and clamp to the probe grid boundary
		// Offset = 0 or 1 along each axis
		uint3 offset = uint3(i, i >> 1, i >> 2) & 1;
		uint3 probe_grid_coord = clamp(base_grid_coord + offset, 0u.xxx, GetScene().ddgi.grid_dimensions - 1);
		//int p = ddgi_probe_index(probe_grid_coord);

		// Make cosine falloff in tangent plane with respect to the angle from the surface to the probe so that we never
		// test a probe that is *behind* the surface.
		// It doesn't have to be cosine, but that is efficient to compute and we must clip to the tangent plane.
		float3 probe_pos = ddgi_probe_position(probe_grid_coord);

		// Bias the position at which visibility is computed; this
		// avoids performing a shadow test *at* a surface, which is a
		// dangerous location because that is exactly the line between
		// shadowed and unshadowed. If the normal bias is too small,
		// there will be light and dark leaks. If it is too large,
		// then samples can pass through thin occluders to the other
		// side (this can only happen if there are MULTIPLE occluders
		// near each other, a wall surface won't pass through itself.)
		half3 probe_to_point = P - probe_pos + N * 0.001;
		half3 dir = normalize(-probe_to_point);

		// Compute the trilinear weights based on the grid cell vertex to smoothly
		// transition between probes. Avoid ever going entirely to zero because that
		// will cause problems at the border probes. This isn't really a lerp. 
		// We're using 1-a when offset = 0 and a when offset = 1.
		half3 trilinear = lerp(1.0 - alpha, alpha, offset);
		half weight = 1.0;

		// Clamp all of the multiplies. We can't let the weight go to zero because then it would be 
		// possible for *all* weights to be equally low and get normalized
		// up to 1/n. We want to distinguish between weights that are 
		// low because of different factors.

		// Smooth backface test
		{
			// Computed without the biasing applied to the "dir" variable. 
			// This test can cause reflection-map looking errors in the image
			// (stuff looks shiny) if the transition is poor.
			half3 true_direction_to_probe = normalize(probe_pos - P);

			// The naive soft backface weight would ignore a probe when
			// it is behind the surface. That's good for walls. But for small details inside of a
			// room, the normals on the details might rule out all of the probes that have mutual
			// visibility to the point. So, we instead use a "wrap shading" test below inspired by
			// NPR work.
			// weight *= max(0.0001, dot(trueDirectionToProbe, wsN));

			// The small offset at the end reduces the "going to zero" impact
			// where this is really close to exactly opposite
			weight *= lerp(saturate(dot(dir, N)), sqr(max(0.0001, (dot(true_direction_to_probe, N) + 1.0) * 0.5)) + 0.2, GetScene().ddgi.smooth_backface);
		}

		// Moment visibility test
#if 1
		[branch]
		if(GetScene().ddgi.depth_texture >= 0)
		{
			//float2 tex_coord = texture_coord_from_direction(-dir, p, ddgi.depth_texture_width, ddgi.depth_texture_height, ddgi.depth_probe_side_length);
			float2 tex_coord = ddgi_probe_depth_uv(probe_grid_coord, -dir);

			half dist_to_probe = length(probe_to_point);

			//float2 temp = textureLod(depth_texture, tex_coord, 0.0f).rg;
			half2 temp = bindless_textures[GetScene().ddgi.depth_texture].SampleLevel(sampler_linear_clamp, tex_coord, 0).xy;
			half mean = temp.x;
			half variance = abs(sqr(temp.x) - temp.y);

			// http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
			// Need the max in the denominator because biasing can cause a negative displacement
			half chebyshev_weight = variance / (variance + sqr(max(dist_to_probe - mean, 0.0)));

			// Increase contrast in the weight 
			chebyshev_weight = max(pow(chebyshev_weight, 3), 0.0);

			weight *= (dist_to_probe <= mean) ? 1.0 : chebyshev_weight;
		}
#endif

		// Avoid zero weight
		weight = max(0.01, weight);

		half3 irradiance_dir = N;

		//float2 tex_coord = texture_coord_from_direction(normalize(irradiance_dir), p, ddgi.irradiance_texture_width, ddgi.irradiance_texture_height, ddgi.irradiance_probe_side_length);
		float2 tex_coord = ddgi_probe_color_uv(probe_grid_coord, irradiance_dir);

		//float3 probe_irradiance = textureLod(irradiance_texture, tex_coord, 0.0f).rgb;
		half3 probe_irradiance = bindless_textures[GetScene().ddgi.color_texture].SampleLevel(sampler_linear_clamp, tex_coord, 0).rgb;

		// A tiny bit of light is really visible due to log perception, so
		// crush tiny weights but keep the curve continuous. This must be done
		// before the trilinear weights, because those should be preserved.
		const half crush_threshold = 0.2;
		if (weight < crush_threshold)
			weight *= weight * weight / sqr(crush_threshold);

		// Trilinear weights
		weight *= trilinear.x * trilinear.y * trilinear.z;

		// Weight in a more-perceptual brightness space instead of radiance space.
		// This softens the transitions between probes with respect to translation.
		// It makes little difference most of the time, but when there are radical transitions
		// between probes this helps soften the ramp.
#ifndef DDGI_LINEAR_BLENDING
		probe_irradiance = sqrt(probe_irradiance);
#endif

		sum_irradiance += weight * probe_irradiance;
		sum_weight += weight;
	}

	if (sum_weight > 0)
	{
		half3 net_irradiance = sum_irradiance / sum_weight;

		// Go back to linear irradiance
#ifndef DDGI_LINEAR_BLENDING
		net_irradiance = sqr(net_irradiance);
#endif

		//net_irradiance *= 0.85; // energy preservation

		return net_irradiance;
		//return 0.5f * PI * net_irradiance;
	}

	return 0;
}

static const uint4 DDGI_COLOR_BORDER_OFFSETS[] = {
	uint4(6, 1, 1, 0),
	uint4(5, 1, 2, 0),
	uint4(4, 1, 3, 0),
	uint4(3, 1, 4, 0),
	uint4(2, 1, 5, 0),
	uint4(1, 1, 6, 0),

	uint4(6, 6, 1, 7),
	uint4(5, 6, 2, 7),
	uint4(4, 6, 3, 7),
	uint4(3, 6, 4, 7),
	uint4(2, 6, 5, 7),
	uint4(1, 6, 6, 7),

	uint4(1, 1, 0, 6),
	uint4(1, 2, 0, 5),
	uint4(1, 3, 0, 4),
	uint4(1, 4, 0, 3),
	uint4(1, 5, 0, 2),
	uint4(1, 6, 0, 1),

	uint4(6, 1, 7, 6),
	uint4(6, 2, 7, 5),
	uint4(6, 3, 7, 4),
	uint4(6, 4, 7, 3),
	uint4(6, 5, 7, 2),
	uint4(6, 6, 7, 1),

	uint4(1, 1, 7, 7),
	uint4(6, 1, 0, 7),
	uint4(1, 6, 7, 0),
	uint4(6, 6, 0, 0),
};
static const uint4 DDGI_DEPTH_BORDER_OFFSETS[68] = {
	uint4(16, 1, 1, 0),
	uint4(15, 1, 2, 0),
	uint4(14, 1, 3, 0),
	uint4(13, 1, 4, 0),
	uint4(12, 1, 5, 0),
	uint4(11, 1, 6, 0),
	uint4(10, 1, 7, 0),
	uint4(9, 1, 8, 0),
	uint4(8, 1, 9, 0),
	uint4(7, 1, 10, 0),
	uint4(6, 1, 11, 0),
	uint4(5, 1, 12, 0),
	uint4(4, 1, 13, 0),
	uint4(3, 1, 14, 0),
	uint4(2, 1, 15, 0),
	uint4(1, 1, 16, 0),
	uint4(16, 16, 1, 17),
	uint4(15, 16, 2, 17),
	uint4(14, 16, 3, 17),
	uint4(13, 16, 4, 17),
	uint4(12, 16, 5, 17),
	uint4(11, 16, 6, 17),
	uint4(10, 16, 7, 17),
	uint4(9, 16, 8, 17),
	uint4(8, 16, 9, 17),
	uint4(7, 16, 10, 17),
	uint4(6, 16, 11, 17),
	uint4(5, 16, 12, 17),
	uint4(4, 16, 13, 17),
	uint4(3, 16, 14, 17),
	uint4(2, 16, 15, 17),
	uint4(1, 16, 16, 17),
	uint4(1, 16, 0, 1),
	uint4(1, 15, 0, 2),
	uint4(1, 14, 0, 3),
	uint4(1, 13, 0, 4),
	uint4(1, 12, 0, 5),
	uint4(1, 11, 0, 6),
	uint4(1, 10, 0, 7),
	uint4(1, 9, 0, 8),
	uint4(1, 8, 0, 9),
	uint4(1, 7, 0, 10),
	uint4(1, 6, 0, 11),
	uint4(1, 5, 0, 12),
	uint4(1, 4, 0, 13),
	uint4(1, 3, 0, 14),
	uint4(1, 2, 0, 15),
	uint4(1, 1, 0, 16),
	uint4(16, 16, 17, 1),
	uint4(16, 15, 17, 2),
	uint4(16, 14, 17, 3),
	uint4(16, 13, 17, 4),
	uint4(16, 12, 17, 5),
	uint4(16, 11, 17, 6),
	uint4(16, 10, 17, 7),
	uint4(16, 9, 17, 8),
	uint4(16, 8, 17, 9),
	uint4(16, 7, 17, 10),
	uint4(16, 6, 17, 11),
	uint4(16, 5, 17, 12),
	uint4(16, 4, 17, 13),
	uint4(16, 3, 17, 14),
	uint4(16, 2, 17, 15),
	uint4(16, 1, 17, 16),
	uint4(16, 16, 0, 0),
	uint4(1, 16, 17, 0),
	uint4(16, 1, 0, 17),
	uint4(1, 1, 17, 17)
};

void MultiscaleMeanEstimator(
	float3 y,
	inout DDGIVarianceData data,
	float shortWindowBlend = 0.08f
)
{
	float3 mean = data.mean;
	float3 shortMean = data.shortMean;
	float vbbr = data.vbbr;
	float3 variance = data.variance;
	float inconsistency = data.inconsistency;

	// Suppress fireflies.
	{
		float3 dev = sqrt(max(1e-5, variance));
		float3 highThreshold = 0.1 + shortMean + dev * 8;
		float3 overflow = max(0, y - highThreshold);
		y -= overflow;
	}

	float3 delta = y - shortMean;
	shortMean = lerp(shortMean, y, shortWindowBlend);
	float3 delta2 = y - shortMean;

	// This should be a longer window than shortWindowBlend to avoid bias
	// from the variance getting smaller when the short-term mean does.
	float varianceBlend = shortWindowBlend * 0.5;
	variance = lerp(variance, delta * delta2, varianceBlend);
	float3 dev = sqrt(max(1e-5, variance));

	float3 shortDiff = mean - shortMean;

	float relativeDiff = dot(float3(0.299, 0.587, 0.114),
		abs(shortDiff) / max(1e-5, dev));
	inconsistency = lerp(inconsistency, relativeDiff, 0.08);

	float varianceBasedBlendReduction =
		clamp(dot(float3(0.299, 0.587, 0.114),
			0.5 * shortMean / max(1e-5, dev)), 1.0 / 32, 1);

	float3 catchUpBlend = clamp(smoothstep(0, 1,
		relativeDiff * max(0.02, inconsistency - 0.2)), 1.0 / 256, 1);
	catchUpBlend *= vbbr;

	vbbr = lerp(vbbr, varianceBasedBlendReduction, 0.1);
	mean = lerp(mean, y, saturate(catchUpBlend));

	// Output
	data.mean = mean;
	data.shortMean = shortMean;
	data.vbbr = vbbr;
	data.variance = variance;
	data.inconsistency = inconsistency;
}

#endif // __cplusplus

#endif // WI_SHADERINTEROP_DDGI_H
