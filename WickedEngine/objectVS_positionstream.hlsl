#include "objectHF.hlsli"

float4 main(Input_Object_POS input) : SV_POSITION
{
	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	surface.position = mul(WORLD, surface.position);

	return mul(g_xCamera_VP, surface.position);
}
