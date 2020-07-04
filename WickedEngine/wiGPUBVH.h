#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiScene_Decl.h"
#include "wiRectPacker.h"
#include "ShaderInterop_Renderer.h"
#include "wiResourceManager.h"

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
	wiGraphics::Texture globalMaterialAtlas;
	std::vector<ShaderMaterial> materialArray;
	std::unordered_map<std::shared_ptr<wiResource>, wiRectPacker::rect_xywh> storedTextures;
	std::unordered_set<std::shared_ptr<wiResource>> sceneTextures;
	void UpdateGlobalMaterialResources(const wiScene::Scene& scene, wiGraphics::CommandList cmd);

public:
	void Build(const wiScene::Scene& scene, wiGraphics::CommandList cmd);
	void Bind(wiGraphics::SHADERSTAGE stage, wiGraphics::CommandList cmd) const;

	void Clear();

	static void Initialize();

};
