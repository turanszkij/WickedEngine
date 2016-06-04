#include "lightHF.hlsli"
#include "pointLightHF.hlsli"


float4 main(VertexToPixel PSIn) : SV_TARGET
{ 
	DEFERREDLIGHT_MAKEPARAMS

	DEFERRED_POINTLIGHT_MAIN

	DEFERREDLIGHT_RETURN
}