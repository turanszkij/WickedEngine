#ifndef _WI_GPU_BVH_H_
#define _WI_GPU_BVH_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiSceneSystem_Decl.h"

#include <memory>

class wiGPUBVH
{
private:
	wiGraphics::GPUBuffer bvhNodeBuffer;
	wiGraphics::GPUBuffer bvhAABBBuffer;
	wiGraphics::GPUBuffer bvhFlagBuffer;
	wiGraphics::GPUBuffer triangleBuffer;
	wiGraphics::GPUBuffer clusterCounterBuffer;
	wiGraphics::GPUBuffer clusterIndexBuffer;
	wiGraphics::GPUBuffer clusterMortonBuffer;
	wiGraphics::GPUBuffer clusterSortedMortonBuffer;
	wiGraphics::GPUBuffer clusterOffsetBuffer;
	wiGraphics::GPUBuffer clusterAABBBuffer;
	wiGraphics::GPUBuffer clusterConeBuffer;
	uint32_t maxTriangleCount = 0;
	uint32_t maxClusterCount = 0;

public:
	void Build(const wiSceneSystem::Scene& scene, GRAPHICSTHREAD threadID);
	void Bind(wiGraphics::SHADERSTAGE stage, GRAPHICSTHREAD threadID) const;

	static void LoadShaders();

};

#endif // _WI_GPU_BVH_H_
