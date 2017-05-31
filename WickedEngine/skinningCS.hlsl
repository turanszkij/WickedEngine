#include "globals.hlsli"
#include "skinningHF.hlsli"

TYPEDBUFFER(vertexBuffer_POS, float4, VBSLOT_0);
TYPEDBUFFER(vertexBuffer_NOR, float4, VBSLOT_1);
TYPEDBUFFER(vertexBuffer_WEI, float4, VBSLOT_2);
TYPEDBUFFER(vertexBuffer_BON, float4, VBSLOT_3);

RWTYPEDBUFFER(streamoutBuffer_POS, float4, 0);
RWTYPEDBUFFER(streamoutBuffer_NOR, float4, 1);
RWTYPEDBUFFER(streamoutBuffer_PRE, float4, 2);

[numthreads(SKINNING_COMPUTE_THREADCOUNT, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint vertexID = DTid.x;

	float4 pos = vertexBuffer_POS[vertexID];
	float4 pre = pos;
	float4 nor = vertexBuffer_NOR[vertexID];
	float4 wei = vertexBuffer_WEI[vertexID];
	float4 bon = vertexBuffer_BON[vertexID];

	Skinning(pos, pre, nor, bon, wei);

	streamoutBuffer_POS[vertexID] = pos;
	streamoutBuffer_NOR[vertexID] = nor;
	streamoutBuffer_PRE[vertexID] = pre;
}