#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#define OBJECTSHADER_USE_COLOR
#include "objectHF.hlsli"
#include "ShaderInterop_Renderer.h"

[earlydepthstencil]
float4 main(PixelInput input) : SV_TARGET
{
	float4 uvsets = input.GetUVSets();
	const float2 pixel = frac((xPaintRadUVSET == 0 ? uvsets.xy : uvsets.zw)) * xPaintRadResolution;

	const float2x2 rot = float2x2(
		cos(xPaintRadBrushRotation), -sin(xPaintRadBrushRotation),
		sin(xPaintRadBrushRotation), cos(xPaintRadBrushRotation)
		);
	const float2 diff = mul((xPaintRadCenter % xPaintRadResolution) - pixel, rot);

	float dist = 0;
	switch (xPaintRadBrushShape)
	{
	default:
	case 0:
		dist = length(diff);
		break;
	case 1:
		dist = max(abs(diff.x), abs(diff.y));
		break;
	}

	float shape = dist - xPaintRadRadius;
	float3 color = shape < 0 ? float3(0, 0, 0) : float3(1, 1, 1);
	float alpha = 1 - saturate(abs(shape) * 0.25f);

	return float4(color, alpha);
}
