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

	Out.uv0 = float2(vI % 2, vI % 4 / 2);
	Out.uv1 = Out.uv0;
	Out.uv0 = Out.uv0 * xTexMulAdd.xy + xTexMulAdd.zw;
	Out.uv1 = Out.uv1 * xTexMulAdd2.xy + xTexMulAdd2.zw;
	Out.uv_screen = Out.pos;

	return Out;
}

