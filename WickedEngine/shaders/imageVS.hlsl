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

	// Since the corners are push constants, they cannot be indexed dynamically
	//	(This was only a problem on AMD in practice)
	switch (vI)
	{
	default:
	case 0:
		Out.pos = push.corners0;
		break;
	case 1:
		Out.pos = push.corners1;
		break;
	case 2:
		Out.pos = push.corners2;
		break;
	case 3:
		Out.pos = push.corners3;
		break;
	}
	Out.uv_screen = Out.pos;

	// Set up inverse bilinear interpolation
	Out.q = Out.pos.xy - push.corners0.xy;
	Out.b1 = push.corners1.xy - push.corners0.xy;
	Out.b2 = push.corners2.xy - push.corners0.xy;
	Out.b3 = push.corners0.xy - push.corners1.xy - push.corners2.xy + push.corners3.xy;

	return Out;
}

