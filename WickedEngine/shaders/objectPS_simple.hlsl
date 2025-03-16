#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#define OBJECTSHADER_USE_COLOR
#define OBJECTSHADER_USE_NORMAL // tessellation compat!
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(PixelInput input) : SV_TARGET
{
	return input.color;
}

