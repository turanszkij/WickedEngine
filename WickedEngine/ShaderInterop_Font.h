#ifndef WI_SHADERINTEROP_FONT_H
#define WI_SHADERINTEROP_FONT_H

#include "ShaderInterop.h"

struct FontCB
{
	float4x4	g_xFont_Transform;
	float4		g_xFont_Color;
};
ROOTCONSTANTS(fontCB, FontCB, CBSLOT_FONT);


#endif // WI_SHADERINTEROP_FONT_H
