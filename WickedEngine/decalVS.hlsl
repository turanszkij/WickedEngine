#include "globals.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
};

VertexToPixel main( uint vid : SV_VERTEXID )
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = float4(CreateCube(vid) * 2 - 1, 1);
	Out.pos = Out.pos2D = mul(g_xTransform, pos);

	return Out;
}