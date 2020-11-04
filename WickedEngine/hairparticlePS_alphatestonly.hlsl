#include "globals.hlsli"
#include "hairparticleHF.hlsli"

TEXTURE2D(texture_color, float4, TEXSLOT_ONDEMAND0);

void main(VertexToPixel input)
{
	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.fade);

	float4 color = texture_color.Sample(sampler_linear_clamp,input.tex);
	ALPHATEST(color.a)
}