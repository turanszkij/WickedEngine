#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float, TEXSLOT_ONDEMAND0);

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
	const float middle = input.SampleLevel(sampler_linear_clamp, uv, 0);

	const float outlineThickness = xPPParams0.y;
	const float4 outlineColor = xPPParams1;
	const float outlineThreshold = xPPParams0.x * middle;

	float2 dim;
	input.GetDimensions(dim.x, dim.y);

	float2 ox = float2(outlineThickness / dim.x, 0.0);
	float2 oy = float2(0.0, outlineThickness / dim.y);
	float2 PP = uv - oy;

	float CC = input.SampleLevel(sampler_linear_clamp, (PP - ox), 0);	float g00 = (CC);
	CC = input.SampleLevel(sampler_linear_clamp, PP, 0);				float g01 = (CC);
	CC = input.SampleLevel(sampler_linear_clamp, (PP + ox), 0);			float g02 = (CC);
	PP = uv;
	CC = input.SampleLevel(sampler_linear_clamp, (PP - ox), 0);			float g10 = (CC);
	CC = middle;														float g11 = (CC) * 0.01f;
	CC = input.SampleLevel(sampler_linear_clamp, (PP + ox), 0);			float g12 = (CC);
	PP = uv + oy;
	CC = input.SampleLevel(sampler_linear_clamp, (PP - ox), 0);			float g20 = (CC);
	CC = input.SampleLevel(sampler_linear_clamp, PP, 0);				float g21 = (CC);
	CC = input.SampleLevel(sampler_linear_clamp, (PP + ox), 0);			float g22 = (CC);
	float K00 = -1;
	float K01 = -2;
	float K02 = -1;
	float K10 = 0;
	float K11 = 0;
	float K12 = 0;
	float K20 = 1;
	float K21 = 2;
	float K22 = 1;
	float sx = 0;
	float sy = 0;
	sx += g00 * K00;
	sx += g01 * K01;
	sx += g02 * K02;
	sx += g10 * K10;
	sx += g11 * K11;
	sx += g12 * K12;
	sx += g20 * K20;
	sx += g21 * K21;
	sx += g22 * K22;
	sy += g00 * K00;
	sy += g01 * K10;
	sy += g02 * K20;
	sy += g10 * K01;
	sy += g11 * K11;
	sy += g12 * K21;
	sy += g20 * K02;
	sy += g21 * K12;
	sy += g22 * K22;
	float dist = sqrt(sx*sx + sy*sy);

	float edge = dist > outlineThreshold ? 1 : 0;

	return float4(outlineColor.rgb, outlineColor.a * edge);
}
