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

#endif // WI_SHADERINTEROP_POSTPROCESS_H
