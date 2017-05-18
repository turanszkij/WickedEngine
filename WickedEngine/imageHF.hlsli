#ifndef _IMAGEHF_
#define _IMAGEHF_
#include "globals.hlsli"

// texture_0	: texture map
// texture_1	: mask map
#define xTexture	texture_0
#define xMaskTex	texture_1

SAMPLERSTATE(Sampler, SSLOT_ONDEMAND0);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0; //intex,movingtex
	float4 dis				: TEXCOORD1; //distortion
};
struct VertexToPixelPostProcess
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

CBUFFER(ImageCB, CBSLOT_IMAGE_IMAGE)
{
	float4x4	xTransform;
	float4		xTexMulAdd;
	float4		xColor;
	float2		xPivot;
	uint		xMirror;

	uint		xMask;
	uint		xDistort;
	uint		xNormalmapSeparate;
	float		xMipLevel;
	float		xFade;
	float		xOpacity;
	float		xPadding0_ImageCB;
	float		xPadding1_ImageCB;
	float		xPadding2_ImageCB;
};

CBUFFER(PostProcessCB, CBSLOT_IMAGE_POSTPROCESS)
{
	float4		xPPParams0;
	float4		xPPParams1;
};

#endif // _IMAGEHF_

