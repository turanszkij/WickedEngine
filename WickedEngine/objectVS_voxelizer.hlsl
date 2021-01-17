#define OBJECTSHADER_LAYOUT_COMMON
#include "objectHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	float3 nor : NORMAL;
	uint emissiveColor : EMISSIVECOLOR;
};

VSOut main(VertexInput input)
{
	VSOut Out;

	float4x4 WORLD = input.GetInstanceMatrix();

	VertexSurface surface;
	surface.create(g_xMaterial, input);

	Out.pos = mul(WORLD, surface.position);
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;
	Out.nor = normalize(mul((float3x3)WORLD, surface.normal));
	Out.emissiveColor = surface.emissiveColor;

	return Out;
}

