#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "ShaderInterop.h"
#include "wiECS.h"
#include "wiSceneSystem_Decl.h"
#include "wiIntersectables.h"

class wiArchive;

namespace wiSceneSystem
{

class wiHairParticle
{
private:
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> cb;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> particleBuffer;
	std::unique_ptr<wiGraphicsTypes::GPUBuffer> simulationBuffer;

	static wiGraphicsTypes::VertexShader *vs;
	static wiGraphicsTypes::PixelShader *ps[SHADERTYPE_COUNT];
	static wiGraphicsTypes::PixelShader *ps_simplest;
	static wiGraphicsTypes::ComputeShader *cs_simulate;
	static wiGraphicsTypes::DepthStencilState dss_default, dss_equal, dss_rejectopaque_keeptransparent;
	static wiGraphicsTypes::RasterizerState rs, ncrs, wirers;
	static wiGraphicsTypes::BlendState bs[2]; // opaque, transparent
	static wiGraphicsTypes::GraphicsPSO PSO[SHADERTYPE_COUNT][2]; // shadertype * transparency
	static wiGraphicsTypes::GraphicsPSO PSO_wire;
	static wiGraphicsTypes::ComputePSO CPSO_simulate;
	static int LOD[3];
public:
	static void LoadShaders();

	void UpdateRenderData(const MeshComponent& mesh, const MaterialComponent& material, GRAPHICSTHREAD threadID);
	void Draw(CameraComponent* camera, const MaterialComponent& material, SHADERTYPE shaderType, bool transparent, GRAPHICSTHREAD threadID) const;

	static void CleanUpStatic();
	static void SetUpStatic();

	uint32_t strandCount = 0;
	float length = 1.0f;
	float stiffness = 10.0f;
	float randomness = 0.2f;
	uint32_t segmentCount = 1;
	uint32_t randomSeed = 1;
	float viewDistance = 200;

	wiECS::Entity meshID = wiECS::INVALID_ENTITY;
	XMFLOAT4X4 world;
	AABB aabb;

	void Serialize(wiArchive& archive);
};

}
