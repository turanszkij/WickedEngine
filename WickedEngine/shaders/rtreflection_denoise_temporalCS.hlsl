#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

static const float g_temporal_stability_factor = 0.99f;
static const float g_temporal_variance_threshold = 0.002f;

TEXTURE2D(temporal_history, float3, TEXSLOT_ONDEMAND0);
TEXTURE2D(rayLengths, float, TEXSLOT_ONDEMAND1);
TEXTURE2D(spatial, float3, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(metadata, uint, TEXSLOT_ONDEMAND3);

RWTEXTURE2D(temporal_output, float3, 0);
RWSTRUCTUREDBUFFER(temporalVarianceMask, uint, 1);

float FFX_DNSR_Reflections_LoadRayLength(int2 pixel_coordinate) {
	return rayLengths.Load(int3(pixel_coordinate, 0));
}

float2 FFX_DNSR_Reflections_LoadMotionVector(int2 pixel_coordinate) {
	return -texture_gbuffer2.Load(int3(pixel_coordinate, 0)).xy;
}

float3 FFX_DNSR_Reflections_LoadNormal(int2 pixel_coordinate) {
	return 2 * texture_gbuffer1.Load(int3(pixel_coordinate, 0)).xyz - 1;
}

float3 FFX_DNSR_Reflections_LoadNormalHistory(int2 pixel_coordinate) {
	return 2 * texture_gbuffer1.Load(int3(pixel_coordinate, 0)).xyz - 1;
}

float FFX_DNSR_Reflections_LoadRoughness(int2 pixel_coordinate) {
	return texture_gbuffer1.Load(int3(pixel_coordinate, 0)).a;
}

float FFX_DNSR_Reflections_LoadRoughnessHistory(int2 pixel_coordinate) {
	return texture_gbuffer1.Load(int3(pixel_coordinate, 0)).a;
}

float3 FFX_DNSR_Reflections_LoadRadianceHistory(int2 pixel_coordinate) {
	return temporal_history.Load(int3(pixel_coordinate, 0)).xyz;
}

float FFX_DNSR_Reflections_LoadDepth(int2 pixel_coordinate) {
	return texture_depth.Load(int3(pixel_coordinate, 0));
}

float3 FFX_DNSR_Reflections_LoadSpatiallyDenoisedReflections(int2 pixel_coordinate) {
	return spatial.Load(int3(pixel_coordinate, 0)).xyz;
}

uint FFX_DNSR_Reflections_LoadTileMetaDataMask(uint index) {
	return metadata[index];
}

void FFX_DNSR_Reflections_StoreTemporallyDenoisedReflections(int2 pixel_coordinate, float3 value) {
	temporal_output[pixel_coordinate] = value;
}

void FFX_DNSR_Reflections_StoreTemporalVarianceMask(int index, uint mask) {
	temporalVarianceMask[index] = mask;
}

#include "ffx-reflection-dnsr/ffx_denoiser_reflections_resolve_temporal.h"

[numthreads(8, 8, 1)]
void main(int2 dispatch_thread_id : SV_DispatchThreadID, int2 group_thread_id : SV_GroupThreadID) {
	//temporal_output[dispatch_thread_id] = spatial[dispatch_thread_id];
	//return;

	uint2 image_size = xPPResolution;
	FFX_DNSR_Reflections_ResolveTemporal(dispatch_thread_id, group_thread_id, image_size, g_temporal_stability_factor, g_temporal_variance_threshold);
}
