#define RAY_BACKFACE_CULLING
#define RAYTRACE_STACK_SHARED
#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"

RWTEXTURE2D(output, float4, 0);
RWTEXTURE2D(output_albedo, float4, 1);
RWTEXTURE2D(output_normal, float4, 2);

[numthreads(RAYTRACING_LAUNCH_BLOCKSIZE, RAYTRACING_LAUNCH_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	uint2 pixel = DTid.xy;
	if (pixel.x >= xTraceResolution.x || pixel.y >= xTraceResolution.y)
	{
		return;
	}
	float3 result = 0;
	float3 energy = 1;

	// for denoiser:
	float3 primary_albedo = 0;
	float3 primary_normal = 0;

	// Compute screen coordinates:
	float2 uv = float2((pixel + xTracePixelOffset) * xTraceResolution_rcp.xy * 2 - 1) * float2(1, -1);
	float seed = xTraceAccumulationFactor;

	// Create starting ray:
	RayDesc ray = CreateCameraRay(uv);

	// Depth of field setup:
	float3 focal_point = ray.Origin + ray.Direction * g_xCamera_FocalLength;
	float3 coc = float3(hemispherepoint_cos(rand(seed, uv), rand(seed, uv)).xy, 0);
	coc.xy *= g_xCamera_ApertureShape.xy;
	coc = mul(coc, float3x3(cross(g_xCamera_Up, g_xCamera_At), g_xCamera_Up, g_xCamera_At));
	coc *= g_xCamera_FocalLength;
	coc *= g_xCamera_ApertureSize;
	coc *= 0.1f;
	ray.Origin = ray.Origin + coc;
	ray.Direction = focal_point - ray.Origin; // will be normalized before tracing!

	uint bounces = xTraceUserData.x;
	const uint bouncelimit = 16;
	for (uint bounce = 0; ((bounce < min(bounces, bouncelimit)) && any(energy)); ++bounce)
	{
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
		RayHit hit = TraceRay_Closest(ray, groupIndex);

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
				envColor = GetDynamicSkyColor(ray.Direction);
			}
			result += max(0, energy * envColor);

			// Erase the ray's energy
			energy = 0.0f;
			break;
		}

		Surface surface;
		ShaderMaterial material;

#ifdef RTAPI
		// ray origin updated for next bounce:
		ray.Origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();

		ShaderMesh mesh = bindless_buffers[NonUniformResourceIndex(q.CommittedInstanceID())].Load<ShaderMesh>(0);
		ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][q.CommittedGeometryIndex()];
		material = bindless_buffers[NonUniformResourceIndex(subset.material)].Load<ShaderMaterial>(0);

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

		surface.P = ray.Origin;
		surface.V = -ray.Direction;
		surface.update();

		result += max(0, energy * surface.emissiveColor.rgb * surface.emissiveColor.a);


		// Light sampling:
		[loop]
		for (uint iterator = 0; iterator < g_xFrame_LightArrayCount; iterator++)
		{
			ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + iterator];

			Lighting lighting;
			lighting.create(0, 0, 0, 0);

			float3 L = 0;
			float dist = 0;
			float3 lightColor = 0;
			SurfaceToLight surfaceToLight = (SurfaceToLight)0;

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
					lightColor = light.GetColor().rgb * light.GetEnergy();

					float3 atmosphereTransmittance = 1;
					if (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY)
					{
						AtmosphereParameters Atmosphere = g_xFrame_Atmosphere;
						atmosphereTransmittance = GetAtmosphericLightTransmittance(Atmosphere, surface.P, L, texture_transmittancelut);
					}
					lightColor *= atmosphereTransmittance;
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
						lightColor = light.GetColor().rgb * light.GetEnergy();

						const float range2 = light.GetRange() * light.GetRange();
						const float att = saturate(1 - (dist2 / range2));
						const float attenuation = att * att;
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
							lightColor = light.GetColor().rgb * light.GetEnergy();

							const float range2 = light.GetRange() * light.GetRange();
							const float att = saturate(1 - (dist2 / range2));
							float attenuation = att * att;
							attenuation *= saturate((1 - (1 - SpotFactor) * 1 / (1 - spotCutOff)));
							lightColor *= attenuation;
						}
					}
				}
			}
			break;
			}

			if (surfaceToLight.NdotL > 0 && dist > 0)
			{
				float3 shadow = surfaceToLight.NdotL * energy;

				RayDesc newRay;
				newRay.Origin = surface.P;
				newRay.Direction = normalize(lerp(L, SampleHemisphere_cos(L, seed, uv), 0.025));
				newRay.TMin = 0.001;
				newRay.TMax = dist;
#ifdef RTAPI
				q.TraceRayInline(
					scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
					0,								// uint RayFlags
					0xFF,							// uint InstanceInclusionMask
					newRay							// RayDesc Ray
				);
				while (q.Proceed())
				{
					ShaderMesh mesh = bindless_buffers[NonUniformResourceIndex(q.CandidateInstanceID())].Load<ShaderMesh>(0);
					ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][q.CandidateGeometryIndex()];
					ShaderMaterial material = bindless_buffers[NonUniformResourceIndex(subset.material)].Load<ShaderMaterial>(0);
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
				shadow = TraceRay_Any(newRay, groupIndex) ? 0 : shadow;
#endif // RTAPI
				if (any(shadow))
				{
					lighting.direct.specular = lightColor * BRDF_GetSpecular(surface, surfaceToLight);
					lighting.direct.diffuse = lightColor * BRDF_GetDiffuse(surface, surfaceToLight);
					result += max(0, shadow * (surface.albedo * (1 - surface.transmission) * lighting.direct.diffuse + lighting.direct.specular)) * surface.opacity;
				}
			}
		}


		if (bounce == 0)
		{
			primary_albedo = surface.albedo;
			primary_normal = surface.N;
		}

		float roulette;

		const float blendChance = 1 - surface.opacity;
		roulette = rand(seed, uv);
		if (roulette < blendChance)
		{
			// Alpha blending

			// Add a new bounce iteration, otherwise the transparent effect can disappear:
			bounces++;

			continue; // skip light sampling
		}
		else
		{
			const float refractChance = surface.transmission;
			roulette = rand(seed, uv);
			if (roulette <= refractChance)
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
				const float specChance = dot(surface.F, 0.333);

				roulette = rand(seed, uv);
				if (roulette <= specChance)
				{
					// Specular reflection
					const float3 R = reflect(ray.Direction, surface.N);
					ray.Direction = lerp(R, SampleHemisphere_cos(R, seed, uv), surface.roughnessBRDF);
					energy *= surface.F / max(0.00001, specChance);
				}
				else
				{
					// Diffuse reflection
					ray.Direction = SampleHemisphere_cos(surface.N, seed, uv);
					energy *= surface.albedo / max(0.00001, 1 - specChance);
				}

				if (dot(ray.Direction, surface.facenormal) <= 0)
				{
					// Don't allow normal map to bend over the face normal more than 90 degrees to avoid light leaks
					//	In this case, the ray is pushed above the surface slightly to not go below
					ray.Origin += surface.facenormal * 0.001;
				}
			}
		}

	}

	// Pre-clear result texture for first bounce and first accumulation sample:
	if (xTraceSampleIndex == 0)
	{
		output[pixel] = 0;
		output_albedo[pixel] = 0;
		output_normal[pixel] = 0;
	}
	output[pixel] = lerp(output[pixel], float4(result, 1), xTraceAccumulationFactor);
	output_albedo[pixel] = lerp(output_albedo[pixel], float4(primary_albedo, 1), xTraceAccumulationFactor);
	output_normal[pixel] = lerp(output_normal[pixel], float4(primary_normal, 1), xTraceAccumulationFactor);
}
