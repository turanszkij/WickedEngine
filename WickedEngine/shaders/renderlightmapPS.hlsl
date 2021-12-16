#define RAY_BACKFACE_CULLING
#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"

struct Input
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 pos3D : WORLDPOSITION;
	float3 normal : NORMAL;
};

float4 main(Input input) : SV_TARGET
{
	Surface surface;
	surface.init();
	surface.N = normalize(input.normal);

	float2 uv = input.uv;
	float seed = xTraceAccumulationFactor;
	RayDesc ray;
	ray.Origin = input.pos3D;
	ray.Direction = sample_hemisphere_cos(surface.N, seed, uv);
	ray.TMin = 0.001;
	ray.TMax = FLT_MAX;
	float3 result = 0;
	float3 energy = 1;

	uint bounces = xTraceUserData.x;
	const uint bouncelimit = 16;
	for (uint bounce = 0; ((bounce < min(bounces, bouncelimit)) && any(energy)); ++bounce)
	{
		surface.P = ray.Origin;

		[loop]
		for (uint iterator = 0; iterator < GetFrame().lightarray_count; iterator++)
		{
			ShaderEntity light = load_entity(GetFrame().lightarray_offset + iterator);

			Lighting lighting;
			lighting.create(0, 0, 0, 0);

			if (!(light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC))
			{
				continue; // dynamic lights will not be baked into lightmap
			}

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
					float3 atmosphereTransmittance = 1.0;
					if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
					{
						atmosphereTransmittance = GetAtmosphericLightTransmittance(GetWeather().atmosphere, surface.P, L, texture_transmittancelut);
					}
					
					float3 lightColor = light.GetColor().rgb * light.GetEnergy() * atmosphereTransmittance;

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
				float3 shadow = NdotL * energy;

				RayDesc newRay;
				newRay.Origin = surface.P;
				newRay.Direction = normalize(lerp(L, sample_hemisphere_cos(L, seed, uv), 0.025f));
				newRay.TMin = 0.001;
				newRay.TMax = dist;
#ifdef RTAPI
				RayQuery<
					RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
				> q;
				q.TraceRayInline(
					scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
					0,								// uint RayFlags
					xTraceUserData.y,				// uint InstanceInclusionMask
					newRay							// RayDesc Ray
				);
				while (q.Proceed())
				{
					PrimitiveID prim;
					prim.primitiveIndex = q.CandidatePrimitiveIndex();
					prim.instanceIndex = q.CandidateInstanceID();
					prim.subsetIndex = q.CandidateGeometryIndex();

					Surface surface;
					if (!surface.load(prim, q.CandidateTriangleBarycentrics()))
						break;

					shadow *= lerp(1, surface.albedo * surface.transmission, surface.opacity);

					[branch]
					if (!any(shadow))
					{
						q.CommitNonOpaqueTriangleHit();
					}
				}
				shadow = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0 : shadow;
#else
				shadow = TraceRay_Any(newRay, xTraceUserData.y, seed, uv) ? 0 : shadow;
#endif // RTAPI
				if (any(shadow))
				{
					result += max(0, shadow * lighting.direct.diffuse / PI);
				}
			}
		}

		// Sample primary ray (scene materials, sky, etc):
		ray.Direction = normalize(ray.Direction);

#ifdef RTAPI
		RayQuery<
			RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
		> q;
		q.TraceRayInline(
			scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
#ifdef RAY_BACKFACE_CULLING
			RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
#endif // RAY_BACKFACE_CULLING
			RAY_FLAG_FORCE_OPAQUE |
			0,								// uint RayFlags
			xTraceUserData.y,							// uint InstanceInclusionMask
			ray								// RayDesc Ray
		);
		q.Proceed();
		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
#else
		RayHit hit = TraceRay_Closest(ray, xTraceUserData.y, seed, uv);

		if (hit.distance >= FLT_MAX - 1)
#endif // RTAPI

		{
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
			result += max(0, energy * envColor);

			// Erase the ray's energy
			energy = 0;
			break;
		}

#ifdef RTAPI
		// ray origin updated for next bounce:
		ray.Origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();

		PrimitiveID prim;
		prim.primitiveIndex = q.CommittedPrimitiveIndex();
		prim.instanceIndex = q.CommittedInstanceID();
		prim.subsetIndex = q.CommittedGeometryIndex();

		if (!surface.load(prim, q.CommittedTriangleBarycentrics()))
			return 0;

#else
		// ray origin updated for next bounce:
		ray.Origin = ray.Origin + ray.Direction * hit.distance;

		if (!surface.load(hit.primitiveID, hit.bary))
			return 0;

#endif // RTAPI

		surface.update();

		result += max(0, energy * surface.emissiveColor);

		// Calculate chances of reflection types:
		const float refractChance = surface.transmission;

		// Roulette-select the ray's path
		float roulette = rand(seed, uv);
		if (roulette < refractChance)
		{
			// Refraction
			const float3 R = refract(ray.Direction, surface.N, 1 - surface.material.refraction);
			ray.Direction = lerp(R, sample_hemisphere_cos(R, seed, uv), surface.roughnessBRDF);
			energy *= surface.albedo;

			// Add a new bounce iteration, otherwise the transparent effect can disappear:
			bounces++;
		}
		else
		{
			// Calculate chances of reflection types:
			const float specChance = dot(surface.F, 0.333f);

			roulette = rand(seed, uv);
			if (roulette < specChance)
			{
				// Specular reflection
				const float3 R = reflect(ray.Direction, surface.N);
				ray.Direction = lerp(R, sample_hemisphere_cos(R, seed, uv), surface.roughnessBRDF);
				energy *= surface.F / specChance;
			}
			else
			{
				// Diffuse reflection
				ray.Direction = sample_hemisphere_cos(surface.N, seed, uv);
				energy *= surface.albedo / (1 - specChance);
			}
		}

	}

	return float4(result, xTraceAccumulationFactor);
}
