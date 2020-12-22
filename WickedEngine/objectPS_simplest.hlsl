#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(PixelInputType_Simple PSIn) : SV_TARGET
{
	return PSIn.color;
}

