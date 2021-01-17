#define OBJECTSHADER_LAYOUT_POS_TEX
#include "objectHF.hlsli"

void main(PixelInput input)
{
	const float2 UV_baseColorMap = g_xMaterial.uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
	clip(texture_basecolormap.Sample(sampler_linear_wrap, UV_baseColorMap).a - g_xMaterial.alphaTest);
}
