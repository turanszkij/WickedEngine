#include "objectHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
};

VSOut main(Input input, uint instanceID : SV_INSTANCEID)
{
	VSOut Out = (VSOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input);

	Out.pos = mul(input.pos, WORLD);
	Out.nor = input.nor.xyz;
	Out.tex = input.tex.xy;

	return Out;
}