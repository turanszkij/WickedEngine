#include "imageHF.hlsli"


static const uint NUM_SAMPLES = 32;
static const uint UNROLL_GRANULARITY = 8;

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float3 color = xTexture.Sample(Sampler,PSIn.tex).rgb;

	float2 lightPos = float2(xPPParams1[0] / GetInternalResolution().x, xPPParams1[1] / GetInternalResolution().y);
	float2 deltaTexCoord =  PSIn.tex - lightPos;
	deltaTexCoord *= xPPParams0[0] / NUM_SAMPLES;
	float illuminationDecay = 1.0f;
	
	[loop] // loop big part (balance register pressure)
	for (uint i = 0; i < NUM_SAMPLES / UNROLL_GRANULARITY; i++)
	{
		[unroll] // unroll small parts (balance register pressure)
		for (uint j = 0; j < UNROLL_GRANULARITY; ++j)
		{
			PSIn.tex.xy -= deltaTexCoord;
			float3 sam = xTexture.SampleLevel(sampler_linear_clamp, PSIn.tex.xy, 0).rgb;
			sam *= illuminationDecay * xPPParams0[1];
			color.rgb += sam;
			illuminationDecay *= xPPParams0[2];
		}
	}

	color*= xPPParams0[3];

	return float4(color, 1);
}