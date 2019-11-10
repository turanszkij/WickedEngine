#include "globals.hlsli"
#include "hairparticleHF.hlsli"

void main(VertexToPixel input)
{
	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.fade);

	float4 color = texture_0.Sample(sampler_linear_clamp,input.tex);
	ALPHATEST(color.a)
}