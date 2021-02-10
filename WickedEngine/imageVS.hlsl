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

	Out.pos = xCorners[vI];
	Out.uv_screen = Out.pos;

	// Set up inverse bilinear interpolation
	Out.q = Out.pos.xy - xCorners[0].xy;
	Out.b1 = xCorners[1].xy - xCorners[0].xy;
	Out.b2 = xCorners[2].xy - xCorners[0].xy;
	Out.b3 = xCorners[0].xy - xCorners[1].xy - xCorners[2].xy + xCorners[3].xy;

	return Out;
}

