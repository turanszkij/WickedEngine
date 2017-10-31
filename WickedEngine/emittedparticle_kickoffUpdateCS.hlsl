#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(counterBuffer, ParticleCounters, 4);
RWRAWBUFFER(indirectBuffers, 5);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// Load dead particle count:
	uint deadCount = counterBuffer[0].deadCount;

	// Load alive particle count from draw argument buffer (which contains alive particle count * 6):
	uint aliveCount_NEW = indirectBuffers.Load(24) / 6;

	// we can not emit more than there are free slots in the dead list:
	uint realEmitCount = min(deadCount, xEmitCount);

	// Fill dispatch argument buffer for emitting (ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ):
	indirectBuffers.Store3(0, uint3(ceil((float)realEmitCount / (float)THREADCOUNT_EMIT), 1, 1));

	// Fill dispatch argument buffer for simulation (ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ):
	indirectBuffers.Store3(12, uint3(ceil((float)(aliveCount_NEW + realEmitCount) / (float)THREADCOUNT_SIMULATION), 1, 1));

	// Reset the draw argument buffer to defaults (VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation):
	indirectBuffers.Store4(24, uint4(0, 1, 0, 0));

	// copy new alivelistcount to current alivelistcount:
	counterBuffer[0].aliveCount = aliveCount_NEW;

	// and write real emit count:
	counterBuffer[0].realEmitCount = realEmitCount;
}