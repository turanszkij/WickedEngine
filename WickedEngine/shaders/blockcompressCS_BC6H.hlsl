#include "globals.hlsli"

#ifdef __PSSL__
// TODO: register spilling issue
#pragma warning (disable:7203)
#endif // __PSSL__

// Whether to use P2 modes (4 endpoints) for compression. Slow, but improves quality.
#ifdef COMPRESS_CUBEMAP
#define ENCODE_P2 0
#else
#define ENCODE_P2 1
#endif // COMPRESS_CUBEMAP

#include "bc6h.hlsli"

#ifdef COMPRESS_CUBEMAP
TextureCube SrcTexture : register(t0);
RWTexture2DArray<uint4> OutputTexture : register(u0);
#else
Texture2D SrcTexture : register(t0);
RWTexture2D<uint4> OutputTexture : register(u0);
#endif // COMPRESS_CUBEMAP


[numthreads(8, 8, 1)]
void main(uint3 groupID : SV_GroupID,
	uint3 dispatchThreadID : SV_DispatchThreadID,
	uint3 groupThreadID : SV_GroupThreadID)
{
	uint2 blockCoord = dispatchThreadID.xy;

	uint2 dim;
	SrcTexture.GetDimensions(dim.x, dim.y);
	dim = align(dim, 4);
	uint2 TextureSizeInBlocks = dim / 4;

	[branch]
	if (any(blockCoord >= TextureSizeInBlocks))
		return;

	float2 TextureSizeRcp = rcp(dim);

	if (all(blockCoord < TextureSizeInBlocks))
	{
		// Gather texels for current 4x4 block
		// 0 1 2 3
		// 4 5 6 7
		// 8 9 10 11
		// 12 13 14 15
		float2 uv = blockCoord * TextureSizeRcp * 4.0f + TextureSizeRcp;
#ifdef COMPRESS_CUBEMAP
		float3 block0UV = uv_to_cubemap(uv, dispatchThreadID.z);
		float3 block1UV = uv_to_cubemap(uv + float2(2.0f * TextureSizeRcp.x, 0.0f), dispatchThreadID.z);
		float3 block2UV = uv_to_cubemap(uv + float2(0.0f, 2.0f * TextureSizeRcp.y), dispatchThreadID.z);
		float3 block3UV = uv_to_cubemap(uv + float2(2.0f * TextureSizeRcp.x, 2.0f * TextureSizeRcp.y), dispatchThreadID.z);
#else
		float2 block0UV = uv;
		float2 block1UV = uv + float2(2.0f * TextureSizeRcp.x, 0.0f);
		float2 block2UV = uv + float2(0.0f, 2.0f * TextureSizeRcp.y);
		float2 block3UV = uv + float2(2.0f * TextureSizeRcp.x, 2.0f * TextureSizeRcp.y);
#endif // COMPRESS_CUBEMAP
		float4 block0X = SrcTexture.GatherRed(sampler_linear_clamp, block0UV);
		float4 block1X = SrcTexture.GatherRed(sampler_linear_clamp, block1UV);
		float4 block2X = SrcTexture.GatherRed(sampler_linear_clamp, block2UV);
		float4 block3X = SrcTexture.GatherRed(sampler_linear_clamp, block3UV);
		float4 block0Y = SrcTexture.GatherGreen(sampler_linear_clamp, block0UV);
		float4 block1Y = SrcTexture.GatherGreen(sampler_linear_clamp, block1UV);
		float4 block2Y = SrcTexture.GatherGreen(sampler_linear_clamp, block2UV);
		float4 block3Y = SrcTexture.GatherGreen(sampler_linear_clamp, block3UV);
		float4 block0Z = SrcTexture.GatherBlue(sampler_linear_clamp, block0UV);
		float4 block1Z = SrcTexture.GatherBlue(sampler_linear_clamp, block1UV);
		float4 block2Z = SrcTexture.GatherBlue(sampler_linear_clamp, block2UV);
		float4 block3Z = SrcTexture.GatherBlue(sampler_linear_clamp, block3UV);

		float3 texels[16];
		texels[0] = float3(block0X.w, block0Y.w, block0Z.w);
		texels[1] = float3(block0X.z, block0Y.z, block0Z.z);
		texels[2] = float3(block1X.w, block1Y.w, block1Z.w);
		texels[3] = float3(block1X.z, block1Y.z, block1Z.z);
		texels[4] = float3(block0X.x, block0Y.x, block0Z.x);
		texels[5] = float3(block0X.y, block0Y.y, block0Z.y);
		texels[6] = float3(block1X.x, block1Y.x, block1Z.x);
		texels[7] = float3(block1X.y, block1Y.y, block1Z.y);
		texels[8] = float3(block2X.w, block2Y.w, block2Z.w);
		texels[9] = float3(block2X.z, block2Y.z, block2Z.z);
		texels[10] = float3(block3X.w, block3Y.w, block3Z.w);
		texels[11] = float3(block3X.z, block3Y.z, block3Z.z);
		texels[12] = float3(block2X.x, block2Y.x, block2Z.x);
		texels[13] = float3(block2X.y, block2Y.y, block2Z.y);
		texels[14] = float3(block3X.x, block3Y.x, block3Z.x);
		texels[15] = float3(block3X.y, block3Y.y, block3Z.y);

#ifdef COMPRESS_CUBEMAP
		OutputTexture[dispatchThreadID] = CompressBC6H(texels);
#else
		OutputTexture[blockCoord] = CompressBC6H(texels);
#endif // COMPRESS_CUBEMAP
	}
}
