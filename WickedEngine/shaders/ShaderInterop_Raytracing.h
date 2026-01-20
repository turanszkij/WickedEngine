#ifndef WI_SHADERINTEROP_RAYTRACING_H
#define WI_SHADERINTEROP_RAYTRACING_H
#include "ShaderInterop.h"


CBUFFER(RaytracingCB, CBSLOT_RENDERER_TRACED)
{
	float2 xTracePixelOffset;
	uint xTraceSampleIndex;
	float xTraceAccumulationFactor;
	uint2 xTraceResolution;
	float2 xTraceResolution_rcp;
	uint4 xTraceUserData;
};


#endif // WI_SHADERINTEROP_RAYTRACING_H
