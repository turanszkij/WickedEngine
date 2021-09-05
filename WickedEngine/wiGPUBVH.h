#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiScene_Decl.h"
#include "wiRectPacker.h"
#include "shaders/ShaderInterop_Renderer.h"
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
	wiGraphics::GPUBuffer primitiveMortonBuffer;
	uint32_t primitiveCapacity = 0;

public:
	void Update(const wiScene::Scene& scene);
	void Build(const wiScene::Scene& scene, wiGraphics::CommandList cmd) const;
	void Bind(wiGraphics::CommandList cmd) const;

	void Clear();

	static void Initialize();

};
