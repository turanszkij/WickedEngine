#include "globals.hlsli"
#include "impostorHF.hlsli"
#include "objectHF.hlsli"

[earlydepthstencil]
GBUFFEROutputType main(VSOut input)
{
	float3 uv_col = input.tex;
	float3 uv_nor = uv_col;
	uv_nor.z += impostorCaptureAngles;
	float3 uv_sur = uv_nor;
	uv_sur.z += impostorCaptureAngles;

	float4 color = impostorTex.Sample(sampler_linear_clamp, uv_col) * unpack_rgba(input.instanceColor);
	float3 N = impostorTex.Sample(sampler_linear_clamp, uv_nor).rgb * 2 - 1;
	float4 surfaceparams = impostorTex.Sample(sampler_linear_clamp, uv_sur);

	float ao = surfaceparams.r;
	float roughness = surfaceparams.g;
	float metalness = surfaceparams.b;
	float reflectance = surfaceparams.a;

	float3 V = g_xCamera_CamPos - input.pos3D;
	float dist = length(V);
	V /= dist;
	Surface surface = CreateSurface(input.pos3D, N, V, color, roughness, ao, metalness, reflectance);
	Lighting lighting = CreateLighting(0, 0, GetAmbient(surface.N), 0);
	surface.pixel = input.pos.xy;

	float2 ScreenCoord = surface.pixel * g_xFrame_InternalResolution_rcp;
	float2 pos2D = ScreenCoord * 2 - 1;
	pos2D.y *= -1;
	float2 velocity = ((input.pos2DPrev.xy / input.pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (pos2D.xy - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);

	TiledLighting(surface, lighting);

	return CreateGbuffer(surface, velocity, lighting);
}