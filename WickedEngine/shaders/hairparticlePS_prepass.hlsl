#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

uint2 main(VertexToPixel input, out uint coverage : SV_Coverage) : SV_Target
{
	ShaderMaterial material = HairGetMaterial();

	float alpha = 1;

	[branch]
	if (material.texture_basecolormap_index >= 0)
	{
		alpha = bindless_textures[material.texture_basecolormap_index].Sample(sampler_linear_clamp, input.tex).a;
	}

	// Distance dithered fade:
	if (dither(input.pos.xy + GetTemporalAASampleRotation()) - input.fade < 0)
	{
		alpha = 0;
	}

	coverage = AlphaToCoverage(alpha, material.alphaTest, input.pos);

	PrimitiveID prim;
	prim.primitiveIndex = input.primitiveID;
	prim.instanceIndex = xHairInstanceIndex;
	prim.subsetIndex = 0;
	return prim.pack();
}
