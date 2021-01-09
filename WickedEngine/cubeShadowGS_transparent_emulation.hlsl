// This geometry shader is intended as fallback support when GPU doesn't support writing to 
//	SV_RenderTargetArrayIndex from Vertex Shader stage

struct GSInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	uint RTIndex : RTINDEX;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
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
		element.RTIndex = input[i].RTIndex;
		output.Append(element);
	}
}
