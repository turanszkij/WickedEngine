#include "globals.hlsli"

RWRAWBUFFER(LightIndexCounter, UAVSLOT_LIGHTINDEXCOUNTERHELPER);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	LightIndexCounter.Store2(0, uint2(0, 0));
}