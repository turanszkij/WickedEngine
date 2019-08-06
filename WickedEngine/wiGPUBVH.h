#ifndef _WI_GPU_BVH_H_
#define _WI_GPU_BVH_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiSceneSystem_Decl.h"
#include "wiRectPacker.h"
#include "ShaderInterop_TracedRendering.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>

class wiGPUBVH
{
private:
	// Scene BVH intersection resources:
	wiGraphics::GPUBuffer bvhNodeBuffer;
	wiGraphics::GPUBuffer bvhParentBuffer;
	wiGraphics::GPUBuffer bvhFlagBuffer;
	wiGraphics::GPUBuffer primitiveCounterBuffer;
	wiGraphics::GPUBuffer primitiveIDBuffer;
	wiGraphics::GPUBuffer primitiveBuffer;
	wiGraphics::GPUBuffer primitiveDataBuffer;
	wiGraphics::GPUBuffer primitiveMortonBuffer;
	uint32_t primitiveCapacity = 0;
	uint32_t primitiveCount = 0;

	// Scene material resources:
	wiGraphics::GPUBuffer globalMaterialBuffer;
	wiGraphics::Texture2D globalMaterialAtlas;
	std::vector<TracedRenderingMaterial> materialArray;
	std::unordered_map<const wiGraphics::Texture2D*, wiRectPacker::rect_xywh> storedTextures;
	std::unordered_set<const wiGraphics::Texture2D*> sceneTextures;
	void UpdateGlobalMaterialResources(const wiSceneSystem::Scene& scene, wiGraphics::CommandList cmd);

public:
	// Update hierarchy + bounding boxes
	void Build(const wiSceneSystem::Scene& scene, wiGraphics::CommandList cmd);
	// Only update bounding boxes
	void Refit(wiGraphics::CommandList cmd);
	// Bind shader resources
	void Bind(wiGraphics::SHADERSTAGE stage, wiGraphics::CommandList cmd) const;

	static void LoadShaders();

};

#endif // _WI_GPU_BVH_H_
