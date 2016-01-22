#ifndef _IMAGEHF_
#define _IMAGEHF_
#include "globals.hlsli"

Texture2D<float> xSceneDepthMap:register(t0);
Texture2D<float4> xNormalMap:register(t1);
Texture2D<float4> xSpecular:register(t2);
Texture2D<float4> xSceneVelocityMap:register(t3);
Texture2D<float4> xRefracTexture:register(t4);
Texture2D<float4> xMaskTex:register(t5);
Texture2D<float4> xTexture:register(t6);

SamplerState Sampler:register(s0);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float4 tex				: TEXCOORD0; //intex,movingtex
	float4 dis				: TEXCOORD1; //distortion
	float mip				: TEXCOORD2;
};
struct VertexToPixelPostProcess
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

CBUFFER(ImageCB, CBSLOT_IMAGE_IMAGE)
{
	float4x4 xViewProjection;
	float4x4 xTrans;
	float4	 xDimensions;
	float4	 xOffsetMirFade;
	float4	 xDrawRec;
	float4	 xBlurOpaPiv;
	float4   xTexOffset;
	float4	 xMaskDistort;
}

CBUFFER(PostProcessCB, CBSLOT_IMAGE_POSTPROCESS)
{
	float	xPPParams[16];
};

#endif // _IMAGEHF_

