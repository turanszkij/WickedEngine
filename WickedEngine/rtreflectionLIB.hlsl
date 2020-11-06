#define DISABLE_SOFT_SHADOWMAP
#define DISABLE_TRANSPARENT_SHADOWMAP
#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "raytracingHF.hlsli"
#include "stochasticSSRHF.hlsli"
#include "lightingHF.hlsli"

#ifndef RAYTRACING_INLINE
RAYTRACINGACCELERATIONSTRUCTURE(scene_acceleration_structure, TEXSLOT_ACCELERATION_STRUCTURE);
#endif // RAYTRACING_INLINE

RWTEXTURE2D(output, float4, 0);

ConstantBuffer<ShaderMaterial> subsets_material[MAX_DESCRIPTOR_INDEXING] : register(b0, space1);
Texture2D<float4> subsets_texture_baseColor[MAX_DESCRIPTOR_INDEXING] : register(t0, space1);
Buffer<uint> subsets_indexBuffer[MAX_DESCRIPTOR_INDEXING] : register(t100000, space1);
ByteAddressBuffer subsets_vertexBuffer_POS[MAX_DESCRIPTOR_INDEXING] : register(t200000, space1);
Buffer<float2> subsets_vertexBuffer_UV0[MAX_DESCRIPTOR_INDEXING] : register(t300000, space1);
Buffer<float2> subsets_vertexBuffer_UV1[MAX_DESCRIPTOR_INDEXING] : register(t400000, space1);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct RayPayload
{
    float3 color;
    float roughness;
};

[shader("raygeneration")]
void RTReflection_Raygen()
{
    const float2 uv = ((float2)DispatchRaysIndex() + 0.5f) / (float2)DispatchRaysDimensions();
    const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);
    if (depth == 0.0f)
        return;

    const float roughness = texture_gbuffer0.SampleLevel(sampler_linear_clamp, uv, 0).a;

    const float3 P = reconstructPosition(uv, depth);
    const float3 N = decodeNormal(texture_gbuffer1.SampleLevel(sampler_point_clamp, uv, 0).xy);
    const float3 V = normalize(g_xCamera_CamPos - P);


    float4 H;
    if (roughness > 0.1f)
    {
        const float surfaceMargin = 0.0f;
        const float maxRegenCount = 15.0f;

        uint2 Random = Rand_PCG16(int3((DispatchRaysIndex().xy + 0.5f), g_xFrame_FrameCount)).xy;

        // Pick the best rays

        float RdotN = 0.0f;
        float regenCount = 0;
        [loop]
        for (; RdotN <= surfaceMargin && regenCount < maxRegenCount; regenCount++)
        {
            // Low-discrepancy sequence
            //float2 Xi = float2(Random) * rcp(65536.0); // equivalent to HammersleyRandom(0, 1, Random).
            float2 Xi = HammersleyRandom16(regenCount, Random); // SingleSPP

            Xi.y = lerp(Xi.y, 0.0f, BRDFBias);

            // I should probably use importance sampling of visible normals http://jcgt.org/published/0007/04/01/paper.pdf
            H = ImportanceSampleGGX(Xi, roughness);
            H = TangentToWorld(H, N);

            RdotN = dot(N, reflect(-V, H.xyz));
        }
    }
    else
    {
        H = float4(N.xyz, 1.0f);
    }

    const float3 R = -reflect(V, H.xyz);

    float seed = rtreflection_seed;

    RayDesc ray;
    ray.TMin = 0.001;
    ray.TMax = rtreflection_range;
    ray.Origin = P + N * 0.1;
    ray.Direction = normalize(R);

    RayPayload payload;
    payload.color = 0;
    payload.roughness = roughness;

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

    output[DispatchRaysIndex().xy] = float4(payload.color, 1);
}

[shader("closesthit")]
void RTReflection_ClosestHit(inout RayPayload payload, in MyAttributes attr)
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
    uint i0 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 0];
    uint i1 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 1];
    uint i2 = subsets_indexBuffer[descriptorIndex][primitiveIndex * 3 + 2];
    float4 uv0, uv1, uv2;
    uv0.xy = subsets_vertexBuffer_UV0[descriptorIndex][i0];
    uv1.xy = subsets_vertexBuffer_UV0[descriptorIndex][i1];
    uv2.xy = subsets_vertexBuffer_UV0[descriptorIndex][i2];
    uv0.zw = subsets_vertexBuffer_UV1[descriptorIndex][i0];
    uv1.zw = subsets_vertexBuffer_UV1[descriptorIndex][i1];
    uv2.zw = subsets_vertexBuffer_UV1[descriptorIndex][i2];
    float3 n0, n1, n2;
    n0 = unpack_unitvector(subsets_vertexBuffer_POS[descriptorIndex].Load4(i0 * 16).w);
    n1 = unpack_unitvector(subsets_vertexBuffer_POS[descriptorIndex].Load4(i1 * 16).w);
    n2 = unpack_unitvector(subsets_vertexBuffer_POS[descriptorIndex].Load4(i2 * 16).w);

    float4 uvsets = uv0 * w + uv1 * u + uv2 * v;
    float3 N = n0 * w + n1 * u + n2 * v;

    N = mul((float3x3)ObjectToWorld3x4(), N);
    N = normalize(N);

    float4 baseColor;
    [branch]
    if (material.uvset_baseColorMap >= 0)
    {
        const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
        baseColor = subsets_texture_baseColor[descriptorIndex].SampleLevel(sampler_linear_wrap, UV_baseColorMap, 2);
        baseColor.rgb = DEGAMMA(baseColor.rgb);
    }
    else
    {
        baseColor = 1;
    }
    baseColor *= material.baseColor;
    float4 color = baseColor;
    float4 emissiveColor = material.emissiveColor;


    // Light sampling:
    float3 P = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 V = normalize(g_xCamera_CamPos - P);
    Surface surface = CreateSurface(P, N, V, baseColor, material.roughness, 1, material.metalness, material.reflectance);
    Lighting lighting = CreateLighting(0, 0, GetAmbient(surface.N), 0);

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
        case ENTITY_TYPE_SPHERELIGHT:
        {
            SphereLight(light, surface, lighting);
        }
        break;
        case ENTITY_TYPE_DISCLIGHT:
        {
            DiscLight(light, surface, lighting);
        }
        break;
        case ENTITY_TYPE_RECTANGLELIGHT:
        {
            RectangleLight(light, surface, lighting);
        }
        break;
        case ENTITY_TYPE_TUBELIGHT:
        {
            TubeLight(light, surface, lighting);
        }
        break;
        }
    }

    LightingPart combined_lighting = CombineLighting(surface, lighting);
    payload.color = baseColor.rgb * combined_lighting.diffuse + emissiveColor.rgb * emissiveColor.a;

#else
    payload.color = float3(1, 0, 0);
#endif // SPIRV
}

[shader("anyhit")]
void RTReflection_AnyHit(inout RayPayload payload, in MyAttributes attr)
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
        return;
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

    if (alpha < 0.9)
    {
        IgnoreHit();
    }
#endif // SPIRV
}

[shader("miss")]
void RTReflection_Miss(inout RayPayload payload)
{
    Surface surface;
    surface.roughness = payload.roughness;
    surface.R = WorldRayDirection();
    payload.color = EnvironmentReflection_Global(surface, surface.roughness * g_xFrame_EnvProbeMipCount);
}
