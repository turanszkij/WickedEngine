#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

// This shader will kick off the BVH builder GPU jobs such as:
//	- BVH hierarchy, AABB propagation
//	- BVH cluster processor

RAWBUFFER(clusterCounterBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(clusterAABBBuffer, TracedRenderingAABB, TEXSLOT_ONDEMAND1);

RWRAWBUFFER(indirectDispatchBuffer, 0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint clusterCount = clusterCounterBuffer.Load(0);

	// Write hierarchy processing arguments:
	const uint threadGroupCount_Hierarchy = (clusterCount < 2) ? 0 : ceil((float)(clusterCount - 1) / (float)TRACEDRENDERING_BVH_HIERARCHY_GROUPSIZE);
	indirectDispatchBuffer.Store3(ARGUMENTBUFFER_OFFSET_HIERARCHY, uint3(threadGroupCount_Hierarchy, 1, 1));

	// Write cluster processing arguments:
	const uint threadGroupCount_ClusterProcessor = ceil((float)(clusterCount) / (float)TRACEDRENDERING_BVH_CLUSTERPROCESSOR_GROUPSIZE);
	indirectDispatchBuffer.Store3(ARGUMENTBUFFER_OFFSET_CLUSTERPROCESSOR, uint3(threadGroupCount_ClusterProcessor, 1, 1));
}
