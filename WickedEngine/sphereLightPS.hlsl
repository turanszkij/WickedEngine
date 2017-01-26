#include "deferredLightHF.hlsli"

LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	DEFERREDLIGHT_SPHERE

	DEFERREDLIGHT_RETURN
}