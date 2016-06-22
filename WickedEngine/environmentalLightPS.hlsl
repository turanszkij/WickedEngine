#include "lightHF.hlsli"
#include "envReflectionHF.hlsli"



LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	DEFERREDLIGHT_ENVIRONMENTALLIGHT

	DEFERREDLIGHT_RETURN
}