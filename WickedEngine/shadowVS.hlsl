#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "windHF.hlsli"


struct VertexOut
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

VertexOut main(Input_Shadow input)
{
	VertexOut Out = (VertexOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
		
	Out.pos = mul(input.pos, WORLD);
	affectWind(Out.pos.xyz, input.tex.w, input.id);

	Out.pos = mul(Out.pos, g_xCamera_VP);
	Out.tex = input.tex.xy;


	return Out;
}