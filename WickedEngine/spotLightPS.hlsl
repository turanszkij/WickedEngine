#include "deferredLightHF.hlsli"

LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	DEFERREDLIGHT_SPOT

	DEFERREDLIGHT_RETURN
}