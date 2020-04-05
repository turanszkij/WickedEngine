#include "objectHF.hlsli"
#include "ShaderInterop_Paint.h"

float4 main(PixelInputType_Simple PSIn) : SV_TARGET
{
	const float2 pixel = (xPaintRadUVSET == 0 ? PSIn.uvsets.xy : PSIn.uvsets.zw) * xPaintRadResolution;
	const float dist = distance(pixel, xPaintRadCenter);
	float circle = dist - xPaintRadRadius;
	float3 color = circle < 0 ? float3(0, 0, 0) : float3(1, 1, 1);
	float alpha = 1 - saturate(abs(circle) * 0.25f);

	return float4(color, alpha);
}
