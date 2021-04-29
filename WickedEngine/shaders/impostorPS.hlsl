#include "globals.hlsli"
#include "impostorHF.hlsli"
#include "objectHF.hlsli"

[earlydepthstencil]
GBuffer main(VSOut input)
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

	Surface surface;
	surface.init();
	surface.roughness = roughness;
	surface.albedo = lerp(lerp(color.rgb, float3(0, 0, 0), reflectance), float3(0, 0, 0), metalness);
	surface.f0 = lerp(lerp(float3(0, 0, 0), float3(1, 1, 1), reflectance), color.rgb, metalness);
	surface.P = input.pos3D;
	surface.N = N;
	surface.V = V;
	surface.pixel = input.pos.xy;
	surface.update();

	Lighting lighting;
	lighting.create(0, 0, GetAmbient(surface.N), 0);

	float2 ScreenCoord = surface.pixel * g_xFrame_InternalResolution_rcp;
	float2 pos2D = ScreenCoord * 2 - 1;
	pos2D.y *= -1;
	float2 velocity = ((input.pos2DPrev.xy / input.pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (pos2D.xy - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);

	TiledLighting(surface, lighting);

	ApplyLighting(surface, lighting, color);

	ApplyFog(dist, color);

	return CreateGBuffer(color, surface);
}
