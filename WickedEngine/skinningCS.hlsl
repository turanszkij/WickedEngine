#include "globals.hlsli"
#include "skinningHF.hlsli"

RAWBUFFER(vertexBuffer_POS, VBSLOT_0);
RAWBUFFER(vertexBuffer_NOR, VBSLOT_1);
RAWBUFFER(vertexBuffer_WEI, VBSLOT_2);
RAWBUFFER(vertexBuffer_BON, VBSLOT_3);

RWRAWBUFFER(streamoutBuffer_POS, 0);
RWRAWBUFFER(streamoutBuffer_NOR, 1);
RWRAWBUFFER(streamoutBuffer_PRE, 2);

[numthreads(SKINNING_COMPUTE_THREADCOUNT, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint fetchAddress = DTid.x * 16;

	uint4 pos_u = vertexBuffer_POS.Load4(fetchAddress);
	uint4 nor_u = vertexBuffer_NOR.Load4(fetchAddress);
	uint4 wei_u = vertexBuffer_WEI.Load4(fetchAddress);
	uint4 bon_u = vertexBuffer_BON.Load4(fetchAddress);
	uint4 pre_u;

	float4 pos = asfloat(pos_u);
	float4 nor = asfloat(nor_u);
	float4 wei = asfloat(wei_u);
	float4 bon = asfloat(bon_u);
	float4 pre = pos;

	Skinning(pos, pre, nor, bon, wei);

	pos_u =	asuint(pos);
	nor_u = asuint(nor);
	pre_u =	asuint(pre);

	streamoutBuffer_POS.Store4(fetchAddress, pos_u);
	streamoutBuffer_NOR.Store4(fetchAddress, nor_u);
	streamoutBuffer_PRE.Store4(fetchAddress, pre_u);
}