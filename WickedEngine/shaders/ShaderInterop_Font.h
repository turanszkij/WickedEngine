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
	uint color;
	int buffer_index;
	uint buffer_offset;
	int texture_index;
};
CONSTANTBUFFER(font, FontConstants, CBSLOT_FONT);


#endif // WI_SHADERINTEROP_FONT_H
