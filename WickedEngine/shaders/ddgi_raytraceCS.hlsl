#include "globals.hlsli"
#include "raytracingHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_DDGI.h"

PUSHCONSTANT(push, PushConstantsDDGIRaytrace);

RWTexture2D<float3> ddgiColorTextureRW : register(u0);
RWTexture2D<float2> ddgiDepthTextureRW : register(u1);

[numthreads(4, 4, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	Texture2D ddgiColorTextureRO = bindless_textures[GetScene().ddgi_color_texture];
	Texture2D ddgiDepthTextureRO = bindless_textures[GetScene().ddgi_color_texture];

	const uint3 probeCoord = DTid.xyz;
	const float3 probePos = ddgi_probe_position(probeCoord);
	const float3 cellSize = ddgi_cellsize();
	const float maxDepth = 1;
	const float blend_speed = 0.005;

	// Initialize current color texture:
	const uint2 pixel_topleft_color = ddgi_probe_color_pixel(probeCoord);
	{
		for (uint x = 0; x < DDGI_COLOR_RESOLUTION; ++x)
		{
			for (uint y = 0; y < DDGI_COLOR_RESOLUTION; ++y)
			{
				uint2 coord = pixel_topleft_color + uint2(x, y);
				if (push.frameIndex == 0)
				{
					ddgiColorTextureRW[coord] = 0;
				}
				else
				{
					ddgiColorTextureRW[coord] = ddgiColorTextureRO[coord].rgb;
				}
			}
		}
	}

	// Initialize current depth texture:
	const uint2 pixel_topleft_depth = ddgi_probe_depth_pixel(probeCoord);
	{
		for (uint x = 0; x < DDGI_DEPTH_RESOLUTION; ++x)
		{
			for (uint y = 0; y < DDGI_DEPTH_RESOLUTION; ++y)
			{
				uint2 coord = pixel_topleft_depth + uint2(x, y);
				if (push.frameIndex == 0)
				{
					ddgiDepthTextureRW[coord] = FLT_MAX;
				}
				else
				{
					ddgiDepthTextureRW[coord] = ddgiDepthTextureRO[coord].rg;
				}
			}
		}
	}

	float seed = 0.123456;
	float2 uv = float2(frac(GetFrame().frame_count.x / 4096.0), flatten3D(probeCoord, DDGI_GRID_DIMENSIONS));

	for (uint i = 0; i < push.rayCount; ++i)
	{
		RayDesc ray;
		ray.Origin = probePos;
		ray.TMin = 0.0001;
		ray.TMax = FLT_MAX;
		//ray.Direction = normalize(decode_oct(float2(rand(seed, uv), rand(seed, uv)) * 2 - 1));
		ray.Direction = normalize(float3(rand(seed, uv), rand(seed, uv), rand(seed, uv)) * 2 - 1);

		const float2 oct_uv = encode_oct(ray.Direction) * 0.5 + 0.5;
		const uint2 coord_color = pixel_topleft_color + oct_uv * DDGI_COLOR_RESOLUTION;
		const uint2 coord_depth = pixel_topleft_depth + oct_uv * DDGI_DEPTH_RESOLUTION;

#ifdef RTAPI
		RayQuery<
			RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
			RAY_FLAG_FORCE_OPAQUE
		> q;
		q.TraceRayInline(
			scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
			0,								// uint RayFlags
			push.instanceInclusionMask,		// uint InstanceInclusionMask
			ray								// RayDesc Ray
		);
		q.Proceed();
		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
#else
		RayHit hit = TraceRay_Closest(ray, push.instanceInclusionMask, seed, uv);

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

			ddgiColorTextureRW[coord_color] = lerp(ddgiColorTextureRW[coord_color], envColor, blend_speed);
			ddgiDepthTextureRW[coord_depth] = lerp(ddgiDepthTextureRW[coord_depth].xy, float2(maxDepth, sqr(maxDepth)), blend_speed);
		}
		else
		{

			Surface surface;

			float hit_depth = 0;
			float3 hit_result = 0;

#ifdef RTAPI

			// ray origin updated for next bounce:
			ray.Origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();
			hit_depth = q.CommittedRayT();

			PrimitiveID prim;
			prim.primitiveIndex = q.CommittedPrimitiveIndex();
			prim.instanceIndex = q.CommittedInstanceID();
			prim.subsetIndex = q.CommittedGeometryIndex();

			if (!surface.load(prim, q.CommittedTriangleBarycentrics()))
				break;

#else

			// ray origin updated for next bounce:
			ray.Origin = ray.Origin + ray.Direction * hit.distance;
			hit_depth = hit.distance;

			if (!surface.load(hit.primitiveID, hit.bary))
				break;

#endif // RTAPI

			surface.P = ray.Origin;
			surface.V = -ray.Direction;
			surface.update();

#if 1
			// Light sampling:
			[loop]
			for (uint iterator = 0; iterator < GetFrame().lightarray_count; iterator++)
			{
				ShaderEntity light = load_entity(GetFrame().lightarray_offset + iterator);

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
						float3 lightColor = light.GetColor().rgb * light.GetEnergy();

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
					float3 shadow = NdotL;

					RayDesc newRay;
					newRay.Origin = surface.P;
#if 1
					newRay.Direction = normalize(lerp(L, sample_hemisphere_cos(L, seed, uv), 0.025f));
#else
					newRay.Direction = L;
#endif
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
					shadow = TraceRay_Any(newRay, push.instanceInclusionMask, seed, uv) ? 0 : shadow;
#endif // RTAPI
					if (any(shadow))
					{
						hit_result += max(0, shadow * lighting.direct.diffuse / PI);
					}
				}
			}

#endif

			hit_result += ddgi_sample_irradiance(surface.P, surface.facenormal);
			
			hit_result *= surface.albedo;
			hit_result += max(0, surface.emissiveColor);

			ddgiColorTextureRW[coord_color] = lerp(ddgiColorTextureRW[coord_color], hit_result, blend_speed);

			hit_depth = clamp(hit_depth, 0, maxDepth);
			ddgiDepthTextureRW[coord_depth] = lerp(ddgiDepthTextureRW[coord_depth].xy, float2(hit_depth, sqr(hit_depth)), blend_speed);

		}

	}

	// Copy color borders:
	const uint2 copy_coord_color = pixel_topleft_color - 1;
	for (uint index = 0; index < 36; ++index)
	{
		uint2 src_coord = copy_coord_color + DDGI_COLOR_BORDER_OFFSETS[index].xy;
		uint2 dst_coord = copy_coord_color + DDGI_COLOR_BORDER_OFFSETS[index].zw;
		ddgiColorTextureRW[dst_coord] = ddgiColorTextureRW[src_coord];
	}

	// Copy depth borders:
	const uint2 copy_coord_depth = pixel_topleft_depth - 1;
	for (index = 0; index < 68; ++index)
	{
		uint2 src_coord = copy_coord_depth + DDGI_DEPTH_BORDER_OFFSETS[index].xy;
		uint2 dst_coord = copy_coord_depth + DDGI_DEPTH_BORDER_OFFSETS[index].zw;
		ddgiDepthTextureRW[dst_coord] = ddgiDepthTextureRW[src_coord];
	}
}
