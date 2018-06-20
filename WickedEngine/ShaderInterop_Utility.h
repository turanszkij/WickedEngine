#ifndef _SHADERINTEROP_UTILITY_H_
#define _SHADERINTEROP_UTILITY_H_
#include "ShaderInterop.h"

CBUFFER(CopyTextureCB, CBSLOT_RENDERER_UTILITY)
{
	uint2 xCopyDest;
	uint2 padding0;
};

#endif //_SHADERINTEROP_UTILITY_H_
