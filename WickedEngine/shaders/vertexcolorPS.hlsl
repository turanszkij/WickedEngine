struct VertexToPixel
{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR;
};

float4 main(VertexToPixel PSIn) : SV_TARGET
{
	return PSIn.col;
}
