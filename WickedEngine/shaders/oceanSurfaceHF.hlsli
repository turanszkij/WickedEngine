#ifndef WI_OCEAN_SURFACE_HF
#define WI_OCEAN_SURFACE_HF
#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"

//static const float OCEAN_NEARPLANE_CUTOFF = 0.1;
#define OCEAN_NEARPLANE_CUTOFF compute_inverse_lineardepth(max(GetCamera().z_near + 1, 1.0))

struct PSIn
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
	min16uint cameraIndex : CAMERAINDEX;
	
	inline float3 GetPos3D()
	{
		return GetCameraIndexed(cameraIndex).screen_to_world(pos);
	}

	inline float3 GetViewVector()
	{
		return GetCameraIndexed(cameraIndex).screen_to_nearplane(pos) - GetPos3D(); // ortho support, cannot use cameraPos!
	}
};

float3 intersectPlaneClampInfinite(in float3 rayOrigin, in float3 rayDirection, in float3 planeNormal, float planeHeight)
{
	float dist = (planeHeight - dot(planeNormal, rayOrigin)) / dot(planeNormal, rayDirection);
	if (dist < 0.0)
		return rayOrigin + rayDirection * dist;
	else
		return float3(rayOrigin.x, planeHeight, rayOrigin.z) - normalize(float3(rayDirection.x, 0, rayDirection.z)) * GetCamera().z_far;
}

#endif // WI_OCEAN_SURFACE_HF
