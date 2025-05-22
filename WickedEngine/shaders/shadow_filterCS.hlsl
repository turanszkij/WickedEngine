#include "globals.hlsli"
#include "shadowHF.hlsli"

ConstantBuffer<ShadowFilterData> filter : register(b2);

Texture2D<float> shadowAtlas : register(t0);
Texture2D<float4> shadowAtlas_transparent : register(t1);

RWTexture2D<float> shadowAtlas_filtered : register(u0);
RWTexture2D<float4> shadowAtlas_transparent_filtered : register(u1);

static const float kGoldenAngle = 2.4;

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const bool ortho = filter.range_rcp < 0;
	const uint2 pixel = clamp(DTid.xy, 0, filter.rect.zw) + filter.rect.xy;
	const float2 uv = (pixel + 0.5) * filter.atlas_resolution_rcp;
	const float4 uv_remap = float4(filter.rect.xy * filter.atlas_resolution_rcp, (filter.rect.xy + filter.rect.zw) * filter.atlas_resolution_rcp);

	const float border = 0.51;
	const float2 topleft = (filter.rect.xy + border) * filter.atlas_resolution_rcp;
	const float2 bottomright = (filter.rect.xy + filter.rect.zw - border) * filter.atlas_resolution_rcp;

	float filtered = 0;
	float4 transparent_filtered = 0;

	const float2 spread_offset = filter.atlas_resolution_rcp * (2 + filter.spread * 8);
	const uint soft_shadow_sample_count = (uint)lerp(8.0, 12.0, saturate(length(filter.spread)));
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
		float4 zzzz = shadowAtlas.GatherRed(sampler_linear_clamp, sample_uv);
		if (ortho)
		{
			zzzz = 1 - zzzz;
		}
		else
		{
			const float2 origin_uv = inverse_lerp(uv_remap.xy, uv_remap.zw, sample_uv);
			zzzz = float4(
				distance(filter.eye, reconstruct_position(origin_uv, zzzz.x, filter.inverse_view_projection)),
				distance(filter.eye, reconstruct_position(origin_uv, zzzz.y, filter.inverse_view_projection)),
				distance(filter.eye, reconstruct_position(origin_uv, zzzz.z, filter.inverse_view_projection)),
				distance(filter.eye, reconstruct_position(origin_uv, zzzz.w, filter.inverse_view_projection))
			);
			zzzz = (zzzz * filter.range_rcp);
		}
		zzzz = saturate(zzzz);
		zzzz = exp(exponential_shadow_bias * zzzz);
		filtered += bilinear(zzzz, frac(sample_uv * filter.atlas_resolution));

		float4 aaaa = shadowAtlas_transparent.GatherAlpha(sampler_linear_clamp, sample_uv);
		if (ortho)
		{
			aaaa = 1 - aaaa;
		}
		else
		{
			const float2 origin_uv = inverse_lerp(uv_remap.xy, uv_remap.zw, sample_uv);
			aaaa = float4(
				distance(filter.eye, reconstruct_position(origin_uv, aaaa.x, filter.inverse_view_projection)),
				distance(filter.eye, reconstruct_position(origin_uv, aaaa.y, filter.inverse_view_projection)),
				distance(filter.eye, reconstruct_position(origin_uv, aaaa.z, filter.inverse_view_projection)),
				distance(filter.eye, reconstruct_position(origin_uv, aaaa.w, filter.inverse_view_projection))
			);
			aaaa = (aaaa * filter.range_rcp);
		}
		aaaa = saturate(aaaa);
		float a = bilinear(aaaa, frac(sample_uv * filter.atlas_resolution));
		transparent_filtered += float4(shadowAtlas_transparent.SampleLevel(sampler_linear_clamp, sample_uv, 0).rgb, a);
	}
	filtered *= soft_shadow_sample_count_rcp;
	transparent_filtered *= soft_shadow_sample_count_rcp;

	if(all(DTid.xy < filter.rect.zw))
	{
		shadowAtlas_filtered[pixel] = filtered;
		shadowAtlas_transparent_filtered[pixel] = transparent_filtered;
	}
}
