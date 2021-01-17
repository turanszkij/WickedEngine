#include "objectHF.hlsli"

PixelInputType_Simple main(Input_Object_POS input)
{
	PixelInputType_Simple Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);

	VertexSurface surface;
	surface.create(g_xMaterial, input);

	surface.position = mul(WORLD, surface.position);

	Out.clip = dot(surface.position, g_xCamera_ClipPlane);

	Out.pos = mul(g_xCamera_VP, surface.position);
	Out.color = 0;
	Out.uvsets = 0;
	Out.pos2DPrev = mul(g_xCamera_PrevVP, surface.position);

	return Out;
}
