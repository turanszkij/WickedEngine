#ifndef _SHADERINTEROP_TRACEDRENDERING_H_
#define _SHADERINTEROP_TRACEDRENDERING_H_
#include "ShaderInterop.h"

#define TRACEDRENDERING_PRIMARY_BLOCKSIZE 8

CBUFFER(TracedRenderingCB, CBSLOT_RENDERER_TRACED)
{
	float2	xTracePixelOffset;
	uint xTraceSample;
	uint xTraceMeshTriangleCount;
};

#endif // _SHADERINTEROP_TRACEDRENDERING_H_
