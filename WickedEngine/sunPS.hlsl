#include "objectHF.hlsli"
#include "globals.hlsli"

struct VSOut {
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
}; 

float4 main(VSOut PSIn) : SV_TARGET
{ 
	float3 nor = normalize(PSIn.nor);
	return float4(max(pow(saturate(dot(GetSunDirection().xyz,nor) + 0.0001),64)*GetSunColor(), 0),1);
}