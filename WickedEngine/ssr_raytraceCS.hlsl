#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(texture_raytrace, float4, 0);
RWTEXTURE2D(texture_mask, float2, 1);

// Use this to use reduced precision, but higher framerate:
#define USE_LINEARDEPTH

static const float rayTraceStride = 1.0f;           // Step in horizontal or vertical pixels between samples.
static const float rayTraceMaxStep = 512.0f;        // Maximum number of iterations. Higher gives better images but may be slow.
static const float rayTraceHitThickness = 1.5f;     // Thickness to ascribe to each pixel in the depth buffer.
static const float rayTraceHitThicknessBias = 7.0f; // Bias to control the thickness along distance.
static const float rayTraceMaxDistance = 1000.0f;   // Maximum camera-space distance to trace before returning a miss.
static const float rayTraceStrideCutoff = 100.0f;   // More distant pixels are smaller in screen space. This value tells at what point to
                                                      // start relaxing the stride to give higher quality reflections for objects far from the camera.
static const float raytraceHZBBias = 1.0f;

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
bool ScreenSpaceRayTrace(float3 csOrig, float3 csDir, float jitter, float roughness, out float2 hitPixel, out float3 hitPoint, out float iterationCount)
{    
    float rayLength = ((csOrig.z + csDir.z * rayTraceMaxDistance) < g_xCamera_ZNearP) ?	(g_xCamera_ZNearP - csOrig.z) / csDir.z : rayTraceMaxDistance;
    
    float3 csRayEnd = csOrig + csDir * rayLength;

    // Project into homogeneous clip space
    float4 clipRayOrigin = mul(g_xCamera_Proj, float4(csOrig, 1.0f));
    float4 clipRayEnd    = mul(g_xCamera_Proj, float4(csRayEnd, 1.0f));

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
    
#if 0
    // Clip to the screen coordinates. Alternatively we could just modify rayTraceMaxStep instead
    float2 yDelta = float2(xPPResolution.y + 2.0f, -2.0f); // - 0.5, 0.5
    float2 xDelta = float2(xPPResolution.x + 2.0f, -2.0f); // - 0.5, 0.5
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
    
    // raytrace iterations based on roughness
    // Matte materials will get less samples
    float roughnessTraceStep = max(rayTraceMaxStep * (1.0 - roughness), 1.0f);

    float stepCount = 0.0f;
    float level = 0.0f; // 1.0f start level. Parameter?

    float prevZMaxEstimate = csOrig.z;
    float rayZMin = prevZMaxEstimate;
    float rayZMax = prevZMaxEstimate;
    float sceneZMax = rayZMax + 100000.0f;
    
    [loop]
    for (; ((PQk.x * stepDirection) <= end) &&
        (stepCount <= roughnessTraceStep - 1) &&
        !intersectsDepthBuffer(sceneZMax, rayZMin, rayZMax) &&
        (sceneZMax != 0.0f) &&
        (level > -1);
        PQk += dPQk, stepCount++)
    {
        if (!is_saturated(hitPixel))
        {
            return false;
        }
        
        rayZMin = prevZMaxEstimate;
        
        // Compute the value at 1/2 step into the future
        rayZMax = (dPQk.z * 0.5f + PQk.z) / (dPQk.w * 0.5f + PQk.w);
        rayZMax = clamp(rayZMax, zMin, zMax);
        prevZMaxEstimate = rayZMax;
        
        [flatten]
        if (rayTraceMaxDistance < rayZMax)
        {
            return false;
        }
        
        [flatten]
        if (rayZMin > rayZMax)
        {
            float t = rayZMin;
            rayZMin = rayZMax;
            rayZMax = t;            
        }

        // A simple HZB approach based on roughness
        level += min(raytraceHZBBias / 10.0f, 6.0f) * roughness;
        
        hitPixel = permute ? PQk.yx : PQk.xy;
        hitPixel *= xPPResolution_rcp;
        
        #ifdef USE_LINEARDEPTH
        sceneZMax = texture_lineardepth.SampleLevel(sampler_point_clamp, hitPixel, level) * g_xCamera_ZFarP;
        #else
        sceneZMax = getLinearDepth(texture_depth.SampleLevel(sampler_point_clamp, hitPixel, 0).r);
        #endif        
    }
    
    // Advance Q based on the number of steps
    Q.xy += dQ.xy * stepCount;
    hitPoint = Q * (1.0f / PQk.w);
    iterationCount = stepCount;

    return intersectsDepthBuffer(sceneZMax, rayZMin, rayZMax);
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
    const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);		
    if (depth == 0.0f)
      return;

    // Everything in view space:
    const float3 P = reconstructPosition(uv, depth, g_xCamera_InvP);
    const float3 N = mul((float3x3)g_xCamera_View, decodeNormal(texture_gbuffer1.SampleLevel(sampler_point_clamp, uv, 0).xy)).xyz;
    const float3 V = normalize(-P);

    const float roughness = GetRoughness(texture_gbuffer0.SampleLevel(sampler_point_clamp, uv, 0).a);
    
    const float roughnessFade = GetRoughnessFade(roughness, SSRMaxRoughness);
    if (roughnessFade <= 0.0f)
    {
        return;
    }
    
    float4 H;
    if (roughness > 0.1f)
    {
        const float surfaceMargin = 0.0f;
        const float maxRegenCount = 15.0f;

        uint2 Random = Rand_PCG16(int3((DTid.xy + 0.5f), g_xFrame_FrameCount)).xy;
        
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
    
    float3 dir = reflect(-V, H.xyz);
    
    float2 hitPixel = float2(0.0f, 0.0f);
    float3 hitPoint = float3(0.0f, 0.0f, 0.0f);  
    float  iterationCount = 0.0f;

    float2 uv2 = (DTid.xy + 0.5f);
    //float jitter = 1.0f + rand(uv2 + g_xFrame_Time);
    float jitter = 1.0f + InterleavedGradientNoise(uv2, g_xFrame_FrameCount);
    
    bool hit = ScreenSpaceRayTrace(P, dir, jitter, roughness, hitPixel, hitPoint, iterationCount);

    float hitDepth = texture_depth.SampleLevel(sampler_point_clamp, hitPixel, 0);

    // Output:
    // xy: hit pixel
    // z:  hit depth
    // w:  pdf
    float4 raytrace = max(0, float4(hitPixel, hitDepth, H.w));
    texture_raytrace[DTid.xy] = raytrace;
    
    // Output:
    // x: hit (bool)
    // y: iteration count / rayTraceMaxStep
    float2 mask = float2(hit, iterationCount / rayTraceMaxStep);
    texture_mask[DTid.xy] = mask;
}
