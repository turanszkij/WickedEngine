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

	float3 sunc = GetSunColor();

	float4 color = float4(normal.y > 0 ? max(pow(saturate(dot(GetSunDirection().xyz, normal) + 0.0001), 64)*sunc, 0) : 0, 1);

	AddCloudLayer(color, normal, true);

	return color;
}