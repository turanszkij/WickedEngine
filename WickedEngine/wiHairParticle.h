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
	GFX_STRUCT ConstantBuffer
	{
		XMMATRIX mWorld;
		XMFLOAT3 color;
		float drawdistance;

		CB_SETBINDSLOT(CBSLOT_OTHER_HAIRPARTICLE)

		ALIGN_16
	};
	static wiGraphicsTypes::VertexLayout il;
	static wiGraphicsTypes::VertexShader vs;
	static wiGraphicsTypes::PixelShader ps, qps;
	static wiGraphicsTypes::GeometryShader gs[3],qgs[2];
	static wiGraphicsTypes::BufferResource cbgs;
	static wiGraphicsTypes::DepthStencilState dss;
	static wiGraphicsTypes::RasterizerState rs,ncrs;
	static wiGraphicsTypes::BlendState bs;
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
	void Draw(Camera* camera, GRAPHICSTHREAD threadID);

	static void CleanUpStatic();
	static void SetUpStatic();
	static void Settings(int lod0,int lod1,int lod2);
	
	XMFLOAT4X4 OriginalMatrix_Inverse;
	Object* object;
	vector<Patch*> patches;
	wiSPTree* spTree;
	wiGraphicsTypes::BufferResource vb[3];
};

