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
	float2 inTex = float2(vI % 2, vI % 4 / 2);

	Out.pos = xCorners[vI];

	Out.tex_original = inTex;

	Out.tex = inTex * xTexMulAdd.xy + xTexMulAdd.zw;

	Out.tex.x = xMirror == 1 ? (1 - Out.tex.x) : Out.tex.x;

	Out.pos2D = Out.pos;

	return Out;
}

