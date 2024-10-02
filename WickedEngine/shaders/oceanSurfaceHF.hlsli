#ifndef WI_OCEAN_SURFACE_HF
#define WI_OCEAN_SURFACE_HF
#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"

struct PSIn
{
	float4 pos		: SV_POSITION;
	float2 uv		: TEXCOORD0;
	
	inline float3 GetPos3D()
	{
		ShaderCamera camera = GetCamera();
		const float2 ScreenCoord = pos.xy * camera.internal_resolution_rcp; // use pixel center!
		const float z = camera.IsOrtho() ? (1 - pos.z) : inverse_lerp(camera.z_near, camera.z_far, pos.w);
		return camera.frustum_corners.position_from_screen(ScreenCoord, z);
	}

	inline float3 GetViewVector()
	{
		ShaderCamera camera = GetCamera();
		const float2 ScreenCoord = pos.xy * camera.internal_resolution_rcp; // use pixel center!
		return camera.frustum_corners.position_near(ScreenCoord) - GetPos3D(); // ortho support, cannot use cameraPos!
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
