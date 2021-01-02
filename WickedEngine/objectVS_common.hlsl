#include "objectHF.hlsli"



PixelInputType main(Input_Object_ALL input)
{
	PixelInputType Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	float4x4 WORLDPREV = MakeWorldMatrixFromInstance(input.instPrev);

	VertexSurface surface;
	surface.create(g_xMaterial, input);

	surface.position = mul(WORLD, surface.position);
	surface.positionPrev = mul(WORLDPREV, surface.positionPrev);
	surface.normal = normalize(mul((float3x3)WORLD, surface.normal));
	surface.tangent.xyz = normalize(mul((float3x3)WORLD, surface.tangent.xyz));

	Out.clip = dot(surface.position, g_xCamera_ClipPlane);

	Out.pos = mul(g_xCamera_VP, surface.position);
	Out.pos2DPrev = mul(g_xCamera_PrevVP, surface.positionPrev);
	Out.pos3D = surface.position.xyz;
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;
	Out.atl = surface.atlas;
	Out.nor = surface.normal;
	Out.tan = surface.tangent;

	return Out;
}
