#include "objectHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
};

VSOut main(Input input)
{
	VSOut Out = (VSOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input);

	Out.pos = mul(input.pos, WORLD);
	Out.tex = input.tex.xy;


	return Out;
}