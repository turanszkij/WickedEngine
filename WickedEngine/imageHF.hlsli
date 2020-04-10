#ifndef WI_IMAGE_HF
#define WI_IMAGE_HF
#include "globals.hlsli"
#include "ShaderInterop_Image.h"

TEXTURE2D(texture_base, float4, TEXSLOT_IMAGE_BASE);
TEXTURE2D(texture_mask, float4, TEXSLOT_IMAGE_MASK);
TEXTURE2D(texture_background, float4, TEXSLOT_IMAGE_BACKGROUND);

SAMPLERSTATE(Sampler, SSLOT_ONDEMAND0);

struct VertextoPixel
{
	float4 pos : SV_POSITION;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
	float4 uv_screen : TEXCOORD2;
};

#endif // WI_IMAGE_HF

