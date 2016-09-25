#include "globals.hlsli"
#include "cullingShaderHF.hlsli"

groupshared uint minDepthInt;
groupshared uint maxDepthInt;

RWTexture2D<float4> tex : register(u0);

STRUCTUREDBUFFER(in_Frustums, Frustum, SBSLOT_TILEFRUSTUMS);
groupshared Frustum frustum;

struct LightArrayType
{
	float3 pos;
	float radius;
	float4 col;
};
RWSTRUCTUREDBUFFER(lightArray, LightArrayType, SBSLOT_LIGHTARRAY);
#define lightCount ((int)g_xColor.x)

bool intersects(LightArrayType light, Frustum frustum, float minZ, float maxZ)
{
	Sphere sphere;
	sphere.c = light.pos;
	sphere.r = light.radius;
	return SphereInsideFrustum(sphere, frustum, minZ, maxZ);
}

groupshared uint visibleLightCount = 0;
groupshared uint visibleLightIndices[1024];

#define BLOCK_SIZE 16
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(
	uint3 groupId : SV_GroupID,
	uint3 groupThreadId : SV_GroupThreadID,
	uint3 dispatchThreadId : SV_DispatchThreadID, // 
	uint groupIndex : SV_GroupIndex)
{
	float depth = texture_lineardepth.Load(uint3(dispatchThreadId.xy, 0)).r / g_xCamera_ZFarP;
	uint depthInt = asuint(depth);

	if (groupIndex == 0) // Avoid contention by other threads in the group.
	{
		minDepthInt = 0xffffffff;
		maxDepthInt = 0;
		//o_LightCount = 0;
		//t_LightCount = 0;
		frustum = in_Frustums[groupId.x + (groupId.y * numThreadGroups.x)];
	}

	//minDepthInt = 0xFFFFFFFF;
	//maxDepthInt = 0;

	GroupMemoryBarrierWithGroupSync();

	InterlockedMin(minDepthInt, depthInt);
	InterlockedMax(maxDepthInt, depthInt);

	GroupMemoryBarrierWithGroupSync();

	float minGroupDepth = asfloat(minDepthInt);
	float maxGroupDepth = asfloat(maxDepthInt);

	//tex[dispatchThreadId.xy] = minGroupDepth;

	uint threadCount = BLOCK_SIZE*BLOCK_SIZE;
	uint passCount = (lightCount + threadCount - 1) / threadCount;

	for (uint passIt = 0; passIt < passCount; ++passIt)
	{
		uint lightIndex = passIt*threadCount + groupIndex;

		// prevent overrun by clamping to last "null" light
		lightIndex = min(lightIndex, lightCount);

		if (intersects(lightArray[lightIndex], frustum, minGroupDepth, maxGroupDepth))
		{
			uint offset;
			InterlockedAdd(visibleLightCount, 1, offset);
			visibleLightIndices[offset] = lightIndex;
		}
	}

	GroupMemoryBarrierWithGroupSync();

	float4 color = 0;
	for (uint lightIt = 0; lightIt < visibleLightCount; ++lightIt)
	{
		uint lightIndex = visibleLightIndices[lightIt];
		LightArrayType light = lightArray[lightIndex];

		color += light.col;
	}
	color.a = 1;

	color.rgb = frustum.planes[0].N;

	tex[dispatchThreadId.xy] = color;
}