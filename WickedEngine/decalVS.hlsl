#include "cube.hlsli"
#include "globals.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
};

VertexToPixel main( uint vid : SV_VERTEXID )
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = CUBE[vid];
	Out.pos = Out.pos2D = mul(pos, g_xTransform);

	return Out;
}