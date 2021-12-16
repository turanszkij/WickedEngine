#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_SurfelGI.h"

PUSHCONSTANT(push, PushConstantsSurfelRaytrace);

StructuredBuffer<Surfel> surfelBuffer : register(t0);
ByteAddressBuffer surfelStatsBuffer : register(t1);
StructuredBuffer<SurfelGridCell> surfelGridBuffer : register(t2);
StructuredBuffer<uint> surfelCellBuffer : register(t3);
StructuredBuffer<uint> surfelAliveBuffer : register(t4);
Texture2D<float2> surfelMomentsTexturePrev : register(t5);

RWStructuredBuffer<SurfelData> surfelDataBuffer : register(u0);
RWTexture2D<float2> surfelMomentsTexture : register(u1);

void surfel_moments_write(uint2 moments_pixel, float dist)
{
	float2 prev = surfelMomentsTexture[moments_pixel];
	float2 blend = prev.x < dist ? 0.005 : 0.5;
	surfelMomentsTexture[moments_pixel] = lerp(prev, float2(dist, sqr(dist)), blend);
}

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	float4 result = 0;

	uint surfel_index = surfelAliveBuffer[DTid.x];
	Surfel surfel = surfelBuffer[surfel_index];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];
	uint life = surfel_data.GetLife();
	uint recycle = surfel_data.GetRecycle();

	const float3 N = normalize(unpack_unitvector(surfel.normal));

	float seed = 0.123456;
	float2 uv = float2(frac(GetFrame().frame_count.x / 4096.0), (float)surfel_index / SURFEL_CAPACITY);

	uint rayCount = surfel.GetRayCount();
	for (uint rayIndex = 0; rayIndex < rayCount; ++rayIndex)
	{
		RayDesc ray;
		ray.Origin = surfel.position;
		ray.TMin = 0.0001;
		ray.TMax = FLT_MAX;
		ray.Direction = normalize(sample_hemisphere_cos(N, seed, uv));

		uint2 moments_pixel = surfel_moment_pixel(surfel_index, N, ray.Direction);


#ifdef RTAPI
		RayQuery<
			RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
			RAY_FLAG_FORCE_OPAQUE
		> q;
		q.TraceRayInline(
			scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
			0,								// uint RayFlags
			push.instanceInclusionMask,		// uint InstanceInclusionMask
			ray								// RayDesc Ray
		);
		q.Proceed();
		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
#else
		RayHit hit = TraceRay_Closest(ray, push.instanceInclusionMask, seed, uv);

		if (hit.distance >= FLT_MAX - 1)
#endif // RTAPI

		{
			surfel_moments_write(moments_pixel, surfel.GetRadius());

			float3 envColor;
			[branch]
			if (IsStaticSky())
			{
				// We have envmap information in a texture:
				envColor = DEGAMMA_SKY(texture_globalenvmap.SampleLevel(sampler_linear_clamp, ray.Direction, 0).rgb);
			}
			else
			{
				envColor = GetDynamicSkyColor(ray.Direction, true, true, false, true);
			}
			result += float4(max(0, envColor), 1);
		}
		else
		{

			Surface surface;

			float hit_depth = 0;
			float3 hit_result = 0;

#ifdef RTAPI

			// ray origin updated for next bounce:
			ray.Origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();
			hit_depth = q.CommittedRayT();

			PrimitiveID prim;
			prim.primitiveIndex = q.CommittedPrimitiveIndex();
			prim.instanceIndex = q.CommittedInstanceID();
			prim.subsetIndex = q.CommittedGeometryIndex();

			if(!surface.load(prim, q.CommittedTriangleBarycentrics()))
				break;

#else

			// ray origin updated for next bounce:
			ray.Origin = ray.Origin + ray.Direction * hit.distance;
			hit_depth = hit.distance;

			if (!surface.load(hit.primitiveID, hit.bary))
				break;

#endif // RTAPI

			if (hit_depth < surfel.GetRadius())
			{
				hit_depth *= 0.8; // bias
			}
			hit_depth = clamp(hit_depth, 0, surfel.GetRadius());
			surfel_moments_write(moments_pixel, hit_depth);

			surface.P = ray.Origin;
			surface.V = -ray.Direction;
			surface.update();

#if 1
			// Light sampling:
			[loop]
			for (uint iterator = 0; iterator < GetFrame().lightarray_count; iterator++)
			{
				ShaderEntity light = load_entity(GetFrame().lightarray_offset + iterator);

				Lighting lighting;
				lighting.create(0, 0, 0, 0);

				float3 L = 0;
				float dist = 0;
				float NdotL = 0;

				switch (light.GetType())
				{
				case ENTITY_TYPE_DIRECTIONALLIGHT:
				{
					dist = FLT_MAX;

					L = light.GetDirection().xyz;
					NdotL = saturate(dot(L, surface.N));

					[branch]
					if (NdotL > 0)
					{
						float3 lightColor = light.GetColor().rgb * light.GetEnergy();

						[branch]
						if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
						{
							lightColor *= GetAtmosphericLightTransmittance(GetWeather().atmosphere, surface.P, L, texture_transmittancelut);
						}

						lighting.direct.diffuse = lightColor;
					}
				}
				break;
				case ENTITY_TYPE_POINTLIGHT:
				{
					L = light.position - surface.P;
					const float dist2 = dot(L, L);
					const float range2 = light.GetRange() * light.GetRange();

					[branch]
					if (dist2 < range2)
					{
						dist = sqrt(dist2);
						L /= dist;
						NdotL = saturate(dot(L, surface.N));

						[branch]
						if (NdotL > 0)
						{
							const float3 lightColor = light.GetColor().rgb * light.GetEnergy();

							lighting.direct.diffuse = lightColor;

							const float range2 = light.GetRange() * light.GetRange();
							const float att = saturate(1.0 - (dist2 / range2));
							const float attenuation = att * att;

							lighting.direct.diffuse *= attenuation;
						}
					}
				}
				break;
				case ENTITY_TYPE_SPOTLIGHT:
				{
					L = light.position - surface.P;
					const float dist2 = dot(L, L);
					const float range2 = light.GetRange() * light.GetRange();

					[branch]
					if (dist2 < range2)
					{
						dist = sqrt(dist2);
						L /= dist;
						NdotL = saturate(dot(L, surface.N));

						[branch]
						if (NdotL > 0)
						{
							const float SpotFactor = dot(L, light.GetDirection());
							const float spotCutOff = light.GetConeAngleCos();

							[branch]
							if (SpotFactor > spotCutOff)
							{
								const float3 lightColor = light.GetColor().rgb * light.GetEnergy();

								lighting.direct.diffuse = lightColor;

								const float range2 = light.GetRange() * light.GetRange();
								const float att = saturate(1.0 - (dist2 / range2));
								float attenuation = att * att;
								attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));

								lighting.direct.diffuse *= attenuation;
							}
						}
					}
				}
				break;
				}

				if (NdotL > 0 && dist > 0)
				{
					float3 shadow = NdotL;

					RayDesc newRay;
					newRay.Origin = surface.P;
#if 1
					newRay.Direction = normalize(lerp(L, sample_hemisphere_cos(L, seed, uv), 0.025f));
#else
					newRay.Direction = L;
#endif
					newRay.TMin = 0.001;
					newRay.TMax = dist;
#ifdef RTAPI
					q.TraceRayInline(
						scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
						RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,	// uint RayFlags
						0xFF,							// uint InstanceInclusionMask
						newRay							// RayDesc Ray
					);
					q.Proceed();
					shadow = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0 : shadow;
#else
					shadow = TraceRay_Any(newRay, push.instanceInclusionMask, seed, uv) ? 0 : shadow;
#endif // RTAPI
					if (any(shadow))
					{
						hit_result += max(0, shadow * lighting.direct.diffuse / PI);
					}
				}
			}

#endif


#ifdef SURFEL_ENABLE_INFINITE_BOUNCES
			// Evaluate surfel cache at hit point for multi bounce:
			{
				float4 surfel_gi = 0;
				uint cellindex = surfel_cellindex(surfel_cell(surface.P));
				SurfelGridCell cell = surfelGridBuffer[cellindex];
				for (uint i = 0; i < cell.count; ++i)
				{
					uint surfel_index = surfelCellBuffer[cell.offset + i];
					Surfel surfel = surfelBuffer[surfel_index];

					float3 L = surfel.position - surface.P;
					float dist2 = dot(L, L);
					if (dist2 < sqr(surfel.GetRadius()))
					{
						float3 normal = normalize(unpack_unitvector(surfel.normal));
						float dotN = dot(surface.N, normal);
						if (dotN > 0)
						{
							float dist = sqrt(dist2);
							float contribution = 1;

							contribution *= saturate(dotN);
							contribution *= saturate(1 - dist / surfel.GetRadius());
							contribution = smoothstep(0, 1, contribution);

							float2 moments = surfelMomentsTexturePrev.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, -L / dist), 0);
							contribution *= surfel_moment_weight(moments, dist);

							surfel_gi += float4(surfel.color, 1) * contribution;

						}
					}
				}
				if (surfel_gi.a > 0)
				{
					surfel_gi.rgb /= surfel_gi.a;
					surfel_gi.a = saturate(surfel_gi.a);
					hit_result += max(0, surfel_gi.rgb);
				}
			}
#endif // SURFEL_ENABLE_INFINITE_BOUNCES

			hit_result *= surface.albedo;
			hit_result += max(0, surface.emissiveColor);
			result += float4(hit_result, 1);

		}

	}


#ifdef SURFEL_ENABLE_IRRADIANCE_SHARING
	// Surfel irradiance sharing:
	{
		Surface surface;
		surface.P = surfel.position;
		surface.N = normalize(unpack_unitvector(surfel.normal));

		uint cellindex = surfel_cellindex(surfel_cell(surface.P));
		SurfelGridCell cell = surfelGridBuffer[cellindex];
		for (uint i = 0; i < cell.count; ++i)
		{
			uint surfel_index = surfelCellBuffer[cell.offset + i];
			Surfel surfel = surfelBuffer[surfel_index];

			float3 L = surfel.position - surface.P;
			float dist2 = dot(L, L);
			if (dist2 < sqr(surfel.GetRadius()))
			{
				float3 normal = normalize(unpack_unitvector(surfel.normal));
				float dotN = dot(surface.N, normal);
				if (dotN > 0)
				{
					float dist = sqrt(dist2);
					float contribution = 1;
					
					//contribution *= saturate(dotN);
					//contribution *= saturate(1 - dist / surfel.GetRadius());
					//contribution = smoothstep(0, 1, contribution);

					float2 moments = surfelMomentsTexturePrev.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, -L / dist), 0);
					contribution *= surfel_moment_weight(moments, dist);

					result += float4(surfel.color, 1) * contribution;

				}
			}
		}
	}
#endif // SURFEL_ENABLE_IRRADIANCE_SHARING

	if (result.a > 0)
	{
		result /= result.a;
		MultiscaleMeanEstimator(result.rgb, surfel_data, 0.08);
	}

	// Copy moment borders:
	uint2 moments_topleft = unflatten2D(surfel_index, SQRT_SURFEL_CAPACITY) * SURFEL_MOMENT_TEXELS;
	for (uint i = 0; i < SURFEL_MOMENT_TEXELS; ++i)
	{
		for (uint j = 0; j < SURFEL_MOMENT_TEXELS; ++j)
		{
			uint2 pixel_write = moments_topleft + uint2(i, j);
			uint2 pixel_read = clamp(pixel_write, moments_topleft + 1, moments_topleft + SURFEL_MOMENT_TEXELS - 2);
			surfelMomentsTexture[pixel_write] = surfelMomentsTexture[pixel_read];
		}
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
