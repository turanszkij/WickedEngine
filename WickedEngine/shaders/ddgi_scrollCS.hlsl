#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"

// Runs once per frame that the DDGI grid scrolls (camera crossed a cell
// boundary), before ray allocation. One thread per probe.
//
// A scroll shifts which world region each buffer slot represents. Buffer slots
// whose world coord falls in the freshly entered slab now hold data from the
// region that just left the grid on the opposite side, so that data is stale
// and must be discarded.
//
// Rather than clearing the probe/variance/depth buffers here, we just flag
// those probes FRESH and zero their relocation offset. The ray-allocation,
// colour-update and depth-update passes already have a "frame 0" full-reset
// path; they treat a FRESH probe exactly like frame 0 (max ray budget,
// variance/mean reset, depth taken directly), and the depth pass clears the
// FRESH flag once consumed. This keeps the reset logic in one place and avoids
// binding/clearing the other buffers from here.

RWStructuredBuffer<DDGIProbe> ddgiProbeBuffer : register(u0);

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	const uint probeIndex = DTid;
	if (probeIndex >= GetScene().ddgi.probe_count)
		return;

	const uint3 dims = GetScene().ddgi.grid_dimensions;
	const int3 scroll_delta = GetScene().ddgi.scroll_delta;

	// This probe's buffer coord, and the world coord it now represents (post-scroll).
	const uint3 buffer_coord = ddgi_probe_coord(probeIndex);
	const uint3 world_coord = ddgi_buffer_to_world_coord(buffer_coord);

	// A positive scroll on an axis brings in new world cells at the high edge;
	// a negative scroll brings them in at the low edge. |scroll_delta| < dims
	// is guaranteed by the CPU (a larger jump is handled as a full reset via
	// frameIndex == 0 instead).
	bool needs_reset = false;
	[unroll]
	for (uint axis = 0; axis < 3; ++axis)
	{
		int delta = scroll_delta[axis];
		uint w = world_coord[axis];
		if (delta > 0 && w >= dims[axis] - (uint)delta)
			needs_reset = true;
		if (delta < 0 && w < (uint)(-delta))
			needs_reset = true;
	}

	if (needs_reset)
	{
		ddgiProbeBuffer[probeIndex].offset = 0;
		ddgiProbeBuffer[probeIndex].flags = DDGIPROBE_FLAG_VALID | DDGIPROBE_FLAG_FRESH;
	}
}
