#ifndef _SHADERINTEROP_FONT_H_
#define _SHADERINTEROP_FONT_H_

#include "ShaderInterop.h"

CBUFFER(FontCB, CBSLOT_FONT)
{
	float4x4	g_xFont_Transform;
	float4		g_xFont_Color;
};


#endif // _SHADERINTEROP_FONT_H_
