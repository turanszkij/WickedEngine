#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(traceResults, min16float3, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(metadata, uint, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(spatial, float3, 0);

groupshared uint g_shared_0[16][16];
groupshared uint g_shared_1[16][16];
groupshared uint g_shared_2[16][16];
groupshared uint g_shared_3[16][16];
groupshared float g_shared_depth[16][16];

min16float3 FFX_DNSR_Reflections_LoadRadianceFromGroupSharedMemory(int2 idx) {
	uint2 tmp;
	tmp.x = g_shared_0[idx.y][idx.x];
	tmp.y = g_shared_1[idx.y][idx.x];
	return min16float4(UnpackFloat16(tmp.x), UnpackFloat16(tmp.y)).xyz;
}

min16float3 FFX_DNSR_Reflections_LoadNormalFromGroupSharedMemory(int2 idx) {
	uint2 tmp;
	tmp.x = g_shared_2[idx.y][idx.x];
	tmp.y = g_shared_3[idx.y][idx.x];
	return min16float4(UnpackFloat16(tmp.x), UnpackFloat16(tmp.y)).xyz;
}

float FFX_DNSR_Reflections_LoadDepthFromGroupSharedMemory(int2 idx) {
	return g_shared_depth[idx.y][idx.x];
}

void FFX_DNSR_Reflections_StoreInGroupSharedMemory(int2 idx, min16float3 radiance, min16float3 normal, float depth) {
	g_shared_0[idx.y][idx.x] = PackFloat16(radiance.xy);
	g_shared_1[idx.y][idx.x] = PackFloat16(min16float2(radiance.z, 0));
	g_shared_2[idx.y][idx.x] = PackFloat16(normal.xy);
	g_shared_3[idx.y][idx.x] = PackFloat16(min16float2(normal.z, 0));
	g_shared_depth[idx.y][idx.x] = depth;
}

float FFX_DNSR_Reflections_LoadRoughness(int2 pixel_coordinate) {
	return texture_gbuffer1.Load(int3(pixel_coordinate, 0)).a;
}

min16float3 FFX_DNSR_Reflections_LoadRadianceFP16(int2 pixel_coordinate) {
	return traceResults.Load(int3(pixel_coordinate, 0)).xyz;
}

min16float3 FFX_DNSR_Reflections_LoadNormalFP16(int2 pixel_coordinate) {
	return (min16float3) (2 * texture_gbuffer1.Load(int3(pixel_coordinate, 0)).xyz - 1);
}

float FFX_DNSR_Reflections_LoadDepth(int2 pixel_coordinate) {
	return texture_depth.Load(int3(pixel_coordinate, 0));
}

void FFX_DNSR_Reflections_StoreSpatiallyDenoisedReflections(int2 pixel_coordinate, min16float3 value) {
	spatial[pixel_coordinate] = value;
}

uint FFX_DNSR_Reflections_LoadTileMetaDataMask(uint index) {
	return metadata[index];
}

#include "ffx-reflection-dnsr/ffx_denoiser_reflections_resolve_spatial.h"

[numthreads(8, 8, 1)]
void main(uint group_index : SV_GroupIndex, uint2 group_id : SV_GroupID) {
	uint2 screen_dimensions = xPPResolution;

	uint2 group_thread_id = FFX_DNSR_Reflections_RemapLane8x8(group_index);
	uint2 dispatch_thread_id = group_id * 8 + group_thread_id;
	//spatial[dispatch_thread_id] = traceResults[dispatch_thread_id];
	//return;
	FFX_DNSR_Reflections_ResolveSpatial((int2)dispatch_thread_id, (int2)group_thread_id, g_samples_per_quad, screen_dimensions);
}
