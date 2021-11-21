#include "volumetricLightHF.hlsli"

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out;

	vertexID_create_fullscreen_triangle(vid, Out.pos);

	Out.pos2D = Out.pos;

	return Out;
}
