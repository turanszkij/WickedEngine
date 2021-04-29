// This geometry shader is intended as fallback support when GPU doesn't support writing to 
//	SV_RenderTargetArrayIndex from Vertex Shader stage

struct GSInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	float2 atl : ATLAS;
	float3 nor : NORMAL;
	float4 tan : TANGENT;
	float3 pos3D : WORLDPOSITION;
	uint RTIndex : RTINDEX;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	float2 atl : ATLAS;
	float3 nor : NORMAL;
	float4 tan : TANGENT;
	float3 pos3D : WORLDPOSITION;
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
		element.color = input[i].color;
		element.uvsets = input[i].uvsets;
		element.atl = input[i].atl;
		element.nor = input[i].nor;
		element.tan = input[i].tan;
		element.pos3D = input[i].pos3D;
		element.RTIndex = input[i].RTIndex;
		output.Append(element);
	}
}