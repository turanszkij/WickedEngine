#pragma once
#include "Particle.h"

class SPTree;
struct Material;

class HairParticle :
	public Particle
{
public:
	struct Point
	{
		XMFLOAT4 posRand;
		XMFLOAT4 normalLen;
		XMFLOAT4 tangent;
	};
	struct Patch
	{
		vector<Point> p;
		XMFLOAT3 min, max;
		//ID3D11Buffer* vb;
		Patch();
		void add(const Point&);
		void CleanUp();
	};
private:
	float length;
	int count;
	string name,densityG,lenG;
	Material* material;
	struct CBGS
	{
		XMMATRIX mView;
		XMMATRIX mProj;
		XMFLOAT4 colTime;
		XMFLOAT3 eye; float drawdistance;
		XMFLOAT3 wind; float pad;
		float windRandomness;
		float windWaveSize;
		float padding[2];
	};
	static ID3D11InputLayout* il;
	static ID3D11VertexShader* vs;
	static ID3D11PixelShader* ps,*qps;
	static ID3D11GeometryShader* gs[3],*qgs[2];
	static ID3D11Buffer* cbgs;
	static ID3D11DepthStencilState* dss;
	static ID3D11RasterizerState* rs,*ncrs;
	static ID3D11SamplerState* ss;
	static ID3D11BlendState* bs;
	static int LOD[3];
public:
	HairParticle();
	HairParticle(const string& newName, float newLen, int newCount
		, const string& newMat, Object* newObject, const string& densityGroup, const string& lengthGroup);
	void CleanUp();

	void SetUpPatches();
	void Draw(const XMFLOAT3& eye, const XMMATRIX& newView, const XMMATRIX& newProj, ID3D11DeviceContext *context);

	static void CleanUpStatic();
	static void SetUpStatic();
	static void Settings(int lod0,int lod1,int lod2);

	Object* object;
	vector<Patch*> patches;
	SPTree* spTree;
	ID3D11Buffer *vb[3];
};

