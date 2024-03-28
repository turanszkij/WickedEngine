#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> output : register(u0);

static const uint DOWNSAMPLE = 2;

// SSGI based on: https://github.com/PanosK92/SpartanEngine/blob/1986a7053abd9e17c2d1564eb0f751a89116b7e6/data/shaders/ssgi.hlsl#L53

static const float PI2                 = 6.28318530f;

float get_random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float3 get_normal(uint2 pos)
{
    // Load returns 0 for any value accessed out of bounds, so clamp.
    pos.x = clamp(pos.x, 0, postprocess.resolution.x);
    pos.y = clamp(pos.y, 0, postprocess.resolution.y);
    
    return decode_oct(texture_normal[pos * DOWNSAMPLE].rg);
}

float3 get_normal(float2 uv)
{
    return decode_oct(texture_normal.SampleLevel(sampler_linear_clamp, uv, 0).rg);
}

float3 get_normal_view_space(uint2 pos)
{
    return normalize(mul(GetCamera().view, float4(get_normal(pos), 0.0f)).xyz);
}

float3 get_normal_view_space(float2 uv)
{
    return normalize(mul(GetCamera().view, float4(get_normal(uv), 0.0f)).xyz);
}

float get_depth(const uint2 position)
{
    return texture_depth[position * DOWNSAMPLE].r;
}

float get_depth(const float2 uv)
{
    return texture_depth.SampleLevel(sampler_linear_clamp, uv, 0).r;
}

float3 get_position(float z, float2 uv)
{
	return reconstruct_position(uv,z);
}

float3 get_position(float2 uv)
{
    return get_position(get_depth(uv), uv);
}

float3 get_position(uint2 pos)
{
    const float2 uv = (pos + 0.5f) * postprocess.resolution_rcp;
    return get_position(get_depth(pos), uv);
}
float3 get_position_view_space(uint2 pos)
{
    return mul(GetCamera().view, float4(get_position(pos), 1.0f)).xyz;
}

float3 get_position_view_space(float2 uv)
{
    return mul(GetCamera().view, float4(get_position(uv), 1.0f)).xyz;
}

// constants
static const uint g_ao_directions = 3;
static const uint g_ao_steps      = 4;
static const float g_ao_radius    = 4.0f;
static const float g_ao_intensity = 4.0f;

// derived constants
static const float ao_samples        = (float)(g_ao_directions * g_ao_steps);
static const float ao_samples_rcp    = 1.0f / ao_samples;
static const float ao_radius2        = g_ao_radius * g_ao_radius;
static const float ao_negInvRadius2  = -1.0f / ao_radius2;
static const float ao_step_direction = PI2 / (float) g_ao_directions;

/*------------------------------------------------------------------------------
                              SOME GTAO FUNCTIONS
   https://www.activision.com/cdn/research/s2016_pbs_activision_occlusion.pptx
------------------------------------------------------------------------------*/

float get_offset_non_temporal(uint2 screen_pos)
{
    int2 position = (int2)(screen_pos);
    return 0.25 * (float)((position.y - position.x) & 3);
}
static const float offsets[]   = { 0.0f, 0.5f, 0.25f, 0.75f };
static const float rotations[] = { 0.1666f, 0.8333, 0.5f, 0.6666, 0.3333, 0.0f }; // 60.0f, 300.0f, 180.0f, 240.0f, 120.0f, 0.0f devived by 360.0f

/*------------------------------------------------------------------------------
                              SSGI
------------------------------------------------------------------------------*/

float compute_occlusion(float3 origin_position, float3 origin_normal, uint2 sample_position)
{
    float3 origin_to_sample = get_position_view_space(sample_position) - origin_position;
    float distance2         = dot(origin_to_sample, origin_to_sample);
    float occlusion         = dot(origin_normal, origin_to_sample) * rsqrt(distance2);
    float falloff           = saturate(distance2 * ao_negInvRadius2 + 1.0f);

    return occlusion * falloff;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 pixel = DTid.xy;
    if (any(int2(pixel) >= postprocess.resolution))
        return;
		
	const float2 velocity = texture_velocity[pixel * DOWNSAMPLE];
		
    float visibility      = 0;
    float3 diffuse_bounce = 0;
	
    const float2 origin_uv       = (pixel + 0.5f) * postprocess.resolution_rcp;
    const float3 origin_position = get_position_view_space(origin_uv);
    const float3 origin_normal   = get_normal_view_space(origin_uv);

    // compute step in pixels
    const float pixel_offset = max((g_ao_radius * postprocess.resolution.x * 0.5f) / origin_position.z, (float)g_ao_steps);
    const float step_offset  = pixel_offset / float(g_ao_steps + 1.0f); // divide by steps + 1 so that the farthest samples are not fully attenuated

    // offsets (noise over space and time)
    const float noise_gradient_temporal  = InterleavedGradientNoise(pixel, GetFrame().frame_count);
    const float offset_spatial           = get_offset_non_temporal(pixel);
    const float offset_temporal          = offsets[GetFrame().frame_count % 4];
    const float offset_rotation_temporal = rotations[GetFrame().frame_count % 6];
    const float ray_offset               = frac(offset_spatial + offset_temporal) + (get_random(origin_uv) * 2.0 - 1.0) * 0.25;

    // compute light/occlusion
    float occlusion = 0.0f;
    [unroll]
    for (uint direction_index = 0; direction_index < g_ao_directions; direction_index++)
    {
        float rotation_angle      = (direction_index + noise_gradient_temporal + offset_rotation_temporal) * ao_step_direction;
        float2 rotation_direction = float2(cos(rotation_angle), sin(rotation_angle)) * postprocess.resolution_rcp;

        [unroll]
        for (uint step_index = 0; step_index < g_ao_steps; step_index++)
        {
            float2 uv_offset = round(max(step_offset * (step_index + ray_offset), 1 + step_index)) * rotation_direction;
			float2 uv = origin_uv + uv_offset;
			uint2 sample_pos = uv * postprocess.resolution;
            float sample_occlusion = compute_occlusion(origin_position, origin_normal, sample_pos);

            occlusion += sample_occlusion;
            diffuse_bounce += input.SampleLevel(sampler_linear_clamp, uv + velocity, 0).rgb * max(0.01f, sample_occlusion); // max(0.01f, x) to avoid black pixels
        }
    }

    visibility = 1.0f - saturate(occlusion * ao_samples_rcp * g_ao_intensity);
    diffuse_bounce *= ao_samples_rcp * g_ao_intensity * 3.0f; // 3.0f - this is what looks right
	
    output[pixel] = float4(diffuse_bounce, visibility);
}
