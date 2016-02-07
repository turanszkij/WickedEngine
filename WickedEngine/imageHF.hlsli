#ifndef _IMAGEHF_
#define _IMAGEHF_
#include "globals.hlsli"

//Texture2D<float> xSceneDepthMap:register(t0);
//Texture2D<float4> xNormalMap:register(t1);
//Texture2D<float4> xSpecular:register(t2);
//Texture2D<float4> xSceneVelocityMap:register(t3);
//Texture2D<float4> xRefracTexture:register(t4);
//Texture2D<float4> xMaskTex:register(t5);
//Texture2D<float4> xTexture:register(t6);

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
	float4x4	xViewProjection;
	float4x4	xTrans;
	float4		xDimensions;
	float4		xDrawRec;
	float4		xTexMulAdd;
	uint		xPivot;
	uint		xMask;
	uint		xDistort;
	uint		xNormalmapSeparate;
	float		xOffsetX;
	float		xOffsetY;
	float		xMirror;
	float		xFade;
	float		xOpacity;
	float		xMipLevel;
	float		xPadding_ImageCB[2];
};

CBUFFER(PostProcessCB, CBSLOT_IMAGE_POSTPROCESS)
{
	float4		xPPParams0;
	float4		xPPParams1;
};

#endif // _IMAGEHF_

