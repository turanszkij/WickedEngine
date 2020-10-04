#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
#include "wiGraphicsDevice.h"
#include "wiScene_Decl.h"
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
	const wiGraphics::Shader* GetVertexShader(VSTYPES id);
	const wiGraphics::Shader* GetHullShader(HSTYPES id);
	const wiGraphics::Shader* GetDomainShader(DSTYPES id);
	const wiGraphics::Shader* GetGeometryShader(GSTYPES id);
	const wiGraphics::Shader* GetPixelShader(PSTYPES id);
	const wiGraphics::Shader* GetComputeShader(CSTYPES id);
	const wiGraphics::InputLayout* GetInputLayout(ILTYPES id);
	const wiGraphics::RasterizerState* GetRasterizerState(RSTYPES id);
	const wiGraphics::DepthStencilState* GetDepthStencilState(DSSTYPES id);
	const wiGraphics::BlendState* GetBlendState(BSTYPES id);
	const wiGraphics::GPUBuffer* GetConstantBuffer(CBTYPES id);
	const wiGraphics::Texture* GetTexture(TEXTYPES id);

	void ModifySampler(const wiGraphics::SamplerDesc& desc, int slot);


	void Initialize();

	// Clears the global scene and the associated renderable resources
	void ClearWorld();

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
	// Returns the previous frame's camera that is currently being used in rendering to reproject
	wiScene::CameraComponent& GetPrevCamera();
	// Returns the planar reflection camera that is currently being used in rendering
	wiScene::CameraComponent& GetRefCamera();
	// Attach camera to entity for the current frame
	void AttachCamera(wiECS::Entity entity);

	// Updates the main scene, performs frustum culling for main camera and other tasks that are only done once per frame. Specify layerMask to only include specific entities in the render frame.
	void UpdatePerFrameData(float dt, uint32_t layerMask = ~0);
	// Updates the GPU state according to the previously called UpdatePerFrameData()
	void UpdateRenderData(wiGraphics::CommandList cmd);
	// Updates all acceleration structures for raytracing API
	void UpdateRaytracingAccelerationStructures(wiGraphics::CommandList cmd);

	// Binds all common constant buffers and samplers that may be used in all shaders
	void BindCommonResources(wiGraphics::CommandList cmd);
	// Updates the per frame constant buffer (need to call at least once per frame)
	void UpdateFrameCB(wiGraphics::CommandList cmd);
	// Updates the per camera constant buffer need to call for each different camera that is used when calling DrawScene() and the like
	void UpdateCameraCB(const wiScene::CameraComponent& camera, wiGraphics::CommandList cmd);
	// Set a global clipping plane state that is available to use in any shader (access as float4 g_xClipPlane)
	void SetClipPlane(const XMFLOAT4& clipPlane, wiGraphics::CommandList cmd);
	// Set a global alpha reference value that can be used by any shaders to perform alpha-testing (access as float g_xAlphaRef)
	void SetAlphaRef(float alphaRef, wiGraphics::CommandList cmd);
	// Resets the global shader alpha reference value to float g_xAlphaRef = 0.75f
	inline void ResetAlphaRef(wiGraphics::CommandList cmd) { SetAlphaRef(0.75f, cmd); }

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
		const wiScene::CameraComponent& camera,
		RENDERPASS renderPass,
		wiGraphics::CommandList cmd,
		uint32_t flags = DRAWSCENE_OPAQUE
	);

	// Draw skydome centered to camera.
	void DrawSky(wiGraphics::CommandList cmd);
	// A black skydome will be draw with only the sun being visible on it
	void DrawSun(wiGraphics::CommandList cmd);
	// Draw shadow maps for each visible light that has associated shadow maps
	void DrawShadowmaps(const wiScene::CameraComponent& camera, wiGraphics::CommandList cmd, uint32_t layerMask = ~0);
	// Draw debug world. You must also enable what parts to draw, eg. SetToDrawGridHelper, etc, see implementation for details what can be enabled.
	void DrawDebugWorld(const wiScene::CameraComponent& camera, wiGraphics::CommandList cmd);
	// Draw Soft offscreen particles. Linear depth should be already readable (see BindDepthTextures())
	void DrawSoftParticles(
		const wiScene::CameraComponent& camera, 
		const wiGraphics::Texture& lineardepth,
		bool distortion, 
		wiGraphics::CommandList cmd
	);
	// Draw deferred lights. Gbuffer and depth textures should already be readable (see BindGBufferTextures(), BindDepthTextures())
	void DrawDeferredLights(
		const wiScene::CameraComponent& camera,
		const wiGraphics::Texture& depthbuffer,
		const wiGraphics::Texture& gbuffer0,
		const wiGraphics::Texture& gbuffer1,
		const wiGraphics::Texture& gbuffer2,
		wiGraphics::CommandList cmd
	);
	// Draw simple light visualizer geometries
	void DrawLightVisualizers(
		const wiScene::CameraComponent& camera, 
		wiGraphics::CommandList cmd
	);
	// Draw volumetric light scattering effects
	void DrawVolumeLights(
		const wiScene::CameraComponent& camera,
		const wiGraphics::Texture& depthbuffer,
		wiGraphics::CommandList cmd
	);
	// Draw Lens Flares for lights that have them enabled
	void DrawLensFlares(
		const wiScene::CameraComponent& camera,
		const wiGraphics::Texture& depthbuffer,
		wiGraphics::CommandList cmd
	);
	// Draw deferred decals
	void DrawDeferredDecals(
		const wiScene::CameraComponent& camera,
		const wiGraphics::Texture& depthbuffer,
		wiGraphics::CommandList cmd
	);
	// Call once per frame to re-render out of date environment probes
	void RefreshEnvProbes(wiGraphics::CommandList cmd);
	// Call once per frame to re-render out of date impostors
	void RefreshImpostors(wiGraphics::CommandList cmd);
	// Call once per frame to repack out of date decals in the atlas
	void RefreshDecalAtlas(wiGraphics::CommandList cmd);
	// Call once per frame to repack out of date lightmaps in the atlas
	void RefreshLightmapAtlas(wiGraphics::CommandList cmd);
	// Voxelize the scene into a voxel grid 3D texture
	void VoxelRadiance(wiGraphics::CommandList cmd);
	// Compute light grid tiles for tiled rendering paths
	//	If you specify lightbuffers (diffuse, specular), then a tiled deferred lighting will be computed as well. Otherwise, only the light grid gets computed
	void ComputeTiledLightCulling(
		const wiGraphics::Texture& depthbuffer,
		wiGraphics::CommandList cmd, 
		const wiGraphics::Texture* gbuffer0 = nullptr,
		const wiGraphics::Texture* gbuffer1 = nullptr,
		const wiGraphics::Texture* gbuffer2 = nullptr,
		const wiGraphics::Texture* lightbuffer_diffuse = nullptr,
		const wiGraphics::Texture* lightbuffer_specular = nullptr
	);
	// Run a compute shader that will resolve a MSAA depth buffer to a single-sample texture
	void ResolveMSAADepthBuffer(const wiGraphics::Texture& dst, const wiGraphics::Texture& src, wiGraphics::CommandList cmd);
	void DownsampleDepthBuffer(const wiGraphics::Texture& src, wiGraphics::CommandList cmd);
	// Compute the luminance for the source image and return the texture containing the luminance value in pixel [0,0]
	const wiGraphics::Texture* ComputeLuminance(const wiGraphics::Texture& sourceImage, wiGraphics::CommandList cmd);

	void ComputeShadingRateClassification(
		const wiGraphics::Texture& gbuffer1,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd
	);

	void DeferredComposition(
		const wiGraphics::Texture& gbuffer0,
		const wiGraphics::Texture& gbuffer1,
		const wiGraphics::Texture& gbuffer2,
		const wiGraphics::Texture& lightmap_diffuse,
		const wiGraphics::Texture& lightmap_specular,
		const wiGraphics::Texture& ao,
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
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float power = 2.0f
		);
	void Postprocess_MSAO(
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float power = 2.0f
		);
	void Postprocess_RTAO(
		const wiGraphics::Texture& depthbuffer,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float range = 1.0f,
		uint32_t samplecount = 16,
		float power = 2.0f
	);
	void Postprocess_RTReflection(
		const wiGraphics::Texture& depthbuffer,
		const wiGraphics::Texture& gbuffer1,
		const wiGraphics::Texture& gbuffer2,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd,
		float range = 1000.0f
	);
	void Postprocess_SSR(
		const wiGraphics::Texture& input,
		const wiGraphics::Texture& depthbuffer,
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& gbuffer1,
		const wiGraphics::Texture& gbuffer2,
		const wiGraphics::Texture& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_SSS(
		const wiGraphics::Texture& lineardepth,
		const wiGraphics::Texture& gbuffer0,
		const wiGraphics::RenderPass& input_output_lightbuffer_diffuse,
		const wiGraphics::RenderPass& input_output_temp1,
		const wiGraphics::RenderPass& input_output_temp2,
		wiGraphics::CommandList cmd
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

	// Build the scene BVH on GPU that can be used by ray traced rendering
	void BuildSceneBVH(wiGraphics::CommandList cmd);

	struct RayBuffers
	{
		uint32_t rayCapacity = 0;
		wiGraphics::GPUBuffer rayBuffer[2];
		wiGraphics::GPUBuffer rayIndexBuffer[2];
		wiGraphics::GPUBuffer rayCountBuffer[2];
		wiGraphics::GPUBuffer raySortBuffer;
		void Create(wiGraphics::GraphicsDevice* device, uint32_t newRayCapacity);
	};
	// Generate rays for every pixel of the internal resolution
	RayBuffers* GenerateScreenRayBuffers(const wiScene::CameraComponent& camera, wiGraphics::CommandList cmd);
	// Render the scene with ray tracing. You provide the ray buffer, where each ray maps to one pixel of the result testure
	void RayTraceScene(
		const RayBuffers* rayBuffers,
		const wiGraphics::Texture* result,
		int accumulation_sample,
		wiGraphics::CommandList cmd
	);
	// Render the scene BVH with ray tracing to the screen
	void RayTraceSceneBVH(wiGraphics::CommandList cmd);

	// Render occluders against a depth buffer
	void OcclusionCulling_Render(wiGraphics::CommandList cmd);
	// Read the occlusion culling results of the previous call to OcclusionCulling_Render. This must be done on the main thread!
	void OcclusionCulling_Read();
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
	void ManageEnvProbes();
	// Invalidate out of date impostors
	void ManageImpostors();
	// New decals will be packed into a texture atlas
	void ManageDecalAtlas();
	// New lightmapped objects will be packed into global lightmap atlas
	void ManageLightmapAtlas();

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
	void SetVoxelRadianceMaxDistance(float value);
	float GetVoxelRadianceMaxDistance();
	int GetVoxelRadianceResolution();
	void SetVoxelRadianceNumCones(int value);
	int GetVoxelRadianceNumCones();
	float GetVoxelRadianceRayStepSize();
	void SetVoxelRadianceRayStepSize(float value);
	bool IsRequestedReflectionRendering();
	bool IsRequestedVolumetricLightRendering();
	const XMFLOAT4& GetWaterPlane();
	void SetGameSpeed(float value);
	float GetGameSpeed();
	void OceanRegenerate(); // regeenrates ocean if it is already created
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
	RAY GetPickRay(long cursorX, long cursorY);


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

	struct CustomShader
	{
		std::string name;

		struct Pass
		{
			uint32_t renderTypeFlags = RENDERTYPE_TRANSPARENT;
			wiGraphics::PipelineState* pso = nullptr;
		};
		Pass passes[RENDERPASS_COUNT] = {};
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

