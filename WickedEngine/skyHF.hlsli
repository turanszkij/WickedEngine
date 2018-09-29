#ifndef _SKY_HF_
#define _SKY_HF_
#include "globals.hlsli"
#include "lightingHF.hlsli"

float3 GetDynamicSkyColor(in float3 normal)
{
	float aboveHorizon = saturate(pow(saturate(normal.y), 0.25f + g_xFrame_Fog.z) / (g_xFrame_Fog.z + 1));
	float3 sky = lerp(GetHorizonColor(), GetZenithColor(), aboveHorizon);

#ifdef NOSUN
	return sky;

#else

	float3 sunc = GetSunColor();

	float3 sun = normal.y > 0 ? max(saturate(dot(GetSunDirection(), normal) > 0.9998 ? 1 : 0)*sunc * 1000, 0) : 0;
	return sky + sun;
#endif // NOSUN
}

void AddCloudLayer(inout float4 color, in float3 normal, bool dark)
{
	float3 o = g_xCamera_CamPos;
	float3 d = normal;
	float3 planeOrigin = float3(0, 1000, 0);
	float3 planeNormal = float3(0, -1, 0);
	float t = Trace_plane(o, d, planeOrigin, planeNormal);

	if (t < 0)
	{
		return;
	}

	float3 cloudPos = o + d * t;
	float2 cloudUV = planeOrigin.xz - cloudPos.xz;
	cloudUV *= g_xFrame_CloudScale;

	float clouds1 = texture_0.SampleLevel(sampler_linear_mirror, cloudUV, 0).r;
	clouds1 = saturate(clouds1 - (1 - g_xFrame_Cloudiness)) /** pow(saturate(normal.y), 0.5)*/;

	float clouds2 = texture_0.SampleLevel(sampler_linear_clamp, normal.xz * 0.5 + 0.5, 0).g;
	clouds2 *= pow(saturate(normal.y), 0.25);
	clouds2 = saturate(clouds2 - 0.2);

	float clouds = clouds1 * clouds2;

	if (dark)
	{
		color.rgb *= pow(saturate(1 - clouds), 16.0f);
	}
	else
	{
		color.rgb = lerp(color.rgb, 1, clouds);
	}
}


#endif // _SKY_HF_
