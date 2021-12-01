#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiEnums.h"
#include "wiMath.h"
#include "wiECS.h"
#include "wiPrimitive.h"
#include "wiVector.h"
#include "wiScene_Decl.h"

#include <memory>

namespace wi
{
	class Archive;
}

namespace wi
{
	class HairParticleSystem
	{
	public:
		wi::graphics::GPUBuffer constantBuffer;
		wi::graphics::GPUBuffer simulationBuffer;
		wi::graphics::GPUBuffer vertexBuffer_POS[2];
		wi::graphics::GPUBuffer vertexBuffer_TEX;
		wi::graphics::GPUBuffer primitiveBuffer;
		wi::graphics::GPUBuffer culledIndexBuffer;
		wi::graphics::GPUBuffer indirectBuffer;
		wi::graphics::GPUBuffer subsetBuffer;

		wi::graphics::GPUBuffer indexBuffer;
		wi::graphics::GPUBuffer vertexBuffer_length;

		wi::graphics::RaytracingAccelerationStructure BLAS;

		void UpdateCPU(
			const wi::scene::TransformComponent& transform,
			const wi::scene::MeshComponent& mesh,
			float dt
		);
		void UpdateGPU(
			uint32_t instanceIndex,
			uint32_t materialIndex,
			const wi::scene::MeshComponent& mesh,
			const wi::scene::MaterialComponent& material,
			wi::graphics::CommandList cmd
		) const;
		void Draw(
			const wi::scene::MaterialComponent& material,
			wi::enums::RENDERPASS renderPass,
			wi::graphics::CommandList cmd
		) const;

		enum FLAGS
		{
			EMPTY = 0,
			_DEPRECATED_REGENERATE_FRAME = 1 << 0,
			REBUILD_BUFFERS = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		wi::ecs::Entity meshID = wi::ecs::INVALID_ENTITY;

		uint32_t strandCount = 0;
		uint32_t segmentCount = 1;
		uint32_t randomSeed = 1;
		float length = 1.0f;
		float stiffness = 10.0f;
		float randomness = 0.2f;
		float viewDistance = 200;
		wi::vector<float> vertex_lengths;

		// Sprite sheet properties:
		uint32_t framesX = 1;
		uint32_t framesY = 1;
		uint32_t frameCount = 1;
		uint32_t frameStart = 0;

		// Non-serialized attributes:
		XMFLOAT4X4 world;
		XMFLOAT4X4 worldPrev;
		wi::primitive::AABB aabb;
		wi::vector<uint32_t> indices; // it is dependent on vertex_lengths and contains triangles with non-zero lengths
		uint32_t layerMask = ~0u;
		mutable bool regenerate_frame = true;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

		static void Initialize();
	};
}
