#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

void main(VertexToPixel input)
{
	// Distance dithered fade:
	clip(dither(input.pos.xy) - input.fade);
	
	ShaderMaterial material = HairGetMaterial();

	[branch]
	if (material.textures[BASECOLORMAP].IsValid())
	{
		float bias = 0;
		if (GetCamera().options & SHADERCAMERA_OPTION_DEDICATED_SHADOW_LODBIAS)
		{
			// Note: this hack is to improve the look of dedicated character shadow cascade which otherwise has too sharp grass shadows and cascade transition becomes too obvious
			bias = 3.2;
		}
		clip(material.textures[BASECOLORMAP].SampleBias(sampler_linear_clamp, input.tex.xyxy, bias).a - material.GetAlphaTest());
	}
}
