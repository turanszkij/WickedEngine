#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "wiIntersectables.h"
#include "ShaderInterop_EmittedParticle.h"

struct Object;
struct Material;

class wiArchive;

class wiEmittedParticle
{
private:
	ParticleCounters debugData = {};
	wiGraphicsTypes::GPUBuffer* debugDataReadbackBuffer;

	wiGraphicsTypes::GPUBuffer* particleBuffer;
	wiGraphicsTypes::GPUBuffer* aliveList[2];
	wiGraphicsTypes::GPUBuffer* deadList;
	wiGraphicsTypes::GPUBuffer* distanceBuffer; // for sorting
	wiGraphicsTypes::GPUBuffer* counterBuffer;
	wiGraphicsTypes::GPUBuffer* indirectBuffers; // kickoffUpdate, simulation, draw, kickoffSort
	wiGraphicsTypes::GPUBuffer* constantBuffer;
	void CreateSelfBuffers();

	static wiGraphicsTypes::ComputeShader		*kickoffUpdateCS, *emitCS, *simulateCS, *simulateCS_SORTING, *simulateCS_DEPTHCOLLISIONS, *simulateCS_SORTING_DEPTHCOLLISIONS;
	static wiGraphicsTypes::ComputeShader		*kickoffSortCS, *sortCS, *sortInnerCS, *sortStepCS;
	static wiGraphicsTypes::GPUBuffer			*sortCB;
	static wiGraphicsTypes::VertexShader		*vertexShader;
	static wiGraphicsTypes::PixelShader			*pixelShader,*simplestPS;
	static wiGraphicsTypes::BlendState			*blendStateAlpha,*blendStateAdd;
	static wiGraphicsTypes::RasterizerState		*rasterizerState,*wireFrameRS;
	static wiGraphicsTypes::DepthStencilState	*depthStencilState;

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

	void UpdateRenderData(GRAPHICSTHREAD threadID);

	void Draw(GRAPHICSTHREAD threadID);
	void CleanUp();

	bool DEBUG = false;
	ParticleCounters GetDebugData() { return debugData; }

	bool SORTING = false;
	bool DEPTHCOLLISIONS = false;

	std::string name;
	Object* object;
	std::string materialName;
	Material* material;

	float size,random_factor,normal_factor;
	float count,life,random_life;
	float scaleX,scaleY,rotation;
	float motionBlurAmount;

	void SetMaxParticleCount(uint32_t value);
	uint32_t GetMaxParticleCount() const { return MAX_PARTICLES; }
	uint32_t GetMemorySizeInBytes() const;

	XMFLOAT3 GetPosition() const;

	void Serialize(wiArchive& archive);
};

