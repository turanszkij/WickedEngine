#include "cube.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 pos2D		: POSITION2D;
};

cbuffer decalCB:register(b0){
	float4x4 xWVP;
}

VertexToPixel main( uint vid : SV_VERTEXID )
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = float4(CUBE[vid],1);
	Out.pos = Out.pos2D = mul(pos,xWVP);

	return Out;
}