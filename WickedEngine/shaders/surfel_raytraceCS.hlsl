#define TEXTURE_SLOT_NONUNIFORM
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
Texture2D<float4> surfelIrradianceTexture : register(t6);

RWStructuredBuffer<SurfelRayDataPacked> surfelRayBuffer : register(u0);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint global_ray_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_RAYCOUNT);
	if (DTid.x >= global_ray_count)
		return;

	SurfelRayData rayData = surfelRayBuffer[DTid.x].load();

	uint surfel_index = rayData.surfelIndex;
	Surfel surfel = surfelBuffer[surfel_index];

	const float3 N = normalize(unpack_unitvector(surfel.normal));

	RNG rng;
	rng.init(DTid.xx, GetFrame().frame_count);

	{
		RayDesc ray;
		ray.Origin = surfel.position;
		ray.TMin = 0.0001;
		ray.TMax = FLT_MAX;
		ray.Direction = normalize(sample_hemisphere_cos(N, rng));

		rayData.direction = ray.Direction;

#ifdef RTAPI
		wiRayQuery q;
		q.TraceRayInline(
			scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
			RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
			RAY_FLAG_FORCE_OPAQUE |
			RAY_FLAG_CULL_BACK_FACING_TRIANGLES,	// uint RayFlags
			push.instanceInclusionMask,		// uint InstanceInclusionMask
			ray								// RayDesc Ray
		);
		while (q.Proceed());
		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
#else
		RayHit hit = TraceRay_Closest(ray, push.instanceInclusionMask, rng);

		if (hit.distance >= FLT_MAX - 1)
#endif // RTAPI

		{
			float3 envColor;
			[branch]
			if (IsStaticSky())
			{
				// We have envmap information in a texture:
				envColor = GetStaticSkyColor(ray.Direction);
			}
			else
			{
				envColor = GetDynamicSkyColor(ray.Direction, true, false, true);
			}
			rayData.radiance = max(0, envColor);
			rayData.depth = -1;
		}
		else
		{

			Surface surface;
			surface.init();

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

			surface.SetBackface(!q.CommittedTriangleFrontFace());
			if(!surface.load(prim, q.CommittedTriangleBarycentrics()))
				return;

#else

			// ray origin updated for next bounce:
			ray.Origin = ray.Origin + ray.Direction * hit.distance;
			hit_depth = hit.distance;

			surface.SetBackface(hit.is_backface);

			if (!surface.load(hit.primitiveID, hit.bary))
				return;

#endif // RTAPI

			surface.P = ray.Origin;
			surface.V = -ray.Direction;
			surface.update();

#if 1
			// Light sampling:
			{
				const uint light_count = GetFrame().lightarray_count;
				const uint light_index = GetFrame().lightarray_offset + rng.next_uint(light_count);
				ShaderEntity light = load_entity(light_index);

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
					L += sample_hemisphere_cos(L, rng) * light.GetRadius();
					NdotL = saturate(dot(L, surface.N));

					[branch]
					if (NdotL > 0)
					{
						float3 lightColor = light.GetColor().rgb;

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
					light.position += light.GetDirection() * (rng.next_float() - 0.5) * light.GetLength();
					light.position += sample_hemisphere_cos(normalize(light.position - surface.P), rng) * light.GetRadius();
					L = light.position - surface.P;
					const float dist2 = dot(L, L);
					const float range = light.GetRange();
					const float range2 = range * range;

					[branch]
					if (dist2 < range2)
					{
						dist = sqrt(dist2);
						L /= dist;
						NdotL = saturate(dot(L, surface.N));

						[branch]
						if (NdotL > 0)
						{
							const float3 lightColor = light.GetColor().rgb;

							lighting.direct.diffuse = lightColor;
							lighting.direct.diffuse *= attenuation_pointlight(dist2, range, range2);
						}
					}
				}
				break;
				case ENTITY_TYPE_SPOTLIGHT:
				{
					float3 Loriginal = normalize(light.position - surface.P);
					light.position += sample_hemisphere_cos(normalize(light.position - surface.P), rng) * light.GetRadius();
					L = light.position - surface.P;
					const float dist2 = dot(L, L);
					const float range = light.GetRange();
					const float range2 = range * range;

					[branch]
					if (dist2 < range2)
					{
						dist = sqrt(dist2);
						L /= dist;
						NdotL = saturate(dot(L, surface.N));

						[branch]
						if (NdotL > 0)
						{
							const float spot_factor = dot(Loriginal, light.GetDirection());
							const float spot_cutoff = light.GetConeAngleCos();

							[branch]
							if (spot_factor > spot_cutoff)
							{
								const float3 lightColor = light.GetColor().rgb;

								lighting.direct.diffuse = lightColor;
								lighting.direct.diffuse *= attenuation_spotlight(dist2, range, range2, spot_factor, light.GetAngleScale(), light.GetAngleOffset());
							}
						}
					}
				}
				break;
				}

				if (NdotL > 0 && dist > 0)
				{
					float3 shadow = 1;

					RayDesc newRay;
					newRay.Origin = surface.P;
					newRay.TMin = 0.001;
					newRay.TMax = dist;
					newRay.Direction = L + max3(surface.sss);

#ifdef RTAPI
					q.TraceRayInline(
						scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
						RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
						RAY_FLAG_FORCE_OPAQUE |
						RAY_FLAG_CULL_FRONT_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,	// uint RayFlags
						0xFF,							// uint InstanceInclusionMask
						newRay							// RayDesc Ray
					);
					while (q.Proceed());
					shadow = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0 : shadow;
#else
					shadow = TraceRay_Any(newRay, push.instanceInclusionMask, rng) ? 0 : shadow;
#endif // RTAPI
					if (any(shadow))
					{
						hit_result += light_count * max(0, shadow * lighting.direct.diffuse * NdotL / PI);
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

					float3 L = surface.P - surfel.position;
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
							
							float2 moments = surfelMomentsTexturePrev.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, L / dist), 0);
							contribution *= surfel_moment_weight(moments, dist);

							surfel_gi += surfelIrradianceTexture.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, surface.N), 0) * contribution;

						}
					}
				}
				if (surfel_gi.a > 0)
				{
					const float energy_conservation = 0.95;
					surfel_gi.rgb *= energy_conservation;
					surfel_gi.rgb /= surfel_gi.a;
					surfel_gi.a = saturate(surfel_gi.a);
					hit_result += max(0, surfel_gi.rgb);
				}
			}
#endif // SURFEL_ENABLE_INFINITE_BOUNCES

			hit_result *= surface.albedo;
			hit_result += surface.emissiveColor;

			rayData.radiance = hit_result;
			rayData.depth = hit_depth;

		}

	}

	surfelRayBuffer[DTid.x].store(rayData);
}
