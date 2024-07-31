#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWByteAddressBuffer counterBuffer : register(u4);
RWByteAddressBuffer indirectBuffers : register(u5);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint aliveCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION);
	int deadCount = counterBuffer.Load<int>(PARTICLECOUNTER_OFFSET_DEADCOUNT);
	
	indirectBuffers.Store3(ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, uint3((aliveCount + THREADCOUNT_SIMULATION - 1) / THREADCOUNT_SIMULATION, 1, 1));
	
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_ALIVECOUNT, aliveCount);
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION, 0);
	
#ifdef __PSSL__
	counterBuffer.TypedStore<int>(PARTICLECOUNTER_OFFSET_DEADCOUNT, max(0, deadCount));
#else
	counterBuffer.Store<int>(PARTICLECOUNTER_OFFSET_DEADCOUNT, max(0, deadCount));
#endif // __PSSL__
	
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_CULLEDCOUNT, 0);
	
	counterBuffer.Store(PARTICLECOUNTER_OFFSET_CELLALLOCATOR, 0);
}
