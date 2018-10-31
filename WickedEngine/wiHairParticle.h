#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiECS.h"
#include "wiSceneSystem_Decl.h"
#include "wiIntersectables.h"

class wiArchive;

namespace wiSceneSystem
{

class wiHairParticle
{
private:
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> cb;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> particleBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> simulationBuffer;
public:

	void UpdateRenderData(const MeshComponent& mesh, const MaterialComponent& material, GRAPHICSTHREAD threadID);
	void Draw(const CameraComponent& camera, const MaterialComponent& material, SHADERTYPE shaderType, bool transparent, GRAPHICSTHREAD threadID) const;

	enum FLAGS
	{
		EMPTY = 0,
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

	// Non-serialized attributes:
	XMFLOAT4X4 world;
	AABB aabb;

	void Serialize(wiArchive& archive, uint32_t seed = 0);

	static void LoadShaders();
	static void Initialize();
	static void CleanUp();
};

}
