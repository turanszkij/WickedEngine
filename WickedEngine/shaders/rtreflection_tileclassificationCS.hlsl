#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

STRUCTUREDBUFFER(temporalVarianceMask, uint, TEXSLOT_ONDEMAND0);

RWSTRUCTUREDBUFFER(rayList, uint, 0);
globallycoherent RWSTRUCTUREDBUFFER(rayCounter, uint, 1);
RWTEXTURE2D(traceResults, float4, 2);
RWSTRUCTUREDBUFFER(metadata, uint, 3);

uint FFX_DNSR_Reflections_LoadTemporalVarianceMask(uint index) {
	return temporalVarianceMask[index];
}

void FFX_DNSR_Reflections_IncrementRayCounter(uint value, out uint original_value) {
	InterlockedAdd(rayCounter[0], value, original_value);
}

void FFX_DNSR_Reflections_StoreRay(int index, uint2 ray_coord, bool copy_horizontal, bool copy_vertical, bool copy_diagonal) {
	rayList[index] = PackRayCoords(ray_coord, copy_horizontal, copy_vertical, copy_diagonal); // Store out pixel to trace
}

void FFX_DNSR_Reflections_StoreTileMetaDataMask(uint index, uint mask) {
	metadata[index] = mask;
}

#include "ffx-reflection-dnsr/ffx_denoiser_reflections_classify_tiles.h"

[numthreads(8, 8, 1)]
void main(uint2 group_id : SV_GroupID, uint group_index : SV_GroupIndex)
{
	uint2 screen_size = xPPResolution;

	uint2 group_thread_id = FFX_DNSR_Reflections_RemapLane8x8(group_index); // Remap lanes to ensure four neighboring lanes are arranged in a quad pattern
	uint2 dispatch_thread_id = group_id * 8 + group_thread_id;

#if 1
	const float2 uv = (dispatch_thread_id.xy + 0.5f) * xPPResolution_rcp;
	const float2 velocity = texture_gbuffer2.SampleLevel(sampler_point_clamp, uv, 0).xy;
	const float2 prevUV = uv + velocity;
	const float4 g1 = texture_gbuffer1.SampleLevel(sampler_linear_clamp, prevUV, 0);
	float roughness = g1.a;
#else
	float roughness = texture_gbuffer1[dispatch_thread_id * 2].a;
#endif

	FFX_DNSR_Reflections_ClassifyTiles(dispatch_thread_id, group_thread_id, roughness, screen_size, g_samples_per_quad, g_temporal_variance_guided_tracing_enabled);

	// Clear intersection results as there wont be any ray that overwrites them
	traceResults[dispatch_thread_id] = 0;
}
