#include "objectHF.hlsli"

float4 main(Input_Object_POS input) : SV_POSITION
{
	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	surface.position = mul(surface.position, WORLD);

	affectWind(surface.position.xyz, surface.wind, g_xFrame_Time);

	float4 pos = mul(surface.position, g_xCamera_VP);

	return pos;
}
