#include "globals.hlsli"
// Calculate average luminance by reduction

RWTEXTURE2D(tex, float, 0);


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
	texture_0.GetDimensions(dim.x, dim.y);
	accumulator[groupIndex] = (dispatchThreadId.x > dim.x || dispatchThreadId.y > dim.y)? 0.5: texture_0.Load(uint3(dispatchThreadId.xy,0)).r;
	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (uint iy = GROUPSIZE >> 1; iy > 0; iy = iy >> 1) {
		if (groupIndex < iy) {
			accumulator[groupIndex] += accumulator[groupIndex + iy];
		}
		GroupMemoryBarrierWithGroupSync();
	}
	if (groupIndex != 0) { return; }

	tex[groupId.xy] = max(0, lerp(tex[groupId.xy], accumulator[0] / (GROUPSIZE), g_xFrame_DeltaTime * 2));
}
