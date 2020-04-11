#ifndef WI_SHADERINTEROP_UTILITY_H
#define WI_SHADERINTEROP_UTILITY_H
#include "ShaderInterop.h"

// MIP Generator params:
#define GENERATEMIPCHAIN_1D_BLOCK_SIZE 64
#define GENERATEMIPCHAIN_2D_BLOCK_SIZE 8
#define GENERATEMIPCHAIN_3D_BLOCK_SIZE 4

CBUFFER(GenerateMIPChainCB, CBSLOT_RENDERER_UTILITY)
{
	uint3 outputResolution;
	uint arrayIndex;
	float3 outputResolution_rcp;
	uint mipgen_options;
};
static const uint MIPGEN_OPTION_BIT_PRESERVE_COVERAGE = 1 << 0;

CBUFFER(FilterEnvmapCB, CBSLOT_RENDERER_UTILITY)
{
	uint2 filterResolution;
	float2 filterResolution_rcp;
	uint filterArrayIndex;
	float filterRoughness;
	uint filterRayCount;
	uint padding_filterCB;
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

#endif // WI_SHADERINTEROP_UTILITY_H
