#ifndef _SHADERINTEROP_POSTPROCESS_H_
#define _SHADERINTEROP_POSTPROCESS_H_
#include "ShaderInterop.h"

static const uint POSTPROCESS_BLOCKSIZE = 8;

CBUFFER(PostProcessCB, CBSLOT_RENDERER_POSTPROCESS)
{
	uint2		xPPResolution;
	float2		xPPResolution_rcp;
	float4		xPPParams0;
	float4		xPPParams1;
};

#endif // _SHADERINTEROP_POSTPROCESS_H_
