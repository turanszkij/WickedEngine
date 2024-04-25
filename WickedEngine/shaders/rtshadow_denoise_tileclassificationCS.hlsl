#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float3> normals : register(t0);
StructuredBuffer<uint4> tiles : register(t2);

RWStructuredBuffer<uint4> metadata : register(u0);

// per-light:
Texture2D<float3> moments_prev : register(t3);
Texture2D<float> history : register(t4);

RWTexture2D<float2> reprojection : register(u1);
RWTexture2D<float3> moments : register(u2);

int FFX_DNSR_Shadows_IsFirstFrame()
{
	return postprocess.params0.w == 0;
}
uint2 FFX_DNSR_Shadows_GetBufferDimensions()
{
	return postprocess.resolution;
}
float2 FFX_DNSR_Shadows_GetInvBufferDimensions()
{
	return postprocess.resolution_rcp;
}
float3 FFX_DNSR_Shadows_GetEye()
{
	return GetCamera().position;
}
float4x4 FFX_DNSR_Shadows_GetProjectionInverse()
{
	return GetCamera().inverse_projection;
}
float4x4 FFX_DNSR_Shadows_GetViewProjectionInverse()
{
	return GetCamera().inverse_view_projection;
}
float4x4 FFX_DNSR_Shadows_GetReprojectionMatrix()
{
	return GetCamera().reprojection;
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
	return normalize(normals[did] - 0.5);
}
uint FFX_DNSR_Shadows_ReadRaytracedShadowMask(uint linear_tile_index)
{
	return tiles[linear_tile_index][uint(rtshadow_denoise_lightindex)];
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
	return -texture_velocity[did * 2].xy;
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
	metadata[idx][uint(rtshadow_denoise_lightindex)] = mask;
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
