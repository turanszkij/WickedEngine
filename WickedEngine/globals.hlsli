#ifndef _SHADERDEF_CONSTANTBUFFERS_
#define _SHADERDEF_CONSTANTBUFFERS_
#include "ConstantBufferMapping.h"
#include "SamplerMapping.h"
#include "TextureMapping.h"

SAMPLERSTATE(			sampler_linear_clamp,	SSLOT_LINEAR_CLAMP	)
SAMPLERSTATE(			sampler_linear_wrap,	SSLOT_LINEAR_WRAP	)
SAMPLERSTATE(			sampler_linear_mirror,	SSLOT_LINEAR_MIRROR	)
SAMPLERSTATE(			sampler_point_clamp,	SSLOT_POINT_CLAMP	)
SAMPLERSTATE(			sampler_point_wrap,		SSLOT_POINT_WRAP	)
SAMPLERSTATE(			sampler_point_mirror,	SSLOT_POINT_MIRROR	)
SAMPLERSTATE(			sampler_aniso_clamp,	SSLOT_ANISO_CLAMP	)
SAMPLERSTATE(			sampler_aniso_wrap,		SSLOT_ANISO_WRAP	)
SAMPLERSTATE(			sampler_aniso_mirror,	SSLOT_ANISO_MIRROR	)
SAMPLERCOMPARISONSTATE(	sampler_cmp_depth,		SSLOT_CMP_DEPTH	)

CBUFFER(WorldCB, CBSLOT_RENDERER_WORLD)
{
	float4		g_xWorld_SunDir;
	float4		g_xWorld_SunColor;
	float3		g_xWorld_Horizon;				float xPadding0_WorldCB;
	float3		g_xWorld_Zenith;				float xPadding1_WorldCB;
	float3		g_xWorld_Ambient;				float xPadding2_WorldCB;
	float3		g_xWorld_Fog;					float xPadding3_WorldCB;
	float2		g_xWorld_ScreenWidthHeight;
	float		xPadding_WorldCB[2];
};

CBUFFER(FrameCB, CBSLOT_RENDERER_FRAME)
{
	float3		g_xFrame_WindDirection;			float xPadding0_FrameCB;
	float		g_xFrame_WindTime;
	float		g_xFrame_WindWaveSize;
	float		g_xFrame_WindRandomness;
	float		xPadding_FrameCB[1];
};
CBUFFER(CameraCB, CBSLOT_RENDERER_CAMERA)
{
	float4x4	g_xCamera_View;
	float4x4	g_xCamera_Proj;
	float4x4	g_xCamera_VP;					// View*Projection
	float4x4	g_xCamera_PrevV;
	float4x4	g_xCamera_PrevP;
	float4x4	g_xCamera_PrevVP;				// PrevView*PrevProjection
	float4x4	g_xCamera_ReflVP;				// ReflectionView*ReflectionProjection
	float4x4	g_xCamera_InvVP;				// Inverse View-Projection
	float3		g_xCamera_CamPos;				float xPadding0_CameraCB;
	float3		g_xCamera_At;					float xPadding1_CameraCB;
	float3		g_xCamera_Up;					float xPadding2_CameraCB;
	float		g_xCamera_ZNearP;
	float		g_xCamera_ZFarP;
	float		xPadding_CameraCB[2];
};
CBUFFER(MaterialCB, CBSLOT_RENDERER_MATERIAL)
{
	float4		g_xMat_diffuseColor;
	float4		g_xMat_specular;
	float4		g_xMat_texMulAdd;
	uint		g_xMat_hasRef;
	uint		g_xMat_hasNor;
	uint		g_xMat_hasTex;
	uint		g_xMat_hasSpe;
	uint		g_xMat_shadeless;
	uint		g_xMat_specular_power;
	uint		g_xMat_toon;
	uint		g_xMat_matIndex;
	float		g_xMat_refractionIndex;
	float		g_xMat_metallic;
	float		g_xMat_emissive;
	float		g_xMat_roughness;
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


#define ALPHATEST(x) clip((x)-0.1);

#endif // _SHADERDEF_CONSTANTBUFFERS_