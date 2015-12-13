#include "postProcessHF.hlsli"
#include "fogHF.hlsli"
#include "gammaHF.hlsli"

Texture2D<float4> xLightMap:register(t7);
Texture2D<float> xAOMap:register(t8);

cbuffer prop:register(b0){
	float4 xAmbient;
	float4 xHorizon;
	float4x4 matProjInv;
	float4 xFogSEH;
};

#include "reconstructPositionHF.hlsli"


float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = xTexture.Load(int3(PSIn.pos.xy, 0));
	float  depth = xSceneDepthMap.Load(int3(PSIn.pos.xy, 0)).r;

	[branch]if(depth<zFarP){
		color=pow(abs(color),GAMMA);
		float4 lighting = xLightMap.SampleLevel(Sampler,PSIn.tex,0);
		color.rgb *= lighting.rgb + xAmbient.rgb * xAOMap.SampleLevel(Sampler, PSIn.tex.xy, 0).r;
		color=pow(abs(color),INV_GAMMA);

		float fog = getFog((depth),xFogSEH.xyz);
		color.rgb = applyFog(color.rgb,xHorizon.rgb,fog);
	}

	return color;
}