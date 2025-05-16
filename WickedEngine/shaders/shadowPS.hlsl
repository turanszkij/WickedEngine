#define OBJECTSHADER_LAYOUT_SHADOW
#include "objectHF.hlsli"

void main(PixelInput input, out float exponential_shadow : SV_Target0)
{
	exponential_shadow = exp(-GetFrame().exponential_shadow_bias * input.pos.z);
}
