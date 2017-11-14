#ifndef _SHADERINTEROP_OCEAN_H_
#define _SHADERINTEROP_OCEAN_H_
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
	uint2 Ocean_Simulation_ImmutableCB_padding;
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
	float		xOceanTexelLengthMul2;
	float4		xOceanTexMulAdd;
	float4		xOceanScreenSpaceParams;
};

#endif // _SHADERINTEROP_OCEAN_H_
