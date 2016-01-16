#pragma once
#include "wiParticle.h"

class wiSPTree;
struct Material;
struct Camera;

class wiHairParticle :
	public wiParticle
{
public:
	GFX_STRUCT Point
	{
		XMFLOAT4 posRand;
		XMFLOAT4 normalLen;
		XMFLOAT4 tangent;

		ALIGN_16
	};
	struct Patch
	{
		vector<Point> p;
		XMFLOAT3 min, max;
		//BufferResource vb;
		Patch();
		void add(const Point&);
		void CleanUp();
	};
private:
	float length;
	int count;
	string name,densityG,lenG;
	Material* material;
	GFX_STRUCT CBGS
	{
		XMMATRIX mWorld;
		XMMATRIX mView;
		XMMATRIX mProj;
		XMMATRIX mPrevViewProjection;
		XMFLOAT4 colTime;
		XMFLOAT3 eye; float drawdistance;
		XMFLOAT3 wind; float pad;
		float windRandomness;
		float windWaveSize;
		float padding[2];

		ALIGN_16
	};
	static VertexLayout il;
	static VertexShader vs;
	static PixelShader ps, qps;
	static GeometryShader gs[3],qgs[2];
	static BufferResource cbgs;
	static DepthStencilState dss;
	static RasterizerState rs,ncrs;
	static Sampler ss;
	static BlendState bs;
	static int LOD[3];
public:
	static void LoadShaders();
private:

public:
	wiHairParticle();
	wiHairParticle(const string& newName, float newLen, int newCount
		, const string& newMat, Object* newObject, const string& densityGroup, const string& lengthGroup);
	void CleanUp();

	void SetUpPatches();
	void Draw(Camera* camera, ID3D11DeviceContext *context);

	static void CleanUpStatic();
	static void SetUpStatic();
	static void Settings(int lod0,int lod1,int lod2);
	
	XMFLOAT4X4 OriginalMatrix_Inverse;
	Object* object;
	vector<Patch*> patches;
	wiSPTree* spTree;
	BufferResource vb[3];
};

