#include "globals.hlsli"
#include "ColorSpaceUtility.hlsli"

PUSHCONSTANT(push, TerrainFallbackUpdatePush);

#if !defined(UPDATE_NORMALMAP) && !defined(UPDATE_SURFACEMAP) && !defined(UPDATE_EMISSIVEMAP)
#define UPDATE_BASECOLORMAP
#endif

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= push.resolution.x || DTid.y >= push.resolution.y)
        return;

    Texture2DArray blendmap = bindless_textures2DArray[descriptor_index(push.blendmap_texture)];
    ByteAddressBuffer blendmap_buffer = bindless_buffers[descriptor_index(push.blendmap_buffer)];

    const float2 uv = (float2(DTid.xy) + 0.5f) * push.resolution_rcp;
    const float2 uv2 = float2(uv.x, 1 - uv.y);

#ifdef UPDATE_BASECOLORMAP
    RWTexture2D<float4> output = bindless_rwtextures[descriptor_index(push.output_texture)];
    float4 total_color = 0;
    float accumulation = 0;
#endif

#ifdef UPDATE_NORMALMAP
    RWTexture2D<float4> output = bindless_rwtextures[descriptor_index(push.output_texture)];
    float2 total_xy = 0;
    float accumulation = 0;
#endif

#ifdef UPDATE_SURFACEMAP
    RWTexture2D<float4> output = bindless_rwtextures[descriptor_index(push.output_texture)];
    float4 total_surface = float4(1, 1, 0, 0);
    float accumulation = 0;
#endif

#ifdef UPDATE_EMISSIVEMAP
    RWTexture2D<float4> output = bindless_rwtextures[descriptor_index(push.output_texture)];
    float4 total_emissive = 0;
    float accumulation = 0;
#endif

    // Front-to-back blending like decal layering
    for (int blendmap_index = (int)push.blendmap_layers - 1; blendmap_index >= 0; --blendmap_index)
    {
        const float weight = blendmap.SampleLevel(sampler_linear_clamp, float3(uv, blendmap_index), 0).r;
        if (weight <= 0)
            continue;

        const uint materialIndex = blendmap_buffer.Load(push.blendmap_buffer_offset + blendmap_index * sizeof(uint));
        ShaderMaterial material = load_material(materialIndex);

#ifdef UPDATE_BASECOLORMAP
        float4 baseColor = material.GetBaseColor();
        if (material.textures[BASECOLORMAP].IsValid())
        {
            Texture2D tex = bindless_textures[descriptor_index(material.textures[BASECOLORMAP].texture_descriptor)];
            float2 dim = 0;
            tex.GetDimensions(dim.x, dim.y);
            const float2 diff = dim * push.resolution_rcp;
            const float lod = log2(max(diff.x, diff.y));
            const float2 overscale = lod < 0 ? diff : 1;
            float4 sampleColor = tex.SampleLevel(sampler_linear_wrap, uv2 / overscale, lod);
            baseColor *= sampleColor;
        }
        total_color = mad(1 - accumulation, weight * baseColor, total_color);
        accumulation = mad(1 - weight, accumulation, weight);
        if (accumulation >= 0.999f)
            break;
#endif

#ifdef UPDATE_NORMALMAP
        if (material.textures[NORMALMAP].IsValid())
        {
            Texture2D tex = bindless_textures[descriptor_index(material.textures[NORMALMAP].texture_descriptor)];
            float2 dim = 0;
            tex.GetDimensions(dim.x, dim.y);
            const float2 diff = dim * push.resolution_rcp;
            const float lod = log2(max(diff.x, diff.y));
            const float2 overscale = lod < 0 ? diff : 1;
            const float2 normalSample = tex.SampleLevel(sampler_linear_wrap, uv2 / overscale, lod).rg;
            total_xy = mad(1 - accumulation, weight * normalSample, total_xy);
            accumulation = mad(1 - weight, accumulation, weight);
            if (accumulation >= 0.999f)
                break;
        }
#endif

#ifdef UPDATE_SURFACEMAP
        float4 surface = float4(1, material.GetRoughness(), material.GetMetalness(), material.GetReflectance());
        if (material.textures[SURFACEMAP].IsValid())
        {
            Texture2D tex = bindless_textures[descriptor_index(material.textures[SURFACEMAP].texture_descriptor)];
            float2 dim = 0;
            tex.GetDimensions(dim.x, dim.y);
            const float2 diff = dim * push.resolution_rcp;
            const float lod = log2(max(diff.x, diff.y));
            const float2 overscale = lod < 0 ? diff : 1;
            const float4 surfaceSample = tex.SampleLevel(sampler_linear_wrap, uv2 / overscale, lod);
            surface *= surfaceSample;
        }
        total_surface = mad(1 - accumulation, weight * surface, total_surface);
        accumulation = mad(1 - weight, accumulation, weight);
        if (accumulation >= 0.999f)
            break;
#endif

#ifdef UPDATE_EMISSIVEMAP
        float4 emissiveColor = 0;
        if (material.textures[EMISSIVEMAP].IsValid())
        {
            Texture2D tex = bindless_textures[descriptor_index(material.textures[EMISSIVEMAP].texture_descriptor)];
            float2 dim = 0;
            tex.GetDimensions(dim.x, dim.y);
            const float2 diff = dim * push.resolution_rcp;
            const float lod = log2(max(diff.x, diff.y));
            const float2 overscale = lod < 0 ? diff : 1;
            const float4 emissiveSample = tex.SampleLevel(sampler_linear_wrap, uv2 / overscale, lod);
            emissiveColor.rgb = emissiveSample.rgb * emissiveSample.a;
            emissiveColor.a = emissiveSample.a;
        }
        total_emissive = mad(1 - accumulation, weight * emissiveColor, total_emissive);
        accumulation = mad(1 - weight, accumulation, weight);
        if (accumulation >= 0.999f)
            break;
#endif
    }

#ifdef UPDATE_BASECOLORMAP
    const float3 srgb = ApplySRGBCurve_Fast(total_color.rgb);
    output[DTid.xy] = float4(srgb, total_color.a);
#endif

#ifdef UPDATE_NORMALMAP
    float2 encoded_xy = saturate(total_xy) * 2.0f - 1.0f;
    const float lenSq = dot(encoded_xy, encoded_xy);
    const float z = sqrt(saturate(1.0f - lenSq));
    float3 normal = normalize(float3(encoded_xy, z));
    float3 stored = normal * 0.5f + 0.5f;
    output[DTid.xy] = float4(stored, 0.0f);
#endif

#ifdef UPDATE_SURFACEMAP
    output[DTid.xy] = total_surface;
#endif

#ifdef UPDATE_EMISSIVEMAP
    const float3 emissive_srgb = ApplySRGBCurve_Fast(total_emissive.rgb);
    output[DTid.xy] = float4(emissive_srgb, total_emissive.a);
#endif
}
