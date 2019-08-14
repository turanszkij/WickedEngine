#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"


struct VertexOut
{
	float4 pos	: SV_POSITION;
};

VertexOut main(Input_Object_POS input)
{
	VertexOut Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);
		
	Out.pos = mul(WORLD, surface.position);
	Out.pos = mul(g_xCamera_VP, Out.pos);

	return Out;
}