#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> cloud_reproject : register(t0);
Texture2D<float2> cloud_reproject_depth : register(t1);
Texture2D<float4> cloud_history : register(t2);

RWTexture2D<float4> output : register(u0);
RWTexture2D<unorm float4> output_cloudMask : register(u1);


// If the clouds are moving fast, the upsampling will most likely not be able to keep up. You can modify these values to relax the effect:
static const float temporalResponse = 0.05;
static const float temporalScale = 2.0;
static const float temporalExposure = 10.0;

// Different aabb clipping method from eg. SSR temporal, suitable for clouds in this case
float4 clip_aabb(float4 aabb_min, float4 aabb_max, float4 prev_sample)
{
	float4 p_clip = 0.5 * (aabb_max + aabb_min);
	float4 e_clip = 0.5 * (aabb_max - aabb_min) + 0.00000001f;

	float4 v_clip = prev_sample - p_clip;
	float4 v_unit = v_clip / e_clip;
	float4 a_unit = abs(v_unit);
	float ma_unit = max(max(a_unit.x, max(a_unit.y, a_unit.z)), a_unit.w);

	if (ma_unit > 1.0)
		return p_clip + v_clip / ma_unit;
	else
		return prev_sample; // point inside aabb
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

	
#if 0 // Exaggerates outline between clouds and geometry 
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
#endif


#if 0 // Standard clipping
	
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

#else // Depth check
	
	float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 1).r; // Half res
	float3 depthWorldPosition = reconstruct_position(uv, depth);
	float tToDepthBuffer = length(depthWorldPosition - GetCamera().position);
	
	float validSampleCount = 1.0;
	
	float4 m1 = 0.0;
	float4 m2 = 0.0;
    [unroll]
	for (uint x = 0; x < 9; x++)
	{
		if (x == 4)
		{
			m1 += sampleColors[x];
			m2 += sampleColors[x] * sampleColors[x];
		}
		else
		{
			float2 reprojectionDepthResults = cloud_reproject_depth.SampleLevel(sampler_point_clamp, uv + (SampleOffset[x] / texelSize), 1);
			if (abs(tToDepthBuffer - reprojectionDepthResults.y) < tToDepthBuffer * 0.1)
			{
				m1 += sampleColors[x];
				m2 += sampleColors[x] * sampleColors[x];
				validSampleCount += 1.0;
			}
		}
	}

	float4 mean = m1 / validSampleCount;
	float4 stddev = sqrt((m2 / validSampleCount) - sqr(mean));
        
#endif

	currentMin = mean - AABBScale * stddev;
	currentMax = mean + AABBScale * stddev;

	currentOutput = sampleColors[4];
	currentMin = min(currentMin, currentOutput);
	currentMax = max(currentMax, currentOutput);
	currentAverage = mean;
}

// Computes post-projection depth from linear depth
float getInverseLinearDepth(float lin, float near, float far)
{
	float z_n = ((lin - 2 * far) * near + far * lin) / (lin * near - far * lin);
	float z = (z_n + 1) / 2;
	return z;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;

#if 0
	
    // Calculate screen dependant motion vector
    float4 prevPos = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    prevPos = mul(GetCamera().inverse_projection, prevPos);
    prevPos = prevPos / prevPos.w;
	
    prevPos.xyz = mul((float3x3)GetCamera().inverse_view, prevPos.xyz);
    prevPos.xyz = mul((float3x3)GetCamera().previous_view, prevPos.xyz);
	
    float4 reproj = mul(GetCamera().projection, prevPos);
    reproj /= reproj.w;
	
    float2 prevUV = reproj.xy * 0.5 + 0.5;
	
#else

	// We must recalculate motion with new upscaled cloud depths:
	
	float x = uv.x * 2 - 1;
	float y = (1 - uv.y) * 2 - 1;
	float2 screenPosition = float2(x, y);

	float currentCloudLinearDepth = cloud_reproject_depth[DTid.xy].x;
	float currentCloudDepth = getInverseLinearDepth(currentCloudLinearDepth, GetCamera().z_near, GetCamera().z_far);
	
	float4 thisClip = float4(screenPosition, currentCloudDepth, 1.0);
	
	float4 prevClip = mul(GetCamera().inverse_view_projection, thisClip);
	prevClip = mul(GetCamera().previous_view_projection, prevClip);
	
	//float4 prevClip = mul(GetCamera().previous_view_projection, worldPosition);
	float2 prevScreen = prevClip.xy / prevClip.w;
	
	float2 screenVelocity = screenPosition - prevScreen;
	float2 prevScreenPosition = screenPosition - screenVelocity;
	
	// Transform from screen position to uv
	float2 prevUV = prevScreenPosition * float2(0.5, -0.5) + 0.5;
    
#endif
		
	float4 previous = cloud_history.SampleLevel(sampler_linear_clamp, prevUV, 0);
    
	float4 current = 0;
	float4 currentMin, currentMax, currentAverage;
	ResolverAABB(cloud_reproject, sampler_point_clamp, 0, temporalExposure, temporalScale, uv, postprocess.resolution, currentMin, currentMax, currentAverage, current);

	//previous = clip_aabb(currentMin.xyz, currentMax.xyz, clamp(currentAverage, currentMin, currentMax), previous);
	previous = clip_aabb(currentMin, currentMax, previous);

	float4 result = lerp(previous, current, temporalResponse);
    
	result = (is_saturated(prevUV) && volumetricclouds_frame > 0) ? result : current;

	output[DTid.xy] = result;

	[branch]
	if (DTid.x % 2 == 0 && DTid.y % 2 == 0)
	{
		// the mask is half the resolution of the clouds
		output_cloudMask[DTid.xy / 2] = pow(saturate(1 - result.a), 64);
	}
}
