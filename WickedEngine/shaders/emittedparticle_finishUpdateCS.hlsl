#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

ByteAddressBuffer counterBuffer : register(t0);

RWStructuredBuffer<EmitterIndirectArgs> indirectBuffer : register(u0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint particleCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_CULLEDCOUNT);
	
	if (xEmitterOptions & EMITTER_OPTION_BIT_MESH_SHADER_ENABLED)
	{
		// Create DispatchMesh argument buffer:
		IndirectDispatchArgs args;
		args.ThreadGroupCountX = (particleCount + THREADCOUNT_MESH_SHADER - 1) / THREADCOUNT_MESH_SHADER;
		args.ThreadGroupCountY = 1;
		args.ThreadGroupCountZ = 1;
		indirectBuffer[0].dispatch = args;
	}
	else
	{
		// Create draw argument buffer
		IndirectDrawArgsInstanced args;
		args.VertexCountPerInstance = 4;
		args.InstanceCount = particleCount;
		args.StartVertexLocation = 0;
		args.StartInstanceLocation = 0;
		indirectBuffer[0].draw = args;
	}
}
