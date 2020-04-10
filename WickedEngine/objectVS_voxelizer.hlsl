#include "objectHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	float3 nor : NORMAL;
};

VSOut main(Input_Object_ALL input)
{
	VSOut Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.pos = mul(WORLD, surface.position);
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;
	Out.nor = normalize(mul((float3x3)WORLD, surface.normal));

	return Out;
}