#ifndef WI_SHADERINTEROP_IMAGE_H
#define WI_SHADERINTEROP_IMAGE_H
#include "ShaderInterop.h"

CBUFFER(ImageCB, CBSLOT_IMAGE)
{
	float4	xCorners[4];
	float4	xTexMulAdd;
	float4	xTexMulAdd2;
	float4	xColor;
};

struct PushConstantsImage
{
	int texture_base_index;
	int texture_mask_index;
	int texture_background_index;
	int sampler_index;
};


#endif // WI_SHADERINTEROP_IMAGE_H
