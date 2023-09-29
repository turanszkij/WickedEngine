#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#define OBJECTSHADER_USE_COLOR
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(PixelInput input) : SV_TARGET
{
	float4 uvsets = input.GetUVSets();
	float4 color;
	[branch]
	if (GetMaterial().textures[BASECOLORMAP].IsValid())
	{
		color = GetMaterial().textures[BASECOLORMAP].Sample(sampler_objectshader, uvsets);
	}
	else
	{
		color = 1;
	}
	color *= input.color;

	clip(color.a - GetMaterial().alphaTest);

	float opacity = color.a;

	[branch]
	if (GetMaterial().transmission > 0)
	{
		float transmission = GetMaterial().transmission;
		[branch]
		if (GetMaterial().textures[TRANSMISSIONMAP].IsValid())
		{
			float transmissionMap = GetMaterial().textures[TRANSMISSIONMAP].Sample(sampler_objectshader, uvsets).r;
			transmission *= transmissionMap;
		}
		opacity *= 1 - transmission;
	}

	color.rgb *= 1 - opacity; // if fully opaque, then black (not let through any light)

	color.a = input.pos.z; // secondary depth

	return color;
}
