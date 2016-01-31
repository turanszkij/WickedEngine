#include "globalsHF.hlsli"
#include "skyHF.hlsli"

struct VSOut{
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

struct PSOut{
	float4 col : SV_TARGET0;
	float4 nor : SV_TARGET1;
	float4 vel : SV_TARGET2;
};

PSOut main(VSOut PSIn)
{
	float2 ScreenCoord = float2(1, -1) * PSIn.pos2D.xy / PSIn.pos2D.w / 2.0f + 0.5f;
	float2 ScreenCoordPrev = float2(1, -1) * PSIn.pos2DPrev.xy / PSIn.pos2DPrev.w / 2.0f + 0.5f;
	float2 vel = ScreenCoord - ScreenCoordPrev;

	PSOut Out = (PSOut)0;
	Out.col = GetSkyColor(PSIn.nor);
	Out.nor = float4(0, 0, 0, 0);
	Out.vel = float4(vel, 0, 0);
	return Out;
}
