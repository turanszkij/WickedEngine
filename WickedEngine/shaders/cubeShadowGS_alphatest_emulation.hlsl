// This geometry shader is intended as fallback support when GPU doesn't support writing to 
//	SV_RenderTargetArrayIndex from Vertex Shader stage

struct GSInput
{
	float4 pos : SV_POSITION;
	float2 uv : UV;
	uint RTIndex : RTINDEX;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float2 uv : UV;
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
		element.uv = input[i].uv;
		element.RTIndex = input[i].RTIndex;
		output.Append(element);
	}
}
