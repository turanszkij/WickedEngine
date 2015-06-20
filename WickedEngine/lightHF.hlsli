#include "specularHF.hlsli"
#include "toonHF.hlsli"
#include "globalsHF.hlsli"
//#include "ViewProp.h"
#include "viewProp.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	//float3 pos3D		: POSITION3D;
	float4 pos2D		: POSITION2D;
	float3 cam			: CAMERAPOS;
};

cbuffer lightStatic:register(b0){
	float4x4 matProjInv;
}

#include "reconstructPositionHF.hlsli"

Texture2D<float> depthMap:register(t0);
Texture2D<float4> normalMap:register(t1);
//Texture2D<float4> specularMap:register(t2);
Texture2D<float4> materialMap:register(t2);
SamplerState Sampler:register(s0);

static const float specularMaximumIntensity = 1;
static const float inf=100000000;



