#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "windHF.hlsli"


struct VertexOut
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

VertexOut main(Input_Object_POS_TEX input)
{
	VertexOut Out = (VertexOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.pos = mul(surface.position, WORLD);
	affectWind(Out.pos.xyz, surface.wind, g_xFrame_Time);

	Out.pos = mul(Out.pos, g_xCamera_VP);
	Out.tex = surface.uv;

	return Out;
}