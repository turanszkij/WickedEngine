#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"

// This shader collects all traced rays (one probe per thread group) and integrates them
//	Rays are first gathered to shared memory
//	Then for each pixel, all traced rays will be evaluated and contributed, weighted based on pixel's own direction and ray's direction
//	This shader will run twice in DDGI, once it integrates the radiances
//	After that it will also integrate the ray depths, when DDGI_UPDATE_DEPTH is defined
//	Based on: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_probe_update.glsl

PUSHCONSTANT(push, DDGIPushConstants);

StructuredBuffer<DDGIRayDataPacked> ddgiRayBuffer : register(t0);

static const float WEIGHT_EPSILON = 0.0001;

#ifdef DDGI_UPDATE_DEPTH
static const uint THREADCOUNT = DDGI_DEPTH_RESOLUTION;
RWTexture2D<float2> output : register(u0);
RWByteAddressBuffer ddgiOffsetBuffer:register(u1);
#else
static const uint THREADCOUNT = DDGI_COLOR_RESOLUTION;
RWTexture2D<uint> output : register(u0);	// raw uint alias for Format::R9G9B9E5_SHAREDEXP
#endif // DDGI_UPDATE_DEPTH

static const uint CACHE_SIZE = THREADCOUNT * THREADCOUNT;
groupshared DDGIRayData ray_cache[CACHE_SIZE];

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint probeIndex = Gid.x;
	const uint3 probeCoord = ddgi_probe_coord(probeIndex);
	const float maxDistance = ddgi_max_distance();

#ifdef DDGI_UPDATE_DEPTH
	[branch]
	if (groupIndex == 0 && push.frameIndex == 0)
	{
		DDGIProbeOffset ofs;
		ofs.store(float3(0, 0, 0));
#ifdef __PSSL__
		ddgiOffsetBuffer.TypedStore<DDGIProbeOffset>(probeIndex * sizeof(DDGIProbeOffset), ofs);
#else
		ddgiOffsetBuffer.Store<DDGIProbeOffset>(probeIndex * sizeof(DDGIProbeOffset), ofs);
#endif // __PSSL__
	}
	float3 probeOffset = ddgiOffsetBuffer.Load<DDGIProbeOffset>(probeIndex * sizeof(DDGIProbeOffset)).load();
	float3 probeOffsetNew = 0;
	const float probeOffsetDistance = maxDistance * DDGI_KEEP_DISTANCE;
#endif // DDGI_UPDATE_DEPTH

#ifdef DDGI_UPDATE_DEPTH
	float2 result = 0;
	const uint2 pixel_topleft = ddgi_probe_depth_pixel(probeCoord);
#else
	float3 result = 0;
	const uint2 pixel_topleft = ddgi_probe_color_pixel(probeCoord);
#endif // DDGI_UPDATE_DEPTH
	const uint2 pixel_current = pixel_topleft + GTid.xy;
	const uint2 copy_coord = pixel_topleft - 1;

	const float3 texel_direction = decode_oct(((GTid.xy + 0.5) / (float2)THREADCOUNT) * 2 - 1);

	float total_weight = 0;

	uint remaining_rays = push.rayCount;
	uint offset = 0;

	while (remaining_rays > 0)
	{
		uint num_rays = min(CACHE_SIZE, remaining_rays);

		if (groupIndex < num_rays)
		{
			ray_cache[groupIndex] = ddgiRayBuffer[probeIndex * DDGI_MAX_RAYCOUNT + groupIndex + offset].load();
		}

		GroupMemoryBarrierWithGroupSync();

		for (uint r = 0; r < num_rays; ++r)
		{
			DDGIRayData ray = ray_cache[r];

#ifdef DDGI_UPDATE_DEPTH
			float depth;
			if (ray.depth > 0)
			{
				depth = clamp(ray.depth - 0.01, 0, maxDistance);
			}
			else
			{
				depth = maxDistance;
			}

			if (depth < probeOffsetDistance)
			{
				probeOffsetNew -= ray.direction * (probeOffsetDistance - depth);
			}
#else
			const float3 radiance = ray.radiance.rgb;
#endif // DDGI_UPDATE_DEPTH

			float weight = saturate(dot(texel_direction, ray.direction));
#ifdef DDGI_UPDATE_DEPTH
			weight = pow(weight, 64);
#endif // DDGI_UPDATE_DEPTH

			if (weight > WEIGHT_EPSILON)
			{
#ifdef DDGI_UPDATE_DEPTH
				result += float2(depth, sqr(depth)) * weight;
#else
				result += ray.radiance.rgb * weight;
#endif // DDGI_UPDATE_DEPTH

				total_weight += weight;
			}
		}

		GroupMemoryBarrierWithGroupSync();

		remaining_rays -= num_rays;
		offset += num_rays;
	}

	if (total_weight > WEIGHT_EPSILON)
	{
		result /= total_weight;
	}

#ifdef DDGI_UPDATE_DEPTH
	const float2 prev_result = bindless_textures[GetScene().ddgi.depth_texture][pixel_current].xy;
#else
	const float3 prev_result = bindless_textures[GetScene().ddgi.color_texture][pixel_current].rgb;
#endif // DDGI_UPDATE_DEPTH

	if (push.frameIndex > 0)
	{
		result = lerp(prev_result, result, push.blendSpeed);
	}

#ifdef DDGI_UPDATE_DEPTH

	output[pixel_current] = result;

	DeviceMemoryBarrierWithGroupSync();

	// Copy depth borders:
	for (uint index = groupIndex; index < 68; index += THREADCOUNT * THREADCOUNT)
	{
		uint2 src_coord = copy_coord + DDGI_DEPTH_BORDER_OFFSETS[index].xy;
		uint2 dst_coord = copy_coord + DDGI_DEPTH_BORDER_OFFSETS[index].zw;
		output[dst_coord] = output[src_coord];
	}

	[branch]
	if (groupIndex == 0)
	{
		probeOffset = lerp(probeOffset, probeOffsetNew, 0.01);
		const float3 limit = ddgi_cellsize() * 0.5;
		probeOffset = clamp(probeOffset, -limit, limit);
		DDGIProbeOffset ofs;
		ofs.store(probeOffset);
#ifdef __PSSL__
		ddgiOffsetBuffer.TypedStore<DDGIProbeOffset>(probeIndex * sizeof(DDGIProbeOffset), ofs);
#else
		ddgiOffsetBuffer.Store<DDGIProbeOffset>(probeIndex * sizeof(DDGIProbeOffset), ofs);
#endif // __PSSL__
	}
#else

	output[pixel_current] = PackRGBE(result);

	DeviceMemoryBarrierWithGroupSync();

	// Copy color borders:
	for (uint index = groupIndex; index < 36; index += THREADCOUNT * THREADCOUNT)
	{
		uint2 src_coord = copy_coord + DDGI_COLOR_BORDER_OFFSETS[index].xy;
		uint2 dst_coord = copy_coord + DDGI_COLOR_BORDER_OFFSETS[index].zw;
		output[dst_coord] = output[src_coord];
	}
#endif // DDGI_UPDATE_DEPTH
}
