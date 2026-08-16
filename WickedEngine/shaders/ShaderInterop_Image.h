#ifndef WI_SHADERINTEROP_IMAGE_H
#define WI_SHADERINTEROP_IMAGE_H
#include "ShaderInterop.h"

enum IMAGE_FLAGS
{
	IMAGE_FLAG_EXTRACT_NORMALMAP = 1u << 0u,
	IMAGE_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084 = 1u << 1u,
	IMAGE_FLAG_OUTPUT_COLOR_SPACE_LINEAR = 1u << 2u,
	IMAGE_FLAG_FULLSCREEN = 1u << 3u,
	IMAGE_FLAG_MIRROR = 1u << 4u,
	IMAGE_FLAG_CORNER_ROUNDING = 1u << 5u,
	IMAGE_FLAG_ANGULAR_DOUBLESIDED = 1u << 6u,
	IMAGE_FLAG_ANGULAR_INVERSE = 1u << 7u,
	IMAGE_FLAG_DISTORTION_MASK = 1u << 8u,
	IMAGE_FLAG_HIGHLIGHT = 1u << 9u,
	IMAGE_FLAG_CUBEMAP_BASE = 1u << 10u,
	IMAGE_FLAG_TEXTURE1D_BASE = 1u << 11u,
	IMAGE_FLAG_GRADIENT_LINEAR = 1u << 12u,
	IMAGE_FLAG_GRADIENT_LINEAR_REFLECTED = 1u << 13u,
	IMAGE_FLAG_GRADIENT_CIRCULAR = 1u << 14u,

	FONT_FLAG_ISFONT = 1u << 15u,
	FONT_FLAG_SDF_RENDERING = 1u << 16u,
	FONT_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084 = 1u << 17u,
	FONT_FLAG_OUTPUT_COLOR_SPACE_LINEAR = 1u << 18u,
};

struct FontVertex
{
	float2 pos;
	float2 uv;
};
namespace SDF
{
	static const uint padding = 5;
	static const uint onedge_value = 127;
	static const float onedge_value_unorm = float(onedge_value) / 255.0f;
	static const float pixel_dist_scale = float(onedge_value) / float(padding);
}

struct alignas(16) ImageConstants
{
	uint flags;
	uint hdr_scaling_aspect; // packed half2
	uint2 packed_color; // packed half4

	float4 texMulAdd;
	float4 texMulAdd2;

	int buffer_index;
	uint buffer_offset;
	int sampler_index;
	int texture_base_index;

	int texture_mask_index;
	int texture_background_index;
	uint bordersoften_saturation; // packed half2
	uint mask_alpha_range; // packed half2

	// parameters for inverse bilinear interpolation:
	float2 b0;
	float2 b1;
	float2 b2;
	float2 b3;

	uint2 highlight_color_spread; // packed half4
	uint highlight_xy; // packed half2
	uint angular_softness_direction; // packed half2

	uint2 softness_bolden_hdrscaling; // packed half3
	uint angular_softness_mad; // packed half2
	uint padding0;

	uint2 gradient_color; // packed half4
	uint gradient_uv_start; // packed half2
	uint gradient_uv_end; // packed half2

	float4x4 transform;

	inline bool IsFont() { return flags & FONT_FLAG_ISFONT; }
};
#ifdef __cplusplus
static_assert(sizeof(ImageConstants) <= 256); // fit into one cb alloc dx12
#endif // __cplusplus

CONSTANTBUFFER(image, ImageConstants, CBSLOT_IMAGE);

#endif // WI_SHADERINTEROP_IMAGE_H
