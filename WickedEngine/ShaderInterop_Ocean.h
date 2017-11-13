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

CBUFFER(Ocean_Rendering_ShadingCB, CBSLOT_OTHER_OCEAN_RENDER_SHADING)
{
	float3		g_SkyColor;
	float		g_TexelLength_x2;

	float3		g_WaterbodyColor;
	float		g_UVScale;

	float		g_Shineness;
	float3		g_SunDir;

	float		g_UVOffset;
	float3		g_SunColor;

	// The parameter is used for fixing an artifact
	float3		g_BendParam;
	float		Ocean_Rendering_ShadingCB_padding0;

	// Perlin noise for distant wave crest
	float		g_PerlinSize;
	float3		g_PerlinAmplitude;

	float3		g_PerlinOctave;
	float		Ocean_Rendering_ShadingCB_padding1;

	float3		g_PerlinGradient;
	float		Ocean_Rendering_ShadingCB_padding2;
};

// Per draw call constants
CBUFFER(Ocean_Rendering_PatchCB, CBSLOT_OTHER_OCEAN_RENDER_PATCH)
{
	// Transform matrices
	matrix		g_matLocal;
	matrix		g_matWorldViewProj;

	// Misc per draw call constants
	float2		g_UVBase;
	float2		g_PerlinMovement;

	float3		g_LocalEye;
	float		Ocean_Rendering_PatchCB_padding;
};

#endif // _SHADERINTEROP_OCEAN_H_
