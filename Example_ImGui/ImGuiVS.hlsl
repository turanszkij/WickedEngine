struct VertexInput
{
	float2 pos		: POSITION;
	float2 col		: COLOR0;
	float2 uv		: TEXCOORD0;
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

cbuffer vertexBuffer : register(b0) 
{
	float4x4 ProjectionMatrix; 
};

VertexOutput main(VertexInput input)
{
	VertexOutput output;
	output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
	output.col = input.col; 
	output.uv = input.uv;
	return Out;
}
