#define RAY_BACKFACE_CULLING
#define TEXTURE_SLOT_NONUNIFORM
#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "stochasticSSRHF.hlsli"

//#define DEBUG_CHARTS

// This value specifies after which bounce the anyhit will be disabled:
static const uint ANYTHIT_CUTOFF_AFTER_BOUNCE_COUNT = 4;

struct Input
{
	float4 pos : SV_POSITION;
	centroid float3 pos3D : WORLDPOSITION;
	centroid float3 normal : NORMAL;

#ifdef DEBUG_CHARTS
	uint primitiveID : SV_PrimitiveID;
#endif // DEBUG_CHARTS
};

static const float2 tangent_directions[] = {
	float2(1, 0),
	float2(-1, 0),
	float2(0, 1),
	float2(0, -1),
};

// Bakery pixel pushing: https://ndotl.wordpress.com/2018/08/29/baking-artifact-free-lightmaps/
//	This can push position outside of enclosed area within a pixel to remove shadow leaks
//	Instead the shadow texel reaching outside, this will make light go inside which is better in most cases
void BakeryPixelPush(inout float3 P, in float3 N, in float2 UV, inout RNG rng, inout float bakerydebug)
{
	float3 dUV1 = max(abs(ddx(P)), abs(ddy(P)));
	float dPos = max(max(dUV1.x, dUV1.y), dUV1.z);
	dPos = dPos * SQRT2; // convert to diagonal (small overshoot)
	
	float3x3 TBN = compute_tangent_frame(N, P, UV);

	for (uint i = 0; i < arraysize(tangent_directions); ++i)
	{
		RayDesc ray;
		ray.Origin = P + N * 0.0001;
		ray.Direction = normalize(mul(float3(tangent_directions[i], 1), TBN));
		ray.TMin = 0.0001;
		ray.TMax = dPos;
	
		bool backface_hit = false;
		float3 hit_pos = 0;
		float3 hit_nor = 0;
	
		Surface surface;
		surface.init();
		surface.V = -ray.Direction;

#ifdef RTAPI
		uint flags = 0;
		wiRayQuery q;
		q.TraceRayInline(
			scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
			flags,							// uint RayFlags
			xTraceUserData.y,				// uint InstanceInclusionMask
			ray								// RayDesc Ray
		);
		while (q.Proceed());
		if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT && !q.CommittedTriangleFrontFace())
		{
			backface_hit = true;
			hit_pos = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();
		
			PrimitiveID prim;
			prim.init();
			prim.primitiveIndex = q.CommittedPrimitiveIndex();
			prim.instanceIndex = q.CommittedInstanceID();
			prim.subsetIndex = q.CommittedGeometryIndex();

			surface.SetBackface(!q.CommittedTriangleFrontFace());

			surface.hit_depth = q.CommittedRayT();
			if (!surface.load(prim, q.CommittedTriangleBarycentrics()))
				return;

			hit_nor = surface.facenormal;
		}
#else
		RayHit hit = TraceRay_Closest(ray, xTraceUserData.y, rng);
		if (hit.distance < FLT_MAX && hit.is_backface)
		{
			backface_hit = true;
			hit_pos = ray.Origin + ray.Direction * hit.distance;
		
			surface.SetBackface(hit.is_backface);

			surface.hit_depth = hit.distance;
			if (!surface.load(hit.primitiveID, hit.bary))
				return;

			hit_nor = surface.facenormal;
		}
#endif // RTAPI

		if (backface_hit)
		{
			bakerydebug = 1;
			P = hit_pos - hit_nor * 0.001;
			return;
		}
	}
}

[earlydepthstencil]
float4 main(Input input) : SV_TARGET
{
#ifdef DEBUG_CHARTS
	return float4(random_color(input.primitiveID), 1);
#endif // DEBUG_CHARTS

	float2 uv = input.pos.xy * xTraceResolution_rcp;

	Surface surface;
	surface.init();
	surface.N = normalize(input.normal);

	RNG rng;
	rng.init((uint2)input.pos.xy, xTraceSampleIndex);

	float3 P = input.pos3D;

	float bakerydebug = 0;
	BakeryPixelPush(P, surface.N, uv, rng, bakerydebug);
	
	RayDesc ray;
	ray.Origin = P;
	ray.Direction = normalize(sample_hemisphere_cos(surface.N, rng));
	ray.TMin = 0.0001;
	ray.TMax = FLT_MAX;
	float3 result = 0;
	float3 energy = 1;

	const uint bounces = xTraceUserData.x;
	for (uint bounce = 0; bounce < bounces; ++bounce)
	{
#ifdef RTAPI
		wiRayQuery q;
#endif // RTAPI

		surface.P = ray.Origin;

		// Light sampling:
		{
			const uint light_count = lights().item_count();
			const uint light_index = lights().first_item() + rng.next_uint(light_count);
			ShaderEntity light = load_entity(light_index);

			if (bounce > 0 || light.IsStaticLight()) // dynamic lights will not be baked into lightmap at first bounce
			{
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
						float3 atmosphereTransmittance = 1.0;
						if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
						{
							atmosphereTransmittance = GetAtmosphericLightTransmittance(GetWeather().atmosphere, surface.P, L, texture_transmittancelut);
						}

						float3 lightColor = light.GetColor().rgb * atmosphereTransmittance;

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
				case ENTITY_TYPE_RECTLIGHT:
				{
					const half4 quaternion = light.GetQuaternion();
					const half3 right = rotate_vector(half3(1, 0, 0), quaternion);
					const half3 up = rotate_vector(half3(0, 1, 0), quaternion);
					const half3 forward = cross(up, right);
					if (dot(surface.P - light.position, forward) <= 0)
						break; // behind light
					light.position += right * (rng.next_float() - 0.5) * light.GetLength();
					light.position += up * (rng.next_float() - 0.5) * light.GetHeight();
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
					float3 shadow = energy;

					RayDesc newRay;
					newRay.Origin = surface.P;
					newRay.TMin = 0.0001;
					newRay.TMax = dist;
					newRay.Direction = normalize(L + max3(surface.sss));

#ifdef RTAPI
					uint flags = RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_CULL_FRONT_FACING_TRIANGLES;
					if (bounce > ANYTHIT_CUTOFF_AFTER_BOUNCE_COUNT)
					{
						flags |= RAY_FLAG_FORCE_OPAQUE;
					}

					q.TraceRayInline(
						scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
						flags,							// uint RayFlags
						xTraceUserData.y,				// uint InstanceInclusionMask
						newRay							// RayDesc Ray
					);
					while (q.Proceed())
					{
						PrimitiveID prim;
						prim.init();
						prim.primitiveIndex = q.CandidatePrimitiveIndex();
						prim.instanceIndex = q.CandidateInstanceID();
						prim.subsetIndex = q.CandidateGeometryIndex();

						Surface surface;
						surface.init();
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
					shadow = TraceRay_Any(newRay, xTraceUserData.y, rng) ? 0 : shadow;
#endif // RTAPI
					if (any(shadow))
					{
						const float3 albedo = bounce == 0 ? 1 : surface.albedo; // bounce 0 is the direct light, it will be multiplied by albedo in main rendering
						result += light_count * albedo / PI * lighting.direct.diffuse * shadow * NdotL;
					}
				}
			}
		}

		// Sample primary ray (scene materials, sky, etc):
		ray.Direction = normalize(ray.Direction);

#ifdef RTAPI

		uint flags = 0;
#ifdef RAY_BACKFACE_CULLING
		flags |= RAY_FLAG_CULL_BACK_FACING_TRIANGLES;
#endif // RAY_BACKFACE_CULLING
		if (bounce > ANYTHIT_CUTOFF_AFTER_BOUNCE_COUNT)
		{
			flags |= RAY_FLAG_FORCE_OPAQUE;
		}

		q.TraceRayInline(
			scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
			flags,							// uint RayFlags
			xTraceUserData.y,				// uint InstanceInclusionMask
			ray								// RayDesc Ray
		);
		while (q.Proceed());
		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
#else
		RayHit hit = TraceRay_Closest(ray, xTraceUserData.y, rng);

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
			result += max(0, energy * envColor);
			break;
		}

#ifdef RTAPI
		// ray origin updated for next bounce:
		ray.Origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();

		PrimitiveID prim;
		prim.init();
		prim.primitiveIndex = q.CommittedPrimitiveIndex();
		prim.instanceIndex = q.CommittedInstanceID();
		prim.subsetIndex = q.CommittedGeometryIndex();

		surface.SetBackface(!q.CommittedTriangleFrontFace());

		if (!surface.load(prim, q.CommittedTriangleBarycentrics()))
			return 0;

#else
		// ray origin updated for next bounce:
		ray.Origin = ray.Origin + ray.Direction * hit.distance;

		surface.SetBackface(hit.is_backface);

		if (!surface.load(hit.primitiveID, hit.bary))
			return 0;

#endif // RTAPI

		surface.update();

		result += energy * surface.emissiveColor;

		if (rng.next_float() < surface.transmission)
		{
			// Refraction
			const float3 R = refract(ray.Direction, surface.N, 1 - surface.material.GetRefraction());
			float roughnessBRDF = sqr(clamp(surface.roughness, min_roughness, 1));
			ray.Direction = lerp(R, sample_hemisphere_cos(R, rng), roughnessBRDF);
			energy *= surface.albedo / max(0.001, surface.transmission);

			// Add a new bounce iteration, otherwise the transparent effect can disappear:
			bounce--;
		}
		else
		{
			const float specular_chance = dot(surface.F, 0.333);
			if (rng.next_float() < specular_chance)
			{
				// Specular reflection
				ray.Direction = ReflectionDir_GGX(-ray.Direction, surface.N, surface.roughness, rng.next_float2()).xyz;
				energy *= surface.F / max(0.001, specular_chance) / max(0.001, 1 - surface.transmission);
			}
			else
			{
				// Diffuse reflection
				ray.Direction = sample_hemisphere_cos(surface.N, rng);
				energy *= surface.albedo * (1 - surface.F) / max(0.001, 1 - specular_chance) / max(0.001, 1 - surface.transmission);
			}
		}

		// Terminate ray's path or apply inverse termination bias:
		const float termination_chance = max3(energy);
		if (rng.next_float() > termination_chance)
		{
			break;
		}
		energy /= termination_chance;

	}

	//if(bakerydebug > 0)
	//	result = float3(1,0,0);
	
	return float4(result, xTraceAccumulationFactor);
}
