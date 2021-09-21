#ifndef WI_SHADERINTEROP_IMAGE_H
#define WI_SHADERINTEROP_IMAGE_H
#include "ShaderInterop.h"

static const uint IMAGE_FLAG_EXTRACT_NORMALMAP = 1 << 0;

struct PushConstantsImage
{
	float4	corners[4];
	float4	texMulAdd;
	float4	texMulAdd2;

	uint2	packed_color; // packed half4
	uint	flags;
	int		sampler_index;

	int		texture_base_index;
	int		texture_mask_index;
	int		texture_background_index;
};
PUSHCONSTANT(push, PushConstantsImage);


#endif // WI_SHADERINTEROP_IMAGE_H
