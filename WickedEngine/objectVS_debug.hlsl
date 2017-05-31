#include "globals.hlsli"

// Vertex layout declaration:
TYPEDBUFFER(vertexBuffer_POS, float4, VBSLOT_0);

float4 main( uint vID : SV_VERTEXID ) : SV_POSITION
{
	// Custom fetch vertex buffer:
	float4 pos = vertexBuffer_POS[vID];

	return mul(float4(pos.xyz, 1), g_xTransform);
}