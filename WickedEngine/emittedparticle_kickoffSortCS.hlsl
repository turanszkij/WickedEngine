#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(counterBuffer, ParticleCounters, 0);
RWRAWBUFFER(indirectBuffers, 1);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// read real alivecount from after simulation:
	int aliveCount_afterSimulation = indirectBuffers.Load(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES) / 6;

	// and store it for the sorting shaders to read:
	counterBuffer[0].aliveCount_afterSimulation = aliveCount_afterSimulation;

	// calculate threadcount:
	uint threadCount = ((aliveCount_afterSimulation - 1) >> 9) + 1;

	// and prepare to dispatch the sort for the alive simulated particles:
	indirectBuffers.Store3(ARGUMENTBUFFER_OFFSET_DISPATCHSORT, uint3(threadCount, 1, 1));
}