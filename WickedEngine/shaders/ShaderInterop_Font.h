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

	float sdf_threshold_top;
	float sdf_threshold_bottom;
	float padding0;
	float padding1;

	float4x4 transform;
};
CONSTANTBUFFER(font, FontConstants, CBSLOT_FONT);

#endif // WI_SHADERINTEROP_FONT_H
