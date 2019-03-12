#include "objectHF.hlsli"


struct HullInputType
{
	float4 pos								: POSITION;
	float4 color							: COLOR;
	float4 tex								: TEXCOORD0;
	float4 nor								: NORMAL;
	float4 posPrev							: POSITIONPREV;
};


HullInputType main(Input_Object_ALL input)
{
	HullInputType Out;
	
	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	float4x4 WORLDPREV = MakeWorldMatrixFromInstance(input.instPrev);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	surface.position = mul(surface.position, WORLD);
	surface.prevPos = mul(surface.prevPos, WORLDPREV);
	surface.normal = normalize(mul(surface.normal, (float3x3)WORLD));

	Out.pos = surface.position;
	Out.color = surface.color;
	Out.tex = float4(surface.uv, surface.atlas);
	Out.nor = float4(surface.normal, 1);
	Out.posPrev = surface.prevPos;

	return Out;
}