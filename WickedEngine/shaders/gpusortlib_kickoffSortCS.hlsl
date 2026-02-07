#include "globals.hlsli"
#include "ShaderInterop_GPUSortLib.h"

RWStructuredBuffer<IndirectDispatchArgs> indirectBuffers : register(u0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// retrieve GPU itemcount:
	uint itemCount = counterBuffer.count;

	IndirectDispatchArgs args;

	if (itemCount > 0)
	{
		args.ThreadGroupCountX = ((itemCount - 1) >> 9) + 1;
		args.ThreadGroupCountY = 1;
		args.ThreadGroupCountZ = 1;
	}
	else
	{
		args.ThreadGroupCountX = 0;
		args.ThreadGroupCountY = 0;
		args.ThreadGroupCountZ = 0;
	}

	indirectBuffers[0] = args;
}
