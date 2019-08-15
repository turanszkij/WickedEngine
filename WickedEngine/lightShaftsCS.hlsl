#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float4, 0);

static const uint NUM_SAMPLES = 32;
static const uint UNROLL_GRANULARITY = 8;

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	float3 color = input.SampleLevel(sampler_linear_clamp, uv, 0).rgb;

	float2 lightPos = xPPParams1.xy;
	float2 deltaTexCoord =  uv - lightPos;
	deltaTexCoord *= xPPParams0.x / NUM_SAMPLES;
	float illuminationDecay = 1.0f;
	
	[loop] // loop big part (balance register pressure)
	for (uint i = 0; i < NUM_SAMPLES / UNROLL_GRANULARITY; i++)
	{
		[unroll] // unroll small parts (balance register pressure)
		for (uint j = 0; j < UNROLL_GRANULARITY; ++j)
		{
			uv.xy -= deltaTexCoord;
			float3 sam = input.SampleLevel(sampler_linear_clamp, uv.xy, 0).rgb;
			sam *= illuminationDecay * xPPParams0.y;
			color.rgb += sam;
			illuminationDecay *= xPPParams0.z;
		}
	}

	color*= xPPParams0.w;

	output[DTid.xy] = float4(color, 1);
}