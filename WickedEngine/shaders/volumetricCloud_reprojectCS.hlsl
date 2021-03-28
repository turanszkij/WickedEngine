#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(cloud_current, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(cloud_depth, float, TEXSLOT_ONDEMAND1);
TEXTURE2D(cloud_history, float4, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(output, float4, 0);


// The rendering uses a temporal upsampling pass similar to Frostbite. See https://odr.chalmers.se/handle/20.500.12380/241770

// If the clouds are moving fast, the upsampling will most likely not be able to keep up. You can modify these values to relax the effect:
static const float temporalResponse = 0.05;
static const float temporalScale = 3.0;
static const float temporalExposure = 10.0;

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

/*float2 CalculateCustomMotion(float4 worldPosition)
{
	float4 thisClip = mul(g_xCamera_VP, worldPosition);
	float4 prevClip = mul(g_xCamera_PrevVP, worldPosition);
    
	float2 thisScreen = thisClip.xy * rcp(thisClip.w);
	float2 prevScreen = prevClip.xy * rcp(prevClip.w);
	thisScreen = (thisScreen.xy * float2(0.5, -0.5) + 0.5);
	prevScreen = (prevScreen.xy * float2(0.5, -0.5) + 0.5);
    
	return thisScreen - prevScreen;
}*/

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
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
    
#if 0
    
    // Calculate screen dependant motion vector
    float4 prevPos = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    prevPos = mul(g_xCamera_InvP, prevPos);
    prevPos = prevPos / prevPos.w;
    
    prevPos.xyz = mul((float3x3)g_xCamera_InvV, prevPos.xyz);
    prevPos.xyz = mul((float3x3)g_xCamera_PrevV, prevPos.xyz);
    
    float4 reproj = mul(g_xCamera_Proj, prevPos);
    reproj /= reproj.w;
    
    float2 prevUV = reproj.xy * 0.5 + 0.5;
            
#else

	float x = uv.x * 2 - 1;
	float y = (1 - uv.y) * 2 - 1;
	float2 screenPosition = float2(x, y);

	float cloudLinearDepth = cloud_depth.SampleLevel(sampler_linear_clamp, uv, 0).r;
	float cloudDepth = getInverseLinearDepth(cloudLinearDepth, g_xCamera_ZNearP, g_xCamera_ZFarP);
	
	float4 thisClip = float4(screenPosition, cloudDepth, 1.0);
	
	float4 prevClip = mul(g_xCamera_InvVP, thisClip);
	prevClip = mul(g_xCamera_PrevVP, prevClip);
	
	//float4 prevClip = mul(g_xCamera_PrevVP, worldPosition);
	float2 prevScreen = prevClip.xy / prevClip.w;
	
	float2 screenVelocity = screenPosition - prevScreen;
	float2 prevScreenPosition = screenPosition - screenVelocity;
	
	// Transform from screen position to uv
	float2 prevUV = prevScreenPosition * float2(0.5, -0.5) + 0.5;
    
#endif
    
	float4 previous = cloud_history.SampleLevel(sampler_linear_clamp, prevUV, 0);
    
	float4 current = 0;
	float4 currentMin, currentMax, currentAverage;
	ResolverAABB(cloud_current, sampler_point_clamp, 0, temporalExposure, temporalScale, uv, xPPResolution, currentMin, currentMax, currentAverage, current);

	previous = clip_aabb(currentMin.xyz, currentMax.xyz, clamp(currentAverage, currentMin, currentMax), previous);
    
	float4 result = lerp(previous, current, temporalResponse);
    
	result = is_saturated(prevUV) ? result : current;

	output[DTid.xy] = result;
}
