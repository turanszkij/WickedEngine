#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

RAYTRACINGACCELERATIONSTRUCTURE(scene_acceleration_structure, TEXSLOT_ACCELERATION_STRUCTURE);

TEXTURE2D(texture_normals, float3, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float, 0);

ConstantBuffer<ShaderMaterial> subsets_material[] : register(b0, space1);
Texture2D<float4> subsets_textures[] : register(t0, space2);
Buffer<uint> subsets_indexBuffer[] : register(t0, space3);
Buffer<float2> subsets_vertexBuffer_UVSETS[] : register(t0, space4);

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
	float u = attr.barycentrics.x;
	float v = attr.barycentrics.y;
	float w = 1 - u - v;
	uint primitiveIndex = PrimitiveIndex();
	uint geometryOffset = InstanceID();
	uint geometryIndex = GeometryIndex(); // requires tier_1_1 GeometryIndex feature!!
	uint descriptorIndex = geometryOffset + geometryIndex;
	ShaderMaterial material = subsets_material[descriptorIndex];
	if (material.uvset_baseColorMap < 0)
	{
		AcceptHitAndEndSearch();
	}
	uint i0 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 0];
	uint i1 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 1];
	uint i2 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 2];
	float2 uv0, uv1, uv2;
	if (material.uvset_baseColorMap == 0)
	{
		uv0 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_0][i0];
		uv1 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_0][i1];
		uv2 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_0][i2];
	}
	else
	{
		uv0 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_1][i0];
		uv1 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_1][i1];
		uv2 = subsets_vertexBuffer_UVSETS[descriptorIndex * VERTEXBUFFER_DESCRIPTOR_UV_COUNT + VERTEXBUFFER_DESCRIPTOR_UV_1][i2];
	}

	float2 uv = uv0 * w + uv1 * u + uv2 * v;
	float alpha = subsets_textures[descriptorIndex * MATERIAL_TEXTURE_SLOT_DESCRIPTOR_COUNT + MATERIAL_TEXTURE_SLOT_DESCRIPTOR_BASECOLOR].SampleLevel(sampler_point_wrap, uv, 2).a;

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
