#include "imageHF.hlsli"


static const int NUM_SAMPLES = 35;

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,0);
	float numSampling = 0.0f;

	color += xTexture.Sample(Sampler,PSIn.tex);
	numSampling++;

	[branch]if(xPPParams1[0] || xPPParams1[1]) //LIGHTSHAFTS
	{

		float2 lightPos = float2(xPPParams1[0] / g_xWorld_ScreenWidthHeight.x, xPPParams1[1] / g_xWorld_ScreenWidthHeight.y);
		float2 deltaTexCoord =  PSIn.tex - lightPos;
		deltaTexCoord *= 1.0f/NUM_SAMPLES * xPPParams0[0];
		float illuminationDecay = 1.0f;
		
		for( int i=0; i<NUM_SAMPLES; i++){
			PSIn.tex.xy-=deltaTexCoord;
			float3 sample = xTexture.SampleLevel(Sampler,PSIn.tex.xy,0).rgb;
			sample *= illuminationDecay*xPPParams0[1];
			color.xyz+=sample;
			illuminationDecay *= xPPParams0[2];
		}

		color*= xPPParams0[3];
	}

	return color/numSampling;
}