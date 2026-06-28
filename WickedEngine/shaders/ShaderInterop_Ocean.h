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
	// Element offset to the second packed FFT field (Dy). The first field packs
	// height (real output) + Dx (imaginary output) via the two-for-one real FFT.
	uint xOceanSecondFieldOffset;
	uint xOcean_padding2;
	float xOceanTimeScale;

	float xOceanChoppyScale;
	float xOceanGridLen;
	float xOceanWaveAmplitude;
	float xOcean_padding1;
};

#endif // WI_SHADERINTEROP_OCEAN_H
