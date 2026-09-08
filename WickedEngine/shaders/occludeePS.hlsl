#include "globals.hlsli"

RWStructuredBuffer<uint2> occlusion_results : register(u0);

[earlydepthstencil] // only UAV write for depth pass!
void main(float4 pos : SV_Position, uint occlusion_slot : OCCLUSION_SLOT)
{
	InterlockedOr(occlusion_results[occlusion_slot].x, 1u);
}
