#include "imageHF.hlsli"

cbuffer shaftBuffer:register(b0){
	float4 xProperties;
	float4 xLightShaft;
	float2 padding;
};
cbuffer psbuffer:register(b2){
	float4 xMaskFadOpaDis;
	float4 xDimension;
};

static const int NUM_SAMPLES = 35;


struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,0);
	float numSampling = 0.0f;

	color += xTexture.Sample(Sampler,PSIn.tex);
	numSampling++;

	[branch]if(xLightShaft.x || xLightShaft.y) //LIGHTSHAFTS
	{

		float2 lightPos = float2(xLightShaft.x/xDimension.x,xLightShaft.y/xDimension.y);
		float2 deltaTexCoord =  PSIn.tex - lightPos;
		deltaTexCoord *= 1.0f/NUM_SAMPLES * xProperties.x;
		float illuminationDecay = 1.0f;
		
		for( int i=0; i<NUM_SAMPLES; i++){
			PSIn.tex.xy-=deltaTexCoord;
			float3 sample = xTexture.SampleLevel(Sampler,PSIn.tex,0);
			sample *= illuminationDecay*xProperties.y;
			color.xyz+=sample;
			illuminationDecay *= xProperties.z;
		}

		color*=xProperties.w;
	}

	return color/numSampling;
}