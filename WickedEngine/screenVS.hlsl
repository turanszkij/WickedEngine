#include "imageHF.hlsli"
#include "fullScreenTriangleHF.hlsli"

VertexToPixelPostProcess main(uint vI : SV_VERTEXID)
{
	VertexToPixelPostProcess Out;

	FullScreenTriangle(vI, Out.pos, Out.tex);

#ifdef SHADERCOMPILER_SPIRV
	Out.pos.y = -Out.pos.y;
#endif 

	return Out;
}