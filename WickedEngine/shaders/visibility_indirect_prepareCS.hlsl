#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

RWStructuredBuffer<ShaderTypeBin> output_bins : register(u0);

[numthreads(SHADERTYPE_BIN_COUNT + 1, 1, 1)] // +1: sky
void main(uint3 DTid : SV_DispatchThreadID)
{
	output_bins[DTid.x].shaderType = DTid.x;
	output_bins[DTid.x].offset = DTid.x * GetCamera().visibility_tilecount_flat;
	output_bins[DTid.x].dispatchX = output_bins[DTid.x].count;
	output_bins[DTid.x].dispatchY = 1;
	output_bins[DTid.x].dispatchZ = 1;
}
