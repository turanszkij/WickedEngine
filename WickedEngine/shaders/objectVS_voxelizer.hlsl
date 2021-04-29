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
	VertexSurface surface;
	surface.create(GetMaterial(), input);

	VSOut Out;
	Out.pos = surface.position;
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;
	Out.nor = surface.normal;
	Out.emissiveColor = surface.emissiveColor;

	return Out;
}

