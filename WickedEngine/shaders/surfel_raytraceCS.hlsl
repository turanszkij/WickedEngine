#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_SurfelGI.h"


void MultiscaleMeanEstimator(
	float3 y,
	inout SurfelData data,
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

STRUCTUREDBUFFER(surfelBuffer, Surfel, TEXSLOT_ONDEMAND0);
RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(surfelGridBuffer, SurfelGridCell, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(surfelCellBuffer, uint, TEXSLOT_ONDEMAND3);

RWSTRUCTUREDBUFFER(surfelDataBuffer, SurfelData, 0);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = DTid.x;
	Surfel surfel = surfelBuffer[surfel_index];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];


	float3 N = normalize(unpack_unitvector(surfel.normal));

	float seed = 0.123456;
	float2 uv = float2(frac(g_xFrame_FrameCount.xx / 4096.0));

	float4 gi = 0;
	uint samplecount = (uint)lerp(1.0, 32.0, pow(saturate(surfel_data.inconsistency), 4));
	for (uint sam = 0; sam < max(1, samplecount); ++sam)
	{
		RayDesc ray;
		ray.Origin = surfel.position;
		ray.TMin = 0.001;
		ray.TMax = FLT_MAX;
		ray.Direction = SampleHemisphere_cos(N, seed, uv);

		float3 result = 0;
		float3 energy = 1;

		uint bounces = 1;
		const uint bouncelimit = 1;
		for (uint bounce = 0; ((bounce < min(bounces, bouncelimit)) && any(energy)); ++bounce)
		{
			// Sample primary ray (scene materials, sky, etc):
			ray.Direction = normalize(ray.Direction);

#ifdef RTAPI
			RayQuery<
				RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
			> q;
			q.TraceRayInline(
				scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
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

			Surface surface;

#ifdef RTAPI

			// ray origin updated for next bounce:
			ray.Origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();

			PrimitiveID prim;
			prim.primitiveIndex = q.CommittedPrimitiveIndex();
			prim.instanceIndex = q.CommittedInstanceID();
			prim.subsetIndex = q.CommittedGeometryIndex();

			surface.load(prim, q.CommittedTriangleBarycentrics());

#else

			// ray origin updated for next bounce:
			ray.Origin = ray.Origin + ray.Direction * hit.distance;

			surface.load(hit.primitiveID, hit.bary);

#endif // RTAPI

			surface.P = ray.Origin;
			surface.V = -ray.Direction;
			surface.update();

			result += max(0, energy * surface.emissiveColor.rgb * surface.emissiveColor.a);

			// Calculate chances of reflection types:
			const float refractChance = surface.transmission;

			// Roulette-select the ray's path
			float roulette = rand(seed, uv);
			if (roulette < refractChance)
			{
				// Refraction
				const float3 R = refract(ray.Direction, surface.N, 1 - surface.material.refraction);
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








#if 1
			[loop]
			for (uint iterator = 0; iterator < g_xFrame_LightArrayCount; iterator++)
			{
				ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + iterator];

				Lighting lighting;
				lighting.create(0, 0, 0, 0);

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
						PrimitiveID prim;
						prim.primitiveIndex = q.CandidatePrimitiveIndex();
						prim.instanceIndex = q.CandidateInstanceID();
						prim.subsetIndex = q.CandidateGeometryIndex();

						Surface surface;
						surface.load(prim, q.CandidateTriangleBarycentrics());

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

#else



			Lighting lighting;
			lighting.create(0, 0, 0, 0);

			[loop]
			for (uint iterator = 0; iterator < g_xFrame_LightArrayCount; iterator++)
			{
				ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + iterator];
				if ((light.layerMask & surface.material.layerMask) == 0)
					continue;

				switch (light.GetType())
				{
				case ENTITY_TYPE_DIRECTIONALLIGHT:
				{
					DirectionalLight(light, surface, lighting);
				}
				break;
				case ENTITY_TYPE_POINTLIGHT:
				{
					PointLight(light, surface, lighting);
				}
				break;
				case ENTITY_TYPE_SPOTLIGHT:
				{
					SpotLight(light, surface, lighting);
				}
				break;
				}
			}

			result += max(0, lighting.direct.diffuse * energy);



#endif








#if 1
			float4 surfel_gi = 0;
			uint cellindex = surfel_cellindex(surfel_gridpos(surface.P));
			SurfelGridCell cell = surfelGridBuffer[cellindex];
			for (uint i = 0; i < cell.count; ++i)
			{
				uint surfel_index = surfelCellBuffer[cell.offset + i];
				Surfel surfel = surfelBuffer[surfel_index];

				float3 L = surfel.position - surface.P;
				float dist2 = dot(L, L);
				if (dist2 <= sqr(GetSurfelRadius()))
				{
					float3 normal = normalize(unpack_unitvector(surfel.normal));
					float dotN = dot(surface.N, normal);
					if (dotN > 0)
					{
						float dist = sqrt(dist2);
						float contribution = 1;
						contribution *= saturate(1 - dist / GetSurfelRadius());
						contribution = smoothstep(0, 1, contribution);
						contribution *= pow(saturate(dotN), SURFEL_NORMAL_TOLERANCE);

						surfel_gi += surfel.color * contribution;

					}
				}
			}
			if (surfel_gi.a > 0)
			{
				surfel_gi /= surfel_gi.a;
				result += max(0, energy * surfel_gi.rgb);
				break;
			}
#endif





		}

		gi += float4(result, 1);
	}
	gi /= gi.a;



	MultiscaleMeanEstimator(gi.rgb, surfel_data, 0.08);

	surfel_data.life += g_xFrame_DeltaTime;
	surfelDataBuffer[surfel_index] = surfel_data;
}
