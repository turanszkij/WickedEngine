#define RTAPI
#define DISABLE_SOFT_SHADOWMAP
#define DISABLE_TRANSPARENT_SHADOWMAP

#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"
#include "stochasticSSRHF.hlsli"
#include "lightingHF.hlsli"

PUSHCONSTANT(postprocess, PostProcess);

TEXTURE2D(texture_depth_history, float, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(output, float4, 0);
RWTEXTURE2D(output_rayLengths, float, 1);

struct RayPayload
{
	float4 data;
};

[shader("raygeneration")]
void RTReflection_Raygen()
{
	uint2 DTid = DispatchRaysIndex().xy;
	const float2 uv = ((float2)DTid.xy + 0.5) / (float2)DispatchRaysDimensions();
	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	if (depth == 0)
		return;

	const float3 P = reconstructPosition(uv, depth);
	const float3 V = normalize(g_xCamera.CamPos - P);

	PrimitiveID prim;
	prim.unpack(texture_gbuffer0[DTid.xy * 2]);

	Surface surface;
	surface.load(prim, P);
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

	float seed = g_xFrame.Time;

	RayDesc ray;
	ray.TMin = 0.01;
	ray.TMax = rtreflection_range;
	ray.Origin = P;
	ray.Direction = normalize(R);

	RayPayload payload;
	payload.data.xyz = 0;
	payload.data.w = roughness;

	TraceRay(
		scene_acceleration_structure,   // AccelerationStructure
		0,                              // RayFlags
		~0,                             // InstanceInclusionMask
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
	surface.load(prim, attr.barycentrics);

	[branch]
	if (payload.data.w < 0)
	{
		payload.data.xyz = surface.N;
		payload.data.w = surface.roughness;
		return;
	}

	surface.pixel = DispatchRaysIndex().xy;
	surface.screenUV = surface.pixel / (float2)DispatchRaysDimensions().xy;

	// Light sampling:
	surface.P = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	surface.V = -WorldRayDirection();
	surface.update();

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

	[loop]
	for (uint iterator = 0; iterator < g_xFrame.LightArrayCount; iterator++)
	{
		ShaderEntity light = load_entity(g_xFrame.LightArrayOffset + iterator);
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

	lighting.indirect.specular += max(0, EnvironmentReflection_Global(surface));

	LightingPart combined_lighting = CombineLighting(surface, lighting);
	payload.data.xyz = surface.albedo * combined_lighting.diffuse + combined_lighting.specular + surface.emissiveColor.rgb * surface.emissiveColor.a;
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
	surface.load(prim, attr.barycentrics);

	[branch]
	if (surface.opacity < surface.material.alphaTest)
	{
		IgnoreHit();
	}
}

[shader("miss")]
void RTReflection_Miss(inout RayPayload payload)
{
	payload.data.xyz = GetDynamicSkyColor(WorldRayDirection());
	payload.data.w = FLT_MAX;
}
