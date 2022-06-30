#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_SurfelGI.h"

static const float WEIGHT_EPSILON = 0.0001;

StructuredBuffer<Surfel> surfelBuffer : register(t0);
ByteAddressBuffer surfelStatsBuffer : register(t1);
StructuredBuffer<SurfelGridCell> surfelGridBuffer : register(t2);
StructuredBuffer<uint> surfelCellBuffer : register(t3);
StructuredBuffer<uint> surfelAliveBuffer : register(t4);
Texture2D<float2> surfelMomentsTexturePrev : register(t5);
StructuredBuffer<SurfelRayDataPacked> surfelRayBuffer : register(t6);

RWStructuredBuffer<SurfelData> surfelDataBuffer : register(u0);
RWTexture2D<float2> surfelMomentsTexture : register(u1);

static const uint THREADCOUNT = 8;
static const uint CACHE_SIZE = THREADCOUNT * THREADCOUNT;
groupshared SurfelRayData ray_cache[CACHE_SIZE];
groupshared float4 result_cache[CACHE_SIZE];

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	uint surfel_index = surfelAliveBuffer[Gid.x];
	Surfel surfel = surfelBuffer[surfel_index];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];
	uint life = surfel_data.GetLife();
	uint recycle = surfel_data.GetRecycle();
	float maxDistance = surfel.GetRadius();

	const float3 P = surfel.position;
	const float3 N = normalize(unpack_unitvector(surfel.normal));

	float3 texel_direction = decode_hemioct(((GTid.xy + 0.5) / (float2)SURFEL_MOMENT_RESOLUTION) * 2 - 1);
	texel_direction = mul(texel_direction, get_tangentspace(N));
	texel_direction = normalize(texel_direction);

	float4 result = 0;
	float2 result_depth = 0;
	float total_weight = 0;

	uint remaining_rays = surfel.GetRayCount();
	uint offset = surfel.GetRayOffset();
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
			result += float4(ray.radiance, 1);

			float depth;
			if (ray.depth > 0)
			{
				depth = clamp(ray.depth, 0, maxDistance);
			}
			else
			{
				depth = maxDistance;
			}
			const float3 radiance = ray.radiance.rgb;

			float weight = saturate(dot(texel_direction, ray.direction) + 0.01);
			weight = pow(weight, 32);

			if (weight > WEIGHT_EPSILON)
			{
				result_depth += float2(depth, sqr(depth)) * weight;
				total_weight += weight;
			}
		}

		GroupMemoryBarrierWithGroupSync();

		remaining_rays -= num_rays;
		offset += num_rays;
	}

	uint2 moments_topleft = unflatten2D(surfel_index, SQRT_SURFEL_CAPACITY) * SURFEL_MOMENT_TEXELS;
	if (total_weight > WEIGHT_EPSILON && GTid.x < SURFEL_MOMENT_RESOLUTION && GTid.y < SURFEL_MOMENT_RESOLUTION)
	{
		result_depth /= total_weight;

		uint2 moments_pixel = moments_topleft + 1 + GTid.xy;
		if (life > 0)
		{
			const float2 prev_moment = surfelMomentsTexturePrev[moments_pixel];
			result_depth = lerp(prev_moment, result_depth, 0.02);
		}
		surfelMomentsTexture[moments_pixel] = result_depth;
	}


#ifdef SURFEL_ENABLE_IRRADIANCE_SHARING
	// Surfel irradiance sharing:
	{
		uint cellindex = surfel_cellindex(surfel_cell(P));
		SurfelGridCell cell = surfelGridBuffer[cellindex];
		for (uint i = 0; i < cell.count; i += THREADCOUNT * THREADCOUNT)
		{
			uint surfel_index = surfelCellBuffer[cell.offset + i];
			Surfel surfel = surfelBuffer[surfel_index];
			const float combined_radius = surfel.GetRadius() + maxDistance;

			float3 L = P - surfel.position;
			float dist2 = dot(L, L);
			if (dist2 < sqr(combined_radius))
			{
				float3 normal = normalize(unpack_unitvector(surfel.normal));
				float dotN = dot(N, normal);
				if (dotN > 0)
				{
					float dist = sqrt(dist2);
					float contribution = 1;

					contribution *= saturate(dotN);
					contribution *= saturate(1 - dist / combined_radius);
					contribution = smoothstep(0, 1, contribution);

					float2 moments = surfelMomentsTexturePrev.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, L / dist), 0);
					contribution *= surfel_moment_weight(moments, dist);

					result += float4(surfel.color, 1) * contribution;

				}
			}
		}
	}
	result_cache[groupIndex] = result;
#endif // SURFEL_ENABLE_IRRADIANCE_SHARING

	AllMemoryBarrierWithGroupSync();

	// Copy moment borders:
	for (uint i = GTid.x; i < SURFEL_MOMENT_TEXELS; i += THREADCOUNT)
	{
		for (uint j = GTid.y; j < SURFEL_MOMENT_TEXELS; j += THREADCOUNT)
		{
			uint2 pixel_write = moments_topleft + uint2(i, j);
			uint2 pixel_read = clamp(pixel_write, moments_topleft + 1, moments_topleft + 1 + SURFEL_MOMENT_RESOLUTION - 1);
			surfelMomentsTexture[pixel_write] = surfelMomentsTexture[pixel_read];
		}
	}

	if (groupIndex > 0)
		return;

#ifdef SURFEL_ENABLE_IRRADIANCE_SHARING
	result = 0;
	for (uint c = 0; c < CACHE_SIZE; ++c)
	{
		result += result_cache[c];
	}
#endif // SURFEL_ENABLE_IRRADIANCE_SHARING

	if (result.a > 0)
	{
		result /= result.a;
		MultiscaleMeanEstimator(result.rgb, surfel_data, 0.08);
	}

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

	surfel_data.life_recycle = 0;
	surfel_data.life_recycle |= life & 0xFFFF;
	surfel_data.life_recycle |= (recycle & 0xFFFF) << 16u;

	surfelDataBuffer[surfel_index] = surfel_data;
}
