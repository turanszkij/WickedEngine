#include "globals.hlsli"
#include "imageHF.hlsli"

VertextoPixel main(uint vI : SV_VERTEXID)
{
	VertextoPixel Out;

	// This vertex shader generates a trianglestrip like this:
	//	1--2
	//	  /
	//	 /
	//	3--4

	Out.pos = push.corners[vI];
	Out.uv_screen = Out.pos;

	// Set up inverse bilinear interpolation
	Out.q = Out.pos.xy - push.corners[0].xy;
	Out.b1 = push.corners[1].xy - push.corners[0].xy;
	Out.b2 = push.corners[2].xy - push.corners[0].xy;
	Out.b3 = push.corners[0].xy - push.corners[1].xy - push.corners[2].xy + push.corners[3].xy;

	return Out;
}

