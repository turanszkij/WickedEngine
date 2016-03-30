#include "globals.hlsli"

RWTexture2D<float> tex : register(u0);


#define SIZEX 16
#define SIZEY 16
#define GROUPSIZE SIZEX*SIZEY
groupshared float accumulator[SIZEX* SIZEY];


//groupshared uint minDepthInt;
//groupshared uint maxDepthInt;

[numthreads(SIZEX, SIZEY, 1)]
void main(
	uint3 groupId : SV_GroupID,
	uint3 groupThreadId : SV_GroupThreadID,
	uint3 dispatchThreadId : SV_DispatchThreadID, // 
	uint groupIndex : SV_GroupIndex)
{
	// minmax depth in tile
	//uint2 pixel = dispatchThreadId.xy;

	//float depth = texture_lineardepth.Load(uint3(pixel,0)).r;
	//uint depthInt = asuint(depth);

	//minDepthInt = 0xFFFFFFFF;
	//maxDepthInt = 0;

	//GroupMemoryBarrierWithGroupSync();

	//InterlockedMin(minDepthInt, depthInt);
	//InterlockedMax(maxDepthInt, depthInt);

	//GroupMemoryBarrierWithGroupSync();

	//float minGroupDepth = asfloat(minDepthInt);
	//float maxGroupDepth = asfloat(maxDepthInt);

	//tex[pixel] = float4(maxGroupDepth.xxx / g_xCamera_ZFarP, 1);


	// reduction

	//Load pixel into local memory buffer.
	float3 color = texture_0.Load(uint3(dispatchThreadId.xy, 0)).rgb;
	accumulator[groupIndex] = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));

	//Wait for all
	GroupMemoryBarrierWithGroupSync(); 
	
	
	[unroll]
	for (uint ix = GROUPSIZE >> 1; ix > 0; ix = ix >> 1) {
		if (groupIndex < ix) {
			accumulator[groupIndex] = (accumulator[groupIndex] + accumulator[groupIndex + ix])*.5;
		}
		GroupMemoryBarrierWithGroupSync();
	}
	if (groupIndex != 0) { return; }

	tex[groupId.xy] = accumulator[0];
}
