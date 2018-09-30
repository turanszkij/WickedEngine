#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"


struct VertexOut
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

VertexOut main(Input_Object_POS_TEX input)
{
	VertexOut Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.pos = mul(surface.position, WORLD);

	Out.pos = mul(Out.pos, g_xCamera_VP);
	Out.tex = surface.uv;

	return Out;
}