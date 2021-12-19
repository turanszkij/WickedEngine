#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> texture_raytrace : register(u0);
RWTexture2D<float> texture_rayLengths : register(u1);

static const float rayTraceStrideMin = 1.0f; // Step in horizontal or vertical pixels between samples.
static const float rayTraceStrideMax = 10.0f; // Define max stride between samples. Roughness will interpolate between it's min and max counterparts.
static const float rayTraceMaxStep = 512.0f; // Maximum number of iterations. Higher gives better images but may be slow.
static const float rayTraceThicknessOffset = 0.0f; // Increse or decrease thickness for each pixels in the depth buffer. [- / +]
static const float rayTraceThicknessBias = 1.0f; // Bias to control the growth of the thickness.
static const bool raytraceThicknessInfinite = false; // Use infinite thickness for maximum performance, but may not be suitable for most scenes.
static const float rayTraceMaxDistance = 1000.0f; // Maximum camera-space distance to trace before returning a miss.
static const float rayTraceStrideCutoff = 100.0f; // More distant pixels are smaller in screen space. This value tells at what point to
                                                            // start relaxing the stride to give higher quality reflections for objects far from the camera.
static const float raytraceHZBBias = 0.05f; // This value tells how fast the roughness increases the level.
static const float raytraceHZBStartLevel = 1.0f;
static const float raytraceHZBMinStep = 0.005f; // Minimum level increasement per iteration.


float DistanceSquared(float2 a, float2 b)
{
	a -= b;
	return dot(a, a);
}

bool IntersectsDepthBuffer(float sceneZMax, float rayZMin, float rayZMax)
{
    // Increase thickness along distance. 
	float thickness = max(sceneZMax * rayTraceThicknessBias + rayTraceThicknessOffset, 1.0);

#if 0 // precision issues in DX12
    // Effectively remove line/tiny artifacts, mostly caused by Zbuffers precision.
	float depthScale = min(1.0f, sceneZMax / rayTraceStrideCutoff);
	sceneZMax += lerp(0.05f, 0.0f, depthScale);
#endif
    
	if (raytraceThicknessInfinite)
		return (rayZMin >= sceneZMax);
	else
		return (rayZMin >= sceneZMax) && (rayZMax - thickness <= sceneZMax);
}

// Heavily adapted from McGuire and Mara's original implementation
// http://casual-effects.blogspot.com/2014/08/screen-space-ray-tracing.html
bool ScreenSpaceRayTrace(float3 csOrig, float3 csDir, float jitter, float roughness, out float2 hitPixel, out float3 hitPoint, out float iterations)
{
	csOrig += csDir * 0.001; // precision issues in DX12
	float rayLength = ((csOrig.z + csDir.z * rayTraceMaxDistance) < GetCamera().z_near) ?
        (GetCamera().z_near - csOrig.z) / csDir.z : rayTraceMaxDistance;
    
	float3 csRayEnd = csOrig + csDir * rayLength;

    // Project into homogeneous clip space
	float4 clipRayOrigin = mul(GetCamera().projection, float4(csOrig, 1.0f));
	float4 clipRayEnd = mul(GetCamera().projection, float4(csRayEnd, 1.0f));
    
	float k0 = 1.0f / clipRayOrigin.w;
	float k1 = 1.0f / clipRayEnd.w;

	float3 Q0 = csOrig * k0;
	float3 Q1 = csRayEnd * k1;
	  
    // Screen-space endpoints
	float2 P0 = clipRayOrigin.xy * k0;
	float2 P1 = clipRayEnd.xy * k1;

    // Project to pixel
	P0 = P0 * float2(0.5, -0.5) + float2(0.5, 0.5);
	P1 = P1 * float2(0.5, -0.5) + float2(0.5, 0.5);

	P0.xy *= postprocess.resolution.xy;
	P1.xy *= postprocess.resolution.xy;
    
#if 0
    // Clip to the screen coordinates. Alternatively we could just modify rayTraceMaxStep instead
    float2 yDelta = float2(postprocess.resolution.y + 2.0f, -2.0f); // - 0.5, 0.5
    float2 xDelta = float2(postprocess.resolution.x + 2.0f, -2.0f); // - 0.5, 0.5
    float alpha = 0.0;
    
    // P0 must be in bounds
    if (P1.y > yDelta.x || P1.y < yDelta.y) 
    {
        float yClip = (P1.y > yDelta.x) ? yDelta.x : yDelta.y;
        float yAlpha = (P1.y - yClip) / (P1.y - P0.y);
        alpha = yAlpha;
    }

    // P1 must be in bounds
    if (P1.x > xDelta.x || P1.x < xDelta.y) 
    {
        float xClip = (P1.x > xDelta.x) ? xDelta.x : xDelta.y;
        float xAlpha = (P1.x - xClip) / (P1.x - P0.x);
        alpha = max(alpha, xAlpha);
    }

    // These are all in homogeneous space, so they interpolate linearly
    P1 = lerp(P1, P0, alpha);
    k1 = lerp(k1, k0, alpha);
    Q1 = lerp(Q1, Q0, alpha);
#endif
    
    // If the line is degenerate, make it cover at least one pixel to avoid handling zero-pixel extent as a special case later
	P1 += (DistanceSquared(P0, P1) < 0.0001f) ? float2(0.01f, 0.01f) : 0.0f;
	float2 screenOffset = P1 - P0;

    // Permute so that the primary iteration is in x to collapse all quadrant-specific DDA cases later
	bool permute = false;
	if (abs(screenOffset.x) < abs(screenOffset.y))
	{
		permute = true;
		screenOffset = screenOffset.yx;
		P0 = P0.yx;
		P1 = P1.yx;
	}

	float stepDirection = sign(screenOffset.x);
	float stepInterval = stepDirection / screenOffset.x;

    // Track the derivatives of Q and k
	float3 dQ = (Q1 - Q0) * stepInterval;
	float dk = (k1 - k0) * stepInterval;
    
    // Because we test 1/2 a texel forward along the ray, on the very last iteration
    // the interpolation can go past the end of the ray. Use these bounds to clamp it.
	float zMin = min(csRayEnd.z, csOrig.z);
	float zMax = max(csRayEnd.z, csOrig.z);
    
	float2 dP = float2(stepDirection, screenOffset.y * stepInterval);

    // Scale derivatives by the desired pixel stride and then offset the starting values by the jitter fraction
#if 1 // Stride based on roughness. Matte materials will recieve higher stride
	float alphaRoughness = roughness * roughness;
	float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    
	float strideScale = 1.0f - min(1.0f, csOrig.z / rayTraceStrideCutoff);
	float strideRoughnessScale = lerp(rayTraceStrideMin, rayTraceStrideMax, min(alphaRoughnessSq, 1.0)); // Climb exponentially at extreme conditions
	float stride = 1.0 + strideScale * strideRoughnessScale;
#else
    float strideScale = 1.0f - min(1.0f, csOrig.z / rayTraceStrideCutoff);
	float stride = 1.0 + strideScale * rayTraceStrideMin;
#endif
    
	dP *= stride;
	dQ *= stride;
	dk *= stride;

	P0 += dP * jitter;
	Q0 += dQ * jitter;
	k0 += dk * jitter;

	float4 PQk = float4(P0, Q0.z, k0);
	float4 dPQk = float4(dP, dQ.z, dk);
	float3 Q = Q0;

    // Adjust end condition for iteration direction
	float end = P1.x * stepDirection;

	float stepCount = 0.0f;
	float level = raytraceHZBStartLevel;

	float prevZMaxEstimate = csOrig.z;
	float rayZMin = prevZMaxEstimate;
	float rayZMax = prevZMaxEstimate;
	float sceneZMax = rayZMax + 100000.0f;
    
    [loop]
	for (; ((PQk.x * stepDirection) <= end) &&
        (stepCount < rayTraceMaxStep) &&
        !IntersectsDepthBuffer(sceneZMax, rayZMin, rayZMax) &&
        (sceneZMax != 0.0f);
        PQk += dPQk, stepCount++)
	{
		if (any(hitPixel < 0.0) || any(hitPixel > 1.0))
		{
			return false;
		}
        
		rayZMin = prevZMaxEstimate;
        
        // Compute the value at 1/2 step into the future
		rayZMax = (dPQk.z * 0.5f + PQk.z) / (dPQk.w * 0.5f + PQk.w);
		rayZMax = clamp(rayZMax, zMin, zMax);
        
		prevZMaxEstimate = rayZMax;
        
		if (rayZMin > rayZMax)
		{
			float t = rayZMin;
			rayZMin = rayZMax;
			rayZMax = t;
		}

        // A simple HZB approach based on roughness
		level += max(raytraceHZBBias * roughness, raytraceHZBMinStep);
		level = min(level, 6.0f);

		hitPixel = permute ? PQk.yx : PQk.xy;
		hitPixel *= postprocess.resolution_rcp;
        
		sceneZMax = texture_lineardepth.SampleLevel(sampler_linear_clamp, hitPixel, level) * GetCamera().z_far;
	}
    
    // Undo the last increment, which ran after the test variables were set up
	//PQk -= dPQk;
	//stepCount -= 1.0;
    
    // Advance Q based on the number of steps
	Q.xy += dQ.xy * stepCount;
	Q.z = PQk.z;
	hitPoint = Q * (1.0f / PQk.w);
	iterations = stepCount;

	return IntersectsDepthBuffer(sceneZMax, rayZMin, rayZMax);
}


[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;
	const float depth = texture_depth.SampleLevel(sampler_linear_clamp, uv, 1);
	if (depth == 0)
		return;

	PrimitiveID prim;
	prim.unpack(texture_gbuffer0[DTid.xy * 2]);

	Surface surface;
	if (!surface.load(prim, reconstruct_position(uv, depth)))
	{
		return;
	}
	if (surface.roughness > 0.6)
	{
		texture_raytrace[DTid.xy] = 0;
		texture_rayLengths[DTid.xy] = 0;
		return;
	}

	// Everything in view space:
	float3 N = normalize(mul((float3x3)GetCamera().view, surface.N));
	float3 P = reconstruct_position(uv, depth, GetCamera().inverse_projection);
	float3 V = normalize(-P);
	const float roughness = GetRoughness(surface.roughness);
    
	const float roughnessFade = GetRoughnessFade(roughness, SSRMaxRoughness);
	if (roughnessFade <= 0)
	{
		texture_raytrace[DTid.xy] = 0;
		return;
	}
    
	float4 H;
	float3 L;
	float jitter;
	if (roughness > 0.05f)
	{
		float3x3 tangentBasis = GetTangentBasis(N);
		float3 tangentV = mul(tangentBasis, V);

#ifdef GGX_SAMPLE_VISIBLE
        
#if 1
		const float2 bluenoise = blue_noise(DTid.xy).xy;

		float2 Xi = bluenoise.xy;
            
		Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);

		H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);
        
#else // Old
        
        // Low-discrepancy sequence
		uint2 Random = Rand_PCG16(int3((DTid.xy + 0.5f), GetFrame().frame_count)).xy;
            
		float2 Xi = HammersleyRandom16(1, Random); // SingleSPP
            
		Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);
            
		H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);
        
#endif

        // Tangent to world
		H.xyz = mul(H.xyz, tangentBasis);
        
#else
        
		const float surfaceMargin = 0.0f;
		const float maxRegenCount = 15.0f;
        
		uint2 Random = Rand_PCG16(int3((DTid.xy + 0.5f), GetFrame().frame_count)).xy;
        
        // By using an uniform importance sampling method, some rays go below the surface.
        // We simply re-generate them at a negligible cost, to get some nice ones.
        
		float RdotN = 0.0f;
		float regenCount = 0;
        [loop]
		for (; RdotN <= surfaceMargin && regenCount < maxRegenCount; regenCount++)
		{
            // Low-discrepancy sequence
            //float2 Xi = float2(Random) * rcp(65536.0); // equivalent to HammersleyRandom(0, 1, Random).
			float2 Xi = HammersleyRandom16(regenCount, Random); // SingleSPP
            
			Xi.y = lerp(Xi.y, 0.0, GGX_IMPORTANCE_SAMPLE_BIAS);
            
			H = ImportanceSampleGGX(Xi, roughness);
            
            // Tangent to world
			H.xyz = mul(H.xyz, tangentBasis);
            
			RdotN = dot(N, reflect(-V, H.xyz));
		}
        
#endif
        
		L = reflect(-V, H.xyz);
		jitter = InterleavedGradientNoise(DTid.xy, GetFrame().frame_count);
	}
	else
	{
		H = float4(N.xyz, 1.0f);
		L = reflect(-V, H.xyz);
		jitter = 0;
	}
    
	float2 hitPixel = float2(0.0f, 0.0f);
	float3 hitPoint = float3(0.0f, 0.0f, 0.0f);
	float iterations = 0.0f;

	bool hit = ScreenSpaceRayTrace(P, L, jitter, roughness, hitPixel, hitPoint, iterations);


	float hitDepth = texture_depth.SampleLevel(sampler_linear_clamp, hitPixel, 1);

    // Output:
    // xy: hit pixel
    // z:  hit depth
    // w:  pdf
	float4 raytrace = max(0, float4(hitPixel, hitDepth, H.w));
	texture_raytrace[DTid.xy] = raytrace;

	if (hit)
	{
		const float3 Phit = reconstruct_position(uv, hitDepth, GetCamera().inverse_projection);
		texture_rayLengths[DTid.xy] = distance(P, Phit);
	}
	else
	{
		texture_rayLengths[DTid.xy] = 0;
	}
}
