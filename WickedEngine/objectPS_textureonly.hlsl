#include "objectHF.hlsli"



float4 main(PixelInputType_Simple input) : SV_TARGET
{
	OBJECT_PS_DITHER

	OBJECT_PS_MAKE_SIMPLE

	OBJECT_PS_SAMPLETEXTURES_SIMPLE

	OBJECT_PS_DEGAMMA

	color.rgb += color.rgb * GetEmissive(emissive);

	OBJECT_PS_OUT_FORWARD_SIMPLE
}

