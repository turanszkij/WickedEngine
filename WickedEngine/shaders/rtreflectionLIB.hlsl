#define RTAPI
#define DISABLE_SOFT_SHADOWMAP
#define DISABLE_TRANSPARENT_SHADOWMAP

#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"
#include "stochasticSSRHF.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_DDGI.h"

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<float4> output_rayIndirectSpecular : register(u0);
RWTexture2D<float4> output_rayDirectionPDF : register(u1);
RWTexture2D<float> output_rayLengths : register(u2);

struct RayPayload
{
	float4 data;
};

#ifndef __spirv__
GlobalRootSignature MyGlobalRootSignature =
{
	WICKED_ENGINE_DEFAULT_ROOTSIGNATURE
};
#endif // __spirv__

[shader("raygeneration")]
void RTReflection_Raygen()
{
	uint2 DTid = DispatchRaysIndex().xy;
	const float2 uv = ((float2)DTid.xy + 0.5) / (float2)DispatchRaysDimensions();

	const uint downsampleFactor = 2;

	// This is necessary for accurate upscaling. This is so we don't reuse the same half-res pixels
	uint2 screenJitter = floor(blue_noise(uint2(0, 0)).xy * downsampleFactor);
	uint2 jitterPixel = screenJitter + DTid.xy * downsampleFactor;
	float2 jitterUV = (screenJitter + DTid.xy + 0.5f) / (float2)DispatchRaysDimensions();

	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, jitterUV, 0);
	const float roughness = texture_roughness[jitterPixel];

	if (!NeedReflection(roughness, depth, rtreflection_roughness_cutoff))
	{
		output_rayIndirectSpecular[DTid.xy] = 0;
		output_rayDirectionPDF[DTid.xy] = 0;
		output_rayLengths[DTid.xy] = FLT_MAX;
		return;
	}

	const float3 N = decode_oct(texture_normal[jitterPixel]);
	const float3 P = reconstruct_position(jitterUV, depth);
	const float3 V = normalize(GetCamera().position - P);

	const float4 GGX = ReflectionDir_GGX(V, N, roughness, blue_noise(DTid.xy).xy);
	const float3 R = GGX.xyz;
	const float PDF = GGX.w;

	float seed = GetFrame().time;

	RayDesc ray;
	ray.TMin = 0.01;
	ray.TMax = rtreflection_range;
	ray.Origin = P;
	ray.Direction = normalize(R);

	RayPayload payload;
	payload.data = 0;

	TraceRay(
		scene_acceleration_structure,   // AccelerationStructure
		0,                              // RayFlags
		asuint(postprocess.params1.x),	// InstanceInclusionMask
		0,                              // RayContributionToHitGroupIndex
		0,                              // MultiplierForGeomtryContributionToShaderIndex
		0,                              // MissShaderIndex
		ray,                            // Ray
		payload                         // Payload
	);

	output_rayIndirectSpecular[DTid.xy] = float4(payload.data.xyz, 1);
	output_rayDirectionPDF[DTid.xy] = float4(R, PDF);
	output_rayLengths[DTid.xy] = payload.data.w;
}

[shader("closesthit")]
void RTReflection_ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	PrimitiveID prim;
	prim.primitiveIndex = PrimitiveIndex();
	prim.instanceIndex = InstanceID();
	prim.subsetIndex = GeometryIndex();

	Surface surface;
	surface.init();
	surface.SetBackface(HitKind() != HIT_KIND_TRIANGLE_FRONT_FACE);
	if (!surface.load(prim, attr.barycentrics))
		return;

	surface.pixel = DispatchRaysIndex().xy;
	surface.screenUV = (surface.pixel + 0.5) / (float2)DispatchRaysDimensions().xy;

	if (surface.material.IsUnlit())
	{
		payload.data.xyz = surface.albedo + surface.emissiveColor;
	}
	else
	{
		// Light sampling:
		surface.P = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
		surface.V = -WorldRayDirection();
		surface.update();

		Lighting lighting;
		lighting.create(0, 0, GetAmbient(surface.N), 0);

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

		lighting.indirect.specular += max(0, EnvironmentReflection_Global(surface));
		lighting.indirect.specular += surface.emissiveColor;

		[branch]
		if (GetScene().ddgi.color_texture >= 0)
		{
			lighting.indirect.diffuse = ddgi_sample_irradiance(surface.P, surface.N);
		}

		ApplyLighting(surface, lighting, payload.data);
	}
	payload.data.w = RayTCurrent();
}

[shader("anyhit")]
void RTReflection_AnyHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	PrimitiveID prim;
	prim.primitiveIndex = PrimitiveIndex();
	prim.instanceIndex = InstanceID();
	prim.subsetIndex = GeometryIndex();

	Surface surface;
	surface.init();
	if (!surface.load(prim, attr.barycentrics))
		return;

	float alphatest = clamp(blue_noise(DispatchRaysIndex().xy, RayTCurrent()).r, 0, 0.99);

	[branch]
	if (surface.opacity - alphatest < 0)
	{
		IgnoreHit();
	}
}

[shader("miss")]
void RTReflection_Miss(inout RayPayload payload)
{
	payload.data.xyz += GetDynamicSkyColor(WorldRayDirection());
	payload.data.w = FLT_MAX;
}
