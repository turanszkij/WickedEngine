#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "bc6h.hlsli"

static const float WEIGHT_EPSILON = 0.0001;

StructuredBuffer<Surfel> surfelBuffer : register(t0);
ByteAddressBuffer surfelStatsBuffer : register(t1);
StructuredBuffer<SurfelGridCell> surfelGridBuffer : register(t2);
StructuredBuffer<uint> surfelCellBuffer : register(t3);
StructuredBuffer<uint> surfelAliveBuffer : register(t4);
StructuredBuffer<SurfelRayDataPacked> surfelRayBuffer : register(t5);

RWStructuredBuffer<SurfelData> surfelDataBuffer : register(u0);
RWStructuredBuffer<SurfelVarianceDataPacked> surfelVarianceBuffer : register(u1);
RWTexture2D<float2> surfelMomentsTexture : register(u2);
RWTexture2D<uint4> surfelIrradianceTexture : register(u3); // aliased BC6H

static const uint THREADCOUNT = 8;
static const uint CACHE_SIZE = THREADCOUNT * THREADCOUNT;
groupshared SurfelRayData ray_cache[CACHE_SIZE];
groupshared float3 shared_texels[16];
groupshared float shared_inconsistency[CACHE_SIZE];

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	const uint surfel_index = surfelAliveBuffer[Gid.x];
	Surfel surfel = surfelBuffer[surfel_index];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];
	uint life = surfel_data.GetLife();
	uint recycle = surfel_data.GetRecycle();
	bool backface = surfel_data.IsBackfaceNormal();
	float maxDistance = surfel.GetRadius();

	const float3 P = surfel.position;
	const float3 N = normalize(unpack_unitvector(surfel.normal));

	float3 texel_direction = decode_hemioct(((GTid.xy + 0.5) / (float2)SURFEL_MOMENT_RESOLUTION) * 2 - 1);
	texel_direction = mul(texel_direction, get_tangentspace(N));
	texel_direction = normalize(texel_direction);

	float3 result = 0;
	float2 result_depth = 0;
	float total_weight = 0;
	float total_depth_weight = 0;

	uint remaining_rays = surfel_data.GetRayCount();
	uint offset = surfel_data.GetRayOffset();
	while (remaining_rays > 0)
	{
		uint num_rays = min(CACHE_SIZE, remaining_rays);

		if (groupIndex < num_rays)
		{
			ray_cache[groupIndex] = surfelRayBuffer[offset + groupIndex].load();
		}

		GroupMemoryBarrierWithGroupSync();

		for (uint r = 0; r < num_rays; ++r)
		{
			SurfelRayData ray = ray_cache[r];

			float depth;
			if (ray.depth > 0)
			{
				depth = clamp(ray.depth, 0, maxDistance);
			}
			else
			{
				depth = maxDistance;
			}

			float weight = saturate(dot(texel_direction, ray.direction) + 0.01);
			if (weight > WEIGHT_EPSILON)
			{
				result += ray.radiance.rgb * weight;
				total_weight += weight;
			}
			
			float depth_weight = pow(weight, 32);
			if (depth_weight > WEIGHT_EPSILON)
			{
				result_depth += float2(depth, sqr(depth)) * depth_weight;
				total_depth_weight += depth_weight;
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
	
	if (total_depth_weight > WEIGHT_EPSILON)
	{
		result_depth /= total_depth_weight;
	}
	
	const uint2 moments_topleft = unflatten2D(surfel_index, SQRT_SURFEL_CAPACITY) * SURFEL_MOMENT_RESOLUTION;

	float inconsistency = 0;
	
	if(GTid.x < SURFEL_MOMENT_RESOLUTION && GTid.y < SURFEL_MOMENT_RESOLUTION)
	{
		uint2 moments_pixel = moments_topleft + GTid.xy;
		if (life > 0)
		{
			const float2 prev_moment = surfelMomentsTexture[moments_pixel];
			result_depth = lerp(prev_moment, result_depth, 0.02);
		}
	
		surfelMomentsTexture[moments_pixel] = result_depth;

		const uint idx = flatten2D(GTid.xy, SURFEL_MOMENT_RESOLUTION);
		const uint variance_data_index = surfel_index * SURFEL_MOMENT_RESOLUTION * SURFEL_MOMENT_RESOLUTION + idx;
		SurfelVarianceData varianceData = surfelVarianceBuffer[variance_data_index].load();
		if (life == 0)
		{
			varianceData = (SurfelVarianceData)0;
			varianceData.mean = result;
			varianceData.shortMean = result;
			varianceData.inconsistency = 1;
		}
		MultiscaleMeanEstimator(result, varianceData, 0.1);
		surfelVarianceBuffer[variance_data_index].store(varianceData);
		result = varianceData.mean;
		inconsistency = varianceData.inconsistency;

		shared_texels[idx] = result;
	}

	const uint lane_count_per_wave = WaveGetLaneCount();
	if(WaveIsFirstLane())
	{
		float wave_max_inconsistency = WaveActiveMax(inconsistency);
		shared_inconsistency[groupIndex / lane_count_per_wave] = wave_max_inconsistency;
	}
	
	GroupMemoryBarrierWithGroupSync();

	// below this, we switch to bigger per-surfel tasks done by just a few select threads:

	if (groupIndex == 0)
	{
		surfelIrradianceTexture[moments_topleft / 4] = CompressBC6H(shared_texels);
	}
	else if(groupIndex == 1)
	{
		life++;

		float3 cam_to_surfel = surfel.position - GetCamera().position;
		if (length(cam_to_surfel) > SURFEL_RECYCLE_DISTANCE)
		{
			ShaderSphere sphere;
			sphere.center = surfel.position;
			sphere.radius = surfel.GetRadius();

			if (GetCamera().frustum.intersects(sphere))
			{
				recycle = 0;
			}
			else
			{
				recycle++;
			}
		}
		else
		{
			recycle = 0;
		}

		life = clamp(life, 0, 255);
		recycle = clamp(recycle, 0, 255);

		surfel_data.properties = 0;
		surfel_data.SetLife(life);
		surfel_data.SetRecycle(recycle);
		surfel_data.SetBackfaceNormal(backface);
		
		const uint wave_count_per_group = THREADCOUNT * THREADCOUNT / lane_count_per_wave;
		surfel_data.max_inconsistency = inconsistency;
		for(uint i = 0; i < wave_count_per_group; ++i)
		{
			surfel_data.max_inconsistency = max(surfel_data.max_inconsistency, shared_inconsistency[i]);
		}

		surfelDataBuffer[surfel_index] = surfel_data;
	}
}
