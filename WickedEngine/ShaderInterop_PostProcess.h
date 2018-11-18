#ifndef _SHADERINTEROP_POSTPROCESS_H_
#define _SHADERINTEROP_POSTPROCESS_H_
#include "ShaderInterop.h"

CBUFFER(PostProcessCB, CBSLOT_IMAGE_POSTPROCESS)
{
	float4		xPPParams0;
	float4		xPPParams1;
};

#endif // _SHADERINTEROP_POSTPROCESS_H_
