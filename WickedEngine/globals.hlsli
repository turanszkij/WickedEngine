#ifndef _SHADERDEF_CONSTANTBUFFERS_
#define _SHADERDEF_CONSTANTBUFFERS_
#include "ConstantBufferMapping.h"

CBUFFER(WorldCB, CBSLOT_RENDERER_WORLD)
{
	float4		g_xWorld_SunDir;
	float4		g_xWorld_SunColor;
	float3		g_xWorld_Horizon;
	float3		g_xWorld_Zenith;
	float3		g_xWorld_Ambient;
	float3		g_xWorld_Fog;
	float2		g_xWorld_ScreenWidthHeight;
	float		xPadding_WorldCB[2];
}

CBUFFER(FrameCB, CBSLOT_RENDERER_FRAME)
{
	float3		g_xFrame_WindDirection;
	float		g_xFrame_WindTime;
	float		g_xFrame_WindWaveSize;
	float		g_xFrame_WindRandomness;
	float		xPadding_FrameCB[2];
};
CBUFFER(CameraCB, CBSLOT_RENDERER_CAMERA)
{
	float4x4	g_xCamera_View;
	float4x4	g_xCamera_Proj;
	float4x4	g_xCamera_VP;			// View*Projection
	float4x4	g_xCamera_PrevV;
	float4x4	g_xCamera_PrevP;
	float4x4	g_xCamera_PrevVP;		// PrevView*PrevProjection
	float4x4	g_xCamera_ReflVP;		// ReflectionView*ReflectionProjection
	float4x4	g_xCamera_InvP;			// Inverse Projection
	float3		g_xCamera_CamPos;
	float3		g_xCamera_At;
	float3		g_xCamera_Up;
	float		g_xCamera_ZNearP;
	float		g_xCamera_ZFarP;
	float		xPadding_CameraCB[3];
};
CBUFFER(MaterialCB, CBSLOT_RENDERER_MATERIAL)
{
	float4		g_xMat_diffuseColor;
	uint		g_xMat_hasRef;
	uint		g_xMat_hasNor;
	uint		g_xMat_hasTex;
	uint		g_xMat_hasSpe;
	float4		g_xMat_specular;
	float		g_xMat_refractionIndex;
	float2		g_xMat_movingTex;
	float		g_xMat_metallic;
	uint		g_xMat_shadeless;
	uint		g_xMat_specular_power;
	uint		g_xMat_toon;
	uint		g_xMat_matIndex;
	float		g_xMat_emit;
	float		xPadding_MaterialCB[3];
};
CBUFFER(DirectionalLightCB, CBSLOT_RENDERER_DIRLIGHT)
{
	float4		g_xDirLight_direction;
	float4		g_xDirLight_col;
	float4		g_xDirLight_mBiasResSoftshadow;
	float4x4	g_xDirLight_ShM[3];
};
CBUFFER(MiscCB, CBSLOT_RENDERER_MISC)
{
	float4x4	g_xTransform;
	float4		g_xColor;
};
CBUFFER(ShadowCB, CBSLOT_RENDERER_SHADOW)
{
	float4x4	g_xShadow_VP;
};

CBUFFER(APICB, CBSLOT_API)
{
	float4		g_xClipPlane;
};


#endif // _SHADERDEF_CONSTANTBUFFERS_