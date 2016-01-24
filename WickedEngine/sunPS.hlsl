#include "effectHF_PS.hlsli"
#include "globalsHF.hlsli"
#include "globals.hlsli"

struct VSOut {
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
}; 

float4 main(VSOut PSIn) : SV_TARGET
{ 
	static const float overBright = 1.005f;
	float3 inNor = normalize(PSIn.nor);
	float3 nor = inNor*overBright;
	return float4(max(pow(abs(dot(g_xWorld_SunDir.xyz,nor)),64)*g_xWorld_SunColor.rgb, 0),1);
}