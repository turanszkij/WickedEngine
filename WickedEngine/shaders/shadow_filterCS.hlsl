#include "globals.hlsli"

PUSHCONSTANT(push, ShadowFilterPush);

Texture2D<float> shadowAtlas : register(t0);

RWTexture2D<float> shadowAtlas_filtered : register(u0);

static const float kGoldenAngle = 2.4;

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 pixel = clamp(DTid.xy, 0, push.rect.zw) + push.rect.xy;
	const float2 uv = (pixel + 0.5) * push.atlas_resolution_rcp;

	const float border = 0.51;
	const float2 topleft = (push.rect.xy + border) * push.atlas_resolution_rcp;
	const float2 bottomright = (push.rect.xy + push.rect.zw - border) * push.atlas_resolution_rcp;

	float filtered = 0;

	const float2 spread_offset = push.atlas_resolution_rcp * (2 + push.spread * 8);
	const uint soft_shadow_sample_count = (uint)lerp(8.0, 64.0, saturate(length(push.spread)));
	const float soft_shadow_sample_count_rcp = 1.0 / (float)soft_shadow_sample_count;
	const float soft_shadow_sample_count_sqrt_rcp = rsqrt((float)soft_shadow_sample_count);

	for (uint i = 0; i < soft_shadow_sample_count; ++i)
	{
		// "Vogel disk" sampling pattern based on: https://github.com/corporateshark/poisson-disk-generator/blob/master/PoissonGenerator.h
		const float r = sqrt(float(i) + 0.5) * soft_shadow_sample_count_sqrt_rcp;
		const float theta = i * kGoldenAngle;
		float2 theta_cos_sin;
		sincos(theta, theta_cos_sin.y, theta_cos_sin.x);
		const float2 sample_uv = clamp(uv + r * theta_cos_sin * spread_offset, topleft, bottomright);
		filtered += shadowAtlas.SampleLevel(sampler_linear_clamp, sample_uv, 0);
	}
	filtered *= soft_shadow_sample_count_rcp;

	if(all(DTid.xy < push.rect.zw))
	{
		shadowAtlas_filtered[pixel] = filtered;
	}
}
