#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RAWBUFFER(counterBuffer, TEXSLOT_ONDEMAND0);

RWRAWBUFFER(indirectBuffers, 0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint particleCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_CULLEDCOUNT);
	
	if (xEmitterOptions & EMITTER_OPTION_BIT_MESH_SHADER_ENABLED)
	{
		// Create DispatchMesh argument buffer:
		IndirectDispatchArgs args;
		args.ThreadGroupCountX = (particleCount + 31) / 32;
		args.ThreadGroupCountY = 1;
		args.ThreadGroupCountZ = 1;
		indirectBuffers.Store<IndirectDispatchArgs>(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, args);
	}
	else
	{
		// Create draw argument buffer
		IndirectDrawArgsInstanced args;
		args.VertexCountPerInstance = 4;
		args.InstanceCount = particleCount;
		args.StartVertexLocation = 0;
		args.StartInstanceLocation = 0;
		indirectBuffers.Store<IndirectDrawArgsInstanced>(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, args);
	}
}
