#ifndef _VOXEL_CONERACING_HF_
#define _VOXEL_CONERACING_HF_
#include "globals.hlsli"

static const float3 CONES[] = {
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

inline float4 ConeTraceRadiance(in Texture3D<float4> voxels, in float3 uvw, in float3 N)
{
	float4 radiance = 0;

	for (uint cone = 0; cone < g_xWorld_VoxelRadianceConeTracingQuality; ++cone) // quality is between 1 and 16 cones
	{
		// try to approximate a hemisphere from random points inside a sphere:
		float3 coneVec = CONES[cone];
		// if point on sphere is facing below normal (so it's located on bottom hemisphere), put it on the opposite hemisphere instead:
		coneVec *= dot(coneVec, N) < 0 ? -1 : 1;
		coneVec *= g_xWorld_VoxelRadianceDataRes_Inverse * float3(1, -1, 1);

		float3 color = 0;
		float alpha = 0;

		float3 tc = uvw;
		uint i = 0;
		while (alpha < 1 && !any(tc - saturate(tc)))
		{
			float mip = 0.8 * i;

			tc += coneVec * (1 + mip);

			float4 sam = voxels.SampleLevel(sampler_linear_clamp, tc, mip);

			float a = 1 - alpha;
			color += a * sam.rgb;
			alpha += a * sam.a;

			if (mip >= (float)g_xWorld_VoxelRadianceDataMIPs)
				break;

			++i;
		}

		radiance += float4(color, alpha);
	}

	// final radiance is average of all the cones radiances
	radiance *= g_xWorld_VoxelRadianceConeTracingQuality_Inverse;
	radiance.a = saturate(radiance.a);

	return max(0, radiance);
}

inline float4 ConeTraceReflection(in Texture3D<float4> voxels, in float3 uvw, in float3 N, in float3 V, in float roughness)
{
	float aperture = pow(roughness, 4);
	float3 coneVec = reflect(-V, N) / g_xWorld_VoxelRadianceDataRes * float3(1, -1, 1);
	float3 tc = uvw + coneVec;
	float NdotV = saturate(dot(N, V));
	coneVec *= lerp(0.25, 4, pow(1 - NdotV, 8));

	float3 color = 0;
	float alpha = 0;

	// TODO: this can result in huge loops if ray enters empty space. 
	//  Simply limiting iteration count is not good enough solution because most reflections won't be found that way.
	//  Need to have at least some form of binary search. Maybe we could reuse lower mip to find where to backtrace?
	uint i = 0;
	while (alpha < 1 && !any(tc - saturate(tc)))
	{
		float mip = aperture * i;

		tc += coneVec * (1 + mip);

		float4 sam = voxels.SampleLevel(sampler_linear_clamp, tc, mip);

		float a = 1 - alpha;
		color += a * sam.rgb;
		alpha += a * sam.a;

		if (mip >= (float)g_xWorld_VoxelRadianceDataMIPs)
		{
			break;
		}

		++i;
	}

	return float4(max(0, color), saturate(alpha));
}

#endif