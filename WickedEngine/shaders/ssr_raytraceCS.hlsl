#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

//#define DEBUG_TILING

Texture2D<float2> texture_depth_hierarchy : register(t0);
Texture2D<float4> input : register(t1);

#if defined(SSR_EARLYEXIT)
StructuredBuffer<uint> tiles : register(t2);
#elif defined(SSR_CHEAP)
StructuredBuffer<uint> tiles : register(t3);
#else
StructuredBuffer<uint> tiles : register(t4);
#endif // SSR_EARLYEXIT

RWTexture2D<float4> output_rayIndirectSpecular : register(u0);
RWTexture2D<float4> output_rayDirectionPDF : register(u1);
RWTexture2D<float> output_rayLengths : register(u2);

static const float traceThickness = 1.5;
static const float blendScreenEdgeFade = 5.0f;

static const float HiZTraceMostDetailedLevel = 0.0;
static const float HiZTraceIterationsMax = 64;

void InitialAdvanceRay(float3 origin, float3 direction, float2 currentMipResolution, float2 currentMipResolution_rcp, float2 floorOffset, float2 uvOffset, out float3 position, out float tCurrent)
{
	float2 currentMipPosition = currentMipResolution * origin.xy;

	// Intersect ray with the half box that is pointing away from the ray origin.
	float2 xyPlane = floor(currentMipPosition) + floorOffset;
	xyPlane = xyPlane * currentMipResolution_rcp + uvOffset;

	// o + d * t = p' => t = (p' - o) / d
	float2 t = (xyPlane - origin.xy) / direction.xy;
	tCurrent = min(t.x, t.y);
	position = origin + tCurrent * direction;
}

bool AdvanceRay(float3 origin, float3 direction, float2 currentMipPosition, float2 currentMipResolution_rcp, float2 floorOffset, float2 uvOffset, float surfaceZ, inout float3 position, inout float tCurrent)
{
	// Create boundary planes
	float2 xyPlane = floor(currentMipPosition) + floorOffset;
	xyPlane = xyPlane * currentMipResolution_rcp + uvOffset;
	float3 boundaryPlanes = float3(xyPlane, surfaceZ);

	// Intersect ray with the half box that is pointing away from the ray origin.
	// o + d * t = p' => t = (p' - o) / d
	float3 t = (boundaryPlanes - origin) / direction;

	// Prevent using z plane when shooting out of the depth buffer.
	t.z = direction.z < 0 ? t.z : FLT_MAX;

	// Choose nearest intersection with a boundary.
	float tMin = min(min(t.x, t.y), t.z);

	// Larger z means closer to the camera.
	bool aboveSurface = surfaceZ < position.z;

	// Decide whether we are able to advance the ray until we hit the xy boundaries or if we had to clamp it at the surface.
	// We use the asuint comparison to avoid NaN / Inf logic, also we actually care about bitwise equality here to see if t_min is the t.z we fed into the min3 above.
	bool skippedTile = asuint(tMin) != asuint(t.z) && aboveSurface;

	// Make sure to only advance the ray if we're still above the surface.
	tCurrent = aboveSurface ? tMin : tCurrent;

	// Advance ray
	position = origin + tCurrent * direction;

	return skippedTile;
}

float2 GetMipResolution(float2 screenDimensions, int mipLevel)
{
	return screenDimensions * pow(0.5, mipLevel);
}

// Based on: https://github.com/GPUOpen-Effects/FidelityFX-SSSR/tree/master
// Requires origin and direction of the ray to be in screen space [0, 1] x [0, 1]
float3 HierarchicalRaymarch(float3 origin, float3 direction, float2 screenSize, out bool validHit)
{
	// Start on mip with highest detail.
	int currentMip = HiZTraceMostDetailedLevel;

	// Could recompute these every iteration, but it's faster to hoist them out and update them.
	float2 currentMipResolution = GetMipResolution(screenSize, currentMip);
	float2 currentMipResolution_rcp = rcp(currentMipResolution);

	// Offset to the bounding boxes uv space to intersect the ray with the center of the next pixel.
	// This means we ever so slightly over shoot into the next region. 
	float2 uvOffset = 0.005 * exp2(HiZTraceMostDetailedLevel) / screenSize;
	uvOffset = select(direction.xy < 0, -uvOffset, uvOffset);

	// Offset applied depending on current mip resolution to move the boundary to the left/right upper/lower border depending on ray direction.
	float2 floorOffset = select(direction.xy < 0, 0, 1);

	// Initially advance ray to avoid immediate self intersections.
	float tCurrent;
	float3 position;
	InitialAdvanceRay(origin, direction, currentMipResolution, currentMipResolution_rcp, floorOffset, uvOffset, position, tCurrent);

	int i = 0;
	while (i < HiZTraceIterationsMax && currentMip >= HiZTraceMostDetailedLevel)
	{
		if (any(position.xy < 0.0) || any(position.xy > 1.0))
		{
			validHit = false;
			return position;
		}

		float2 currentMipPosition = currentMipResolution * position.xy;
		float surfaceZ = texture_depth_hierarchy.Load(int3(currentMipPosition, currentMip)).r;

		bool skippedTile = AdvanceRay(origin, direction, currentMipPosition, currentMipResolution_rcp, floorOffset, uvOffset, surfaceZ, position, tCurrent);

		currentMip += skippedTile ? 1 : -1;
		currentMipResolution *= skippedTile ? 0.5 : 2;
		currentMipResolution_rcp *= skippedTile ? 2 : 0.5;

		i++;
	}

	validHit = (i <= HiZTraceIterationsMax);

	return position;
}

static const uint rayMarchIterationsMax = 60; // primary ray march step count (higher will find more in distance, but slower)
static const float rayMarchStepIncrease = 1.05f; // primary ray march step increase (higher will travel more distance, but can miss details)
static const uint rayMarchFineIterationsMax = 2; // binary step count (higher is nicer but slower)
static const float rayMarchTolerance = 0.000002; // early exit factor for binary search (smaller is nicer but slower)
static const float rayMarchLevelIncrement = 0.3; // level increment based on ray travel distance and roughness (higher values improves performance, but traces at lower resolution)

// samplePos where ray march left of
float3 BinarySearch(float3 samplePos, float3 V, float level)
{
	for (uint i = 0; i < rayMarchFineIterationsMax; i++)
	{
		float sampleDepth = texture_depth_hierarchy.SampleLevel(sampler_point_clamp, samplePos.xy, level).g;

		if (abs(samplePos.z - sampleDepth) < rayMarchTolerance)
		{
			return samplePos;
		}

		if (samplePos.z >= sampleDepth)
		{
			samplePos += V;
		}

		V *= 0.5f;
		samplePos -= V;
	}

	return samplePos;
}

// P and V in screen space [0, 1] x [0, 1]
float3 RayMarch(float3 P, float3 V, float roughness, float jitter, out bool validHit)
{
	float3 samplePos = P + V * jitter;

	float sampleDepth = 0;
	float level = 1;

	uint iterations = 0;
	while (iterations <= rayMarchIterationsMax)
	{
		if (any(samplePos.xy < 0.0) || any(samplePos.xy > 1.0))
		{
			validHit = false;
			return samplePos;
		}

		samplePos += V;

		sampleDepth = texture_depth_hierarchy.SampleLevel(sampler_point_clamp, samplePos.xy, level).g;

		if (sampleDepth > samplePos.z)
		{
			samplePos = BinarySearch(samplePos, V, level);
			break;
		}

		V *= rayMarchStepIncrease;
		level += rayMarchLevelIncrement * roughness;

		iterations++;
	}

	validHit = (iterations <= rayMarchIterationsMax);
	return float3(samplePos.xy, sampleDepth);
}

float CalculateEdgeVignette(float2 hitPixel)
{
	float2 hitPixelNDC = hitPixel * 2.0 - 1.0;

	//float maxDimension = min(1.0, max(abs(hitPixelNDC.x), abs(hitPixelNDC.y)));
	//float attenuation = 1.0 - max(0.0, maxDimension - blendScreenEdgeFade) / (1.0 - blendScreenEdgeFade);

	float2 vignette = saturate(abs(hitPixelNDC) * blendScreenEdgeFade - (blendScreenEdgeFade - 1.0f));
	float attenuation = saturate(1.0 - dot(vignette, vignette));

	return attenuation;
}

float ValidateHit(float3 hit, float hitDepth, float2 prevHitUV)
{
	float vignetteHit = CalculateEdgeVignette(hit.xy);
	float vignetteHitPrev = CalculateEdgeVignette(prevHitUV);
	float vignette = min(vignetteHit, vignetteHitPrev);

	float3 surfaceViewPosition = reconstruct_position(hit.xy, hitDepth, GetCamera().inverse_projection);
	float3 hitViewPosition = reconstruct_position(hit.xy, hit.z, GetCamera().inverse_projection);

	float distance = length(surfaceViewPosition - hitViewPosition);
	float confidence = 1.0 - smoothstep(0.0, traceThickness, distance);

	return vignette * confidence;
}

[numthreads(POSTPROCESS_BLOCKSIZE * POSTPROCESS_BLOCKSIZE, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	// This pass is rendered at half-res
	const uint downsampleFactor = 2;

	const uint2 pixel = GetReflectionIndirectDispatchCoord(Gid, GTid, tiles, downsampleFactor);
	const float2 uv = (pixel + 0.5f) * postprocess.resolution_rcp;

#ifdef SSR_EARLYEXIT

	output_rayIndirectSpecular[pixel] = 0;
	output_rayDirectionPDF[pixel] = 0;
	output_rayLengths[pixel] = 0;

#else

	// This is necessary for accurate upscaling. This is so we don't reuse the same half-res pixels
	uint2 screenJitter = floor(blue_noise(uint2(0, 0)).xy * downsampleFactor);
	uint2 jitterPixel = screenJitter + pixel * downsampleFactor;
	float2 jitterUV = (screenJitter + pixel + 0.5f) * postprocess.resolution_rcp;

	// Due to HiZ tracing, the tracing and the pass components must match depth.
	float depth = texture_depth_hierarchy[screenJitter + pixel].r;
	float roughness = texture_roughness[jitterPixel];

	if (!NeedReflection(roughness, depth, ssr_roughness_cutoff))
	{
		output_rayIndirectSpecular[pixel] = 0.0;
		output_rayDirectionPDF[pixel] = 0.0;
		output_rayLengths[pixel] = 0.0;
		return;
	}

	float3 N = decode_oct(texture_normal[jitterPixel]);
	float3 P = reconstruct_position(jitterUV, depth);
	float3 V = normalize(GetCamera().position - P);

	float4 H;
	float3 L;
	if (roughness > 0.05f)
	{
		float3x3 tangentBasis = GetTangentBasis(N);
		float3 tangentV = mul(tangentBasis, V);

#ifdef GGX_SAMPLE_VISIBLE

		const float2 bluenoise = blue_noise(pixel).xy;

		float2 Xi = bluenoise.xy;

		Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);

		H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);

		// Tangent to world
		H.xyz = mul(H.xyz, tangentBasis);

#else

		const float surfaceMargin = 0.0f;
		const float maxRegenCount = 15.0f;

		// By using an uniform importance sampling method, some rays go below the surface.
		// We simply re-generate them at a negligible cost, to get some nice ones.

		float RdotN = 0.0f;
		float regenCount = 0;
		[loop]
		for (; RdotN <= surfaceMargin && regenCount < maxRegenCount; regenCount++)
		{
			// Low-discrepancy sequence
			const float2 bluenoise = blue_noise(pixel, regenCount).xy;

			float2 Xi = bluenoise.xy;

			Xi.y = lerp(Xi.y, 0.0, GGX_IMPORTANCE_SAMPLE_BIAS);

			H = ImportanceSampleGGX(Xi, roughness);

			// Tangent to world
			H.xyz = mul(H.xyz, tangentBasis);

			RdotN = dot(N, reflect(-V, H.xyz));
		}

#endif // GGX_SAMPLE_VISIBLE

		L = reflect(-V, H.xyz);
	}
	else
	{
		H = float4(N.xyz, 1.0f);
		L = reflect(-V, H.xyz);
	}

	float4 rayStartClip = mul(GetCamera().view_projection, float4(P, 1)); // World to Clip
	float4 rayEndClip = mul(GetCamera().view_projection, float4(P + L, 1));

	float3 rayStartScreen = rayStartClip.xyz * rcp(rayStartClip.w);
	float3 rayEndScreen = rayEndClip.xyz * rcp(rayEndClip.w);

	rayStartScreen.xy = rayStartScreen.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
	rayEndScreen.xy = rayEndScreen.xy * float2(0.5, -0.5) + float2(0.5, 0.5);

#ifdef SSR_CHEAP

	rayStartScreen.xy *= postprocess.params1.xy; // Ratio factor between hierarchy and pass
	rayEndScreen.xy *= postprocess.params1.xy;

	float3 rayDirectionScreen = rayEndScreen - rayStartScreen;

	// The ray marching benefits from jittering to create a smoother transition between samples and LOD
	float jitter = InterleavedGradientNoise(pixel, GetFrame().frame_count % 8u); // Temporally stabilize

	bool validHit = false;
	float3 hit = RayMarch(rayStartScreen, rayDirectionScreen, roughness, jitter, validHit);

	hit.xy *= postprocess.params1.zw; // Undo ratio

#else

	float3 rayDirectionScreen = rayEndScreen - rayStartScreen;

	bool validHit = false;
	float3 hit = HierarchicalRaymarch(rayStartScreen, rayDirectionScreen, postprocess.resolution, validHit);

#endif // SSR_CHEAP

	float2 prevHitUV = texture_velocity.SampleLevel(sampler_point_clamp, hit.xy, 0).xy + hit.xy;

	float hitDepth = texture_depth.SampleLevel(sampler_point_clamp, hit.xy, 0);
	float confidence = validHit ? ValidateHit(hit, hitDepth, prevHitUV) : 0;

	float4 indirectSpecular;
	indirectSpecular.rgb = confidence > 0 ? input.SampleLevel(sampler_point_clamp, prevHitUV, 0).rgb : 0;
	indirectSpecular.a = confidence;

	output_rayIndirectSpecular[pixel] = indirectSpecular;

	output_rayDirectionPDF[pixel] = float4(L, H.w);

	if (validHit)
	{
		const float3 Phit = reconstruct_position(jitterUV, hit.z, GetCamera().inverse_projection);
		output_rayLengths[pixel] = distance(P, Phit);
	}
	else
	{
		output_rayLengths[pixel] = 0;
	}

#endif // SSR_EARLYEXIT

#ifdef DEBUG_TILING
	float3 color = input[pixel].rgb;
#if defined(SSR_EARLYEXIT)
	output_rayIndirectSpecular[pixel] = float4(lerp(color, float3(0, 0, 1), 0.5f), 1.0);
#elif defined(SSR_CHEAP)
	output_rayIndirectSpecular[pixel] = float4(lerp(color, float3(0, 1, 0), 0.5f), 1.0);
#else
	output_rayIndirectSpecular[pixel] = float4(lerp(color, float3(1, 0, 0), 0.5f), 1.0);
#endif // SSR_EARLYEXIT
#endif // DEBUG_TILING
}
