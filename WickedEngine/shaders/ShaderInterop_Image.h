#ifndef WI_SHADERINTEROP_IMAGE_H
#define WI_SHADERINTEROP_IMAGE_H
#include "ShaderInterop.h"

static const uint IMAGE_FLAG_EXTRACT_NORMALMAP = 1 << 0;
static const uint IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084 = 1 << 1;
static const uint IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR = 1 << 2;
static const uint IMAGE_FLAG_FULLSCREEN = 1 << 3;

struct ImageConstants
{
	float4 corners0;
	float4 corners1;
	float4 corners2;
	float4 corners3;

	uint2 texMulAdd; // packed half4
	uint2 texMulAdd2; // packed half4

	uint2 output_resolution;
	float2 output_resolution_rcp;
};
CONSTANTBUFFER(image, ImageConstants, CBSLOT_IMAGE);

struct ImagePushConstants
{
	uint2 packed_color; // packed half4
	uint flags;
	float hdr_scaling;

	int sampler_index;
	int texture_base_index;
	int texture_mask_index;
	int texture_background_index;
};
PUSHCONSTANT(image_push, ImagePushConstants);

#endif // WI_SHADERINTEROP_IMAGE_H
