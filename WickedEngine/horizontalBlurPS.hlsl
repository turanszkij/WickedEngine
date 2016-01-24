#include "imageHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,0);
	float2 texelSize = xPPParams[7] * xPPParams[5];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(-4.0f, 0.0f) * texelSize, xPPParams[6]) * xPPParams[4];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(-3.0f, 0.0f) * texelSize, xPPParams[6]) * xPPParams[3];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(-2.0f, 0.0f) * texelSize, xPPParams[6]) * xPPParams[2];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(-1.0f, 0.0f) * texelSize, xPPParams[6]) * xPPParams[1];
	color += xTexture.SampleLevel(Sampler, PSIn.tex, xPPParams[6]) * xPPParams[0];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(1.0f, 0.0f) * texelSize, xPPParams[6]) * xPPParams[1];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(2.0f, 0.0f) * texelSize, xPPParams[6]) * xPPParams[2];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(3.0f, 0.0f) * texelSize, xPPParams[6]) * xPPParams[3];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(4.0f, 0.0f) * texelSize, xPPParams[6]) * xPPParams[4];
	return color;
	//return xTexture.SampleLevel(Sampler, PSIn.tex, 0);
}