#include "deferredLightHF.hlsli"
#include "fullScreenTriangleHF.hlsli"

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out;

	FullScreenTriangle(vid, Out.pos);
		
	Out.pos2D = Out.pos;
	Out.lightIndex = g_xMisc_int4[0];

	return Out;
}