#include "globals.hlsli"
#include "cullingShaderHF.hlsli"

// View space frustums for the grid cells.
RWSTRUCTUREDBUFFER(out_Frustums, Frustum, SBSLOT_TILEFRUSTUMS);

#define BLOCK_SIZE 16
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(
	uint3 groupId : SV_GroupID,
	uint3 groupThreadId : SV_GroupThreadID,
	uint3 dispatchThreadId : SV_DispatchThreadID, // 
	uint groupIndex : SV_GroupIndex)
{
	// View space eye position is always at the origin.
	const float3 eyePos = float3(0, 0, 0);

	// Compute the 4 corner points on the far clipping plane to use as the 
	// frustum vertices.
	float4 screenSpace[4];
	// Top left point
	screenSpace[0] = float4(dispatchThreadId.xy * BLOCK_SIZE, 1.0f, 1.0f);
	// Top right point
	screenSpace[1] = float4(float2(dispatchThreadId.x + 1, dispatchThreadId.y) * BLOCK_SIZE, 1.0f, 1.0f);
	// Bottom left point
	screenSpace[2] = float4(float2(dispatchThreadId.x, dispatchThreadId.y + 1) * BLOCK_SIZE, 1.0f, 1.0f);
	// Bottom right point
	screenSpace[3] = float4(float2(dispatchThreadId.x + 1, dispatchThreadId.y + 1) * BLOCK_SIZE, 1.0f, 1.0f);

	float3 viewSpace[4];
	// Now convert the screen space points to view space
	for (int i = 0; i < 4; i++)
	{
		viewSpace[i] = ScreenToView(screenSpace[i]).xyz;
	}

	// Now build the frustum planes from the view space points
	Frustum frustum;

	// Left plane
	frustum.planes[0] = ComputePlane(eyePos, viewSpace[2], viewSpace[0]);
	// Right plane
	frustum.planes[1] = ComputePlane(eyePos, viewSpace[1], viewSpace[3]);
	// Top plane
	frustum.planes[2] = ComputePlane(eyePos, viewSpace[0], viewSpace[1]);
	// Bottom plane
	frustum.planes[3] = ComputePlane(eyePos, viewSpace[3], viewSpace[2]);

	// Store the computed frustum in global memory (if our thread ID is in bounds of the grid).
	if (dispatchThreadId.x < numThreads.x && dispatchThreadId.y < numThreads.y)
	{
		uint index = dispatchThreadId.x + (dispatchThreadId.y * numThreads.x);
		out_Frustums[index] = frustum;
	}
}