#ifndef WI_SHADERINTEROP_POSTPROCESS_H
#define WI_SHADERINTEROP_POSTPROCESS_H
#include "ShaderInterop.h"

static const uint POSTPROCESS_BLOCKSIZE = 8;
static const uint POSTPROCESS_LINEARDEPTH_BLOCKSIZE = 16;
static const uint POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT = 256;

CBUFFER(PostProcessCB, CBSLOT_RENDERER_POSTPROCESS)
{
	uint2 xPPResolution;
	float2 xPPResolution_rcp;
	float4 xPPParams0;
	float4 xPPParams1;
};

#define lineardepth_inputresolution xPPParams0.xy
#define lineardepth_inputresolution_rcp xPPParams0.zw

#define ssr_input_maxmip xPPParams0.x
#define ssr_input_resolution_max xPPParams0.y

#define ssao_range xPPParams0.x
#define ssao_samplecount xPPParams0.y
#define ssao_power xPPParams0.z

#define rtao_range ssao_range
#define rtao_samplecount ssao_samplecount
#define rtao_power ssao_power
#define rtao_seed xPPParams0.w

#define rtreflection_range ssao_range
#define rtreflection_seed xPPParams0.w

static const uint POSTPROCESS_HBAO_THREADCOUNT = 320;
#define hbao_direction xPPParams0.xy
#define hbao_power xPPParams0.z
#define hbao_uv_to_view_A xPPParams1.xy
#define hbao_uv_to_view_B xPPParams1.zw

static const uint POSTPROCESS_MSAO_BLOCKSIZE = 16;
CBUFFER(MSAOCB, CBSLOT_RENDERER_POSTPROCESS)
{
	float4 xInvThicknessTable[3];
	float4 xSampleWeightTable[3];
	float2 xInvSliceDimension;
	float xRejectFadeoff;
	float xRcpAccentuation;
};
//#define MSAO_SAMPLE_EXHAUSTIVELY
CBUFFER(MSAO_UPSAMPLECB, CBSLOT_RENDERER_POSTPROCESS)
{
	float2 InvLowResolution;
	float2 InvHighResolution;
	float NoiseFilterStrength;
	float StepSize;
	float kBlurTolerance;
	float kUpsampleTolerance;
};

CBUFFER(ShadingRateClassificationCB, CBSLOT_RENDERER_POSTPROCESS)
{
	uint xShadingRateTileSize;
	uint SHADING_RATE_1X1;
	uint SHADING_RATE_1X2;
	uint SHADING_RATE_2X1;

	uint SHADING_RATE_2X2;
	uint SHADING_RATE_2X4;
	uint SHADING_RATE_4X2;
	uint SHADING_RATE_4X4;
};

static const uint MOTIONBLUR_TILESIZE = 32;
#define motionblur_strength xPPParams0.x

static const uint DEPTHOFFIELD_TILESIZE = 32;
#define dof_focus xPPParams0.x
#define dof_scale xPPParams0.y
#define dof_aspect xPPParams0.z
#define dof_maxcoc xPPParams0.w

#define tonemap_exposure xPPParams0.x
#define tonemap_dither xPPParams0.y
#define tonemap_colorgrading xPPParams0.z

static const uint TILE_STATISTICS_OFFSET_EARLYEXIT = 0;
static const uint TILE_STATISTICS_OFFSET_CHEAP = TILE_STATISTICS_OFFSET_EARLYEXIT + 4;
static const uint TILE_STATISTICS_OFFSET_EXPENSIVE = TILE_STATISTICS_OFFSET_CHEAP + 4;
static const uint INDIRECT_OFFSET_EARLYEXIT = TILE_STATISTICS_OFFSET_EXPENSIVE + 4;
static const uint INDIRECT_OFFSET_CHEAP = INDIRECT_OFFSET_EARLYEXIT + 4 * 3;
static const uint INDIRECT_OFFSET_EXPENSIVE = INDIRECT_OFFSET_CHEAP + 4 * 3;
static const uint TILE_STATISTICS_CAPACITY = INDIRECT_OFFSET_EXPENSIVE + 4 * 3;

#endif // WI_SHADERINTEROP_POSTPROCESS_H
