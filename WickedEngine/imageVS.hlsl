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
	Out.pos2D = Out.pos;

	Out.tex = float2(vI % 2, vI % 4 / 2);
	Out.tex.x = xMirror == 1 ? (1 - Out.tex.x) : Out.tex.x;
	Out.tex2 = Out.tex;
	Out.tex = Out.tex * xTexMulAdd.xy + xTexMulAdd.zw;
	Out.tex2 = Out.tex2 * xTexMulAdd2.xy + xTexMulAdd2.zw;

	return Out;
}

