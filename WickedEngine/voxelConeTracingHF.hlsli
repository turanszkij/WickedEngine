#ifndef _VOXEL_CONERACING_HF_
#define _VOXEL_CONERACING_HF_
#include "globals.hlsli"

static const float3 CONES[] = {
	float3(0.355512, -0.709318, -0.102371),
	float3(0.534186, 0.71511, -0.115167),
	float3(-0.87866, 0.157139, -0.115167),
	float3(0.140679, -0.475516, -0.0639818),
	float3(-0.0796121, 0.158842, -0.677075),
	float3(-0.0759516, -0.101676, -0.483625),
	float3(0.12493, -0.0223423, -0.483625),
	float3(-0.0720074, 0.243395, -0.967251),
	float3(-0.207641, 0.414286, 0.187755),
	float3(-0.277332, -0.371262, 0.187755),
	float3(0.63864, -0.114214, 0.262857),
	float3(-0.184051, 0.622119, 0.262857),
	float3(0.110007, -0.219486, 0.435574),
	float3(0.235085, 0.314707, 0.696918),
	float3(-0.290012, 0.0518654, 0.522688),
	float3(0.0975089, -0.329594, 0.609803)
};

inline float4 ConeTraceRadiance(in Texture3D<float4> voxels, in float3 uvw, in float3 N)
{
	const uint numCones = clamp(g_xWorld_VoxelRadianceConeTracingQuality, 1, 16);

	// Cone tracing:
	float4 radiance = 0;
	for (uint cone = 0; cone < numCones; ++cone)
	{
		// try to approximate a hemisphere from random points inside a sphere:
		float3 coneVec = CONES[cone];
		// if point on sphere is facing below normal (so it's located on bottom hemisphere), put it on the opposite hemisphere instead:
		coneVec *= dot(coneVec, N) < 0 ? -1 : 1;
		// try to avoid cone extending below horizon by offsetting a bit by normal:
		coneVec += N * 0.1f;
		coneVec *= g_xWorld_VoxelRadianceDataRes_Inverse * float3(1, -1, 1);

		float4 accumulation = 0;
		float3 tc = uvw;
		uint i = 0;
		while (accumulation.a < 1 && !any(tc - saturate(tc)))
		{
			float mip = 0.7 * i;

			tc += coneVec * (1 + mip);

			float4 sam = voxels.SampleLevel(sampler_linear_clamp, tc, mip);
			accumulation.a += sam.a;
			accumulation.rgb += sam.rgb * sam.a;

			if (mip >= (float)g_xWorld_VoxelRadianceDataMIPs)
				break;

			++i;
		}
		float searchDist = length(uvw - tc) * g_xWorld_VoxelRadianceDataRes * g_xWorld_VoxelRadianceFalloff;
		accumulation.a /= searchDist;
		radiance += accumulation;
	}
	radiance /= numCones;
	radiance.a = saturate(radiance.a);

	return max(0, radiance);
}

inline float4 ConeTraceReflection(in Texture3D<float4> voxels, in float3 uvw, in float3 N, in float3 V, in float roughness)
{
	float aperture = pow(roughness, 4);
	float3 coneVec = reflect(-V, N) / g_xWorld_VoxelRadianceDataRes * float3(1, -1, 1);
	float4 accumulation = 0;
	float3 tc = uvw + coneVec;
	float NdotV = saturate(dot(N, V));
	coneVec *= lerp(0.25, 4, pow(1 - NdotV, 8));

	// TODO: this can result in huge loops if ray enters empty space. 
	//  Simply limiting iteration count is not good enough solution because most reflections won't be found that way.
	//  Need to have at least some form of binary search. Maybe we could reuse lower mip to find where to backtrace?
	uint i = 0;
	while (accumulation.a < 1 && !any(tc - saturate(tc)))
	{
		float mip = aperture * i;

		tc += coneVec * (1 + mip);

		float4 sam = voxels.SampleLevel(sampler_linear_clamp, tc, mip);
		accumulation.a += sam.a;
		accumulation.rgb += sam.rgb * sam.a;

		if (mip >= (float)g_xWorld_VoxelRadianceDataMIPs)
		{
			break;
		}

		++i;
	}

	return float4(max(0, accumulation.rgb), saturate(accumulation.a));
}

#endif