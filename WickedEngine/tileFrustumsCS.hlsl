#include "globals.hlsli"
#include "cullingShaderHF.hlsli"

// View space frustums for the grid cells.
RWSTRUCTUREDBUFFER(out_Frustums, Frustum, 0);

[numthreads(TILED_CULLING_BLOCKSIZE, TILED_CULLING_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// View space eye position is always at the origin.
	const float3 eyePos = float3(0, 0, 0);

	// Compute 4 points on the far clipping plane to use as the 
	// frustum vertices.
	float4 screenSpace[4];
	// Top left point
	screenSpace[0] = float4(DTid.xy * TILED_CULLING_BLOCKSIZE, 1.0f, 1.0f);
	// Top right point
	screenSpace[1] = float4(float2(DTid.x + 1, DTid.y) * TILED_CULLING_BLOCKSIZE, 1.0f, 1.0f);
	// Bottom left point
	screenSpace[2] = float4(float2(DTid.x, DTid.y + 1) * TILED_CULLING_BLOCKSIZE, 1.0f, 1.0f);
	// Bottom right point
	screenSpace[3] = float4(float2(DTid.x + 1, DTid.y + 1) * TILED_CULLING_BLOCKSIZE, 1.0f, 1.0f);

	float3 viewSpace[4];
	// Now convert the screen space points to view space
	for (int i = 0; i < 4; i++)
	{
		viewSpace[i] = ScreenToView(screenSpace[i]).xyz;
	}

	// Now build the frustum planes from the view space points
	Frustum frustum;

	// Left plane
	frustum.planes[0] = ComputePlane(viewSpace[2], eyePos, viewSpace[0]);
	// Right plane
	frustum.planes[1] = ComputePlane(viewSpace[1], eyePos, viewSpace[3]);
	// Top plane
	frustum.planes[2] = ComputePlane(viewSpace[0], eyePos, viewSpace[1]);
	// Bottom plane
	frustum.planes[3] = ComputePlane(viewSpace[3], eyePos, viewSpace[2]);

	// Store the computed frustum in global memory (if our thread ID is in bounds of the grid).
	if (DTid.x < xDispatchParams_numThreads.x && DTid.y < xDispatchParams_numThreads.y)
	{
		out_Frustums[flatten2D(DTid.xy, xDispatchParams_numThreads.xy)] = frustum;
	}
}