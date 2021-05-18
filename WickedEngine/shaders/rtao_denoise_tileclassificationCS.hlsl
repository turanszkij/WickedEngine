#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(normals, float3, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(tiles, uint, TEXSLOT_ONDEMAND1);
TEXTURE2D(moments_prev, float3, TEXSLOT_ONDEMAND2);
TEXTURE2D(history, float, TEXSLOT_ONDEMAND3);
TEXTURE2D(texture_depth_history, float, TEXSLOT_ONDEMAND4);

RWTEXTURE2D(reprojection, float2, 0);
RWTEXTURE2D(moments, float3, 1);
RWSTRUCTUREDBUFFER(metadata, uint, 2);

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
	return tiles[linear_tile_index];
}
float3 FFX_DNSR_Shadows_ReadPreviousMomentsBuffer(int2 history_pos)
{
	return moments_prev[history_pos];
}
float FFX_DNSR_Shadows_ReadHistory(float2 history_uv)
{
	return history.SampleLevel(sampler_linear_clamp, history_uv, 0);
}
float2 FFX_DNSR_Shadows_ReadVelocity(uint2 did)
{
	return -texture_gbuffer2[did * 2].xy;
}

void FFX_DNSR_Shadows_WriteReprojectionResults(uint2 did, float2 value)
{
	reprojection[did] = value;
}
void FFX_DNSR_Shadows_WriteMoments(uint2 did, float3 value)
{
	moments[did] = value;
}
void FFX_DNSR_Shadows_WriteMetadata(uint idx, uint mask)
{
	metadata[idx] = mask;
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
	FFX_DNSR_Shadows_TileClassification(groupIndex, Gid.xy);
}
