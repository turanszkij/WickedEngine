#include "globals.hlsli"

RWTexture2D<float> accumulator : register(u0);

[numthreads(1, 1, 1)]
void main(
	uint3 groupId : SV_GroupID,
	uint3 groupThreadId : SV_GroupThreadID,
	uint3 dispatchThreadId : SV_DispatchThreadID, // 
	uint groupIndex : SV_GroupIndex)
{
	accumulator[uint2(0,0)] = lerp(accumulator.Load(0), texture_0.Load(0).r, 0.08f);
}