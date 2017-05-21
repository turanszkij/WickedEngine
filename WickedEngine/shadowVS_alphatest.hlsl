#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "windHF.hlsli"


struct VertexOut
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

VertexOut main(Input_Shadow_POS_TEX input)
{
	VertexOut Out = (VertexOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	Out.pos = mul(float4(input.pos.xyz, 1), WORLD);
	affectWind(Out.pos.xyz, input.pos.w, g_xFrame_Time);

	Out.pos = mul(Out.pos, g_xCamera_VP);
	Out.tex = input.tex.xy;


	return Out;
}