#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
// This will cut out bright parts (>1) and also downsample 4x

PUSHCONSTANT(bloom, Bloom);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = DTid.xy + 0.5;

	half3 color = 0;

	Texture2D<half4> texture_input = bindless_textures_half4[descriptor_index(bloom.texture_input)];
	color += texture_input.SampleLevel(sampler_linear_clamp, (uv + float2(-0.5, -0.5)) * bloom.resolution_rcp, 0).rgb;
	color += texture_input.SampleLevel(sampler_linear_clamp, (uv + float2(0.5, -0.5)) * bloom.resolution_rcp, 0).rgb;
	color += texture_input.SampleLevel(sampler_linear_clamp, (uv + float2(-0.5, 0.5)) * bloom.resolution_rcp, 0).rgb;
	color += texture_input.SampleLevel(sampler_linear_clamp, (uv + float2(0.5, 0.5)) * bloom.resolution_rcp, 0).rgb;

	color /= 4.0;

	float exposure = bloom.exposure;

	[branch]
	if (bloom.buffer_input_luminance >= 0)
	{
		exposure *= bindless_buffers[descriptor_index(bloom.buffer_input_luminance)].Load<float>(LUMINANCE_BUFFER_OFFSET_EXPOSURE);
	}
	
	color *= exposure;

	color = min(color, 10); // clamp upper limit: avoid incredibly large values to overly dominate bloom (high speculars were causing problems)
	color = max(color - bloom.threshold, 0);

	bindless_rwtextures[descriptor_index(bloom.texture_output)][DTid.xy] = float4(color, 1);
}
