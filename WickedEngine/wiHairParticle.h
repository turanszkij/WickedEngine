#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "ShaderInterop.h"
#include "wiSceneSystem_Decl.h"

class wiArchive;

namespace wiSceneSystem
{

class wiHairParticle
{
public:
	struct Patch
	{
		XMFLOAT4 posLen;
		UINT normalRand;
		UINT tangent;
	};
private:
	CBUFFER(ConstantBuffer, CBSLOT_OTHER_HAIRPARTICLE)
	{
		XMMATRIX mWorld;
		XMFLOAT3 color; float __pad0;
		float LOD0;
		float LOD1;
		float LOD2;
		float __pad1;
	};

	std::unique_ptr<wiGraphicsTypes::GPUBuffer> cb;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> particleBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> ib;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> ib_transposed;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> drawargs;

	static wiGraphicsTypes::VertexShader *vs;
	static wiGraphicsTypes::PixelShader *ps[SHADERTYPE_COUNT];
	static wiGraphicsTypes::PixelShader *ps_simplest;
	static wiGraphicsTypes::DepthStencilState dss_default, dss_equal, dss_rejectopaque_keeptransparent;
	static wiGraphicsTypes::RasterizerState rs, ncrs, wirers;
	static wiGraphicsTypes::BlendState bs[2]; // opaque, transparent
	static wiGraphicsTypes::GraphicsPSO PSO[SHADERTYPE_COUNT][2]; // shadertype * transparency
	static wiGraphicsTypes::GraphicsPSO PSO_wire;
	static int LOD[3];
public:
	static void LoadShaders();

public:

	void Generate();
	void ComputeCulling(wiSceneSystem::CameraComponent* camera, GRAPHICSTHREAD threadID);
	void Draw(wiSceneSystem::CameraComponent* camera, SHADERTYPE shaderType, bool transparent, GRAPHICSTHREAD threadID);

	static void CleanUpStatic();
	static void SetUpStatic();
	static void Settings(int lod0,int lod1,int lod2);

	float length = 1.0f;
	XMFLOAT4X4 OriginalMatrix_Inverse;
	size_t particleCount = 0;
};

}
