#include "objectHF.hlsli"
#include "objectHF_PS.hlsli"

PixelOutputType main(PixelInputType input)
{
	OBJECT_PS_DITHER

	OBJECT_PS_MAKE

	OBJECT_PS_DEGAMMA

	OBJECT_PS_NORMALMAPPING

	OBJECT_PS_ENVIRONMENTMAPPING
		
	OBJECT_PS_OUT_GBUFFER
}

