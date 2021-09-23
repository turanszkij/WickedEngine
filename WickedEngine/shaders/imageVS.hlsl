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

	float4 corners[] = {
		unpack_half4(push.corners0),
		unpack_half4(push.corners1),
		unpack_half4(push.corners2),
		unpack_half4(push.corners3),
	};

	Out.pos = corners[vI];
	Out.uv_screen = Out.pos;

	// Set up inverse bilinear interpolation
	Out.q = Out.pos.xy - corners[0].xy;
	Out.b1 = corners[1].xy - corners[0].xy;
	Out.b2 = corners[2].xy - corners[0].xy;
	Out.b3 = corners[0].xy - corners[1].xy - corners[2].xy + corners[3].xy;

	return Out;
}

