#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

void main(VertexToPixel input, out float exponential_shadow : SV_Target0)
{
	// Distance dithered fade:
	clip(dither(input.pos.xy) - input.fade);
	
	ShaderMaterial material = HairGetMaterial();

	[branch]
	if (material.textures[BASECOLORMAP].IsValid())
	{
		clip(material.textures[BASECOLORMAP].Sample(sampler_linear_clamp, input.tex.xyxy).a - material.GetAlphaTest());
	}
	
	exponential_shadow = exp(exponential_shadow_constant * input.pos.z);
}
