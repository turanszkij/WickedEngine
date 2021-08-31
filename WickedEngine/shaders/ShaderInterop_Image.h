#ifndef WI_SHADERINTEROP_IMAGE_H
#define WI_SHADERINTEROP_IMAGE_H
#include "ShaderInterop.h"

struct PushConstantsImage
{
	float4	corners[4];
	float4	texMulAdd;
	float4	texMulAdd2;
	float4	color;

	int texture_base_index;
	int texture_mask_index;
	int texture_background_index;
	int sampler_index;
};
PUSHCONSTANT(push, PushConstantsImage);


#endif // WI_SHADERINTEROP_IMAGE_H
