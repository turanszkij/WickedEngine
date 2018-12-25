#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiGraphicsDevice.h"
#include "wiSceneSystem_Decl.h"
#include "wiECS.h"

struct RAY;

namespace wiRenderer
{
	// Common render target formats:
	static const wiGraphicsTypes::FORMAT RTFormat_ldr = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_hdr = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_0 = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_1 = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_2 = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_deferred_lightbuffer = wiGraphicsTypes::FORMAT_R11G11B10_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_lineardepth = wiGraphicsTypes::FORMAT_R16_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_ssao = wiGraphicsTypes::FORMAT_R8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_waterripple = wiGraphicsTypes::FORMAT_R8G8B8A8_SNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_normalmaps = wiGraphicsTypes::FORMAT_R8G8B8A8_SNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_depthresolve = wiGraphicsTypes::FORMAT_R32_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_voxelradiance = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_envprobe = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_impostor = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_lightmap_object = wiGraphicsTypes::FORMAT_R32G32B32A32_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_lightmap_global = wiGraphicsTypes::FORMAT_R11G11B10_FLOAT;

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

	// Set the main graphics device globally:
	void SetDevice(wiGraphicsTypes::GraphicsDevice* newDevice);
	// Retrieve the main graphics device:
	wiGraphicsTypes::GraphicsDevice* GetDevice();

	// Returns the shader path that you can also modify
	std::string& GetShaderPath();
	// Reload shaders, use the argument to modify the shader path. If the argument is empty, the shader path will not be modified
	void ReloadShaders(const std::string& path = "");

	// Returns the main scene which is currently being used in rendering
	wiSceneSystem::Scene& GetScene();
	// Returns the main camera that is currently being used in rendering (and also for post processing)
	wiSceneSystem::CameraComponent& GetCamera();
	// Returns the previous frame's camera that is currently being used in rendering to reproject
	wiSceneSystem::CameraComponent& GetPrevCamera();
	// Returns the planar reflection camera that is currently being used in rendering
	wiSceneSystem::CameraComponent& GetRefCamera();

	// Updates the main scene, performs frustum culling for main camera and other tasks that are only done once per frame
	void UpdatePerFrameData(float dt);
	// Updates the GPU state according to the previously called UpatePerFrameData()
	void UpdateRenderData(GRAPHICSTHREAD threadID);

	// Binds all persistent constant buffers, samplers that can used globally in all shaders for a whole frame
	void BindPersistentState(GRAPHICSTHREAD threadID);
	// Updates the per frame constant buffer (need to call at least once per frame)
	void UpdateFrameCB(GRAPHICSTHREAD threadID);
	// Updates the per camera constant buffer need to call for each different camera that is used when calling DrawScene() and the like
	void UpdateCameraCB(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	// Set a global clipping plane state that is available to use in any shader (access as float4 g_xClipPlane)
	void SetClipPlane(const XMFLOAT4& clipPlane, GRAPHICSTHREAD threadID);
	// Set a global alpha reference value that can be used by any shaders to perform alpha-testing (access as float g_xAlphaRef)
	void SetAlphaRef(float alphaRef, GRAPHICSTHREAD threadID);
	// Resets the global shader alpha reference value to float g_xAlphaRef = 0.75f
	inline void ResetAlphaRef(GRAPHICSTHREAD threadID) { SetAlphaRef(0.75f, threadID); }
	// Binds the gbuffer textures that will be used in deferred rendering, or thin gbuffer in case of forward rendering
	void BindGBufferTextures(wiGraphicsTypes::Texture2D* slot0, wiGraphicsTypes::Texture2D* slot1, wiGraphicsTypes::Texture2D* slot2, GRAPHICSTHREAD threadID);
	// Binds the hardware depth buffer and a linear depth buffer to be read by shaders
	void BindDepthTextures(wiGraphicsTypes::Texture2D* depth, wiGraphicsTypes::Texture2D* linearDepth, GRAPHICSTHREAD threadID);

	// Draw skydome centered to camera. Its color will be either dynamically computed, or the global environment map will be used if you called SetEnvironmentMap()
	void DrawSky(GRAPHICSTHREAD threadID);
	// A black skydome will be draw with only the sun being visible on it
	void DrawSun(GRAPHICSTHREAD threadID);
	// Draw the world from a camera. You must call UpdateCameraCB() at least once in this frame prior to this
	void DrawScene(const wiSceneSystem::CameraComponent& camera, bool tessellation, GRAPHICSTHREAD threadID, SHADERTYPE shaderType, bool grass, bool occlusionCulling, uint32_t layerMask = 0xFFFFFFFF);
	// Draw the transparent world from a camera. You must call UpdateCameraCB() at least once in this frame prior to this
	void DrawScene_Transparent(const wiSceneSystem::CameraComponent& camera, SHADERTYPE shaderType, GRAPHICSTHREAD threadID, bool grass, bool occlusionCulling, uint32_t layerMask = 0xFFFFFFFF);
	// Draw shadow maps for each visible light that has associated shadow maps
	void DrawForShadowMap(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID, uint32_t layerMask = 0xFFFFFFFF);
	// Draw debug world. You must also enable what parts to draw, eg. SetToDrawGridHelper, etc, see implementation for details what can be enabled.
	void DrawDebugWorld(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	// Draw Soft offscreen particles. Linear depth should be already readable (see BindDepthTextures())
	void DrawSoftParticles(const wiSceneSystem::CameraComponent& camera, bool distortion, GRAPHICSTHREAD threadID);
	// Draw deferred lights. Gbuffer and depth textures should already be readable (see BindGBufferTextures(), BindDepthTextures())
	void DrawLights(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	// Draw simple light visualizer geometries
	void DrawLightVisualizers(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	// Draw volumetric light scattering effects. Linear depth should be already readable (see BindDepthTextures())
	void DrawVolumeLights(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	// Draw Lens Flares for lights that have them enabled. Linear depth should be already readable (see BindDepthTextures())
	void DrawLensFlares(GRAPHICSTHREAD threadID);
	// Draw deferred decals. Gbuffer and depth textures should already be readable (see BindGBufferTextures(), BindDepthTextures())
	void DrawDecals(const wiSceneSystem::CameraComponent& camera, GRAPHICSTHREAD threadID);
	// Call once per frame to re-render out of date environment probes
	void RefreshEnvProbes(GRAPHICSTHREAD threadID);
	// Call once per frame to re-render out of date impostors
	void RefreshImpostors(GRAPHICSTHREAD threadID);
	// Voxelize the scene into a voxel grid 3D texture
	void VoxelRadiance(GRAPHICSTHREAD threadID);
	// Compute light grid tiles for tiled rendering paths
	//	If you specify lightbuffers (diffuse, specular), then a tiled deferred lighting will be computed as well. Otherwise, only the light grid gets computed
	void ComputeTiledLightCulling(GRAPHICSTHREAD threadID, wiGraphicsTypes::Texture2D* lightbuffer_diffuse = nullptr, wiGraphicsTypes::Texture2D* lightbuffer_specular = nullptr);
	// Run a compute shader that will resolve a MSAA depth buffer to a single-sample texture
	void ResolveMSAADepthBuffer(wiGraphicsTypes::Texture2D* dst, wiGraphicsTypes::Texture2D* src, GRAPHICSTHREAD threadID);
	// Build the scene BVH on GPU that can be used by ray traced rendering
	void BuildSceneBVH(GRAPHICSTHREAD threadID);
	// Render the scene with ray tracing only
	void DrawTracedScene(const wiSceneSystem::CameraComponent& camera, wiGraphicsTypes::Texture2D* result, GRAPHICSTHREAD threadID);

	// Render occluders against a depth buffer
	void OcclusionCulling_Render(GRAPHICSTHREAD threadID);
	// Read the occlusion culling results of the previous call to OcclusionCulling_Render. This must be done on the main thread!
	void OcclusionCulling_Read();
	// Issue end-of frame operations
	void EndFrame();


	enum MIPGENFILTER
	{
		MIPGENFILTER_POINT,
		MIPGENFILTER_LINEAR,
		MIPGENFILTER_LINEAR_MAXIMUM,
		MIPGENFILTER_GAUSSIAN,
		MIPGENFILTER_BICUBIC,
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

	// New decals will be packed into a texture atlas
	void ManageDecalAtlas(GRAPHICSTHREAD threadID);
	// New lightmapped objects will be packed into global lightmap atlas
	void ManageLightmapAtlas(GRAPHICSTHREAD threadID);

	void PutWaterRipple(const std::string& image, const XMFLOAT3& pos);
	void ManageWaterRipples();
	void DrawWaterRipples(GRAPHICSTHREAD threadID);



	// Set any param to -1 if don't want to modify
	void SetShadowProps2D(int resolution, int count, int softShadowQuality);
	// Set any param to -1 if don't want to modify
	void SetShadowPropsCube(int resolution, int count);

	// Returns the resolution that is used for all spotlight and directional light shadow maps
	int GetShadowRes2D();
	// Returns the resolution that is used for all pointlight and area light shadow maps
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
	void SetEnvironmentMap(wiGraphicsTypes::Texture2D* tex);
	wiGraphicsTypes::Texture2D* GetEnvironmentMap();
	wiGraphicsTypes::Texture2D* GetLuminance(wiGraphicsTypes::Texture2D* sourceImage, GRAPHICSTHREAD threadID);
	const XMFLOAT4& GetWaterPlane();
	void SetGameSpeed(float value);
	float GetGameSpeed();
	void SetOceanEnabled(bool enabled);
	bool GetOceanEnabled();
	void InvalidateBVH(); // invalidates scene bvh so if something wants to use it, it will recompute and validate it
	void SetLightmapBakeBounceCount(uint32_t bounces);
	uint32_t GetLightmapBakeBounceCount();

	wiGraphicsTypes::Texture2D* GetGlobalLightmap();

	// Gets pick ray according to the current screen resolution and pointer coordinates. Can be used as input into RayIntersectWorld()
	RAY GetPickRay(long cursorX, long cursorY);

	struct RayIntersectWorldResult
	{
		wiECS::Entity entity = wiECS::INVALID_ENTITY;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
		float distance = FLT_MAX;
		int subsetIndex = -1;
		int vertexID0 = -1;
		int vertexID1 = -1;
		int vertexID2 = -1;
		XMFLOAT4X4 orientation = IDENTITYMATRIX;

		bool operator==(const RayIntersectWorldResult& other)
		{
			return entity == other.entity;
		}
	};
	// Given a ray, finds the closest intersection point against all instances
	RayIntersectWorldResult RayIntersectWorld(const RAY& ray, UINT renderTypeMask = RENDERTYPE_OPAQUE, uint32_t layerMask = ~0);


	// Add box to render in next frame. It will be rendered in DrawDebugWorld()
	void AddRenderableBox(const XMFLOAT4X4& boxMatrix, const XMFLOAT4& color = XMFLOAT4(1,1,1,1));

	struct RenderableLine
	{
		XMFLOAT3 start = XMFLOAT3(0, 0, 0);
		XMFLOAT3 end = XMFLOAT3(0, 0, 0);
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
	};
	// Add line to render in the next frame. It will be rendered in DrawDebugWorld()
	void AddRenderableLine(const RenderableLine& line);

	struct RenderablePoint
	{
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		float size = 1.0f;
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
	};
	void AddRenderablePoint(const RenderablePoint& point);

	// Add a texture that should be mipmapped whenever it is feasible to do so
	void AddDeferredMIPGen(wiGraphicsTypes::Texture2D* tex);

	// Helper function to open a wiscene file and add the contents to the current scene
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	attached		:	everything will be attached to a base entity
	//
	//	returns INVALID_ENTITY if attached argument was false, else it returns the base entity handle
	wiECS::Entity LoadModel(const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), bool attached = false);

};

