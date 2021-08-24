#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

RWSTRUCTUREDBUFFER(surfelGridBuffer, SurfelGridCell, 0);
RWSTRUCTUREDBUFFER(surfelCellBuffer, uint, 1);
RWSTRUCTUREDBUFFER(surfelCellAllocationBuffer, uint, 2);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	SurfelGridCell cell = surfelGridBuffer[DTid.x];
	InterlockedAdd(surfelCellAllocationBuffer[0], cell.count, cell.offset);
	cell.count = 0;
	surfelGridBuffer[DTid.x] = cell;
}
