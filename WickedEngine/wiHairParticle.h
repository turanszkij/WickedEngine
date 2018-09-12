#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "ShaderInterop.h"
#include "wiECS.h"
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
		XMFLOAT4 color;
		float LOD0;
		float LOD1;
		float LOD2;
		float __pad1;
	};

	std::unique_ptr<wiGraphicsTypes::GPUBuffer> cb;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> particleBuffer;

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

	void Generate(const MeshComponent& mesh);
	void Draw(wiSceneSystem::CameraComponent* camera, const MaterialComponent& material, SHADERTYPE shaderType, bool transparent, GRAPHICSTHREAD threadID) const;

	static void CleanUpStatic();
	static void SetUpStatic();
	static void Settings(int lod0,int lod1,int lod2);

	float length = 1.0f;
	size_t particleCount = 0;
	wiECS::Entity meshID = wiECS::INVALID_ENTITY;
	XMFLOAT4X4 world;
};

}
