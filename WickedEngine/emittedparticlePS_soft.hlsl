#include "globals.hlsli"
#include "emittedparticleHF.hlsli"
#include "depthConvertHF.hlsli"
#include "ShaderInterop_EmittedParticle.h"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 color = texture_0.Sample(sampler_linear_clamp,input.tex);
	clip(color.a - 1.0f / 255.0f);

	float2 pTex = input.pos2D.xy / input.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
	float4 depthScene = texture_lineardepth.GatherRed(sampler_linear_clamp, pTex) * g_xFrame_MainCamera_ZFarP;
	float depthFragment = input.pos2D.w;
	float fade = saturate(1.0 / input.size*(max(max(depthScene.x, depthScene.y), max(depthScene.z, depthScene.w)) - depthFragment));

	float4 inputColor;
	inputColor.r = ((input.color >> 0)  & 0x000000FF) / 255.0f;
	inputColor.g = ((input.color >> 8)  & 0x000000FF) / 255.0f;
	inputColor.b = ((input.color >> 16) & 0x000000FF) / 255.0f;
	inputColor.a = ((input.color >> 24) & 0x000000FF) / 255.0f;

	float opacity = saturate(color.a * inputColor.a * fade);

	color.rgb *= inputColor.rgb * (1 + xParticleEmissive);
	color.a = opacity;

#ifdef DISTORTION
	// just make normal maps blendable:
	color.rgb = color.rgb - 0.5f;
#endif // DISTORTION

	return max(color, 0);
}
