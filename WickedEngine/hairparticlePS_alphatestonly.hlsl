#include "globals.hlsli"
#include "hairparticleHF.hlsli"

void main(VertexToPixel PSIn)
{
#ifdef GRASS_FADE_DITHER
	clip(dither(PSIn.pos.xy) - PSIn.fade);
#endif

	float4 color = texture_0.Sample(sampler_linear_clamp,PSIn.tex);
	ALPHATEST(color.a)
}