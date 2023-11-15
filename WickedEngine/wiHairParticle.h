#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiEnums.h"
#include "wiMath.h"
#include "wiECS.h"
#include "wiPrimitive.h"
#include "wiVector.h"
#include "wiScene_Decl.h"
#include "wiScene_Components.h"

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
		wi::graphics::GPUBuffer generalBuffer;
		wi::scene::MeshComponent::BufferView simulation_view;
		wi::scene::MeshComponent::BufferView vb_pos[2];
		wi::scene::MeshComponent::BufferView vb_nor;
		wi::scene::MeshComponent::BufferView vb_pos_raytracing;
		wi::scene::MeshComponent::BufferView vb_uvs;
		wi::scene::MeshComponent::BufferView ib_culled;
		wi::scene::MeshComponent::BufferView indirect_view;
		wi::graphics::GPUBuffer primitiveBuffer;

		wi::graphics::GPUBuffer indexBuffer;
		wi::graphics::GPUBuffer vertexBuffer_length;

		wi::graphics::RaytracingAccelerationStructure BLAS;

		void CreateFromMesh(const wi::scene::MeshComponent& mesh);
		void CreateRenderData();
		void CreateRaytracingRenderData();

		void UpdateCPU(
			const wi::scene::TransformComponent& transform,
			const wi::scene::MeshComponent& mesh,
			float dt
		);

		struct UpdateGPUItem
		{
			const HairParticleSystem* hair = nullptr;
			uint32_t instanceIndex = 0;
			const wi::scene::MeshComponent* mesh = nullptr;
			const wi::scene::MaterialComponent* material = nullptr;
		};
		// Update a batch of hair particles by GPU
		static void UpdateGPU(
			const UpdateGPUItem* items,
			uint32_t itemCount,
			wi::graphics::CommandList cmd
		);

		mutable bool gpu_initialized = false;
		void InitializeGPUDataIfNeeded(wi::graphics::CommandList cmd);

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
		wi::primitive::AABB aabb;
		wi::vector<uint32_t> indices; // it is dependent on vertex_lengths and contains triangles with non-zero lengths
		uint32_t layerMask = ~0u;
		mutable bool regenerate_frame = true;
		wi::graphics::Format position_format = wi::graphics::Format::R16G16B16A16_UNORM;

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);

		static void Initialize();

		constexpr uint32_t GetParticleCount() const { return strandCount * segmentCount; }
		uint64_t GetMemorySizeInBytes() const;
	};
}
