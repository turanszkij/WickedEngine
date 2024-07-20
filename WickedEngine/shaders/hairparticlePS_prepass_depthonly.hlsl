#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

void main(VertexToPixel input, out uint coverage : SV_Coverage)
{
	ShaderMaterial material = HairGetMaterial();

	float alpha = 1;

	[branch]
	if (material.textures[BASECOLORMAP].IsValid())
	{
		alpha = material.textures[BASECOLORMAP].Sample(sampler_linear_clamp, input.tex.xyxy).a;
	}

	// Distance dithered fade:
	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.fade);

	coverage = AlphaToCoverage(alpha, material.GetAlphaTest(), input.pos);
}
