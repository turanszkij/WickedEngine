#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#define OBJECTSHADER_USE_COLOR
#include "objectHF.hlsli"

[earlydepthstencil]
void main(PixelInput input, out float exponential_shadow : SV_Target0, out float4 transparent_shadow : SV_Target1)
{
	ShaderMaterial material = GetMaterial();
	
	float4 uvsets = input.GetUVSets();

	half4 color;
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

	half opacity = color.a;

	color.rgb = 1; // disable water shadow because it has already fog

	color.rgb += texture_caustics.SampleLevel(sampler_linear_mirror, uvsets.xy, 0).rgb;

	color.a = input.pos.z; // secondary depth
	
	exponential_shadow = exp(-GetFrame().exponential_shadow_bias * input.pos.z);
	transparent_shadow = color;
}
