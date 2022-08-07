#ifndef WI_SHADERINTEROP_IMAGE_H
#define WI_SHADERINTEROP_IMAGE_H
#include "ShaderInterop.h"

static const uint IMAGE_FLAG_EXTRACT_NORMALMAP = 1u << 0u;
static const uint IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084 = 1u << 1u;
static const uint IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR = 1u << 2u;
static const uint IMAGE_FLAG_FULLSCREEN = 1u << 3u;
static const uint IMAGE_FLAG_MIRROR = 1u << 4u;
static const uint IMAGE_FLAG_CORNER_ROUNDING = 1u << 5u;

struct ImageConstants
{
	uint flags;
	float hdr_scaling;
	uint2 packed_color; // packed half4

	uint2 texMulAdd; // packed half4
	uint2 texMulAdd2; // packed half4

	int buffer_index;
	uint buffer_offset;
	int sampler_index;
	int texture_base_index;

	int texture_mask_index;
	int texture_background_index;
	float border_soften;
	int padding0;

	// parameters for inverse bilinear interpolation:
	uint b0; // packed half2
	uint b1; // packed half2
	uint b2; // packed half2
	uint b3; // packed half2
};
CONSTANTBUFFER(image, ImageConstants, CBSLOT_IMAGE);

#endif // WI_SHADERINTEROP_IMAGE_H
