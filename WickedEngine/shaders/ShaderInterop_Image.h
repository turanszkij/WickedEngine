#ifndef WI_SHADERINTEROP_IMAGE_H
#define WI_SHADERINTEROP_IMAGE_H
#include "ShaderInterop.h"

static const uint IMAGE_FLAG_EXTRACT_NORMALMAP = 1u << 0u;
static const uint IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084 = 1u << 1u;
static const uint IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR = 1u << 2u;
static const uint IMAGE_FLAG_FULLSCREEN = 1u << 3u;
static const uint IMAGE_FLAG_MIRROR = 1u << 4u;
static const uint IMAGE_FLAG_CORNER_ROUNDING = 1u << 5u;
static const uint IMAGE_FLAG_ANGULAR_DOUBLESIDED = 1u << 6u;
static const uint IMAGE_FLAG_ANGULAR_INVERSE = 1u << 7u;
static const uint IMAGE_FLAG_DISTORTION_MASK = 1u << 8u;

struct ImageConstants
{
	uint flags;
	float hdr_scaling;
	uint2 packed_color; // packed half4

	float4 texMulAdd;
	float4 texMulAdd2;

	int buffer_index;
	uint buffer_offset;
	int sampler_index;
	int texture_base_index;

	int texture_mask_index;
	int texture_background_index;
	float border_soften;
	uint mask_alpha_range; // packed half2

	// parameters for inverse bilinear interpolation:
	float2 b0;
	float2 b1;
	float2 b2;
	float2 b3;

	float2 angular_softness_direction;
	float angular_softness_scale;
	float angular_softness_offset;
};
CONSTANTBUFFER(image, ImageConstants, CBSLOT_IMAGE);

#endif // WI_SHADERINTEROP_IMAGE_H
