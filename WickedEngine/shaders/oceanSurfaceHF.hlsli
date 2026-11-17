#ifndef WI_OCEAN_SURFACE_HF
#define WI_OCEAN_SURFACE_HF
#include "oceanHF.hlsli"
#include "ShaderInterop_Ocean.h"

//static const float OCEAN_NEARPLANE_CUTOFF = 0.1;
#define OCEAN_NEARPLANE_CUTOFF compute_inverse_lineardepth(max(GetCamera().z_near + 1, 1.0))

struct PSIn
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
	min16uint cameraIndex : CAMERAINDEX;
	float3 world_position : WORLDPOSITION;
	
	inline float3 GetPos3D()
	{
		// Actual displaced surface, including in shadow atlas tiles with a viewport offset.
		return world_position;
	}

	inline float3 GetViewVector()
	{
		return GetCameraIndexed(cameraIndex).screen_to_view(pos);
	}
};

float intersectPlaneClampInfiniteDist(in float3 rayOrigin, in float3 rayDirection, in float3 planeNormal, float planeHeight)
{
	return (planeHeight - dot(planeNormal, rayOrigin)) / dot(planeNormal, rayDirection);
}
float3 intersectPlaneClampInfinite(in float3 rayOrigin, in float3 rayDirection, in float3 planeNormal, float planeHeight)
{
	float dist = intersectPlaneClampInfiniteDist(rayOrigin, rayDirection, planeNormal, planeHeight);
	if (dist > 0.0)
		return rayOrigin + rayDirection * dist;
	else
		return float3(rayOrigin.x, planeHeight, rayOrigin.z) + normalize(float3(rayDirection.x, 0, rayDirection.z)) * GetCamera().z_far;
}

// Volumetric lights use opaque depth, which doesn't contain the ocean surface.
// Trace the approximate surface, but never stop a ray at a cut-out.
inline void ocean_clip_volumetric_ray(float3 O, float3 D, inout float3 P, inout float distance_to_opaque)
{
	const ShaderOcean ocean = GetWeather().ocean;
	if (!ocean.IsValid() || D.y == 0)
	{
		return;
	}
	float t = (ocean.water_height - O.y) / D.y;
	const bool has_exclusions = ocean_has_exclusions(ocean);
	if (has_exclusions)
	{
		const float3 approx_hit = O + D * clamp(t, 0, distance_to_opaque);
		const float wave_height = ocean_height_at(ocean, approx_hit.xz) - ocean.water_height;
		const float wave_fade = smoothstep(0.08, 0.8, saturate(length(approx_hit - O) * 0.005));
		t += wave_height * (1 - wave_fade) / D.y;
	}
	if (t > 0 && t < distance_to_opaque)
	{
		const float3 hit = O + D * t;
		if (!has_exclusions || !ocean_exclusion_check(hit))
		{
			P = hit;
			distance_to_opaque = t;
		}
	}
}

#endif // WI_OCEAN_SURFACE_HF
