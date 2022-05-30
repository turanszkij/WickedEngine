#ifndef WI_SHADERINTEROP_FONT_H
#define WI_SHADERINTEROP_FONT_H
#include "ShaderInterop.h"

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

	float3 padding;
	float sdf_threshold_bottom;

	float4x4 transform;
};
CONSTANTBUFFER(font, FontConstants, CBSLOT_FONT);

static const int FONT_SDF_ONEDGE_VALUE = 180;
static const float FONT_SDF_THRESHOLD_TOP = float(FONT_SDF_ONEDGE_VALUE) / 255.0f;

#endif // WI_SHADERINTEROP_FONT_H
