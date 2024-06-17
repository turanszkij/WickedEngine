#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#include "objectHF.hlsli"

void main(PixelInput input)
{
	float alpha = 1;

	[branch]
	if (GetMaterial().textures[BASECOLORMAP].IsValid())
	{
		alpha = GetMaterial().textures[BASECOLORMAP].Sample(sampler_point_wrap, input.GetUVSets()).a;
	}
	
	[branch]
	if (GetMaterial().textures[TRANSPARENCYMAP].IsValid())
	{
		alpha *= GetMaterial().textures[TRANSPARENCYMAP].Sample(sampler_point_wrap, input.GetUVSets()).r;
	}
	
	ShaderMeshInstance meshinstance = load_instance(input.GetInstanceIndex());
	
	clip(alpha - GetMaterial().alphaTest - meshinstance.alphaTest);
}
