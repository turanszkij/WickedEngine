#ifndef WI_SHADERINTEROP_FONT_H
#define WI_SHADERINTEROP_FONT_H
#include "ShaderInterop.h"

static const uint FONT_FLAG_SDF_RENDERING = 1u << 0u;
static const uint FONT_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084 = 1u << 1u;
static const uint FONT_FLAG_OUTPUT_COLOR_SPACE_LINEAR = 1u << 2u;

struct FontVertex
{
	float2 pos;
	float2 uv;
};

struct FontConstants
{
	int buffer_index;
	uint buffer_offset;
	int texture_index;
	uint color;

	float sdf_threshold_top;
	float sdf_threshold_bottom;
	uint flags;
	float hdr_scaling;

	float4x4 transform;
};
CONSTANTBUFFER(font, FontConstants, CBSLOT_FONT);

#endif // WI_SHADERINTEROP_FONT_H
