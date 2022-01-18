#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"

PUSHCONSTANT(push, DDGIPushConstants);

StructuredBuffer<DDGIRayData> ddgiRayBuffer : register(t0);

static const uint CACHE_SIZE = 64;
groupshared DDGIRayData ray_cache[CACHE_SIZE];

static const float WEIGHT_EPSILON = 0.0001;

#ifdef DDGI_UPDATE_DEPTH
static const uint THREADCOUNT = DDGI_DEPTH_RESOLUTION;
RWTexture2D<float2> output : register(u0);
#else
static const uint THREADCOUNT = DDGI_COLOR_RESOLUTION;
RWTexture2D<float3> output : register(u0);
#endif // DDGI_UPDATE_DEPTH

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint probeIndex = Gid.x;
	const uint3 probeCoord = ddgi_probe_coord(probeIndex);
	const float maxDistance = length(ddgi_cellsize()) * 1.5;

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
			ray_cache[groupIndex] = ddgiRayBuffer[probeIndex * DDGI_MAX_RAYCOUNT + groupIndex + offset];
		}

		GroupMemoryBarrierWithGroupSync();

		const float energy_conservation = 0.95;
		for (uint r = 0; r < num_rays; ++r)
		{
			DDGIRayData ray = ray_cache[r];

#ifdef DDGI_UPDATE_DEPTH
			float ray_probe_distance;
			if (ray.depth >= 0)
			{
				ray_probe_distance = min(maxDistance, ray.depth - 0.01);
			}
			else
			{
				ray_probe_distance = maxDistance;
			}
#else
			const float3 radiance = ray.radiance.rgb * energy_conservation;
#endif // DDGI_UPDATE_DEPTH

			float weight = max(0, dot(texel_direction, ray.direction));
#ifdef DDGI_UPDATE_DEPTH
			weight = pow(weight, 2);
#endif // DDGI_UPDATE_DEPTH

			if (weight > WEIGHT_EPSILON)
			{
#ifdef DDGI_UPDATE_DEPTH
				result += float2(ray.depth * weight, sqr(ray.depth) * weight);
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
	const float2 prev_result = bindless_textures[GetScene().ddgi_depth_texture][pixel_current].xy;
#else
	const float3 prev_result = bindless_textures[GetScene().ddgi_color_texture][pixel_current].rgb;
#endif // DDGI_UPDATE_DEPTH

	if (push.frameIndex > 0)
	{
		result = lerp(prev_result, result, 0.02);
	}

	output[pixel_current] = result;

#ifdef DDGI_UPDATE_DEPTH
	// Copy depth borders:
	for (uint index = groupIndex; index < 68; index += THREADCOUNT * THREADCOUNT)
	{
		uint2 src_coord = copy_coord + DDGI_DEPTH_BORDER_OFFSETS[index].xy;
		uint2 dst_coord = copy_coord + DDGI_DEPTH_BORDER_OFFSETS[index].zw;
		output[dst_coord] = output[src_coord];
	}
#else
	// Copy color borders:
	for (uint index = groupIndex; index < 36; index += THREADCOUNT * THREADCOUNT)
	{
		uint2 src_coord = copy_coord + DDGI_COLOR_BORDER_OFFSETS[index].xy;
		uint2 dst_coord = copy_coord + DDGI_COLOR_BORDER_OFFSETS[index].zw;
		output[dst_coord] = output[src_coord];
	}
#endif // DDGI_UPDATE_DEPTH
}
