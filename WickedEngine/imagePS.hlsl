#include "imageHF.hlsli"

cbuffer psbuffer:register(b2){
	float4 xMaskFadOpaDis;
	float4 xDimension;
};

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float4 tex				: TEXCOORD0; //intex,movingtex
	float4 dis				: TEXCOORD1; //distortion
	float mip				: TEXCOORD2;
};

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,0);

		
	[branch]if(xMaskFadOpaDis.w==1){
		float2 distortionCo;
			distortionCo.x = PSIn.dis.x/PSIn.dis.w/2.0f + 0.5f;
			distortionCo.y = -PSIn.dis.y/PSIn.dis.w/2.0f + 0.5f;
			float2 distort = normalize(float3(xNormalMap.SampleLevel(Sampler, PSIn.tex, PSIn.mip).rg, 1))*0.04f*(1 - xMaskFadOpaDis.z);
		PSIn.tex.xy=distortionCo+distort;
	}

	color += xTexture.SampleLevel(Sampler, PSIn.tex + PSIn.tex.zw, PSIn.mip);
	[branch]if(xMaskFadOpaDis.x==1)
		color *= xMaskTex.SampleLevel(Sampler, PSIn.tex, PSIn.mip).a;
	[branch]if(xMaskFadOpaDis.w==2)
		color=2*color-1;
	

	color.rgb*=1-xMaskFadOpaDis.y;
	color.a-=xMaskFadOpaDis.z;
	color.a=saturate(color.a);


	return color;
}