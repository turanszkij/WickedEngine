#include "globals.hlsli"
#include "BlockCompress.hlsli"

#if !defined(BC3) && !defined(BC5)
#define BC1
#endif // !BC3 && !BC5

Texture2D input : register(t0);

#ifdef BC1
RWTexture2D<uint2> output : register(u0);
#endif // BC1

#ifdef BC3
RWTexture2D<uint4> output : register(u0);
#endif // BC3

#ifdef BC5
RWTexture2D<uint4> output : register(u0);
#endif // BC5

#if 0
// Dithering is to fix bad-looking gradients in BC1 and BC3 RGB compression:
#define DITHER(color) (color + (dither((float2)DTid.xy) - 0.5f) / 64.0f)
#else
#define DITHER(color) (color)
#endif

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
#ifdef BC1
	float3 block[16];
#endif // BC1

#ifdef BC3
	float3 block[16];
	float block_a[16];
#endif // BC3

#ifdef BC5
	float block_u[16];
	float block_v[16];
#endif // BC5

	uint2 dim;
	input.GetDimensions(dim.x, dim.y);
	const float2 dim_rcp = rcp(dim);
	const float2 uv = float2(DTid.xy * 4 + 1) * dim_rcp;

	//SUB-BLOCK///////////////////////////////////////////////////////////////////////

	float4 red = input.GatherRed(sampler_linear_clamp, uv, int2(0, 0));
	float4 green = input.GatherGreen(sampler_linear_clamp, uv, int2(0, 0));
	float4 blue = input.GatherBlue(sampler_linear_clamp, uv, int2(0, 0));
	float4 alpha = input.GatherAlpha(sampler_linear_clamp, uv, int2(0, 0));

#if defined(BC1) || defined(BC3)
	block[0] = DITHER(float3(red[3], green[3], blue[3]));
	block[1] = DITHER(float3(red[2], green[2], blue[2]));
	block[4] = DITHER(float3(red[0], green[0], blue[0]));
	block[5] = DITHER(float3(red[1], green[1], blue[1]));
#endif // BC1 || BC3

#ifdef BC3
	block_a[0] = alpha[3];
	block_a[1] = alpha[2];
	block_a[4] = alpha[0];
	block_a[5] = alpha[1];
#endif // BC3

#ifdef BC5
	block_u[0] = red[3];
	block_u[1] = red[2];
	block_u[4] = red[0];
	block_u[5] = red[1];
	block_v[0] = green[3];
	block_v[1] = green[2];
	block_v[4] = green[0];
	block_v[5] = green[1];
#endif // BC5

	//SUB-BLOCK///////////////////////////////////////////////////////////////////////

	red = input.GatherRed(sampler_linear_clamp, uv, int2(2, 0));
	green = input.GatherGreen(sampler_linear_clamp, uv, int2(2, 0));
	blue = input.GatherBlue(sampler_linear_clamp, uv, int2(2, 0));
	alpha = input.GatherAlpha(sampler_linear_clamp, uv, int2(2, 0));

#if defined(BC1) || defined(BC3)
	block[2] = DITHER(float3(red[3], green[3], blue[3]));
	block[3] = DITHER(float3(red[2], green[2], blue[2]));
	block[6] = DITHER(float3(red[0], green[0], blue[0]));
	block[7] = DITHER(float3(red[1], green[1], blue[1]));
#endif // BC1 || BC3

#ifdef BC3
	block_a[2] = alpha[3];
	block_a[3] = alpha[2];
	block_a[6] = alpha[0];
	block_a[7] = alpha[1];
#endif // BC3

#ifdef BC5
	block_u[2] = red[3];
	block_u[3] = red[2];
	block_u[6] = red[0];
	block_u[7] = red[1];
	block_v[2] = green[3];
	block_v[3] = green[2];
	block_v[6] = green[0];
	block_v[7] = green[1];
#endif // BC5

	//SUB-BLOCK///////////////////////////////////////////////////////////////////////

	red = input.GatherRed(sampler_linear_clamp, uv, int2(0, 2));
	green = input.GatherGreen(sampler_linear_clamp, uv, int2(0, 2));
	blue = input.GatherBlue(sampler_linear_clamp, uv, int2(0, 2));
	alpha = input.GatherAlpha(sampler_linear_clamp, uv, int2(0, 2));

#if defined(BC1) || defined(BC3)
	block[8] = DITHER(float3(red[3], green[3], blue[3]));
	block[9] = DITHER(float3(red[2], green[2], blue[2]));
	block[12] = DITHER(float3(red[0], green[0], blue[0]));
	block[13] = DITHER(float3(red[1], green[1], blue[1]));
#endif // BC1 || BC3

#ifdef BC3
	block_a[8] = alpha[3];
	block_a[9] = alpha[2];
	block_a[12] = alpha[0];
	block_a[13] = alpha[1];
#endif // BC3

#ifdef BC5
	block_u[8] = red[3];
	block_u[9] = red[2];
	block_u[12] = red[0];
	block_u[13] = red[1];
	block_v[8] = green[3];
	block_v[9] = green[2];
	block_v[12] = green[0];
	block_v[13] = green[1];
#endif // BC5

	//SUB-BLOCK///////////////////////////////////////////////////////////////////////

	red = input.GatherRed(sampler_linear_clamp, uv, int2(2, 2));
	green = input.GatherGreen(sampler_linear_clamp, uv, int2(2, 2));
	blue = input.GatherBlue(sampler_linear_clamp, uv, int2(2, 2));
	alpha = input.GatherAlpha(sampler_linear_clamp, uv, int2(2, 2));

#if defined(BC1) || defined(BC3)
	block[10] = DITHER(float3(red[3], green[3], blue[3]));
	block[11] = DITHER(float3(red[2], green[2], blue[2]));
	block[14] = DITHER(float3(red[0], green[0], blue[0]));
	block[15] = DITHER(float3(red[1], green[1], blue[1]));
#endif // BC1 || BC3

#ifdef BC3
	block_a[10] = alpha[3];
	block_a[11] = alpha[2];
	block_a[14] = alpha[0];
	block_a[15] = alpha[1];
#endif // BC3

#ifdef BC5
	block_u[10] = red[3];
	block_u[11] = red[2];
	block_u[14] = red[0];
	block_u[15] = red[1];
	block_v[10] = green[3];
	block_v[11] = green[2];
	block_v[14] = green[0];
	block_v[15] = green[1];
#endif // BC5

	//COMPRESS-WRITE///////////////////////////////////////////////////////////////////

#ifdef BC1
	output[DTid.xy] = CompressBC1Block(block);
#endif // BC1

#ifdef BC3
	output[DTid.xy] = CompressBC3Block(block, block_a);
#endif // BC3

#ifdef BC5
	output[DTid.xy] = CompressBC5Block(block_u, block_v);
#endif // BC5
}
