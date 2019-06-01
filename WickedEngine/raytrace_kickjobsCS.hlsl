#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

RWRAWBUFFER(counterBuffer_WRITE, 0);
RWRAWBUFFER(indirectBuffer, 1);

RAWBUFFER(counterBuffer_READ, TEXSLOT_UNIQUE0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// Load raycount from previous step:
	uint rayCount = counterBuffer_READ.Load(0);

	// Convert raycount to dispatch thread group count:
	uint groupCount = (rayCount + TRACEDRENDERING_TRACE_GROUPSIZE - 1) / TRACEDRENDERING_TRACE_GROUPSIZE;

	// write the indirect dispatch arguments:
	indirectBuffer.Store3(0, uint3(groupCount, 1, 1));

	// Reset counter buffer for this step:
	counterBuffer_WRITE.Store(0, 0);
}
