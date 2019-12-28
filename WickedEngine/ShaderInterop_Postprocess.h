#ifndef WI_SHADERINTEROP_POSTPROCESS_H
#define WI_SHADERINTEROP_POSTPROCESS_H
#include "ShaderInterop.h"

static const uint POSTPROCESS_BLOCKSIZE = 8;

CBUFFER(PostProcessCB, CBSLOT_RENDERER_POSTPROCESS)
{
	uint2		xPPResolution;
	float2		xPPResolution_rcp;
	float4		xPPParams0;
	float4		xPPParams1;
};

static const uint MOTIONBLUR_TILESIZE = 32;

static const uint DEPTHOFFIELD_TILESIZE = 32;
#define dof_focus xPPParams0.x
#define dof_scale xPPParams0.y

#endif // WI_SHADERINTEROP_POSTPROCESS_H
