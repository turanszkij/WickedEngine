#define OBJECTSHADER_LAYOUT_SHADOW_TEX
#define OBJECTSHADER_USE_COLOR
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(PixelInput input) : SV_TARGET
{
	return input.color;
}

