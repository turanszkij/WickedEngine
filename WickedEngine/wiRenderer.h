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
	static const wiGraphics::FORMAT RTFormat_ldr = wiGraphics::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphics::FORMAT RTFormat_hdr = wiGraphics::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphics::FORMAT RTFormat_gbuffer_0 = wiGraphics::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphics::FORMAT RTFormat_gbuffer_1 = wiGraphics::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphics::FORMAT RTFormat_gbuffer_2 = wiGraphics::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphics::FORMAT RTFormat_deferred_lightbuffer = wiGraphics::FORMAT_R11G11B10_FLOAT;
	static const wiGraphics::FORMAT RTFormat_lineardepth = wiGraphics::FORMAT_R16_UNORM;
	static const wiGraphics::FORMAT RTFormat_ssao = wiGraphics::FORMAT_R8_UNORM;
	static const wiGraphics::FORMAT RTFormat_waterripple = wiGraphics::FORMAT_R8G8B8A8_SNORM;
	static const wiGraphics::FORMAT RTFormat_normalmaps = wiGraphics::FORMAT_R8G8B8A8_SNORM;
	static const wiGraphics::FORMAT RTFormat_depthresolve = wiGraphics::FORMAT_R32_FLOAT;
	static const wiGraphics::FORMAT RTFormat_voxelradiance = wiGraphics::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphics::FORMAT RTFormat_envprobe = wiGraphics::FORMAT_R11G11B10_FLOAT;
	static const wiGraphics::FORMAT RTFormat_impostor = wiGraphics::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphics::FORMAT RTFormat_lightmap_object = wiGraphics::FORMAT_R32G32B32A32_FLOAT;
	static const wiGraphics::FORMAT RTFormat_lightmap_global = wiGraphics::FORMAT_R11G11B10_FLOAT;

	static const wiGraphics::FORMAT DSFormat_full = wiGraphics::FORMAT_D32_FLOAT_S8X24_UINT;
	static const wiGraphics::FORMAT DSFormat_full_alias = wiGraphics::FORMAT_R32G8X24_TYPELESS;
	static const wiGraphics::FORMAT DSFormat_small = wiGraphics::FORMAT_D16_UNORM;
	static const wiGraphics::FORMAT DSFormat_small_alias = wiGraphics::FORMAT_R16_TYPELESS;

	inline UINT CombineStencilrefs(STENCILREF engineStencilRef, uint8_t userStencilRef)
	{
		return (userStencilRef << 4) | static_cast<uint8_t>(engineStencilRef);
	}

	const wiGraphics::Sampler* GetSampler(int slot);
	const wiGraphics::VertexShader* GetVertexShader(VSTYPES id);
	const wiGraphics::HullShader* GetHullShader(VSTYPES id);
	const wiGraphics::DomainShader* GetDomainShader(VSTYPES id);
	const wiGraphics::GeometryShader* GetGeometryShader(VSTYPES id);
	const wiGraphics::PixelShader* GetPixelShader(PSTYPES id);
	const wiGraphics::ComputeShader* GetComputeShader(PSTYPES id);
	const wiGraphics::VertexLayout* GetVertexLayout(VLTYPES id);
	const wiGraphics::RasterizerState* GetRasterizerState(RSTYPES id);
	const wiGraphics::DepthStencilState* GetDepthStencilState(DSSTYPES id);
	const wiGraphics::BlendState* GetBlendState(BSTYPES id);
	const wiGraphics::GPUBuffer* GetConstantBuffer(CBTYPES id);
	const wiGraphics::Texture* GetTexture(TEXTYPES id);

	void ModifySampler(const wiGraphics::SamplerDesc& desc, int slot);


	void Initialize();
	void CleanUp();
	void LoadBuffers();
	void LoadShaders();
	void SetUpStates();
	void ClearWorld();

	// Set the main graphics device globally:
	void SetDevice(wiGraphics::GraphicsDevice* newDevice);
	// Retrieve the main graphics device:
	wiGraphics::GraphicsDevice* GetDevice();

	// Returns the shader path that you can also modify
	std::string& GetShaderPath();
	// Reload shaders, use the argument to modify the shader path. If the argument is empty, the shader path will not be modified
	void ReloadShaders(const std::string& path = "");

	// Returns the main camera that is currently being used in rendering (and also for post processing)
	wiSceneSystem::CameraComponent& GetCamera();
	// Returns the previous frame's camera that is currently being used in rendering to reproject
	wiSceneSystem::CameraComponent& GetPrevCamera();
	// Returns the planar reflection camera that is currently being used in rendering
	wiSceneSystem::CameraComponent& GetRefCamera();
	// Attach camera to entity for the current frame
	void AttachCamera(wiECS::Entity entity);

	// Updates the main scene, performs frustum culling for main camera and other tasks that are only done once per frame. Specify layerMask to only include specific entities in the render frame.
	void UpdatePerFrameData(float dt, uint32_t layerMask = ~0);
	// Updates the GPU state according to the previously called UpatePerFrameData()
	void UpdateRenderData(wiGraphics::CommandList cmd);

	// Binds all common constant buffers and samplers that may be used in all shaders
	void BindCommonResources(wiGraphics::CommandList cmd);
	// Updates the per frame constant buffer (need to call at least once per frame)
	void UpdateFrameCB(wiGraphics::CommandList cmd);
	// Updates the per camera constant buffer need to call for each different camera that is used when calling DrawScene() and the like
	void UpdateCameraCB(const wiSceneSystem::CameraComponent& camera, wiGraphics::CommandList cmd);
	// Set a global clipping plane state that is available to use in any shader (access as float4 g_xClipPlane)
	void SetClipPlane(const XMFLOAT4& clipPlane, wiGraphics::CommandList cmd);
	// Set a global alpha reference value that can be used by any shaders to perform alpha-testing (access as float g_xAlphaRef)
	void SetAlphaRef(float alphaRef, wiGraphics::CommandList cmd);
	// Resets the global shader alpha reference value to float g_xAlphaRef = 0.75f
	inline void ResetAlphaRef(wiGraphics::CommandList cmd) { SetAlphaRef(0.75f, cmd); }

	// Draw skydome centered to camera. Its color will be either dynamically computed, or the global environment map will be used if you called SetEnvironmentMap()
	void DrawSky(wiGraphics::CommandList cmd);
	// A black skydome will be draw with only the sun being visible on it
	void DrawSun(wiGraphics::CommandList cmd);
	// Draw the world from a camera. You must call UpdateCameraCB() at least once in this frame prior to this
	void DrawScene(const wiSceneSystem::CameraComponent& camera, bool tessellation, wiGraphics::CommandList cmd, RENDERPASS renderPass, bool grass, bool occlusionCulling);
	// Draw the transparent world from a camera. You must call UpdateCameraCB() at least once in this frame prior to this
	void DrawScene_Transparent(const wiSceneSystem::CameraComponent& camera, const wiGraphics::Texture2D& lineardepth, RENDERPASS renderPass, wiGraphics::CommandList cmd, bool grass, bool occlusionCulling);
	// Draw shadow maps for each visible light that has associated shadow maps
	void DrawShadowmaps(const wiSceneSystem::CameraComponent& camera, wiGraphics::CommandList cmd, uint32_t layerMask = ~0);
	// Draw debug world. You must also enable what parts to draw, eg. SetToDrawGridHelper, etc, see implementation for details what can be enabled.
	void DrawDebugWorld(const wiSceneSystem::CameraComponent& camera, wiGraphics::CommandList cmd);
	// Draw Soft offscreen particles. Linear depth should be already readable (see BindDepthTextures())
	void DrawSoftParticles(
		const wiSceneSystem::CameraComponent& camera, 
		const wiGraphics::Texture2D& lineardepth,
		bool distortion, 
		wiGraphics::CommandList cmd
	);
	// Draw deferred lights. Gbuffer and depth textures should already be readable (see BindGBufferTextures(), BindDepthTextures())
	void DrawDeferredLights(
		const wiSceneSystem::CameraComponent& camera,
		const wiGraphics::Texture2D& depthbuffer,
		const wiGraphics::Texture2D& gbuffer0,
		const wiGraphics::Texture2D& gbuffer1,
		const wiGraphics::Texture2D& gbuffer2,
		wiGraphics::CommandList cmd
	);
	// Draw simple light visualizer geometries
	void DrawLightVisualizers(
		const wiSceneSystem::CameraComponent& camera, 
		wiGraphics::CommandList cmd
	);
	// Draw volumetric light scattering effects
	void DrawVolumeLights(
		const wiSceneSystem::CameraComponent& camera,
		const wiGraphics::Texture2D& depthbuffer,
		wiGraphics::CommandList cmd
	);
	// Draw Lens Flares for lights that have them enabled
	void DrawLensFlares(
		const wiSceneSystem::CameraComponent& camera,
		const wiGraphics::Texture2D& depthbuffer,
		wiGraphics::CommandList cmd
	);
	// Draw deferred decals
	void DrawDeferredDecals(
		const wiSceneSystem::CameraComponent& camera,
		const wiGraphics::Texture2D& depthbuffer,
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
		const wiGraphics::Texture2D& depthbuffer,
		wiGraphics::CommandList cmd, 
		const wiGraphics::Texture2D* gbuffer0 = nullptr,
		const wiGraphics::Texture2D* gbuffer1 = nullptr,
		const wiGraphics::Texture2D* gbuffer2 = nullptr,
		const wiGraphics::Texture2D* lightbuffer_diffuse = nullptr,
		const wiGraphics::Texture2D* lightbuffer_specular = nullptr
	);
	// Run a compute shader that will resolve a MSAA depth buffer to a single-sample texture
	void ResolveMSAADepthBuffer(const wiGraphics::Texture2D& dst, const wiGraphics::Texture2D& src, wiGraphics::CommandList cmd);
	void DownsampleDepthBuffer(const wiGraphics::Texture2D& src, wiGraphics::CommandList cmd);
	// Compute the luminance for the source image and return the texture containing the luminance value in pixel [0,0]
	const wiGraphics::Texture2D* ComputeLuminance(const wiGraphics::Texture2D& sourceImage, wiGraphics::CommandList cmd);

	void DeferredComposition(
		const wiGraphics::Texture2D& gbuffer0,
		const wiGraphics::Texture2D& gbuffer1,
		const wiGraphics::Texture2D& gbuffer2,
		const wiGraphics::Texture2D& lightmap_diffuse,
		const wiGraphics::Texture2D& lightmap_specular,
		const wiGraphics::Texture2D& ao,
		const wiGraphics::Texture2D& lineardepth,
		wiGraphics::CommandList cmd);


	void Postprocess_Blur_Gaussian(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& temp,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd,
		float amountX = 1.0f,
		float amountY = 1.0f,
		float mip = 0.0f
	);
	void Postprocess_Blur_Bilateral(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& lineardepth,
		const wiGraphics::Texture2D& temp,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd,
		float amountX = 1.0f,
		float amountY = 1.0f,
		float depth_threshold = 1.0f,
		float mip = 0.0f
	);
	void Postprocess_SSAO(
		const wiGraphics::Texture2D& depthbuffer, 
		const wiGraphics::Texture2D& lineardepth, 
		const wiGraphics::Texture2D& temp,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd,
		float range = 1.0f,
		uint32_t samplecount = 16,
		float blur = 2.3f
	);
	void Postprocess_SSR(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& depthbuffer,
		const wiGraphics::Texture2D& lineardepth,
		const wiGraphics::Texture2D& gbuffer1,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_SSS(
		const wiGraphics::Texture2D& lineardepth,
		const wiGraphics::Texture2D& gbuffer0,
		const wiGraphics::RenderPass& input_output_lightbuffer_diffuse,
		const wiGraphics::RenderPass& input_output_temp1,
		const wiGraphics::RenderPass& input_output_temp2,
		wiGraphics::CommandList cmd
	);
	void Postprocess_LightShafts(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd,
		const XMFLOAT2& center
	);
	void Postprocess_DepthOfField(
		const wiGraphics::Texture2D& input_sharp,
		const wiGraphics::Texture2D& input_blurred,
		const wiGraphics::Texture2D& output,
		const wiGraphics::Texture2D& lineardepth,
		wiGraphics::CommandList cmd,
		float focus = 10.0f
	);
	void Postprocess_Outline(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd,
		float threshold = 0.1f,
		float thickness = 1.0f,
		const XMFLOAT4& color = XMFLOAT4(0, 0, 0, 1)
	);
	void Postprocess_MotionBlur(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& velocity,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_BloomSeparate(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd,
		float threshold
	);
	void Postprocess_FXAA(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_TemporalAA(
		const wiGraphics::Texture2D& input_current,
		const wiGraphics::Texture2D& input_history,
		const wiGraphics::Texture2D& velocity,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_Colorgrade(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& lookuptable,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_Lineardepth(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd
	);
	void Postprocess_Sharpen(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd,
		float amount = 1.0f
	);
	void Postprocess_Tonemap(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& input_luminance,
		const wiGraphics::Texture2D& input_distortion,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd,
		float exposure
	);
	void Postprocess_Chromatic_Aberration(
		const wiGraphics::Texture2D& input,
		const wiGraphics::Texture2D& output,
		wiGraphics::CommandList cmd,
		float amount = 1.0f
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
	RayBuffers* GenerateScreenRayBuffers(const wiSceneSystem::CameraComponent& camera, wiGraphics::CommandList cmd);
	// Render the scene with ray tracing. You provide the ray buffer, where each ray maps to one pixel of the result testure
	void RayTraceScene(
		const RayBuffers* rayBuffers,
		const wiGraphics::Texture2D* result,
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
		MIPGENFILTER_BICUBIC,
	};
	void GenerateMipChain(const wiGraphics::Texture1D& texture, MIPGENFILTER filter, wiGraphics::CommandList cmd, int arrayIndex = -1);
	void GenerateMipChain(const wiGraphics::Texture2D& texture, MIPGENFILTER filter, wiGraphics::CommandList cmd, int arrayIndex = -1);
	void GenerateMipChain(const wiGraphics::Texture3D& texture, MIPGENFILTER filter, wiGraphics::CommandList cmd, int arrayIndex = -1);

	enum BORDEREXPANDSTYLE
	{
		BORDEREXPAND_DISABLE,
		BORDEREXPAND_WRAP,
		BORDEREXPAND_CLAMP,
	};
	// Performs copy operation even between different texture formats
	//	Can also expand border region according to desired sampler func
	void CopyTexture2D(
		const wiGraphics::Texture2D& dst, UINT DstMIP, UINT DstX, UINT DstY, 
		const wiGraphics::Texture2D& src, UINT SrcMIP, 
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
	void SetEnvironmentMap(wiGraphics::Texture2D* tex);
	const wiGraphics::Texture2D* GetEnvironmentMap();
	const XMFLOAT4& GetWaterPlane();
	void SetGameSpeed(float value);
	float GetGameSpeed();
	void SetOceanEnabled(bool enabled);
	bool GetOceanEnabled();
	void InvalidateBVH(); // invalidates scene bvh so if something wants to use it, it will recompute and validate it
	void SetRaytraceBounceCount(uint32_t bounces);
	uint32_t GetRaytraceBounceCount();
	void SetRaytraceDebugBVHVisualizerEnabled(bool value);
	bool GetRaytraceDebugBVHVisualizerEnabled();

	const wiGraphics::Texture2D* GetGlobalLightmap();

	// Gets pick ray according to the current screen resolution and pointer coordinates. Can be used as input into RayIntersectWorld()
	RAY GetPickRay(long cursorX, long cursorY);


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
	void AddDeferredMIPGen(const wiGraphics::Texture2D* tex);

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

