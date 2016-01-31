#ifndef _LIGHTHF_
#define _LIGHTHF_
#include "specularHF.hlsli"
#include "toonHF.hlsli"
#include "globalsHF.hlsli"
#include "globals.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
};

#include "reconstructPositionHF.hlsli"

Texture2D<float> depthMap:register(t0);
Texture2D<float4> normalMap:register(t1);
//Texture2D<float4> specularMap:register(t2);
Texture2D<float4> materialMap:register(t2);

static const float specularMaximumIntensity = 1;

#endif // _LIGHTHF_



