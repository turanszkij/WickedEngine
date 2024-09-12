struct VertexInput
{
	float2 pos		: POSITION;
	float2 uv		: TEXCOORD0;
	float4 col		: COLOR0;
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
	float4 col : COLOR0;
};

cbuffer vertexBuffer : register(b0) 
{
	float4x4 ProjectionMatrix; 
};

[RootSignature("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), CBV(b0), DescriptorTable(SRV(t0)), DescriptorTable(Sampler(s0))")]
VertexOutput main(VertexInput input)
{
	VertexOutput output;
	output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
	output.uv = input.uv;
	output.col = input.col; 
	return output;
}
