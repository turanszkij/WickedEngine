#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(normals, float3, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_depth_history, float, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(tiles, uint4, TEXSLOT_ONDEMAND2);

RWSTRUCTUREDBUFFER(metadata, uint4, 0);

// per-light:
TEXTURE2D(moments_prev[4], float3, TEXSLOT_ONDEMAND3);
TEXTURE2D(history[4], float, TEXSLOT_ONDEMAND7);

RWTEXTURE2D(reprojection[4], float2, 1);
RWTEXTURE2D(moments[4], float3, 5);

groupshared uint light_index;

int FFX_DNSR_Shadows_IsFirstFrame()
{
	return xPPParams0.w == 0;
}
uint2 FFX_DNSR_Shadows_GetBufferDimensions()
{
	return xPPResolution;
}
float2 FFX_DNSR_Shadows_GetInvBufferDimensions()
{
	return xPPResolution_rcp;
}
float3 FFX_DNSR_Shadows_GetEye()
{
	return g_xCamera_CamPos;
}
float4x4 FFX_DNSR_Shadows_GetProjectionInverse()
{
	return g_xCamera_InvP;
}
float4x4 FFX_DNSR_Shadows_GetViewProjectionInverse()
{
	return g_xCamera_InvVP;
}
float4x4 FFX_DNSR_Shadows_GetReprojectionMatrix()
{
	return g_xCamera_Reprojection;
}

float FFX_DNSR_Shadows_ReadDepth(uint2 did)
{
	return texture_depth[did * 2];
}
float FFX_DNSR_Shadows_ReadPreviousDepth(int2 idx)
{
	return texture_depth_history[idx * 2];
}
float3 FFX_DNSR_Shadows_ReadNormals(uint2 did)
{
	return normalize(normals[did] * 2 - 1);
}
uint FFX_DNSR_Shadows_ReadRaytracedShadowMask(uint linear_tile_index)
{
	return tiles[linear_tile_index][light_index];
}
float3 FFX_DNSR_Shadows_ReadPreviousMomentsBuffer(int2 history_pos)
{
	return moments_prev[light_index][history_pos];
}
float FFX_DNSR_Shadows_ReadHistory(float2 history_uv)
{
	return history[light_index].SampleLevel(sampler_linear_clamp, history_uv, 0);
}
float2 FFX_DNSR_Shadows_ReadVelocity(uint2 did)
{
	return -texture_gbuffer2[did * 2].xy;
}

void FFX_DNSR_Shadows_WriteReprojectionResults(uint2 did, float2 value)
{
	reprojection[light_index][did] = value;
}
void FFX_DNSR_Shadows_WriteMoments(uint2 did, float3 value)
{
	moments[light_index][did] = value;
}
void FFX_DNSR_Shadows_WriteMetadata(uint idx, uint mask)
{
	metadata[idx][light_index] = mask;
}

bool FFX_DNSR_Shadows_IsShadowReciever(uint2 did)
{
	float depth = FFX_DNSR_Shadows_ReadDepth(did);
	return (depth > 0.0f) && (depth < 1.0f);
}


#define INVERTED_DEPTH_RANGE
#include "ffx-shadows-dnsr/ffx_denoiser_shadows_tileclassification.h"

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	light_index = Gid.z;
	GroupMemoryBarrierWithGroupSync();

	FFX_DNSR_Shadows_TileClassification(groupIndex, Gid.xy);
}
