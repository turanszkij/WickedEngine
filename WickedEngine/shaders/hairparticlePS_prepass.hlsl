#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

struct PSOut
{
	uint2 primitiveID : SV_Target0;
	uint coverage : SV_Coverage;
};

PSOut main(VertexToPixel input)
{
	ShaderMaterial material = HairGetMaterial();

	const float lineardepth = input.pos.w;

	const uint2 pixel = input.pos.xy;
	clip(dither(pixel + GetTemporalAASampleRotation()) - input.fade);

	float4 color = 1;

	[branch]
	if (material.texture_basecolormap_index >= 0)
	{
		color = bindless_textures[material.texture_basecolormap_index].Sample(sampler_linear_clamp, input.tex);
	}

	float alphatest = material.alphaTest;
	if (GetFrame().options & OPTION_BIT_TEMPORALAA_ENABLED)
	{
		alphatest = clamp(blue_noise(input.pos.xy, lineardepth).r, 0, 0.99);
	}
	clip(color.a - alphatest);

	PrimitiveID prim;
	prim.primitiveIndex = input.primitiveID;
	prim.instanceIndex = xHairInstanceIndex;
	prim.subsetIndex = 0;

	PSOut Out;
	Out.primitiveID = prim.pack();
	Out.coverage = AlphaToCoverage(color.a, material.alphaTest, pixel);
	return Out;
}
