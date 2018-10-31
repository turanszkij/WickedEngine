#ifndef _WI_GPU_BVH_H_
#define _WI_GPU_BVH_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiSceneSystem_Decl.h"

class wiGPUBVH
{
private:
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> bvhNodeBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> bvhAABBBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> bvhFlagBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> triangleBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> clusterCounterBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> clusterIndexBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> clusterMortonBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> clusterSortedMortonBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> clusterOffsetBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> clusterAABBBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> clusterConeBuffer;

public:
	wiGPUBVH();
	~wiGPUBVH();

	void Build(const wiSceneSystem::Scene& scene, GRAPHICSTHREAD threadID);
	void Bind(GRAPHICSTHREAD threadID);

	static void LoadShaders();

};

#endif // _WI_GPU_BVH_H_
