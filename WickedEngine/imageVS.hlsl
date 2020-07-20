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

	Out.pos = imageCB.xCorners[vI];

	Out.uv0 = float2(vI % 2, vI % 4 / 2);
	Out.uv1 = Out.uv0;
	Out.uv0 = Out.uv0 * imageCB.xTexMulAdd.xy + imageCB.xTexMulAdd.zw;
	Out.uv1 = Out.uv1 * imageCB.xTexMulAdd2.xy + imageCB.xTexMulAdd2.zw;
	Out.uv_screen = Out.pos;

	return Out;
}

