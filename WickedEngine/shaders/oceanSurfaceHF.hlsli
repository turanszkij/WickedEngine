#ifndef WI_OCEAN_SURFACE_HF
#define WI_OCEAN_SURFACE_HF
#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"

struct PSIn
{
	float4 pos		: SV_POSITION;
	float3 pos3D	: WORLDPOSITION;
	float2 uv		: TEXCOORD0;
	uint behind : BEHIND;
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
