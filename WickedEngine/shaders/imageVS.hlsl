#include "globals.hlsli"
#include "imageHF.hlsli"

VertextoPixel main(uint vI : SV_VertexID)
{
	VertextoPixel Out;

	[branch]
	if (image_push.flags & IMAGE_FLAG_FULLSCREEN)
	{
		vertexID_create_fullscreen_triangle(vI, Out.pos);
	}
	else
	{
		// This vertex shader generates a trianglestrip like this:
		//	1--2
		//	  /
		//	 /
		//	3--4

		// If the corners are push constants, they cannot be indexed dynamically
		//	(This was only a problem on AMD in practice)
		switch (vI)
		{
		default:
		case 0:
			Out.pos = image.corners0;
			break;
		case 1:
			Out.pos = image.corners1;
			break;
		case 2:
			Out.pos = image.corners2;
			break;
		case 3:
			Out.pos = image.corners3;
			break;
		}

		// Set up inverse bilinear interpolation
		Out.q = Out.pos.xy - image.corners0.xy;
		Out.b1 = image.corners1.xy - image.corners0.xy;
		Out.b2 = image.corners2.xy - image.corners0.xy;
		Out.b3 = image.corners0.xy - image.corners1.xy - image.corners2.xy + image.corners3.xy;
	}

	return Out;
}

