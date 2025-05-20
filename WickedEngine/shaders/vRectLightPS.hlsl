#include "globals.hlsli"

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
	float4 color = float4(xLightColor.rgb * xLightEnerdis.x / (2 * PI), 1); // 2*PI: balance the displaying of light visualizer with the diffuse strength of lighting.

	int maskTexDescriptor = (int)xLightEnerdis.y;
	if (maskTexDescriptor >= 0)
	{
		color *= bindless_textures[descriptor_index(maskTexDescriptor)].Sample(sampler_linear_clamp, uv);
	}

	return color;
}
