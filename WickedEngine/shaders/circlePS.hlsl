struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
};

float4 main(GSOutput PSIn) : SV_TARGET
{
	return float4(PSIn.col.rgb,1);
}