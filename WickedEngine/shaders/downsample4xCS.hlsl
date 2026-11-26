#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;

	const float hdr_to_srgb_scaling = postprocess.params0.x;

	float4 color = 0;

	uint2 dim;
	input.GetDimensions(dim.x, dim.y);
	float2 dim_rcp = rcp(dim);

	color += tonemap(input.SampleLevel(sampler_linear_clamp, uv + float2(-1, -1) * dim_rcp, 0));
	color += tonemap(input.SampleLevel(sampler_linear_clamp, uv + float2(1, -1) * dim_rcp, 0));
	color += tonemap(input.SampleLevel(sampler_linear_clamp, uv + float2(-1, 1) * dim_rcp, 0));
	color += tonemap(input.SampleLevel(sampler_linear_clamp, uv + float2(1, 1) * dim_rcp, 0));

	color /= 4.0f;
	
	color.rgb = inverse_tonemap(color.rgb);

	if (hdr_to_srgb_scaling > 0)
	{
		// reverse of the IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR in imagePS
		//	this is used when scene HDR result is used for GUI background
		color.rgb = ApplySRGBCurve_Fast(color.rgb / hdr_to_srgb_scaling);
	}

	output[DTid.xy] = color;
}
