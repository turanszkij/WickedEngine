#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

RWStructuredBuffer<ShaderTypeBin> output_binning : register(u14);

groupshared uint allocator;

[numthreads(SHADERTYPE_BIN_COUNT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x == 0)
	{
		allocator = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	InterlockedAdd(allocator, output_binning[DTid.x].count, output_binning[DTid.x].offset);
	output_binning[DTid.x].dispatchX = (output_binning[DTid.x].count + 63u) / 64u;
	output_binning[DTid.x].dispatchY = 1;
	output_binning[DTid.x].dispatchZ = 1;
	output_binning[DTid.x].count = 0;
}
