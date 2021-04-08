#define OBJECTSHADER_LAYOUT_COMMON
#include "objectHF.hlsli"

float4 main(PixelInput input) : SV_Target0
{
	float4 color;
	[branch]
	if (GetMaterial().uvset_baseColorMap >= 0)
	{
		const float2 UV_baseColorMap = GetMaterial().uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		color = texture_basecolormap.Sample(sampler_objectshader, UV_baseColorMap);
		color.rgb = DEGAMMA(color.rgb);
	}
	else
	{
		color = 1;
	}
	color *= input.color;
	clip(color.a - 0.5);
	color.a = 1;

	return color;
}

