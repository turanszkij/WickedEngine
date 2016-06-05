#include "lightHF.hlsli"
#include "pointLightHF.hlsli"


LightOutputType main(VertexToPixel PSIn)
{ 
	DEFERREDLIGHT_MAKEPARAMS

	DEFERRED_POINTLIGHT_MAIN

	DEFERREDLIGHT_RETURN
}