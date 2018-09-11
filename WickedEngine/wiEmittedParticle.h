#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "wiIntersectables.h"
#include "ShaderInterop_EmittedParticle.h"
#include "wiImageEffects.h"
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

	static wiGraphicsTypes::ComputeShader		*kickoffUpdateCS, *finishUpdateCS, *emitCS, *emitCS_FROMMESH, *sphpartitionCS, *sphpartitionoffsetsCS, *sphpartitionoffsetsresetCS, *sphdensityCS, *sphforceCS, *simulateCS, *simulateCS_SORTING, *simulateCS_DEPTHCOLLISIONS, *simulateCS_SORTING_DEPTHCOLLISIONS;
	static wiGraphicsTypes::VertexShader		*vertexShader;
	static wiGraphicsTypes::PixelShader			*pixelShader[PARTICLESHADERTYPE_COUNT];
	static wiGraphicsTypes::BlendState			blendStates[BLENDMODE_COUNT];
	static wiGraphicsTypes::RasterizerState		rasterizerState,wireFrameRS;
	static wiGraphicsTypes::DepthStencilState	depthStencilState;

	static wiGraphicsTypes::GraphicsPSO			PSO[BLENDMODE_COUNT][PARTICLESHADERTYPE_COUNT];
	static wiGraphicsTypes::GraphicsPSO			PSO_wire;
	static wiGraphicsTypes::ComputePSO			CPSO_kickoffUpdate, CPSO_finishUpdate, CPSO_emit, CPSO_emit_FROMMESH, CPSO_sphpartition, CPSO_sphpartitionoffsets, CPSO_sphpartitionoffsetsreset, CPSO_sphdensity, CPSO_sphforce, CPSO_simulate, CPSO_simulate_SORTING, CPSO_simulate_DEPTHCOLLISIONS, CPSO_simulate_SORTING_DEPTHCOLLISIONS;

public:
	static void LoadShaders();
	static void SetUpStatic();
	static void CleanUpStatic();
private:
	static void LoadBuffers();
	static void SetUpStates();

	float emit = 0.0f;

	bool buffersUpToDate = false;
	uint32_t MAX_PARTICLES = 1000;

public:
	void Update(float dt);
	void Burst(float num);
	void Restart();

	wiECS::Entity meshID = wiECS::INVALID_ENTITY;

	// Must have a transform and material component, but mesh is optional
	void UpdateRenderData(const TransformComponent& transform, const MaterialComponent& material, const MeshComponent* mesh, GRAPHICSTHREAD threadID);
	void Draw(const MaterialComponent& material, GRAPHICSTHREAD threadID);

	bool DEBUG = false;
	ParticleCounters GetDebugData() { return debugData; }
	bool PAUSED = false;

	bool SORTING = false;
	bool DEPTHCOLLISIONS = false;
	bool SPH_FLUIDSIMULATION = false;
	float FIXED_TIMESTEP = -1.0f; // -1 : variable timestep; >=0 : fixed timestep

	PARTICLESHADERTYPE shaderType = SOFT;

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
};

}

