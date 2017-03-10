#ifndef _VOXEL_CONERACING_HF_
#define _VOXEL_CONERACING_HF_
#include "globals.hlsli"

static const float3 CONES[] = {
	float3(0,0,0),
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
	uint3 dim;
	uint mips;
	voxels.GetDimensions(0, dim.x, dim.y, dim.z, mips);

	const uint numCones = clamp(g_xWorld_VoxelRadianceConeTracingQuality, 1, 16);

	// Cone tracing:
	float4 radiance = 0;
	for (uint cone = 0; cone < numCones; ++cone)
	{
		float3 coneVec = normalize(N * 2 + CONES[cone]) / g_xWorld_VoxelRadianceDataRes * float3(1, -1, 1);

		float4 accumulation = 0;
		float step = 0;
		float3 tc = uvw;
		for (uint i = 0; i < g_xWorld_VoxelRadianceDataRes; ++i)
		{
			step++;
			float mip = 0.7 * i;

			tc += coneVec * (1 + mip);

			float4 sam = voxels.SampleLevel(sampler_linear_clamp, tc, mip);
			accumulation.a += sam.a;
			accumulation.rgb += sam.rgb * accumulation.a / g_xWorld_VoxelRadianceFalloff;

			if (accumulation.a >= 1.0f || mip >= (float)mips || any(tc - saturate(tc)))
				break;
		}
		radiance += accumulation / step;
	}
	radiance /= numCones;
	radiance.a = saturate(radiance.a);

	return radiance;
}

#endif