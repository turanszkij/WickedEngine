#include "globals.hlsli"
#include "emittedparticleHF.hlsli"
#include "depthConvertHF.hlsli"

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = texture_0.Sample(sampler_linear_clamp,PSIn.tex);
	clip(color.a - 1.0f / 255.0f);

	float2 pTex = PSIn.pos2D.xy / PSIn.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
	float4 depthScene = (texture_lineardepth.GatherRed(sampler_linear_clamp, pTex));
	float depthFragment = PSIn.pos2D.w;
	float fade = saturate(1.0 / PSIn.opaAddDarkSiz.w*(max(max(depthScene.x, depthScene.y), max(depthScene.z, depthScene.w)) - depthFragment));

	color.a = saturate(color.a * PSIn.opaAddDarkSiz.x * fade);

	return max(color, 0);
}
