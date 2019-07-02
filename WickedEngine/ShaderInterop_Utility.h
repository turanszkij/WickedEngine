#ifndef _SHADERINTEROP_UTILITY_H_
#define _SHADERINTEROP_UTILITY_H_
#include "ShaderInterop.h"

// MIP Generator params:
#define GENERATEMIPCHAIN_1D_BLOCK_SIZE 64
#define GENERATEMIPCHAIN_2D_BLOCK_SIZE 8
#define GENERATEMIPCHAIN_3D_BLOCK_SIZE 4

CBUFFER(GenerateMIPChainCB, CBSLOT_RENDERER_UTILITY)
{
	uint3 outputResolution;
	uint arrayIndex;
};

CBUFFER(FilterEnvmapCB, CBSLOT_RENDERER_UTILITY)
{
	uint2 filterResolution;
	uint filterArrayIndex;
	float filterRoughness;
	uint3 padding_filterCB;
	uint filterRayCount;
};



// CopyTexture2D params:
CBUFFER(CopyTextureCB, CBSLOT_RENDERER_UTILITY)
{
	int2 xCopyDest;
	int2 xCopySrcSize;
	int2 padding0;
	int  xCopySrcMIP;
	int  xCopyBorderExpandStyle;
};

#endif //_SHADERINTEROP_UTILITY_H_
