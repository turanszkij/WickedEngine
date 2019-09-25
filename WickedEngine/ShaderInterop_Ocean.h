#ifndef WI_SHADERINTEROP_OCEAN_H
#define WI_SHADERINTEROP_OCEAN_H
#include "ShaderInterop.h"

#define OCEAN_COMPUTE_TILESIZE 16

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
	float g_Time;
	float g_ChoppyScale;
	float g_GridLen;
	float Ocean_Simulation_PerFrameCB_padding;
};


// Rendering constants:

CBUFFER(Ocean_RenderCB, CBSLOT_OTHER_OCEAN_RENDER)
{
	float3		xOceanWaterColor;
	float		xOceanTexelLength;
	float4		xOceanScreenSpaceParams;
	float		xOceanPatchSizeRecip;
	float		xOceanMapHalfTexel;
	float		xOceanWaterHeight;
	float		xOceanSurfaceDisplacementTolerance;
};

#endif // WI_SHADERINTEROP_OCEAN_H
