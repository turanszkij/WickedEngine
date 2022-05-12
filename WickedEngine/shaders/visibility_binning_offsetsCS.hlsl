#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

RWStructuredBuffer<ShaderTypeBin> output_bins : register(u0);

groupshared uint allocator;

[numthreads(SHADERTYPE_BIN_COUNT + 1, 1, 1)] // +1: sky
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x == 0)
	{
		allocator = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	InterlockedAdd(allocator, output_bins[DTid.x].count, output_bins[DTid.x].offset);
	output_bins[DTid.x].dispatchX = (output_bins[DTid.x].count + 63u) / 64u;
	output_bins[DTid.x].dispatchY = 1;
	output_bins[DTid.x].dispatchZ = 1;
	output_bins[DTid.x].count = 0;
	output_bins[DTid.x].shaderType = DTid.x;
}
