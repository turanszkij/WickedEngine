#include "skyHF.hlsli"

struct VSOut {
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
};

float4 main(VSOut PSIn) : SV_TARGET
{
	return GetSkyColor(PSIn.nor);
}