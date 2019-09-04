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


// Cloud noise based on: https://www.shadertoy.com/view/4tdSWr
float2 hash(float2 p) 
{
	p = float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)));
	return -1.0 + 2.0*frac(sin(p)*43758.5453123);
}
float noise(in float2 p) 
{
	const float K1 = 0.366025404; // (sqrt(3)-1)/2;
	const float K2 = 0.211324865; // (3-sqrt(3))/6;
	float2 i = floor(p + (p.x + p.y)*K1);
	float2 a = p - i + (i.x + i.y)*K2;
	float2 o = (a.x > a.y) ? float2(1.0, 0.0) : float2(0.0, 1.0); //float2 of = 0.5 + 0.5*float2(sign(a.x-a.y), sign(a.y-a.x));
	float2 b = a - o + K2;
	float2 c = a - 1.0 + 2.0*K2;
	float3 h = max(0.5 - float3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
	float3 n = h * h*h*h*float3(dot(a, hash(i + 0.0)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));
	return dot(n, float3(70.0, 70.0, 70.0));
}

void AddCloudLayer(inout float4 color, in float3 normal, bool dark)
{
	if (g_xFrame_Cloudiness <= 0)
	{
		return;
	}

	// Trace a cloud layer plane:
	const float3 o = g_xCamera_CamPos;
	const float3 d = normal;
	const float3 planeOrigin = float3(0, 1000, 0);
	const float3 planeNormal = float3(0, -1, 0);
	const float t = Trace_plane(o, d, planeOrigin, planeNormal);

	if (t < 0)
	{
		return;
	}
	
	const float3 cloudPos = o + d * t;
	const float2 cloudUV = cloudPos.xz * g_xFrame_CloudScale;
	const float cloudTime = g_xFrame_Time * g_xFrame_CloudSpeed;
	const float2x2 m = float2x2(1.6, 1.2, -1.2, 1.6);
	const uint quality = 8;

	// rotate uvs like a flow effect:
	float q = 0;
	{
		float2 uv = cloudUV * 0.5f;
		float swirling = 0.1;
		for (uint i = 0; i < quality; i++)
		{
			q += noise(uv) * swirling;
			uv = mul(m, uv);
			swirling *= 0.4;
		}
	}

	float weight = 0.8;

	// main noise:
	float r = 0.0;
	{
		const float time = cloudTime * 0.1f;
		float2 uv = cloudUV;
		uv -= q - time;
		for (uint i = 0; i < quality; i++)
		{
			r += abs(weight*noise(uv));
			uv = mul(m, uv) + time;
			weight *= 0.7;
		}
	}

	// secondary noise:
	float f = 0.0;
	{
		const float time = cloudTime * 0.2f;
		float2 uv = cloudUV * 0.8f;
		uv -= q - time;
		weight = 0.7;
		for (uint i = 0; i < quality; i++)
		{
			f += weight * noise(uv);
			uv = mul(m, uv) + time;
			weight *= 0.6;
		}
	}

	f *= r + f;
	f = f * r;

	// lerp between "choppy clouds" and "uniform clouds". Lower cloudiness will produce choppy clouds, but very high cloudiness will switch to overcast unfiform clouds:
	float clouds = lerp(f * 9.0f * g_xFrame_Cloudiness + 0.3f, f * 0.5f + 0.5f, pow(saturate(g_xFrame_Cloudiness), 8));
	clouds = saturate(clouds - (1 - g_xFrame_Cloudiness)); // modulate constant cloudiness
	clouds *= pow(1 - saturate(length(abs(cloudPos.xz * 0.00001f))), 16); //fade close to horizon

	if (dark)
	{
		color.rgb *= pow(saturate(1 - clouds), 16.0f); // only sun and clouds. Boost clouds to have nicer sun shafts occlusion
	}
	else
	{
		color.rgb = lerp(color.rgb, 1, clouds); // sky and clouds on top
	}
}


#endif // _SKY_HF_
