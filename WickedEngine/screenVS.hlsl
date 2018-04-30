#include "imageHF.hlsli"
#include "fullScreenTriangleHF.hlsli"

VertexToPixelPostProcess main(uint vI : SV_VERTEXID)
{
	VertexToPixelPostProcess Out;

	FullScreenTriangle(vI, Out.pos, Out.tex);

	return Out;
}