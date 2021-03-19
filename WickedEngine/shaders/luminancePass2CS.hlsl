#include "globals.hlsli"
// Calculate average luminance by reduction

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float, 0);


#define SIZEX 16
#define SIZEY 16
#define GROUPSIZE SIZEX*SIZEY
groupshared float accumulator[GROUPSIZE];

[numthreads(SIZEX, SIZEY, 1)]
void main(
	uint3 groupId : SV_GroupID,
	uint3 groupThreadId : SV_GroupThreadID,
	uint3 dispatchThreadId : SV_DispatchThreadID, // 
	uint groupIndex : SV_GroupIndex)
{
	uint2 dim;
	input.GetDimensions(dim.x, dim.y);
	accumulator[groupIndex] = (dispatchThreadId.x > dim.x || dispatchThreadId.y > dim.y)? 0.5 : input[dispatchThreadId.xy].r;
	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (uint iy = GROUPSIZE >> 1; iy > 0; iy = iy >> 1) {
		if (groupIndex < iy) {
			accumulator[groupIndex] += accumulator[groupIndex + iy];
		}
		GroupMemoryBarrierWithGroupSync();
	}
	if (groupIndex != 0) { return; }

	output[groupId.xy] = max(0, lerp(output[groupId.xy], accumulator[0] / (GROUPSIZE), g_xFrame_DeltaTime * 2));
}
