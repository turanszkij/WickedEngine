#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ShaderInterop_HairParticle.h"

TEXTURE2D(texture_color, float4, TEXSLOT_ONDEMAND0);

uint2 main(VertexToPixel input) : SV_TARGET
{
	const float lineardepth = input.pos.w;

	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.fade);

	float4 color = texture_color.Sample(sampler_linear_clamp,input.tex);

	float alphatest = g_xMaterial.alphaTest;
	if (g_xFrame_Options & OPTION_BIT_TEMPORALAA_ENABLED)
	{
		alphatest = clamp(blue_noise(input.pos.xy, lineardepth).r, 0, 0.99);
	}
	clip(color.a - alphatest);

	return PackVisibility(input.primitiveID, xHairInstanceID, 0);
}
