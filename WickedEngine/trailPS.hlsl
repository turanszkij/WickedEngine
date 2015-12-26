Texture2D<float4> xRefracTexture:register(t0);
Texture2D<float4> xDistortionTexture:register(t1);
SamplerState xSampler:register(s0);

struct VertexToPixel{
	float4 pos						: SV_POSITION;
	float2 tex						: TEXCOORD0;
	float4 col						: TEXCOORD1;
	float4 dis						: TEXCOORD2;
};

float4 main(VertexToPixel PSIn) : SV_TARGET
{
	float4 color = float4(PSIn.col.rgb,1);

	float2 distortTexDim;
	xDistortionTexture.GetDimensions(distortTexDim.x, distortTexDim.y);

	float2 distortionCo;
		distortionCo.x = PSIn.dis.x/PSIn.dis.w/2.0f + 0.5f;
		distortionCo.y = -PSIn.dis.y/PSIn.dis.w/2.0f + 0.5f;
	float2 distort = (xDistortionTexture.Sample(xSampler,PSIn.tex/float2(distortTexDim.x,1)).rg - float2(0.5f, 0.5f))*0.3f*PSIn.col.a;
	color.rgb += xRefracTexture.SampleLevel(xSampler,distortionCo+distort,0).rgb;

	return float4(color.rgb,1);
}