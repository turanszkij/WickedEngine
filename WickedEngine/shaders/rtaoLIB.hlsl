#define RTAPI
#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

TEXTURE2D(texture_normals, float3, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float, 0);


struct RayPayload
{
	float color;
};

[shader("raygeneration")]
void RTAO_Raygen()
{
	const float2 uv = ((float2)DispatchRaysIndex() + 0.5f) / (float2)DispatchRaysDimensions();
	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	if (depth == 0.0f)
		return;

	const float3 P = reconstructPosition(uv, depth);
	float3 N = normalize(texture_normals.SampleLevel(sampler_point_clamp, uv, 0) * 2 - 1);

	float seed = g_xFrame_Time;

	RayDesc ray;
	ray.TMin = 0.01;
	ray.TMax = rtao_range;
	ray.Origin = P;

	RayPayload payload;
	payload.color = 0;

	for (uint i = 0; i < (uint)rtao_samplecount; ++i)
	{
		ray.Direction = normalize(SampleHemisphere_cos(N, seed, uv));

		TraceRay(
			scene_acceleration_structure,         // AccelerationStructure
			RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,     // RayFlags
			~0,                                   // InstanceInclusionMask
			0,                                    // RayContributionToHitGroupIndex
			0,                                    // MultiplierForGeomtryContributionToShaderIndex
			0,                                    // MissShaderIndex
			ray,                                  // Ray
			payload                               // Payload
		);
	}
	payload.color /= rtao_samplecount;

	output[DispatchRaysIndex().xy] = pow(saturate(payload.color), rtao_power);
}

[shader("closesthit")]
void RTAO_ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	//payload.color = 0;
}

[shader("anyhit")]
void RTAO_AnyHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	ShaderMesh mesh = bindless_buffers[InstanceID()].Load<ShaderMesh>(0);
	ShaderMeshSubset subset = bindless_subsets[mesh.subsetbuffer][GeometryIndex()];
	ShaderMaterial material = bindless_buffers[subset.material].Load<ShaderMaterial>(0);
	[branch]
	if (!material.IsCastingShadow())
	{
		IgnoreHit();
		return;
	}
	[branch]
	if (material.texture_basecolormap_index < 0)
	{
		AcceptHitAndEndSearch();
		return;
	}

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
	if (surface.opacity >= material.alphaTest)
	{
		AcceptHitAndEndSearch();
	}
	else
	{
		IgnoreHit();
	}
}

[shader("miss")]
void RTAO_Miss(inout RayPayload payload)
{
	payload.color += 1;
}
