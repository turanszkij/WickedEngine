#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RAWBUFFER(counterBuffer, TEXSLOT_ONDEMAND0);

RWRAWBUFFER(indirectBuffers, 0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint particleCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION);
	
	if (xEmitterOptions & EMITTER_OPTION_BIT_MESH_SHADER_ENABLED)
	{
		// Create DispatchMesh argument buffer (X, Y, Z group counts):
		indirectBuffers.Store4(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, uint4((particleCount + 31) / 32, 1, 1, 0));
	}
	else
	{
		// Create draw argument buffer (VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation):
		indirectBuffers.Store4(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, uint4(4, particleCount, 0, 0));
	}
}
