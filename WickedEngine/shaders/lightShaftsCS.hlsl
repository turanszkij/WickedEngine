#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<half4> input : register(t0);

RWTexture2D<half4> output : register(u0);

static const uint NUM_SAMPLES = 64;

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 uv = (DTid.xy + 0.5) * postprocess.resolution_rcp;

	half3 color = input.SampleLevel(sampler_linear_clamp, uv, 0).rgb;

	float2 lightPos = postprocess.params1.xy;
	float2 deltaTexCoord =  uv - lightPos;
	deltaTexCoord *= postprocess.params0.x / NUM_SAMPLES;
	half illuminationDecay = 1.0;
	
	for (uint i = 0; i < NUM_SAMPLES; i++)
	{
		uv.xy -= deltaTexCoord;
		half3 sam = input.SampleLevel(sampler_linear_clamp, uv.xy, 0).rgb;
		sam *= illuminationDecay * postprocess.params0.y;
		color.rgb += sam;
		illuminationDecay *= postprocess.params0.z;
	}

	color *= postprocess.params0.w;

	output[DTid.xy] = half4(color, 1);
}
