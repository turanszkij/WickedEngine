#include "lightHF.hlsli"
#include "spotLightHF.hlsli"


LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	DEFERRED_SPOTLIGHT_MAIN

	DEFERREDLIGHT_RETURN
}