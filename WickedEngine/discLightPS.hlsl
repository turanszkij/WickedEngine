#include "deferredLightHF.hlsli"

LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	DEFERREDLIGHT_DISC

	DEFERREDLIGHT_RETURN
}