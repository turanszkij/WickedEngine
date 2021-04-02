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
	float seed = xTraceRandomSeed;
	RayDesc ray;
	ray.Origin = input.pos3D;
	ray.Direction = SampleHemisphere_cos(surface.N, seed, uv);
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
		for (uint iterator = 0; iterator < g_xFrame_LightArrayCount; iterator++)
		{
			ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + iterator];

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
					if (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY)
					{
						AtmosphereParameters Atmosphere = GetAtmosphereParameters();
						atmosphereTransmittance = GetAtmosphericLightTransmittance(Atmosphere, surface.P, L, texture_transmittancelut);
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

				float3 sampling_offset = float3(rand(seed, uv), rand(seed, uv), rand(seed, uv)) * 2 - 1;

				RayDesc newRay;
				newRay.Origin = surface.P;
				newRay.Direction = normalize(L + sampling_offset * 0.025f);
				newRay.TMin = 0.001;
				newRay.TMax = dist;
#ifdef RTAPI
				RayQuery<
					RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
				> q;
				q.TraceRayInline(
					scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
					0,								// uint RayFlags
					0xFF,							// uint InstanceInclusionMask
					newRay							// RayDesc Ray
				);
				while (q.Proceed())
				{
					ShaderMesh mesh = bindless_buffers[q.CandidateInstanceID()].Load<ShaderMesh>(0);
					ShaderMeshSubset subset = bindless_subsets[mesh.subsetbuffer][q.CandidateGeometryIndex()];
					ShaderMaterial material = bindless_buffers[subset.material].Load<ShaderMaterial>(0);
					[branch]
					if (!material.IsCastingShadow())
					{
						continue;
					}

					Surface surface;
					EvaluateObjectSurface(
						mesh,
						subset,
						material,
						q.CandidatePrimitiveIndex(),
						q.CandidateTriangleBarycentrics(),
						q.CandidateObjectToWorld3x4(),
						surface
					);

					shadow *= lerp(1, surface.albedo * surface.transmission, surface.opacity);

					[branch]
					if (!any(shadow))
					{
						q.CommitNonOpaqueTriangleHit();
					}
				}
				shadow = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0 : shadow;
#else
				shadow = TraceRay_Any(newRay) ? 0 : shadow;
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
			0xFF,							// uint InstanceInclusionMask
			ray								// RayDesc Ray
		);
		q.Proceed();
		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
#else
		RayHit hit = TraceRay_Closest(ray);

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

		ShaderMaterial material;

#ifdef RTAPI
		// ray origin updated for next bounce:
		ray.Origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();

		ShaderMesh mesh = bindless_buffers[q.CommittedInstanceID()].Load<ShaderMesh>(0);
		ShaderMeshSubset subset = bindless_subsets[mesh.subsetbuffer][q.CommittedGeometryIndex()];
		material = bindless_buffers[subset.material].Load<ShaderMaterial>(0);

		EvaluateObjectSurface(
			mesh,
			subset,
			material,
			q.CommittedPrimitiveIndex(),
			q.CommittedTriangleBarycentrics(),
			q.CommittedObjectToWorld3x4(),
			surface
		);

#else
		// ray origin updated for next bounce:
		ray.Origin = ray.Origin + ray.Direction * hit.distance;

		EvaluateObjectSurface(
			hit,
			material,
			surface
		);

#endif // RTAPI

		surface.update();

		// Calculate chances of reflection types:
		const float refractChance = surface.transmission;

		// Roulette-select the ray's path
		float roulette = rand(seed, uv);
		if (roulette < refractChance)
		{
			// Refraction
			const float3 R = refract(ray.Direction, surface.N, 1 - material.refraction);
			ray.Direction = lerp(R, SampleHemisphere_cos(R, seed, uv), surface.roughnessBRDF);
			energy *= surface.albedo;

			// Add a new bounce iteration, otherwise the transparent effect can disappear:
			bounces++;
		}
		else
		{
			// Calculate chances of reflection types:
			const float3 F = F_Schlick(surface.f0, saturate(dot(-ray.Direction, surface.N)));
			const float specChance = dot(F, 0.333f);

			roulette = rand(seed, uv);
			if (roulette < specChance)
			{
				// Specular reflection
				const float3 R = reflect(ray.Direction, surface.N);
				ray.Direction = lerp(R, SampleHemisphere_cos(R, seed, uv), surface.roughnessBRDF);
				energy *= F / specChance;
			}
			else
			{
				// Diffuse reflection
				ray.Direction = SampleHemisphere_cos(surface.N, seed, uv);
				energy *= surface.albedo / (1 - specChance);
			}
		}

		result += max(0, energy * surface.emissiveColor.rgb * surface.emissiveColor.a);
	}

	return float4(result, xTraceAccumulationFactor);
}
