#include "globals.hlsli"
#include "bitonicSortHF.hlsli"

groupshared unsigned int transpose_shared_data[TRANSPOSE_BLOCK_SIZE * TRANSPOSE_BLOCK_SIZE];

[numthreads(TRANSPOSE_BLOCK_SIZE, TRANSPOSE_BLOCK_SIZE, 1)]
void main(uint3 Gid : SV_GroupID,
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID,
	uint GI : SV_GroupIndex)
{
	transpose_shared_data[GI] = Input.Load((DTid.y * g_iWidth + DTid.x) * _stride);
	GroupMemoryBarrierWithGroupSync();
	uint2 XY = DTid.yx - GTid.yx + GTid.xy;
	Data.Store((XY.y * g_iHeight + XY.x) * _stride, transpose_shared_data[GTid.x * TRANSPOSE_BLOCK_SIZE + GTid.y]);
}
