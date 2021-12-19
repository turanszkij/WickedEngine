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
	float4x4 transform;
};
CONSTANTBUFFER(font, FontConstants, CBSLOT_FONT);

struct FontPushConstants
{
	uint color;
	int buffer_index;
	uint buffer_offset;
	int texture_index;
};
PUSHCONSTANT(font_push, FontPushConstants);

#endif // WI_SHADERINTEROP_FONT_H
