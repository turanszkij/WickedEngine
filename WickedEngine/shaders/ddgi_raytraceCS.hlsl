#define TEXTURE_SLOT_NONUNIFORM
#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_DDGI.h"

// This shader runs one probe per thread group and each thread will trace rays and write the trace result to a ray data buffer
//	ray data buffer will be later integrated by ddgi_updateCS shader which updates the DDGI irradiance and depth textures

PUSHCONSTANT(push, DDGIPushConstants);

StructuredBuffer<uint> rayallocationBuffer : register(t0);
Buffer<uint> raycountBuffer : register(t1);

RWStructuredBuffer<DDGIRayDataPacked> rayBuffer : register(u0);

groupshared float shared_inconsistency[DDGI_COLOR_RESOLUTION * DDGI_COLOR_RESOLUTION];
groupshared uint shared_rayCount;

static const uint THREADCOUNT = 32;

// spherical fibonacci: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_ray_trace.rgen
#define madfrac(A, B) ((A) * (B)-floor((A) * (B)))
static const float PHI = sqrt(5) * 0.5 + 0.5;
float3 spherical_fibonacci(float i, float n)
{
	float phi = 2.0 * PI * madfrac(i, PHI - 1);
	float cos_theta = 1.0 - (2.0 * i + 1.0) * (1.0 / n);
	float sin_theta = sqrt(clamp(1.0 - cos_theta * cos_theta, 0.0f, 1.0f));
	return float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

[numthreads(THREADCOUNT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint allocCount = rayallocationBuffer[3];
	if(DTid.x >= allocCount)
		return;
	const uint rayAlloc = rayallocationBuffer[4 + DTid.x];
	const uint probeIndex = rayAlloc & 0xFFFFF;
	const uint rayIndex = rayAlloc >> 20u;
	const uint rayCount = raycountBuffer[probeIndex] * DDGI_RAY_BUCKET_COUNT;
	const uint3 probeCoord = ddgi_probe_coord(probeIndex);
	const float3 probePos = ddgi_probe_position(probeCoord);

	RNG rng;
	rng.init(DTid.xx, GetFrame().frame_count);

	const float3x3 random_orientation = (float3x3)g_xTransform;
	
	{
		RayDesc ray;
		ray.Origin = probePos;
		ray.TMin = 0; // don't need TMin because we are not tracing from a surface
		ray.TMax = FLT_MAX;
		ray.Direction = normalize(mul(random_orientation, spherical_fibonacci(rayIndex, rayCount)));

#ifdef RTAPI
		wiRayQuery q;
		q.TraceRayInline(
			scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
			RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
			RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
			RAY_FLAG_FORCE_OPAQUE,			// uint RayFlags
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

			DDGIRayData rayData;
			rayData.direction = ray.Direction;
			rayData.depth = -1;
			rayData.radiance = float4(envColor, 1);
			rayBuffer[probeIndex * DDGI_MAX_RAYCOUNT + rayIndex].store(rayData);
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

			if (!surface.load(prim, q.CommittedTriangleBarycentrics()))
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

							lighting.direct.diffuse = lightColor * attenuation_pointlight(dist2, range, range2);
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

								lighting.direct.diffuse = lightColor * attenuation_spotlight(dist2, range, range2, spot_factor, light.GetAngleScale(), light.GetAngleOffset());
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
						RAY_FLAG_CULL_FRONT_FACING_TRIANGLES |
						RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
						RAY_FLAG_CULL_FRONT_FACING_TRIANGLES |
						RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,	// uint RayFlags
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

			// Infinite bounces based on previous frame probe sampling:
			if (push.frameIndex > 0)
			{
				const float energy_conservation = 0.95;
				hit_result += ddgi_sample_irradiance(surface.P, surface.facenormal) * energy_conservation;
			}
			hit_result *= surface.albedo;
			hit_result += surface.emissiveColor;

			DDGIRayData rayData;
			rayData.direction = ray.Direction;
			rayData.depth = hit_depth;
			rayData.radiance = float4(hit_result, 1);
			rayBuffer[probeIndex * DDGI_MAX_RAYCOUNT + rayIndex].store(rayData);
		}

	}
}
