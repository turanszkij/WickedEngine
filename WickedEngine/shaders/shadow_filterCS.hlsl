#include "globals.hlsli"

PUSHCONSTANT(push, ShadowFilterPush);

Texture2D<float> shadowAtlas : register(t0);

RWTexture2D<float4> shadowAtlas_filtered : register(u0);

static const uint soft_shadow_sample_count = 16;
static const float soft_shadow_sample_count_rcp = 1.0 / (float)soft_shadow_sample_count;
static const float soft_shadow_sample_count_4_rcp = 1.0 / (float)(soft_shadow_sample_count * 4);
static const float soft_shadow_sample_count_sqrt_rcp = rsqrt((float)soft_shadow_sample_count);
static const float kGoldenAngle = 2.4;

// Moment shadow mapping things from: https://github.com/TheRealMJP/Shadows
float4 GetOptimizedMoments(in float depth)
{
    float square = depth * depth;
    float4 moments = float4(depth, square, square * depth, square * square);
    float4 optimized = mul(moments, float4x4(-2.07224649f,    13.7948857237f,  0.105877704f,   9.7924062118f,
                                              32.23703778f,  -59.4683975703f, -1.9077466311f, -33.7652110555f,
                                             -68.571074599f,  82.0359750338f,  9.3496555107f,  47.9456096605f,
                                              39.3703274134f,-35.364903257f,  -6.6543490743f, -23.9728048165f));
    optimized[0] += 0.035955884801f;
    return optimized;
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 pixel = clamp(DTid.xy, 0, push.rect.zw) + push.rect.xy;
	const float2 uv = (pixel + 0.5) * push.atlas_resolution_rcp;

	const float border = 0.51;
	const float2 topleft = (push.rect.xy + border) * push.atlas_resolution_rcp;
	const float2 bottomright = (push.rect.xy + push.rect.zw - border) * push.atlas_resolution_rcp;
	
	float4 moments = 0;
	
	const float2 spread_offset = push.atlas_resolution_rcp * (2 + push.spread * 8);
	for (uint i = 0; i < soft_shadow_sample_count; ++i)
	{
		// "Vogel disk" sampling pattern based on: https://github.com/corporateshark/poisson-disk-generator/blob/master/PoissonGenerator.h
		const float r = sqrt(float(i) + 0.5) * soft_shadow_sample_count_sqrt_rcp;
		const float theta = i * kGoldenAngle;
		float2 theta_cos_sin;
		sincos(theta, theta_cos_sin.y, theta_cos_sin.x);
		const float2 sample_uv = clamp(uv + r * theta_cos_sin * spread_offset, topleft, bottomright);
		const float4 depths = 1 - shadowAtlas.GatherRed(sampler_point_clamp, sample_uv).r;
		moments += GetOptimizedMoments(depths.w);
		moments += GetOptimizedMoments(depths.z);
		moments += GetOptimizedMoments(depths.x);
		moments += GetOptimizedMoments(depths.y);
	}
	moments *= soft_shadow_sample_count_4_rcp;
	
	if(all(DTid.xy < push.rect.zw))
	{
		shadowAtlas_filtered[pixel] = moments;
	}
}
