#define RTAPI
#define DISABLE_SOFT_SHADOWMAP
#define DISABLE_TRANSPARENT_SHADOWMAP

#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"
#include "stochasticSSRHF.hlsli"
#include "lightingHF.hlsli"

STRUCTUREDBUFFER(rayCounter, uint, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(rayList, uint, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(traceResults, float4, 0);
RWTEXTURE2D(rayLengths, float4, 1);

struct RayPayload
{
	float3 color;
	float rayLength;
	uint2 pixel;
};

[shader("raygeneration")]
void RTReflection_Raygen()
{
	uint ray_index = DispatchRaysIndex().x;
	if (ray_index >= rayCounter[1]) return;
	uint packed_coords = rayList[ray_index];

	int2 coords;
	bool copy_horizontal;
	bool copy_vertical;
	bool copy_diagonal;
	UnpackRayCoords(packed_coords, coords, copy_horizontal, copy_vertical, copy_diagonal);

	uint2 DTid = (uint2)coords;
	//uint2 DTid = DispatchRaysIndex().xy;
	const float2 uv = ((float2)DTid.xy + 0.5) * xPPResolution_rcp;
	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	if (depth == 0)
		return;

	const float2 velocity = texture_gbuffer2.SampleLevel(sampler_point_clamp, uv, 0).xy;
	const float2 prevUV = uv + velocity;

	const float4 g1 = texture_gbuffer1.SampleLevel(sampler_linear_clamp, prevUV, 0);
	const float3 P = reconstructPosition(uv, depth);
	const float3 N = normalize(g1.rgb * 2 - 1);
	const float3 V = normalize(g_xCamera_CamPos - P);
	const float roughness = g1.a;


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
	ray.TMin = 0.05;
	ray.TMax = rtreflection_range;
	ray.Origin = P;
	ray.Direction = normalize(R);

	RayPayload payload;
	payload.color = 0;
	payload.rayLength = 0;
	payload.pixel = DTid.xy;

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

	float4 reflection_result = float4(payload.color, 1);

	traceResults[DTid.xy] = reflection_result;
	rayLengths[DTid.xy] = payload.rayLength;

	uint2 copy_target = coords ^ 0b1; // Flip last bit to find the mirrored coords along the x and y axis within a quad.
	if (copy_horizontal) {
		uint2 copy_coords = uint2(copy_target.x, coords.y);
		traceResults[copy_coords] = reflection_result;
		rayLengths[copy_coords] = payload.rayLength;
	}
	if (copy_vertical) {
		uint2 copy_coords = uint2(coords.x, copy_target.y);
		traceResults[copy_coords] = reflection_result;
		rayLengths[copy_coords] = payload.rayLength;
	}
	if (copy_diagonal) {
		uint2 copy_coords = copy_target;
		traceResults[copy_coords] = reflection_result;
		rayLengths[copy_coords] = payload.rayLength;
	}
}

[shader("closesthit")]
void RTReflection_ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	ShaderMesh mesh = bindless_buffers[InstanceID()].Load<ShaderMesh>(0);
	ShaderMeshSubset subset = bindless_subsets[mesh.subsetbuffer][GeometryIndex()];
	ShaderMaterial material = bindless_buffers[subset.material].Load<ShaderMaterial>(0);

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

	surface.pixel = payload.pixel;
	surface.screenUV = surface.pixel * xPPResolution_rcp;

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
	payload.color = surface.albedo * combined_lighting.diffuse + combined_lighting.specular + surface.emissiveColor.rgb * surface.emissiveColor.a;
	payload.rayLength = RayTCurrent();
}

[shader("anyhit")]
void RTReflection_AnyHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	ShaderMesh mesh = bindless_buffers[InstanceID()].Load<ShaderMesh>(0);
	ShaderMeshSubset subset = bindless_subsets[mesh.subsetbuffer][GeometryIndex()];
	ShaderMaterial material = bindless_buffers[subset.material].Load<ShaderMaterial>(0);

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
	payload.color = GetDynamicSkyColor(WorldRayDirection());
}
