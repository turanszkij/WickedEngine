#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

RWSTRUCTUREDBUFFER(rayBuffer, StoredRay, 0);

[numthreads(TRACEDRENDERING_LAUNCH_BLOCKSIZE, TRACEDRENDERING_LAUNCH_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// Compute screen coordinates:
	float2 uv = float2((DTid.xy + xTracePixelOffset) * g_xWorld_InternalResolution_Inverse * 2.0f - 1.0f) * float2(1, -1);

	// Target pixel:
	uint pixelID = flatten2D(DTid.xy, GetInternalResolution());

	// Create starting ray:
	Ray ray = CreateCameraRay(uv);

	// The launch writes each ray to the pixel location:
	rayBuffer[pixelID] = CreateStoredRay(ray, pixelID);
}