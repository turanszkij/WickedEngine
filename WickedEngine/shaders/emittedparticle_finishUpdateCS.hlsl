#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

ByteAddressBuffer counterBuffer : register(t0);

RWByteAddressBuffer indirectBuffers : register(u0);

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
#ifdef __PSSL__
		indirectBuffers.TypedStore<IndirectDispatchArgs>(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, args);
#else
		indirectBuffers.Store<IndirectDispatchArgs>(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, args);
#endif // __PSSL__
	}
	else
	{
		// Create draw argument buffer
		IndirectDrawArgsInstanced args;
		args.VertexCountPerInstance = 4;
		args.InstanceCount = particleCount;
		args.StartVertexLocation = 0;
		args.StartInstanceLocation = 0;
#ifdef __PSSL__
		indirectBuffers.TypedStore<IndirectDrawArgsInstanced>(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, args);
#else
		indirectBuffers.Store<IndirectDrawArgsInstanced>(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, args);
#endif // __PSSL__
	}
}
