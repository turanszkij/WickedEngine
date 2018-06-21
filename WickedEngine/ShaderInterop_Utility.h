#ifndef _SHADERINTEROP_UTILITY_H_
#define _SHADERINTEROP_UTILITY_H_
#include "ShaderInterop.h"

CBUFFER(CopyTextureCB, CBSLOT_RENDERER_UTILITY)
{
	int2 xCopyDest;
	int2 xCopySrcSize;
	int  xCopySrcMIP;
	int  xCopyBorderExpandStyle;
	int2 padding0;
};

#endif //_SHADERINTEROP_UTILITY_H_
