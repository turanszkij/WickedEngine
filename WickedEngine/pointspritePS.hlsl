#include "depthConvertHF.hlsli"
#include "globalsHF.hlsli"


Texture2D xTexture : register(t0);
Texture2D<float> depthMap:register(t1);
SamplerState Sampler : register(s0);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	float4 opaAddDarkSiz		: TEXCOORD1;
	//float3 col				: TEXCOORD2;
	float4 pp				: TEXCOORD2;
};


float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float2 pTex;
		pTex.x = PSIn.pp.x/PSIn.pp.w/2.0f +0.5f;
		pTex.y = -PSIn.pp.y/PSIn.pp.w/2.0f +0.5f;
	float4 depthScene=(depthMap.GatherRed(Sampler,pTex));
	float depthFragment=PSIn.pp.z;
	float fade = saturate(1.0/PSIn.opaAddDarkSiz.w*(max(max(depthScene.x,depthScene.y),max(depthScene.z,depthScene.w))-depthFragment));
	//fade = depthScene<depthFragment?0:1;

	float4 color = float4(0,0,0,0);
	color=xTexture.Sample(Sampler,PSIn.tex);

	[branch]if(PSIn.opaAddDarkSiz.z){
		color.rgb=float3(0,0,0);
	}
	else{
		[branch]if(PSIn.opaAddDarkSiz.y){
			color.rgba-=PSIn.opaAddDarkSiz.x;
			color.rgba*=fade;
		}
		else{
			color.a-=PSIn.opaAddDarkSiz.x;
			color.a*=fade;
		}
	}

	//color.rgb*=PSIn.col.rgb;

	//return clamp( color, 0, inf );
	return max(color, 0);
}
