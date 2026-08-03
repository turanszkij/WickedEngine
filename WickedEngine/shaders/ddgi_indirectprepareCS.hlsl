#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"

RWStructuredBuffer<uint> rayallocationBuffer : register(u0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// The allocation counter is the raw sum of requested rays; the allocation
	// pass clamps what it actually writes to ray_buffer_capacity (the transient
	// ray buffers are sized for the probes refreshed per frame, not for every
	// probe). Clamp the dispatch the same way so no thread reads a record slot
	// beyond capacity.
	uint rayCount = min(rayallocationBuffer[0], GetScene().ddgi.ray_buffer_capacity);
	rayallocationBuffer[0] = (rayCount + 31) / 32;
	rayallocationBuffer[1] = 1;
	rayallocationBuffer[2] = 1;
	rayallocationBuffer[3] = rayCount;
}
