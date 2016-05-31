#ifndef _OBJECT_HF_VS_
#define _OBJECT_HF_VS_
#include "ConstantBufferMapping.h"
#include "objectHF.hlsli"

CBUFFER(MaterialCB_VS, CBSLOT_RENDERER_MATERIAL)
{
	float4 g_xMatVS_texMulAdd;
	uint   g_xMatVS_matIndex;
	float  xPadding_MaterialCB_VS[3];
};

#endif // _OBJECT_HF_VS_