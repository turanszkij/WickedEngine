#define RTAPI
#define DISABLE_SOFT_SHADOWMAP
#define SURFACE_LOAD_MIPCONE
#define TEXTURE_SLOT_NONUNIFORM

#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"
#include "stochasticSSRHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "ShaderInterop_DDGI.h"

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<float4> output_rayIndirectDiffuse : register(u0);

struct RayPayload
{
	float3 data;
};

[numthreads(8, 4, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
	const float2 uv = ((float2)DTid.xy + 0.5) * postprocess.resolution_rcp;

	const uint downsampleFactor = 2;

	// This is necessary for accurate upscaling. This is so we don't reuse the same half-res pixels
	uint2 screenJitter = floor(blue_noise(uint2(0, 0)).xy * downsampleFactor);
	uint2 jitterPixel = screenJitter + DTid.xy * downsampleFactor;
	float2 jitterUV = (screenJitter + DTid.xy + 0.5f) * postprocess.resolution_rcp;

	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, jitterUV, 0);
	if (depth == 0)
		return;

	const float lineardepth = texture_lineardepth.SampleLevel(sampler_linear_clamp, jitterUV, 0);
	const float roughness = 1;

	const float3 N = decode_oct(texture_normal[jitterPixel]);
	const float3 P = reconstruct_position(jitterUV, depth);
	const float3 V = normalize(GetCamera().position - P);

	RayPayload payload;
	payload.data = 0;

	//const float2 bluenoise = blue_noise(DTid.xy).xy;
	//const float3 R = normalize(mul(hemispherepoint_cos(bluenoise.x, bluenoise.y), get_tangentspace(N)));

	const uint samplecount = 2;
	for (uint i = 0; i < samplecount; ++i)
	{
		const float2 bluenoise = blue_noise(DTid.xy, (float)i / (float)samplecount).xy;
		const float3 R = normalize(mul(hemispherepoint_cos(bluenoise.x, bluenoise.y), get_tangentspace(N)));

		RayDesc ray;
		ray.TMin = 0.01;
		ray.TMax = rtdiffuse_range;
		ray.Origin = P;
		ray.Direction = normalize(R);

		const float minraycone = 0.05;
		RayCone raycone = RayCone::from_spread_angle(pixel_cone_spread_angle_from_image_height(postprocess.resolution.y));
		raycone = raycone.propagate(sqr(max(minraycone, roughness)), lineardepth * GetCamera().z_far);

		float4 additive_dist = float4(0, 0, 0, FLT_MAX);

		wiRayQuery q;
		q.TraceRayInline(
			scene_acceleration_structure,	// RaytracingAccelerationStructure AccelerationStructure
			RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
			RAY_FLAG_CULL_BACK_FACING_TRIANGLES, // uint RayFlags
			asuint(postprocess.params1.x),	// uint InstanceInclusionMask
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
			surface.V = -ray.Direction;
			surface.raycone = raycone;
			surface.hit_depth = q.CandidateTriangleRayT();
			if (!surface.load(prim, q.CandidateTriangleBarycentrics()))
				break;

			float alphatest = clamp(blue_noise(DTid.xy, q.CandidateTriangleRayT()).r, 0, 0.99);

			if (surface.material.IsAdditive())
			{
				additive_dist.xyz += surface.emissiveColor;
				additive_dist.w = min(additive_dist.w, q.CandidateTriangleRayT());
			}
			else if (surface.opacity - alphatest >= 0)
			{
				q.CommitNonOpaqueTriangleHit();
			}
		}

		if (additive_dist.w <= q.CommittedRayT())
		{
			payload.data.xyz += max(0, additive_dist.xyz);
		}

		if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
		{
			// miss:
			[branch]
			if (GetScene().ddgi.color_texture >= 0)
			{
				payload.data += ddgi_sample_irradiance(P, N);
			}
			else if (GetFrame().options & OPTION_BIT_SURFELGI_ENABLED && GetCamera().texture_surfelgi_index >= 0 && surfel_cellvalid(surfel_cell(P)))
			{
				payload.data += bindless_textures[GetCamera().texture_surfelgi_index][DTid.xy * 2].rgb * GetFrame().gi_boost;
			}
			else
			{
				payload.data.xyz += GetAmbient(q.WorldRayDirection());
			}
		}
		else
		{
			// closest hit:
			PrimitiveID prim;
			prim.primitiveIndex = q.CommittedPrimitiveIndex();
			prim.instanceIndex = q.CommittedInstanceID();
			prim.subsetIndex = q.CommittedGeometryIndex();

			Surface surface;
			surface.init();
			surface.SetBackface(!q.CommittedTriangleFrontFace());
			surface.V = -ray.Direction;
			surface.raycone = raycone;
			surface.hit_depth = q.CommittedRayT();
			if (!surface.load(prim, q.CommittedTriangleBarycentrics()))
				return;

			surface.pixel = DTid.xy;
			surface.screenUV = (surface.pixel + 0.5) * postprocess.resolution_rcp.xy;

			if (surface.material.IsUnlit())
			{
				payload.data.xyz = surface.albedo + surface.emissiveColor;
			}
			else
			{
				// Light sampling:
				surface.P = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();
				surface.V = -q.WorldRayDirection();
				surface.update();

				Lighting lighting;
				lighting.create(0, 0, 0, 0);

				[loop]
				for (uint iterator = 0; iterator < GetFrame().lightarray_count; iterator++)
				{
					ShaderEntity light = load_entity(GetFrame().lightarray_offset + iterator);
					if ((light.layerMask & surface.material.layerMask) == 0)
						continue;

					if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
					{
						continue; // static lights will be skipped (they are used in lightmap baking)
					}

					switch (light.GetType())
					{
					case ENTITY_TYPE_DIRECTIONALLIGHT:
					{
						light_directional(light, surface, lighting);
					}
					break;
					case ENTITY_TYPE_POINTLIGHT:
					{
						light_point(light, surface, lighting);
					}
					break;
					case ENTITY_TYPE_SPOTLIGHT:
					{
						light_spot(light, surface, lighting);
					}
					break;
					}
				}

				lighting.indirect.specular += surface.emissiveColor;

				[branch]
				if (GetScene().ddgi.color_texture >= 0)
				{
					lighting.indirect.diffuse = ddgi_sample_irradiance(surface.P, surface.N);
				}

				float4 color = 0;
				ApplyLighting(surface, lighting, color);
				payload.data.xyz += color.rgb;
			}
		}
	}

	payload.data /= (float)samplecount;

	output_rayIndirectDiffuse[DTid.xy] = float4(payload.data.xyz, 1);
}
