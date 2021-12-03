#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiScene_Decl.h"

namespace wi
{
	struct GPUBVH
	{
		// Scene BVH intersection resources:
		wi::graphics::GPUBuffer bvhNodeBuffer;
		wi::graphics::GPUBuffer bvhParentBuffer;
		wi::graphics::GPUBuffer bvhFlagBuffer;
		wi::graphics::GPUBuffer primitiveCounterBuffer;
		wi::graphics::GPUBuffer primitiveIDBuffer;
		wi::graphics::GPUBuffer primitiveBuffer;
		wi::graphics::GPUBuffer primitiveMortonBuffer;
		uint32_t primitiveCapacity = 0;
		bool IsValid() const { return primitiveCounterBuffer.IsValid(); }

		void Update(const wi::scene::Scene& scene);
		void Build(const wi::scene::Scene& scene, wi::graphics::CommandList cmd) const;

		void Clear();

		static void Initialize();
	};
}
