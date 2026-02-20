#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

StructuredBuffer<ParticleCounters> counterBuffer : register(t0);

RWStructuredBuffer<EmitterIndirectArgs> indirectBuffer : register(u0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint culled_count = counterBuffer[0].culledCount;
	uint alive_count = counterBuffer[0].aliveCount;
	
	// Create DispatchMesh argument buffer:
	IndirectDispatchArgs dispatch;
	dispatch.ThreadGroupCountX = (culled_count + THREADCOUNT_MESH_SHADER - 1) / THREADCOUNT_MESH_SHADER;
	dispatch.ThreadGroupCountY = 1;
	dispatch.ThreadGroupCountZ = 1;
	indirectBuffer[0].dispatch = dispatch;

	// Create draw argument buffer
	IndirectDrawArgsInstanced draw;
	draw.VertexCountPerInstance = 4;
	draw.InstanceCount = culled_count;
	draw.StartVertexLocation = 0;
	draw.StartInstanceLocation = 0;
	indirectBuffer[0].draw_culled = draw;

	draw.InstanceCount = alive_count;
	indirectBuffer[0].draw_all = draw;
}
