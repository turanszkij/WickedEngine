#include "globals.hlsli"

TEXTURE3D(input, float4, 0);
RWTEXTURE3D(output, float4, 0);

groupshared float4 accumulation[4 * 4 * 4];

[numthreads(4, 4, 4)]
void main( uint3 DTid : SV_DispatchThreadID, uint GroupIndex : SV_GroupIndex )
{
	// TODO!

	float4 current = input[DTid];

	//output[DTid] = 1 - current.aaaa;

	accumulation[GroupIndex] = current;

	GroupMemoryBarrierWithGroupSync();

	float4 avg = 0;
	for (uint i = 0; i < 4 * 4 * 4; ++i)
	{
		avg += accumulation[i];
	}
	avg /= 4 * 4 * 4;

	output[DTid] = 1 - avg.aaaa;
}