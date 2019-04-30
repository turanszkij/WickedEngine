#ifndef _SHADERINTEROP_IMAGE_H_
#define _SHADERINTEROP_IMAGE_H_
#include "ShaderInterop.h"

CBUFFER(ImageCB, CBSLOT_IMAGE_IMAGE)
{
	float4		xCorners[4];
	float4		xTexMulAdd;
	float4		xTexMulAdd2;
	float4		xColor;
	uint		xMirror;
	float		xMipLevel;
	float2		padding_imageCB;
};
CBUFFER(PostProcessCB, CBSLOT_IMAGE_POSTPROCESS)
{
	float4		xPPParams0;
	float4		xPPParams1;
};


#endif // _SHADERINTEROP_IMAGE_H_
