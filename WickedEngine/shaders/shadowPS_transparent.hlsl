#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#define OBJECTSHADER_USE_COLOR
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(PixelInput input) : SV_TARGET
{
	ShaderMaterial material = GetMaterial();

	float4 uvsets = input.GetUVSets();
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
	
	[branch]
	if (material.textures[TRANSPARENCYMAP].IsValid())
	{
		color.a *= material.textures[TRANSPARENCYMAP].Sample(sampler_objectshader, uvsets).r;
	}
	
	color *= input.color;
	
	ShaderMeshInstance meshinstance = load_instance(input.GetInstanceIndex());

	clip(color.a - material.GetAlphaTest() - meshinstance.alphaTest);

	float opacity = color.a;

	[branch]
	if (material.GetTransmission() > 0)
	{
		float transmission = material.GetTransmission();
		[branch]
		if (material.textures[TRANSMISSIONMAP].IsValid())
		{
			float transmissionMap = material.textures[TRANSMISSIONMAP].Sample(sampler_objectshader, uvsets).r;
			transmission *= transmissionMap;
		}
		opacity *= 1 - transmission;
	}

	color.rgb *= 1 - opacity; // if fully opaque, then black (not let through any light)

	color.a = input.pos.z; // secondary depth

	return color;
}
