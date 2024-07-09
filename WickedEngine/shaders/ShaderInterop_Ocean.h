#ifndef WI_SHADERINTEROP_OCEAN_H
#define WI_SHADERINTEROP_OCEAN_H
#include "ShaderInterop.h"

#define OCEAN_COMPUTE_TILESIZE 8

// Simulation constants:

CBUFFER(Ocean_Simulation_ImmutableCB, CBSLOT_OTHER_OCEAN_SIMULATION_IMMUTABLE)
{
	uint g_ActualDim;
	uint g_InWidth;
	uint g_OutWidth;
	uint g_OutHeight;

	uint g_DtxAddressOffset;
	uint g_DtyAddressOffset;
};

CBUFFER(Ocean_Simulation_PerFrameCB, CBSLOT_OTHER_OCEAN_SIMULATION_PERFRAME)
{
	float g_TimeScale;
	float g_ChoppyScale;
	float g_GridLen;
	float Ocean_Simulation_PerFrameCB_padding;
};


// Rendering constants:

CBUFFER(Ocean_RenderCB, CBSLOT_OTHER_OCEAN_RENDER)
{
	float4		xOceanWaterColor;
	float4		xOceanExtinctionColor;
	float4		xOceanScreenSpaceParams;

	float		xOceanTexelLength;
	float		xOceanPatchSizeRecip;
	float		xOceanMapHalfTexel;
	float		xOceanWaterHeight;

	float		xOceanSurfaceDisplacementTolerance;
	float		xOceanPadding1;
	float		xOceanPadding2;
	float		xOceanPadding3;
};

#endif // WI_SHADERINTEROP_OCEAN_H
