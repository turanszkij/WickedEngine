#ifndef WI_SHADERINTEROP_POSTPROCESS_H
#define WI_SHADERINTEROP_POSTPROCESS_H
#include "ShaderInterop.h"

static const uint POSTPROCESS_BLOCKSIZE = 8;
static const uint POSTPROCESS_LINEARDEPTH_BLOCKSIZE = 16;

CBUFFER(PostProcessCB, CBSLOT_RENDERER_POSTPROCESS)
{
	uint2		xPPResolution;
	float2		xPPResolution_rcp;
	float4		xPPParams0;
	float4		xPPParams1;
};

#define lineardepth_inputresolution xPPParams0.xy
#define lineardepth_inputresolution_rcp xPPParams0.zw

#define ssao_range xPPParams0.x
#define ssao_samplecount xPPParams0.y
#define ssao_power xPPParams0.z

static const uint MOTIONBLUR_TILESIZE = 32;
#define motionblur_strength xPPParams0.x

static const uint DEPTHOFFIELD_TILESIZE = 32;
#define dof_focus xPPParams0.x
#define dof_scale xPPParams0.y
#define dof_aspect xPPParams0.z
#define dof_maxcoc xPPParams0.w

static const uint TILE_STATISTICS_OFFSET_EARLYEXIT = 0;
static const uint TILE_STATISTICS_OFFSET_CHEAP = TILE_STATISTICS_OFFSET_EARLYEXIT + 4;
static const uint TILE_STATISTICS_OFFSET_EXPENSIVE = TILE_STATISTICS_OFFSET_CHEAP + 4;
static const uint INDIRECT_OFFSET_EARLYEXIT = TILE_STATISTICS_OFFSET_EXPENSIVE + 4;
static const uint INDIRECT_OFFSET_CHEAP = INDIRECT_OFFSET_EARLYEXIT + 4 * 3;
static const uint INDIRECT_OFFSET_EXPENSIVE = INDIRECT_OFFSET_CHEAP + 4 * 3;
static const uint TILE_STATISTICS_CAPACITY = INDIRECT_OFFSET_EXPENSIVE + 4 * 3;

#endif // WI_SHADERINTEROP_POSTPROCESS_H
