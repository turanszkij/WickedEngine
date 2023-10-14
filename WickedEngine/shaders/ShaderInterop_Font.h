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

namespace SDF
{
	static const uint padding = 5;
	static const uint onedge_value = 127;
	static const float onedge_value_unorm = float(onedge_value) / 255.0f;
	static const float pixel_dist_scale = float(onedge_value) / float(padding);
}
struct FontConstants
{
	int buffer_index;
	uint buffer_offset;
	int texture_index;
	int padding0;

	float4 color;

	float softness;
	float bolden;
	uint flags;
	float hdr_scaling;

	float4x4 transform;
};
CONSTANTBUFFER(font, FontConstants, CBSLOT_FONT);

#endif // WI_SHADERINTEROP_FONT_H
