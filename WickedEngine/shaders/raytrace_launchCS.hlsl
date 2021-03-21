#include "globals.hlsli"
#include "raytracingHF.hlsli"

RWSTRUCTUREDBUFFER(rayIndexBuffer, uint, 0);
RWSTRUCTUREDBUFFER(rayBuffer, RaytracingStoredRay, 1);

[numthreads(RAYTRACING_LAUNCH_BLOCKSIZE, RAYTRACING_LAUNCH_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (DTid.x < xTraceResolution.x && DTid.y < xTraceResolution.y)
	{
		// Compute screen coordinates:
		float2 uv = float2((DTid.xy + xTracePixelOffset) * xTraceResolution_rcp.xy * 2.0f - 1.0f) * float2(1, -1);

		// Create starting ray:
		Ray ray = CreateCameraRay(uv);
		ray.pixelID = (DTid.x & 0xFFFF) | ((DTid.y & 0xFFFF) << 16);

		// The launch writes each ray to the pixel location:
		const uint rayIndex = flatten2D(DTid.xy, xTraceResolution.xy);
		rayIndexBuffer[rayIndex] = rayIndex;
		rayBuffer[rayIndex] = CreateStoredRay(ray);
	}
}
