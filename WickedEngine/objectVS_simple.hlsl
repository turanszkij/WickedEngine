#include "objectHF.hlsli"

PixelInputType_Simple main(Input_Object_POS_TEX input)
{
	PixelInputType_Simple Out;
	
	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	surface.position = mul(surface.position, WORLD);

	Out.clip = dot(surface.position, g_xClipPlane);

	Out.pos = mul(surface.position, g_xCamera_VP);
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;

	return Out;
}