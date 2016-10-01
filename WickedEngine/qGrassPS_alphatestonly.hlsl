#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"

float4 main(QGS_OUT PSIn) : SV_Target
{
#ifdef GRASS_FADE_DITHER
	clip(dither(PSIn.pos.xy) - PSIn.fade);
#endif

	float4 color = texture_0.Sample(sampler_linear_clamp,PSIn.tex);
	ALPHATEST(color.a)

	return 0;
}