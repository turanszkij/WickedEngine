#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#include "objectHF.hlsli"

void main(PixelInput input)
{
	[branch]
	if (GetMaterial().textures[BASECOLORMAP].IsValid())
	{
		clip(GetMaterial().textures[BASECOLORMAP].Sample(sampler_point_wrap, input.GetUVSets()).a - GetMaterial().alphaTest);
	}
}
