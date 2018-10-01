#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiGraphicsAPI.h"
#include "wiSceneSystem_Decl.h"
#include "wiECS.h"

struct RAY;

namespace wiRenderer
{
	static const wiGraphicsTypes::FORMAT RTFormat_ldr = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_hdr = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_0 = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_1 = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_2 = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_3 = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	// NOTE: Light buffer precision seems OK when using FORMAT_R11G11B10_FLOAT format
	// But the environmental light now also writes the AO to ALPHA so it has been changed to FORMAT_R16G16B16A16_FLOAT
	static const wiGraphicsTypes::FORMAT RTFormat_deferred_lightbuffer = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_lineardepth = wiGraphicsTypes::FORMAT_R16_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_ssao = wiGraphicsTypes::FORMAT_R8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_waterripple = wiGraphicsTypes::FORMAT_R8G8B8A8_SNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_normalmaps = wiGraphicsTypes::FORMAT_R8G8B8A8_SNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_depthresolve = wiGraphicsTypes::FORMAT_R32_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_voxelradiance = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_envprobe = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_impostor = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;

	static const wiGraphicsTypes::FORMAT DSFormat_full = wiGraphicsTypes::FORMAT_D32_FLOAT_S8X24_UINT;
	static const wiGraphicsTypes::FORMAT DSFormat_full_alias = wiGraphicsTypes::FORMAT_R32G8X24_TYPELESS;
	static const wiGraphicsTypes::FORMAT DSFormat_small = wiGraphicsTypes::FORMAT_D16_UNORM;
	static const wiGraphicsTypes::FORMAT DSFormat_small_alias = wiGraphicsTypes::FORMAT_R16_TYPELESS;

	wiGraphicsTypes::Sampler* GetSampler(int slot);
	wiGraphicsTypes::VertexShader* GetVertexShader(VSTYPES id);
	wiGraphicsTypes::HullShader* GetHullShader(VSTYPES id);
	wiGraphicsTypes::DomainShader* GetDomainShader(VSTYPES id);
	wiGraphicsTypes::GeometryShader* GetGeometryShader(VSTYPES id);
	wiGraphicsTypes::PixelShader* GetPixelShader(PSTYPES id);
	wiGraphicsTypes::ComputeShader* GetComputeShader(PSTYPES id);
	wiGraphicsTypes::VertexLayout* GetVertexLayout(VLTYPES id);
	wiGraphicsTypes::RasterizerState* GetRasterizerState(RSTYPES id);
	wiGraphicsTypes::DepthStencilState* GetDepthStencilState(DSSTYPES id);
	wiGraphicsTypes::BlendState* GetBlendState(BSTYPES id);
	wiGraphicsTypes::GPUBuffer* GetConstantBuffer(CBTYPES id);
	wiGraphicsTypes::Texture* GetTexture(TEXTYPES id);

	void ModifySampler(const wiGraphicsTypes::SamplerDesc& desc, int slot);


	void Initialize();
	void CleanUp();
	void LoadBuffers();
	void LoadShaders();
	void SetUpStates();
	void ClearWorld();

	void SetDevice(wiGraphicsTypes::GraphicsDevice* newDevice);
	wiGraphicsTypes::GraphicsDevice* GetDevice();

	std::string& GetShaderPath();
	void ReloadShaders(const std::string& path = "");

	wiSceneSystem::Scene& GetScene();
	wiSceneSystem::CameraComponent& GetCamera();
	wiSceneSystem::CameraComponent& GetPrevCamera();
	wiSceneSystem::CameraComponent& GetRefCamera();

	void FixedUpdate();
	void UpdatePerFrameData(float dt);
	void UpdateRenderData(GRAPHICSTHREAD threadID);


	void BindPersistentState(GRAPHICSTHREAD threadID);
	void UpdateFrameCB(GRAPHICSTHREAD threadID);
	void UpdateCameraCB(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	void SetClipPlane(const XMFLOAT4& clipPlane, GRAPHICSTHREAD threadID);
	void SetAlphaRef(float alphaRef, GRAPHICSTHREAD threadID);
	inline void ResetAlphaRef(GRAPHICSTHREAD threadID) { SetAlphaRef(0.75f, threadID); }
	void BindGBufferTextures(wiGraphicsTypes::Texture2D* slot0, wiGraphicsTypes::Texture2D* slot1, wiGraphicsTypes::Texture2D* slot2, wiGraphicsTypes::Texture2D* slot3, wiGraphicsTypes::Texture2D* slot4, GRAPHICSTHREAD threadID);
	void BindDepthTextures(wiGraphicsTypes::Texture2D* depth, wiGraphicsTypes::Texture2D* linearDepth, GRAPHICSTHREAD threadID);

	void DrawSky(GRAPHICSTHREAD threadID);
	void DrawSun(GRAPHICSTHREAD threadID);
	void DrawWorld(const wiSceneSystem::CameraComponent& camera, bool tessellation, GRAPHICSTHREAD threadID, SHADERTYPE shaderType, bool grass, bool occlusionCulling, uint32_t layerMask = 0xFFFFFFFF);
	void DrawForShadowMap(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID, uint32_t layerMask = 0xFFFFFFFF);
	void DrawWorldTransparent(const wiSceneSystem::CameraComponent& camera, SHADERTYPE shaderType, GRAPHICSTHREAD threadID, bool grass, bool occlusionCulling, uint32_t layerMask = 0xFFFFFFFF);
	void DrawDebugWorld(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	void DrawSoftParticles(const wiSceneSystem::CameraComponent& camera, bool distortion, GRAPHICSTHREAD threadID);
	void DrawTrails(GRAPHICSTHREAD threadID, wiGraphicsTypes::Texture2D* refracRes);
	void DrawLights(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	void DrawLightVisualizers(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	void DrawVolumeLights(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	void DrawLensFlares(GRAPHICSTHREAD threadID);
	void DrawDecals(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	void RefreshEnvProbes(GRAPHICSTHREAD threadID);
	void RefreshImpostors(GRAPHICSTHREAD threadID);
	void VoxelRadiance(GRAPHICSTHREAD threadID);
	void ComputeTiledLightCulling(bool deferred, GRAPHICSTHREAD threadID);
	void ResolveMSAADepthBuffer(wiGraphicsTypes::Texture2D* dst, wiGraphicsTypes::Texture2D* src, GRAPHICSTHREAD threadID);
	void BuildSceneBVH(GRAPHICSTHREAD threadID);
	void DrawTracedScene(const wiSceneSystem::CameraComponent& camera, wiGraphicsTypes::Texture2D* result, GRAPHICSTHREAD threadID);

	void OcclusionCulling_Render(GRAPHICSTHREAD threadID);
	void OcclusionCulling_Read();
	void EndFrame();


	enum MIPGENFILTER
	{
		MIPGENFILTER_POINT,
		MIPGENFILTER_LINEAR,
		MIPGENFILTER_LINEAR_MAXIMUM,
		MIPGENFILTER_GAUSSIAN,
	};
	void GenerateMipChain(wiGraphicsTypes::Texture1D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID, int arrayIndex = -1);
	void GenerateMipChain(wiGraphicsTypes::Texture2D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID, int arrayIndex = -1);
	void GenerateMipChain(wiGraphicsTypes::Texture3D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID, int arrayIndex = -1);

	enum BORDEREXPANDSTYLE
	{
		BORDEREXPAND_DISABLE,
		BORDEREXPAND_WRAP,
		BORDEREXPAND_CLAMP,
	};
	// Performs copy operation even between different texture formats
	//	Can also expand border region according to desired sampler func
	void CopyTexture2D(wiGraphicsTypes::Texture2D* dst, UINT DstMIP, UINT DstX, UINT DstY, wiGraphicsTypes::Texture2D* src, UINT SrcMIP, GRAPHICSTHREAD threadID,
		BORDEREXPANDSTYLE borderExpand = BORDEREXPAND_DISABLE);

	// dst: Texture2D with unordered access, the output will be written to this
	// refinementCount: 0: auto select, 1: perfect noise, greater numbers: smoother clouds, slower processing
	// randomness: random seed
	void GenerateClouds(wiGraphicsTypes::Texture2D* dst, UINT refinementCount, float randomness, GRAPHICSTHREAD threadID);

	void ManageDecalAtlas(GRAPHICSTHREAD threadID);

	void PutWaterRipple(const std::string& image, const XMFLOAT3& pos);
	void ManageWaterRipples();
	void DrawWaterRipples(GRAPHICSTHREAD threadID);



	// Set any param to -1 if don't want to modify
	void SetShadowProps2D(int resolution, int count, int softShadowQuality);
	// Set any param to -1 if don't want to modify
	void SetShadowPropsCube(int resolution, int count);

	int GetShadowRes2D();
	int GetShadowResCube();



	void SetResolutionScale(float value);
	float GetResolutionScale();
	void SetTransparentShadowsEnabled(float value);
	float GetTransparentShadowsEnabled();
	XMUINT2 GetInternalResolution();
	bool ResolutionChanged();
	void SetGamma(float value);
	float GetGamma();
	void SetWireRender(bool value);
	bool IsWireRender();
	void SetToDrawDebugBoneLines(bool param);
	bool GetToDrawDebugBoneLines();
	void SetToDrawDebugPartitionTree(bool param);
	bool GetToDrawDebugPartitionTree();
	bool GetToDrawDebugEnvProbes();
	void SetToDrawDebugEnvProbes(bool value);
	void SetToDrawDebugEmitters(bool param);
	bool GetToDrawDebugEmitters();
	void SetToDrawDebugForceFields(bool param);
	bool GetToDrawDebugForceFields();
	void SetToDrawDebugCameras(bool param);
	bool GetToDrawDebugCameras();
	bool GetToDrawGridHelper();
	void SetToDrawGridHelper(bool value);
	bool GetToDrawVoxelHelper();
	void SetToDrawVoxelHelper(bool value);
	void SetDebugLightCulling(bool enabled);
	bool GetDebugLightCulling();
	void SetAdvancedLightCulling(bool enabled);
	bool GetAdvancedLightCulling();
	void SetAlphaCompositionEnabled(bool enabled);
	bool GetAlphaCompositionEnabled();
	void SetOcclusionCullingEnabled(bool enabled);
	bool GetOcclusionCullingEnabled();
	void SetLDSSkinningEnabled(bool enabled);
	bool GetLDSSkinningEnabled();
	void SetTemporalAAEnabled(bool enabled);
	bool GetTemporalAAEnabled();
	void SetTemporalAADebugEnabled(bool enabled);
	bool GetTemporalAADebugEnabled();
	void SetFreezeCullingCameraEnabled(bool enabled);
	bool GetFreezeCullingCameraEnabled();
	void SetVoxelRadianceEnabled(bool enabled);
	bool GetVoxelRadianceEnabled();
	void SetVoxelRadianceSecondaryBounceEnabled(bool enabled);
	bool GetVoxelRadianceSecondaryBounceEnabled();
	void SetVoxelRadianceReflectionsEnabled(bool enabled);
	bool GetVoxelRadianceReflectionsEnabled();
	void SetVoxelRadianceVoxelSize(float value);
	float GetVoxelRadianceVoxelSize();
	int GetVoxelRadianceResolution();
	void SetVoxelRadianceNumCones(int value);
	int GetVoxelRadianceNumCones();
	float GetVoxelRadianceRayStepSize();
	void SetVoxelRadianceRayStepSize(float value);
	void SetSpecularAAParam(float value);
	float GetSpecularAAParam();
	void SetAdvancedRefractionsEnabled(bool value);
	bool GetAdvancedRefractionsEnabled();
	bool IsRequestedReflectionRendering();
	void SetEnviromentMap(wiGraphicsTypes::Texture2D* tex);
	wiGraphicsTypes::Texture2D* GetEnviromentMap();
	wiGraphicsTypes::Texture2D* GetLuminance(wiGraphicsTypes::Texture2D* sourceImage, GRAPHICSTHREAD threadID);
	const XMFLOAT4& GetWaterPlane();
	void SetGameSpeed(float value);
	float GetGameSpeed();
	void SetOceanEnabled(bool enabled);
	bool GetOceanEnabled();


	RAY GetPickRay(long cursorX, long cursorY);

	struct RayIntersectWorldResult
	{
		wiECS::Entity entity = wiECS::INVALID_ENTITY;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
		float distance = FLT_MAX;
		int subsetIndex = -1;
	};
	RayIntersectWorldResult RayIntersectWorld(const RAY& ray, UINT renderTypeMask = RENDERTYPE_OPAQUE, uint32_t layerMask = ~0);


	// Add box to render in next frame
	void AddRenderableBox(const XMFLOAT4X4& boxMatrix, const XMFLOAT4& color = XMFLOAT4(1,1,1,1));

	struct RenderableLine
	{
		XMFLOAT3 start = XMFLOAT3(0, 0, 0);
		XMFLOAT3 end = XMFLOAT3(0, 0, 0);
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
	};
	void AddRenderableLine(const RenderableLine& line);

	void AddDeferredMIPGen(wiGraphicsTypes::Texture2D* tex);

	void LoadModel(const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity());

};

