#include "blurHF.hlsli"

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,0);
	float2 texelSize = weightTexelStren.y * weightTexelStren.z;
	color += xTexture.Sample(Sampler, PSIn.tex + float2(0.0f,-4.0f) * texelSize) * weightTexelStren.x;
	color += xTexture.Sample(Sampler, PSIn.tex + float2(0.0f,-3.0f) * texelSize) * weight.w;
	color += xTexture.Sample(Sampler, PSIn.tex + float2(0.0f,-2.0f) * texelSize) * weight.z;
	color += xTexture.Sample(Sampler, PSIn.tex + float2(0.0f,-1.0f) * texelSize) * weight.y;
	color += xTexture.Sample(Sampler, PSIn.tex) * weight.x;
	color += xTexture.Sample(Sampler, PSIn.tex + float2(0.0f,1.0f) * texelSize) * weight.y;
	color += xTexture.Sample(Sampler, PSIn.tex + float2(0.0f,2.0f) * texelSize) * weight.z;
	color += xTexture.Sample(Sampler, PSIn.tex + float2(0.0f,3.0f) * texelSize) * weight.w;
	color += xTexture.Sample(Sampler, PSIn.tex + float2(0.0f,4.0f) * texelSize) * weightTexelStren.x;
	return color;
}