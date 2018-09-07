#include "objectHF.hlsli"


struct HullInputType
{
	float3 pos								: POSITION;
	float3 posPrev							: POSITIONPREV;
	float4 tex								: TEXCOORD0;
	float4 nor								: NORMAL;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	nointerpolation float dither			: DITHER;
};


HullInputType main(Input_Object_ALL input)
{
	HullInputType Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	surface.position = mul(surface.position, WORLD);
	surface.normal = normalize(mul(surface.normal, (float3x3)WORLD));

	Out.pos = surface.position.xyz;
	Out.tex = surface.uv.xyxy;

	Out.nor = float4(surface.normal, 1);

	// todo: leave these but I'm lazy to create appropriate hull/domain shaders now...
	Out.posPrev = 0;
	Out.instanceColor = 0;
	Out.dither = 0;

	return Out;
}