#include "postProcessHF.hlsli"

// This will cut out bright parts (>1) and also downsample 4x

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float2 dim;
	xTexture.GetDimensions(dim.x, dim.y);
	dim = 1.0 / dim;

	float3 color = 0;

	color += xTexture.SampleLevel(sampler_linear_clamp, PSIn.tex + float2(-1, -1) * dim, 0).rgb;
	color += xTexture.SampleLevel(sampler_linear_clamp, PSIn.tex + float2(1, -1) * dim, 0).rgb;
	color += xTexture.SampleLevel(sampler_linear_clamp, PSIn.tex + float2(-1, 1) * dim, 0).rgb;
	color += xTexture.SampleLevel(sampler_linear_clamp, PSIn.tex + float2(1, 1) * dim, 0).rgb;

	color /= 4.0f;

	const float bloomThreshold = xPPParams0.x;
	color = max(0, color - bloomThreshold);

	return float4(color, 1);
}