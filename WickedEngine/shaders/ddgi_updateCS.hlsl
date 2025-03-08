#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"
#include "bc6h.hlsli"

// This shader collects all traced rays (one probe per thread group) and integrates them
//	Rays are first gathered to shared memory
//	Then for each pixel, all traced rays will be evaluated and contributed, weighted based on pixel's own direction and ray's direction
//	This shader will run twice in DDGI, once it integrates the radiances
//	After that it will also integrate the ray depths, when DDGI_UPDATE_DEPTH is defined
//	Based on: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_probe_update.glsl

PUSHCONSTANT(push, DDGIPushConstants);

StructuredBuffer<DDGIRayDataPacked> ddgiRayBuffer : register(t0);
Buffer<uint> ddgiRayCountBuffer : register(t1);

static const float WEIGHT_EPSILON = 0.0001;

#ifdef DDGI_UPDATE_DEPTH
static const uint THREADCOUNT = DDGI_DEPTH_RESOLUTION;
static const uint RESOLUTION = DDGI_DEPTH_RESOLUTION;
RWTexture2D<float2> output : register(u0);
groupshared uint shared_depths[DDGI_DEPTH_RESOLUTION * DDGI_DEPTH_RESOLUTION];
#else
static const uint THREADCOUNT = DDGI_COLOR_RESOLUTION;
static const uint RESOLUTION = DDGI_COLOR_RESOLUTION;
RWStructuredBuffer<DDGIVarianceDataPacked> varianceBuffer : register(u0);
groupshared uint shared_texels[DDGI_COLOR_TEXELS * DDGI_COLOR_TEXELS];
#endif // DDGI_UPDATE_DEPTH

RWStructuredBuffer<DDGIProbe> ddgiProbeBuffer : register(u1);

static const uint CACHE_SIZE = THREADCOUNT * THREADCOUNT;
groupshared DDGIRayData ray_cache[CACHE_SIZE];

static const int3 voxel_neighbors[] = {
	// First priority is direct straight neighbors:
	int3(-1, 0, 0),
	int3(1, 0, 0),
	int3(0, -1, 0),
	int3(0, 1, 0),
	int3(0, 0, -1),
	int3(0, 0, 1),

	// Second priority is diagonal neighbors:
	int3(-1, 0, -1),
	int3(1, 0, -1),
	int3(-1, 0, 1),
	int3(1, 0, 1),
	int3(-1, -1, -1),
	int3(1, -1, -1),
	int3(-1, -1, 1),
	int3(1, -1, 1),
	int3(-1, 1, -1),
	int3(1, 1, -1),
	int3(-1, 1, 1),
	int3(1, 1, 1),
};
int3 get_nearby_empty_voxel(ShaderVoxelGrid voxelgrid, uint3 coord)
{
	for(uint i = 0; i < arraysize(voxel_neighbors); ++i)
	{
		if(!voxelgrid.check_voxel(coord + voxel_neighbors[i]))
			return voxel_neighbors[i];
	}
	return 0;
}

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint2 GTid : SV_GroupThreadID, uint2 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint probeIndex = Gid.x;
	const uint3 probeCoord = ddgi_probe_coord(probeIndex);
	const float maxDistance = ddgi_max_distance();

#ifdef DDGI_UPDATE_DEPTH
	[branch]
	if (groupIndex == 0 && push.frameIndex == 0)
	{
		ddgiProbeBuffer[probeIndex].offset = 0;
	}
	const float3 probe_limit = ddgi_cellsize() * 0.5;
	float3 probeOffsetNew = 0;
	const float probeOffsetDistance = maxDistance * DDGI_KEEP_DISTANCE;
#endif // DDGI_UPDATE_DEPTH

#ifdef DDGI_UPDATE_DEPTH
	float2 result = 0;
	const uint2 pixel_topleft = ddgi_probe_depth_pixel(probeCoord);
	const uint2 pixel_current = pixel_topleft + GTid.xy;
	const uint2 copy_coord = pixel_topleft - 1;
#else
	float3 result = 0;
#endif // DDGI_UPDATE_DEPTH

	const float3 texel_direction = decode_oct((((GTid.xy % RESOLUTION) + 0.5) / (float2)RESOLUTION) * 2 - 1);

	float total_weight = 0;

	uint remaining_rays = min(ddgiRayCountBuffer[probeIndex] * DDGI_RAY_BUCKET_COUNT, DDGI_MAX_RAYCOUNT);
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
	const float2 prev_result = output[pixel_current].xy;
	if (push.frameIndex > 0)
	{
		result = lerp(prev_result, result, 0.02);
	}
	shared_depths[flatten2D(GTid, DDGI_DEPTH_RESOLUTION)] = pack_half2(result);
	output[pixel_current] = result;

	GroupMemoryBarrierWithGroupSync();

	// Copy depth borders:
	for (uint index = groupIndex; index < arraysize(DDGI_DEPTH_BORDER_OFFSETS); index += THREADCOUNT * THREADCOUNT)
	{
		uint src_coord = flatten2D(DDGI_DEPTH_BORDER_OFFSETS[index].xy - 1, DDGI_DEPTH_RESOLUTION);
		uint2 dst_coord = copy_coord + DDGI_DEPTH_BORDER_OFFSETS[index].zw;
		output[dst_coord] = unpack_half2(shared_depths[src_coord]);
	}

	[branch]
	if (groupIndex == 0)
	{
		[branch]
		if(GetScene().voxelgrid.IsValid())
		{
			// If there is a voxel grid, that can help offset the probes when they are stuck in closed spaces:
			ShaderVoxelGrid voxelgrid = GetScene().voxelgrid;
			uint3 coord = voxelgrid.world_to_coord(ddgi_probe_position_rest(probeCoord));
			if(voxelgrid.check_voxel(coord))
			{
				probeOffsetNew += get_nearby_empty_voxel(voxelgrid, coord) * voxelgrid.voxelSize * 2;
			}
		}
		
		float3 probeOffset = unpack_half3(ddgiProbeBuffer[probeIndex].offset);
		probeOffset *= probe_limit;
		probeOffset = lerp(probeOffset, probeOffsetNew, 0.01);
		probeOffset = clamp(probeOffset, -probe_limit, probe_limit);
		probeOffset /= probe_limit;
		ddgiProbeBuffer[probeIndex].offset = pack_half3(probeOffset);
	}
#else

	if (GTid.x < DDGI_COLOR_RESOLUTION && GTid.y < DDGI_COLOR_RESOLUTION)
	{
		const uint idx = flatten2D(GTid.xy, DDGI_COLOR_RESOLUTION);
		const uint variance_data_index = probeIndex * DDGI_COLOR_RESOLUTION * DDGI_COLOR_RESOLUTION + idx;
		DDGIVarianceData varianceData = varianceBuffer[variance_data_index].load();
		if (push.frameIndex == 0)
		{
			varianceData = (DDGIVarianceData)0;
			varianceData.mean = result;
			varianceData.shortMean = result;
			varianceData.inconsistency = 1;
		}
		MultiscaleMeanEstimator(result, varianceData, push.blendSpeed);
		varianceBuffer[variance_data_index].store(varianceData);
		result = varianceData.mean;

		shared_texels[flatten2D(GTid, DDGI_COLOR_TEXELS)] = PackRGBE(result);
	}

	GroupMemoryBarrierWithGroupSync();

	if (groupIndex == 0)
	{
		SH::L1_RGB radiance = SH::L1_RGB::Zero();
		for (int x = 0; x < RESOLUTION; ++x)
		{
			for (int y = 0; y < RESOLUTION; ++y)
			{
				const float3 direction = decode_oct(((float2(x, y) + 0.5) / (float2) RESOLUTION) * 2 - 1);
				float3 value = UnpackRGBE(shared_texels[flatten2D(int2(x, y), DDGI_COLOR_TEXELS)]);
				radiance = SH::Add(radiance, SH::ProjectOntoL1_RGB(direction, value));
			}
		}
		radiance = SH::Multiply(radiance, rcp(RESOLUTION * RESOLUTION * SPHERE_SAMPLING_PDF));
		ddgiProbeBuffer[probeIndex].radiance = SH::Pack(radiance);
		
		//draw_line(ddgi_probe_position(probeCoord), ddgi_probe_position(probeCoord) + OptimalLinearDirection(radiance));
	}
	
#endif // DDGI_UPDATE_DEPTH
}
