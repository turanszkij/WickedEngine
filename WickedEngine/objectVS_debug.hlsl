#include "globals.hlsli"

// Vertex layout declaration:
RAWBUFFER(vertexBuffer_POS, VBSLOT_0);

float4 main( uint vID : SV_VERTEXID ) : SV_POSITION
{
	// Custom fetch vertex buffer:
	const uint fetchAddress = vID * 16;
	float4 pos = asfloat(vertexBuffer_POS.Load4(fetchAddress));

	return mul(float4(pos.xyz, 1), g_xTransform);
}