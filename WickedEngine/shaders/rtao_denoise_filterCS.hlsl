#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

#define float16_t2 min16float2
#define float16_t3 min16float3

Texture2D<float3> normals : register(t0);
StructuredBuffer<uint> metadata : register(t1);
Texture2D<float2> input : register(t2);

RWTexture2D<float2> history : register(u0);
RWTexture2D<unorm float> output : register(u1);

uint2 FFX_DNSR_Shadows_GetBufferDimensions()
{
	return postprocess.resolution;
}
float2 FFX_DNSR_Shadows_GetInvBufferDimensions()
{
	return postprocess.resolution_rcp;
}
float4x4 FFX_DNSR_Shadows_GetProjectionInverse()
{
	return GetCamera().inverse_projection;
}

float FFX_DNSR_Shadows_GetDepthSimilaritySigma()
{
	return 1.0f;
}

float FFX_DNSR_Shadows_ReadDepth(uint2 did)
{
	return texture_depth[did * 2];
}
float16_t3 FFX_DNSR_Shadows_ReadNormals(uint2 did)
{
	return normalize((float16_t3)normals[did] * 2 - 1);
}

bool FFX_DNSR_Shadows_IsShadowReciever(uint2 did)
{
	float depth = FFX_DNSR_Shadows_ReadDepth(did);
	return (depth > 0.0f) && (depth < 1.0f);
}

float16_t2 FFX_DNSR_Shadows_ReadInput(int2 p)
{
	return (float16_t2)input[p].xy;
}

uint FFX_DNSR_Shadows_ReadTileMetaData(uint p)
{
	return metadata[p];
}

#include "ffx-shadows-dnsr/ffx_denoiser_shadows_filter.h"

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint2 gid : SV_GroupID, uint2 gtid : SV_GroupThreadID, uint2 did : SV_DispatchThreadID)
{
	const uint PASS_INDEX = (uint)postprocess.params1.x;
	const uint STEP_SIZE = (uint)postprocess.params1.y;

	bool bWriteOutput = false;
	float2 const results = FFX_DNSR_Shadows_FilterSoftShadowsPass(gid, gtid, did, bWriteOutput, PASS_INDEX, STEP_SIZE);

	if (PASS_INDEX < 2)
	{
		if (bWriteOutput)
		{
			history[did] = results;
		}
	}
	else
	{
		// final pass:
		// Recover some of the contrast lost during denoising
		const float shadow_remap = max(1.2f - results.y, 1.0f);
		const float mean = saturate(pow(abs(results.x), shadow_remap));

		if (bWriteOutput)
		{
			output[did].x = FFX_DNSR_Shadows_IsShadowReciever(did) ? pow(mean, rtao_power) : 1;
		}
	}
}
