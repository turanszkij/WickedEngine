#include "objectHF.hlsli"


GBUFFEROutputType main(PixelInputType input)
{
	OBJECT_PS_DITHER

	OBJECT_PS_MAKE

	OBJECT_PS_SPECULARANTIALIASING
		
	OBJECT_PS_OUT_GBUFFER
}

