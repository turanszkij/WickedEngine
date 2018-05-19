#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "wiIntersectables.h"
#include "ShaderInterop_EmittedParticle.h"
#include "wiImageEffects.h"

struct Object;
struct Material;

class wiArchive;

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
	wiGraphicsTypes::GPUBuffer* debugDataReadbackBuffer = nullptr;
	wiGraphicsTypes::GPUBuffer* debugDataReadbackIndexBuffer = nullptr;
	wiGraphicsTypes::GPUBuffer* debugDataReadbackDistanceBuffer = nullptr;

	wiGraphicsTypes::GPUBuffer* particleBuffer = nullptr;
	wiGraphicsTypes::GPUBuffer* aliveList[2] = { nullptr, nullptr };
	wiGraphicsTypes::GPUBuffer* deadList = nullptr;
	wiGraphicsTypes::GPUBuffer* distanceBuffer = nullptr; // for sorting
	wiGraphicsTypes::GPUBuffer* sphPartitionCellIndices = nullptr; // for SPH
	wiGraphicsTypes::GPUBuffer* sphPartitionCellOffsets = nullptr; // for SPH
	wiGraphicsTypes::GPUBuffer* densityBuffer = nullptr; // for SPH
	wiGraphicsTypes::GPUBuffer* counterBuffer = nullptr;
	wiGraphicsTypes::GPUBuffer* indirectBuffers = nullptr; // kickoffUpdate, simulation, draw
	wiGraphicsTypes::GPUBuffer* constantBuffer = nullptr;
	void CreateSelfBuffers();

	static wiGraphicsTypes::ComputeShader		*kickoffUpdateCS, *finishUpdateCS, *emitCS, *sphpartitionCS, *sphpartitionoffsetsCS, *sphpartitionoffsetsresetCS, *sphdensityCS, *sphforceCS, *simulateCS, *simulateCS_SORTING, *simulateCS_DEPTHCOLLISIONS, *simulateCS_SORTING_DEPTHCOLLISIONS;
	static wiGraphicsTypes::VertexShader		*vertexShader;
	static wiGraphicsTypes::PixelShader			*pixelShader[PARTICLESHADERTYPE_COUNT];
	static wiGraphicsTypes::BlendState			blendStates[BLENDMODE_COUNT];
	static wiGraphicsTypes::RasterizerState		rasterizerState,wireFrameRS;
	static wiGraphicsTypes::DepthStencilState	depthStencilState;

	static wiGraphicsTypes::GraphicsPSO			PSO[BLENDMODE_COUNT][PARTICLESHADERTYPE_COUNT];
	static wiGraphicsTypes::GraphicsPSO			PSO_wire;
	static wiGraphicsTypes::ComputePSO			CPSO_kickoffUpdate, CPSO_finishUpdate, CPSO_emit, CPSO_sphpartition, CPSO_sphpartitionoffsets, CPSO_sphpartitionoffsetsreset, CPSO_sphdensity, CPSO_sphforce, CPSO_simulate, CPSO_simulate_SORTING, CPSO_simulate_DEPTHCOLLISIONS, CPSO_simulate_SORTING_DEPTHCOLLISIONS;

public:
	static void LoadShaders();
private:
	static void LoadBuffers();
	static void SetUpStates();

	float emit;

	bool buffersUpToDate = false;
	uint32_t MAX_PARTICLES;

public:
	wiEmittedParticle();
	wiEmittedParticle(const std::string& newName, const std::string& newMat, Object* newObject, float newSize, float newRandomFac, float newNormalFac
		,float newCount, float newLife, float newRandLife, float newScaleX, float newScaleY, float newRot);
	wiEmittedParticle(const wiEmittedParticle& other);
	static void SetUpStatic();
	static void CleanUpStatic();

	void Update(float dt);
	void Burst(float num);
	void Restart();

	void UpdateRenderData(GRAPHICSTHREAD threadID);

	void Draw(GRAPHICSTHREAD threadID);
	void CleanUp();

	bool DEBUG = false;
	ParticleCounters GetDebugData() { return debugData; }
	bool PAUSED = false;

	bool SORTING = false;
	bool DEPTHCOLLISIONS = false;
	bool SPH_FLUIDSIMULATION = false;
	float FIXED_TIMESTEP = -1.0f; // -1 : variable timestep; >=0 : fixed timestep

	PARTICLESHADERTYPE shaderType = SOFT;

	std::string name;
	Object* object;
	std::string materialName;
	Material* material;

	float size,random_factor,normal_factor;
	float count,life,random_life;
	float scaleX,scaleY,rotation;
	float motionBlurAmount;
	float mass = 1.0f;

	float SPH_h = 1.0f;		// smoothing radius
	float SPH_K = 250.0f;	// pressure constant
	float SPH_p0 = 1.0f;	// reference density
	float SPH_e = 0.018f;	// viscosity constant

	void SetMaxParticleCount(uint32_t value);
	uint32_t GetMaxParticleCount() const { return MAX_PARTICLES; }
	uint32_t GetMemorySizeInBytes() const;

	XMFLOAT3 GetPosition() const;

	void Serialize(wiArchive& archive);
};

