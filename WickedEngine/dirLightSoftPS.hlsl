#include "lightHF.hlsli"

#define DIRECTIONALLIGHT_SOFT
#include "dirLightHF.hlsli"



LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	DEFERRED_DIRLIGHT_MAIN

	DEFERREDLIGHT_RETURN
}