#pragma once
#include "wiParticle.h"

struct Material;
struct AABB;
struct Light;

class wiEmittedParticle :
	public wiParticle
{
private:
	struct Point
	{
		XMFLOAT3 pos;
		XMFLOAT4 sizOpaMir;
		//XMFLOAT3 col;
		float rot,rotVel;
		XMFLOAT3 vel;
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

	struct ConstantBuffer
	{
		XMMATRIX mView;
		XMMATRIX mProjection;
		XMVECTOR mCamPos;
		XMFLOAT4 mAdd;
	};
	ID3D11Buffer		   *vertexBuffer;
	static ID3D11InputLayout   *vertexLayout;
	static ID3D11VertexShader  *vertexShader;
	static ID3D11PixelShader   *pixelShader,*simplestPS;
	static ID3D11GeometryShader*geometryShader;
	static ID3D11Buffer*           constantBuffer;
	static ID3D11BlendState*		blendStateAlpha,*blendStateAdd;
	static ID3D11SamplerState*			sampleState;
	static ID3D11RasterizerState*		rasterizerState,*wireFrameRS;
	static ID3D11DepthStencilState*	depthStencilState;

	static void LoadShaders();
	static void SetUpCB();
	static void SetUpStates();
	void LoadVertexBuffer();

	static set<wiEmittedParticle*> systems;
	

	//std::vector<SkinnedVertex> emitterVertexList;
	Object* object;
	
	float getNewVelocityModifier(){ return 1+((rand()%10001+1)*0.0001)*random_factor;}
	float getNewPositionModifier(){ return (rand()%((int)(random_factor*1000)+1))*0.001f - random_factor*0.5f; }
	float getNewRotationModifier(){ return (rand()%((int)(random_factor*1000)+1))*0.001f - random_factor*0.5f; }
	float getNewLifeSpan(){ return life + (rand()%((int)(random_life*1000)+1))*0.001f - random_life*0.5f; }
	int getRandomPointOnEmitter();

	XMFLOAT4X4 transform4;
	XMFLOAT3X3 transform3;
	void addPoint(const XMMATRIX& t4, const XMMATRIX& t3);

	float emit;

public:
	wiEmittedParticle(){};
	wiEmittedParticle(std::string newName, std::string newMat, Object* newObject, float newSize, float newRandomFac, float newNormalFac
		,float newCount, float newLife, float newRandLife, float newScaleX, float newScaleY, float newRot);
	void* operator new(size_t);
	void operator delete(void*);
	static void SetUpStatic();
	static void CleanUpStatic();

	long getCount();
	static long getNumwiParticles();

	void Update(float gamespeed);
	void Burst(float num);

#define DRAW_DEFAULT 0
#define DRAW_DARK 1
	void Draw(const XMVECTOR eye, const XMMATRIX& view, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, int FLAG = DRAW_DEFAULT);
	void DrawPremul(const XMVECTOR eye, const XMMATRIX& view, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, int FLAG = DRAW_DEFAULT);
	void DrawNonPremul(const XMVECTOR eye, const XMMATRIX& view, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, int FLAG = DRAW_DEFAULT);
	void CleanUp();

	std::string name;
	//std::string material;
	Material* material;
	Light* light;
	
	AABB* bounding_box;
	long lastSquaredDistMulThousand;

	float size,random_factor,normal_factor;
	float count,life,random_life;
	float scaleX,scaleY,rotation;
};

