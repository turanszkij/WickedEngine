#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "ShaderInterop.h"
#include "ShaderInterop_EmittedParticle.h"
#include "wiIntersectables.h"

#include <set>
#include <deque>

struct SkinnedVertex;
struct Mesh;
struct Object;
struct Material;
struct Light;
struct Camera;

class wiArchive;

class wiEmittedParticle
{
private:
	//struct Point
	//{
	//	XMFLOAT3 pos;
	//	XMFLOAT4 sizOpaMir;
	//	float rot;
	//	XMFLOAT3 vel;
	//	float rotVel;
	//	float life;
	//	float maxLife;
	//	XMFLOAT2 sizBeginEnd;

	//	Point(){}
	//	Point(const XMFLOAT3& newPos, const XMFLOAT4& newSizOpaMir, const XMFLOAT3& newVel/*, const XMFLOAT3& newCol*/, float newLife, float newRotVel
	//		,float scaleX, float scaleY)
	//	{
	//		pos=newPos;
	//		sizOpaMir=newSizOpaMir;
	//		vel=newVel;
	//		//col=newCol;
	//		life=maxLife=newLife;
	//		rot=newRotVel;
	//		rotVel=newRotVel;
	//		sizBeginEnd.x = sizOpaMir.x;
	//		sizBeginEnd.y = sizOpaMir.x*scaleX;
	//	}
	//};


	//CBUFFER(ConstantBuffer, CBSLOT_OTHER_EMITTEDPARTICLE)
	//{
	//	float		mMotionBlurAmount;
	//	UINT		mEmitterMeshIndexCount;
	//	UINT		mEmitCount;
	//};

	wiGraphicsTypes::GPUBuffer* particleBuffer;
	wiGraphicsTypes::GPUBuffer* aliveList[2];
	wiGraphicsTypes::GPUBuffer* deadList;
	wiGraphicsTypes::GPUBuffer* counterBuffer;
	wiGraphicsTypes::GPUBuffer* indirectDispatchBuffer;
	wiGraphicsTypes::GPUBuffer* indirectDrawBuffer;
	wiGraphicsTypes::GPUBuffer* constantBuffer;
	void CreateSelfBuffers();

	static wiGraphicsTypes::ComputeShader *kickoffUpdateCS, *emitCS, *simulateargsCS, *drawargsCS, *simulateCS;
	static wiGraphicsTypes::VertexShader  *vertexShader;
	static wiGraphicsTypes::PixelShader   *pixelShader,*simplestPS;
	static wiGraphicsTypes::BlendState		*blendStateAlpha,*blendStateAdd;
	static wiGraphicsTypes::RasterizerState		*rasterizerState,*wireFrameRS;
	static wiGraphicsTypes::DepthStencilState	*depthStencilState;

public:
	static void LoadShaders();
private:
	static void LoadBuffers();
	static void SetUpStates();

	float emit;

	uint32_t MAX_PARTICLES;

public:
	wiEmittedParticle();
	wiEmittedParticle(const std::string& newName, const std::string& newMat, Object* newObject, float newSize, float newRandomFac, float newNormalFac
		,float newCount, float newLife, float newRandLife, float newScaleX, float newScaleY, float newRot);
	static void SetUpStatic();
	static void CleanUpStatic();

	void Update(float dt);
	void Burst(float num);

	void UpdateRenderData(GRAPHICSTHREAD threadID);

	void Draw(GRAPHICSTHREAD threadID);
	void CleanUp();

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

