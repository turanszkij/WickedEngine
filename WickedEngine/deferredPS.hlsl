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
	float4 color=0;
	float2 depthMapSize;
	xSceneDepthMap.GetDimensions(depthMapSize.x,depthMapSize.y);
	float  depth  = ( xSceneDepthMap.Load(int4(depthMapSize*PSIn.tex.xy,0,0)).r );

	[branch]if(depth<zFarP){
		color = xTexture.SampleLevel(Sampler,PSIn.tex,0);
		color=pow(color,GAMMA);
		float4 lighting = xLightMap.SampleLevel(Sampler,PSIn.tex,0);
		color.rgb*=lighting.rgb+xAmbient;
		color=pow(color,INV_GAMMA);
		color.rgb *= xAOMap.SampleLevel(Sampler,PSIn.tex,0).r;

		float fog = getFog((depth),xFogSEH);
		color.rgb = applyFog(color.rgb,xHorizon.rgb,fog);
	}

	return color;
}