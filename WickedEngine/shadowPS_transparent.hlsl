#include "globals.hlsli"
#include "objectHF.hlsli"

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float4 color			: COLOR;
	float4 uvsets			: UVSETS;
};

float4 main(VertextoPixel input) : SV_TARGET
{
	float2 pixel = input.pos.xy;

	float4 color;
	[branch]
	if (g_xMaterial.uvset_baseColorMap >= 0)
	{
		const float2 UV_baseColorMap = g_xMaterial.uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		color = texture_basecolormap.Sample(sampler_objectshader, UV_baseColorMap);
		color.rgb = DEGAMMA(color.rgb);
	}
	else
	{
		color = 1;
	}
	color *= input.color;

#ifndef DISABLE_ALPHATEST
	clip(color.a - g_xMaterial.alphaTest);
#endif // DISABLE_ALPHATEST

	float opacity = color.a;

	color.rgb *= 1 - opacity; // if fully opaque, then black (not let through any light)

	return color;
}
