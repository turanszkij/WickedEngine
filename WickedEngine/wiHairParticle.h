#pragma once
#include "wiParticle.h"
#include "ConstantBufferMapping.h"

class wiSPTree;
struct Material;
struct Camera;

class wiArchive;

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
		//GPUBuffer vb;
		Patch();
		void add(const Point&);
		void CleanUp();
	};
private:
	GFX_STRUCT ConstantBuffer
	{
		XMMATRIX mWorld;
		XMFLOAT3 color;
		float drawdistance;

		CB_SETBINDSLOT(CBSLOT_OTHER_HAIRPARTICLE)

		ALIGN_16
	};
	static wiGraphicsTypes::VertexLayout *il;
	static wiGraphicsTypes::VertexShader *vs;
	static wiGraphicsTypes::PixelShader *ps[SHADERTYPE_COUNT], *qps[SHADERTYPE_COUNT];
	static wiGraphicsTypes::GeometryShader *gs[3],*qgs[2];
	static wiGraphicsTypes::GPUBuffer *cbgs;
	static wiGraphicsTypes::DepthStencilState *dss;
	static wiGraphicsTypes::RasterizerState *rs,*ncrs;
	static wiGraphicsTypes::BlendState *bs;
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
	void Draw(Camera* camera, SHADERTYPE shaderType, GRAPHICSTHREAD threadID);

	static void CleanUpStatic();
	static void SetUpStatic();
	static void Settings(int lod0,int lod1,int lod2);

	float length;
	int count;
	string name, densityG, lenG, materialName;
	Material* material;
	XMFLOAT4X4 OriginalMatrix_Inverse;
	Object* object;
	vector<Patch*> patches;
	wiSPTree* spTree;
	wiGraphicsTypes::GPUBuffer *vb[3];

	void Serialize(wiArchive& archive);
};

