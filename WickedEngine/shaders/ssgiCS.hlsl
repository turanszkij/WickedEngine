#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

#define OCCLUSION_TEST

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float4> input : register(t0);
Texture2DArray<float> input_depth : register(t1);
Texture2DArray<float2> input_normal : register(t2);

RWTexture2D<float4> output_diffuse : register(u0);

static const float radius = 14;
static const float radius2 = radius * radius;
static const float radius2_rcp_negative  = -rcp(radius2);

#if 0
static const uint depth_test_count = 1;
static const float depth_tests[] = {0.33};
#else
static const uint depth_test_count = 3;
static const float depth_tests[] = {0.125, 0.25, 0.75};
#endif

float3 compute_diffuse(
	float3 origin_position,
	float3 origin_normal,
	float2 origin_uv,
	float2 sample_uv,
	uint layer
)
{
	float sample_depth = input_depth.SampleLevel(sampler_point_clamp, float3(sample_uv, layer), 0);
	float3 sample_position = reconstruct_position(sample_uv, sample_depth, GetCamera().inverse_projection);
    float3 origin_to_sample = sample_position - origin_position;
    float distance2 = dot(origin_to_sample, origin_to_sample);
    float occlusion = saturate(dot(origin_normal, origin_to_sample));
    occlusion *= saturate(distance2 * radius2_rcp_negative + 1.0f);
	//occlusion /= max(0.001, distance2 * 0.05);
	
#ifdef OCCLUSION_TEST
	if(occlusion > 0)
	{
		const float origin_z = origin_position.z;
		const float sample_z = sample_position.z;
		for(uint i = 0; i < depth_test_count; ++i)
		{
			const float t = depth_tests[i];
			const float z = lerp(origin_z, sample_z, t);
			const float2 uv = lerp(origin_uv, sample_uv, t);
			const float depth = input_depth.SampleLevel(sampler_point_clamp, float3(uv, layer), 0);
			const float lineardepth = compute_lineardepth(depth);
			if(lineardepth < z - 0.1)
			{
				return occlusion * input.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
			}
		}
	}
#endif // OCCLUSION_TEST

    return occlusion * input.SampleLevel(sampler_linear_clamp, sample_uv, 0).rgb;
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint layer = DTid.z;
	const uint2 pixel = DTid.xy;
	const float2 uv = (pixel + 0.5) * postprocess.resolution_rcp;

	const float depth = input_depth[uint3(pixel, layer)];
	const float3 P = reconstruct_position(uv, depth, GetCamera().inverse_projection);
	const float3 N = mul((float3x3)GetCamera().view, decode_oct(input_normal[uint3(pixel, layer)].rg));
	
	float3 diffuse = 0;
	float sum = 0;
	int range = int(postprocess.params0.x);
	float spread = postprocess.params0.y * 2;
	spread += dither(pixel);
	for(int x = -range; x <= range; ++x)
	{
		for(int y = -range; y <= range; ++y)
		{
			const float2 offset = float2(x, y) * spread * postprocess.resolution_rcp;
			const float2 sample_uv = uv + offset;
			const float weight = 1;
			diffuse += compute_diffuse(P, N, uv, sample_uv, layer) * weight;
			sum += weight;
		}
	}
	if(sum > 0)
	{
		diffuse = diffuse / sum;
	}
	
    uint2 OutPixel = DTid.xy << 2 | uint2(DTid.z & 3, DTid.z >> 2);
    output_diffuse[OutPixel] = float4(diffuse, 1);
}
