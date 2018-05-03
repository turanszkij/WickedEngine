#pragma once
#include "CommonInclude.h"
#include "ShaderInterop.h"
#include "wiEnums.h"
#include "wiGraphicsAPI.h"
#include "wiSPTree.h"
#include "wiWindowRegistration.h"

#include <unordered_set>

struct Transform;
struct Vertex;
struct SkinnedVertex;
struct Material;
struct Object;
struct Mesh;
struct Armature;
struct Bone;
struct KeyFrame;
struct SHCAM;
struct Light;
struct Decal;
struct WorldInfo;
struct Wind;
struct Camera;
struct RAY;
struct Camera;
struct Model;
struct Scene;

class  Lines;
class  Cube;
class  wiTranslator;
class  wiParticle;
class  wiEmittedParticle;
class  wiHairParticle;
class  wiSprite;
class  wiSPTree;
class  TaskThread;
struct Cullable;
class  PHYSICS;
class  wiRenderTarget;
class  wiWaterPlane;
class  wiOcean;
struct wiOceanParameter;

typedef std::map<std::string, Mesh*> MeshCollection;
typedef std::map<std::string, Material*> MaterialCollection;


class wiRenderer
{
public:
	static wiGraphicsTypes::GraphicsDevice* graphicsDevice;
	static wiGraphicsTypes::GraphicsDevice* GetDevice() { assert(graphicsDevice != nullptr);  return graphicsDevice; }


	static void Present(std::function<void()> drawToScreen1=nullptr, std::function<void()> drawToScreen2=nullptr, std::function<void()> drawToScreen3=nullptr);

	
	static wiGraphicsTypes::Sampler				*samplers[SSLOT_COUNT];
	static wiGraphicsTypes::VertexShader		*vertexShaders[VSTYPE_LAST];
	static wiGraphicsTypes::PixelShader			*pixelShaders[PSTYPE_LAST];
	static wiGraphicsTypes::GeometryShader		*geometryShaders[GSTYPE_LAST];
	static wiGraphicsTypes::HullShader			*hullShaders[HSTYPE_LAST];
	static wiGraphicsTypes::DomainShader		*domainShaders[DSTYPE_LAST];
	static wiGraphicsTypes::ComputeShader		*computeShaders[CSTYPE_LAST];
	static wiGraphicsTypes::VertexLayout		*vertexLayouts[VLTYPE_LAST];
	static wiGraphicsTypes::RasterizerState		*rasterizers[RSTYPE_LAST];
	static wiGraphicsTypes::DepthStencilState	*depthStencils[DSSTYPE_LAST];
	static wiGraphicsTypes::BlendState			*blendStates[BSTYPE_LAST];
	static wiGraphicsTypes::GPUBuffer			*constantBuffers[CBTYPE_LAST];
	static wiGraphicsTypes::GPUBuffer			*resourceBuffers[RBTYPE_LAST];
	static wiGraphicsTypes::Texture				*textures[TEXTYPE_LAST];
	static wiGraphicsTypes::Sampler				*customsamplers[SSTYPE_LAST];

	static wiGraphicsTypes::GPURingBuffer		*dynamicVertexBufferPool;

	static const wiGraphicsTypes::FORMAT RTFormat_ldr = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_hdr = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_0 = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_1 = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_2 = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_gbuffer_3 = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	// NOTE: Light buffer precision seems OK when using FORMAT_R11G11B10_FLOAT format
	// But the environmental light now also writes the AO to ALPHA so it has been changed to FORMAT_R16G16B16A16_FLOAT
	static const wiGraphicsTypes::FORMAT RTFormat_deferred_lightbuffer = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_lineardepth = wiGraphicsTypes::FORMAT_R32_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_ssao = wiGraphicsTypes::FORMAT_R8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_waterripple = wiGraphicsTypes::FORMAT_R8G8B8A8_SNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_normalmaps = wiGraphicsTypes::FORMAT_R8G8B8A8_SNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_depthresolve = wiGraphicsTypes::FORMAT_R32_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_voxelradiance = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_envprobe = wiGraphicsTypes::FORMAT_R16G16B16A16_FLOAT;
	static const wiGraphicsTypes::FORMAT RTFormat_impostor_albedo = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_impostor_normal = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;
	static const wiGraphicsTypes::FORMAT RTFormat_impostor_surface = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM;

	static const wiGraphicsTypes::FORMAT DSFormat_full = wiGraphicsTypes::FORMAT_D32_FLOAT_S8X24_UINT;
	static const wiGraphicsTypes::FORMAT DSFormat_full_alias = wiGraphicsTypes::FORMAT_R32G8X24_TYPELESS;
	static const wiGraphicsTypes::FORMAT DSFormat_small = wiGraphicsTypes::FORMAT_D16_UNORM;
	static const wiGraphicsTypes::FORMAT DSFormat_small_alias = wiGraphicsTypes::FORMAT_R16_TYPELESS;
	
	static float GAMMA;
	static int SHADOWRES_2D, SHADOWRES_CUBE, SHADOWCOUNT_2D, SHADOWCOUNT_CUBE, SOFTSHADOWQUALITY_2D;
	static bool HAIRPARTICLEENABLED, EMITTERSENABLED;
	static float SPECULARAA;
	static float renderTime, renderTime_Prev, deltaTime;
	static XMFLOAT2 temporalAAJitter, temporalAAJitterPrev;
	static float RESOLUTIONSCALE;
	static bool TRANSPARENTSHADOWSENABLED;
	static bool ALPHACOMPOSITIONENABLED;

	static void SetShadowProps2D(int resolution, int count, int softShadowQuality);
	static void SetShadowPropsCube(int resolution, int count);

	// Constant Buffers:
	// Persistent buffers:
	CBUFFER(WorldCB, CBSLOT_RENDERER_WORLD)
	{
		XMFLOAT2 mScreenWidthHeight;
		XMFLOAT2 mScreenWidthHeight_Inverse;
		XMFLOAT2 mInternalResolution;
		XMFLOAT2 mInternalResolution_Inverse;
		float	 mGamma;
		XMFLOAT3 mHorizon;
		XMFLOAT3 mZenith;
		float mCloudScale;
		XMFLOAT3 mAmbient;
		float mCloudiness;
		XMFLOAT3 mFog;
		float mSpecularAA;
		float mVoxelRadianceDataSize;
		float mVoxelRadianceDataSize_Inverse;
		UINT mVoxelRadianceDataRes;
		float mVoxelRadianceDataRes_Inverse;
		UINT mVoxelRadianceDataMIPs;
		UINT mVoxelRadianceDataNumCones;
		float mVoxelRadianceDataNumCones_Inverse;
		float mVoxelRadianceDataRayStepSize;
		BOOL mVoxelRadianceReflectionsEnabled;
		XMFLOAT3 mVoxelRadianceDataCenter;
		BOOL mAdvancedRefractions;
		XMUINT3 mEntityCullingTileCount;
		BOOL mTransparentShadowsEnabled;
		int	mGlobalEnvProbeIndex;
		UINT mEnvProbeMipCount;
		float mEnvProbeMipCount_Inverse;
	};
	CBUFFER(FrameCB, CBSLOT_RENDERER_FRAME)
	{
		float mTime;
		float mTimePrev;
		float mDeltaTime;
		UINT mFrameCount;
		UINT mLightArrayOffset;
		UINT mLightArrayCount;
		UINT mDecalArrayOffset;
		UINT mDecalArrayCount;
		UINT mForceFieldArrayOffset;
		UINT mForceFieldArrayCount;
		UINT mEnvProbeArrayOffset;
		UINT mEnvProbeArrayCount;
		XMFLOAT3 mWindDirection;
		float mWindWaveSize;
		float mWindRandomness;
		int mSunEntityArrayIndex;
		BOOL mVoxelRadianceRetargetted;
		UINT mTemporalAASampleRotation;
		XMFLOAT2 mTemporalAAJitter;
		XMFLOAT2 mTemporalAAJitterPrev;
		XMMATRIX mVP;
		XMMATRIX mView;
		XMMATRIX mProj;
		XMFLOAT3 mCamPos;
		float mCamDistanceFromOrigin;
		XMMATRIX mPrevV;
		XMMATRIX mPrevP;
		XMMATRIX mPrevVP;
		XMMATRIX mPrevInvVP;
		XMMATRIX mReflVP;
		XMMATRIX mInvV;
		XMMATRIX mInvP;
		XMMATRIX mInvVP;
		XMFLOAT3 mAt;
		float    mZNearP;
		XMFLOAT3 mUp;
		float    mZFarP;
		XMFLOAT4 mFrustumPlanesWS[6];
	};
	CBUFFER(CameraCB, CBSLOT_RENDERER_CAMERA)
	{
		XMMATRIX mVP;
		XMMATRIX mView;
		XMMATRIX mProj;
		XMFLOAT3 mCamPos;				float pad0;
	};
	CBUFFER(MiscCB, CBSLOT_RENDERER_MISC)
	{
		XMMATRIX mTransform;
		XMFLOAT4 mColor;
	};

	// On demand buffers:
	CBUFFER(VolumeLightCB, CBSLOT_RENDERER_VOLUMELIGHT)
	{
		XMMATRIX world;
		XMFLOAT4 col;
		XMFLOAT4 enerdis;
	};
	CBUFFER(DecalCB, CBSLOT_RENDERER_DECAL)
	{
		XMMATRIX mDecalVP;
		int hasTexNor;
		XMFLOAT3 eye;
		float opacity;
		XMFLOAT3 front;
	};
	CBUFFER(CubeMapRenderCB, CBSLOT_RENDERER_CUBEMAPRENDER)
	{
		XMMATRIX mViewProjection[6];
	};
	CBUFFER(APICB, CBSLOT_API)
	{
		XMFLOAT4 clipPlane;
		float	alphaRef;
		float	pad[3];

		APICB(const XMFLOAT4& clipPlane = XMFLOAT4(0, 0, 0, 0), float alphaRef = 0.75f) :clipPlane(clipPlane), alphaRef(alphaRef) {}
	};
	CBUFFER(TessellationCB, CBSLOT_RENDERER_TESSELLATION)
	{
		XMFLOAT4 tessellationFactors;
	};
	CBUFFER(DispatchParamsCB, CBSLOT_RENDERER_DISPATCHPARAMS)
	{
		UINT numThreadGroups[3];	
		UINT value0;
		UINT numThreads[3];
		UINT value1;
	};

protected:

	static void SetUpBoneLines();
	static void UpdateBoneLines();
	static void SetUpCubes();
	static void UpdateCubes();
	


	static bool	wireRender, debugSpheres, debugBoneLines, debugPartitionTree, debugEnvProbes, debugEmitters, debugForceFields, gridHelper, voxelHelper, advancedLightCulling, advancedRefractions;
	static bool ldsSkinningEnabled;
	static bool requestReflectionRendering;


	static wiGraphicsTypes::Texture2D* enviroMap,*colorGrading;
	static void LoadBuffers();
	static void LoadShaders();
	static void SetUpStates();
	static int vertexCount;
	static int visibleCount;


	static void addVertexCount(const int& toadd){vertexCount+=toadd;}


	static float GameSpeed, overrideGameSpeed;

	static Scene* scene;

	static wiWaterPlane waterPlane;

	static bool debugLightCulling;
	static bool occlusionCulling;
	static bool temporalAA, temporalAADEBUG;
	static bool freezeCullingCamera;

	struct VoxelizedSceneData
	{
		bool enabled;
		int res;
		float voxelsize;
		XMFLOAT3 center;
		XMFLOAT3 extents;
		int numCones;
		float rayStepSize;
		bool secondaryBounceEnabled;
		bool reflectionsEnabled;
		bool centerChangedThisFrame;
		UINT mips;

		VoxelizedSceneData() :enabled(false), res(128), voxelsize(1.0f), center(XMFLOAT3(0, 0, 0)), extents(XMFLOAT3(0, 0, 0)), numCones(8),
			rayStepSize(0.5f), secondaryBounceEnabled(true), reflectionsEnabled(false), centerChangedThisFrame(true), mips(7)
		{}
	} static voxelSceneData;

	static wiGraphicsTypes::GPUQuery occlusionQueries[256];

	static UINT entityArrayOffset_Lights, entityArrayCount_Lights;
	static UINT entityArrayOffset_Decals, entityArrayCount_Decals;
	static UINT entityArrayOffset_ForceFields, entityArrayCount_ForceFields;
	static UINT entityArrayOffset_EnvProbes, entityArrayCount_EnvProbes;

public:
	static std::string SHADERPATH;

	wiRenderer();
	void CleanUp();
	static void SetUpStaticComponents();
	static void CleanUpStatic();
	
	static void FixedUpdate();
	// Render data that needs to be updated on the main thread!
	static void UpdatePerFrameData(float dt);
	static void UpdateRenderData(GRAPHICSTHREAD threadID);
	static void OcclusionCulling_Render(GRAPHICSTHREAD threadID);
	static void OcclusionCulling_Read();
	static void UpdateImages();
	static void ManageImages();
	static void PutDecal(Decal* decal);
	static void PutWaterRipple(const std::string& image, const XMFLOAT3& pos);
	static void ManageWaterRipples();
	static void DrawWaterRipples(GRAPHICSTHREAD threadID);
	static void SetGameSpeed(float value){GameSpeed=value; if(GameSpeed<0) GameSpeed=0;};
	static float GetGameSpeed();
	static void SetResolutionScale(float value) { RESOLUTIONSCALE = value; }
	static float GetResolutionScale() { return RESOLUTIONSCALE; }
	static void SetTransparentShadowsEnabled(float value) { TRANSPARENTSHADOWSENABLED = value; }
	static float GetTransparentShadowsEnabled() { return TRANSPARENTSHADOWSENABLED; }
	static XMUINT2 GetInternalResolution() { return XMUINT2((UINT)ceilf(GetDevice()->GetScreenWidth()*GetResolutionScale()), (UINT)ceilf(GetDevice()->GetScreenHeight()*GetResolutionScale())); }
	static bool ResolutionChanged();
	static void SetGamma(float value) { GAMMA = value; }
	static float GetGamma() { return GAMMA; }
	static void SetWireRender(bool value) { wireRender = value; }
	static bool IsWireRender() { return wireRender; }
	static void ToggleDebugSpheres(){debugSpheres=!debugSpheres;}
	static void SetToDrawDebugSpheres(bool param){debugSpheres=param;}
	static bool GetToDrawDebugSpheres() { return debugSpheres; }
	static void SetToDrawDebugBoneLines(bool param) { debugBoneLines = param; }
	static bool GetToDrawDebugBoneLines() { return debugBoneLines; }
	static void SetToDrawDebugPartitionTree(bool param){debugPartitionTree=param;}
	static bool GetToDrawDebugPartitionTree(){return debugPartitionTree;}
	static bool GetToDrawDebugEnvProbes() { return debugEnvProbes; }
	static void SetToDrawDebugEnvProbes(bool value) { debugEnvProbes = value; }
	static void SetToDrawDebugEmitters(bool param) { debugEmitters = param; }
	static bool GetToDrawDebugEmitters() { return debugEmitters; }
	static void SetToDrawDebugForceFields(bool param) { debugForceFields = param; }
	static bool GetToDrawDebugForceFields() { return debugForceFields; }
	static bool GetToDrawGridHelper() { return gridHelper; }
	static void SetToDrawGridHelper(bool value) { gridHelper = value; }
	static bool GetToDrawVoxelHelper() { return voxelHelper; }
	static void SetToDrawVoxelHelper(bool value) { voxelHelper = value; }
	static void SetDebugLightCulling(bool enabled) { debugLightCulling = enabled; }
	static bool GetDebugLightCulling() { return debugLightCulling; }
	static void SetAdvancedLightCulling(bool enabled) { advancedLightCulling = enabled; }
	static bool GetAdvancedLightCulling() { return advancedLightCulling; }
	static void SetAlphaCompositionEnabled(bool enabled) { ALPHACOMPOSITIONENABLED = enabled; }
	static bool GetAlphaCompositionEnabled() { return ALPHACOMPOSITIONENABLED; }
	static void SetOcclusionCullingEnabled(bool enabled); // also inits query pool!
	static bool GetOcclusionCullingEnabled() { return occlusionCulling; }
	static void SetLDSSkinningEnabled(bool enabled) { ldsSkinningEnabled = enabled; }
	static bool GetLDSSkinningEnabled() { return ldsSkinningEnabled; }
	static void SetTemporalAAEnabled(bool enabled) { temporalAA = enabled; }
	static bool GetTemporalAAEnabled() { return temporalAA; }
	static void SetTemporalAADebugEnabled(bool enabled) { temporalAADEBUG = enabled; }
	static bool GetTemporalAADebugEnabled() { return temporalAADEBUG; }
	static void SetFreezeCullingCameraEnabled(bool enabled) { freezeCullingCamera = enabled; }
	static bool GetFreezeCullingCameraEnabled() { return freezeCullingCamera; }
	static void SetVoxelRadianceEnabled(bool enabled) { voxelSceneData.enabled = enabled; }
	static bool GetVoxelRadianceEnabled() { return voxelSceneData.enabled; }
	static void SetVoxelRadianceSecondaryBounceEnabled(bool enabled) { voxelSceneData.secondaryBounceEnabled = enabled; }
	static bool GetVoxelRadianceSecondaryBounceEnabled() { return voxelSceneData.secondaryBounceEnabled; }
	static void SetVoxelRadianceReflectionsEnabled(bool enabled) { voxelSceneData.reflectionsEnabled = enabled; }
	static bool GetVoxelRadianceReflectionsEnabled() { return voxelSceneData.reflectionsEnabled; }
	static void SetVoxelRadianceVoxelSize(float value) { voxelSceneData.voxelsize = value; }
	static float GetVoxelRadianceVoxelSize() { return voxelSceneData.voxelsize; }
	//static void SetVoxelRadianceResolution(int value) { voxelSceneData.res = value; }
	static int GetVoxelRadianceResolution() { return voxelSceneData.res; }
	static void SetVoxelRadianceNumCones(int value) { voxelSceneData.numCones = value; }
	static int GetVoxelRadianceNumCones() { return voxelSceneData.numCones; }
	static float GetVoxelRadianceRayStepSize() { return voxelSceneData.rayStepSize; }
	static void SetVoxelRadianceRayStepSize(float value) { voxelSceneData.rayStepSize = value; }
	static void SetSpecularAAParam(float value) { SPECULARAA = value; }
	static float GetSpecularAAParam() { return SPECULARAA; }
	static void SetAdvancedRefractionsEnabled(bool value) { advancedRefractions = value; }
	static bool GetAdvancedRefractionsEnabled(); // also needs additional driver support for now...
	static bool IsRequestedReflectionRendering() { return requestReflectionRendering; }
	static wiGraphicsTypes::Texture2D* GetColorGrading(){return colorGrading;};
	static void SetColorGrading(wiGraphicsTypes::Texture2D* tex){colorGrading=tex;};
	static void SetEnviromentMap(wiGraphicsTypes::Texture2D* tex){ enviroMap = tex; }
	static wiGraphicsTypes::Texture2D* GetEnviromentMap(){ return enviroMap; }
	static wiGraphicsTypes::Texture2D* GetLuminance(wiGraphicsTypes::Texture2D* sourceImage, GRAPHICSTHREAD threadID);
	static wiWaterPlane GetWaterPlane();

	static Transform* getTransformByName(const std::string& name);
	static Transform* getTransformByID(uint64_t id);
	static Armature* getArmatureByName(const std::string& get);
	static int getActionByName(Armature* armature, const std::string& get);
	static int getBoneByName(Armature* armature, const std::string& get);
	static Material* getMaterialByName(const std::string& get);
	HitSphere* getSphereByName(const std::string& get);
	static Object* getObjectByName(const std::string& name);
	static Light* getLightByName(const std::string& name);

	static void ReloadShaders(const std::string& path = "");
	static void BindPersistentState(GRAPHICSTHREAD threadID);

	static Mesh::Vertex_FULL TransformVertex(const Mesh* mesh, int vertexI, const XMMATRIX& mat = XMMatrixIdentity());

	struct FrameCulling
	{
		Frustum frustum;
		CulledCollection culledRenderer;
		CulledCollection culledRenderer_opaque;
		CulledCollection culledRenderer_transparent;
		std::vector<wiHairParticle*> culledHairParticleSystems;
		CulledList culledLights;
		std::list<Decal*> culledDecals;
		std::list<EnvironmentProbe*> culledEnvProbes;

		void Clear()
		{
			culledRenderer.clear();
			culledRenderer_opaque.clear();
			culledRenderer_transparent.clear();
			culledHairParticleSystems.clear();
			culledLights.clear();
			culledDecals.clear();
			culledEnvProbes.clear();
		}
	};
	static std::unordered_map<Camera*, FrameCulling> frameCullings;

	inline static XMUINT3 GetEntityCullingTileCount()
	{
		return XMUINT3(
			(UINT)ceilf((float)GetInternalResolution().x / (float)TILED_CULLING_BLOCKSIZE),
			(UINT)ceilf((float)GetInternalResolution().y / (float)TILED_CULLING_BLOCKSIZE),
			1);
	}
	
public:
	

	static void UpdateWorldCB(GRAPHICSTHREAD threadID);
	static void UpdateFrameCB(GRAPHICSTHREAD threadID);
	static void UpdateCameraCB(Camera* camera, GRAPHICSTHREAD threadID);
	static void SetClipPlane(const XMFLOAT4& clipPlane, GRAPHICSTHREAD threadID);
	static void SetAlphaRef(float alphaRef, GRAPHICSTHREAD threadID);
	static void ResetAlphaRef(GRAPHICSTHREAD threadID) { SetAlphaRef(0.75f, threadID); }
	static void UpdateGBuffer(wiGraphicsTypes::Texture2D* slot0, wiGraphicsTypes::Texture2D* slot1, wiGraphicsTypes::Texture2D* slot2, wiGraphicsTypes::Texture2D* slot3, wiGraphicsTypes::Texture2D* slot4, GRAPHICSTHREAD threadID);
	static void UpdateDepthBuffer(wiGraphicsTypes::Texture2D* depth, wiGraphicsTypes::Texture2D* linearDepth, GRAPHICSTHREAD threadID);
	
	static void RenderMeshes(const XMFLOAT3& eye, const CulledCollection& culledRenderer, SHADERTYPE shaderType, UINT renderTypeFlags, GRAPHICSTHREAD threadID, bool tessellation = false, bool occlusionCulling = false);
	static void DrawSky(GRAPHICSTHREAD threadID);
	static void DrawSun(GRAPHICSTHREAD threadID);
	static void DrawWorld(Camera* camera, bool tessellation, GRAPHICSTHREAD threadID, SHADERTYPE shaderType, bool grass, bool occlusionCulling);
	static void DrawForShadowMap(GRAPHICSTHREAD threadID);
	static void DrawWorldTransparent(Camera* camera, SHADERTYPE shaderType, GRAPHICSTHREAD threadID, bool grass, bool occlusionCulling);
	void DrawDebugSpheres(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugBoneLines(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugLines(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugBoxes(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawTranslators(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugEnvProbes(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugGridHelper(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugVoxels(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugEmitters(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawDebugForceFields(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawSoftParticles(Camera* camera, bool distortion, GRAPHICSTHREAD threadID);
	static void DrawTrails(GRAPHICSTHREAD threadID, wiGraphicsTypes::Texture2D* refracRes);
	static void DrawImagesAdd(GRAPHICSTHREAD threadID, wiGraphicsTypes::Texture2D* refracRes);
	//alpha-opaque
	static void DrawImages(GRAPHICSTHREAD threadID, wiGraphicsTypes::Texture2D* refracRes);
	static void DrawImagesNormals(GRAPHICSTHREAD threadID, wiGraphicsTypes::Texture2D* refracRes);
	static void DrawLights(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawLightVisualizers(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawVolumeLights(Camera* camera, GRAPHICSTHREAD threadID);
	static void DrawLensFlares(GRAPHICSTHREAD threadID);
	static void DrawDecals(Camera* camera, GRAPHICSTHREAD threadID);
	static void RefreshEnvProbes(GRAPHICSTHREAD threadID);
	static void VoxelRadiance(GRAPHICSTHREAD threadID);

	static void ComputeTiledLightCulling(bool deferred, GRAPHICSTHREAD threadID);
	static void ResolveMSAADepthBuffer(wiGraphicsTypes::Texture2D* dst, wiGraphicsTypes::Texture2D* src, GRAPHICSTHREAD threadID);

	enum MIPGENFILTER
	{
		MIPGENFILTER_POINT,
		MIPGENFILTER_LINEAR,
		MIPGENFILTER_LINEAR_MAXIMUM,
		MIPGENFILTER_GAUSSIAN,
	};
	static void GenerateMipChain(wiGraphicsTypes::Texture1D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID);
	static void GenerateMipChain(wiGraphicsTypes::Texture2D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID);
	static void GenerateMipChain(wiGraphicsTypes::Texture3D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID);

	// dst: Texture2D with unordered access, the output will be written to this
	// refinementCount: 0: auto select, 1: perfect noise, greater numbers: smoother clouds, slower processing
	// randomness: random seed
	static void GenerateClouds(wiGraphicsTypes::Texture2D* dst, UINT refinementCount, float randomness, GRAPHICSTHREAD threadID);

	static void ManageDecalAtlas(GRAPHICSTHREAD threadID);
	
	static XMVECTOR GetSunPosition();
	static XMFLOAT4 GetSunColor();
	static int GetSunArrayIndex();
	static int getVertexCount(){return vertexCount;}
	static void resetVertexCount(){vertexCount=0;}
	static int getVisibleObjectCount(){return visibleCount;}
	static void resetVisibleObjectCount(){visibleCount=0;}

	static void FinishLoading();
	static wiSPTree* spTree;
	static wiSPTree* spTree_lights;

	// The scene holds all models, world information and wind information
	static Scene& GetScene();

	static std::vector<Lines*> boneLines;
	static std::vector<Lines*> linesTemp;
	static std::vector<Cube> cubes;

	static std::unordered_set<Object*> objectsWithTrails;
	static std::unordered_set<wiEmittedParticle*> emitterSystems;
	
	static std::deque<wiSprite*> images;
	static std::deque<wiSprite*> waterRipples;
	static void ClearWorld();


	static wiRenderTarget normalMapRT, imagesRT, imagesRTAdd;
	
	static Camera* cam, *refCam, *prevFrameCam;
	static Camera* getCamera(){ return cam; }
	static Camera* getRefCamera(){ return refCam; }

	std::string DIRECTORY;

	struct Picked
	{
		Transform* transform;
		Object* object;
		Light* light;
		Decal* decal;
		EnvironmentProbe* envProbe;
		ForceField* forceField;
		XMFLOAT3 position,normal;
		float distance;
		int subsetIndex;

		Picked()
		{
			Clear();
		}

		// Subset index, position, normal, distance don't distinguish between pickeds! 
		bool operator==(const Picked& other)
		{
			return
				transform == other.transform &&
				object == other.object &&
				light == other.light &&
				decal == other.decal &&
				envProbe == other.envProbe &&
				forceField == other.forceField
				;
		}
		void Clear()
		{
			distance = 0;
			subsetIndex = -1;
			SAFE_INIT(transform);
			SAFE_INIT(object);
			SAFE_INIT(light);
			SAFE_INIT(decal);
			SAFE_INIT(envProbe);
			SAFE_INIT(forceField);
		}
	};

	// Pick closest object in the world
	// pickType: PICKTYPE enum values ocncatenated with | operator
	// layer : concatenated string of layers to check against, empty string : all layers will be checked
	// layerDisable : concatenated string of layers to NOT check against
	static Picked Pick(long cursorX, long cursorY, int pickType = PICK_OPAQUE, const std::string& layer = "", const std::string& layerDisable = "");
	static Picked Pick(RAY& ray, int pickType = PICK_OPAQUE, const std::string& layer = "", const std::string& layerDisable = "");
	static RAY getPickRay(long cursorX, long cursorY);
	static void RayIntersectMeshes(const RAY& ray, const CulledList& culledObjects, std::vector<Picked>& points,
		int pickType = PICK_OPAQUE, bool dynamicObjects = true, const std::string& layer = "", const std::string& layerDisable = "", bool onlyVisible = false);
	static void CalculateVertexAO(Object* object);

	static PHYSICS* physicsEngine;
	static void SynchronizeWithPhysicsEngine(float dt = 1.0f / 60.0f);

	static wiOcean* ocean;
	static void SetOceanEnabled(bool enabled, const wiOceanParameter& params);
	static wiOcean* GetOcean() { return ocean; }

	static Model* LoadModel(const std::string& fileName, const XMMATRIX& transform = XMMatrixIdentity(), const std::string& ident = "common");
	static void LoadWorldInfo(const std::string& fileName);
	static void LoadDefaultLighting();

	static void PutEnvProbe(const XMFLOAT3& position);

	static void CreateImpostor(Mesh* mesh);

	static std::vector<wiTranslator*> renderableTranslators;
	// Add translator to render in next frame
	static void AddRenderableTranslator(wiTranslator* translator);

	static std::vector<std::pair<XMFLOAT4X4,XMFLOAT4>> renderableBoxes;
	// Add box to render in next frame
	static void AddRenderableBox(const XMFLOAT4X4& boxMatrix, const XMFLOAT4& color = XMFLOAT4(1,1,1,1));

	// Add model to the scene
	static void AddModel(Model* value);
	// Add Object Instance
	static void Add(Object* value);
	// Add Light Instance
	static void Add(Light* value);
	// Add Force Field Instance
	static void Add(ForceField* value);

	// Remove from the scene
	static void Remove(Object* value);
	static void Remove(Light* value);
	static void Remove(Decal* value);
	static void Remove(EnvironmentProbe* value);
	static void Remove(ForceField* value);
};

