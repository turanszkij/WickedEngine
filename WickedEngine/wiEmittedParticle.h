#pragma once
#include "wiParticle.h"
#include "ConstantBufferMapping.h"

struct Material;
struct AABB;
struct Light;
struct Camera;

class wiArchive;

class wiEmittedParticle :
	public wiParticle
{
private:
	struct Point
	{
		XMFLOAT3 pos;
		XMFLOAT4 sizOpaMir;
		//XMFLOAT3 col;
		float rot;
		XMFLOAT3 vel;
		float rotVel;
		float life;
		float maxLife;
		float sizBeginEnd[2];

		Point(){}
		Point(const XMFLOAT3& newPos, const XMFLOAT4& newSizOpaMir, const XMFLOAT3& newVel/*, const XMFLOAT3& newCol*/, float newLife, float newRotVel
			,float scaleX, float scaleY)
		{
			pos=newPos;
			sizOpaMir=newSizOpaMir;
			vel=newVel;
			//col=newCol;
			life=maxLife=newLife;
			rot=newRotVel;
			rotVel=newRotVel;
			sizBeginEnd[0] = sizOpaMir.x;
			sizBeginEnd[1] = sizOpaMir.x*scaleX;
		}
	};
	deque<Point> points;

	GFX_STRUCT ConstantBuffer
	{
		XMFLOAT2	mAdd;
		float		mMotionBlurAmount;
		float		padding;

		CB_SETBINDSLOT(CBSLOT_OTHER_EMITTEDPARTICLE)

		ALIGN_16
	};
	wiGraphicsTypes::GPUBuffer		   *vertexBuffer;
	static wiGraphicsTypes::VertexLayout   *vertexLayout;
	static wiGraphicsTypes::VertexShader  *vertexShader;
	static wiGraphicsTypes::PixelShader   *pixelShader,*simplestPS;
	static wiGraphicsTypes::GeometryShader *geometryShader;
	static wiGraphicsTypes::GPUBuffer           *constantBuffer;
	static wiGraphicsTypes::BlendState		*blendStateAlpha,*blendStateAdd;
	static wiGraphicsTypes::RasterizerState		*rasterizerState,*wireFrameRS;
	static wiGraphicsTypes::DepthStencilState	*depthStencilState;

public:
	static void LoadShaders();
private:
	static void SetUpCB();
	static void SetUpStates();
	void LoadVertexBuffer();

	static set<wiEmittedParticle*> systems;
	

	//std::vector<SkinnedVertex> emitterVertexList;
	
	float getNewVelocityModifier(){ return 1+((rand()%10001+1)*0.0001f)*random_factor;}
	float getNewPositionModifier(){ return (rand()%((int)(random_factor*1000)+1))*0.001f - random_factor*0.5f; }
	float getNewRotationModifier(){ return (rand()%((int)(random_factor*1000)+1))*0.001f - random_factor*0.5f; }
	float getNewLifeSpan(){ return life + (rand()%((int)(random_life*1000)+1))*0.001f - random_life*0.5f; }
	int getRandomPointOnEmitter();

	XMFLOAT4X4 transform4;
	XMFLOAT3X3 transform3;
	void addPoint(const XMMATRIX& t4, const XMMATRIX& t3);

	float emit;

	XMFLOAT3* posSamples;
	float* radSamples;
	float* energySamples;
	int currentSample;
	void SetupLightInterpolators();
public:
	wiEmittedParticle();
	wiEmittedParticle(std::string newName, std::string newMat, Object* newObject, float newSize, float newRandomFac, float newNormalFac
		,float newCount, float newLife, float newRandLife, float newScaleX, float newScaleY, float newRot);
	static void SetUpStatic();
	static void CleanUpStatic();

	int getCount();
	static long getNumParticles();

	void Update(float dt);
	void Burst(float num);

	void UpdateRenderData(GRAPHICSTHREAD threadID);

#define DRAW_DEFAULT 0
#define DRAW_DARK 1
	void Draw(GRAPHICSTHREAD threadID, int FLAG = DRAW_DEFAULT);
	void DrawPremul(GRAPHICSTHREAD threadID, int FLAG = DRAW_DEFAULT);
	void DrawNonPremul(GRAPHICSTHREAD threadID, int FLAG = DRAW_DEFAULT);
	void CleanUp();

	string name;
	Object* object;
	string materialName;
	Material* material;
	Light* light;
	string lightName;
	
	AABB* bounding_box;

	float size,random_factor,normal_factor;
	float count,life,random_life;
	float scaleX,scaleY,rotation;
	float motionBlurAmount;

	void CreateLight();

	void Serialize(wiArchive& archive);
};

