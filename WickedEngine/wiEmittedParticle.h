#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiIntersect.h"
#include "ShaderInterop_EmittedParticle.h"
#include "wiEnums.h"
#include "wiScene_Decl.h"
#include "wiECS.h"

#include <memory>

class wiArchive;

namespace wiScene
{

class wiEmittedParticle
{
public:

	// This is serialized, order of enums shouldn't change!
	enum PARTICLESHADERTYPE
	{
		SOFT,
		SOFT_DISTORTION,
		SIMPLEST,
		SOFT_LIGHTING,
		PARTICLESHADERTYPE_COUNT,
		ENUM_FORCE_UINT32 = 0xFFFFFFFF,
	};

private:
	ParticleCounters statistics = {};
	wiGraphics::GPUBuffer statisticsReadbackBuffer[wiGraphics::GraphicsDevice::GetBackBufferCount() + 2];

	wiGraphics::GPUBuffer particleBuffer;
	wiGraphics::GPUBuffer aliveList[2];
	wiGraphics::GPUBuffer deadList;
	wiGraphics::GPUBuffer distanceBuffer; // for sorting
	wiGraphics::GPUBuffer sphPartitionCellIndices; // for SPH
	wiGraphics::GPUBuffer sphPartitionCellOffsets; // for SPH
	wiGraphics::GPUBuffer densityBuffer; // for SPH
	wiGraphics::GPUBuffer counterBuffer;
	wiGraphics::GPUBuffer indirectBuffers; // kickoffUpdate, simulation, draw
	wiGraphics::GPUBuffer constantBuffer;
	void CreateSelfBuffers();

	float emit = 0.0f;
	int burst = 0;

	bool buffersUpToDate = false;
	uint32_t MAX_PARTICLES = 1000;

public:
	void UpdateCPU(const TransformComponent& transform, float dt);
	void Burst(int num);
	void Restart();

	// Must have a transform and material component, but mesh is optional
	void UpdateGPU(const TransformComponent& transform, const MaterialComponent& material, const MeshComponent* mesh, wiGraphics::CommandList cmd) const;
	void Draw(const CameraComponent& camera, const MaterialComponent& material, wiGraphics::CommandList cmd) const;

	ParticleCounters GetStatistics() { return statistics; }

	enum FLAGS
	{
		FLAG_EMPTY = 0,
		FLAG_DEBUG = 1 << 0,
		FLAG_PAUSED = 1 << 1,
		FLAG_SORTING = 1 << 2,
		FLAG_DEPTHCOLLISION = 1 << 3,
		FLAG_SPH_FLUIDSIMULATION = 1 << 4,
		FLAG_HAS_VOLUME = 1 << 5,
		FLAG_FRAME_BLENDING = 1 << 6,
	};
	uint32_t _flags = FLAG_EMPTY;

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
	float random_color = 0;

	XMFLOAT3 velocity = {}; // starting velocity of all new particles
	XMFLOAT3 gravity = {}; // constant gravity force
	float drag = 1.0f; // constant drag (per frame velocity multiplier, reducing it will make particles slow down over time)

	float SPH_h = 1.0f;		// smoothing radius
	float SPH_K = 250.0f;	// pressure constant
	float SPH_p0 = 1.0f;	// reference density
	float SPH_e = 0.018f;	// viscosity constant

	// Sprite sheet properties:
	uint32_t framesX = 1;
	uint32_t framesY = 1;
	uint32_t frameCount = 1;
	uint32_t frameStart = 0;
	float frameRate = 0; // frames per second

	void SetMaxParticleCount(uint32_t value);
	uint32_t GetMaxParticleCount() const { return MAX_PARTICLES; }
	uint32_t GetMemorySizeInBytes() const;

	// Non-serialized attributes:
	XMFLOAT3 center;
	uint32_t statisticsReadBackIndex = 0;

	inline bool IsDebug() const { return _flags & FLAG_DEBUG; }
	inline bool IsPaused() const { return _flags & FLAG_PAUSED; }
	inline bool IsSorted() const { return _flags & FLAG_SORTING; }
	inline bool IsDepthCollisionEnabled() const { return _flags & FLAG_DEPTHCOLLISION; }
	inline bool IsSPHEnabled() const { return _flags & FLAG_SPH_FLUIDSIMULATION; }
	inline bool IsVolumeEnabled() const { return _flags & FLAG_HAS_VOLUME; }
	inline bool IsFrameBlendingEnabled() const { return _flags & FLAG_FRAME_BLENDING; }

	inline void SetDebug(bool value) { if (value) { _flags |= FLAG_DEBUG; } else { _flags &= ~FLAG_DEBUG; } }
	inline void SetPaused(bool value) { if (value) { _flags |= FLAG_PAUSED; } else { _flags &= ~FLAG_PAUSED; } }
	inline void SetSorted(bool value) { if (value) { _flags |= FLAG_SORTING; } else { _flags &= ~FLAG_SORTING; } }
	inline void SetDepthCollisionEnabled(bool value) { if (value) { _flags |= FLAG_DEPTHCOLLISION; } else { _flags &= ~FLAG_DEPTHCOLLISION; } }
	inline void SetSPHEnabled(bool value) { if (value) { _flags |= FLAG_SPH_FLUIDSIMULATION; } else { _flags &= ~FLAG_SPH_FLUIDSIMULATION; } }
	inline void SetVolumeEnabled(bool value) { if (value) { _flags |= FLAG_HAS_VOLUME; } else { _flags &= ~FLAG_HAS_VOLUME; } }
	inline void SetFrameBlendingEnabled(bool value) { if (value) { _flags |= FLAG_FRAME_BLENDING; } else { _flags &= ~FLAG_FRAME_BLENDING; } }

	void Serialize(wiArchive& archive, wiECS::EntitySerializer& seri);

	static void Initialize();
};

}

