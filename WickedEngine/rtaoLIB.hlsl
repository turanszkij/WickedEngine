#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

RAYTRACINGACCELERATIONSTRUCTURE(scene_acceleration_structure, TEXSLOT_ACCELERATION_STRUCTURE);

TEXTURE2D(texture_normals, float3, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float, 0);

ConstantBuffer<ShaderMaterial> bindless_materials[] : register(b0, space1);
ConstantBuffer<ShaderMesh> bindless_meshes[] : register(b0, space2);
StructuredBuffer<ShaderMeshSubset> bindless_subsets[] : register(t0, space3);
Texture2D<float4> bindless_textures[] : register(t0, space4);
Buffer<uint> bindless_ib[] : register(t0, space5);
Buffer<float2> bindless_vb_uvset[] : register(t0, space6);


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
	ray.Origin = trace_bias_position(P, N);

	RayPayload payload;
	payload.color = 0;

	for (uint i = 0; i < (uint)rtao_samplecount; ++i)
	{
		ray.Direction = SampleHemisphere_cos(N, seed, uv);

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
	ShaderMesh mesh = bindless_meshes[InstanceID()];
	ShaderMeshSubset subset = bindless_subsets[mesh.subsetbuffer][GeometryIndex()];
	ShaderMaterial material = bindless_materials[subset.material];
	if (material.texture_basecolormap_index < 0)
	{
		AcceptHitAndEndSearch();
	}
	uint primitiveIndex = PrimitiveIndex();
	uint i0 = bindless_ib[mesh.ib][primitiveIndex * 3 + 0];
	uint i1 = bindless_ib[mesh.ib][primitiveIndex * 3 + 1];
	uint i2 = bindless_ib[mesh.ib][primitiveIndex * 3 + 2];
	float2 uv0, uv1, uv2;
	if (material.uvset_baseColorMap == 0)
	{
		uv0 = bindless_vb_uvset[mesh.vb_uv0][i0];
		uv1 = bindless_vb_uvset[mesh.vb_uv0][i1];
		uv2 = bindless_vb_uvset[mesh.vb_uv0][i2];
	}
	else
	{
		uv0 = bindless_vb_uvset[mesh.vb_uv1][i0];
		uv1 = bindless_vb_uvset[mesh.vb_uv1][i1];
		uv2 = bindless_vb_uvset[mesh.vb_uv1][i2];
	}

	float u = attr.barycentrics.x;
	float v = attr.barycentrics.y;
	float w = 1 - u - v;
	float2 uv = uv0 * w + uv1 * u + uv2 * v;
	float alpha = bindless_textures[material.texture_basecolormap_index].SampleLevel(sampler_point_wrap, uv, 2).a;

	if (alpha - material.alphaTest > 0)
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
