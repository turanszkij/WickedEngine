#include "imageHF.hlsli"


static const uint NUM_SAMPLES = 35;

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float3 color = xTexture.Sample(Sampler,PSIn.tex).rgb;

	float2 lightPos = float2(xPPParams1[0] / GetInternalResolution().x, xPPParams1[1] / GetInternalResolution().y);
	float2 deltaTexCoord =  PSIn.tex - lightPos;
	deltaTexCoord *= xPPParams0[0] / NUM_SAMPLES;
	float illuminationDecay = 1.0f;
	
	[unroll]
	for (uint i = 0; i < NUM_SAMPLES; i++) 
	{
		PSIn.tex.xy -= deltaTexCoord;
		float3 sam = xTexture.SampleLevel(sampler_linear_clamp, PSIn.tex.xy, 0).rgb;
		sam *= illuminationDecay*xPPParams0[1];
		color.rgb += sam;
		illuminationDecay *= xPPParams0[2];
	}

	color*= xPPParams0[3];

	return float4(color, 1);
}