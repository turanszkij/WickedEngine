#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_Renderer.h"

void MultiscaleMeanEstimator(
	float3 y,
	inout Surfel data,
	float shortWindowBlend = 0.08f
)
{
	float3 mean = data.mean;
	float3 shortMean = data.shortMean;
	float vbbr = data.vbbr;
	float3 variance = data.variance;
	float inconsistency = data.inconsistency;

	// Suppress fireflies.
	{
		float3 dev = sqrt(max(1e-5, variance));
		float3 highThreshold = 0.1 + shortMean + dev * 8;
		float3 overflow = max(0, y - highThreshold);
		y -= overflow;
	}

	float3 delta = y - shortMean;
	shortMean = lerp(shortMean, y, shortWindowBlend);
	float3 delta2 = y - shortMean;

	// This should be a longer window than shortWindowBlend to avoid bias
	// from the variance getting smaller when the short-term mean does.
	float varianceBlend = shortWindowBlend * 0.5;
	variance = lerp(variance, delta * delta2, varianceBlend);
	float3 dev = sqrt(max(1e-5, variance));

	float3 shortDiff = mean - shortMean;

	float relativeDiff = dot(float3(0.299, 0.587, 0.114),
		abs(shortDiff) / max(1e-5, dev));
	inconsistency = lerp(inconsistency, relativeDiff, 0.08);

	float varianceBasedBlendReduction =
		clamp(dot(float3(0.299, 0.587, 0.114),
			0.5 * shortMean / max(1e-5, dev)), 1.0 / 32, 1);

	float3 catchUpBlend = clamp(smoothstep(0, 1,
		relativeDiff * max(0.02, inconsistency - 0.2)), 1.0 / 256, 1);
	catchUpBlend *= vbbr;

	vbbr = lerp(vbbr, varianceBasedBlendReduction, 0.1);
	mean = lerp(mean, y, saturate(catchUpBlend));

	// Output
	data.mean = mean;
	data.shortMean = shortMean;
	data.vbbr = vbbr;
	data.variance = variance;
	data.inconsistency = inconsistency;
}

RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND6);

RWSTRUCTUREDBUFFER(surfelBuffer, Surfel, 0);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	const float2 bluenoise = blue_noise(unflatten2D(DTid.x, 256)).xy;

	Surfel surfel = surfelBuffer[DTid.x];

	float seed = surfel.life;
	float2 uv = float2(g_xFrame_Time, (float)DTid.x / (float)surfel_count);

	Surface surface;
	surface.init();
	surface.N = normalize(unpack_unitvector(surfel.normal));

	float4 gi = 0;
	uint samplecount = (uint)lerp(8.0, 1.0, saturate(surfel.life));
	for (uint sam = 0; sam < max(1, samplecount); ++sam)
	{
		RayDesc ray;
		ray.Origin = surfel.position;
		ray.TMin = 0.001;
		ray.TMax = FLT_MAX;
		ray.Direction = SampleHemisphere_cos(surface.N, seed, uv);

		float3 result = 0;
		float3 energy = 1;

		uint bounces = 8;
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

				if (bounce == 0)
					continue;
				//if (!(light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC))
				//{
				//	continue; // dynamic lights will not be baked into lightmap
				//}

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
							atmosphereTransmittance = GetAtmosphericLightTransmittance(g_xFrame_Atmosphere, surface.P, L, texture_transmittancelut);
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
					newRay.Direction = normalize(lerp(L, SampleHemisphere_cos(L, seed, uv), 0.025f));
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
				const float specChance = dot(surface.F, 0.333f);

				roulette = rand(seed, uv);
				if (roulette < specChance)
				{
					// Specular reflection
					const float3 R = reflect(ray.Direction, surface.N);
					ray.Direction = lerp(R, SampleHemisphere_cos(R, seed, uv), surface.roughnessBRDF);
					energy *= surface.F / specChance;
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

		gi += float4(result, 1);
	}
	gi /= gi.a;

	MultiscaleMeanEstimator(gi.rgb, surfel, 0.09);

	surfel.life += g_xFrame_DeltaTime;
	surfelBuffer[DTid.x] = surfel;
}
