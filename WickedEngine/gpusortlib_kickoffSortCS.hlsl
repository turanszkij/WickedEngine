#include "globals.hlsli"
#include "ShaderInterop_GPUSortLib.h"

RAWBUFFER(counterBuffer, 0);

RWRAWBUFFER(indirectBuffers, 0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// retrieve GPU itemcount:
	uint itemCount = counterBuffer.Load(counterReadOffset);

	if (itemCount > 0)
	{
		// calculate threadcount:
		uint threadCount = ((itemCount - 1) >> 9) + 1;

		// and prepare to dispatch the sort for the alive simulated particles:
		indirectBuffers.Store3(0, uint3(threadCount, 1, 1));
	}
	else
	{
		// dispatch nothing:
		indirectBuffers.Store3(0, uint3(0, 0, 0));
	}
}