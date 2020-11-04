#define DISABLE_DECALS
#define DISABLE_ENVMAPS
#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"

TEXTURE2D(texture_color, float4, TEXSLOT_ONDEMAND0);

[earlydepthstencil]
GBUFFEROutputType main(VertexToPixel input)
{
	float4 color = texture_color.Sample(sampler_linear_wrap, input.tex);
	color.rgb = DEGAMMA(color.rgb);
	color.rgb *= input.color;
	float opacity = 1;
	float3 V = g_xCamera_CamPos - input.pos3D;
	float dist = length(V);
	V /= dist;
	float emissive = 0;
	Surface surface = CreateSurface(input.pos3D, input.nor, V, color, 1, 1, 0, 0);
	Lighting lighting = CreateLighting(0, 0, GetAmbient(surface.N), 0);
	surface.pixel = input.pos.xy;
	float depth = input.pos.z;
	float3 reflection = 0;

	float2 ScreenCoord = surface.pixel * g_xFrame_InternalResolution_rcp;
	float2 pos2D = ScreenCoord * 2 - 1;
	pos2D.y *= -1;
	float2 velocity = ((input.pos2DPrev.xy / input.pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (pos2D.xy - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);

	TiledLighting(surface, lighting);

	return CreateGbuffer(surface, velocity, lighting);
}
