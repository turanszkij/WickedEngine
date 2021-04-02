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
	float3 direction = SampleHemisphere_cos(surface.N, seed, uv);
	Ray ray = CreateRay(trace_bias_position(input.pos3D, surface.N), direction);
	float3 result = 0;

	uint bounces = xTraceUserData.x;
	const uint bouncelimit = 16;
	for (uint bounce = 0; ((bounce < min(bounces, bouncelimit)) && any(ray.energy)); ++bounce)
	{
		surface.P = ray.origin;

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
				float3 shadow = NdotL * ray.energy;

				float3 sampling_offset = float3(rand(seed, uv), rand(seed, uv), rand(seed, uv)) * 2 - 1;

				Ray newRay;
				newRay.origin = trace_bias_position(surface.P, surface.N);
				newRay.direction = L + sampling_offset * 0.025f;
				newRay.energy = 0;
				newRay.Update();
#ifdef RTAPI
				RayDesc apiray;
				apiray.TMin = 0.001;
				apiray.TMax = dist;
				apiray.Origin = newRay.origin;
				apiray.Direction = newRay.direction;
				RayQuery<
					RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
				> q;
				q.TraceRayInline(
					scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
					0,								// uint RayFlags
					0xFF,							// uint InstanceInclusionMask
					apiray							// RayDesc Ray
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
				shadow = TraceRay_Any(newRay, dist) ? 0 : shadow;
#endif // RTAPI
				if (any(shadow))
				{
					result += max(0, shadow * lighting.direct.diffuse / PI);
				}
			}
		}

		// Sample primary ray (scene materials, sky, etc):

#ifdef RTAPI
		RayDesc apiray;
		apiray.TMin = 0.001;
		apiray.TMax = FLT_MAX;
		apiray.Origin = ray.origin;
		apiray.Direction = ray.direction;
		RayQuery<
			RAY_FLAG_FORCE_OPAQUE |
			RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
		> q;
		q.TraceRayInline(scene_acceleration_structure, 0, 0xFF, apiray);
		q.Proceed();
		if(q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
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
				envColor = DEGAMMA_SKY(texture_globalenvmap.SampleLevel(sampler_linear_clamp, ray.direction, 0).rgb);
			}
			else
			{
				envColor = GetDynamicSkyColor(ray.direction, true, true, false, true);
			}
			result += max(0, ray.energy * envColor);

			// Erase the ray's energy
			ray.energy = 0;
			break;
		}

		ShaderMaterial material;

#ifdef RTAPI

		// ray origin updated for next bounce:
		ray.origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();

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

		// Non-RTAPI path: sampling from texture atlas
		ray.origin = hit.position;

		EvaluateObjectSurface(
			hit,
			material,
			surface
		);

#endif // RTAPI

		surface.update();

		result += max(0, ray.energy * surface.emissiveColor.rgb * surface.emissiveColor.a);

		// Calculate chances of reflection types:
		const float refractChance = material.transmission;

		// Roulette-select the ray's path
		float roulette = rand(seed, uv);
		if (roulette < refractChance)
		{
			// Refraction
			const float3 R = refract(ray.direction, surface.N, 1 - material.refraction);
			ray.direction = lerp(R, SampleHemisphere_cos(R, seed, uv), surface.roughnessBRDF);
			ray.energy *= surface.albedo;

			// The ray penetrates the surface, so push DOWN along normal to avoid self-intersection:
			ray.origin = trace_bias_position(ray.origin, -surface.N);

			// Add a new bounce iteration, otherwise the transparent effect can disappear:
			bounces++;
		}
		else
		{
			// Calculate chances of reflection types:
			const float3 F = F_Schlick(surface.f0, saturate(dot(-ray.direction, surface.N)));
			const float specChance = dot(F, 0.333f);

			roulette = rand(seed, uv);
			if (roulette < specChance)
			{
				// Specular reflection
				const float3 R = reflect(ray.direction, surface.N);
				ray.direction = lerp(R, SampleHemisphere_cos(R, seed, uv), surface.roughnessBRDF);
				ray.energy *= F / specChance;
			}
			else
			{
				// Diffuse reflection
				ray.direction = SampleHemisphere_cos(surface.N, seed, uv);
				ray.energy *= surface.albedo / (1 - specChance);
			}

			// Ray reflects from surface, so push UP along normal to avoid self-intersection:
			ray.origin = trace_bias_position(ray.origin, surface.N);
		}

		ray.Update();
	}

	return float4(result, xTraceAccumulationFactor);
}
