#ifndef _IMAGEHF_
#define _IMAGEHF_
#include "globals.hlsli"
#include "ShaderInterop_Image.h"

TEXTURE2D(texture_base, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_mask, float4, TEXSLOT_ONDEMAND1);

SAMPLERSTATE(Sampler, SSLOT_ONDEMAND0);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 uv0				: TEXCOORD0;
	float2 uv1				: TEXCOORD1;
};

#endif // _IMAGEHF_

