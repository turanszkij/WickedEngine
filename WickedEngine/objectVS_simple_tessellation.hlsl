#include "objectHF.hlsli"


struct HullInputType
{
	float4 pos								: POSITION;
	float4 color							: COLOR;
	float4 uvsets							: UVSETS;
	float4 atlas							: ATLAS;
	float4 nor								: NORMAL;
	float4 posPrev							: POSITIONPREV;
};


HullInputType main(Input_Object_ALL input)
{
	HullInputType Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	surface.position = mul(WORLD, surface.position);
	surface.normal = normalize(mul((float3x3)WORLD, surface.normal));

	Out.pos = surface.position;
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;
	Out.atlas = surface.atlas.xyxy;
	Out.nor = float4(surface.normal, 1);

	// todo: leave these but I'm lazy to create appropriate hull/domain shaders now...
	Out.posPrev = 0;

	return Out;
}