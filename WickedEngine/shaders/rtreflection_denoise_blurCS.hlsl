#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(temporal_output, min16float3, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(metadata, uint, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, float4, 0);

groupshared uint g_shared_0[12][12];
groupshared uint g_shared_1[12][12];

void FFX_DNSR_Reflections_LoadFromGroupSharedMemory(int2 idx, out min16float3 radiance, out min16float roughness) {
	uint2 tmp;
	tmp.x = g_shared_0[idx.x][idx.y];
	tmp.y = g_shared_1[idx.x][idx.y];

	min16float4 min16tmp = min16float4(UnpackFloat16(tmp.x), UnpackFloat16(tmp.y));
	radiance = min16tmp.xyz;
	roughness = min16tmp.w;
}

void FFX_DNSR_Reflections_StoreInGroupSharedMemory(int2 idx, min16float3 radiance, min16float roughness) {
	min16float4 tmp = min16float4(radiance, roughness);
	g_shared_0[idx.x][idx.y] = PackFloat16(tmp.xy);
	g_shared_1[idx.x][idx.y] = PackFloat16(tmp.zw);
}

min16float3 FFX_DNSR_Reflections_LoadRadianceFP16(int2 pixel_coordinate) {
	return temporal_output.Load(int3(pixel_coordinate, 0)).xyz;
}

min16float FFX_DNSR_Reflections_LoadRoughnessFP16(int2 pixel_coordinate) {
	return (min16float)texture_gbuffer1.Load(int3(pixel_coordinate * 2, 0)).a;
}

void FFX_DNSR_Reflections_StoreDenoisedReflectionResult(int2 pixel_coordinate, min16float3 value) {
	output[pixel_coordinate] = float4(value, 1);
}

uint FFX_DNSR_Reflections_LoadTileMetaDataMask(uint index) {
	return metadata[index];
}

#include "ffx-reflection-dnsr/ffx_denoiser_reflections_blur.h"

[numthreads(8, 8, 1)]
void main(int2 dispatch_thread_id : SV_DispatchThreadID, int2 group_thread_id : SV_GroupThreadID) {
	//output[dispatch_thread_id] = float4(temporal_output[dispatch_thread_id], 1);
	//return;

	uint2 screen_dimensions = xPPResolution;
	FFX_DNSR_Reflections_Blur(dispatch_thread_id, group_thread_id, screen_dimensions);
}
