#include "globals.hlsli"
#include "ddgiHF.hlsli"

PUSHCONSTANT(push, DDGIPushConstants);

StructuredBuffer<DDGIVarianceDataPacked> varianceBuffer : register(t0);

RWStructuredBuffer<uint> rayallocationBuffer : register(u0);
RWBuffer<uint> raycountBuffer : register(u1);
RWBuffer<uint> rayBaseBuffer : register(u2);

groupshared float shared_inconsistency[DDGI_COLOR_RESOLUTION * DDGI_COLOR_RESOLUTION];
groupshared uint shared_rayCount;
groupshared uint shared_rayAllocation;

static const uint THREADCOUNT = 32;

[numthreads(THREADCOUNT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint probeIndex = Gid.x;
	uint cascade;
	uint3 probeCoord;
	ddgi_decode_probe(probeIndex, cascade, probeCoord);

	// Staggered updates: a cascade that is not refreshed this frame allocates
	// no rays (so nothing is traced and its probes keep their last result). The
	// active flag is uniform across the group (one probe per group), so this
	// early-out is uniform control flow. Zero the ray count defensively.
	if (GetScene().ddgi.cascades[cascade].active == 0)
	{
		if (groupIndex == 0)
		{
			raycountBuffer[probeIndex] = 0;
			rayBaseBuffer[probeIndex] = 0;
		}
		return;
	}

	const float3 probePos = ddgi_probe_position(cascade, probeCoord);

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
		sphere.radius = max3(ddgi_cellsize(cascade)) * 2;
		if (!GetCamera().frustum.intersects(sphere))
		{
			rayCount *= 0.1;
		}

		rayCount = align(rayCount, DDGI_RAY_BUCKET_COUNT);
		rayCount = clamp(rayCount, DDGI_RAY_BUCKET_COUNT, DDGI_MAX_RAYCOUNT);

		StructuredBuffer<DDGIProbe> probe_buffer = bindless_structured_ddgi_probes[descriptor_index(GetScene().ddgi.probe_buffer)];
		const uint probe_flags = probe_buffer[probeIndex].flags;
		// Fresh = needs a full reset-and-converge: global frame 0, just
		// (re)placed by a grid scroll (FRESH), or never refreshed at all
		// (INITIALIZED not yet set - the zeroed creation state; coarse cascades
		// skip frame 0 because the transient ray buffers only fit two cascades,
		// so their first round-robin activation converges through this path).
		// COLOR_RESET (a probe just ejected from solid geometry) also needs the
		// full budget so it converges from its new open position in one step
		// rather than trickling up from the black it had while buried.
		const bool probe_fresh = (push.frameIndex == 0)
			|| (probe_flags & DDGIPROBE_FLAG_FRESH) != 0
			|| (probe_flags & DDGIPROBE_FLAG_COLOR_RESET) != 0
			|| (probe_flags & DDGIPROBE_FLAG_INITIALIZED) == 0;
		const bool probe_valid = (probe_flags & DDGIPROBE_FLAG_VALID) != 0;

		if(probe_fresh)
		{
			// Full ray budget on frame 0 and for probes just (re)placed by a
			// grid scroll, so their reset state converges in one frame instead
			// of trickling in.
			rayCount = DDGI_MAX_RAYCOUNT;
		}
		else if(!probe_valid)
		{
			// Buried probes are down-weighted to almost nothing when sampled,
			// so spending the variance-driven budget on them (their backface
			// shading is noisy, which otherwise inflates that budget) is wasted
			// work. Cap them at a small fixed count - just enough for the depth
			// pass to keep re-measuring the backface ratio and relocating, so a
			// probe that becomes exposed recovers and is handed the full budget
			// again. Too few rays here (e.g. a single 4-ray bucket) would make
			// the 25% test flicker frame to frame since the ray directions are
			// re-randomized each frame.
			rayCount = DDGI_RAY_BUCKET_COUNT * 8; // 32 rays (of up to 512)
		}

		// Reserve this probe's ray range in the compacted ray buffer, then
		// clamp to its capacity. The transient ray buffers are sized for the
		// probes refreshed in one frame (see ShaderDDGI::ray_buffer_capacity),
		// so the clamp guarantees no out-of-bounds writes even in a worst-case
		// frame. Everything stays DDGI_RAY_BUCKET_COUNT-aligned: rayCount is
		// aligned, so every base (a sum of aligned counts) and the capacity (a
		// multiple of DDGI_MAX_RAYCOUNT) are too.
		InterlockedAdd(rayallocationBuffer[0], rayCount, shared_rayAllocation);
		const uint capacity = GetScene().ddgi.ray_buffer_capacity;
		const uint base = shared_rayAllocation;
		rayCount = base >= capacity ? 0u : min(rayCount, capacity - base);

		raycountBuffer[probeIndex] = rayCount / DDGI_RAY_BUCKET_COUNT;
		rayBaseBuffer[probeIndex] = base;
		shared_rayCount = rayCount;
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
