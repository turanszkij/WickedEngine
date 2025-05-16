#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#include "objectHF.hlsli"

void main(PixelInput input, out float exponential_shadow : SV_Target0)
{
	ShaderMaterial material = GetMaterial();
	
	half alpha = 1;

	[branch]
	if (material.textures[BASECOLORMAP].IsValid())
	{
		alpha = material.textures[BASECOLORMAP].Sample(sampler_point_wrap, input.GetUVSets()).a;
	}
	
	[branch]
	if (material.textures[TRANSPARENCYMAP].IsValid())
	{
		alpha *= material.textures[TRANSPARENCYMAP].Sample(sampler_point_wrap, input.GetUVSets()).r;
	}
	
	ShaderMeshInstance meshinstance = load_instance(input.GetInstanceIndex());
	
	clip(alpha - material.GetAlphaTest() - meshinstance.GetAlphaTest());
	
	exponential_shadow = exp(exponential_shadow_constant * input.pos.z);
}
