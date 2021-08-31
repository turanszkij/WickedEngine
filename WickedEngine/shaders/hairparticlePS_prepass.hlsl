#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

uint2 main(VertexToPixel input) : SV_TARGET
{
	ShaderMaterial material = HairGetMaterial();

	const float lineardepth = input.pos.w;

	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.fade);

	float4 color = 1;

	[branch]
	if (material.texture_basecolormap_index >= 0)
	{
		color = bindless_textures[material.texture_basecolormap_index].Sample(sampler_linear_clamp, input.tex);
	}

	float alphatest = material.alphaTest;
	if (g_xFrame.Options & OPTION_BIT_TEMPORALAA_ENABLED)
	{
		alphatest = clamp(blue_noise(input.pos.xy, lineardepth).r, 0, 0.99);
	}
	clip(color.a - alphatest);

	PrimitiveID prim;
	prim.primitiveIndex = input.primitiveID;
	prim.instanceIndex = xHairInstanceIndex;
	prim.subsetIndex = 0;
	return prim.pack();
}
