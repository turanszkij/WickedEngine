#ifndef _EFFECTHF_PS_
#define _EFFECTHF_PS_

TextureCube enviroTex:register(t0);
Texture2D<float4> xTextureRef:register(t1);
Texture2D<float4> xTextureRefrac:register(t2);

Texture2D<float4> xTextureTex:register(t3);
Texture2D<float>  xTextureMat:register(t4);
Texture2D<float4> xTextureNor:register(t5);
Texture2D<float4> xTextureSpe:register(t6);

SamplerState texSampler:register(s0);
SamplerState mapSampler:register(s1);

struct PixelOutputType
{
	float4 col	: SV_TARGET0;
	float4 nor	: SV_TARGET1;
	//float4 spe	: SV_TARGET2;
	float4 vel	: SV_TARGET2;
};

#include "tangentComputeHF.hlsli"

#endif // _EFFECTHF_PS_
