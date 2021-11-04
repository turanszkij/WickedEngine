struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
	float4 col : COLOR0;
};

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0); 

float4 main(VertexOutput input) : SV_TARGET
{
	return input.col * texture0.Sample(sampler0, input.uv);
}
