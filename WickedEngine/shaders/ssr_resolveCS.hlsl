#include "globals.hlsli"
#include "brdf.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(texture_raytrace, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_main, float4, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(texture_resolve, float4, 0);


static const float2 spatialReuseOffsets3x3[9] =
{
	float2(0.0, 0.0),
	float2(0.0, 1.0),
	float2(1.0, -1.0),
	float2(-1.0, -1.0),
	float2(-1.0, 0.0),
	float2(0.0, -1.0),
	float2(1.0, 0.0),
	float2(-1.0, 1.0),
	float2(1.0, 1.0)
};

// Not in use, but could perhaps be useful in the future.
/*float2 CalculateTailDirection(float3 viewNormal)
{
	float3 upVector = abs(viewNormal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 T = normalize(cross(upVector, viewNormal));

	float tailDirection = T.x * -viewNormal.y;
    
	return lerp(float2(1.0, 0.1), float2(0.1, 1.0), tailDirection);
}*/

float CalculateEdgeFade(float2 hitPixel)
{
	float2 hitPixelNDC = hitPixel * 2.0 - 1.0;
    
    //float maxDimension = min(1.0, max(abs(hitPixelNDC.x), abs(hitPixelNDC.y)));
    //float attenuation = 1.0 - max(0.0, maxDimension - blendScreenEdgeFade) / (1.0 - blendScreenEdgeFade);

	float2 vignette = saturate(abs(hitPixelNDC) * SSRBlendScreenEdgeFade - (SSRBlendScreenEdgeFade - 1.0f));
	float attenuation = saturate(1.0 - dot(vignette, vignette));
    
	return attenuation;
}

void GetSampleInfo(float2 velocity, float2 neighborUV, float2 uv, float3 P, float3 V, float3 N, float NdotV, float specularConeTangent, float roughness, out float4 sampleColor, out float weight)
{
    // Sample local pixel information
	float4 raytraceSource = texture_raytrace.SampleLevel(sampler_point_clamp, neighborUV, 0);
    
	float2 hitPixel = raytraceSource.xy + velocity;
	float hitDepth = raytraceSource.z;
	float hitPDF = raytraceSource.w;

	float intersectionCircleRadius = specularConeTangent * length(hitPixel - uv);
	float sourceMip = clamp(log2(intersectionCircleRadius * ssr_input_resolution_max), 0.0, ssr_input_maxmip) * SSRResolveConeMip;
    
	sampleColor.rgb = texture_main.SampleLevel(sampler_linear_clamp, hitPixel, sourceMip).rgb; // Scene color
	sampleColor.a = CalculateEdgeFade(raytraceSource.xy); // Opacity - Since this is used for masking, we can ignore velocity 
    
    // BRDF Weight
    
	float3 hitViewPosition = reconstructPosition(hitPixel, hitDepth, g_xCamera_InvP);
    
	float3 L = normalize(hitViewPosition - P);
	float3 H = normalize(L + V);

	float NdotH = saturate(dot(N, H));
	float NdotL = saturate(dot(N, L));
    
	Surface surface;
	surface.roughnessBRDF = roughness * roughness;
	surface.NdotV = NdotV;
    
	SurfaceToLight surfaceToLight;
	surfaceToLight.NdotH = NdotH;
	surfaceToLight.NdotL = NdotL;
    
    // Calculate BRDF where Fresnel = 1
	float Vis = V_SmithGGXCorrelated(surface.roughnessBRDF, surface.NdotV, surfaceToLight.NdotL);
	float D = D_GGX(surface.roughnessBRDF, surfaceToLight.NdotH, surfaceToLight.H);
	float specularLight = Vis * D * PI / 4.0;

	weight = specularLight / max(hitPDF, 0.00001f);
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 1);
	if (depth == 0.0f)
		return;

	const float2 velocity = texture_gbuffer2.SampleLevel(sampler_point_clamp, uv, 0).xy;
	const float2 prevUV = uv + velocity;

    // Everthing in view space:
	const float4 g1 = texture_gbuffer1.SampleLevel(sampler_linear_clamp, prevUV, 0);
	const float3 P = reconstructPosition(uv, depth, g_xCamera_InvP);
	const float3 N = normalize(mul((float3x3)g_xCamera_View, g1.rgb * 2 - 1).xyz);
	const float3 V = normalize(-P);
	const float NdotV = saturate(dot(N, V));
    
	const float roughness = GetRoughness(g1.a);

    // Early out, useless if the roughness is out of range
	float roughnessFade = GetRoughnessFade(roughness, SSRMaxRoughness);
	if (roughnessFade <= 0.0f)
	{
		texture_resolve[DTid.xy] = 0;
		return;
	}
	
	// Since we aren't importance sampling in this range, no need to resolve
	if (roughness < 0.05f)
	{
		float4 raytraceSource = texture_raytrace.SampleLevel(sampler_point_clamp, uv, 0);
		float2 hitPixel = raytraceSource.xy + velocity;
		
		float4 sampleColor;
		sampleColor.rgb = texture_main.SampleLevel(sampler_linear_clamp, hitPixel, 0).rgb; // Scene color
		sampleColor.a = CalculateEdgeFade(raytraceSource.xy); // Opacity
		
		texture_resolve[DTid.xy] = sampleColor;
		return;
	}
	
    
    // Cone mip sampling
	float specularConeTangent = lerp(0.0, roughness * (1.0 - GGX_IMPORTANCE_SAMPLE_BIAS), NdotV * sqrt(roughness));
	specularConeTangent *= lerp(saturate(NdotV * 2), 1.0f, sqrt(roughness));
    
    
#if 1 // EAW spatial resolve
    
    
	float4 result = 0.0f;
	float weightSum = 0.0f;
    
#define BLOCK_SAMPLE_RADIUS 1
    
    [unroll]
	for (int y = -BLOCK_SAMPLE_RADIUS; y <= BLOCK_SAMPLE_RADIUS; y++)
	{
        [loop]
		for (int x = -BLOCK_SAMPLE_RADIUS; x <= BLOCK_SAMPLE_RADIUS; x++)
		{
			if (uint(abs(x) + abs(y)) % 2 == 0)
				continue;
			
			float2 offsetUV = float2(x, y) * xPPResolution_rcp * SSRResolveSpatialSize;
			float2 neighborUV = uv + offsetUV;
            
			float4 sampleColor;
			float weight;
			GetSampleInfo(velocity, neighborUV, uv, P, V, N, NdotV, specularConeTangent, roughness, sampleColor, weight);
            
			sampleColor.rgb *= rcp(1 + Luminance(sampleColor.rgb));
            
			result += sampleColor * weight;
			weightSum += weight;
		}
	}
	result /= weightSum;
    
	result.rgb *= rcp(1 - Luminance(result.rgb));
    
#undef BLOCK_SAMPLE_RADIUS
	
    
#else // Frostbite presentation, spatial resolve
    

	float4 result = 0.0f;
	float weightSum = 0.0f;
    
#define NUM_RESOLVE 4 // Four samples to achieve effective ray reuse patterns
	
    [unroll]
	for (uint i = 0; i < NUM_RESOLVE; i++)
	{
		float2 offsetUV = spatialReuseOffsets3x3[i] * xPPResolution_rcp * SSRResolveSpatialSize;
		float2 neighborUV = uv + offsetUV;
        
		float4 sampleColor;
		float weight;
		GetSampleInfo(velocity, neighborUV, uv, P, V, N, NdotV, specularConeTangent, roughness, sampleColor, weight);
        
		sampleColor.rgb *= rcp( 1 + Luminance(sampleColor.rgb) );
		
		result += sampleColor * weight;
		weightSum += weight;
	}
	result /= weightSum;
    
	result.rgb *= rcp( 1 - Luminance(result.rgb) );
	
#undef NUM_RESOLVE
    
    
#endif
    
    
	result *= roughnessFade;
	result *= SSRIntensity;
    
	texture_resolve[DTid.xy] = max(result, 0.00001f);
}
