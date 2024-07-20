#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

#ifdef __PSSL__
#pragma PSSL_target_output_format (target 0 FMT_32_R)
#endif // __PSSL__

uint main(VertexToPixel input, out uint coverage : SV_Coverage) : SV_Target
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

	PrimitiveID prim;
	prim.primitiveIndex = input.primitiveID;
	prim.instanceIndex = xHairInstanceIndex;
	prim.subsetIndex = 0;
	return prim.pack();
}
