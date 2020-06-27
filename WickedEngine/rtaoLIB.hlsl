#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"

RWTEXTURE2D(output, unorm float, 0);

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
            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
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

[shader("miss")]
void RTAO_Miss(inout RayPayload payload)
{
    payload.color += 1;
}
