#include "objectHF.hlsli"

PixelInputType main(Input_Object_POS_TEX input)
{
	PixelInputType Out;
	
	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);
		
	surface.position = mul(WORLD, surface.position);
	surface.normal = normalize(mul((float3x3)WORLD, surface.normal));

	Out.clip = 0;
		
	Out.pos = Out.pos2D = mul(g_xCamera_VP, surface.position);
	Out.pos2DPrev = Out.pos2D; // no need for water
	Out.pos3D = surface.position.xyz;
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;
	Out.atl = 0;
	Out.nor = surface.normal;

	return Out;
}