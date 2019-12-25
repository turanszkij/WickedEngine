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

static const uint MOTIONBLUR_TILECOUNT = 24;
#define MOTIONBLUR_TILESIZE ((xPPResolution + MOTIONBLUR_TILECOUNT - 1) / MOTIONBLUR_TILECOUNT)

#endif // WI_SHADERINTEROP_POSTPROCESS_H
