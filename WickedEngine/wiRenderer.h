#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiGraphicsDevice.h"
#include "wiScene.h"
#include "wiECS.h"
#include "wiIntersect.h"

#include <memory>

struct RAY;
struct wiResource;

namespace wiRenderer
{
	inline uint32_t CombineStencilrefs(STENCILREF engineStencilRef, uint8_t userStencilRef)
	{
		return (userStencilRef << 4) | static_cast<uint8_t>(engineStencilRef);
	}

	const wiGraphics::Sampler* GetSampler(int slot);
	const wiGraphics::Shader* GetShader(SHADERTYPE id);
	const wiGraphics::InputLayout* GetInputLayout(ILTYPES id);
	const wiGraphics::RasterizerState* GetRasterizerState(RSTYPES id);
	const wiGraphics::DepthStencilState* GetDepthStencilState(DSSTYPES id);
	const wiGraphics::BlendState* GetBlendState(BSTYPES id);
	const wiGraphics::GPUBuffer* GetConstantBuffer(CBTYPES id);
	const wiGraphics::Texture* GetTexture(TEXTYPES id);

	void ModifySampler(const wiGraphics::SamplerDesc& desc, int slot);


	void Initialize();

	// Clears the scene and the associated renderer resources
	void ClearWorld(wiScene::Scene& scene);

	// Set the main graphics device globally:
	void SetDevice(std::shared_ptr<wiGraphics::GraphicsDevice> newDevice);
	// Retrieve the main graphics device:
	wiGraphics::GraphicsDevice* GetDevice();

	// Returns the shader loading directory
	const std::string& GetShaderPath();
	// Sets the shader loading directory
	void SetShaderPath(const std::string& path);
	// Reload shaders
	void ReloadShaders();

	bool LoadShader(wiGraphics::SHADERSTAGE stage, wiGraphics::Shader& shader, const std::string& filename);

	// Returns the main camera that is currently being used in rendering (and also for post processing)
	wiScene::CameraComponent& GetCamera();
	// Attach camera to entity for the current frame
	void AttachCamera(wiECS::Entity entity);


	struct Visibility
	{
		// User fills these:
		const wiScene::Scene* scene = nullptr;
		const wiScene::CameraComponent* camera = nullptr;
		enum FLAGS
		{
			EMPTY = 0,
			ALLOW_OBJECTS = 1 << 0,
			ALLOW_LIGHTS = 1 << 1,
			ALLOW_DECALS = 1 << 2,
			ALLOW_ENVPROBES = 1 << 3,
			ALLOW_EMITTERS = 1 << 4,
			ALLOW_HAIRS = 1 << 5,
			ALLOW_REQUEST_REFLECTION = 1 << 6,

			ALLOW_EVERYTHING = ~0u
		};
		uint32_t flags = EMPTY;

		// wiRenderer::UpdateVisibility() fills these:
		Frustum frustum;
		std::vector<uint32_t> visibleObjects;
		std::vector<uint32_t> visibleLights;
		std::vector<uint32_t> visibleDecals;
		std::vector<uint32_t> visibleEnvProbes;
		std::vector<uint32_t> visibleEmitters;
		std::vector<uint32_t> visibleHairs;

		std::atomic<uint32_t> object_counter;
		std::atomic<uint32_t> light_counter;
		std::atomic<uint32_t> decal_counter;

		wiSpinLock locker;
		bool planar_reflection_visible = false;
		float closestRefPlane = FLT_MAX;
		XMFLOAT4 reflectionPlane = XMFLOAT4(0, 1, 0, 0);

		void Clear()
		{
			visibleObjects.clear();
			visibleLights.clear();
			visibleDecals.clear();
			visibleEnvProbes.clear();
			visibleEmitters.clear();
			visibleHairs.clear();

			object_counter.store(0);
			light_counter.store(0);
			decal_counter.store(0);

			closestRefPlane = FLT_MAX;
			planar_reflection_visible = false;
		}
	};

	// Performs frustum culling. Specify layerMask to only include specific layers in rendering
	void UpdateVisibility(Visibility& vis, uint32_t layerMask = ~0);
	// Prepares the scene for rendering
	void UpdatePerFrameData(wiScene::Scene& scene, const Visibility& vis, float dt);
	// Updates the GPU state according to the previously called UpdatePerFrameData()
	void UpdateRenderData(const Visibility& vis, wiGraphics::CommandList cmd);
	// Updates all acceleration structures for raytracing API
	void UpdateRaytracingAccelerationStructures(const wiScene::Scene& scene, wiGraphics::CommandList cmd);

	// Binds all common constant buffers and samplers that may be used in all shaders
	void BindCommonResources(wiGraphics::CommandList cmd);
	// Updates the per frame constant buffer (need to call at least once per frame)
	void UpdateFrameCB(const wiScene::Scene& scene, wiGraphics::CommandList cmd);
	// Updates the per camera constant buffer need to call for each different camera that is used when calling DrawScene() and the like
	//	camera_previous : camera from previous frame, used for reprojection effects.
	//	camera_reflection : camera that renders planar reflection
	void UpdateCameraCB(
		const wiScene::CameraComponent& camera,
		const wiScene::CameraComponent& camera_previous,
		const wiScene::CameraComponent& camera_reflection,
		wiGraphics::CommandList cmd
	);


	enum DRAWSCENE_FLAGS
	{
		DRAWSCENE_OPAQUE = 1 << 0,
		DRAWSCENE_TRANSPARENT = 1 << 1,
		DRAWSCENE_OCCLUSIONCULLING = 1 << 2,
		DRAWSCENE_TESSELLATION = 1 << 3,
		DRAWSCENE_HAIRPARTICLE = 1 << 4,
	};

	// Draw the world from a camera. You must call UpdateCameraCB() at least once in this frame prior to this
	void DrawScene(
		const Visibility& vis,
		RENDERPASS renderPass,
		wiGraphics::CommandList cmd,
		uint32_t flags = DRAWSCENE_OPAQUE
	);

	// Render mip levels for textures that reqested it:
	void ProcessDeferredMipGenRequests(wiGraphics::CommandList cmd);

	// Compute essential atmospheric scattering textures for skybox, fog and clouds
	void RenderAtmosphericScatteringTextures(wiGraphics::CommandList cmd);
	// Update atmospheric scattering primarily for environment probes.
	void RefreshAtmosphericScatteringTextures(wiGraphics::CommandList cmd);
	// Draw skydome centered to camera.
	void DrawSky(const wiScene::Scene& scene, wiGraphics::CommandList cmd);
	// A black skydome will be draw with only the sun being visible on it
	void DrawSun(wiGraphics::CommandList cmd);
	// Draw shadow maps for each visible light that has associated shadow maps
	void DrawShadowmaps(
		const Visibility& vis,
		wiGraphics::CommandList cmd,
		uint32_t layerMask = ~0
	);
	// Draw debug world. You must also enable what parts to draw, eg. SetToDrawGridHelper, etc, see implementation for details what can be enabled.
	void DrawDebugWorld(
		const wiScene::Scene& scene,
		const wiScene::CameraComponent& camera,
		wiGraphics::CommandList cmd
	);
	// Draw Soft offscreen particles.
	void DrawSoftParticles(
		const Visibility& vis,
		const wiGraphics::Texture& lineardepth,
		bool distortion, 
		wiGraphics::CommandList cmd
	);
	// Draw simple light visualizer geometries
	void DrawLightVisualizers(
		const Visibility& vis,
		wiGraphics::CommandList cmd
	);
	// Draw volumetric light scattering effects
	void DrawVolumeLights(
		const Visibility& vis,
		const wiGraphics::Texture& depthbuffer,
		wiGraphics::CommandList cmd
	);
	// Draw Lens Flares for lights that have them enabled
	void DrawLensFlares(
		const Visibility& vis,
		const wiGraphics::Texture& depthbuffer,
		wiGraphics::CommandList cmd
	);
	// Call once per frame to re-render out of date environment probes
	void RefreshEnvProbes(const Visibility& vis, wiGraphics::CommandList cmd);
	// Call once per frame to re-render out of date impostors
	void RefreshImpostors(const wiScene::Scene& scene, wiGraphics::CommandList cmd);
	// Call once per frame to repack out of date decals in the atlas
	void RefreshDecalAtlas(const wiScene::Scene& scene, wiGraphics::CommandList cmd);
	// Call once per frame to repack out of date lightmaps in the atlas
	void RefreshLightmapAtlas(const wiScene::Scene& scene, wiGraphics::CommandList cmd);
	// Voxelize the scene into a voxel grid 3D texture
	void VoxelRadiance(const Visibility& vis, wiGraphics::CommandList cmd);
	// Compute light grid tiles
	void ComputeTiledLightCulling(
		const wiScene::CameraComponent& camera,
		const wiGraphics::Texture& depthbuffer,
		wiGraphics::CommandList cmd
	);
	// Run a compute shader that will resolve a MSAA depth buffer to a single-sample texture
	void ResolveMSAADepthBuffer(const wiGraphics::Texture& dst, const wiGraphics::Texture& src, wiGraphics::CommandList cmd);
	void DownsampleDepthBuffer(const wiGraphics::Texture& src, wiGraphics::CommandList cmd);
	// Compute the luminance for the source image and return the texture containing the luminance value in pixel [0,0]
	const wiGraphics::Texture* ComputeLuminance(const wiGraphics::Texture& sourceImage, wiGraphics::CommandList cmd);

	void ComputeShadingRateClassification(
		const wiGraphics::Texture gbuffer[GBUFFER_COUNT],
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd
	);

	void DeferredComposition(
		const wiGraphics::Texture gbuffer[GBUFFER_COUNT],
		const wiGraphics::Texture& depth,
		wiGraphics::CommandList cmd
	);

	void Postprocess_Blur_Gaussian(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& temp,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		int mip_src = -1,
		int mip_dst = -1,
		bool wide = false
	);
	void Postprocess_Blur_Bilateral(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& temp,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float depth_threshold = 1.0f,
		int mip_src = -1,
		int mip_dst = -1,
		bool wide = false
	);
	void Postprocess_SSAO(
		const wiGraphics::Texture& depthbuffer,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float range = 1.0f,
		uint32_t samplecount = 16,
		float power = 2.0f
	);
	void Postprocess_HBAO(
		const wiScene::CameraComponent& camera,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float power = 2.0f
		);
	void Postprocess_MSAO(
		const wiScene::CameraComponent& camera,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float power = 2.0f
		);
	void Postprocess_RTAO(
		const wiScene::Scene& scene,
		const wiGraphics::Texture& depthbuffer,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& depth_history,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float range = 1.0f,
		uint32_t samplecount = 16,
		float power = 2.0f
	);
	void Postprocess_RTReflection(
		const wiScene::Scene& scene,
		const wiGraphics::Texture& depthbuffer,
		const wiGraphics::Texture gbuffer[GBUFFER_COUNT],
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float range = 1000.0f
	);
	void Postprocess_SSR(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& depthbuffer,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture gbuffer[GBUFFER_COUNT],
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_SSS(
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture gbuffer[GBUFFER_COUNT],
		const wiGraphics::RenderPass& input_output_lightbuffer_diffuse,
		const wiGraphics::RenderPass& input_output_temp1,
		const wiGraphics::RenderPass& input_output_temp2,
		wiGraphics::CommandList cmd,
		float amount = 1.0f
	);
	void Postprocess_LightShafts(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		const XMFLOAT2& center
	);
	void Postprocess_DepthOfField(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& output,
		const wiGraphics::Texture& lineardepth,
		wiGraphics::CommandList cmd,
		float focus = 10.0f,
		float scale = 1.0f,
		float aspect = 1.0f,
		float max_coc = 18.0f
	);
	void Postprocess_Outline(
		const wiGraphics::Texture& input,
		wiGraphics::CommandList cmd,
		float threshold = 0.1f,
		float thickness = 1.0f,
		const XMFLOAT4& color = XMFLOAT4(0, 0, 0, 1)
	);
	void Postprocess_MotionBlur(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& velocity,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float strength = 100.0f
	);
	void Postprocess_Bloom(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& bloom,
		const wiGraphics::Texture& bloom_tmp,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float threshold = 1.0f
	);
	void Postprocess_VolumetricClouds(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& output,
		const wiGraphics::Texture& lightshaftoutput,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& depthbuffer,
		wiGraphics::CommandList cmd
	);
	void Postprocess_FXAA(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_TemporalAA(
		const wiGraphics::Texture& input_current,
		const wiGraphics::Texture& input_history,
		const wiGraphics::Texture& velocity,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& depth_history,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_Lineardepth(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_Sharpen(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float amount = 1.0f
	);
	void Postprocess_Tonemap(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& input_luminance,
		const wiGraphics::Texture& input_distortion,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float exposure,
		bool dither,
		const wiGraphics::Texture* colorgrade_lookuptable
	);
	void Postprocess_Chromatic_Aberration(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float amount = 1.0f
	);
	void Postprocess_Upsample_Bilateral(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		bool is_pixelshader = false,
		float threshold = 1.0f
	);
	void Postprocess_Downsample4x(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_NormalsFromDepth(
		const wiGraphics::Texture& depthbuffer,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_Denoise(
		const wiGraphics::Texture& input_output_current,
		const wiGraphics::Texture& temporal_history,
		const wiGraphics::Texture& temporal_current,
		const wiGraphics::Texture& velocity,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& depth_history,
		wiGraphics::CommandList cmd
	);

	// Build the scene BVH on GPU that can be used by ray traced rendering
	void BuildSceneBVH(const wiScene::Scene& scene, wiGraphics::CommandList cmd);

	struct RayBuffers
	{
		uint32_t rayCapacity = 0;
		wiGraphics::GPUBuffer rayBuffer[2];
		wiGraphics::GPUBuffer rayIndexBuffer[2];
		wiGraphics::GPUBuffer rayCountBuffer[2];
		wiGraphics::GPUBuffer raySortBuffer;
		void Create(uint32_t newRayCapacity);
	};
	// Generate rays for every pixel of the internal resolution
	RayBuffers* GenerateScreenRayBuffers(const wiScene::CameraComponent& camera, wiGraphics::CommandList cmd);
	// Render the scene with ray tracing. You provide the ray buffer, where each ray maps to one pixel of the result testure
	void RayTraceScene(
		const wiScene::Scene& scene,
		const RayBuffers* rayBuffers,
		const wiGraphics::Texture* result,
		int accumulation_sample,
		wiGraphics::CommandList cmd
	);
	// Render the scene BVH with ray tracing to the screen
	void RayTraceSceneBVH(wiGraphics::CommandList cmd);

	// Render occluders against a depth buffer
	void OcclusionCulling_Render(const wiScene::CameraComponent& camera_previous, const Visibility& vis, wiGraphics::CommandList cmd);
	// Read the occlusion culling results of the previous call to OcclusionCulling_Render. This must be done on the main thread!
	void OcclusionCulling_Read(wiScene::Scene& scene, const Visibility& vis);
	// Issue end-of frame operations
	void EndFrame();


	enum MIPGENFILTER
	{
		MIPGENFILTER_POINT,
		MIPGENFILTER_LINEAR,
		MIPGENFILTER_GAUSSIAN,
	};
	struct MIPGEN_OPTIONS
	{
		int arrayIndex = -1;
		const wiGraphics::Texture* gaussian_temp = nullptr;
		bool preserve_coverage = false;
		bool wide_gauss = false;
	};
	void GenerateMipChain(const wiGraphics::Texture& texture, MIPGENFILTER filter, wiGraphics::CommandList cmd, const MIPGEN_OPTIONS& options = {});

	enum BORDEREXPANDSTYLE
	{
		BORDEREXPAND_DISABLE,
		BORDEREXPAND_WRAP,
		BORDEREXPAND_CLAMP,
	};
	// Performs copy operation even between different texture formats
	//	Can also expand border region according to desired sampler func
	void CopyTexture2D(
		const wiGraphics::Texture& dst, uint32_t DstMIP, uint32_t DstX, uint32_t DstY, 
		const wiGraphics::Texture& src, uint32_t SrcMIP, 
		wiGraphics::CommandList cmd,
		BORDEREXPANDSTYLE borderExpand = BORDEREXPAND_DISABLE);

	// Assign texture slots to out of date environemnt probes
	void ManageEnvProbes(wiScene::Scene& scene);
	// Invalidate out of date impostors
	void ManageImpostors(wiScene::Scene& scene);
	// New decals will be packed into a texture atlas
	void ManageDecalAtlas(wiScene::Scene& scene);
	// New lightmapped objects will be packed into global lightmap atlas
	void ManageLightmapAtlas(wiScene::Scene& scene);

	void PutWaterRipple(const std::string& image, const XMFLOAT3& pos);
	void ManageWaterRipples();
	void DrawWaterRipples(wiGraphics::CommandList cmd);



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
	void SetVariableRateShadingClassification(bool enabled);
	bool GetVariableRateShadingClassification();
	void SetVariableRateShadingClassificationDebug(bool enabled);
	bool GetVariableRateShadingClassificationDebug();
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
	void SetVoxelRadianceMaxDistance(float value);
	float GetVoxelRadianceMaxDistance();
	int GetVoxelRadianceResolution();
	void SetVoxelRadianceNumCones(int value);
	int GetVoxelRadianceNumCones();
	float GetVoxelRadianceRayStepSize();
	void SetVoxelRadianceRayStepSize(float value);
	bool IsRequestedVolumetricLightRendering();
	void SetGameSpeed(float value);
	float GetGameSpeed();
	void OceanRegenerate(const wiScene::WeatherComponent& weather); // regeenrates ocean if it is already created
	void InvalidateBVH(); // invalidates scene bvh so if something wants to use it, it will recompute and validate it
	void SetRaytraceBounceCount(uint32_t bounces);
	uint32_t GetRaytraceBounceCount();
	void SetRaytraceDebugBVHVisualizerEnabled(bool value);
	bool GetRaytraceDebugBVHVisualizerEnabled();
	void SetRaytracedShadowsEnabled(bool value);
	bool GetRaytracedShadowsEnabled();
	void SetTessellationEnabled(bool value);
	bool GetTessellationEnabled();

	const wiGraphics::Texture* GetGlobalLightmap();

	// Gets pick ray according to the current screen resolution and pointer coordinates. Can be used as input into RayIntersectWorld()
	RAY GetPickRay(long cursorX, long cursorY, const wiScene::CameraComponent& camera = GetCamera());


	// Add box to render in next frame. It will be rendered in DrawDebugWorld()
	void DrawBox(const XMFLOAT4X4& boxMatrix, const XMFLOAT4& color = XMFLOAT4(1,1,1,1));
	// Add sphere to render in next frame. It will be rendered in DrawDebugWorld()
	void DrawSphere(const SPHERE& sphere, const XMFLOAT4& color = XMFLOAT4(1, 1, 1, 1));
	// Add capsule to render in next frame. It will be rendered in DrawDebugWorld()
	void DrawCapsule(const CAPSULE& capsule, const XMFLOAT4& color = XMFLOAT4(1, 1, 1, 1));

	struct RenderableLine
	{
		XMFLOAT3 start = XMFLOAT3(0, 0, 0);
		XMFLOAT3 end = XMFLOAT3(0, 0, 0);
		XMFLOAT4 color_start = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 color_end = XMFLOAT4(1, 1, 1, 1);
	};
	// Add line to render in the next frame. It will be rendered in DrawDebugWorld()
	void DrawLine(const RenderableLine& line);

	struct RenderableLine2D
	{
		XMFLOAT2 start = XMFLOAT2(0, 0);
		XMFLOAT2 end = XMFLOAT2(0, 0);
		XMFLOAT4 color_start = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT4 color_end = XMFLOAT4(1, 1, 1, 1);
	};
	// Add 2D line to render in the next frame. It will be rendered in DrawDebugWorld() in screen space
	void DrawLine(const RenderableLine2D& line);

	struct RenderablePoint
	{
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		float size = 1.0f;
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
	};
	void DrawPoint(const RenderablePoint& point);

	struct RenderableTriangle
	{
		XMFLOAT3 positionA = XMFLOAT3(0, 0, 0);
		XMFLOAT4 colorA = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT3 positionB = XMFLOAT3(0, 0, 0);
		XMFLOAT4 colorB = XMFLOAT4(1, 1, 1, 1);
		XMFLOAT3 positionC = XMFLOAT3(0, 0, 0);
		XMFLOAT4 colorC = XMFLOAT4(1, 1, 1, 1);
	};
	void DrawTriangle(const RenderableTriangle& triangle, bool wireframe = false);

	struct PaintRadius
	{
		wiECS::Entity objectEntity = wiECS::INVALID_ENTITY;
		int subset = -1;
		uint32_t uvset = 0;
		float radius = 0;
		XMUINT2 center;
		XMUINT2 dimensions;
	};
	void DrawPaintRadius(const PaintRadius& paintrad);

	// Add a texture that should be mipmapped whenever it is feasible to do so
	void AddDeferredMIPGen(std::shared_ptr<wiResource> res, bool preserve_coverage = false);
	void AddDeferredMaterialUpdate(size_t index);
	void AddDeferredMorphUpdate(size_t index);

	struct CustomShader
	{
		std::string name;
		uint32_t renderTypeFlags = RENDERTYPE_OPAQUE;
		wiGraphics::PipelineState pso[RENDERPASS_COUNT] = {};
	};
	// Registers a custom shader that can be set to materials. 
	//	Returns the ID of the custom shader that can be used with MaterialComponent::SetCustomShaderID()
	int RegisterCustomShader(const CustomShader& customShader);
	const std::vector<CustomShader>& GetCustomShaders();

	// Helper utility to manage async GPU query readback from the CPU
	//	GPUQueryRing<latency> here latency specifies the ring size of queries and 
	//	directly correlates with the frame latency between Issue (GPU) <-> Read back (CPU)
	template<int latency>
	struct GPUQueryRing
	{
		wiGraphics::GPUQuery queries[latency];
		int id = 0;
		bool active[latency] = {};

		// Creates a number of queries in the async ring
		void Create(wiGraphics::GraphicsDevice* device, const wiGraphics::GPUQueryDesc* desc)
		{
			for (int i = 0; i < latency; ++i)
			{
				device->CreateQuery(desc, &queries[i]);
				active[i] = false;
				id = 0;
			}
		}

		// Returns the current query suitable for GPU execution and marks it as active
		//	Use this with GraphicsDevice::QueryBegin() and GraphicsDevice::QueryEnd()
		inline wiGraphics::GPUQuery* Get_GPU()
		{
			active[id] = true;
			return &queries[id];
		}

		// Returns the current query suitable for CPU readback and marks it as inactive
		//	It will return nullptr if none of the queries are suitable for readback yet
		//	Use this with GraphicsDevice::QueryRead(). Only call once per frame per QueryRing instance!
		inline wiGraphics::GPUQuery* Get_CPU()
		{
			id = (id + 1) % latency;
			if (!active[id])
			{
				return nullptr;
			}
			active[id] = false;
			return &queries[id];
		}
	};

};

