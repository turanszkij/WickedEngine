#include "lightHF.hlsli"
#include "quad.hlsli"

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	Out.pos = Out.pos2D = float4(QUAD[vid],1);
	return Out;
}