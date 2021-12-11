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

#undef WICKED_ENGINE_ROOTSIGNATURE_GRAPHICS
#define WICKED_ENGINE_ROOTSIGNATURE_GRAPHICS \
	"RootConstants(num32BitConstants=32, b999), " \
	WICKED_ENGINE_ROOTSIGNATURE_PART_BINDLESS


#endif // WI_SHADERINTEROP_FONT_H
