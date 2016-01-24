#include "imageHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	//return float4(xTexture.SampleLevel(Sampler, PSIn.tex, 4).rgb,1);
	float4 color = float4(0,0,0,0);
	float2 texelSize = xPPParams1[3] * xPPParams1[1];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, -4.0f) * texelSize, xPPParams1[2]) * xPPParams1[0];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, -3.0f) * texelSize, xPPParams1[2]) * xPPParams0[3];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, -2.0f) * texelSize, xPPParams1[2]) * xPPParams0[2];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, -1.0f) * texelSize, xPPParams1[2]) * xPPParams0[1];
	color += xTexture.SampleLevel(Sampler, PSIn.tex, xPPParams1[2]) * xPPParams0[0];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, 1.0f) * texelSize, xPPParams1[2]) * xPPParams0[1];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, 2.0f) * texelSize, xPPParams1[2]) * xPPParams0[2];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, 3.0f) * texelSize, xPPParams1[2]) * xPPParams0[3];
	color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, 4.0f) * texelSize, xPPParams1[2]) * xPPParams1[0];
	return color;
}