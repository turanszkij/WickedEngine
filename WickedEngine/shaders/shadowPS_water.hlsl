#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#define OBJECTSHADER_USE_COLOR
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(PixelInput input) : SV_TARGET
{
	float2 pixel = input.pos.xy;

	float4 color;
	[branch]
	if (GetMaterial().textures[BASECOLORMAP].IsValid())
	{
		color = GetMaterial().textures[BASECOLORMAP].Sample(sampler_objectshader, input.uvsets);
	}
	else
	{
		color = 1;
	}
	color *= input.color;

	float opacity = color.a;

	color.rgb = 1; // disable water shadow because it has already fog

	color.rgb += caustic_pattern(input.uvsets.xy * 20, GetFrame().time);

	color.a = input.pos.z; // secondary depth

	return color;
}
