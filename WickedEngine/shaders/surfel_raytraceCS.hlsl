#define TEXTURE_SLOT_NONUNIFORM
#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_SurfelGI.h"

PUSHCONSTANT(push, PushConstantsSurfelRaytrace);

StructuredBuffer<Surfel> surfelBuffer : register(t0);
StructuredBuffer<SurfelStats> surfelStatsBuffer : register(t1);
StructuredBuffer<SurfelGridCell> surfelGridBuffer : register(t2);
StructuredBuffer<uint> surfelCellBuffer : register(t3);
StructuredBuffer<uint> surfelAliveBuffer : register(t4);
Texture2D<float2> surfelMomentsTexturePrev : register(t5);
#ifdef SURFEL_RAY_SORTING
StructuredBuffer<uint> surfelRaySortPayloadBuffer : register(t6); // sorted -> original ray slot
#endif // SURFEL_RAY_SORTING

RWStructuredBuffer<SurfelRayDataPacked> surfelRayBuffer : register(u0);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// Bound to the budget: rayCount is the uncapped sum of all ray requests,
	// but only the first SURFEL_RAY_BUDGET ray slots are actually written.
	// Tracing past that processes stale slots (wasted work) and makes the
	// budget inert.
	uint global_ray_count = min(surfelStatsBuffer[0].rayCount, SURFEL_RAY_BUDGET);
	if (DTid.x >= global_ray_count)
		return;

	// With ray sorting, thread DTid.x processes the ray at the sorted position:
	// remap to its ORIGINAL slot so results still land where integrate reads
	// them (by each surfel's rayOffset). Consecutive threads now trace rays
	// from nearby surfels (Morton order) -> coherent BVH traversal. Without
	// sorting, identity.
#ifdef SURFEL_RAY_SORTING
	const uint ray_slot = surfelRaySortPayloadBuffer[DTid.x];
#else
	const uint ray_slot = DTid.x;
#endif // SURFEL_RAY_SORTING

	SurfelRayData rayData = surfelRayBuffer[ray_slot].load();

	uint surfel_index = rayData.surfelIndex;
	Surfel surfel = surfelBuffer[surfel_index];

	const float3 N = normalize(unpack_half3(surfel.normal));

	RNG rng;
	rng.init(ray_slot.xx, GetFrame().frame_count);
	
	float3 radiance = 0;
	
#if 1
	// Light sampling - direct static:
	{
		Surface surface;
		surface.init();
		surface.P = surfel.position;
		surface.N = normalize(sample_hemisphere_cos(N, rng));
		surface.update();
		
		const uint light_count = lights().item_count();
		const uint light_index = lights().first_item() + rng.next_uint(light_count);
		ShaderEntity light = load_entity(light_index);
		
		if (light.IsStaticLight())
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

						lighting.direct.diffuse = lightColor;
						lighting.direct.diffuse *= attenuation_pointlight(dist2, range, light.GetRange2Rcp());
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
						lighting.direct.diffuse *= attenuation_pointlight(dist2, range, light.GetRange2Rcp());
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
							lighting.direct.diffuse *= attenuation_spotlight(dist2, range, light.GetRange2Rcp(), spot_factor, light.GetAngleScale(), light.GetAngleOffset());
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
				newRay.Direction = normalize(L + max3(surface.sss));

	#ifdef RTAPI
				wiRayQuery q;
				q.TraceRayInline(
					scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
					RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
					RAY_FLAG_FORCE_OPAQUE |
					//RAY_FLAG_CULL_FRONT_FACING_TRIANGLES |
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
					radiance += light_count * max(0, shadow * lighting.direct.diffuse * NdotL / PI);
				}
			}
		}
	}

#endif

	{
		// Ray guiding: bias the indirect bounce toward where this surfel
		// already sees bright incoming light (the dominant direction of its
		// cached L1 SH radiance). The integrate pass reconstructs each
		// direction bin as a weighted average of nearby rays, so steering the
		// ray distribution needs no PDF reweighting. A cosine fallback keeps
		// exploration (so dark bins stay sampled and unbiased) and guarantees
		// the ray stays in the surface hemisphere. Guiding strength scales with
		// how directional the cached radiance is, so flat/unconverged surfels
		// fall back to pure cosine.
		float3 bounce_dir = normalize(sample_hemisphere_cos(N, rng));
		{
			SH::L1_RGB sh = surfel.radiance.Unpack();
			const float3 luma = float3(0.299, 0.587, 0.114);
			const float3 dom = float3(dot(sh.C[3], luma), dot(sh.C[1], luma), dot(sh.C[2], luma));
			const float dc = max(dot(sh.C[0], luma), 1e-4);
			const float strength = saturate(length(dom) / dc);
			if (strength > 0.05 && rng.next_float() < SURFEL_RAY_GUIDE_FRACTION * strength)
			{
				const float3 guided = normalize(sample_hemisphere_cos(normalize(dom), rng));
				if (dot(guided, N) > 0)
					bounce_dir = guided;
			}
		}

		RayDesc ray;
		ray.Origin = surfel.position;
		ray.TMin = 0.0001;
		ray.TMax = FLT_MAX;
		ray.Direction = bounce_dir;

		rayData.direction = ray.Direction;

#ifdef RTAPI
		wiRayQuery q;
		q.TraceRayInline(
			scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
			RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
			//RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
			RAY_FLAG_FORCE_OPAQUE,	// uint RayFlags
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
			radiance += envColor;
			
			rayData.radiance = max(0, radiance);
			rayData.depth = -1;
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
			prim.init();
			prim.primitiveIndex = q.CommittedPrimitiveIndex();
			prim.instanceIndex = q.CommittedInstanceID();
			prim.subsetIndex = q.CommittedGeometryIndex();

			surface.SetBackface(!q.CommittedTriangleFrontFace());
			if(!surface.load(prim, q.CommittedTriangleBarycentrics()))
				return;

#else

			// ray origin updated for next bounce:
			ray.Origin = ray.Origin + ray.Direction * hit.distance;
			hit_depth = hit.distance;

			surface.SetBackface(hit.is_backface);

			if (!surface.load(hit.primitiveID, hit.bary))
				return;

#endif // RTAPI

			if (surface.IsBackface())
			{
				hit_depth *= 0.5; // push inwards to help avoid shadow leaks from inwards to outside
			}

			surface.P = ray.Origin;
			surface.V = -ray.Direction;
			surface.update();

#if 1
			// Light sampling:
			{
				const uint light_count = lights().item_count();
				const uint light_index = lights().first_item() + rng.next_uint(light_count);
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

							lighting.direct.diffuse = lightColor;
							lighting.direct.diffuse *= attenuation_pointlight(dist2, range, light.GetRange2Rcp());
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
							lighting.direct.diffuse *= attenuation_pointlight(dist2, range, light.GetRange2Rcp());
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
								lighting.direct.diffuse *= attenuation_spotlight(dist2, range, light.GetRange2Rcp(), spot_factor, light.GetAngleScale(), light.GetAngleOffset());
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
					newRay.Direction = normalize(L + max3(surface.sss));

#ifdef RTAPI
					q.TraceRayInline(
						scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
						RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
						RAY_FLAG_FORCE_OPAQUE |
						//RAY_FLAG_CULL_FRONT_FACING_TRIANGLES |
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


#ifdef SURFEL_ENABLE_INFINITE_BOUNCES
			// Evaluate surfel cache at hit point for multi bounce:
			{
				float4 surfel_gi = 0;
				// Gather only the levels around the hit point's own level (same
				// as surfel_coverageCS): surfels here share surfel_level(P), so
				// +/-1 suffices and keeps this to <=3 levels.
				const uint base_level = surfel_level(surface.P);
				const uint level_lo = (base_level > 0) ? (base_level - 1) : 0;
				const uint level_hi = min(base_level + 1, SURFEL_GRID_LEVELS - 1);
				for (uint level = level_lo; level <= level_hi; ++level)
				{
				int3 gridpos = surfel_cell(surface.P, level);
				if (!surfel_cellvalid(gridpos))
					continue;
				SurfelGridCell cell = surfelGridBuffer[surfel_cellindex(gridpos, level)];
				for (uint i = 0; i < cell.count; ++i)
				{
					uint surfel_index = surfelCellBuffer[cell.offset + i];
					Surfel surfel = surfelBuffer[surfel_index];

					float3 L = surface.P - surfel.position;
					float dist2 = dot(L, L);
					if (dist2 < sqr(surfel.GetRadius()))
					{
						float3 normal = normalize(unpack_half3(surfel.normal));
						float dotN = dot(surface.N, normal);
						if (dotN > 0)
						{
							float dist = sqrt(dist2);

							// Smooth radial falloff, matching surfel_coverageCS
							// so the multi-bounce lookup weights surfels
							// identically.
							float falloff = saturate(1 - dist2 / sqr(surfel.GetRadius()));
							falloff *= falloff;
							float contribution = saturate(dotN) * falloff;

							float2 moments = surfelMomentsTexturePrev.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, L / dist), 0);
							contribution *= surfel_moment_weight(moments, dist);

							// max(0): L1 SH irradiance can ring negative away
							// from its dominant lobe; unphysical, and here it
							// would feed negative energy back into the surfel
							// cache via the multi-bounce (matching the coverage
							// gather clamp).
							surfel_gi += float4(max(0, SH::CalculateIrradiance(surfel.radiance.Unpack(), surface.N)), 1) * contribution;

						}
					}
				}
				}
				if (surfel_gi.a > 0)
				{
					float energy_conservation = 0.95;
					energy_conservation /= surfel_gi.a;
					energy_conservation /= PI;
					surfel_gi.rgb *= energy_conservation;
					hit_result += max(0, surfel_gi.rgb);
				}
			}
#endif // SURFEL_ENABLE_INFINITE_BOUNCES

			hit_result *= surface.albedo;
			hit_result += surface.emissiveColor;
			radiance += hit_result;

			rayData.radiance = radiance;
			rayData.depth = hit_depth;

		}

	}

	// Sanitize the ray radiance before it is stored/packed. A sky/sun hit (or a
	// stray firefly) can exceed the R11G11B10 pack range and unpack as +Inf;
	// the integrate pass's MultiscaleMeanEstimator firefly clamp then does Inf
	// - Inf = NaN, which permanently poisons the surfel's cached radiance and
	// spreads via the multi-bounce feedback (line ~560), and the coverage
	// gather reads it as NaN and flashes the whole grid cell black/white.
	// Replace any non-finite component with 0 and clamp to a finite max the
	// pack format represents, so Inf/NaN can never enter the surfel cache from
	// here.
	rayData.radiance = (any(isnan(rayData.radiance)) || any(isinf(rayData.radiance)))
		? (float3)0
		: min(rayData.radiance, SURFEL_RAY_RADIANCE_MAX);

	surfelRayBuffer[ray_slot].store(rayData);
}
