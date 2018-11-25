#ifndef _IMAGEHF_
#define _IMAGEHF_
#include "globals.hlsli"
#include "ShaderInterop_Image.h"

#define xTexture		texture_0
#define xMaskTex		texture_1
#define xDistortionTex	texture_2

SAMPLERSTATE(Sampler, SSLOT_ONDEMAND0);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex_original		: TEXCOORD0;
	float2 tex				: TEXCOORD1;
	float4 pos2D			: TEXCOORD2;
};
struct VertexToPixelPostProcess
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

#endif // _IMAGEHF_

