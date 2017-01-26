#include "deferredLightHF.hlsli"

LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	DEFERREDLIGHT_TUBE

	DEFERREDLIGHT_RETURN
}