#include "objectHF.hlsli"

struct VertexOut
{
	float4 pos				: SV_POSITION;
	float4 color			: COLOR;
	float4 uvsets			: UVSETS;
};

VertexOut main(Input_Object_POS_TEX input)
{
	VertexOut Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	surface.position = mul(WORLD, surface.position);

	Out.pos = mul(g_xCamera_VP, surface.position);
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;

	return Out;
}