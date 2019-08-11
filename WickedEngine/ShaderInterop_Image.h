#ifndef _SHADERINTEROP_IMAGE_H_
#define _SHADERINTEROP_IMAGE_H_
#include "ShaderInterop.h"

CBUFFER(ImageCB, CBSLOT_IMAGE)
{
	float4		xCorners[4];
	float4		xTexMulAdd;
	float4		xTexMulAdd2;
	float4		xColor;
	float2		padding_imageCB;
	uint		xMirror;
	float		xMipLevel;
};


#endif // _SHADERINTEROP_IMAGE_H_
