#define OBJECTSHADER_LAYOUT_SHADOW
#include "objectHF.hlsli"

void main(PixelInput input, out float exponential_shadow : SV_Target0)
{
	exponential_shadow = exp(exponential_shadow_constant * input.pos.z);
}
