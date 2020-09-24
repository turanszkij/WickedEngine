#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

TEXTURE2D(texture_normals, float3, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float, 0);

ConstantBuffer<ShaderMaterial> subsets_material[MAX_DESCRIPTOR_INDEXING] : register(b0, space1);
Texture2D<float4> subsets_texture_baseColor[MAX_DESCRIPTOR_INDEXING] : register(t0, space1);
Buffer<uint> subsets_indexBuffer[MAX_DESCRIPTOR_INDEXING] : register(t100000, space1);
Buffer<float2> subsets_vertexBuffer_UV0[MAX_DESCRIPTOR_INDEXING] : register(t300000, space1);
Buffer<float2> subsets_vertexBuffer_UV1[MAX_DESCRIPTOR_INDEXING] : register(t400000, space1);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct RayPayload
{
    float color;
};

[shader("raygeneration")]
void RTAO_Raygen()
{
    const float2 uv = ((float2)DispatchRaysIndex() + 0.5f) / (float2)DispatchRaysDimensions();
    const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);
    if (depth == 0.0f)
        return;

    const float3 P = reconstructPosition(uv, depth);
    float3 N = normalize(texture_normals.SampleLevel(sampler_point_clamp, uv, 0) * 2 - 1);

    float seed = rtao_seed;

    RayDesc ray;
    ray.TMin = 0.001;
    ray.TMax = rtao_range;
    ray.Origin = P + N * 0.1;

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
void RTAO_ClosestHit(inout RayPayload payload, in MyAttributes attr)
{
    //payload.color = 0;
}

[shader("anyhit")]
void RTAO_AnyHit(inout RayPayload payload, in MyAttributes attr)
{
#ifndef SPIRV
    float u = attr.barycentrics.x;
    float v = attr.barycentrics.y;
    float w = 1 - u - v;
    uint primitiveIndex = PrimitiveIndex();
    uint geometryOffset = InstanceID();
#ifdef RAYTRACING_GEOMETRYINDEX
    uint geometryIndex = GeometryIndex(); // requires tier_1_1 GeometryIndex feature!!
#else
    uint geometryIndex = 0;
#endif // RAYTRACING_GEOMETRYINDEX
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
        uv0 = subsets_vertexBuffer_UV0[descriptorIndex][i0];
        uv1 = subsets_vertexBuffer_UV0[descriptorIndex][i1];
        uv2 = subsets_vertexBuffer_UV0[descriptorIndex][i2];
    }
    else
    {
        uv0 = subsets_vertexBuffer_UV1[descriptorIndex][i0];
        uv1 = subsets_vertexBuffer_UV1[descriptorIndex][i1];
        uv2 = subsets_vertexBuffer_UV1[descriptorIndex][i2];
    }

    float2 uv = uv0 * w + uv1 * u + uv2 * v;
    float alpha = subsets_texture_baseColor[descriptorIndex].SampleLevel(sampler_point_wrap, uv, 2).a;

    if (alpha > 0.9)
    {
        AcceptHitAndEndSearch();
    }
    else
    {
        IgnoreHit();
    }
#endif // SPIRV
}

[shader("miss")]
void RTAO_Miss(inout RayPayload payload)
{
    payload.color += 1;
}
