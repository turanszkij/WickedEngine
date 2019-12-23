#ifndef WI_VOXEL_CONERACING_HF
#define WI_VOXEL_CONERACING_HF
#include "globals.hlsli"

static const float3 CONES[] = 
{
	float3(0.57735, 0.57735, 0.57735),
	float3(0.57735, -0.57735, -0.57735),
	float3(-0.57735, 0.57735, -0.57735),
	float3(-0.57735, -0.57735, 0.57735),
	float3(-0.903007, -0.182696, -0.388844),
	float3(-0.903007, 0.182696, 0.388844),
	float3(0.903007, -0.182696, 0.388844),
	float3(0.903007, 0.182696, -0.388844),
	float3(-0.388844, -0.903007, -0.182696),
	float3(0.388844, -0.903007, 0.182696),
	float3(0.388844, 0.903007, -0.182696),
	float3(-0.388844, 0.903007, 0.182696),
	float3(-0.182696, -0.388844, -0.903007),
	float3(0.182696, 0.388844, -0.903007),
	float3(-0.182696, 0.388844, 0.903007),
	float3(0.182696, -0.388844, 0.903007)
};

// voxels:			3D Texture containing voxel scene with direct diffuse lighting (or direct + secondary indirect bounce)
// P:				world-space position of receiving surface
// N:				world-space normal vector of receiving surface
// coneDirection:	world-space cone direction in the direction to perform the trace
// coneAperture:	tan(coneHalfAngle)
inline float4 ConeTrace(in Texture3D<float4> voxels, in float3 P, in float3 N, in float3 coneDirection, in float coneAperture)
{
	float3 color = 0;
	float alpha = 0;
	
	// We need to offset the cone start position to avoid sampling its own voxel (self-occlusion):
	//	Unfortunately, it will result in disconnection between nearby surfaces :(
	float dist = g_xFrame_VoxelRadianceDataSize; // offset by cone dir so that first sample of all cones are not the same
	float3 startPos = P + N * g_xFrame_VoxelRadianceDataSize * 2 * SQRT2; // sqrt2 is diagonal voxel half-extent

	// We will break off the loop if the sampling distance is too far for performance reasons:
	while (dist < g_xFrame_VoxelRadianceMaxDistance && alpha < 1)
	{
		float diameter = max(g_xFrame_VoxelRadianceDataSize, 2 * coneAperture * dist);
		float mip = log2(diameter * g_xFrame_VoxelRadianceDataSize_rcp);

		// Because we do the ray-marching in world space, we need to remap into 3d texture space before sampling:
		//	todo: optimization could be doing ray-marching in texture space
		float3 tc = startPos + coneDirection * dist;
		tc = (tc - g_xFrame_VoxelRadianceDataCenter) * g_xFrame_VoxelRadianceDataSize_rcp;
		tc *= g_xFrame_VoxelRadianceDataRes_rcp;
		tc = tc * float3(0.5f, -0.5f, 0.5f) + 0.5f;

		// break if the ray exits the voxel grid, or we sample from the last mip:
		if (!is_saturated(tc) || mip >= (float)g_xFrame_VoxelRadianceDataMIPs)
			break;

		float4 sam = voxels.SampleLevel(sampler_linear_clamp, tc, mip);

		// this is the correct blending to avoid black-staircase artifact (ray stepped front-to back, so blend front to back):
		float a = 1 - alpha;
		color += a * sam.rgb;
		alpha += a * sam.a;

		// step along ray:
		dist += diameter * g_xFrame_VoxelRadianceRayStepSize;
	}

	return float4(color, alpha);
}

// voxels:			3D Texture containing voxel scene with direct diffuse lighting (or direct + secondary indirect bounce)
// P:				world-space position of receiving surface
// N:				world-space normal vector of receiving surface
inline float4 ConeTraceRadiance(in Texture3D<float4> voxels, in float3 P, in float3 N)
{
	float4 radiance = 0;

	for (uint cone = 0; cone < g_xFrame_VoxelRadianceNumCones; ++cone) // quality is between 1 and 16 cones
	{
		// approximate a hemisphere from random points inside a sphere:
		//  (and modulate cone with surface normal, no banding this way)
		float3 coneDirection = normalize(CONES[cone] + N);
		// if point on sphere is facing below normal (so it's located on bottom hemisphere), put it on the opposite hemisphere instead:
		coneDirection *= dot(coneDirection, N) < 0 ? -1 : 1;

		radiance += ConeTrace(voxels, P, N, coneDirection, tan(PI * 0.5f * 0.33f));
	}

	// final radiance is average of all the cones radiances
	radiance *= g_xFrame_VoxelRadianceNumCones_rcp;
	radiance.a = saturate(radiance.a);

	return max(0, radiance);
}

// voxels:			3D Texture containing voxel scene with direct diffuse lighting (or direct + secondary indirect bounce)
// P:				world-space position of receiving surface
// N:				world-space normal vector of receiving surface
// V:				world-space view-vector (cameraPosition - P)
inline float4 ConeTraceReflection(in Texture3D<float4> voxels, in float3 P, in float3 N, in float3 V, in float roughness)
{
	float aperture = tan(roughness * PI * 0.5f * 0.1f);
	float3 coneDirection = reflect(-V, N);

	float4 reflection = ConeTrace(voxels, P, N, coneDirection, aperture);

	return float4(max(0, reflection.rgb), saturate(reflection.a));
}

#endif