#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_lineardepth_minmax, float2, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, float4, 0);

//#define USE_RAYMARCH

#ifdef USE_RAYMARCH
static const uint	coarseStepCount = 16;       // primary ray march step count (higher will find more in distance, but slower)
static const float	coarseStepIncrease = 1.18f; // primary ray march step increase (higher will travel more distance, but can miss details)
static const uint	fineStepCount = 16;	    // binary step count (higher is nicer but slower)
static const float  tolerance = 0.9f;		    // early exit factor for binary search (smaller is nicer but slower)

float4 SSRBinarySearch(in float3 origin, in float3 direction)
{
	for (uint i = 0; i < fineStepCount; i++)
	{
		float4 coord = mul(g_xCamera_Proj, float4(origin, 1.0f));
		coord.xy /= coord.w;
		coord.xy = coord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		const float depth = texture_lineardepth_minmax.SampleLevel(sampler_point_clamp, coord.xy, 0).g * g_xCamera_ZFarP;

		if (abs(origin.z - depth) < tolerance)
			return float4(coord.xy, depth, 1);

		if (origin.z <= depth)
			origin += direction;

		direction *= 0.5f;
		origin -= direction;
	}

	return 0;
}

float4 SSRRayMarch(in float3 origin, in float3 direction)
{
	for (uint i = 0; i < coarseStepCount; i++)
	{
		origin += direction;

		float4 coord = mul(g_xCamera_Proj, float4(origin, 1.0f));
		coord.xy /= coord.w;
		coord.xy = coord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		const float depth = texture_lineardepth_minmax.SampleLevel(sampler_point_clamp, coord.xy, 0).r * g_xCamera_ZFarP;

		if (origin.z > depth)
			return SSRBinarySearch(origin, direction);

		direction *= coarseStepIncrease;
	}

	return 0;
}

#else

static const float rayTraceStride = 0.0f;           // Step in horizontal or vertical pixels between samples.
static const float rayTraceMaxStep = 900.0f;        // Maximum number of iterations. Higher gives better images but may be slow.
static const float rayTraceHitThickness = 1.5f;     // Thickness to ascribe to each pixel in the depth buffer.
static const float rayTraceHitThicknessBias = 7.0f; // Bias to control the thickness along distance.
static const float rayTraceMaxDistance = 1000.0f;   // Maximum camera-space distance to trace before returning a miss.
static const float rayTraceStrideCutoff = 100.0f;   // More distant pixels are smaller in screen space. This value tells at what point to
							// start relaxing the stride to give higher quality reflections for objects far from the camera.

static const float blendScreenEdgeFade = 5.0f;
static const bool  blendReflectSky = true;

float DistanceSquared(float2 a, float2 b)
{
    a -= b;
    return dot(a, a);
}

bool intersectsDepthBuffer(float z, float minZ, float maxZ)
{
    // Increase thickness along distance. 
    // This will help objects from dissapering in the distance.
    float thicknessScale = min(1.0f, z / rayTraceStrideCutoff);
    float thickness = rayTraceHitThickness * rayTraceHitThicknessBias * thicknessScale;
    thickness = clamp(thickness, rayTraceHitThickness, 10.0f);
    
    // Effectively remove line/tiny artifacts, mostly caused by Zbuffers precision.
    float depthScale = min(1.0f, z / rayTraceStrideCutoff);
    z += lerp(0.05f, 0.0f, depthScale);
    
    return (minZ >= z) && (maxZ - thickness <= z);
}

// Heavily adapted from McGuire and Mara's original implementation
// http://casual-effects.blogspot.com/2014/08/screen-space-ray-tracing.html
bool ScreenSpaceRayTrace(float3 csOrig, float3 csDir, float jitter, out float2 hitPixel, out float3 hitPoint, out float iterationCount)
{
    float rayLength = ((csOrig.z + csDir.z * rayTraceMaxDistance) < g_xCamera_ZNearP) ?	(g_xCamera_ZNearP - csOrig.z) / csDir.z : rayTraceMaxDistance;
    
    float3 csRayEnd = csOrig + csDir * rayLength;

	// Project into homogeneous clip space
    float4 clipRayOrigin = mul(g_xCamera_Proj, float4(csOrig, 1.0f));
    float4 clipRayEnd = mul(g_xCamera_Proj, float4(csRayEnd, 1.0f));

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

    P0.xy *= xPPResolution.xy;
    P1.xy *= xPPResolution.xy;
    
#if 1
    // Clip to the screen coordinates. Alternatively we could just modify rayTraceMaxStep instead
    // This will also improve the framerate, without losing quality or features
    float2 yDelta = float2(xPPResolution.y + 1.0f, -1.0f); // - 0.5, 0.5
    float2 xDelta = float2(xPPResolution.x + 1.0f, -1.0f); // - 0.5, 0.5
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
    float2 dP = float2(stepDirection, screenOffset.y * stepInterval);

    // Scale derivatives by the desired pixel stride and then offset the starting values by the jitter fraction
    float strideScale = 1.0f - min(1.0f, csOrig.z / rayTraceStrideCutoff);
    float stride = 1.0f + strideScale * rayTraceStride;
    
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
    
    float prevZMaxEstimate = csOrig.z;
    float rayZMin = prevZMaxEstimate;
    float rayZMax = prevZMaxEstimate;
    float sceneZMax = rayZMax + 100000.0f;
    
    [loop]
    for (;((PQk.x * stepDirection) <= end) && 
        (stepCount <= rayTraceMaxStep - 1) &&
		!intersectsDepthBuffer(sceneZMax, rayZMin, rayZMax) &&
		(sceneZMax != 0.0f);
		PQk += dPQk, stepCount++)
    {
        rayZMin = prevZMaxEstimate;
        rayZMax = (dPQk.z * 0.5f + PQk.z) / (dPQk.w * 0.5f + PQk.w);
        prevZMaxEstimate = rayZMax;

        if (rayZMin > rayZMax)
        {            
            float t = rayZMin;
            rayZMin = rayZMax;
            rayZMax = t;
        }

        hitPixel = permute ? PQk.yx : PQk.xy;
        hitPixel *= xPPResolution_rcp;
        
        sceneZMax = texture_lineardepth_minmax.SampleLevel(sampler_point_clamp, hitPixel, 0).g * g_xCamera_ZFarP;
        //sceneZMax = texture_lineardepth.SampleLevel(sampler_point_clamp, hitPixel, 0).r * g_xCamera_ZFarP; // Traces better because of higher resolution, but steals some performance.
        //sceneZMax = getLinearDepth(texture_depth.SampleLevel(sampler_point_clamp, hitPixel, 0).r); // Float precision is unnecessary and is really performance heavy!
    }
    
    // Advance Q based on the number of steps
    Q.xy += dQ.xy * stepCount;
    hitPoint = Q * (1.0f / PQk.w);
    iterationCount = stepCount;

    return intersectsDepthBuffer(sceneZMax, rayZMin, rayZMax);
}

float CalculateBlendIntersection(float iterationCount, float2 hitPixel, bool hit)
{
    float confidence = 1.0 - pow(iterationCount / rayTraceMaxStep, 8.0f);

    float2 hitPixelNDC = hitPixel * 2.0 - 1.0;
    
    //float maxDimension = min(1.0, max(abs(hitPixelNDC.x), abs(hitPixelNDC.y)));
    //float attenuation = 1.0 - max(0.0, maxDimension - blendScreenEdgeFade) / (1.0 - blendScreenEdgeFade);

    float2 vignette = saturate(abs(hitPixelNDC) * blendScreenEdgeFade - (blendScreenEdgeFade - 1.0f));
    float attenuation = saturate(1.0 - dot(vignette, vignette));
    
    float blend = confidence * attenuation;
    
    if (!hit && !blendReflectSky)
        blend = 0.0;
    
    return blend;
}

#endif

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);		
	if (depth == 0.0f) 
		return;

	// Everything in view space:
	const float3 P = reconstructPosition(uv, depth, g_xCamera_InvP); // specify matrix to get view-space position!
	const float3 N = mul((float3x3)g_xCamera_View, decodeNormal(texture_gbuffer1.SampleLevel(sampler_point_clamp, uv, 0).xy)).xyz;
	const float3 R = normalize(reflect(P.xyz, N.xyz));

#ifdef USE_RAYMARCH
	const float4 hit = SSRRayMarch(P, R);

	float4 color;
	if (hit.w)
	{
		const float2 edgefactor = 1 - pow(saturate(abs(hit.xy - 0.5) * 2), 8);

		const float blend = saturate(
			min(edgefactor.x, edgefactor.y) *	// screen edge fade
			saturate(R.z)						// camera facing fade
		);

		color = max(0, float4(input.SampleLevel(sampler_linear_clamp, hit.xy, 0).rgb, blend));
	}
	else
	{
		color = 0;
	}

	output[DTid.xy] = color;
#else

    float2 hitPixel = float2(0.0f, 0.0f);
    float3 hitPoint = float3(0.0f, 0.0f, 0.0f);
    float iterationCount = 0.0f;

    float2 uv2 = (DTid.xy + 0.5f);
    //float jitter = 1.0f + rand(uv2 + g_xFrame_Time);
    float jitter = 1.0f + InterleavedGradientNoise(uv2, (g_xFrame_FrameCount % 8) * (g_xFrame_Options & OPTION_BIT_TEMPORALAA_ENABLED)) * 0.5f; // Seems like a more stable noise
    
    bool hit = ScreenSpaceRayTrace(P, R, jitter, hitPixel, hitPoint, iterationCount);
    
    if (!is_saturated(hitPixel))
    {
        hit = false;
    }
    
    float blend = CalculateBlendIntersection(iterationCount, hitPixel, hit);
    
    float4 color = max(0, float4(input.SampleLevel(sampler_linear_clamp, hitPixel.xy, 0).rgb, blend));
    //float4 color = float4(blend.xxx, 1.0f); // quick debug
    
    output[DTid.xy] = color;

#endif
}
