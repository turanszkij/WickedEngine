#include "globals.hlsli"
// This geometry shader is intended as fallback support when GPU doesn't support writing to 
//	SV_RenderTargetArrayIndex from Vertex Shader stage

struct GSInput
{
	float4 pos : SV_POSITION;
	uint instanceIndex_dither : INSTANCEINDEX_DITHER;
	float4 uvsets : UVSETS;
	half4 color : COLOR;
	half4 tan : TANGENT;
	float3 nor : NORMAL;
	half2 atl : ATLAS;
	float3 pos3D : WORLDPOSITION;
	half ao : AMBIENT_OCCLUSION;
	uint RTIndex : RTINDEX;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	uint instanceIndex_dither : INSTANCEINDEX_DITHER;
	float4 uvsets : UVSETS;
	half4 color : COLOR;
	half4 tan : TANGENT;
	float3 nor : NORMAL;
	half2 atl : ATLAS;
	float3 pos3D : WORLDPOSITION;
	half ao : AMBIENT_OCCLUSION;
	uint RTIndex : SV_RenderTargetArrayIndex;
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
		element.instanceIndex_dither = input[i].instanceIndex_dither;
		element.color = input[i].color;
		element.uvsets = input[i].uvsets;
		element.atl = input[i].atl;
		element.nor = input[i].nor;
		element.ao = input[i].ao;
		element.tan = input[i].tan;
		element.pos3D = input[i].pos3D;
		element.RTIndex = input[i].RTIndex;
		output.Append(element);
	}
}
