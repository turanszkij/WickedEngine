#include "globals.hlsli"
#include "icosphere.hlsli"

struct VSOut {
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
};

VSOut main(uint vid : SV_VERTEXID)
{
	VSOut Out = (VSOut)0;
	Out.pos = mul(float4(ICOSPHERE[vid], 0), g_xCamera_ReflVP);
	Out.nor = ICOSPHERE[vid];
	return Out;
}