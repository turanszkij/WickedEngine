#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#define OBJECTSHADER_USE_COLOR
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(PixelInput input) : SV_TARGET
{
	ShaderMaterial material = GetMaterial();
	
	float4 uvsets = input.GetUVSets();
	float2 pixel = input.pos.xy;

	float4 color;
	[branch]
	if (material.textures[BASECOLORMAP].IsValid())
	{
		color = material.textures[BASECOLORMAP].Sample(sampler_objectshader, uvsets);
	}
	else
	{
		color = 1;
	}
	color *= input.color;

	float opacity = color.a;

	color.rgb = 1; // disable water shadow because it has already fog

	color.rgb += texture_caustics.SampleLevel(sampler_linear_mirror, uvsets.xy, 0).rgb;

	color.a = input.pos.z; // secondary depth

	return color;
}
