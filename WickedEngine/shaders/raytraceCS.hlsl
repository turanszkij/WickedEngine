#define RAY_BACKFACE_CULLING
#define RAYTRACE_STACK_SHARED
#define SURFACE_LOAD_MIPCONE
#define SVT_FEEDBACK
#define TEXTURE_SLOT_NONUNIFORM
#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "stochasticSSRHF.hlsli"

// This value specifies after which bounce the anyhit will be disabled:
static const uint ANYTHIT_CUTOFF_AFTER_BOUNCE_COUNT = 1;

RWTexture2D<float4> output : register(u0);
RWTexture2D<float4> output_albedo : register(u1);
RWTexture2D<float4> output_normal : register(u2);
RWTexture2D<float> output_depth : register(u3);
RWTexture2D<uint> output_stencil : register(u4);

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

	RNG rng;
	rng.init(pixel.xy, xTraceSampleIndex);

	// for denoiser:
	float3 primary_albedo = 0;
	float3 primary_normal = 0;

	// Compute screen coordinates:
	float2 uv = (pixel + xTracePixelOffset) * xTraceResolution_rcp.xy;

	// Create starting ray:
	RayDesc ray = CreateCameraRay(uv_to_clipspace(uv));

	// Depth of field setup:
	float3 focal_point = ray.Origin + ray.Direction * GetCamera().focal_length;
	float3 coc = float3(hemispherepoint_cos(rng.next_float(), rng.next_float()).xy, 0);
	coc.xy *= GetCamera().aperture_shape.xy;
	coc = mul(coc, float3x3(cross(GetCamera().up, GetCamera().forward), GetCamera().up, GetCamera().forward));
	coc *= GetCamera().focal_length;
	coc *= GetCamera().aperture_size;
	coc *= 0.1f;
	ray.Origin = ray.Origin + coc;
	ray.Direction = focal_point - ray.Origin; // will be normalized before tracing!

	ray_clip_plane(ray, GetCamera().clip_plane);

	RayCone raycone = pixel_ray_cone_from_image_height(xTraceResolution.y);

	float depth = 0;
	uint stencil = 0;

	const uint bounces = xTraceUserData.x;
	for (uint bounce = 0; bounce < bounces; ++bounce)
	{
		ray.Direction = normalize(ray.Direction);

		float4 additive_dist = float4(0, 0, 0, FLT_MAX);

#ifdef RTAPI
		wiRayQuery q;

		uint flags = RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
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
		while (q.Proceed())
		{
			PrimitiveID prim;
			prim.primitiveIndex = q.CandidatePrimitiveIndex();
			prim.instanceIndex = q.CandidateInstanceID();
			prim.subsetIndex = q.CandidateGeometryIndex();

			Surface surface;
			surface.init();
			surface.V = ray.Direction;
			surface.raycone = raycone;
			surface.hit_depth = q.CandidateTriangleRayT();
			if (!surface.load(prim, q.CandidateTriangleBarycentrics()))
				break;

			if (surface.material.IsAdditive())
			{
				additive_dist.xyz += energy * surface.emissiveColor;
				additive_dist.w = min(additive_dist.w, q.CandidateTriangleRayT());
			}
			else if (surface.opacity - rng.next_float() >= 0)
			{
				q.CommitNonOpaqueTriangleHit();
			}
		}

		if (additive_dist.w <= q.CommittedRayT())
		{
			result += max(0, energy * additive_dist.xyz);
		}

		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
#else
		RayHit hit = TraceRay_Closest(ray, xTraceUserData.y, rng, groupIndex);

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
				envColor = GetDynamicSkyColor(ray.Direction);
			}
			result += max(0, energy * envColor);
			break;
		}

		Surface surface;
		surface.init();
		surface.V = -ray.Direction;
		surface.raycone = raycone;

#ifdef RTAPI
		// ray origin updated for next bounce:
		ray.Origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();

		PrimitiveID prim;
		prim.primitiveIndex = q.CommittedPrimitiveIndex();
		prim.instanceIndex = q.CommittedInstanceID();
		prim.subsetIndex = q.CommittedGeometryIndex();

		surface.SetBackface(!q.CommittedTriangleFrontFace());

		surface.hit_depth = q.CommittedRayT();
		if (!surface.load(prim, q.CommittedTriangleBarycentrics()))
			return;

#else
		// ray origin updated for next bounce:
		ray.Origin = ray.Origin + ray.Direction * hit.distance;

		surface.SetBackface(hit.is_backface);

		surface.hit_depth = hit.distance;
		if (!surface.load(hit.primitiveID, hit.bary))
			return;

#endif // RTAPI

		surface.P = ray.Origin;
		surface.V = -ray.Direction;
		surface.update();

		result += energy * surface.emissiveColor;

		float roughnessBRDF = sqr(clamp(surface.roughness, min_roughness, 1));
		raycone = raycone.propagate(roughnessBRDF, surface.hit_depth);

		if (bounce == 0)
		{
			primary_albedo = surface.albedo;
			primary_normal = surface.N;
			stencil = surface.material.GetStencilRef();
			uint userStencilRefOverride = surface.inst.GetUserStencilRef();
			if (userStencilRefOverride > 0)
			{
				stencil = (userStencilRefOverride << 4u) | (stencil & 0xF);
			}
			float4 tmp = mul(GetCamera().view_projection, float4(surface.P, 1));
			tmp.xyz /= max(0.0001, tmp.w); // max: avoid nan
			depth = saturate(tmp.z); // saturate: avoid blown up values
		}

		if (surface.material.IsUnlit())
		{
			result += surface.albedo * energy;
			break;
		}

		// Light sampling:
		if (!surface.material.IsUnlit())
		{
			const uint light_count = GetFrame().lightarray_count;
			const uint light_index = GetFrame().lightarray_offset + rng.next_uint(light_count);
			ShaderEntity light = load_entity(light_index);

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
				L += sample_hemisphere_cos(L, rng) * light.GetRadius();

				surfaceToLight.create(surface, L);

				[branch]
				if (any(surfaceToLight.NdotL_sss))
				{
					lightColor = light.GetColor().rgb;

					[branch]
					if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
					{
						lightColor *= GetAtmosphericLightTransmittance(GetWeather().atmosphere, surface.P, L, texture_transmittancelut);
					}
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

					surfaceToLight.create(surface, L);

					[branch]
					if (any(surfaceToLight.NdotL_sss))
					{
						lightColor = light.GetColor().rgb;
						lightColor *= attenuation_pointlight(dist2, range, range2);
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

					surfaceToLight.create(surface, L);

					[branch]
					if (any(surfaceToLight.NdotL_sss))
					{
						const float spot_factor = dot(Loriginal, light.GetDirection());
						const float spot_cutoff = light.GetConeAngleCos();

						[branch]
						if (spot_factor > spot_cutoff)
						{
							lightColor = light.GetColor().rgb;
							lightColor *= attenuation_spotlight(dist2, range, range2, spot_factor, light.GetAngleScale(), light.GetAngleOffset());
						}
					}
				}
			}
			break;
			}

			if (any(surfaceToLight.NdotL_sss) && dist > 0)
			{
				float3 shadow = energy;

				if(light.IsCastingShadow() && surface.IsReceiveShadow())
				{
					RayDesc newRay;
					newRay.Origin = surface.P;
					newRay.TMin = 0.001;
					newRay.TMax = dist;
					newRay.Direction = L + max3(surface.sss);

#ifdef RTAPI

					uint flags = RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_CULL_FRONT_FACING_TRIANGLES;
					if (bounce > ANYTHIT_CUTOFF_AFTER_BOUNCE_COUNT)
					{
						flags |= RAY_FLAG_FORCE_OPAQUE;
					}

					q.TraceRayInline(
						scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
						flags,							// uint RayFlags
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
						surface.init();
						surface.hit_depth = q.CandidateTriangleRayT();
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
					shadow = TraceRay_Any(newRay, xTraceUserData.y, rng, groupIndex) ? 0 : shadow;
#endif // RTAPI
				}
				
				if (any(shadow))
				{
					lightColor *= shadow;
					lighting.direct.diffuse = lightColor * BRDF_GetDiffuse(surface, surfaceToLight);
					lighting.direct.specular = lightColor * BRDF_GetSpecular(surface, surfaceToLight);
					result += light_count * mad(surface.albedo / PI * (1 - surface.transmission), lighting.direct.diffuse, lighting.direct.specular) * surface.opacity;
				}
			}
		}

		// Terminate ray's path or apply inverse termination bias:
		const float termination_chance = max3(energy);
		if (rng.next_float() > termination_chance)
		{
			break;
		}
		energy /= termination_chance;

		// Set up next bounce:
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

			if (dot(ray.Direction, surface.facenormal) <= 0)
			{
				// Don't allow normal map to bend over the face normal more than 90 degrees to avoid light leaks
				//	In this case, the ray is pushed above the surface slightly to not go below
				ray.Origin += surface.facenormal * 0.001;
			}
		}

	}

	output[pixel] = lerp(output[pixel], float4(result, 1), xTraceAccumulationFactor);
	output_albedo[pixel] = lerp(output_albedo[pixel], float4(primary_albedo, 1), xTraceAccumulationFactor);
	output_normal[pixel] = lerp(output_normal[pixel], float4(primary_normal, 1), xTraceAccumulationFactor);

	if (xTraceSampleIndex == 0)
	{
		output_depth[pixel] = depth;
		output_stencil[pixel] = stencil;
	}
}
