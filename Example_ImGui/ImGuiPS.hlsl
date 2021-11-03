struct VertexOutput
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

sampler sampler0; 
Texture2D texture0; 

float4 main(VertexOutput input) : SV_TARGET
{
	return input.col * texture0.Sample(sampler0, input.uv);;
}
