#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiIntersectables.h"
#include "ShaderInterop_EmittedParticle.h"
#include "wiEnums.h"
#include "wiSceneSystem_Decl.h"
#include "wiECS.h"

class wiArchive;

namespace wiSceneSystem
{

class wiEmittedParticle
{
public:
	enum PARTICLESHADERTYPE
	{
		SOFT,
		SOFT_DISTORTION,
		SIMPLEST,
		PARTICLESHADERTYPE_COUNT,
		ENUM_FORCE_UINT32 = 0xFFFFFFFF,
	};

private:
	ParticleCounters debugData = {};
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> debugDataReadbackBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> debugDataReadbackIndexBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> debugDataReadbackDistanceBuffer;

	std::unique_ptr<wiGraphicsTypes::GPUBuffer> particleBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> aliveList[2];
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> deadList;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> distanceBuffer; // for sorting
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> sphPartitionCellIndices; // for SPH
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> sphPartitionCellOffsets; // for SPH
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> densityBuffer; // for SPH
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> counterBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> indirectBuffers; // kickoffUpdate, simulation, draw
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> constantBuffer;
	void CreateSelfBuffers();

	float emit = 0.0f;

	bool buffersUpToDate = false;
	uint32_t MAX_PARTICLES = 1000;

public:
	void Update(float dt);
	void Burst(float num);
	void Restart();

	// Must have a transform and material component, but mesh is optional
	void UpdateRenderData(const TransformComponent& transform, const MaterialComponent& material, const MeshComponent* mesh, GRAPHICSTHREAD threadID);
	void Draw(const CameraComponent& camera, const MaterialComponent& material, GRAPHICSTHREAD threadID);

	ParticleCounters GetDebugData() { return debugData; }

	enum FLAGS
	{
		EMPTY = 0,
		DEBUG = 1 << 0,
		PAUSED = 1 << 1,
		SORTING = 1 << 2,
		DEPTHCOLLISION = 1 << 3,
		SPH_FLUIDSIMULATION = 1 << 4,
	};
	uint32_t _flags = EMPTY;

	PARTICLESHADERTYPE shaderType = SOFT;

	wiECS::Entity meshID = wiECS::INVALID_ENTITY;

	float FIXED_TIMESTEP = -1.0f; // -1 : variable timestep; >=0 : fixed timestep

	float size = 1.0f;
	float random_factor = 1.0f;
	float normal_factor = 1.0f;
	float count = 0.0f;
	float life = 1.0f;
	float random_life = 1.0f;
	float scaleX = 1.0f;
	float scaleY = 1.0f;
	float rotation = 0.0f;
	float motionBlurAmount = 0.0f;
	float mass = 1.0f;

	float SPH_h = 1.0f;		// smoothing radius
	float SPH_K = 250.0f;	// pressure constant
	float SPH_p0 = 1.0f;	// reference density
	float SPH_e = 0.018f;	// viscosity constant

	void SetMaxParticleCount(uint32_t value);
	uint32_t GetMaxParticleCount() const { return MAX_PARTICLES; }
	uint32_t GetMemorySizeInBytes() const;

	// Non-serialized attributes:
	XMFLOAT3 center;

	inline bool IsDebug() const { return _flags & DEBUG; }
	inline bool IsPaused() const { return _flags & PAUSED; }
	inline bool IsSorted() const { return _flags & SORTING; }
	inline bool IsDepthCollisionEnabled() const { return _flags & DEPTHCOLLISION; }
	inline bool IsSPHEnabled() const { return _flags & SPH_FLUIDSIMULATION; }

	inline void SetDebug(bool value) { if (value) { _flags |= DEBUG; } else { _flags &= ~DEBUG; } }
	inline void SetPaused(bool value) { if (value) { _flags |= PAUSED; } else { _flags &= ~PAUSED; } }
	inline void SetSorted(bool value) { if (value) { _flags |= SORTING; } else { _flags &= ~SORTING; } }
	inline void SetDepthCollisionEnabled(bool value) { if (value) { _flags |= DEPTHCOLLISION; } else { _flags &= ~DEPTHCOLLISION; } }
	inline void SetSPHEnabled(bool value) { if (value) { _flags |= SPH_FLUIDSIMULATION; } else { _flags &= ~SPH_FLUIDSIMULATION; } }

	void Serialize(wiArchive& archive, uint32_t seed = 0);

	static void LoadShaders();
	static void Initialize();
	static void CleanUp();
};

}

