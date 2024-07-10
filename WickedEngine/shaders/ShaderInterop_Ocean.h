#ifndef WI_SHADERINTEROP_OCEAN_H
#define WI_SHADERINTEROP_OCEAN_H
#include "ShaderInterop.h"

static const uint OCEAN_COMPUTE_TILESIZE = 8;

CBUFFER(OceanCB, CBSLOT_OTHER_OCEAN)
{
	float4 xOceanWaterColor;
	float4 xOceanExtinctionColor;
	float4 xOceanScreenSpaceParams;

	float xOceanTexelLength;
	float xOceanPatchSizeRecip;
	float xOceanMapHalfTexel;
	float xOceanWaterHeight;

	float xOceanSurfaceDisplacementTolerance;
	uint xOceanActualDim;
	uint xOceanInWidth;
	uint xOceanOutWidth;

	uint xOceanOutHeight;
	uint xOceanDtxAddressOffset;
	uint xOceanDtyAddressOffset;
	float xOceanTimeScale;

	float xOceanChoppyScale;
	float xOceanGridLen;
	float xOcean_padding0;
	float xOcean_padding1;
};

#endif // WI_SHADERINTEROP_OCEAN_H
