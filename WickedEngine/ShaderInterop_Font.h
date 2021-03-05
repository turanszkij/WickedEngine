#ifndef WI_SHADERINTEROP_FONT_H
#define WI_SHADERINTEROP_FONT_H

#include "ShaderInterop.h"

CBUFFER(FontCB, CBSLOT_FONT)
{
	float4x4	g_xFont_Transform;
	float4		g_xFont_Color;
	uint		g_xFont_BufferOffset;
	int			g_xFont_TextureIndex;
	float		g_xFont_padding0;
	float		g_xFont_padding1;
};


#endif // WI_SHADERINTEROP_FONT_H
