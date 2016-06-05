#include "skyHF.hlsli"
#include "objectHF.hlsli"

struct VSOut{
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

GBUFFEROutputType main(VSOut PSIn)
{
	float2 ScreenCoord = float2(1, -1) * PSIn.pos2D.xy / PSIn.pos2D.w / 2.0f + 0.5f;
	float2 ScreenCoordPrev = float2(1, -1) * PSIn.pos2DPrev.xy / PSIn.pos2DPrev.w / 2.0f + 0.5f;
	float2 velocity = ScreenCoord - ScreenCoordPrev;

	float4 color = float4(GetSkyColor(PSIn.nor), 0);
	float emissive = 0;
	float3 N = 0;
	float roughness = 0;
	float reflectance = 0;
	float metalness = 0;
	float ao = 1;
	float sss = 0;

	OBJECT_PS_OUT_GBUFFER
}
