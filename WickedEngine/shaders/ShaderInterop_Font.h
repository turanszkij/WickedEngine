#ifndef WI_SHADERINTEROP_FONT_H
#define WI_SHADERINTEROP_FONT_H
#include "ShaderInterop.h"

struct PushConstantsFont
{
	float4x4 transform;
	uint color;
	int buffer_index;
	uint buffer_offset;
	int texture_index;
};
PUSHCONSTANT(push, PushConstantsFont);


#endif // WI_SHADERINTEROP_FONT_H
