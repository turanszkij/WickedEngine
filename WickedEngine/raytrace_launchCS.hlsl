#include "globals.hlsli"
#include "raytracingHF.hlsli"

RWSTRUCTUREDBUFFER(rayIndexBuffer, uint, 0);
RWSTRUCTUREDBUFFER(raySortBuffer, float, 1);
RWSTRUCTUREDBUFFER(rayBuffer, RaytracingStoredRay, 2);

[numthreads(RAYTRACING_LAUNCH_BLOCKSIZE, RAYTRACING_LAUNCH_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (DTid.x < xTraceResolution.x && DTid.y < xTraceResolution.y)
	{
		// Compute screen coordinates:
		float2 uv = float2((DTid.xy + xTracePixelOffset) * xTraceResolution_rcp.xy * 2.0f - 1.0f) * float2(1, -1);

		// Target pixel:
		uint pixelID = flatten2D(DTid.xy, xTraceResolution.xy);

		// Create starting ray:
		Ray ray = CreateCameraRay(uv);

		// The launch writes each ray to the pixel location:
		rayIndexBuffer[pixelID] = pixelID;
		raySortBuffer[pixelID] = CreateRaySortCode(ray);
		rayBuffer[pixelID] = CreateStoredRay(ray, pixelID);
	}
}
