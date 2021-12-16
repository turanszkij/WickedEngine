#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> resolve_current : register(t0);
Texture2D<float4> resolve_history : register(t1);
Texture2D<float> rayLengths : register(t3);

RWTexture2D<float4> output : register(u0);

static const float temporalResponseMin = 0.75;
static const float temporalResponseMax = 0.95f;
static const float temporalScale = 3.0;
static const float temporalExposure = 10.0f;

inline float Luma4(float3 color)
{
    return (color.g * 2) + (color.r + color.b);
}

inline float HdrWeight4(float3 color, float exposure)
{
    return rcp(Luma4(color) * exposure + 4.0f);
}

float4 clip_aabb(float3 aabb_min, float3 aabb_max, float4 p, float4 q)
{
    float3 p_clip = 0.5 * (aabb_max + aabb_min);
    float3 e_clip = 0.5 * (aabb_max - aabb_min) + 0.00000001f;

    float4 v_clip = q - float4(p_clip, p.w);
    float3 v_unit = v_clip.xyz / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.0)
        return float4(p_clip, p.w) + v_clip / ma_unit;
    else
        return q; // point inside aabb
}

inline void ResolverAABB(Texture2D<float4> currentColor, SamplerState currentSampler, float sharpness, float exposureScale, float AABBScale, float2 uv, float2 texelSize, inout float4 currentMin, inout float4 currentMax, inout float4 currentAverage, inout float4 currentOutput)
{
    const int2 SampleOffset[9] = { int2(-1.0, -1.0), int2(0.0, -1.0), int2(1.0, -1.0), int2(-1.0, 0.0), int2(0.0, 0.0), int2(1.0, 0.0), int2(-1.0, 1.0), int2(0.0, 1.0), int2(1.0, 1.0) };
    
    // Modulate Luma HDR
    
    float4 sampleColors[9];
    [unroll]
    for (uint i = 0; i < 9; i++)
    {
        sampleColors[i] = currentColor.SampleLevel(currentSampler, uv + (SampleOffset[i] / texelSize), 0.0f);
    }

    float sampleWeights[9];
    [unroll]
    for (uint j = 0; j < 9; j++)
    {
        sampleWeights[j] = HdrWeight4(sampleColors[j].rgb, exposureScale);
    }

    float totalWeight = 0;
    [unroll]
    for (uint k = 0; k < 9; k++)
    {
        totalWeight += sampleWeights[k];
    }
    sampleColors[4] = (sampleColors[0] * sampleWeights[0] + sampleColors[1] * sampleWeights[1] + sampleColors[2] * sampleWeights[2] + sampleColors[3] * sampleWeights[3] + sampleColors[4] * sampleWeights[4] +
                       sampleColors[5] * sampleWeights[5] + sampleColors[6] * sampleWeights[6] + sampleColors[7] * sampleWeights[7] + sampleColors[8] * sampleWeights[8]) / totalWeight;

    // Variance Clipping (AABB)
    
    float4 m1 = 0.0;
    float4 m2 = 0.0;
    [unroll]
    for (uint x = 0; x < 9; x++)
    {
        m1 += sampleColors[x];
        m2 += sampleColors[x] * sampleColors[x];
    }

    float4 mean = m1 / 9.0;
    float4 stddev = sqrt((m2 / 9.0) - sqr(mean));
        
    currentMin = mean - AABBScale * stddev;
    currentMax = mean + AABBScale * stddev;

    currentOutput = sampleColors[4];
    currentMin = min(currentMin, currentOutput);
    currentMax = max(currentMax, currentOutput);
    currentAverage = mean;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	if ((uint)ssr_frame == 0)
	{
		output[DTid.xy] = resolve_current[DTid.xy];
		return;
	}

	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 0);
	if (depth == 0)
		return;

	const float2 velocity = texture_gbuffer1.SampleLevel(sampler_point_clamp, uv, 0).xy;
	float2 prevUV = uv + velocity;
	if (!is_saturated(prevUV))
	{
		output[DTid.xy] = resolve_current[DTid.xy];
		return;
	}

	const float3 P = reconstruct_position(uv, depth, GetCamera().inverse_projection);

	PrimitiveID prim;
	prim.unpack(texture_gbuffer0[DTid.xy * 2]);

	Surface surface;
	if (!surface.load(prim, P))
		return;

	const float roughness = surface.roughness;

	if (roughness < 0.01)
	{
		output[DTid.xy] = resolve_current[DTid.xy];
		//return;
	}

	// Secondary reprojection based on ray lengths:
	//	https://www.ea.com/seed/news/seed-dd18-presentation-slides-raytracing (Slide 45)
	if (roughness < 0.5)
	{
		float rayLength = rayLengths[DTid.xy];
		if (rayLength > 0)
		{
			const float3 P = reconstruct_position(uv, depth);
			const float3 V = normalize(GetCamera().position - P);
			const float3 rayEnd = P - V * rayLength;
			float4 rayEndPrev = mul(GetCamera().previous_view_projection, float4(rayEnd, 1));
			rayEndPrev.xy /= rayEndPrev.w;
			prevUV = rayEndPrev.xy * float2(0.5, -0.5) + 0.5;
		}
	}

	// Disocclusion fallback:
	float depth_current = compute_lineardepth(depth);
	float depth_history = compute_lineardepth(texture_depth_history.SampleLevel(sampler_point_clamp, prevUV, 1));
	if (abs(depth_current - depth_history) > 1)
	{
		output[DTid.xy] = resolve_current[DTid.xy];
		//output[DTid.xy] = float4(1, 0, 0, 1);
		return;
	}
    
    float4 previous = resolve_history.SampleLevel(sampler_linear_clamp, prevUV, 0);

    // Luma HDR and AABB minmax
    
    float4 current = 0;
    float4 currentMin, currentMax, currentAverage;
    ResolverAABB(resolve_current, sampler_linear_clamp, 0, temporalExposure, temporalScale, uv, postprocess.resolution, currentMin, currentMax, currentAverage, current);

    previous.xyz = clip_aabb(currentMin.xyz, currentMax.xyz, clamp(currentAverage, currentMin, currentMax), previous).xyz;
    previous.a = clamp(previous.a, currentMin.a, currentMax.a);
    
    // Blend color & history
    
	float blendFinal = lerp(temporalResponseMin, temporalResponseMax, saturate(1.0 - length(velocity) * 100));
    
    float4 result = lerp(current, previous, blendFinal);
    
    output[DTid.xy] = max(0, result);
}
