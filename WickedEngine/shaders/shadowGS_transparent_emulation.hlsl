#include "globals.hlsli"
// This geometry shader is intended as fallback support when GPU doesn't support writing to 
//	SV_ViewportArrayIndex from Vertex Shader stage

struct GSInput
{
	float4 pos : SV_POSITION;
	float4 uvsets : UVSETS;
	half4 color : COLOR;
	uint VPIndex : VPINDEX;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 uvsets : UVSETS;
	half4 color : COLOR;
	uint VPIndex : SV_ViewportArrayIndex;
};

[maxvertexcount(3)]
void main(
	triangle GSInput input[3],
	inout TriangleStream< GSOutput > output
)
{
	for (uint i = 0; i < 3; i++)
	{
		GSOutput element;
		element.pos = input[i].pos;
		element.color = input[i].color;
		element.uvsets = input[i].uvsets;
		element.VPIndex = input[i].VPIndex;
		output.Append(element);
	}
}
