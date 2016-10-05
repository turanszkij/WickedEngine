#include "deferredLightHF.hlsli"

LightOutputType main( VertexToPixel PSIn )
{
	DEFERREDLIGHT_MAKEPARAMS

	DEFERREDLIGHT_DIRECTIONAL

	DEFERREDLIGHT_RETURN
}