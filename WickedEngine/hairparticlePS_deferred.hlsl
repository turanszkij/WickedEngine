#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ditherHF.hlsli"

GBUFFEROutputType main(VertexToPixel input)
{
#ifdef GRASS_FADE_DITHER
	clip(dither(input.pos.xy) - input.fade);
#endif

	float4 color = texture_0.Sample(sampler_linear_clamp, input.tex);
	color.rgb *= input.color;
	ALPHATEST(color.a)
	color = DEGAMMA(color);
	float emissive = 0;
	Surface surface = CreateSurface(0, input.nor, 0, color, 1, 0, 0);
	float ao = 1;
	float sss = 0;
	float2 velocity = ((input.pos2DPrev.xy / input.pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (input.pos2D.xy / input.pos2D.w - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);

	return CreateGbuffer(color, surface, velocity, 0, 0, 1);
}