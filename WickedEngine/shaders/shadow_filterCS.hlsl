#include "globals.hlsli"
#include "shadowHF.hlsli"

ConstantBuffer<ShadowFilterData> filter : register(b2);

Texture2D<float> shadowAtlas : register(t0);
Texture2D<half4> shadowAtlas_transparent : register(t1);

RWTexture2D<float> shadowAtlas_filtered : register(u0);
RWTexture2D<half4> shadowAtlas_transparent_filtered : register(u1);

static const float kGoldenAngle = 2.4;

static const uint THREADCOUNT = SHADOW_FILTER_THREADSIZE;
static const int TILE_BORDER = 16;
static const int TILE_SIZE = TILE_BORDER + THREADCOUNT + TILE_BORDER;
groupshared float cache_z[TILE_SIZE * TILE_SIZE];
groupshared uint2 cache_rgba[TILE_SIZE * TILE_SIZE];
inline uint coord_to_cache(int2 coord)
{
	return flatten2D(clamp(coord, 0, TILE_SIZE - 1), TILE_SIZE);
}

[numthreads(THREADCOUNT, THREADCOUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	const float border = 0.51;
	const float2 topleft = (filter.rect.xy + border) * filter.atlas_resolution_rcp;
	const float2 bottomright = (filter.rect.xy + filter.rect.zw - border) * filter.atlas_resolution_rcp;

	// preload grid cache:
	const int2 tile_upperleft = Gid.xy * THREADCOUNT - TILE_BORDER + filter.rect.xy;
	for(uint x = GTid.x * 2; x < TILE_SIZE; x += THREADCOUNT * 2)
	for(uint y = GTid.y * 2; y < TILE_SIZE; y += THREADCOUNT * 2)
	{
		const int2 pixel = tile_upperleft + int2(x, y);
		const float2 uv = clamp((pixel + 0.5f) * filter.atlas_resolution_rcp, topleft, bottomright);
		const float2 origin_uv = inverse_lerp(topleft, bottomright, uv);
		float4 zzzz = shadowAtlas.GatherRed(sampler_linear_clamp, uv);
		half4 rrrr = shadowAtlas_transparent.GatherRed(sampler_linear_clamp, uv);
		half4 gggg = shadowAtlas_transparent.GatherGreen(sampler_linear_clamp, uv);
		half4 bbbb = shadowAtlas_transparent.GatherBlue(sampler_linear_clamp, uv);
		half4 aaaa = shadowAtlas_transparent.GatherAlpha(sampler_linear_clamp, uv);
		const uint t = coord_to_cache(int2(x, y));

		zzzz = 1 - zzzz;
		zzzz = saturate(zzzz);
		zzzz = exp(exponential_shadow_bias * zzzz);
		
		cache_z[t] = zzzz.w;
		cache_z[t + 1] = zzzz.z;
		cache_z[t + TILE_SIZE] = zzzz.x;
		cache_z[t + TILE_SIZE + 1] = zzzz.y;
		
		cache_rgba[t] = pack_half4(rrrr.w, gggg.w, bbbb.w, aaaa.w);
		cache_rgba[t + 1] = pack_half4(rrrr.z, gggg.z, bbbb.z, aaaa.z);
		cache_rgba[t + TILE_SIZE] = pack_half4(rrrr.x, gggg.x, bbbb.x, aaaa.x);
		cache_rgba[t + TILE_SIZE + 1] = pack_half4(rrrr.y, gggg.y, bbbb.y, aaaa.y);
	}
	GroupMemoryBarrierWithGroupSync();
	
	const uint2 pixel = clamp(DTid.xy, 0, filter.rect.zw) + filter.rect.xy;
	const float2 uv = (pixel + 0.5) * filter.atlas_resolution_rcp;

	float filtered = 0;
	float4 transparent_filtered = 0;

	const float2 spread_offset = filter.atlas_resolution_rcp * clamp(2 + filter.spread * 8, 2, TILE_BORDER);
	const uint soft_shadow_sample_count = (uint)lerp(4.0, 16.0, saturate(length(filter.spread)));
	const float soft_shadow_sample_count_rcp = 1.0 / (float)soft_shadow_sample_count;
	const float soft_shadow_sample_count_sqrt_rcp = rsqrt((float)soft_shadow_sample_count);

	// sample from cache:
	for (uint i = 0; i < soft_shadow_sample_count; ++i)
	{
		// "Vogel disk" sampling pattern based on: https://github.com/corporateshark/poisson-disk-generator/blob/master/PoissonGenerator.h
		const float r = sqrt(float(i) + 0.5) * soft_shadow_sample_count_sqrt_rcp;
		const float theta = i * kGoldenAngle;
		float2 theta_cos_sin;
		sincos(theta, theta_cos_sin.y, theta_cos_sin.x);
		const float2 sample_uv = clamp(uv + r * theta_cos_sin * spread_offset, topleft, bottomright);
		const float2 sample_pixel_float = sample_uv * filter.atlas_resolution;
		const float2 sample_pixel_frac = frac(sample_pixel_float);
		const uint2 sample_pixel = floor(sample_pixel_float);
		const int2 sample_coord = sample_pixel - tile_upperleft;
		const uint t = coord_to_cache(sample_coord);
		const float4 zzzz = float4(cache_z[t], cache_z[t + 1], cache_z[t + TILE_SIZE], cache_z[t + TILE_SIZE + 1]);
		const half4 rgba0 = unpack_half4(cache_rgba[t]);
		const half4 rgba1 = unpack_half4(cache_rgba[t + 1]);
		const half4 rgba2 = unpack_half4(cache_rgba[t + TILE_SIZE]);
		const half4 rgba3 = unpack_half4(cache_rgba[t + TILE_SIZE + 1]);
		const half4 rrrr = half4(rgba0.r, rgba1.r, rgba2.r, rgba3.r);
		const half4 gggg = half4(rgba0.g, rgba1.g, rgba2.g, rgba3.g);
		const half4 bbbb = half4(rgba0.b, rgba1.b, rgba2.b, rgba3.b);
		const half4 aaaa = half4(rgba0.a, rgba1.a, rgba2.a, rgba3.a);
		filtered += bilinear(zzzz, sample_pixel_frac);
		transparent_filtered.r += bilinear(rrrr, sample_pixel_frac);
		transparent_filtered.g += bilinear(gggg, sample_pixel_frac);
		transparent_filtered.b += bilinear(bbbb, sample_pixel_frac);
		transparent_filtered.a += bilinear(aaaa, sample_pixel_frac);
	}
	filtered *= soft_shadow_sample_count_rcp;
	transparent_filtered *= soft_shadow_sample_count_rcp;

	if(all(DTid.xy < filter.rect.zw))
	{
		shadowAtlas_filtered[pixel] = filtered;
		shadowAtlas_transparent_filtered[pixel] = transparent_filtered;
	}
}
