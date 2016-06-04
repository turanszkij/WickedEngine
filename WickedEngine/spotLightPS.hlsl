#include "lightHF.hlsli"
#include "spotLightHF.hlsli"


float4 main(VertexToPixel PSIn) : SV_TARGET
{
	DEFERREDLIGHT_MAKEPARAMS

	DEFERRED_SPOTLIGHT_MAIN

	DEFERREDLIGHT_RETURN
}