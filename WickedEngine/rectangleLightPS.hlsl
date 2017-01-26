#include "deferredLightHF.hlsli"

LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	DEFERREDLIGHT_RECTANGLE

	DEFERREDLIGHT_RETURN
}