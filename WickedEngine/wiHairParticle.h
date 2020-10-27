#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiEnums.h"
#include "wiECS.h"
#include "wiScene_Decl.h"
#include "wiIntersect.h"

#include <memory>

class wiArchive;

namespace wiScene
{

class wiHairParticle
{
private:
	wiGraphics::GPUBuffer cb;
	wiGraphics::GPUBuffer particleBuffer;
	wiGraphics::GPUBuffer simulationBuffer;
	wiGraphics::GPUBuffer culledIndexBuffer;
	wiGraphics::GPUBuffer indirectBuffer;

	wiGraphics::GPUBuffer indexBuffer;
	wiGraphics::GPUBuffer vertexBuffer_length;
public:

	void UpdateCPU(const TransformComponent& transform, const MeshComponent& mesh, float dt);
	void UpdateGPU(const MeshComponent& mesh, const MaterialComponent& material, wiGraphics::CommandList cmd) const;
	void Draw(const CameraComponent& camera, const MaterialComponent& material, RENDERPASS renderPass, wiGraphics::CommandList cmd) const;

	enum FLAGS
	{
		EMPTY = 0,
		REGENERATE_FRAME = 1 << 0,
		REBUILD_BUFFERS = 1 << 1,
	};
	uint32_t _flags = EMPTY;

	wiECS::Entity meshID = wiECS::INVALID_ENTITY;

	uint32_t strandCount = 0;
	uint32_t segmentCount = 1;
	uint32_t randomSeed = 1;
	float length = 1.0f;
	float stiffness = 10.0f;
	float randomness = 0.2f;
	float viewDistance = 200;
	std::vector<float> vertex_lengths;

	// Sprite sheet properties:
	uint32_t framesX = 1;
	uint32_t framesY = 1;
	uint32_t frameCount = 1;
	uint32_t frameStart = 0;

	// Non-serialized attributes:
	XMFLOAT4X4 world;
	XMFLOAT4X4 worldPrev;
	AABB aabb;
	std::vector<uint32_t> indices; // it is dependent on vertex_lengths and contains triangles with non-zero lengths

	void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);

	static void Initialize();
};

}
