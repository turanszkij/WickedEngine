#ifndef _WI_GPU_BVH_H_
#define _WI_GPU_BVH_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiSceneSystem_Decl.h"

#include <memory>

class wiGPUBVH
{
private:
	wiGraphicsTypes::GPUBuffer bvhNodeBuffer;
	wiGraphicsTypes::GPUBuffer bvhAABBBuffer;
	wiGraphicsTypes::GPUBuffer bvhFlagBuffer;
	wiGraphicsTypes::GPUBuffer triangleBuffer;
	wiGraphicsTypes::GPUBuffer clusterCounterBuffer;
	wiGraphicsTypes::GPUBuffer clusterIndexBuffer;
	wiGraphicsTypes::GPUBuffer clusterMortonBuffer;
	wiGraphicsTypes::GPUBuffer clusterSortedMortonBuffer;
	wiGraphicsTypes::GPUBuffer clusterOffsetBuffer;
	wiGraphicsTypes::GPUBuffer clusterAABBBuffer;
	wiGraphicsTypes::GPUBuffer clusterConeBuffer;
	uint32_t maxTriangleCount = 0;
	uint32_t maxClusterCount = 0;

public:
	wiGPUBVH();
	~wiGPUBVH();

	void Build(const wiSceneSystem::Scene& scene, GRAPHICSTHREAD threadID);
	void Bind(wiGraphicsTypes::SHADERSTAGE stage, GRAPHICSTHREAD threadID);

	static void LoadShaders();

};

#endif // _WI_GPU_BVH_H_
