#include "postProcessHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 center = xTexture.Load(uint3(PSIn.pos.xy + float2(0, 0),		0));
	float4 top =	xTexture.Load(uint3(PSIn.pos.xy + float2(0, -1),	0));
	float4 left =	xTexture.Load(uint3(PSIn.pos.xy + float2(-1, 0),	0));
	float4 right =	xTexture.Load(uint3(PSIn.pos.xy + float2(1, 0),		0));
	float4 bottom = xTexture.Load(uint3(PSIn.pos.xy + float2(0, 1),		0));

	return saturate(center + (4 * center - top - bottom - left - right) * xPPParams0[0]);
}