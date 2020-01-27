#include "globals.hlsli"
#include "raytracingHF.hlsli"

RAWBUFFER(counterBuffer_READ, TEXSLOT_ONDEMAND7);
STRUCTUREDBUFFER(rayBuffer_READ, RaytracingStoredRay, TEXSLOT_ONDEMAND8);

RWSTRUCTUREDBUFFER(rayIndexBuffer_WRITE, uint, 0);

static const uint numArray = RAYTRACING_SORT_GROUPSIZE;
static const uint numArrayPowerOfTwo = 2 << firstbithigh(numArray - 1);
groupshared uint2 Array[numArray];

void BitonicSort(in uint localIdxFlattened)
{
    for (uint nMergeSize = 2; nMergeSize <= numArrayPowerOfTwo; nMergeSize = nMergeSize * 2)
    {
        for (uint nMergeSubSize = nMergeSize >> 1; nMergeSubSize > 0; nMergeSubSize = nMergeSubSize >> 1)
        {
            uint tmp_index = localIdxFlattened;
            uint index_low = tmp_index & (nMergeSubSize - 1);
            uint index_high = 2 * (tmp_index - index_low);
            uint index = index_high + index_low;

            uint nSwapElem = nMergeSubSize == nMergeSize >> 1 ? index_high + (2 * nMergeSubSize - 1) - index_low : index_high + nMergeSubSize + index_low;

            if (nSwapElem < numArray && index < numArray)
            {
                if (Array[index].x > Array[nSwapElem].x)
                {
                    uint2 uTemp = Array[index];
                    Array[index] = Array[nSwapElem];
                    Array[nSwapElem] = uTemp;
                }
            }
            GroupMemoryBarrierWithGroupSync();
        }
    }
}

[numthreads(RAYTRACING_SORT_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    uint sortcode = ~0;

    [branch]
    if (DTid.x < counterBuffer_READ.Load(0))
    {
        sortcode = CreateRaySortCode(LoadRay(rayBuffer_READ[DTid.x]));
    }

    Array[groupIndex] = uint2(sortcode, DTid.x);
    GroupMemoryBarrierWithGroupSync();

    BitonicSort(groupIndex);
    GroupMemoryBarrierWithGroupSync();

    rayIndexBuffer_WRITE[DTid.x] = Array[groupIndex].y;
}
