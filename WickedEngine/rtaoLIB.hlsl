#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

RWTEXTURE2D(output, unorm float, 0);

#ifdef RAYTRACING_TIER_1_1
ConstantBuffer<ShaderMaterial> subsets_material[MAX_DESCRIPTOR_INDEXING] : register(b0, space1);
Texture2D<float4> subsets_texture_baseColor[MAX_DESCRIPTOR_INDEXING] : register(t0, space1);
Buffer<uint> subsets_indexBuffer[MAX_DESCRIPTOR_INDEXING] : register(t100000, space1);
Buffer<float2> subsets_vertexBuffer_UV0[MAX_DESCRIPTOR_INDEXING] : register(t300000, space1);
Buffer<float2> subsets_vertexBuffer_UV1[MAX_DESCRIPTOR_INDEXING] : register(t400000, space1);
#endif // RAYTRACING_TIER_1_1

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
    
    float2 uv0 = uv; // center
    float2 uv1 = uv + float2(1, 0) / (float2)DispatchRaysDimensions(); // right 
    float2 uv2 = uv + float2(0, 1) / (float2)DispatchRaysDimensions(); // top

    float depth0 = texture_depth.SampleLevel(sampler_point_clamp, uv0, 0).r;
    float depth1 = texture_depth.SampleLevel(sampler_point_clamp, uv1, 0).r;
    float depth2 = texture_depth.SampleLevel(sampler_point_clamp, uv2, 0).r;

    float3 P0 = reconstructPosition(uv0, depth0);
    float3 P1 = reconstructPosition(uv1, depth1);
    float3 P2 = reconstructPosition(uv2, depth2);

    float3 N = normalize(cross(P1 - P0, P2 - P0));

    float seed = 666;

    RayDesc ray;
    ray.TMin = 0.001;
    ray.TMax = rtao_range;
    ray.Origin = P + N * 0.1;

    RayPayload payload = { 0 };

    for (uint i = 0; i < (uint)rtao_samplecount; ++i)
    {
        ray.Direction = SampleHemisphere_cos(N, seed, uv);
        TraceRay(scene_acceleration_structure,
#ifndef RAYTRACING_TIER_1_1 // tier 1_0 method of alpha test without GeometryIndex() is not implemented yet
            RAY_FLAG_FORCE_OPAQUE |
            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
#endif // RAYTRACING_TIER_1_1
            RAY_FLAG_SKIP_CLOSEST_HIT_SHADER
            , ~0, 0, 1, 0, ray, payload);
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
#ifdef RAYTRACING_TIER_1_1
    float u = attr.barycentrics.x;
    float v = attr.barycentrics.y;
    float w = 1 - u - v;
    uint primitiveIndex = PrimitiveIndex();
    uint geometryOffset = InstanceID();
    uint geometryIndex = GeometryIndex(); // requires tier_1_1!!
    uint descriptorIndex = geometryOffset + geometryIndex;
    ShaderMaterial material = subsets_material[descriptorIndex];
    uint i0 = subsets_indexBuffer[descriptorIndex][primitiveIndex / 3 + 0];
    uint i1 = subsets_indexBuffer[descriptorIndex][primitiveIndex / 3 + 1];
    uint i2 = subsets_indexBuffer[descriptorIndex][primitiveIndex / 3 + 2];
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
    float4 baseColor = material.baseColor;
    baseColor *= subsets_texture_baseColor[descriptorIndex].SampleLevel(sampler_point_clamp, uv, 0);

    if (baseColor.a > 0.9)
    {
        AcceptHitAndEndSearch();
    }
    else
    {
        payload.color += 1 - baseColor.a;
    }
#endif // RAYTRACING_TIER_1_1
}

[shader("miss")]
void RTAO_Miss(inout RayPayload payload)
{
    payload.color += 1;
}
