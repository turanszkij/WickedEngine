#ifndef WI_SHADERINTEROP_POSTPROCESS_H
#define WI_SHADERINTEROP_POSTPROCESS_H
#include "ShaderInterop.h"

static const uint POSTPROCESS_BLOCKSIZE = 8;
static const uint POSTPROCESS_LINEARDEPTH_BLOCKSIZE = 16;
static const uint POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT = 256;

struct PostProcess
{
	uint2 resolution;
	float2 resolution_rcp;
	float4 params0;
	float4 params1;
};

#define luminance_adaptionrate postprocess.params0.x

#define lineardepth_inputresolution postprocess.params0.xy
#define lineardepth_inputresolution_rcp postprocess.params0.zw

#define ssr_input_maxmip postprocess.params0.x
#define ssr_input_resolution_max postprocess.params0.y

#define ssao_range postprocess.params0.x
#define ssao_samplecount postprocess.params0.y
#define ssao_power postprocess.params0.z

#define rtao_range ssao_range
#define rtao_power ssao_power

#define rtreflection_range ssao_range

static const uint POSTPROCESS_HBAO_THREADCOUNT = 320;
#define hbao_direction postprocess.params0.xy
#define hbao_power postprocess.params0.z
#define hbao_uv_to_view_A postprocess.params1.xy
#define hbao_uv_to_view_B postprocess.params1.zw

#define sss_step postprocess.params0

static const uint POSTPROCESS_MSAO_BLOCKSIZE = 16;
struct MSAO
{
	float4 xInvThicknessTable[3];
	float4 xSampleWeightTable[3];
	float2 xInvSliceDimension;
	float xRejectFadeoff;
	float xRcpAccentuation;
};
//#define MSAO_SAMPLE_EXHAUSTIVELY
struct MSAO_UPSAMPLE
{
	float2 InvLowResolution;
	float2 InvHighResolution;
	float NoiseFilterStrength;
	float StepSize;
	float kBlurTolerance;
	float kUpsampleTolerance;
};

struct ShadingRateClassification
{
	uint TileSize;
	uint SHADING_RATE_1X1;
	uint SHADING_RATE_1X2;
	uint SHADING_RATE_2X1;

	uint SHADING_RATE_2X2;
	uint SHADING_RATE_2X4;
	uint SHADING_RATE_4X2;
	uint SHADING_RATE_4X4;
};

struct FSR
{
	uint4 Const0;
	uint4 Const1;
	uint4 Const2;
	uint4 Const3;
};

static const uint MOTIONBLUR_TILESIZE = 32;
#define motionblur_strength postprocess.params0.x

static const uint DEPTHOFFIELD_TILESIZE = 32;
#define dof_cocscale postprocess.params0.x
#define dof_maxcoc postprocess.params0.y

struct PushConstantsTonemap
{
	float2 resolution_rcp;
	float exposure;
	float dither;
	float eyeadaptionkey;
	int texture_input;
	int texture_input_luminance;
	int texture_input_distortion;
	int texture_colorgrade_lookuptable;
	int texture_output;
};

static const uint TILE_STATISTICS_OFFSET_EARLYEXIT = 0;
static const uint TILE_STATISTICS_OFFSET_CHEAP = TILE_STATISTICS_OFFSET_EARLYEXIT + 4;
static const uint TILE_STATISTICS_OFFSET_EXPENSIVE = TILE_STATISTICS_OFFSET_CHEAP + 4;
static const uint INDIRECT_OFFSET_EARLYEXIT = TILE_STATISTICS_OFFSET_EXPENSIVE + 4;
static const uint INDIRECT_OFFSET_CHEAP = INDIRECT_OFFSET_EARLYEXIT + 4 * 3;
static const uint INDIRECT_OFFSET_EXPENSIVE = INDIRECT_OFFSET_CHEAP + 4 * 3;
static const uint TILE_STATISTICS_CAPACITY = INDIRECT_OFFSET_EXPENSIVE + 4 * 3;


#endif // WI_SHADERINTEROP_POSTPROCESS_H
