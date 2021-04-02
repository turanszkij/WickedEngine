#define RAY_BACKFACE_CULLING
#define RAYTRACE_STACK_SHARED
#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"

RWTEXTURE2D(resultTexture, float4, 0);

[numthreads(RAYTRACING_LAUNCH_BLOCKSIZE, RAYTRACING_LAUNCH_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	uint2 pixel = DTid.xy;
	if (pixel.x >= xTraceResolution.x || pixel.y >= xTraceResolution.y)
	{
		return;
	}
	float3 result = 0;

	// Compute screen coordinates:
	float2 screenUV = float2((pixel + xTracePixelOffset) * xTraceResolution_rcp.xy * 2.0f - 1.0f) * float2(1, -1);
	float seed = xTraceRandomSeed;

	// Create starting ray:
	Ray ray = CreateCameraRay(screenUV);

	uint bounces = xTraceUserData.x;
	const uint bouncelimit = 16;
	for (uint bounce = 0; ((bounce < min(bounces, bouncelimit)) && any(ray.energy)); ++bounce)
	{
		ray.Update();

#ifdef RTAPI
		RayDesc apiray;
		apiray.TMin = 0.001;
		apiray.TMax = FLT_MAX;
		apiray.Origin = ray.origin;
		apiray.Direction = ray.direction;
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
			apiray							// RayDesc Ray
		);
		q.Proceed();
		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
#else
		RayHit hit = TraceRay_Closest(ray, groupIndex);

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
				envColor = GetDynamicSkyColor(ray.direction);
			}
			result += max(0, ray.energy * envColor);

			// Erase the ray's energy
			ray.energy = 0.0f;
			break;
		}

		Surface surface;
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
		// ray origin updated for next bounce:
		ray.origin = hit.position;

		EvaluateObjectSurface(
			hit,
			material,
			surface
		);

#endif // RTAPI

		surface.P = ray.origin;
		surface.V = normalize(g_xCamera_CamPos - surface.P);
		surface.update();

		float3 current_energy = ray.energy;
		result += max(0, current_energy * surface.emissiveColor.rgb * surface.emissiveColor.a);


		float roulette;

		const float blendChance = 1 - surface.opacity;
		roulette = rand(seed, screenUV);
		if (roulette < blendChance)
		{
			// Alpha blending

			// The ray penetrates the surface, so push DOWN along normal to avoid self-intersection:
			ray.origin = trace_bias_position(ray.origin, -surface.N);

			// Add a new bounce iteration, otherwise the transparent effect can disappear:
			bounces++;

			continue; // skip light sampling
		}
		else
		{
			const float refractChance = surface.transmission;
			roulette = rand(seed, screenUV);
			if (roulette < refractChance)
			{
				// Refraction
				const float3 R = refract(ray.direction, surface.N, 1 - material.refraction);
				ray.direction = lerp(R, SampleHemisphere_cos(R, seed, screenUV), surface.roughnessBRDF);
				ray.energy *= surface.albedo;

				// The ray penetrates the surface, so push DOWN along normal to avoid self-intersection:
				ray.origin = trace_bias_position(ray.origin, -surface.N);

				// Add a new bounce iteration, otherwise the transparent effect can disappear:
				bounces++;
			}
			else
			{
				const float3 F = F_Schlick(surface.f0, saturate(dot(-ray.direction, surface.N)));
				const float specChance = dot(F, 0.333);

				roulette = rand(seed, screenUV);
				if (roulette < specChance)
				{
					// Specular reflection
					const float3 R = reflect(ray.direction, surface.N);
					ray.direction = lerp(R, SampleHemisphere_cos(R, seed, screenUV), surface.roughnessBRDF);
					ray.energy *= F / specChance;
				}
				else
				{
					// Diffuse reflection
					ray.direction = SampleHemisphere_cos(surface.N, seed, screenUV);
					ray.energy *= surface.albedo / (1 - specChance);
				}

				if (dot(ray.direction, surface.facenormal) <= 0)
				{
					// Don't allow normal map to bend over the face normal more than 90 degrees to avoid light leaks
					//	In this case, we will not allow more bounces,
					//	but the current light sampling is still fine to avoid abrupt cutoff
					ray.energy = 0;
				}

				// Ray reflects from surface, so push UP along normal to avoid self-intersection:
				ray.origin = trace_bias_position(ray.origin, surface.N);
			}
		}

		surface.P = ray.origin;

		float3 lightColor = 0;
		SurfaceToLight surfaceToLight = (SurfaceToLight)0;

		// Light sampling:
		[loop]
		for (uint iterator = 0; iterator < g_xFrame_LightArrayCount; iterator++)
		{
			ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + iterator];

			Lighting lighting;
			lighting.create(0, 0, 0, 0);

			float3 L = 0;
			float dist = 0;

			switch (light.GetType())
			{
			case ENTITY_TYPE_DIRECTIONALLIGHT:
			{
				dist = FLT_MAX;

				L = light.GetDirection().xyz;

				surfaceToLight.create(surface, L);

				[branch]
				if (surfaceToLight.NdotL > 0)
				{
					float3 atmosphereTransmittance = 1;
					if (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY)
					{
						AtmosphereParameters Atmosphere = GetAtmosphereParameters();
						atmosphereTransmittance = GetAtmosphericLightTransmittance(Atmosphere, surface.P, L, texture_transmittancelut);
					}

					lightColor = light.GetColor().rgb * light.GetEnergy() * atmosphereTransmittance;
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

					surfaceToLight.create(surface, L);

					[branch]
					if (surfaceToLight.NdotL > 0)
					{
						const float range2 = light.GetRange() * light.GetRange();
						const float att = saturate(1 - (dist2 / range2));
						const float attenuation = att * att;

						lightColor = light.GetColor().rgb * light.GetEnergy();
						lightColor *= attenuation;
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

					surfaceToLight.create(surface, L);

					[branch]
					if (surfaceToLight.NdotL > 0)
					{
						const float SpotFactor = dot(L, light.GetDirection());
						const float spotCutOff = light.GetConeAngleCos();

						[branch]
						if (SpotFactor > spotCutOff)
						{
							const float range2 = light.GetRange() * light.GetRange();
							const float att = saturate(1 - (dist2 / range2));
							float attenuation = att * att;
							attenuation *= saturate((1 - (1 - SpotFactor) * 1 / (1 - spotCutOff)));

							lightColor = light.GetColor().rgb * light.GetEnergy();
							lightColor *= attenuation;
						}
					}
				}
			}
			break;
			}

			if (surfaceToLight.NdotL > 0 && dist > 0)
			{
				float3 shadow = surfaceToLight.NdotL * current_energy;

				float3 sampling_offset = float3(rand(seed, screenUV), rand(seed, screenUV), rand(seed, screenUV)) * 2 - 1; // todo: should be specific to light surface

				Ray newRay;
				newRay.origin = surface.P;
				newRay.direction = L + sampling_offset * 0.025;
				newRay.energy = 0;
				newRay.Update();
#ifdef RTAPI
				RayDesc apiray;
				apiray.TMin = 0.001;
				apiray.TMax = dist;
				apiray.Origin = newRay.origin;
				apiray.Direction = newRay.direction;
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
				shadow = TraceRay_Any(newRay, dist, groupIndex) ? 0 : shadow;
#endif // RTAPI
				if (any(shadow))
				{
					lighting.direct.specular = lightColor * BRDF_GetSpecular(surface, surfaceToLight);
					lighting.direct.diffuse = lightColor * BRDF_GetDiffuse(surface, surfaceToLight);
					result += max(0, shadow * (surface.albedo * lighting.direct.diffuse + lighting.direct.specular));
				}
			}
		}

	}

	// Pre-clear result texture for first bounce and first accumulation sample:
	if (xTraceUserData.y == 0)
	{
		resultTexture[pixel] = 0;
	}
	resultTexture[pixel] = lerp(resultTexture[pixel], float4(result, 1), xTraceAccumulationFactor);
}
