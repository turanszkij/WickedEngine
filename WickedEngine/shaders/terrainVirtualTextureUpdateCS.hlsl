#include "globals.hlsli"

static const uint region_count = 4;
Texture2D<float4> region_weights_texture : register(t0);

struct VirtualTexturePush
{
	uint4 offset_size;
};
PUSHCONSTANT(push, VirtualTexturePush);

struct Terrain
{
	ShaderMaterial materials[region_count];
};
ConstantBuffer<Terrain> terrain : register(b0);

RWTexture2D<unorm float4> output_baseColorMap_mip0 : register(u0);
RWTexture2D<unorm float4> output_baseColorMap_mip1 : register(u1);
RWTexture2D<unorm float4> output_baseColorMap_mip2 : register(u2);
RWTexture2D<unorm float4> output_baseColorMap_mip3 : register(u3);

RWTexture2D<unorm float4> output_normalMap_mip0 : register(u4);
RWTexture2D<unorm float4> output_normalMap_mip1 : register(u5);
RWTexture2D<unorm float4> output_normalMap_mip2 : register(u6);
RWTexture2D<unorm float4> output_normalMap_mip3 : register(u7);

RWTexture2D<unorm float4> output_surfaceMap_mip0 : register(u8);
RWTexture2D<unorm float4> output_surfaceMap_mip1 : register(u9);
RWTexture2D<unorm float4> output_surfaceMap_mip2 : register(u10);
RWTexture2D<unorm float4> output_surfaceMap_mip3 : register(u11);

groupshared float lds_r[8][8];
groupshared float lds_g[8][8];
groupshared float lds_b[8][8];
groupshared float lds_a[8][8];

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint2 GTid : SV_GroupThreadID)
{
	if (DTid.x >= push.offset_size.z || DTid.y >= push.offset_size.w)
		return;

	float2 output_dim = push.offset_size.zw;
	const float2 uv = (DTid.xy + 0.5f) / output_dim;

	float4 region_weights = region_weights_texture.SampleLevel(sampler_linear_clamp, uv, 0);

	float weight_sum = 0;
	float4 total_baseColor = 0;
	float4 total_surface = 0;
	float2 total_normal = 0;
	for (uint i = 0; i < region_count; ++i)
	{
		float weight = region_weights[i];

		ShaderMaterial material = terrain.materials[i];

		float4 baseColor = material.baseColor;
		[branch]
		if (material.texture_basecolormap_index >= 0)
		{
			Texture2D tex = bindless_textures[material.texture_basecolormap_index];
			float2 dim = 0;
			tex.GetDimensions(dim.x, dim.y);
			float2 diff = dim / output_dim;
			float lod = log2(max(diff.x, diff.y));
			float2 overscale = lod < 0 ? diff : 1;
			float4 baseColorMap = tex.SampleLevel(sampler_linear_wrap, uv / overscale, lod);
			baseColor *= baseColorMap;
		}
		total_baseColor += baseColor * weight;

		float4 surface = float4(1, material.roughness, material.metalness, material.reflectance);
		[branch]
		if (material.texture_surfacemap_index >= 0)
		{
			Texture2D tex = bindless_textures[material.texture_surfacemap_index];
			float2 dim = 0;
			tex.GetDimensions(dim.x, dim.y);
			float2 diff = dim / output_dim;
			float lod = log2(max(diff.x, diff.y));
			float2 overscale = lod < 0 ? diff : 1;
			float4 surfaceMap = tex.SampleLevel(sampler_linear_wrap, uv / overscale, lod);
			surface *= surfaceMap;
		}
		total_surface += surface * weight;
		
		float2 normal = float2(0.5, 0.5);
		[branch]
		if (material.texture_normalmap_index >= 0)
		{
			Texture2D tex = bindless_textures[material.texture_normalmap_index];
			float2 dim = 0;
			tex.GetDimensions(dim.x, dim.y);
			float2 diff = dim / output_dim;
			float lod = log2(max(diff.x, diff.y));
			float2 overscale = lod < 0 ? diff : 1;
			float2 normalMap = tex.SampleLevel(sampler_linear_wrap, uv / overscale, lod).rg;
			normal = normalMap;
		}
		total_normal += normal * weight;

		weight_sum += weight;
	}
	total_baseColor /= weight_sum;
	total_surface /= weight_sum;
	total_normal /= weight_sum;

	const uint2 pixel = DTid.xy + push.offset_size.xy;
	output_baseColorMap_mip0[pixel] = total_baseColor;
	output_surfaceMap_mip0[pixel] = total_surface;
	output_normalMap_mip0[pixel] = float4(total_normal, 1, 1);

#if 0
	// Mip writes:

	// Basecolormap:

	lds_r[GTid.x][GTid.y] = total_baseColor.r;
	lds_g[GTid.x][GTid.y] = total_baseColor.g;
	lds_b[GTid.x][GTid.y] = total_baseColor.b;

	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		float4 value = (
			float4(lds_r[GTid.x + 0][GTid.y + 0], lds_g[GTid.x + 0][GTid.y + 0], lds_b[GTid.x + 0][GTid.y + 0], 1) +
			float4(lds_r[GTid.x + 1][GTid.y + 0], lds_g[GTid.x + 1][GTid.y + 0], lds_b[GTid.x + 1][GTid.y + 0], 1) +
			float4(lds_r[GTid.x + 0][GTid.y + 1], lds_g[GTid.x + 0][GTid.y + 1], lds_b[GTid.x + 0][GTid.y + 1], 1) +
			float4(lds_r[GTid.x + 1][GTid.y + 1], lds_g[GTid.x + 1][GTid.y + 1], lds_b[GTid.x + 1][GTid.y + 1], 1)
			) / 4.0f;

		output_baseColorMap_mip1[pixel / 2] = value;

		lds_r[GTid.x][GTid.y] = value.r;
		lds_g[GTid.x][GTid.y] = value.g;
		lds_b[GTid.x][GTid.y] = value.b;
	}

	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		float4 value = (
			float4(lds_r[GTid.x + 0][GTid.y + 0], lds_g[GTid.x + 0][GTid.y + 0], lds_b[GTid.x + 0][GTid.y + 0], 1) +
			float4(lds_r[GTid.x + 2][GTid.y + 0], lds_g[GTid.x + 2][GTid.y + 0], lds_b[GTid.x + 2][GTid.y + 0], 1) +
			float4(lds_r[GTid.x + 0][GTid.y + 2], lds_g[GTid.x + 0][GTid.y + 2], lds_b[GTid.x + 0][GTid.y + 2], 1) +
			float4(lds_r[GTid.x + 2][GTid.y + 2], lds_g[GTid.x + 2][GTid.y + 2], lds_b[GTid.x + 2][GTid.y + 2], 1)
			) / 4.0f;

		output_baseColorMap_mip2[pixel / 4] = value;

		lds_r[GTid.x][GTid.y] = value.r;
		lds_g[GTid.x][GTid.y] = value.g;
		lds_b[GTid.x][GTid.y] = value.b;
	}

	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 8 == 0 && GTid.y % 8 == 0)
	{
		float4 value = (
			float4(lds_r[GTid.x + 0][GTid.y + 0], lds_g[GTid.x + 0][GTid.y + 0], lds_b[GTid.x + 0][GTid.y + 0], 1) +
			float4(lds_r[GTid.x + 4][GTid.y + 0], lds_g[GTid.x + 4][GTid.y + 0], lds_b[GTid.x + 4][GTid.y + 0], 1) +
			float4(lds_r[GTid.x + 0][GTid.y + 4], lds_g[GTid.x + 0][GTid.y + 4], lds_b[GTid.x + 0][GTid.y + 4], 1) +
			float4(lds_r[GTid.x + 4][GTid.y + 4], lds_g[GTid.x + 4][GTid.y + 4], lds_b[GTid.x + 4][GTid.y + 4], 1)
			) / 4.0f;

		output_baseColorMap_mip3[pixel / 8] = value;
	}

	GroupMemoryBarrierWithGroupSync();

	// Normalmap:

	lds_r[GTid.x][GTid.y] = total_normal.r;
	lds_g[GTid.x][GTid.y] = total_normal.g;

	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		float4 value = (
			float4(lds_r[GTid.x + 0][GTid.y + 0], lds_g[GTid.x + 0][GTid.y + 0], 1, 1) +
			float4(lds_r[GTid.x + 1][GTid.y + 0], lds_g[GTid.x + 1][GTid.y + 0], 1, 1) +
			float4(lds_r[GTid.x + 0][GTid.y + 1], lds_g[GTid.x + 0][GTid.y + 1], 1, 1) +
			float4(lds_r[GTid.x + 1][GTid.y + 1], lds_g[GTid.x + 1][GTid.y + 1], 1, 1)
			) / 4.0f;

		output_normalMap_mip1[pixel / 2] = value;

		lds_r[GTid.x][GTid.y] = value.r;
		lds_g[GTid.x][GTid.y] = value.g;
		lds_b[GTid.x][GTid.y] = value.b;
	}

	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		float4 value = (
			float4(lds_r[GTid.x + 0][GTid.y + 0], lds_g[GTid.x + 0][GTid.y + 0], 1, 1) +
			float4(lds_r[GTid.x + 2][GTid.y + 0], lds_g[GTid.x + 2][GTid.y + 0], 1, 1) +
			float4(lds_r[GTid.x + 0][GTid.y + 2], lds_g[GTid.x + 0][GTid.y + 2], 1, 1) +
			float4(lds_r[GTid.x + 2][GTid.y + 2], lds_g[GTid.x + 2][GTid.y + 2], 1, 1)
			) / 4.0f;

		output_normalMap_mip2[pixel / 4] = value;

		lds_r[GTid.x][GTid.y] = value.r;
		lds_g[GTid.x][GTid.y] = value.g;
		lds_b[GTid.x][GTid.y] = value.b;
	}

	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 8 == 0 && GTid.y % 8 == 0)
	{
		float4 value = (
			float4(lds_r[GTid.x + 0][GTid.y + 0], lds_g[GTid.x + 0][GTid.y + 0], 1, 1) +
			float4(lds_r[GTid.x + 4][GTid.y + 0], lds_g[GTid.x + 4][GTid.y + 0], 1, 1) +
			float4(lds_r[GTid.x + 0][GTid.y + 4], lds_g[GTid.x + 0][GTid.y + 4], 1, 1) +
			float4(lds_r[GTid.x + 4][GTid.y + 4], lds_g[GTid.x + 4][GTid.y + 4], 1, 1)
			) / 4.0f;

		output_normalMap_mip3[pixel / 8] = value;
	}

	GroupMemoryBarrierWithGroupSync();

	// Surfacemap:

	lds_r[GTid.x][GTid.y] = total_surface.r;
	lds_g[GTid.x][GTid.y] = total_surface.g;
	lds_b[GTid.x][GTid.y] = total_surface.b;
	lds_a[GTid.x][GTid.y] = total_surface.a;

	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		float4 value = (
			float4(lds_r[GTid.x + 0][GTid.y + 0], lds_g[GTid.x + 0][GTid.y + 0], lds_b[GTid.x + 0][GTid.y + 0], lds_a[GTid.x + 0][GTid.y + 0]) +
			float4(lds_r[GTid.x + 1][GTid.y + 0], lds_g[GTid.x + 1][GTid.y + 0], lds_b[GTid.x + 1][GTid.y + 0], lds_a[GTid.x + 1][GTid.y + 0]) +
			float4(lds_r[GTid.x + 0][GTid.y + 1], lds_g[GTid.x + 0][GTid.y + 1], lds_b[GTid.x + 0][GTid.y + 1], lds_a[GTid.x + 0][GTid.y + 1]) +
			float4(lds_r[GTid.x + 1][GTid.y + 1], lds_g[GTid.x + 1][GTid.y + 1], lds_b[GTid.x + 1][GTid.y + 1], lds_a[GTid.x + 1][GTid.y + 1])
			) / 4.0f;

		output_surfaceMap_mip1[pixel / 2] = value;

		lds_r[GTid.x][GTid.y] = value.r;
		lds_g[GTid.x][GTid.y] = value.g;
		lds_b[GTid.x][GTid.y] = value.b;
	}

	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		float4 value = (
			float4(lds_r[GTid.x + 0][GTid.y + 0], lds_g[GTid.x + 0][GTid.y + 0], lds_b[GTid.x + 0][GTid.y + 0], lds_a[GTid.x + 0][GTid.y + 0]) +
			float4(lds_r[GTid.x + 2][GTid.y + 0], lds_g[GTid.x + 2][GTid.y + 0], lds_b[GTid.x + 2][GTid.y + 0], lds_a[GTid.x + 2][GTid.y + 0]) +
			float4(lds_r[GTid.x + 0][GTid.y + 2], lds_g[GTid.x + 0][GTid.y + 2], lds_b[GTid.x + 0][GTid.y + 2], lds_a[GTid.x + 0][GTid.y + 2]) +
			float4(lds_r[GTid.x + 2][GTid.y + 2], lds_g[GTid.x + 2][GTid.y + 2], lds_b[GTid.x + 2][GTid.y + 2], lds_a[GTid.x + 2][GTid.y + 2])
			) / 4.0f;

		output_surfaceMap_mip2[pixel / 4] = value;

		lds_r[GTid.x][GTid.y] = value.r;
		lds_g[GTid.x][GTid.y] = value.g;
		lds_b[GTid.x][GTid.y] = value.b;
	}

	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 8 == 0 && GTid.y % 8 == 0)
	{
		float4 value = (
			float4(lds_r[GTid.x + 0][GTid.y + 0], lds_g[GTid.x + 0][GTid.y + 0], lds_b[GTid.x + 0][GTid.y + 0], lds_a[GTid.x + 0][GTid.y + 0]) +
			float4(lds_r[GTid.x + 4][GTid.y + 0], lds_g[GTid.x + 4][GTid.y + 0], lds_b[GTid.x + 4][GTid.y + 0], lds_a[GTid.x + 4][GTid.y + 0]) +
			float4(lds_r[GTid.x + 0][GTid.y + 4], lds_g[GTid.x + 0][GTid.y + 4], lds_b[GTid.x + 0][GTid.y + 4], lds_a[GTid.x + 0][GTid.y + 4]) +
			float4(lds_r[GTid.x + 4][GTid.y + 4], lds_g[GTid.x + 4][GTid.y + 4], lds_b[GTid.x + 4][GTid.y + 4], lds_a[GTid.x + 4][GTid.y + 4])
			) / 4.0f;

		output_surfaceMap_mip3[pixel / 8] = value;
	}
#endif
}
