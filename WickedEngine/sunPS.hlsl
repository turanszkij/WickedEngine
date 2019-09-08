#define DARK_SKY // no sky colors, only sun
#include "objectHF.hlsli"
#include "globals.hlsli"
#include "skyHF.hlsli"

struct VSOut {
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

float4 main(VSOut PSIn) : SV_TARGET
{ 
	float3 normal = normalize(PSIn.nor);

	return float4(GetDynamicSkyColor(normal, true, true, true), 1);
}