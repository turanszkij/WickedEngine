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

static const uint LUMINANCE_BLOCKSIZE = 32;
static const uint LUMINANCE_NUM_HISTOGRAM_BINS = LUMINANCE_BLOCKSIZE * LUMINANCE_BLOCKSIZE;
static const uint LUMINANCE_BUFFER_OFFSET_EXPOSURE = 0;
static const uint LUMINANCE_BUFFER_OFFSET_LUMINANCE = LUMINANCE_BUFFER_OFFSET_EXPOSURE + 4;
static const uint LUMINANCE_BUFFER_OFFSET_HISTOGRAM = LUMINANCE_BUFFER_OFFSET_LUMINANCE + 4;
#define luminance_adaptionrate postprocess.params0.x
#define luminance_log_min postprocess.params0.y
#define luminance_log_max postprocess.params0.z
#define luminance_log_range postprocess.params0.w
#define luminance_log_range_rcp postprocess.params1.x
#define luminance_pixelcount postprocess.params1.y
#define luminance_eyeadaptionkey postprocess.params1.z

struct Bloom
{
	float2 resolution_rcp;
	float threshold;
	float exposure;
	int texture_input;
	int texture_output;
	int buffer_input_luminance;
};

#define lineardepth_inputresolution postprocess.params0.xy
#define lineardepth_inputresolution_rcp postprocess.params0.zw

static const uint SSR_TILESIZE = 32;
#define ssr_roughness_cutoff postprocess.params0.z
#define ssr_frame postprocess.params0.w

#define ssao_range postprocess.params0.x
#define ssao_samplecount postprocess.params0.y
#define ssao_power postprocess.params0.z

#define rtao_range ssao_range
#define rtao_power ssao_power

#define rtshadow_denoise_lightindex postprocess.params0.y

#define rtdiffuse_range ssao_range
#define rtdiffuse_frame ssr_frame
#define ssgi_frame ssr_frame

#define rtreflection_range ssao_range
#define rtreflection_roughness_cutoff ssr_roughness_cutoff
#define rtreflection_frame ssr_frame

static const uint POSTPROCESS_HBAO_THREADCOUNT = 320;
#define hbao_direction postprocess.params0.xy
#define hbao_power postprocess.params0.z
#define hbao_uv_to_view_A postprocess.params1.xy
#define hbao_uv_to_view_B postprocess.params1.zw

#define volumetricclouds_frame postprocess.params0.z

static const uint POSTPROCESS_MSAO_BLOCKSIZE = 16;
struct MSAO
{
	float4 xInvThicknessTable[3];
	float4 xSampleWeightTable[3];
	float2 xInvSliceDimension;
	float xRejectFadeoff;
	float xRcpAccentuation;
};
CONSTANTBUFFER(msao, MSAO, CBSLOT_MSAO);

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
CONSTANTBUFFER(fsr, FSR, CBSLOT_FSR);

static const uint MOTIONBLUR_TILESIZE = 32;
#define motionblur_strength postprocess.params0.x

static const uint DEPTHOFFIELD_TILESIZE = 32;
#define dof_cocscale postprocess.params0.x
#define dof_maxcoc postprocess.params0.y

static const uint TONEMAP_FLAG_DITHER = 1 << 0;
static const uint TONEMAP_FLAG_ACES = 1 << 1;
struct PushConstantsTonemap
{
	float2 resolution_rcp;
	uint2 exposure_brightness_contrast_saturation;

	int texture_input;
	int buffer_input_luminance;
	int texture_input_distortion;
	int texture_colorgrade_lookuptable;

	int texture_bloom;
	int texture_output;
	uint display_colorspace;
	uint flags;
};

struct PostprocessTileStatistics
{
	IndirectDispatchArgs dispatch_earlyexit;
	IndirectDispatchArgs dispatch_cheap;
	IndirectDispatchArgs dispatch_expensive;
};


#endif // WI_SHADERINTEROP_POSTPROCESS_H
