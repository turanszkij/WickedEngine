#include "imageHF.hlsli"


static const int NUM_SAMPLES = 35;

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,0);
	//float numSampling = 0.0f;

	//color += xTexture.Sample(Sampler,PSIn.tex);
	//numSampling++;

	//[branch]if(xLightShaft.x || xLightShaft.y) //LIGHTSHAFTS
	//{

	//	float2 lightPos = float2(xLightShaft.x/xDimension.x,xLightShaft.y/xDimension.y);
	//	float2 deltaTexCoord =  PSIn.tex - lightPos;
	//	deltaTexCoord *= 1.0f/NUM_SAMPLES * xProperties.x;
	//	float illuminationDecay = 1.0f;
	//	
	//	for( int i=0; i<NUM_SAMPLES; i++){
	//		PSIn.tex.xy-=deltaTexCoord;
	//		float3 sample = xTexture.SampleLevel(Sampler,PSIn.tex.xy,0).rgb;
	//		sample *= illuminationDecay*xProperties.y;
	//		color.xyz+=sample;
	//		illuminationDecay *= xProperties.z;
	//	}

	//	color*=xProperties.w;
	//}

	//return color/numSampling;

	return 0;
}