#define RTAPI
#define DISABLE_SOFT_SHADOWMAP
#define DISABLE_TRANSPARENT_SHADOWMAP

#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"
#include "stochasticSSRHF.hlsli"
#include "lightingHF.hlsli"

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<float4> output : register(u0);
RWTexture2D<float> output_rayLengths : register(u1);

struct RayPayload
{
	float4 data;
};

#ifndef SPIRV
GlobalRootSignature MyGlobalRootSignature =
{
	WICKED_ENGINE_DEFAULT_ROOTSIGNATURE
};
#endif // SPIRV

[shader("raygeneration")]
void RTReflection_Raygen()
{
	uint2 DTid = DispatchRaysIndex().xy;
	const float2 uv = ((float2)DTid.xy + 0.5) / (float2)DispatchRaysDimensions();
	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	if (depth == 0)
		return;

	const float3 P = reconstruct_position(uv, depth);
	const float3 V = normalize(GetCamera().position - P);

	PrimitiveID prim;
	prim.unpack(texture_gbuffer0[DTid.xy * 2]);

	//output[DTid] = float4(saturate(P * 0.1), 1);
	//return;

	Surface surface;
	if (!surface.load(prim, P))
	{
		return;
	}
	if (surface.roughness > 0.6)
	{
		output[DTid.xy] = float4(max(0, EnvironmentReflection_Global(surface)), 1);
		output_rayLengths[DTid.xy] = FLT_MAX;
		return;
	}

	float3 N = surface.N;
	float roughness = surface.roughness;

	// The ray direction selection part is the same as in from ssr_raytraceCS.hlsl:
	float4 H;
	float3 L;
	if (roughness > 0.05f)
	{
		float3x3 tangentBasis = GetTangentBasis(N);
		float3 tangentV = mul(tangentBasis, V);

		const float2 bluenoise = blue_noise(DTid.xy).xy;

		float2 Xi = bluenoise.xy;

		Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);

		H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);

		// Tangent to world
		H.xyz = mul(H.xyz, tangentBasis);


		L = reflect(-V, H.xyz);
	}
	else
	{
		H = float4(N.xyz, 1.0f);
		L = reflect(-V, H.xyz);
	}


	const float3 R = L;

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

	output[DTid.xy] = float4(payload.data.xyz, 1);
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
	surface.is_frontface = (HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE);
	if (!surface.load(prim, attr.barycentrics))
		return;

	surface.pixel = DispatchRaysIndex().xy;
	surface.screenUV = surface.pixel / (float2)DispatchRaysDimensions().xy;

	if (surface.material.IsUnlit())
	{
		payload.data.xyz += surface.albedo + surface.emissiveColor;
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

		LightingPart combined_lighting = CombineLighting(surface, lighting);
		payload.data.xyz += surface.albedo * combined_lighting.diffuse + combined_lighting.specular + surface.emissiveColor;
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
