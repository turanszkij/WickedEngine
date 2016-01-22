#ifndef _VOLUMELIGHT_HF_
#define _VOLUMELIGHT_HF_
#include "globals.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 col			: COLOR;
};

CBUFFER(VolumeLightCB, CBSLOT_RENDERER_VOLUMELIGHT)
{
	float4x4 lightWorld;
	float4 lightColor;
	float4 lightEnerdis;
};

#endif // _VOLUMELIGHT_HF_