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
	wiGraphicsTypes::GPUBuffer* indirectSimulateBuffer;
	wiGraphicsTypes::GPUBuffer* indirectDrawBuffer;
	wiGraphicsTypes::GPUBuffer* constantBuffer;
	void CreateSelfBuffers();

	static wiGraphicsTypes::ComputeShader *emitCS, *simulateargsCS, *drawargsCS, *simulateCS;
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
	
	float getNewVelocityModifier(){ return 1+((rand()%10001+1)*0.0001f)*random_factor;}
	float getNewPositionModifier(){ return (rand()%((int)(random_factor*1000)+1))*0.001f - random_factor*0.5f; }
	float getNewRotationModifier(){ return (rand()%((int)(random_factor*1000)+1))*0.001f - random_factor*0.5f; }
	float getNewLifeSpan(){ return life + (rand()%((int)(random_life*1000)+1))*0.001f - random_life*0.5f; }
	int getRandomPointOnEmitter();

	XMFLOAT4X4 transform4;
	XMFLOAT3X3 transform3;
	void addPoint(const XMMATRIX& t4, const XMMATRIX& t3);

	float emit;

	//XMFLOAT3* posSamples;
	//float* radSamples;
	//float* energySamples;
	//int currentSample;
	void SetupLightInterpolators();
public:
	wiEmittedParticle();
	wiEmittedParticle(const std::string& newName, const std::string& newMat, Object* newObject, float newSize, float newRandomFac, float newNormalFac
		,float newCount, float newLife, float newRandLife, float newScaleX, float newScaleY, float newRot);
	static void SetUpStatic();
	static void CleanUpStatic();

	int getCount();

	void Update(float dt);
	void Burst(float num);

	void UpdateAttachedLight(float dt);
	void UpdateRenderData(GRAPHICSTHREAD threadID);

	void Draw(GRAPHICSTHREAD threadID);
	void CleanUp();

	std::string name;
	Object* object;
	std::string materialName;
	Material* material;
	Light* light;
	std::string lightName;
	
	AABB bounding_box;

	float size,random_factor,normal_factor;
	float count,life,random_life;
	float scaleX,scaleY,rotation;
	float motionBlurAmount;

	void CreateLight();

	void Serialize(wiArchive& archive);
};

