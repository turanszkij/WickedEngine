#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

RAWBUFFER(clusterCounterBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(clusterIndexBuffer, uint, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(clusterMortonBuffer, uint, TEXSLOT_ONDEMAND2);

RWSTRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, 0);

#define __clz firstbithigh

inline int2 determineRange(uint count, uint idx)
{
	//todo
	return int2(count, idx);
}

inline int findSplit(int first, int last)
{
	// Identical Morton codes => split the range in the middle.

	uint firstCode = clusterMortonBuffer[first];
	uint lastCode = clusterMortonBuffer[last];

	if (firstCode == lastCode)
		return (first + last) >> 1;

	// Calculate the number of highest bits that are the same
	// for all objects, using the count-leading-zeros intrinsic.

	int commonPrefix = __clz(firstCode ^ lastCode);

	// Use binary search to find where the next bit differs.
	// Specifically, we are looking for the highest object that
	// shares more than commonPrefix bits with the first one.

	int split = first; // initial guess
	int step = last - first;

	do
	{
		step = (step + 1) >> 1; // exponential decrease
		int newSplit = split + step; // proposed new position

		if (newSplit < last)
		{
			uint splitCode = clusterMortonBuffer[newSplit];
			int splitPrefix = __clz(firstCode ^ splitCode);
			if (splitPrefix > commonPrefix)
				split = newSplit; // accept proposal
		}
	} while (step > 1);

	return split;
}



[numthreads(TRACEDRENDERING_BVH_HIERARCHY_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint idx = DTid.x;
	const uint clusterCount = clusterCounterBuffer.Load(0);

	if (idx < clusterCount - 1)
	{
		// Find out which range of objects the node corresponds to.
		// (This is where the magic happens!)

		int2 range = determineRange(clusterCount, idx);
		int first = range.x;
		int last = range.y;

		// Determine where to split the range.

		int split = findSplit(first, last);

		// Select childA.

		uint childA = split;
		if (split == first)
		{
			//childA = &leafNodes[split];
			childA = BVH_MakeLeafNode(childA);
		}
		else
		{
			//childA = &internalNodes[split];
		}

		// Select childB.

		uint childB = split + 1;
		if (split + 1 == last)
		{
			//childB = &leafNodes[split + 1];
			childB = BVH_MakeLeafNode(childB);
		}
		else
		{
			//childB = &internalNodes[split + 1];
		}

		// Record parent-child relationships.

		bvhNodeBuffer[idx].childA = childA;
		bvhNodeBuffer[idx].childB = childB;
		//childA->parent = &internalNodes[idx];
		//childB->parent = &internalNodes[idx];
		if (!BVH_IsLeafNode(childA))
		{
			bvhNodeBuffer[childA].parent = idx;
		}
		if (!BVH_IsLeafNode(childB))
		{
			bvhNodeBuffer[childB].parent = idx;
		}
	}
}
