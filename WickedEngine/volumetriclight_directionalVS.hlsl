#include "volumetricLightHF.hlsli"

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out;

	FullScreenTriangle(vid, Out.pos);

	Out.pos2D = Out.pos;

	return Out;
}
