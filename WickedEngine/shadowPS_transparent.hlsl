#define OBJECTSHADER_LAYOUT_POS_TEX
#define OBJECTSHADER_USE_COLOR
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(PixelInput input) : SV_TARGET
{
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

	float opacity = color.a;

	[branch]
	if (g_xMaterial.transmission > 0)
	{
		float transmission = g_xMaterial.transmission;
		[branch]
		if (g_xMaterial.uvset_transmissionMap >= 0)
		{
			const float2 UV_transmissionMap = g_xMaterial.uvset_transmissionMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			float transmissionMap = texture_transmissionmap.Sample(sampler_objectshader, UV_transmissionMap);
			transmission *= transmissionMap;
		}
		opacity *= 1 - transmission;
	}

	color.rgb *= 1 - opacity; // if fully opaque, then black (not let through any light)

	color.a = input.pos.z; // secondary depth

	return color;
}
