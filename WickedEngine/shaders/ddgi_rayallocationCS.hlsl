#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"

PUSHCONSTANT(push, DDGIPushConstants);

StructuredBuffer<DDGIVarianceDataPacked> varianceBuffer : register(t0);

RWStructuredBuffer<uint> rayallocationBuffer : register(u0);
RWBuffer<uint> raycountBuffer : register(u1);

groupshared float shared_inconsistency[DDGI_COLOR_RESOLUTION * DDGI_COLOR_RESOLUTION];
groupshared uint shared_rayCount;
groupshared uint shared_rayAllocation;

static const uint THREADCOUNT = 32;

[numthreads(THREADCOUNT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint probeIndex = Gid.x;
	const uint3 probeCoord = ddgi_probe_coord(probeIndex);
	const float3 probePos = ddgi_probe_position(probeCoord);

	float inconsistency = 0;
	for(uint i = groupIndex; i < DDGI_COLOR_RESOLUTION * DDGI_COLOR_RESOLUTION; i += THREADCOUNT)
	{
		inconsistency = max(inconsistency, varianceBuffer[probeIndex * DDGI_COLOR_RESOLUTION * DDGI_COLOR_RESOLUTION + i].load().inconsistency);
	}
	
	const uint lane_count_per_wave = WaveGetLaneCount();
	if(WaveIsFirstLane())
	{
		float wave_max_inconsistency = WaveActiveMax(inconsistency);
		shared_inconsistency[groupIndex / lane_count_per_wave] = wave_max_inconsistency;
	}

	GroupMemoryBarrierWithGroupSync();

	if(groupIndex == 0)
	{
		const uint wave_count_per_group = THREADCOUNT / lane_count_per_wave;
		float max_inconsistency = inconsistency;
		for(uint i = 0; i < wave_count_per_group; ++i)
		{
			max_inconsistency = max(max_inconsistency, shared_inconsistency[i]);
		}
		uint rayCount = saturate(max_inconsistency) * push.rayCount;
		
		ShaderSphere sphere;
		sphere.center = probePos;
		sphere.radius = max3(ddgi_cellsize()) * 2;
		if (!GetCamera().frustum.intersects(sphere))
		{
			rayCount *= 0.1;
		}

		rayCount = align(rayCount, DDGI_RAY_BUCKET_COUNT);
		rayCount = clamp(rayCount, DDGI_RAY_BUCKET_COUNT, DDGI_MAX_RAYCOUNT);

		if(push.frameIndex == 0)
			rayCount = DDGI_MAX_RAYCOUNT;
		
		raycountBuffer[probeIndex] = rayCount / DDGI_RAY_BUCKET_COUNT;
		shared_rayCount = rayCount;

		InterlockedAdd(rayallocationBuffer[0], rayCount, shared_rayAllocation);
	}

	GroupMemoryBarrierWithGroupSync();

	uint rayCount = shared_rayCount;
	uint rayAllocation = shared_rayAllocation;

	for(uint i = groupIndex; i < rayCount; i += THREADCOUNT)
	{
		uint alloc = 0;
		alloc |= probeIndex & 0xFFFFF;
		alloc |= (i & 0xFFF) << 20u;
		rayallocationBuffer[4 + rayAllocation + i] = alloc;
	}
}
