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
		ray.pixelID = flatten2D(DTid.xy, xTraceResolution.xy);

		// The launch writes each ray to the pixel location:
		rayIndexBuffer[ray.pixelID] = ray.pixelID;
		rayBuffer[ray.pixelID] = CreateStoredRay(ray);
	}
}
