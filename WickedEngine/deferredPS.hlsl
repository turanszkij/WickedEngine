#include "postProcessHF.hlsli"
#include "fogHF.hlsli"
#include "gammaHF.hlsli"

Texture2D<float4> xLightMap:register(t7);
Texture2D<float> xAOMap:register(t8);


#include "reconstructPositionHF.hlsli"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = xTexture.Load(int3(PSIn.pos.xy, 0));
	float  depth = xSceneDepthMap.Load(int3(PSIn.pos.xy, 0)).r;

	[branch]if(depth<g_xCamera_ZFarP){
		color=pow(abs(color),GAMMA);
		float4 lighting = xLightMap.SampleLevel(Sampler,PSIn.tex,0);
		color.rgb *= lighting.rgb + g_xWorld_Ambient.rgb;
		color.rgb *= xAOMap.SampleLevel(Sampler, PSIn.tex.xy, 0).rrr;
		color=pow(abs(color),INV_GAMMA);

		float fog = getFog((depth));
		color.rgb = applyFog(color.rgb,fog);
	}

	return color;
}