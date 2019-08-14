#include "globals.hlsli"
#include "icosphere.hlsli"
#include "skyHF.hlsli"

struct VSOut {
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

VSOut main(uint vid : SV_VERTEXID)
{
	VSOut Out;
	Out.pos = Out.pos2D = mul(g_xCamera_VP, float4(ICOSPHERE[vid].xyz, 0));
	Out.nor = ICOSPHERE[vid].xyz;
	Out.pos2DPrev = mul(g_xFrame_MainCamera_PrevVP, float4(ICOSPHERE[vid].xyz, 0));
	return Out;
}
