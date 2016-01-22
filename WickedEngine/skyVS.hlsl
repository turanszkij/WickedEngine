#include "icosphere.hlsli"
#include "globals.hlsli"

struct VSOut{
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

VSOut main(uint vid : SV_VERTEXID)
{
	VSOut Out = (VSOut)0;
	Out.pos = Out.pos2D = mul(float4(mul(ICOSPHERE[vid], (float3x3)g_xCamera_View), 0), g_xCamera_Proj);
	Out.nor=ICOSPHERE[vid];
	Out.pos2DPrev = mul(float4(mul(ICOSPHERE[vid], (float3x3)g_xCamera_PrevV), 0), g_xCamera_PrevP);
	return Out;
}
