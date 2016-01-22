#include "imageHF.hlsli"

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,0);
	//float2 texelSize = weightTexelStrenMip.y * weightTexelStrenMip.z;
	//color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, -4.0f) * texelSize, weightTexelStrenMip.w) * weightTexelStrenMip.x;
	//color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, -3.0f) * texelSize, weightTexelStrenMip.w) * weight.w;
	//color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, -2.0f) * texelSize, weightTexelStrenMip.w) * weight.z;
	//color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, -1.0f) * texelSize, weightTexelStrenMip.w) * weight.y;
	//color += xTexture.SampleLevel(Sampler, PSIn.tex, weightTexelStrenMip.w) * weight.x;
	//color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, 1.0f) * texelSize, weightTexelStrenMip.w) * weight.y;
	//color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, 2.0f) * texelSize, weightTexelStrenMip.w) * weight.z;
	//color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, 3.0f) * texelSize, weightTexelStrenMip.w) * weight.w;
	//color += xTexture.SampleLevel(Sampler, PSIn.tex + float2(0.0f, 4.0f) * texelSize, weightTexelStrenMip.w) * weightTexelStrenMip.x;
	return color;
}