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

		float2 distortionCo;
			distortionCo.x = PSIn.dis.x/PSIn.dis.w/2.0f + 0.5f;
			distortionCo.y = -PSIn.dis.y/PSIn.dis.w/2.0f + 0.5f;
		float2 distort = (float2(0.5f,0.5f)-xDistortionTexture.Sample(xSampler,PSIn.tex).rg)*0.3f*PSIn.col.a;
		color.rgb += xRefracTexture.Sample(xSampler,distortionCo+distort).rgb;

	return color;
}