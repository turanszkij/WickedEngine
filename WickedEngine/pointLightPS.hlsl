#include "deferredLightHF.hlsli"

LightOutputType main(VertexToPixel PSIn)
{ 
	DEFERREDLIGHT_MAKEPARAMS

	DEFERREDLIGHT_POINT

	DEFERREDLIGHT_RETURN
}