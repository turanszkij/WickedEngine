#define RTAPI
#define DISABLE_SOFT_SHADOWMAP
#define DISABLE_TRANSPARENT_SHADOWMAP

#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"
#include "stochasticSSRHF.hlsli"
#include "lightingHF.hlsli"

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

	bool disocclusion = false;
	const float2 velocity = texture_gbuffer2.SampleLevel(sampler_point_clamp, uv, 0).xy;
	const float2 prevUV = uv + velocity;
	if (!is_saturated(prevUV))
	{
		//output[DTid.xy] = float4(1, 0, 0, 1);
		//return;
		disocclusion = true;
	}

	// Disocclusion fallback:
	float depth_current = getLinearDepth(depth);
	float depth_history = getLinearDepth(texture_depth_history.SampleLevel(sampler_point_clamp, prevUV, 1));
	if (abs(depth_current - depth_history) > 1)
	{
		//output[DTid.xy] = float4(1, 0, 0, 1);
		//return;
		disocclusion = true;
	}

	const float3 P = reconstructPosition(uv, depth);
	const float3 V = normalize(g_xCamera_CamPos - P);

	float3 N;
	float roughness;
	if (disocclusion)
	{
		// When reprojection is invalid, trace the surface parameters:
		RayDesc ray;
		ray.Origin = P - V * 0.005;
		ray.Direction = V;
		ray.TMin = 0;
		ray.TMax = 0.01;

		RayPayload payload;
		payload.data = -1; // indicate closesthit shader will just fill normal and roughness

		TraceRay(
			scene_acceleration_structure,   // AccelerationStructure
			RAY_FLAG_FORCE_OPAQUE |
			RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
			0,                              // RayFlags
			~0,                             // InstanceInclusionMask
			0,                              // RayContributionToHitGroupIndex
			0,                              // MultiplierForGeomtryContributionToShaderIndex
			0,                              // MissShaderIndex
			ray,                            // Ray
			payload                         // Payload
		);

		N = payload.data.xyz;
		roughness = payload.data.w;
	}
	else
	{
		// When reprojection valid, just sample surface parameters from gbuffer:
		const float4 g1 = texture_gbuffer1.SampleLevel(sampler_linear_clamp, prevUV, 0);
		N = normalize(g1.rgb * 2 - 1);
		roughness = g1.a;
	}

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

	float seed = g_xFrame_Time;

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
	ShaderMesh mesh = bindless_buffers[NonUniformResourceIndex(InstanceID())].Load<ShaderMesh>(0);
	ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][GeometryIndex()];
	ShaderMaterial material = bindless_buffers[NonUniformResourceIndex(subset.material)].Load<ShaderMaterial>(0);

	Surface surface;

	EvaluateObjectSurface(
		mesh,
		subset,
		material,
		PrimitiveIndex(),
		attr.barycentrics,
		ObjectToWorld3x4(),
		surface
	);

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
	for (uint iterator = 0; iterator < g_xFrame_LightArrayCount; iterator++)
	{
		ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + iterator];
		if ((light.layerMask & material.layerMask) == 0)
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
	ShaderMesh mesh = bindless_buffers[NonUniformResourceIndex(InstanceID())].Load<ShaderMesh>(0);
	ShaderMeshSubset subset = bindless_subsets[NonUniformResourceIndex(mesh.subsetbuffer)][GeometryIndex()];
	ShaderMaterial material = bindless_buffers[NonUniformResourceIndex(subset.material)].Load<ShaderMaterial>(0);

	Surface surface;

	EvaluateObjectSurface(
		mesh,
		subset,
		material,
		PrimitiveIndex(),
		attr.barycentrics,
		ObjectToWorld3x4(),
		surface
	);

	[branch]
	if (surface.opacity < material.alphaTest)
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
