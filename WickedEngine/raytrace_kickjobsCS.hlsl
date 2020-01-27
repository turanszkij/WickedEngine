#include "globals.hlsli"
#include "ShaderInterop_Raytracing.h"

RWRAWBUFFER(counterBuffer_WRITE, 0);
RWRAWBUFFER(indirectBuffer, 1);

RAWBUFFER(counterBuffer_READ, TEXSLOT_UNIQUE0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// Load raycount from previous step:
	uint rayCount = counterBuffer_READ.Load(0);

	// write the indirect dispatch arguments:
	indirectBuffer.Store3(RAYTRACE_INDIRECT_OFFSET_TRACE, uint3((rayCount + RAYTRACING_TRACE_GROUPSIZE - 1) / RAYTRACING_TRACE_GROUPSIZE, 1, 1));
	indirectBuffer.Store3(RAYTRACE_INDIRECT_OFFSET_TILESORT, uint3((rayCount + RAYTRACING_SORT_GROUPSIZE - 1) / RAYTRACING_SORT_GROUPSIZE, 1, 1));

	// Reset counter buffer for this step:
	counterBuffer_WRITE.Store(0, 0);
}
