#ifndef _SHADERINTEROP_IMAGE_H_
#define _SHADERINTEROP_IMAGE_H_
#include "ShaderInterop.h"

CBUFFER(ImageCB, CBSLOT_IMAGE_IMAGE)
{
	float4x4	xTransform;
	float4		xTexMulAdd;
	float4		xColor;
	float2		xPivot;
	uint		xMirror;
	float		xMipLevel;
};
CBUFFER(PostProcessCB, CBSLOT_IMAGE_POSTPROCESS)
{
	float4		xPPParams0;
	float4		xPPParams1;
};


#endif // _SHADERINTEROP_IMAGE_H_
