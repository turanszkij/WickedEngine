#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_SurfelGI.h"


STRUCTUREDBUFFER(surfelBuffer, Surfel, TEXSLOT_ONDEMAND0);
RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(surfelGridBuffer, SurfelGridCell, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(surfelCellBuffer, uint, TEXSLOT_ONDEMAND3);

RWSTRUCTUREDBUFFER(surfelDataBuffer, SurfelData, 0);
RWTEXTURE2D(surfelMomentsTexture, float2, 1);

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

	uint surfel_index = DTid.x;
	Surfel surfel = surfelBuffer[surfel_index];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];

	if (surfel_data.life == 0)
	{
		uint2 moments_pixel = unflatten2D(surfel_index, SQRT_SURFEL_CAPACITY) * SURFEL_MOMENT_TEXELS;
		for (int i = 0; i < SURFEL_MOMENT_TEXELS; ++i)
		{
			for (int j = 0; j < SURFEL_MOMENT_TEXELS; ++j)
			{
				uint2 pixel_write = moments_pixel + uint2(i, j);
				surfelMomentsTexture[pixel_write] = float2(surfel.radius, sqr(surfel.radius));
			}
		}
	}

	const float3 N = normalize(unpack_unitvector(surfel.normal));

	float seed = 0.123456;
	float2 uv = float2(frac(g_xFrame.FrameCount.x / 4096.0), (float)surfel_index / SURFEL_CAPACITY);

	RayDesc ray;
	ray.Origin = surfel.position;
	ray.TMin = 0.001;
	ray.TMax = FLT_MAX;
	ray.Direction = normalize(SampleHemisphere_cos(N, seed, uv));

	uint2 moments_pixel = surfel_moment_pixel(surfel_index, N, ray.Direction);
	float moment_blend = 0.5;

	float3 result = 0;
	float3 energy = 1;


#ifdef RTAPI
	RayQuery<
		RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
		RAY_FLAG_FORCE_OPAQUE
	> q;
	q.TraceRayInline(
		scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
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
		surfel_data.hitpos = 0;
		surfel_data.hitnormal = ~0;

		surfel_moments_write(moments_pixel, surfel.radius);

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
	}

	Surface surface;

	float hit_depth = 0;

#ifdef RTAPI

	// ray origin updated for next bounce:
	ray.Origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();
	hit_depth = q.CommittedRayT();

	PrimitiveID prim;
	prim.primitiveIndex = q.CommittedPrimitiveIndex();
	prim.instanceIndex = q.CommittedInstanceID();
	prim.subsetIndex = q.CommittedGeometryIndex();

	surface.load(prim, q.CommittedTriangleBarycentrics());

#else

	// ray origin updated for next bounce:
	ray.Origin = ray.Origin + ray.Direction * hit.distance;
	hit_depth = hit.distance;

	surface.load(hit.primitiveID, hit.bary);

#endif // RTAPI

	hit_depth = min(hit_depth, surfel.radius);
	surfel_moments_write(moments_pixel, hit_depth);

	surface.P = ray.Origin;
	surface.V = -ray.Direction;
	surface.update();

	result += max(0, energy * surface.emissiveColor.rgb * surface.emissiveColor.a);

	// Calculate chances of reflection types:
	const float specChance = dot(surface.F, 0.333f);

	float roulette = rand(seed, uv);
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








#if 1
	// Light sampling:
	[loop]
	for (uint iterator = 0; iterator < g_xFrame.LightArrayCount; iterator++)
	{
		ShaderEntity light = load_entity(g_xFrame.LightArrayOffset + iterator);

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
				float3 atmosphereTransmittance = 1.0;
				if (g_xFrame.Options & OPTION_BIT_REALISTIC_SKY)
				{
					atmosphereTransmittance = GetAtmosphericLightTransmittance(g_xFrame.Atmosphere, surface.P, L, texture_transmittancelut);
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
			q.TraceRayInline(
				scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
				RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,	// uint RayFlags
				0xFF,							// uint InstanceInclusionMask
				newRay							// RayDesc Ray
			);
			q.Proceed();
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

#endif


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

	surfel_data.hitpos = surface.P;
	surfel_data.hitnormal = pack_unitvector(surface.facenormal);
	surfel_data.hitenergy = energy;
	surfel_data.traceresult = result;

	surfel_data.life++;
	surfelDataBuffer[surfel_index] = surfel_data;
}
