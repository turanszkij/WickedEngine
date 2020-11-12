#include "wiRenderer.h"
#include "wiHairParticle.h"
#include "wiEmittedParticle.h"
#include "wiSprite.h"
#include "wiScene.h"
#include "wiHelper.h"
#include "wiMath.h"
#include "wiTextureHelper.h"
#include "wiEnums.h"
#include "wiRandom.h"
#include "wiRectPacker.h"
#include "wiBackLog.h"
#include "wiProfiler.h"
#include "wiOcean.h"
#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_Postprocess.h"
#include "ShaderInterop_Skinning.h"
#include "ShaderInterop_Raytracing.h"
#include "ShaderInterop_BVH.h"
#include "ShaderInterop_Utility.h"
#include "ShaderInterop_Paint.h"
#include "wiGPUSortLib.h"
#include "wiAllocators.h"
#include "wiGPUBVH.h"
#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiEvent.h"
#include "wiPlatform.h"

#include <algorithm>
#include <unordered_set>
#include <deque>
#include <array>
#include <atomic>

using namespace std;
using namespace wiGraphics;
using namespace wiScene;
using namespace wiECS;
using namespace wiAllocators;

namespace wiRenderer
{

std::shared_ptr<GraphicsDevice> device;

Shader					shaders[SHADERTYPE_COUNT];
Texture					textures[TEXTYPE_COUNT];
InputLayout				inputLayouts[ILTYPE_COUNT];
RasterizerState			rasterizers[RSTYPE_COUNT];
DepthStencilState		depthStencils[DSSTYPE_COUNT];
BlendState				blendStates[BSTYPE_COUNT];
GPUBuffer				constantBuffers[CBTYPE_COUNT];
GPUBuffer				resourceBuffers[RBTYPE_COUNT];
Sampler					samplers[SSLOT_COUNT];

string SHADERPATH = wiHelper::GetOriginalWorkingDirectory() + "../WickedEngine/shaders/";

LinearAllocator updateFrameAllocator; // can be used by an update thread
LinearAllocator renderFrameAllocators[COMMANDLIST_COUNT]; // can be used by graphics threads
inline LinearAllocator& GetUpdateFrameAllocator()
{
	if (updateFrameAllocator.get_capacity() == 0)
	{
		updateFrameAllocator.reserve(4 * 1024 * 1024);
	}
	return updateFrameAllocator;
}
inline LinearAllocator& GetRenderFrameAllocator(CommandList cmd)
{
	if (renderFrameAllocators[cmd].get_capacity() == 0)
	{
		renderFrameAllocators[cmd].reserve(4 * 1024 * 1024);
	}
	return renderFrameAllocators[cmd];
}

float GAMMA = 2.2f;
uint32_t SHADOWRES_2D = 1024;
uint32_t SHADOWRES_CUBE = 256;
uint32_t SHADOWCOUNT_2D = 5 + 3 + 3;
uint32_t SHADOWCOUNT_CUBE = 5;
uint32_t SOFTSHADOWQUALITY_2D = 2;
bool TRANSPARENTSHADOWSENABLED = false;
bool wireRender = false;
bool debugBoneLines = false;
bool debugPartitionTree = false;
bool debugEmitters = false;
bool freezeCullingCamera = false;
bool debugEnvProbes = false;
bool debugForceFields = false;
bool debugCameras = false;
bool gridHelper = false;
bool voxelHelper = false;
bool requestReflectionRendering = false;
bool requestVolumetricLightRendering = false;
bool advancedLightCulling = true;
bool variableRateShadingClassification = false;
bool variableRateShadingClassificationDebug = false;
bool ldsSkinningEnabled = true;
bool scene_bvh_invalid = true;
float renderTime = 0;
float renderTime_Prev = 0;
float deltaTime = 0;
XMFLOAT2 temporalAAJitter = XMFLOAT2(0, 0);
XMFLOAT2 temporalAAJitterPrev = XMFLOAT2(0, 0);
float RESOLUTIONSCALE = 1.0f;
GPUQueryRing<GraphicsDevice::GetBackBufferCount() + 1> occlusionQueries[256];
uint32_t entityArrayOffset_Lights = 0;
uint32_t entityArrayCount_Lights = 0;
uint32_t entityArrayOffset_Decals = 0;
uint32_t entityArrayCount_Decals = 0;
uint32_t entityArrayOffset_ForceFields = 0;
uint32_t entityArrayCount_ForceFields = 0;
uint32_t entityArrayOffset_EnvProbes = 0;
uint32_t entityArrayCount_EnvProbes = 0;
float GameSpeed = 1;
bool debugLightCulling = false;
bool occlusionCulling = false;
bool temporalAA = false;
bool temporalAADEBUG = false;
uint32_t raytraceBounceCount = 2;
bool raytraceDebugVisualizer = false;
bool raytracedShadows = false;
bool tessellationEnabled = false;
Entity cameraTransform = INVALID_ENTITY;


struct VoxelizedSceneData
{
	bool enabled = false;
	uint32_t res = 128;
	float voxelsize = 1;
	XMFLOAT3 center = XMFLOAT3(0, 0, 0);
	XMFLOAT3 extents = XMFLOAT3(0, 0, 0);
	uint32_t numCones = 2;
	float rayStepSize = 0.75f;
	float maxDistance = 20.0f;
	bool secondaryBounceEnabled = false;
	bool reflectionsEnabled = true;
	bool centerChangedThisFrame = true;
	uint32_t mips = 7;
} voxelSceneData;

std::unique_ptr<wiOcean> ocean;

Texture shadowMapArray_2D;
Texture shadowMapArray_Cube;
Texture shadowMapArray_Transparent;
std::vector<RenderPass> renderpasses_shadow2D;
std::vector<RenderPass> renderpasses_shadow2DTransparent;
std::vector<RenderPass> renderpasses_shadowCube;

deque<wiSprite> waterRipples;

std::vector<pair<XMFLOAT4X4, XMFLOAT4>> renderableBoxes;
std::vector<pair<SPHERE, XMFLOAT4>> renderableSpheres;
std::vector<pair<CAPSULE, XMFLOAT4>> renderableCapsules;
std::vector<RenderableLine> renderableLines;
std::vector<RenderableLine2D> renderableLines2D;
std::vector<RenderablePoint> renderablePoints;
std::vector<RenderableTriangle> renderableTriangles_solid;
std::vector<RenderableTriangle> renderableTriangles_wireframe;
std::vector<PaintRadius> paintrads;

XMFLOAT4 waterPlane = XMFLOAT4(0, 1, 0, 0);

wiSpinLock deferredMIPGenLock;
std::vector<std::pair<std::shared_ptr<wiResource>, bool>> deferredMIPGens;

wiGPUBVH sceneBVH;


static const int atlasClampBorder = 1;

static Texture decalAtlas;
static unordered_map<std::shared_ptr<wiResource>, wiRectPacker::rect_xywh> packedDecals;

Texture globalLightmap;
unordered_map<const void*, wiRectPacker::rect_xywh> packedLightmaps;




void SetDevice(std::shared_ptr<GraphicsDevice> newDevice)
{
	device = newDevice;
}
GraphicsDevice* GetDevice()
{
	return device.get();
}

// Direct reference to a renderable instance:
struct RenderBatch
{
	uint32_t hash;
	uint32_t instance;
	float distance;

	inline void Create(size_t meshIndex, size_t instanceIndex, float _distance)
	{
		hash = 0;

		assert(meshIndex < 0x00FFFFFF);
		hash |= (uint32_t)(meshIndex & 0x00FFFFFF);
		hash |= ((uint32_t)_distance & 0xFF) << 24;

		instance = (uint32_t)instanceIndex;
		distance = _distance;
	}

	inline uint32_t GetMeshIndex() const
	{
		return hash & 0x00FFFFFF;
	}
	inline uint32_t GetInstanceIndex() const
	{
		return instance;
	}
	inline float GetDistance() const
	{
		return distance;
	}
};

// This is just a utility that points to a linear array of render batches:
struct RenderQueue
{
	const CameraComponent* camera = nullptr;
	RenderBatch* batchArray = nullptr;
	uint32_t batchCount = 0;

	enum RenderQueueSortType
	{
		SORT_FRONT_TO_BACK,
		SORT_BACK_TO_FRONT,
	};

	inline bool empty() const { return batchArray == nullptr || batchCount == 0; }
	inline void add(RenderBatch* item) 
	{ 
		assert(item != nullptr); 
		if (empty())
		{
			batchArray = item;
		}
		batchCount++; 
	}
	inline void sort(RenderQueueSortType sortType = SORT_FRONT_TO_BACK)
	{
		if (batchCount > 1)
		{
			std::sort(batchArray, batchArray + batchCount, [sortType](const RenderBatch& a, const RenderBatch& b) -> bool {
				return ((sortType == SORT_FRONT_TO_BACK) ? (a.hash < b.hash) : (a.hash > b.hash));
			});

			//for (size_t i = 0; i < batchCount - 1; ++i)
			//{
			//	for (size_t j = i + 1; j < batchCount; ++j)
			//	{
			//		bool swap = false;
			//		swap = sortType == SORT_FRONT_TO_BACK && batchArray[i].hash > batchArray[j].hash;
			//		swap = sortType == SORT_BACK_TO_FRONT && batchArray[i].hash < batchArray[j].hash;

			//		if (swap)
			//		{
			//			RenderBatch tmp = batchArray[i];
			//			batchArray[i] = batchArray[j];
			//			batchArray[j] = tmp;
			//		}
			//	}
			//}
		}
	}
};

// This is a storage for component indices inside the camera frustum. These can directly index the corresponding ComponentManagers:
struct FrameCulling
{
	Frustum frustum;
	vector<uint32_t> culledObjects;
	vector<uint32_t> culledLights;
	vector<uint32_t> culledDecals;
	vector<uint32_t> culledEnvProbes;
	vector<uint32_t> culledEmitters;
	vector<uint32_t> culledHairs;

	atomic<uint32_t> object_counter;
	atomic<uint32_t> light_counter;
	atomic<uint32_t> decal_counter;

	void Clear()
	{
		culledObjects.clear();
		culledLights.clear();
		culledDecals.clear();
		culledEnvProbes.clear();
		culledEmitters.clear();
		culledHairs.clear();

		object_counter.store(0);
		light_counter.store(0);
		decal_counter.store(0);
	}
};
unordered_map<const CameraComponent*, FrameCulling> frameCullings;

vector<size_t> pendingMaterialUpdates;
vector<size_t> pendingMorphUpdates;

enum AS_UPDATE_TYPE
{
	AS_COMPLETE,
	AS_REBUILD,
	AS_UPDATE,
};
unordered_map<Entity, AS_UPDATE_TYPE> pendingBottomLevelBuilds;

struct Instance
{
	XMFLOAT4 mat0;
	XMFLOAT4 mat1;
	XMFLOAT4 mat2;
	XMUINT4 userdata;

	inline void Create(const XMFLOAT4X4& matIn, const XMFLOAT4& colorIn = XMFLOAT4(1, 1, 1, 1), float dither = 0, uint32_t subInstance = 0) volatile
	{
		mat0.x = matIn._11;
		mat0.y = matIn._21;
		mat0.z = matIn._31;
		mat0.w = matIn._41;

		mat1.x = matIn._12;
		mat1.y = matIn._22;
		mat1.z = matIn._32;
		mat1.w = matIn._42;

		mat2.x = matIn._13;
		mat2.y = matIn._23;
		mat2.z = matIn._33;
		mat2.w = matIn._43;

		XMFLOAT4 color = colorIn;
		color.w *= 1 - dither;
		userdata.x = wiMath::CompressColor(color);
		userdata.y = subInstance;

		userdata.z = 0;
		userdata.w = 0;
	}
};
struct InstancePrev
{
	XMFLOAT4 mat0;
	XMFLOAT4 mat1;
	XMFLOAT4 mat2;

	inline void Create(const XMFLOAT4X4& matIn) volatile
	{
		mat0.x = matIn._11;
		mat0.y = matIn._21;
		mat0.z = matIn._31;
		mat0.w = matIn._41;

		mat1.x = matIn._12;
		mat1.y = matIn._22;
		mat1.z = matIn._32;
		mat1.w = matIn._42;

		mat2.x = matIn._13;
		mat2.y = matIn._23;
		mat2.z = matIn._33;
		mat2.w = matIn._43;
	}
};
struct InstanceAtlas
{
	XMFLOAT4 atlasMulAdd;

	InstanceAtlas(){}
	InstanceAtlas(const XMFLOAT4& atlasRemap)
	{
		Create(atlasRemap);
	}
	inline void Create(const XMFLOAT4& atlasRemap) volatile
	{
		atlasMulAdd.x = atlasRemap.x;
		atlasMulAdd.y = atlasRemap.y;
		atlasMulAdd.z = atlasRemap.z;
		atlasMulAdd.w = atlasRemap.w;
	}
};


const Sampler* GetSampler(int slot)
{
	return &samplers[slot];
}
const Shader* GetShader(SHADERTYPE id)
{
	return &shaders[id];
}
const InputLayout* GetInputLayout(ILTYPES id)
{
	return &inputLayouts[id];
}
const RasterizerState* GetRasterizerState(RSTYPES id)
{
	return &rasterizers[id];
}
const DepthStencilState* GetDepthStencilState(DSSTYPES id)
{
	return &depthStencils[id];
}
const BlendState* GetBlendState(BSTYPES id)
{
	return &blendStates[id];
}
const GPUBuffer* GetConstantBuffer(CBTYPES id)
{
	return &constantBuffers[id];
}
const Texture* GetTexture(TEXTYPES id)
{
	return &textures[id];
}


enum OBJECTRENDERING_DOUBLESIDED
{
	OBJECTRENDERING_DOUBLESIDED_DISABLED,
	OBJECTRENDERING_DOUBLESIDED_ENABLED,
	OBJECTRENDERING_DOUBLESIDED_COUNT
};
enum OBJECTRENDERING_TESSELLATION
{
	OBJECTRENDERING_TESSELLATION_DISABLED,
	OBJECTRENDERING_TESSELLATION_ENABLED,
	OBJECTRENDERING_TESSELLATION_COUNT
};
enum OBJECTRENDERING_ALPHATEST
{
	OBJECTRENDERING_ALPHATEST_DISABLED,
	OBJECTRENDERING_ALPHATEST_ENABLED,
	OBJECTRENDERING_ALPHATEST_COUNT
};
PipelineState PSO_object
	[MaterialComponent::SHADERTYPE_COUNT]
	[RENDERPASS_COUNT]
	[BLENDMODE_COUNT]
	[OBJECTRENDERING_DOUBLESIDED_COUNT]
	[OBJECTRENDERING_TESSELLATION_COUNT]
	[OBJECTRENDERING_ALPHATEST_COUNT];
PipelineState PSO_object_terrain[RENDERPASS_COUNT];
PipelineState PSO_object_wire;

std::vector<CustomShader> customShaders;
int RegisterCustomShader(const CustomShader& customShader)
{
	int result = (int)customShaders.size();
	customShaders.push_back(customShader);
	return result;
}
const std::vector<CustomShader>& GetCustomShaders()
{
	return customShaders;
}

inline const PipelineState* GetObjectPSO(
	RENDERPASS renderPass,
	uint32_t renderTypeFlags,
	const MeshComponent& mesh,
	const MaterialComponent& material,
	bool tessellation,
	bool forceAlphaTestForDithering
)
{
	if (IsWireRender())
	{
		switch (renderPass)
		{
		case RENDERPASS_TEXTURE:
		case RENDERPASS_MAIN:
			return &PSO_object_wire;
		}
		return nullptr;
	}

	if (mesh.IsTerrain())
	{
		return &PSO_object_terrain[renderPass];
	}


	if (material.customShaderID >= 0 && material.customShaderID < (int)customShaders.size())
	{
		const CustomShader& customShader = customShaders[material.customShaderID];
		if (renderTypeFlags & customShader.renderTypeFlags)
		{
			return &customShader.pso[renderPass];
		}
		else
		{
			return nullptr;
		}
	}

	const BLENDMODE blendMode = material.GetBlendMode();
	const bool doublesided = mesh.IsDoubleSided();
	const bool alphatest = material.IsAlphaTestEnabled() || forceAlphaTestForDithering;

	const PipelineState& pso = PSO_object[material.shaderType][renderPass][blendMode][doublesided][tessellation][alphatest];
	assert(pso.IsValid());
	return &pso;
}

ILTYPES GetILTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest, bool transparent)
{
	ILTYPES realVL = ILTYPE_OBJECT_POS_TEX;

	switch (renderPass)
	{
	case RENDERPASS_TEXTURE:
		if (tessellation)
		{
			realVL = ILTYPE_OBJECT_ALL;
		}
		else
		{
			realVL = ILTYPE_OBJECT_POS_TEX;
		}
		break;
	case RENDERPASS_MAIN:
	case RENDERPASS_ENVMAPCAPTURE:
	case RENDERPASS_VOXELIZE:
		realVL = ILTYPE_OBJECT_ALL;
		break;
	case RENDERPASS_DEPTHONLY:
		if (tessellation)
		{
			realVL = ILTYPE_OBJECT_ALL;
		}
		else
		{
			if (alphatest)
			{
				realVL = ILTYPE_OBJECT_POS_TEX;
			}
			else
			{
				realVL = ILTYPE_OBJECT_POS;
			}
		}
		break;
	case RENDERPASS_SHADOW:
	case RENDERPASS_SHADOWCUBE:
		if (alphatest || transparent)
		{
			realVL = ILTYPE_OBJECT_POS_TEX;
		}
		else
		{
			realVL = ILTYPE_OBJECT_POS;
		}
		break;
	}

	return realVL;
}
SHADERTYPE GetVSTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest, bool transparent)
{
    SHADERTYPE realVS = VSTYPE_OBJECT_SIMPLE;

	switch (renderPass)
	{
	case RENDERPASS_TEXTURE:
		if (tessellation)
		{
			realVS = VSTYPE_OBJECT_SIMPLE_TESSELLATION;
		}
		else
		{
			realVS = VSTYPE_OBJECT_SIMPLE;
		}
		break;
	case RENDERPASS_MAIN:
		if (tessellation)
		{
			realVS = VSTYPE_OBJECT_COMMON_TESSELLATION;
		}
		else
		{
			realVS = VSTYPE_OBJECT_COMMON;
		}
		break;
	case RENDERPASS_DEPTHONLY:
		if (tessellation)
		{
			realVS = VSTYPE_OBJECT_SIMPLE_TESSELLATION;
		}
		else
		{
			if (alphatest)
			{
				realVS = VSTYPE_OBJECT_SIMPLE;
			}
			else
			{
				realVS = VSTYPE_OBJECT_POSITIONSTREAM;
			}
		}
		break;
	case RENDERPASS_ENVMAPCAPTURE:
		realVS = VSTYPE_ENVMAP;
		break;
	case RENDERPASS_SHADOW:
		if (transparent)
		{
			realVS = VSTYPE_SHADOW_TRANSPARENT;
		}
		else
		{
			if (alphatest)
			{
				realVS = VSTYPE_SHADOW_ALPHATEST;
			}
			else
			{
				realVS = VSTYPE_SHADOW;
			}
		}
		break;
	case RENDERPASS_SHADOWCUBE:
		if (alphatest)
		{
			realVS = VSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST;
		}
		else
		{
			realVS = VSTYPE_SHADOWCUBEMAPRENDER;
		}
		break;
	case RENDERPASS_VOXELIZE:
		realVS = VSTYPE_VOXELIZER;
		break;
	}

	return realVS;
}
SHADERTYPE GetGSTYPE(RENDERPASS renderPass, bool alphatest)
{
    SHADERTYPE realGS = SHADERTYPE_COUNT;

	switch (renderPass)
	{
	case RENDERPASS_VOXELIZE:
		realGS = GSTYPE_VOXELIZER;
		break;
	case RENDERPASS_ENVMAPCAPTURE:
		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS))
			break;
		realGS = GSTYPE_ENVMAP_EMULATION;
		break;
	case RENDERPASS_SHADOWCUBE:
		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS))
			break;
		if (alphatest)
		{
			realGS = GSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST_EMULATION;
		}
		else
		{
			realGS = GSTYPE_SHADOWCUBEMAPRENDER_EMULATION;
		}
		break;
	}

	return realGS;
}
SHADERTYPE GetHSTYPE(RENDERPASS renderPass, bool tessellation)
{
	switch (renderPass)
	{
	case RENDERPASS_TEXTURE:
	case RENDERPASS_DEPTHONLY:
	case RENDERPASS_MAIN:
            return tessellation ? HSTYPE_OBJECT : SHADERTYPE_COUNT;
		break;
	}

	return SHADERTYPE_COUNT;
}
SHADERTYPE GetDSTYPE(RENDERPASS renderPass, bool tessellation)
{
	switch (renderPass)
	{
	case RENDERPASS_TEXTURE:
	case RENDERPASS_DEPTHONLY:
	case RENDERPASS_MAIN:
            return tessellation ? DSTYPE_OBJECT : SHADERTYPE_COUNT;
		break;
	}

	return SHADERTYPE_COUNT;
}
SHADERTYPE GetPSTYPE(RENDERPASS renderPass, bool alphatest, bool transparent, MaterialComponent::SHADERTYPE shaderType)
{
    SHADERTYPE realPS = SHADERTYPE_COUNT;

	switch (renderPass)
	{
	case RENDERPASS_TEXTURE:
		realPS = PSTYPE_OBJECT_TEXTUREONLY;
		break;
	case RENDERPASS_MAIN:
		switch (shaderType)
		{
		case wiScene::MaterialComponent::SHADERTYPE_PBR:
			realPS = transparent ? PSTYPE_OBJECT_TRANSPARENT : PSTYPE_OBJECT;
			break;
		case wiScene::MaterialComponent::SHADERTYPE_PBR_PLANARREFLECTION:
			realPS = transparent ? PSTYPE_OBJECT_TRANSPARENT_PLANARREFLECTION : PSTYPE_OBJECT_PLANARREFLECTION;
			break;
		case wiScene::MaterialComponent::SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING:
			realPS = transparent ? PSTYPE_OBJECT_TRANSPARENT_POM : PSTYPE_OBJECT_POM;
			break;
		case wiScene::MaterialComponent::SHADERTYPE_PBR_ANISOTROPIC:
			realPS = transparent ? PSTYPE_OBJECT_TRANSPARENT_ANISOTROPIC : PSTYPE_OBJECT_ANISOTROPIC;
			break;
		case wiScene::MaterialComponent::SHADERTYPE_WATER:
			realPS = transparent ? PSTYPE_OBJECT_WATER : SHADERTYPE_COUNT;
			break;
		case wiScene::MaterialComponent::SHADERTYPE_CARTOON:
			realPS = transparent ? PSTYPE_OBJECT_TRANSPARENT_CARTOON : PSTYPE_OBJECT_CARTOON;
			break;
		case wiScene::MaterialComponent::SHADERTYPE_UNLIT:
			realPS = transparent ? PSTYPE_OBJECT_TRANSPARENT_UNLIT : PSTYPE_OBJECT_UNLIT;
			break;
		default:
			break;
		}
		break;
	case RENDERPASS_DEPTHONLY:
		if (alphatest)
		{
			realPS = PSTYPE_OBJECT_ALPHATESTONLY;
		}
		else
		{
			realPS = SHADERTYPE_COUNT;
		}
		break;
	case RENDERPASS_ENVMAPCAPTURE:
		realPS = PSTYPE_ENVMAP;
		break;
	case RENDERPASS_SHADOW:
		if (transparent)
		{
			realPS = PSTYPE_SHADOW_TRANSPARENT;
		}
		else
		{
			if (alphatest)
			{
				realPS = PSTYPE_SHADOW_ALPHATEST;
			}
			else
			{
				realPS = SHADERTYPE_COUNT;
			}
		}
		break;
	case RENDERPASS_SHADOWCUBE:
		if (alphatest)
		{
			realPS = PSTYPE_SHADOW_ALPHATEST;
		}
		else
		{
			realPS = SHADERTYPE_COUNT;
		}
		break;
	case RENDERPASS_VOXELIZE:
		realPS = PSTYPE_VOXELIZER;
		break;
	default:
		break;
	}

	return realPS;
}

PipelineState PSO_decal;
PipelineState PSO_occlusionquery;
PipelineState PSO_impostor[RENDERPASS_COUNT];
PipelineState PSO_impostor_wire;
PipelineState PSO_captureimpostor_albedo;
PipelineState PSO_captureimpostor_normal;
PipelineState PSO_captureimpostor_surface;
inline const PipelineState* GetImpostorPSO(RENDERPASS renderPass)
{
	if (IsWireRender())
	{
		switch (renderPass)
		{
		case RENDERPASS_TEXTURE:
		case RENDERPASS_MAIN:
			return &PSO_impostor_wire;
		}
		return nullptr;
	}

	return &PSO_impostor[renderPass];
}

PipelineState PSO_lightvisualizer[LightComponent::LIGHTTYPE_COUNT];
PipelineState PSO_volumetriclight[LightComponent::LIGHTTYPE_COUNT];

PipelineState PSO_renderlightmap;

PipelineState PSO_lensflare;

PipelineState PSO_downsampledepthbuffer;
PipelineState PSO_deferredcomposition;
PipelineState PSO_sss_skin;
PipelineState PSO_sss_snow;
PipelineState PSO_upsample_bilateral;
PipelineState PSO_outline;

enum SKYRENDERING
{
	SKYRENDERING_STATIC,
	SKYRENDERING_DYNAMIC,
	SKYRENDERING_SUN,
	SKYRENDERING_ENVMAPCAPTURE_STATIC,
	SKYRENDERING_ENVMAPCAPTURE_DYNAMIC,
	SKYRENDERING_COUNT
};
PipelineState PSO_sky[SKYRENDERING_COUNT];

enum DEBUGRENDERING
{
	DEBUGRENDERING_ENVPROBE,
	DEBUGRENDERING_GRID,
	DEBUGRENDERING_CUBE,
	DEBUGRENDERING_LINES,
	DEBUGRENDERING_TRIANGLE_SOLID,
	DEBUGRENDERING_TRIANGLE_WIREFRAME,
	DEBUGRENDERING_EMITTER,
	DEBUGRENDERING_PAINTRADIUS,
	DEBUGRENDERING_VOXEL,
	DEBUGRENDERING_FORCEFIELD_POINT,
	DEBUGRENDERING_FORCEFIELD_PLANE,
	DEBUGRENDERING_RAYTRACE_BVH,
	DEBUGRENDERING_COUNT
};
PipelineState PSO_debug[DEBUGRENDERING_COUNT];


bool LoadShader(SHADERSTAGE stage, Shader& shader, const std::string& filename)
{
	vector<uint8_t> buffer;
	if (wiHelper::FileRead(SHADERPATH + filename, buffer)) 
	{
		return device->CreateShader(stage, buffer.data(), buffer.size(), &shader);
	}
	wiHelper::messageBox("Shader not found: " + SHADERPATH + filename);
	return false;
}


void LoadShaders()
{
	wiJobSystem::context ctx;

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		InputLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, 0, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		};
		LoadShader(VS, shaders[VSTYPE_OBJECT_DEBUG], "objectVS_debug.cso");
		device->CreateInputLayout(layout, arraysize(layout), &shaders[VSTYPE_OBJECT_DEBUG], &inputLayouts[ILTYPE_OBJECT_DEBUG]);
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		InputLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, 0, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					0, MeshComponent::Vertex_TEX::FORMAT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					1, MeshComponent::Vertex_TEX::FORMAT, 2, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "ATLAS",					0, MeshComponent::Vertex_TEX::FORMAT, 3, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",					0, MeshComponent::Vertex_COL::FORMAT, 4, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT",				0, MeshComponent::Vertex_TAN::FORMAT, 5, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "PREVPOS",				0, MeshComponent::Vertex_POS::FORMAT, 6, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, 7, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, 7, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, 7, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEUSERDATA",		0, FORMAT_R32G32B32A32_UINT,  7, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIXPREV",		0, FORMAT_R32G32B32A32_FLOAT, 7, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIXPREV",		1, FORMAT_R32G32B32A32_FLOAT, 7, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIXPREV",		2, FORMAT_R32G32B32A32_FLOAT, 7, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEATLAS",			0, FORMAT_R32G32B32A32_FLOAT, 7, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		LoadShader(VS, shaders[VSTYPE_OBJECT_COMMON], "objectVS_common.cso");
		device->CreateInputLayout(layout, arraysize(layout), &shaders[VSTYPE_OBJECT_COMMON], &inputLayouts[ILTYPE_OBJECT_ALL]);
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		InputLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, 0, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEUSERDATA",		0, FORMAT_R32G32B32A32_UINT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		LoadShader(VS, shaders[VSTYPE_OBJECT_POSITIONSTREAM], "objectVS_positionstream.cso");
		device->CreateInputLayout(layout, arraysize(layout), &shaders[VSTYPE_OBJECT_POSITIONSTREAM], &inputLayouts[ILTYPE_OBJECT_POS]);

		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		InputLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, 0, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					0, MeshComponent::Vertex_TEX::FORMAT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					1, MeshComponent::Vertex_TEX::FORMAT, 2, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, 3, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, 3, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, 3, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEUSERDATA",		0, FORMAT_R32G32B32A32_UINT, 3, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		LoadShader(VS, shaders[VSTYPE_OBJECT_SIMPLE], "objectVS_simple.cso");
		device->CreateInputLayout(layout, arraysize(layout), &shaders[VSTYPE_OBJECT_SIMPLE], &inputLayouts[ILTYPE_OBJECT_POS_TEX]);

		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		InputLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, 0, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEUSERDATA",		0, FORMAT_R32G32B32A32_UINT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		LoadShader(VS, shaders[VSTYPE_SHADOW], "shadowVS.cso");
		device->CreateInputLayout(layout, arraysize(layout), &shaders[VSTYPE_SHADOW], &inputLayouts[ILTYPE_SHADOW_POS]);

		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		InputLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, 0, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					0, MeshComponent::Vertex_TEX::FORMAT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					1, MeshComponent::Vertex_TEX::FORMAT, 2, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, 3, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, 3, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, 3, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEUSERDATA",		0, FORMAT_R32G32B32A32_UINT, 3, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		LoadShader(VS, shaders[VSTYPE_SHADOW_ALPHATEST], "shadowVS_alphatest.cso");
		device->CreateInputLayout(layout, arraysize(layout), &shaders[VSTYPE_SHADOW_ALPHATEST], &inputLayouts[ILTYPE_SHADOW_POS_TEX]);

		LoadShader(VS, shaders[VSTYPE_SHADOW_TRANSPARENT], "shadowVS_transparent.cso");
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		InputLayoutDesc layout[] =
		{
			{ "POSITION", 0, FORMAT_R32G32B32A32_FLOAT, 0, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, FORMAT_R32G32B32A32_FLOAT, 0, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		};
		LoadShader(VS, shaders[VSTYPE_VERTEXCOLOR], "vertexcolorVS.cso");
		device->CreateInputLayout(layout, arraysize(layout), &shaders[VSTYPE_VERTEXCOLOR], &inputLayouts[ILTYPE_VERTEXCOLOR]);

		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		InputLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",		0, MeshComponent::Vertex_POS::FORMAT, 0, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "ATLAS",						0, MeshComponent::Vertex_TEX::FORMAT, 1, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIXPREV",			0, FORMAT_R32G32B32A32_FLOAT, 2, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIXPREV",			1, FORMAT_R32G32B32A32_FLOAT, 2, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIXPREV",			2, FORMAT_R32G32B32A32_FLOAT, 2, InputLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		LoadShader(VS, shaders[VSTYPE_RENDERLIGHTMAP], "renderlightmapVS.cso");
		device->CreateInputLayout(layout, arraysize(layout), &shaders[VSTYPE_RENDERLIGHTMAP], &inputLayouts[ILTYPE_RENDERLIGHTMAP]);
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_OBJECT_COMMON_TESSELLATION], "objectVS_common_tessellation.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_OBJECT_SIMPLE_TESSELLATION], "objectVS_simple_tessellation.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_IMPOSTOR], "impostorVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_VOLUMETRICLIGHT_DIRECTIONAL], "volumetriclight_directionalVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_VOLUMETRICLIGHT_POINT], "volumetriclight_pointVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_VOLUMETRICLIGHT_SPOT], "volumetriclight_spotVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_LIGHTVISUALIZER_SPOTLIGHT], "vSpotLightVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_LIGHTVISUALIZER_POINTLIGHT], "vPointLightVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_LIGHTVISUALIZER_SPHERELIGHT], "vSphereLightVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_LIGHTVISUALIZER_DISCLIGHT], "vDiscLightVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_LIGHTVISUALIZER_RECTANGLELIGHT], "vRectangleLightVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_LIGHTVISUALIZER_TUBELIGHT], "vTubeLightVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SPHERE], "sphereVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_CUBE], "cubeVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SKY], "skyVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_VOXELIZER], "objectVS_voxelizer.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_VOXEL], "voxelVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_FORCEFIELDVISUALIZER_POINT], "forceFieldPointVisualizerVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_FORCEFIELDVISUALIZER_PLANE], "forceFieldPlaneVisualizerVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_RAYTRACE_SCREEN], "raytrace_screenVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SCREEN], "screenVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_LENSFLARE], "lensFlareVS.cso"); });

	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS))
	{
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_ENVMAP], "envMapVS.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_ENVMAP_SKY], "envMap_skyVS.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SHADOWCUBEMAPRENDER], "cubeShadowVS.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST], "cubeShadowVS_alphatest.cso"); });
	}
	else
	{
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_ENVMAP], "envMapVS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_ENVMAP_SKY], "envMap_skyVS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SHADOWCUBEMAPRENDER], "cubeShadowVS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST], "cubeShadowVS_alphatest_emulation.cso"); });

		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_ENVMAP_EMULATION], "envMapGS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_ENVMAP_SKY_EMULATION], "envMap_skyGS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_SHADOWCUBEMAPRENDER_EMULATION], "cubeShadowGS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST_EMULATION], "cubeShadowGS_alphatest_emulation.cso"); });
	}

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT], "objectPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT], "objectPS_transparent.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_PLANARREFLECTION], "objectPS_planarreflection.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_PLANARREFLECTION], "objectPS_transparent_planarreflection.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_POM], "objectPS_pom.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_POM], "objectPS_transparent_pom.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_ANISOTROPIC], "objectPS_anisotropic.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_ANISOTROPIC], "objectPS_transparent_anisotropic.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_CARTOON], "objectPS_cartoon.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_CARTOON], "objectPS_transparent_cartoon.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_UNLIT], "objectPS_unlit.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_UNLIT], "objectPS_transparent_unlit.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_WATER], "objectPS_water.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TERRAIN], "objectPS_terrain.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_IMPOSTOR], "impostorPS.cso"); });

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_HOLOGRAM], "objectPS_hologram.cso"); });

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_DEBUG], "objectPS_debug.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_PAINTRADIUS], "objectPS_paintradius.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_SIMPLEST], "objectPS_simplest.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_BLACKOUT], "objectPS_blackout.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TEXTUREONLY], "objectPS_textureonly.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_ALPHATESTONLY], "objectPS_alphatestonly.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_IMPOSTOR_ALPHATESTONLY], "impostorPS_alphatestonly.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_IMPOSTOR_SIMPLE], "impostorPS_simple.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_IMPOSTOR_WIRE], "impostorPS_wire.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_LIGHTVISUALIZER], "lightVisualizerPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_VOLUMETRICLIGHT_DIRECTIONAL], "volumetricLight_DirectionalPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_VOLUMETRICLIGHT_POINT], "volumetricLight_PointPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_VOLUMETRICLIGHT_SPOT], "volumetricLight_SpotPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_ENVMAP], "envMapPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_ENVMAP_TERRAIN], "envMapPS_terrain.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_ENVMAP_SKY_STATIC], "envMap_skyPS_static.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_ENVMAP_SKY_DYNAMIC], "envMap_skyPS_dynamic.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_CAPTUREIMPOSTOR_ALBEDO], "captureImpostorPS_albedo.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_CAPTUREIMPOSTOR_NORMAL], "captureImpostorPS_normal.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_CAPTUREIMPOSTOR_SURFACE], "captureImpostorPS_surface.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_CUBEMAP], "cubeMapPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_VERTEXCOLOR], "vertexcolorPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SKY_STATIC], "skyPS_static.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SKY_DYNAMIC], "skyPS_dynamic.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SUN], "sunPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SHADOW_ALPHATEST], "shadowPS_alphatest.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SHADOW_TRANSPARENT], "shadowPS_transparent.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SHADOW_WATER], "shadowPS_water.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_VOXELIZER], "objectPS_voxelizer.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_VOXELIZER_TERRAIN], "objectPS_voxelizer_terrain.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_VOXEL], "voxelPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_FORCEFIELDVISUALIZER], "forceFieldVisualizerPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_RENDERLIGHTMAP], "renderlightmapPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_RAYTRACE_DEBUGBVH], "raytrace_debugbvhPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_DOWNSAMPLEDEPTHBUFFER], "downsampleDepthBuffer4xPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_DEFERREDCOMPOSITION], "deferredPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_POSTPROCESS_SSS_SKIN], "sssPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_POSTPROCESS_SSS_SNOW], "sssPS_snow.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL], "upsample_bilateralPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_POSTPROCESS_OUTLINE], "outlinePS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_LENSFLARE], "lensFlarePS.cso"); });

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_VOXELIZER], "objectGS_voxelizer.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_VOXEL], "voxelGS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_LENSFLARE], "lensFlareGS.cso"); });


	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_LUMINANCE_PASS1], "luminancePass1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_LUMINANCE_PASS2], "luminancePass2CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_SHADINGRATECLASSIFICATION], "shadingRateClassificationCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_SHADINGRATECLASSIFICATION_DEBUG], "shadingRateClassificationCS_DEBUG.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_TILEFRUSTUMS], "tileFrustumsCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_LIGHTCULLING], "lightCullingCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_LIGHTCULLING_DEBUG], "lightCullingCS_DEBUG.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_LIGHTCULLING_ADVANCED], "lightCullingCS_ADVANCED.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_LIGHTCULLING_ADVANCED_DEBUG], "lightCullingCS_ADVANCED_DEBUG.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_RESOLVEMSAADEPTHSTENCIL], "resolveMSAADepthStencilCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_VOXELSCENECOPYCLEAR], "voxelSceneCopyClearCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_VOXELSCENECOPYCLEAR_TEMPORALSMOOTHING], "voxelSceneCopyClearCS_TemporalSmoothing.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_VOXELRADIANCESECONDARYBOUNCE], "voxelRadianceSecondaryBounceCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_VOXELCLEARONLYNORMAL], "voxelClearOnlyNormalCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_SKYATMOSPHERE_TRANSMITTANCELUT], "skyAtmosphere_transmittanceLutCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], "skyAtmosphere_multiScatteredLuminanceLutCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_SKYATMOSPHERE_SKYVIEWLUT], "skyAtmosphere_skyViewLutCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_GENERATEMIPCHAIN2D_UNORM4], "generateMIPChain2DCS_unorm4.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_GENERATEMIPCHAIN2D_FLOAT4], "generateMIPChain2DCS_float4.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_GENERATEMIPCHAIN3D_UNORM4], "generateMIPChain3DCS_unorm4.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_GENERATEMIPCHAIN3D_FLOAT4], "generateMIPChain3DCS_float4.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_GENERATEMIPCHAINCUBE_UNORM4], "generateMIPChainCubeCS_unorm4.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_GENERATEMIPCHAINCUBE_FLOAT4], "generateMIPChainCubeCS_float4.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_GENERATEMIPCHAINCUBEARRAY_UNORM4], "generateMIPChainCubeArrayCS_unorm4.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_GENERATEMIPCHAINCUBEARRAY_FLOAT4], "generateMIPChainCubeArrayCS_float4.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_FILTERENVMAP], "filterEnvMapCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_COPYTEXTURE2D_UNORM4], "copytexture2D_unorm4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_COPYTEXTURE2D_FLOAT4], "copytexture2D_float4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_COPYTEXTURE2D_UNORM4_BORDEREXPAND], "copytexture2D_unorm4_borderexpandCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_COPYTEXTURE2D_FLOAT4_BORDEREXPAND], "copytexture2D_float4_borderexpandCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_SKINNING], "skinningCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_SKINNING_LDS], "skinningCS_LDS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_RAYTRACE_LAUNCH], "raytrace_launchCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_RAYTRACE_KICKJOBS], "raytrace_kickjobsCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_RAYTRACE_CLOSESTHIT], "raytrace_closesthitCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_RAYTRACE_SHADE], "raytrace_shadeCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_RAYTRACE_TILESORT], "raytrace_tilesortCS.cso"); });

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_PAINT_TEXTURE], "paint_textureCS.cso"); });

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT1], "blur_gaussian_float1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT3], "blur_gaussian_float3CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT4], "blur_gaussian_float4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_UNORM1], "blur_gaussian_unorm1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_UNORM4], "blur_gaussian_unorm4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT1], "blur_gaussian_wide_float1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT3], "blur_gaussian_wide_float3CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT4], "blur_gaussian_wide_float4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_UNORM1], "blur_gaussian_wide_unorm1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_UNORM4], "blur_gaussian_wide_unorm4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT1], "blur_bilateral_float1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT3], "blur_bilateral_float3CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT4], "blur_bilateral_float4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_UNORM1], "blur_bilateral_unorm1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_UNORM4], "blur_bilateral_unorm4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT1], "blur_bilateral_wide_float1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT3], "blur_bilateral_wide_float3CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT4], "blur_bilateral_wide_float4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_UNORM1], "blur_bilateral_wide_unorm1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_UNORM4], "blur_bilateral_wide_unorm4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_SSAO], "ssaoCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_HBAO], "hbaoCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MSAO_PREPAREDEPTHBUFFERS1], "msao_preparedepthbuffers1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MSAO_PREPAREDEPTHBUFFERS2], "msao_preparedepthbuffers2CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MSAO_INTERLEAVE], "msao_interleaveCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MSAO], "msaoCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE], "msao_blurupsampleCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_BLENDOUT], "msao_blurupsampleCS_blendout.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_PREMIN], "msao_blurupsampleCS_premin.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_PREMIN_BLENDOUT], "msao_blurupsampleCS_premin_blendout.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_SSR_RAYTRACE], "ssr_raytraceCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_SSR_RESOLVE], "ssr_resolveCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_SSR_TEMPORAL], "ssr_temporalCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_SSR_MEDIAN], "ssr_medianCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_LIGHTSHAFTS], "lightShaftsCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_TILEMAXCOC_HORIZONTAL], "depthoffield_tileMaxCOC_horizontalCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_TILEMAXCOC_VERTICAL], "depthoffield_tileMaxCOC_verticalCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_NEIGHBORHOODMAXCOC], "depthoffield_neighborhoodMaxCOCCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_KICKJOBS], "depthoffield_kickjobsCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS], "depthoffield_prepassCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS_EARLYEXIT], "depthoffield_prepassCS_earlyexit.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN], "depthoffield_mainCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN_EARLYEXIT], "depthoffield_mainCS_earlyexit.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN_CHEAP], "depthoffield_mainCS_cheap.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_POSTFILTER], "depthoffield_postfilterCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_UPSAMPLE], "depthoffield_upsampleCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_TILEMAXVELOCITY_HORIZONTAL], "motionblur_tileMaxVelocity_horizontalCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_TILEMAXVELOCITY_VERTICAL], "motionblur_tileMaxVelocity_verticalCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_NEIGHBORHOODMAXVELOCITY], "motionblur_neighborhoodMaxVelocityCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_KICKJOBS], "motionblur_kickjobsCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR], "motionblurCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_EARLYEXIT], "motionblurCS_earlyexit.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_CHEAP], "motionblurCS_cheap.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLOOMSEPARATE], "bloomseparateCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_BLOOMCOMBINE], "bloomcombineCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_SHAPENOISE], "volumetricCloud_shapenoiseCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_DETAILNOISE], "volumetricCloud_detailnoiseCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_CURLNOISE], "volumetricCloud_curlnoiseCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_WEATHERMAP], "volumetricCloud_weathermapCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_RENDER], "volumetricCloud_renderCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_REPROJECT], "volumetricCloud_reprojectCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_FINAL], "volumetricCloud_finalCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_FXAA], "fxaaCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_TEMPORALAA], "temporalaaCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_LINEARDEPTH], "lineardepthCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_SHARPEN], "sharpenCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_TONEMAP], "tonemapCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_CHROMATIC_ABERRATION], "chromatic_aberrationCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT1], "upsample_bilateral_float1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UNORM1], "upsample_bilateral_unorm1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT4], "upsample_bilateral_float4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UNORM4], "upsample_bilateral_unorm4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DOWNSAMPLE4X], "downsample4xCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_NORMALSFROMDEPTH], "normalsfromdepthCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DENOISE], "denoiseCS.cso"); });


	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(HS, shaders[HSTYPE_OBJECT], "objectHS.cso"); });

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(DS, shaders[DSTYPE_OBJECT], "objectDS.cso"); });

	wiJobSystem::Wait(ctx);

	// default objectshaders:
	wiJobSystem::Dispatch(ctx, MaterialComponent::SHADERTYPE_COUNT, 1, [](wiJobArgs args) {
		MaterialComponent::SHADERTYPE shaderType = (MaterialComponent::SHADERTYPE)args.jobIndex;

		for (int renderPass = 0; renderPass < RENDERPASS_COUNT; ++renderPass)
		{
			for (int blendMode = 0; blendMode < BLENDMODE_COUNT; ++blendMode)
			{
				for (int doublesided = 0; doublesided < OBJECTRENDERING_DOUBLESIDED_COUNT; ++doublesided)
				{
					for (int tessellation = 0; tessellation < OBJECTRENDERING_TESSELLATION_COUNT; ++tessellation)
					{
						for (int alphatest = 0; alphatest < OBJECTRENDERING_ALPHATEST_COUNT; ++alphatest)
						{
							const bool transparency = blendMode != BLENDMODE_OPAQUE;
							SHADERTYPE realVS = GetVSTYPE((RENDERPASS)renderPass, tessellation, alphatest, transparency);
							ILTYPES realVL = GetILTYPE((RENDERPASS)renderPass, tessellation, alphatest, transparency);
							SHADERTYPE realHS = GetHSTYPE((RENDERPASS)renderPass, tessellation);
							SHADERTYPE realDS = GetDSTYPE((RENDERPASS)renderPass, tessellation);
							SHADERTYPE realGS = GetGSTYPE((RENDERPASS)renderPass, alphatest);
							SHADERTYPE realPS = GetPSTYPE((RENDERPASS)renderPass, alphatest, transparency, shaderType);

							if (tessellation && (realHS == SHADERTYPE_COUNT || realDS == SHADERTYPE_COUNT))
							{
								continue;
							}

							PipelineStateDesc desc;
							desc.il = realVL < ILTYPE_COUNT ? &inputLayouts[realVL] : nullptr;
							desc.vs = realVS < SHADERTYPE_COUNT ? &shaders[realVS] : nullptr;
							desc.hs = realHS < SHADERTYPE_COUNT ? &shaders[realHS] : nullptr;
							desc.ds = realDS < SHADERTYPE_COUNT ? &shaders[realDS] : nullptr;
							desc.gs = realGS < SHADERTYPE_COUNT ? &shaders[realGS] : nullptr;
							desc.ps = realPS < SHADERTYPE_COUNT ? &shaders[realPS] : nullptr;

							switch (blendMode)
							{
							case BLENDMODE_OPAQUE:
								desc.bs = &blendStates[BSTYPE_OPAQUE];
								break;
							case BLENDMODE_ALPHA:
								desc.bs = &blendStates[BSTYPE_TRANSPARENT];
								break;
							case BLENDMODE_ADDITIVE:
								desc.bs = &blendStates[BSTYPE_ADDITIVE];
								break;
							case BLENDMODE_PREMULTIPLIED:
								desc.bs = &blendStates[BSTYPE_PREMULTIPLIED];
								break;
							default:
								assert(0);
								break;
							}

							switch (renderPass)
							{
							case RENDERPASS_DEPTHONLY:
							case RENDERPASS_SHADOW:
							case RENDERPASS_SHADOWCUBE:
								desc.bs = &blendStates[transparency ? BSTYPE_TRANSPARENTSHADOWMAP : BSTYPE_COLORWRITEDISABLE];
								break;
							default:
								break;
							}

							switch (renderPass)
							{
							case RENDERPASS_SHADOW:
							case RENDERPASS_SHADOWCUBE:
								desc.dss = &depthStencils[transparency ? DSSTYPE_DEPTHREAD : DSSTYPE_SHADOW];
								break;
							case RENDERPASS_MAIN:
								if (blendMode == BLENDMODE_ADDITIVE)
								{
									desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
								}
								else
								{
									desc.dss = &depthStencils[transparency ? DSSTYPE_DEFAULT : DSSTYPE_DEPTHREADEQUAL];
								}
								break;
							case RENDERPASS_ENVMAPCAPTURE:
								desc.dss = &depthStencils[DSSTYPE_ENVMAP];
								break;
							case RENDERPASS_VOXELIZE:
								desc.dss = &depthStencils[DSSTYPE_XRAY];
								break;
							default:
								if (blendMode == BLENDMODE_ADDITIVE)
								{
									desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
								}
								else
								{
									desc.dss = &depthStencils[DSSTYPE_DEFAULT];
								}
								break;
							}

							switch (renderPass)
							{
							case RENDERPASS_SHADOW:
							case RENDERPASS_SHADOWCUBE:
								desc.rs = &rasterizers[doublesided ? RSTYPE_SHADOW_DOUBLESIDED : RSTYPE_SHADOW];
								break;
							case RENDERPASS_VOXELIZE:
								desc.rs = &rasterizers[RSTYPE_VOXELIZE];
								break;
							default:
								desc.rs = &rasterizers[doublesided ? RSTYPE_DOUBLESIDED : RSTYPE_FRONT];
								break;
							}

							if (tessellation)
							{
								desc.pt = PATCHLIST;
							}
							else
							{
								desc.pt = TRIANGLELIST;
							}

							device->CreatePipelineState(&desc, &PSO_object[shaderType][renderPass][blendMode][doublesided][tessellation][alphatest]);
						}
					}
				}
			}
		}
	});

	wiJobSystem::Dispatch(ctx, RENDERPASS_COUNT, 1, [](wiJobArgs args) {

		SHADERTYPE realVS = GetVSTYPE((RENDERPASS) args.jobIndex, false, false, false);
		ILTYPES realVL = GetILTYPE((RENDERPASS)args.jobIndex, false, false, false);

		PipelineStateDesc desc;
		desc.rs = &rasterizers[RSTYPE_FRONT];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		desc.il = &inputLayouts[realVL];
		desc.vs = &shaders[realVS];

		switch (args.jobIndex)
		{
		case RENDERPASS_TEXTURE:
			desc.ps = &shaders[PSTYPE_OBJECT_TEXTUREONLY]; // textureonly doesn't have worldpos or normal inputs, so it will not use terrain blending
			break;
		case RENDERPASS_MAIN:
			desc.dss = &depthStencils[DSSTYPE_DEPTHREADEQUAL];
			desc.ps = &shaders[PSTYPE_OBJECT_TERRAIN];
			break;
		case RENDERPASS_DEPTHONLY:
			desc.ps = nullptr;
			break;
		case RENDERPASS_VOXELIZE:
			desc.dss = &depthStencils[DSSTYPE_XRAY];
			desc.rs = &rasterizers[RSTYPE_VOXELIZE];
			desc.vs = &shaders[VSTYPE_VOXELIZER];
			desc.gs = &shaders[GSTYPE_VOXELIZER];
			desc.ps = &shaders[PSTYPE_VOXELIZER_TERRAIN];
			break;
		case RENDERPASS_ENVMAPCAPTURE:
			desc.ps = &shaders[PSTYPE_ENVMAP_TERRAIN];
			break;
		case RENDERPASS_SHADOW:
		case RENDERPASS_SHADOWCUBE:
			desc.dss = &depthStencils[DSSTYPE_SHADOW];
			desc.rs = &rasterizers[RSTYPE_SHADOW_DOUBLESIDED];
			desc.ps = nullptr;
			break;
		default:
			return;
		}

		device->CreatePipelineState(&desc, &PSO_object_terrain[args.jobIndex]);
	});

	// Clear custom shaders (Custom shaders coming from user will need to be handled by the user in case of shader reload):
	customShaders.clear();

	// Hologram sample shader will be registered as custom shader:
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		SHADERTYPE realVS = GetVSTYPE(RENDERPASS_MAIN, false, false, true);
		ILTYPES realVL = GetILTYPE(RENDERPASS_MAIN, false, false, true);

		PipelineStateDesc desc;
		desc.vs = &shaders[realVS];
		desc.il = &inputLayouts[realVL];
		desc.ps = &shaders[PSTYPE_OBJECT_HOLOGRAM];

		desc.bs = &blendStates[BSTYPE_ADDITIVE];
		desc.rs = &rasterizers[RSTYPE_FRONT];
		desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		desc.pt = TRIANGLELIST;

		PipelineState pso;
		device->CreatePipelineState(&desc, &pso);

		CustomShader customShader;
		customShader.name = "Hologram";
		customShader.renderTypeFlags = RENDERTYPE_TRANSPARENT;
		customShader.pso[RENDERPASS_MAIN] = pso;
		RegisterCustomShader(customShader);
		});


	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_OBJECT_SIMPLE];
		desc.ps = &shaders[PSTYPE_OBJECT_SIMPLEST];
		desc.rs = &rasterizers[RSTYPE_WIRE];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		desc.il = &inputLayouts[ILTYPE_OBJECT_POS_TEX];

		device->CreatePipelineState(&desc, &PSO_object_wire);
		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_CUBE];
		desc.rs = &rasterizers[RSTYPE_OCCLUDEE];
		desc.bs = &blendStates[BSTYPE_COLORWRITEDISABLE];
		desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
		desc.pt = TRIANGLESTRIP;

		device->CreatePipelineState(&desc, &PSO_occlusionquery);
		});
	wiJobSystem::Dispatch(ctx, RENDERPASS_COUNT, 1, [](wiJobArgs args) {
		const bool impostorRequest =
			args.jobIndex != RENDERPASS_VOXELIZE &&
			args.jobIndex != RENDERPASS_SHADOW &&
			args.jobIndex != RENDERPASS_SHADOWCUBE &&
			args.jobIndex != RENDERPASS_ENVMAPCAPTURE;
		if (!impostorRequest)
		{
			return;
		}

		PipelineStateDesc desc;
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		desc.il = nullptr;

		switch (args.jobIndex)
		{
		case RENDERPASS_MAIN:
			desc.dss = &depthStencils[DSSTYPE_DEPTHREADEQUAL];
			desc.vs = &shaders[VSTYPE_IMPOSTOR];
			desc.ps = &shaders[PSTYPE_IMPOSTOR];
			break;
		case RENDERPASS_DEPTHONLY:
			desc.vs = &shaders[VSTYPE_IMPOSTOR];
			desc.ps = &shaders[PSTYPE_IMPOSTOR_ALPHATESTONLY];
			break;
		default:
			desc.vs = &shaders[VSTYPE_IMPOSTOR];
			desc.ps = &shaders[PSTYPE_IMPOSTOR_SIMPLE];
			break;
		}

		device->CreatePipelineState(&desc, &PSO_impostor[args.jobIndex]);
		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_IMPOSTOR];
		desc.ps = &shaders[PSTYPE_IMPOSTOR_WIRE];
		desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		desc.il = nullptr;

		device->CreatePipelineState(&desc, &PSO_impostor_wire);
		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_OBJECT_COMMON];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_CAPTUREIMPOSTOR];
		desc.il = &inputLayouts[ILTYPE_OBJECT_ALL];

		desc.ps = &shaders[PSTYPE_CAPTUREIMPOSTOR_ALBEDO];
		device->CreatePipelineState(&desc, &PSO_captureimpostor_albedo);

		desc.ps = &shaders[PSTYPE_CAPTUREIMPOSTOR_NORMAL];
		device->CreatePipelineState(&desc, &PSO_captureimpostor_normal);

		desc.ps = &shaders[PSTYPE_CAPTUREIMPOSTOR_SURFACE];
		device->CreatePipelineState(&desc, &PSO_captureimpostor_surface);
		});

	wiJobSystem::Dispatch(ctx, LightComponent::LIGHTTYPE_COUNT, 1, [](wiJobArgs args) {
		PipelineStateDesc desc;

		// deferred lights:

		desc.pt = TRIANGLELIST;


		// light visualizers:
		if (args.jobIndex != LightComponent::DIRECTIONAL)
		{

			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.ps = &shaders[PSTYPE_LIGHTVISUALIZER];

			switch (args.jobIndex)
			{
			case LightComponent::POINT:
				desc.bs = &blendStates[BSTYPE_ADDITIVE];
				desc.vs = &shaders[VSTYPE_LIGHTVISUALIZER_POINTLIGHT];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				break;
			case LightComponent::SPOT:
				desc.bs = &blendStates[BSTYPE_ADDITIVE];
				desc.vs = &shaders[VSTYPE_LIGHTVISUALIZER_SPOTLIGHT];
				desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
				break;
			case LightComponent::SPHERE:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = &shaders[VSTYPE_LIGHTVISUALIZER_SPHERELIGHT];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				break;
			case LightComponent::DISC:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = &shaders[VSTYPE_LIGHTVISUALIZER_DISCLIGHT];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				break;
			case LightComponent::RECTANGLE:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = &shaders[VSTYPE_LIGHTVISUALIZER_RECTANGLELIGHT];
				desc.rs = &rasterizers[RSTYPE_BACK];
				break;
			case LightComponent::TUBE:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = &shaders[VSTYPE_LIGHTVISUALIZER_TUBELIGHT];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				break;
			}

			device->CreatePipelineState(&desc, &PSO_lightvisualizer[args.jobIndex]);
		}


		// volumetric lights:
		if (args.jobIndex <= LightComponent::SPOT)
		{
			desc.dss = &depthStencils[DSSTYPE_XRAY];
			desc.bs = &blendStates[BSTYPE_ADDITIVE];
			desc.rs = &rasterizers[RSTYPE_BACK];

			switch (args.jobIndex)
			{
			case LightComponent::DIRECTIONAL:
				desc.vs = &shaders[VSTYPE_VOLUMETRICLIGHT_DIRECTIONAL];
				desc.ps = &shaders[PSTYPE_VOLUMETRICLIGHT_DIRECTIONAL];
				break;
			case LightComponent::POINT:
				desc.vs = &shaders[VSTYPE_VOLUMETRICLIGHT_POINT];
				desc.ps = &shaders[PSTYPE_VOLUMETRICLIGHT_POINT];
				break;
			case LightComponent::SPOT:
				desc.vs = &shaders[VSTYPE_VOLUMETRICLIGHT_SPOT];
				desc.ps = &shaders[PSTYPE_VOLUMETRICLIGHT_SPOT];
				break;
			}

			device->CreatePipelineState(&desc, &PSO_volumetriclight[args.jobIndex]);
		}


		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.il = &inputLayouts[ILTYPE_RENDERLIGHTMAP];
		desc.vs = &shaders[VSTYPE_RENDERLIGHTMAP];
		desc.ps = &shaders[PSTYPE_RENDERLIGHTMAP];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_TRANSPARENT];
		desc.dss = &depthStencils[DSSTYPE_XRAY];

		device->CreatePipelineState(&desc, &PSO_renderlightmap);
		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_SCREEN];
		desc.ps = &shaders[PSTYPE_DOWNSAMPLEDEPTHBUFFER];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_WRITEONLY];

		device->CreatePipelineState(&desc, &PSO_downsampledepthbuffer);
		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_SCREEN];
		desc.ps = &shaders[PSTYPE_DEFERREDCOMPOSITION];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_DEFERREDCOMPOSITION];

		device->CreatePipelineState(&desc, &PSO_deferredcomposition);
		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_SCREEN];
		desc.ps = &shaders[PSTYPE_POSTPROCESS_SSS_SKIN];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_SSS];

		device->CreatePipelineState(&desc, &PSO_sss_skin);
		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_SCREEN];
		desc.ps = &shaders[PSTYPE_POSTPROCESS_SSS_SNOW];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_SSS];

		device->CreatePipelineState(&desc, &PSO_sss_snow);
		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_SCREEN];
		desc.ps = &shaders[PSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_PREMULTIPLIED];
		desc.dss = &depthStencils[DSSTYPE_XRAY];

		device->CreatePipelineState(&desc, &PSO_upsample_bilateral);
		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_SCREEN];
		desc.ps = &shaders[PSTYPE_POSTPROCESS_OUTLINE];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_TRANSPARENT];
		desc.dss = &depthStencils[DSSTYPE_XRAY];

		device->CreatePipelineState(&desc, &PSO_outline);
		});
	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.vs = &shaders[VSTYPE_LENSFLARE];
		desc.ps = &shaders[PSTYPE_LENSFLARE];
		desc.gs = &shaders[GSTYPE_LENSFLARE];
		desc.bs = &blendStates[BSTYPE_ADDITIVE];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.dss = &depthStencils[DSSTYPE_XRAY];
		desc.pt = POINTLIST;

		device->CreatePipelineState(&desc, &PSO_lensflare);
		});
	wiJobSystem::Dispatch(ctx, SKYRENDERING_COUNT, 1, [](wiJobArgs args) {
		PipelineStateDesc desc;
		desc.rs = &rasterizers[RSTYPE_SKY];
		desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];

		switch (args.jobIndex)
		{
		case SKYRENDERING_STATIC:
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.vs = &shaders[VSTYPE_SKY];
			desc.ps = &shaders[PSTYPE_SKY_STATIC];
			break;
		case SKYRENDERING_DYNAMIC:
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.vs = &shaders[VSTYPE_SKY];
			desc.ps = &shaders[PSTYPE_SKY_DYNAMIC];
			break;
		case SKYRENDERING_SUN:
			desc.bs = &blendStates[BSTYPE_ADDITIVE];
			desc.vs = &shaders[VSTYPE_SKY];
			desc.ps = &shaders[PSTYPE_SUN];
			break;
		case SKYRENDERING_ENVMAPCAPTURE_STATIC:
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.vs = &shaders[VSTYPE_ENVMAP_SKY];
			desc.ps = &shaders[PSTYPE_ENVMAP_SKY_STATIC];
			if (!device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS))
			{
				desc.gs = &shaders[GSTYPE_ENVMAP_SKY_EMULATION];
			}
			break;
		case SKYRENDERING_ENVMAPCAPTURE_DYNAMIC:
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.vs = &shaders[VSTYPE_ENVMAP_SKY];
			desc.ps = &shaders[PSTYPE_ENVMAP_SKY_DYNAMIC];
			if (!device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS))
			{
				desc.gs = &shaders[GSTYPE_ENVMAP_SKY_EMULATION];
			}
			break;
		}

		device->CreatePipelineState(&desc, &PSO_sky[args.jobIndex]);
		});
	wiJobSystem::Dispatch(ctx, DEBUGRENDERING_COUNT, 1, [](wiJobArgs args) {
		PipelineStateDesc desc;

		switch (args.jobIndex)
		{
		case DEBUGRENDERING_ENVPROBE:
			desc.vs = &shaders[VSTYPE_SPHERE];
			desc.ps = &shaders[PSTYPE_CUBEMAP];
			desc.dss = &depthStencils[DSSTYPE_DEFAULT];
			desc.rs = &rasterizers[RSTYPE_FRONT];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.pt = TRIANGLELIST;
			break;
		case DEBUGRENDERING_GRID:
			desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
			desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
			desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = LINELIST;
			break;
		case DEBUGRENDERING_CUBE:
			desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
			desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
			desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = LINELIST;
			break;
		case DEBUGRENDERING_LINES:
			desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
			desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
			desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
			desc.dss = &depthStencils[DSSTYPE_XRAY];
			desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = LINELIST;
			break;
		case DEBUGRENDERING_TRIANGLE_SOLID:
			desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
			desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
			desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = TRIANGLELIST;
			break;
		case DEBUGRENDERING_TRIANGLE_WIREFRAME:
			desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
			desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
			desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = TRIANGLELIST;
			break;
		case DEBUGRENDERING_EMITTER:
			desc.vs = &shaders[VSTYPE_OBJECT_DEBUG];
			desc.ps = &shaders[PSTYPE_OBJECT_DEBUG];
			desc.il = &inputLayouts[ILTYPE_OBJECT_DEBUG];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.pt = TRIANGLELIST;
			break;
		case DEBUGRENDERING_PAINTRADIUS:
			desc.vs = &shaders[VSTYPE_OBJECT_SIMPLE];
			desc.ps = &shaders[PSTYPE_OBJECT_PAINTRADIUS];
			desc.il = &inputLayouts[ILTYPE_OBJECT_POS_TEX];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.rs = &rasterizers[RSTYPE_FRONT];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = TRIANGLELIST;
			break;
		case DEBUGRENDERING_VOXEL:
			desc.vs = &shaders[VSTYPE_VOXEL];
			desc.ps = &shaders[PSTYPE_VOXEL];
			desc.gs = &shaders[GSTYPE_VOXEL];
			desc.dss = &depthStencils[DSSTYPE_DEFAULT];
			desc.rs = &rasterizers[RSTYPE_BACK];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.pt = POINTLIST;
			break;
		case DEBUGRENDERING_FORCEFIELD_POINT:
			desc.vs = &shaders[VSTYPE_FORCEFIELDVISUALIZER_POINT];
			desc.ps = &shaders[PSTYPE_FORCEFIELDVISUALIZER];
			desc.dss = &depthStencils[DSSTYPE_XRAY];
			desc.rs = &rasterizers[RSTYPE_BACK];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = TRIANGLELIST;
			break;
		case DEBUGRENDERING_FORCEFIELD_PLANE:
			desc.vs = &shaders[VSTYPE_FORCEFIELDVISUALIZER_PLANE];
			desc.ps = &shaders[PSTYPE_FORCEFIELDVISUALIZER];
			desc.dss = &depthStencils[DSSTYPE_XRAY];
			desc.rs = &rasterizers[RSTYPE_FRONT];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = TRIANGLESTRIP;
			break;
		case DEBUGRENDERING_RAYTRACE_BVH:
			desc.vs = &shaders[VSTYPE_RAYTRACE_SCREEN];
			desc.ps = &shaders[PSTYPE_RAYTRACE_DEBUGBVH];
			desc.dss = &depthStencils[DSSTYPE_XRAY];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = TRIANGLELIST;
			break;
		}

		device->CreatePipelineState(&desc, &PSO_debug[args.jobIndex]);
		});

	wiJobSystem::Wait(ctx);

}
void LoadBuffers()
{
	GPUBufferDesc bd;

	// The following buffers will be DEFAULT (long lifetime, slow update, fast read):
	bd.CPUAccessFlags = 0;
	bd.Usage = USAGE_DEFAULT;
	bd.MiscFlags = 0;

	bd.ByteWidth = sizeof(FrameCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_FRAME]);
	device->SetName(&constantBuffers[CBTYPE_FRAME], "FrameCB");

	bd.ByteWidth = sizeof(CameraCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_CAMERA]);
	device->SetName(&constantBuffers[CBTYPE_CAMERA], "CameraCB");


	bd.ByteWidth = sizeof(ShaderEntity) * SHADER_ENTITY_COUNT;
	bd.BindFlags = BIND_SHADER_RESOURCE;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(ShaderEntity);
	device->CreateBuffer(&bd, nullptr, &resourceBuffers[RBTYPE_ENTITYARRAY]);
	device->SetName(&resourceBuffers[RBTYPE_ENTITYARRAY], "EntityArray");

	bd.ByteWidth = sizeof(XMMATRIX) * MATRIXARRAY_COUNT;
	bd.BindFlags = BIND_SHADER_RESOURCE;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(XMMATRIX);
	device->CreateBuffer(&bd, nullptr, &resourceBuffers[RBTYPE_MATRIXARRAY]);
	device->SetName(&resourceBuffers[RBTYPE_MATRIXARRAY], "MatrixArray");


	// The following buffers will be DYNAMIC (short lifetime, fast update, slow read):
	bd.Usage = USAGE_DYNAMIC;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	bd.BindFlags = BIND_CONSTANT_BUFFER;

	bd.ByteWidth = sizeof(MiscCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_MISC]);
	device->SetName(&constantBuffers[CBTYPE_MISC], "MiscCB");

	bd.ByteWidth = sizeof(APICB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_API]);
	device->SetName(&constantBuffers[CBTYPE_API], "APICB");

	bd.ByteWidth = sizeof(VolumeLightCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_VOLUMELIGHT]);
	device->SetName(&constantBuffers[CBTYPE_VOLUMELIGHT], "VolumelightCB");

	bd.ByteWidth = sizeof(CubemapRenderCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_CUBEMAPRENDER]);
	device->SetName(&constantBuffers[CBTYPE_CUBEMAPRENDER], "CubemapRenderCB");

	bd.ByteWidth = sizeof(TessellationCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_TESSELLATION]);
	device->SetName(&constantBuffers[CBTYPE_TESSELLATION], "TessellationCB");

	bd.ByteWidth = sizeof(RaytracingCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_RAYTRACE]);
	device->SetName(&constantBuffers[CBTYPE_RAYTRACE], "RayTraceCB");

	bd.ByteWidth = sizeof(GenerateMIPChainCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_MIPGEN]);
	device->SetName(&constantBuffers[CBTYPE_MIPGEN], "MipGeneratorCB");

	bd.ByteWidth = sizeof(FilterEnvmapCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_FILTERENVMAP]);
	device->SetName(&constantBuffers[CBTYPE_FILTERENVMAP], "FilterEnvmapCB");

	bd.ByteWidth = sizeof(CopyTextureCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_COPYTEXTURE]);
	device->SetName(&constantBuffers[CBTYPE_COPYTEXTURE], "CopyTextureCB");

	bd.ByteWidth = sizeof(ForwardEntityMaskCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_FORWARDENTITYMASK]);
	device->SetName(&constantBuffers[CBTYPE_FORWARDENTITYMASK], "ForwardEntityMaskCB");

	bd.ByteWidth = sizeof(PostProcessCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_POSTPROCESS]);
	device->SetName(&constantBuffers[CBTYPE_POSTPROCESS], "PostProcessCB");

	bd.ByteWidth = sizeof(MSAOCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_POSTPROCESS_MSAO]);
	device->SetName(&constantBuffers[CBTYPE_POSTPROCESS_MSAO], "MSAOCB");

	bd.ByteWidth = sizeof(MSAO_UPSAMPLECB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_POSTPROCESS_MSAO_UPSAMPLE]);
	device->SetName(&constantBuffers[CBTYPE_POSTPROCESS_MSAO_UPSAMPLE], "MSAO_UPSAMPLECB");

	bd.ByteWidth = sizeof(LensFlareCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_LENSFLARE]);
	device->SetName(&constantBuffers[CBTYPE_LENSFLARE], "LensFlareCB");

	bd.ByteWidth = sizeof(PaintRadiusCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_PAINTRADIUS]);
	device->SetName(&constantBuffers[CBTYPE_PAINTRADIUS], "PaintRadiusCB");

	bd.ByteWidth = sizeof(ShadingRateClassificationCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_SHADINGRATECLASSIFICATION]);
	device->SetName(&constantBuffers[CBTYPE_SHADINGRATECLASSIFICATION], "ShadingRateClassificationCB");


}
void SetUpStates()
{
	SamplerDesc samplerDesc;
	samplerDesc.Filter = FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = FLT_MAX;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_LINEAR_MIRROR]);

	samplerDesc.Filter = FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_LINEAR_CLAMP]);

	samplerDesc.Filter = FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_LINEAR_WRAP]);

	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_POINT_MIRROR]);

	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_POINT_WRAP]);


	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_POINT_CLAMP]);

	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MaxAnisotropy = 16;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_ANISO_CLAMP]);

	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 16;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_ANISO_WRAP]);

	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MaxAnisotropy = 16;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_ANISO_MIRROR]);

	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 16;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_OBJECTSHADER]);

	samplerDesc.Filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = COMPARISON_GREATER_EQUAL;
	device->CreateSampler(&samplerDesc, &samplers[SSLOT_CMP_DEPTH]);



	RasterizerStateDesc rs;
	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_BACK;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_FRONT]);


	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_BACK;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = -2.0f;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_SHADOW]);

	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_NONE;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = -2.0f;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_SHADOW_DOUBLESIDED]);

	rs.FillMode = FILL_WIREFRAME;
	rs.CullMode = CULL_BACK;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_WIRE]);
	rs.AntialiasedLineEnable = true;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_WIRE_SMOOTH]);

	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_NONE;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_DOUBLESIDED]);

	rs.FillMode = FILL_WIREFRAME;
	rs.CullMode = CULL_NONE;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_WIRE_DOUBLESIDED]);
	rs.AntialiasedLineEnable = true;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH]);

	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_FRONT;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_BACK]);

	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_FRONT;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_OCCLUDEE]);

	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_FRONT;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = false;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_SKY]);

	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_NONE;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false; // do it in the shader for now...
	device->CreateRasterizerState(&rs, &rasterizers[RSTYPE_VOXELIZE]);




	DepthStencilStateDesc dsd;
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_GREATER;

	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0;
	dsd.StencilWriteMask = 0xFF;
	dsd.FrontFace.StencilFunc = COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_REPLACE;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_ALWAYS;
	dsd.BackFace.StencilPassOp = STENCIL_OP_REPLACE;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DEFAULT]);

	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_GREATER;
	dsd.StencilEnable = false;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_SHADOW]);

	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_GREATER;
	dsd.StencilEnable = false;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_CAPTUREIMPOSTOR]);

	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_LESS;
	dsd.StencilEnable = false;
	dsd.StencilReadMask = 0;
	dsd.StencilWriteMask = 0;
	dsd.FrontFace.StencilFunc = COMPARISON_GREATER_EQUAL;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_GREATER_EQUAL;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	//device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DEFERREDLIGHT]);

	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_LESS;
	dsd.StencilEnable = false;
	dsd.StencilReadMask = 0;
	dsd.StencilWriteMask = 0;
	dsd.FrontFace.StencilFunc = COMPARISON_LESS_EQUAL;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_LESS_EQUAL;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DEFERREDCOMPOSITION]);


	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthEnable = false;
	dsd.StencilEnable = true;
	dsd.DepthFunc = COMPARISON_GREATER;
	dsd.StencilReadMask = STENCILREF_MASK_ENGINE;
	dsd.StencilWriteMask = 0x00;
	dsd.FrontFace.StencilFunc = COMPARISON_EQUAL;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_EQUAL;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_SSS]);


	dsd.DepthEnable = true;
	dsd.StencilEnable = false;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_GREATER_EQUAL;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DEPTHREAD]);

	dsd.DepthEnable = false;
	dsd.StencilEnable = false;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_XRAY]);


	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_EQUAL;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DEPTHREADEQUAL]);


	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_GREATER;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_ENVMAP]);

	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_ALWAYS;
	dsd.StencilEnable = false;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_WRITEONLY]);




	BlendStateDesc bd;
	bd.RenderTarget[0].BlendEnable = false;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_MAX;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.AlphaToCoverageEnable = false;
	bd.IndependentBlendEnable = false;
	device->CreateBlendState(&bd, &blendStates[BSTYPE_OPAQUE]);

	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.AlphaToCoverageEnable = false;
	bd.IndependentBlendEnable = false;
	device->CreateBlendState(&bd, &blendStates[BSTYPE_TRANSPARENT]);

	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	bd.AlphaToCoverageEnable = false;
	device->CreateBlendState(&bd, &blendStates[BSTYPE_PREMULTIPLIED]);


	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false,
		bd.AlphaToCoverageEnable = false;
	device->CreateBlendState(&bd, &blendStates[BSTYPE_ADDITIVE]);


	bd.RenderTarget[0].BlendEnable = false;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_DISABLE;
	bd.IndependentBlendEnable = false,
		bd.AlphaToCoverageEnable = false;
	device->CreateBlendState(&bd, &blendStates[BSTYPE_COLORWRITEDISABLE]);


	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN | COLOR_WRITE_ENABLE_BLUE; // alpha is not written by deferred lights!
	bd.IndependentBlendEnable = false,
		bd.AlphaToCoverageEnable = false;
	//device->CreateBlendState(&bd, &blendStates[BSTYPE_DEFERREDLIGHT]);

	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA; // can overwrite ambient and lightmap
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN | COLOR_WRITE_ENABLE_BLUE; // alpha is not written by deferred lights!
	bd.IndependentBlendEnable = false;
	bd.AlphaToCoverageEnable = false;
	device->CreateBlendState(&bd, &blendStates[BSTYPE_ENVIRONMENTALLIGHT]);

	bd.RenderTarget[0].SrcBlend = BLEND_INV_SRC_COLOR;
	bd.RenderTarget[0].DestBlend = BLEND_INV_DEST_COLOR;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.AlphaToCoverageEnable = false;
	bd.IndependentBlendEnable = false;
	device->CreateBlendState(&bd, &blendStates[BSTYPE_INVERSE]);


	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN | COLOR_WRITE_ENABLE_BLUE;
	bd.RenderTarget[1] = bd.RenderTarget[0];
	bd.RenderTarget[1].RenderTargetWriteMask = COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN;
	bd.AlphaToCoverageEnable = false;
	bd.IndependentBlendEnable = true;
	device->CreateBlendState(&bd, &blendStates[BSTYPE_DECAL]);


	bd.RenderTarget[0].SrcBlend = BLEND_DEST_COLOR;
	bd.RenderTarget[0].DestBlend = BLEND_ZERO;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_DEST_ALPHA;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.AlphaToCoverageEnable = false;
	bd.IndependentBlendEnable = false;
	device->CreateBlendState(&bd, &blendStates[BSTYPE_MULTIPLY]);


	bd.RenderTarget[0].SrcBlend = BLEND_DEST_COLOR;
	bd.RenderTarget[0].DestBlend = BLEND_ZERO;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.AlphaToCoverageEnable = false;
	bd.IndependentBlendEnable = false;
	device->CreateBlendState(&bd, &blendStates[BSTYPE_TRANSPARENTSHADOWMAP]);
}

void ModifySampler(const SamplerDesc& desc, int slot)
{
	device->CreateSampler(&desc, &samplers[slot]);
}

const std::string& GetShaderPath()
{
	return SHADERPATH;
}
void SetShaderPath(const std::string& path)
{
	SHADERPATH = path;
}
void ReloadShaders()
{
	device->ClearPipelineStateCache();

	wiEvent::FireEvent(SYSTEM_EVENT_RELOAD_SHADERS, 0);
}

CameraComponent& GetCamera()
{
	static CameraComponent camera;
	return camera;
}
CameraComponent& GetPrevCamera()
{
	static CameraComponent camera;
	return camera;
}
CameraComponent& GetRefCamera()
{
	static CameraComponent camera;
	return camera;
}
void AttachCamera(wiECS::Entity entity)
{
	cameraTransform = entity;
}

void Initialize()
{
	auto camera_setup = [](uint64_t userdata) {
		GetCamera().CreatePerspective((float)GetInternalResolution().x, (float)GetInternalResolution().y, 0.1f, 800);
	};
	camera_setup(0);
	static wiEvent::Handle handle1 = wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_RESOLUTION, [=](uint64_t userdata) { camera_setup(userdata); });

	frameCullings[&GetCamera()].Clear();
	frameCullings[&GetRefCamera()].Clear();

	SetUpStates();
	LoadBuffers();

	static wiEvent::Handle handle2 = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
	LoadShaders();

	SetShadowProps2D(SHADOWRES_2D, SHADOWCOUNT_2D, SOFTSHADOWQUALITY_2D);
	SetShadowPropsCube(SHADOWRES_CUBE, SHADOWCOUNT_CUBE);


	static wiEvent::Handle handle3 = wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_RESOLUTION, [](uint64_t userdata) {
		int width = userdata & 0xFFFF;
		int height = (userdata >> 16) & 0xFFFF;
		device->SetResolution(width, height);
	});

	static wiEvent::Handle handle4 = wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_DPI, [](uint64_t userdata) {
		int dpi = userdata & 0xFFFF;
		wiPlatform::GetWindowState().dpi = dpi;
	});

	TextureDesc desc;
	desc.Width = 1;
	desc.Height = 1;
	desc.Format = FORMAT_R11G11B10_FLOAT;
	desc.BindFlags = BIND_SHADER_RESOURCE;
	device->CreateTexture(&desc, nullptr, &globalLightmap);

	wiBackLog::post("wiRenderer Initialized");
}
void ClearWorld()
{
	waterRipples.clear();

	GetScene().Clear();

	sceneBVH.Clear();
	scene_bvh_invalid = true;

	deferredMIPGenLock.lock();
	deferredMIPGens.clear();
	deferredMIPGenLock.unlock();


	for (auto& x : frameCullings)
	{
		FrameCulling& culling = x.second;
		culling.Clear();
	}

	packedDecals.clear();
	packedLightmaps.clear();

	pendingMaterialUpdates.clear();
	pendingMorphUpdates.clear();
	pendingBottomLevelBuilds.clear();
}

static const uint32_t CASCADE_COUNT = 3;
// Don't store this structure on heap!
struct SHCAM
{
	XMMATRIX VP;
	Frustum frustum;					// This frustum can be used for intersection test with wiIntersect primitives
	BoundingFrustum boundingfrustum;	// This boundingfrustum can be used for frustum vs frustum intersection test

	SHCAM() {}
	SHCAM(const XMFLOAT3& eyePos, const XMFLOAT4& rotation, float nearPlane, float farPlane, float fov) 
	{
		const XMVECTOR E = XMLoadFloat3(&eyePos);
		const XMVECTOR Q = XMQuaternionNormalize(XMLoadFloat4(&rotation));
		const XMMATRIX rot = XMMatrixRotationQuaternion(Q);
		const XMVECTOR to = XMVector3TransformNormal(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), rot);
		const XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rot);
		const XMMATRIX V = XMMatrixLookToLH(E, to, up);
		const XMMATRIX P = XMMatrixPerspectiveFovLH(fov, 1, farPlane, nearPlane);
		VP = XMMatrixMultiply(V, P);
		frustum.Create(VP);

		
		BoundingFrustum::CreateFromMatrix(boundingfrustum, P);
		std::swap(boundingfrustum.Near, boundingfrustum.Far);
		boundingfrustum.Transform(boundingfrustum, XMMatrixInverse(nullptr, V));
		XMStoreFloat4(&boundingfrustum.Orientation, XMQuaternionNormalize(XMLoadFloat4(&boundingfrustum.Orientation)));
	};
};
inline void CreateSpotLightShadowCam(const LightComponent& light, SHCAM& shcam)
{
	shcam = SHCAM(light.position, light.rotation, 0.1f, light.GetRange(), light.fov);
}
inline void CreateDirLightShadowCams(const LightComponent& light, CameraComponent camera, std::array<SHCAM, CASCADE_COUNT>& shcams)
{
	if (GetTemporalAAEnabled())
	{
		// remove camera jittering
		camera.jitter = XMFLOAT2(0, 0);
		camera.UpdateCamera();
	}

	const XMMATRIX lightRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation));
	const XMVECTOR to = XMVector3TransformNormal(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), lightRotation);
	const XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), lightRotation);
	const XMMATRIX lightView = XMMatrixLookToLH(XMVectorZero(), to, up); // important to not move (zero out eye vector) the light view matrix itself because texel snapping must be done on projection matrix!
	const float nearPlane = camera.zNearP;
	const float farPlane = camera.zFarP;
	const float referenceFarPlane = 800.0f; // cascade splits here were tested with this depth range
	const float referenceSplitClamp = std::min(1.0f, referenceFarPlane / farPlane); // if far plane is greater than reference, do not increase cascade sizes further
	const float splits[CASCADE_COUNT + 1] = {
		referenceSplitClamp * 0.0f,		// near plane
		referenceSplitClamp * 0.01f,	// near-mid split
		referenceSplitClamp * 0.1f,		// mid-far split
		referenceSplitClamp * 1.0f,		// far plane
	};

	// Unproject main frustum corners into world space (notice the reversed Z projection!):
	const XMMATRIX unproj = camera.GetInvViewProjection();
	const XMVECTOR frustum_corners[] =
	{
		XMVector3TransformCoord(XMVectorSet(-1, -1, 1, 1), unproj),	// near
		XMVector3TransformCoord(XMVectorSet(-1, -1, 0, 1), unproj),	// far
		XMVector3TransformCoord(XMVectorSet(-1, 1, 1, 1), unproj),	// near
		XMVector3TransformCoord(XMVectorSet(-1, 1, 0, 1), unproj),	// far
		XMVector3TransformCoord(XMVectorSet(1, -1, 1, 1), unproj),	// near
		XMVector3TransformCoord(XMVectorSet(1, -1, 0, 1), unproj),	// far
		XMVector3TransformCoord(XMVectorSet(1, 1, 1, 1), unproj),	// near
		XMVector3TransformCoord(XMVectorSet(1, 1, 0, 1), unproj),	// far
	};

	// Compute shadow cameras:
	for (int cascade = 0; cascade < CASCADE_COUNT; ++cascade)
	{
		// Compute cascade sub-frustum in light-view-space from the main frustum corners:
		const float split_near = splits[cascade];
		const float split_far = splits[cascade + 1];
		const XMVECTOR corners[] =
		{
			XMVector3Transform(XMVectorLerp(frustum_corners[0], frustum_corners[1], split_near), lightView),
			XMVector3Transform(XMVectorLerp(frustum_corners[0], frustum_corners[1], split_far), lightView),
			XMVector3Transform(XMVectorLerp(frustum_corners[2], frustum_corners[3], split_near), lightView),
			XMVector3Transform(XMVectorLerp(frustum_corners[2], frustum_corners[3], split_far), lightView),
			XMVector3Transform(XMVectorLerp(frustum_corners[4], frustum_corners[5], split_near), lightView),
			XMVector3Transform(XMVectorLerp(frustum_corners[4], frustum_corners[5], split_far), lightView),
			XMVector3Transform(XMVectorLerp(frustum_corners[6], frustum_corners[7], split_near), lightView),
			XMVector3Transform(XMVectorLerp(frustum_corners[6], frustum_corners[7], split_far), lightView),
		};

		// Compute cascade bounding sphere center:
		XMVECTOR center = XMVectorZero();
		for (int j = 0; j < arraysize(corners); ++j)
		{
			center = XMVectorAdd(center, corners[j]);
		}
		center = center / float(arraysize(corners));

		// Compute cascade bounding sphere radius:
		float radius = 0;
		for (int j = 0; j < arraysize(corners); ++j)
		{
			radius = std::max(radius, XMVectorGetX(XMVector3Length(XMVectorSubtract(corners[j], center))));
		}

		// Fit AABB onto bounding sphere:
		XMVECTOR vRadius = XMVectorReplicate(radius);
		XMVECTOR vMin = XMVectorSubtract(center, vRadius);
		XMVECTOR vMax = XMVectorAdd(center, vRadius);

		// Snap cascade to texel grid:
		const XMVECTOR extent = XMVectorSubtract(vMax, vMin);
		const XMVECTOR texelSize = extent / float(wiRenderer::GetShadowRes2D());
		vMin = XMVectorFloor(vMin / texelSize) * texelSize;
		vMax = XMVectorFloor(vMax / texelSize) * texelSize;
		center = (vMin + vMax) * 0.5f;

		XMFLOAT3 _center;
		XMFLOAT3 _min;
		XMFLOAT3 _max;
		XMStoreFloat3(&_center, center);
		XMStoreFloat3(&_min, vMin);
		XMStoreFloat3(&_max, vMax);

		// Extrude bounds to avoid early shadow clipping:
		float ext = abs(_center.z - _min.z);
		ext = std::max(ext, farPlane * 0.5f);
		_min.z = _center.z - ext;
		_max.z = _center.z + ext;

		const XMMATRIX lightProjection = XMMatrixOrthographicOffCenterLH(_min.x, _max.x, _min.y, _max.y, _max.z, _min.z); // notice reversed Z!

		shcams[cascade].VP = XMMatrixMultiply(lightView, lightProjection);
		shcams[cascade].frustum.Create(shcams[cascade].VP);
	}

}


ForwardEntityMaskCB ForwardEntityCullingCPU(const FrameCulling& culling, const AABB& batch_aabb, RENDERPASS renderPass)
{
	// Performs CPU light culling for a renderable batch:
	//	Similar to GPU-based tiled light culling, but this is only for simple forward passes (drawcall-granularity)

	const Scene& scene = GetScene();

	ForwardEntityMaskCB cb;
	cb.xForwardLightMask.x = 0;
	cb.xForwardLightMask.y = 0;
	cb.xForwardDecalMask = 0;
	cb.xForwardEnvProbeMask = 0;

	uint32_t buckets[2] = { 0,0 };
	for (size_t i = 0; i < std::min(size_t(64), culling.culledLights.size()); ++i) // only support indexing 64 lights at max for now
	{
		const uint32_t lightIndex = culling.culledLights[i];
		const AABB& light_aabb = scene.aabb_lights[lightIndex];
		if (light_aabb.intersects(batch_aabb))
		{
			const uint8_t bucket_index = uint8_t(i / 32);
			const uint8_t bucket_place = uint8_t(i % 32);
			buckets[bucket_index] |= 1 << bucket_place;
		}
	}
	cb.xForwardLightMask.x = buckets[0];
	cb.xForwardLightMask.y = buckets[1];

	for (size_t i = 0; i < std::min(size_t(32), culling.culledDecals.size()); ++i)
	{
		const uint32_t decalIndex = culling.culledDecals[culling.culledDecals.size() - 1 - i]; // note: reverse order, for correct blending!
		const AABB& decal_aabb = scene.aabb_decals[decalIndex];
		if (decal_aabb.intersects(batch_aabb))
		{
			const uint8_t bucket_place = uint8_t(i % 32);
			cb.xForwardDecalMask |= 1 << bucket_place;
		}
	}

	if (renderPass != RENDERPASS_ENVMAPCAPTURE)
	{
		for (size_t i = 0; i < std::min(size_t(32), culling.culledEnvProbes.size()); ++i)
		{
			const uint32_t probeIndex = culling.culledEnvProbes[culling.culledEnvProbes.size() - 1 - i]; // note: reverse order, for correct blending!
			const AABB& probe_aabb = scene.aabb_probes[probeIndex];
			if (probe_aabb.intersects(batch_aabb))
			{
				const uint8_t bucket_place = uint8_t(i % 32);
				cb.xForwardEnvProbeMask |= 1 << bucket_place;
			}
		}
	}

	return cb;
}

void BindConstantBuffers(SHADERSTAGE stage, CommandList cmd)
{
	device->BindConstantBuffer(stage, &constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), cmd);
	device->BindConstantBuffer(stage, &constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), cmd);
	device->BindConstantBuffer(stage, &constantBuffers[CBTYPE_API], CB_GETBINDSLOT(APICB), cmd);
}
void BindShadowmaps(SHADERSTAGE stage, CommandList cmd)
{
	device->BindResource(stage, &shadowMapArray_2D, TEXSLOT_SHADOWARRAY_2D, cmd);
	device->BindResource(stage, &shadowMapArray_Cube, TEXSLOT_SHADOWARRAY_CUBE, cmd);
	if (GetTransparentShadowsEnabled())
	{
		device->BindResource(stage, &shadowMapArray_Transparent, TEXSLOT_SHADOWARRAY_TRANSPARENT, cmd);
	}
}
void BindEnvironmentTextures(SHADERSTAGE stage, CommandList cmd)
{
	device->BindResource(stage, &textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], TEXSLOT_ENVMAPARRAY, cmd);
	device->BindResource(stage, &textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], TEXSLOT_ENVMAPARRAY, cmd);
	device->BindResource(stage, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
	device->BindResource(stage, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
	device->BindResource(stage, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);
	device->BindResource(stage, GetVoxelRadianceSecondaryBounceEnabled() ? &textures[TEXTYPE_3D_VOXELRADIANCE_HELPER] : &textures[TEXTYPE_3D_VOXELRADIANCE], TEXSLOT_VOXELRADIANCE, cmd);

	if (GetScene().weather.skyMap != nullptr)
	{
		device->BindResource(stage, GetScene().weather.skyMap->texture, TEXSLOT_GLOBALENVMAP, cmd);
	}
}

void RenderMeshes(
	const RenderQueue& renderQueue,
	RENDERPASS renderPass,
	uint32_t renderTypeFlags,
	CommandList cmd,
	bool tessellation = false,
	const Frustum* frusta = nullptr,
	uint32_t frustum_count = 1
)
{
	if (!renderQueue.empty())
	{
		const Scene& scene = GetScene();

		device->EventBegin("RenderMeshes", cmd);

		tessellation = tessellation && device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_TESSELLATION);
		if (tessellation)
		{
			BindConstantBuffers(DS, cmd);
		}

		struct InstBuf
		{
			Instance instance;
			InstancePrev instancePrev;
			InstanceAtlas instanceAtlas;
		};

		// Do we need to bind every vertex buffer or just a reduced amount for this pass?
		const bool advancedVBRequest =
			!IsWireRender() && (
				renderPass == RENDERPASS_MAIN ||
				renderPass == RENDERPASS_ENVMAPCAPTURE ||
				renderPass == RENDERPASS_VOXELIZE
				);

		// Do we need to bind all textures or just a reduced amount for this pass?
		const bool easyTextureBind =
			renderPass == RENDERPASS_SHADOW ||
			renderPass == RENDERPASS_SHADOWCUBE ||
			renderPass == RENDERPASS_DEPTHONLY;

		// Do we need to compute a light mask for this pass on the CPU?
		const bool forwardLightmaskRequest =
			renderPass == RENDERPASS_ENVMAPCAPTURE ||
			renderPass == RENDERPASS_VOXELIZE;

		// Pre-allocate space for all the instances in GPU-buffer:
		const uint32_t instanceDataSize = advancedVBRequest ? sizeof(InstBuf) : sizeof(Instance);
		size_t alloc_size = renderQueue.batchCount * frustum_count * instanceDataSize;
		GraphicsDevice::GPUAllocation instances = device->AllocateGPU(alloc_size, cmd);

		// Purpose of InstancedBatch:
		//	The RenderQueue is sorted by meshIndex. There can be multiple instances for a single meshIndex,
		//	and the InstancedBatchArray contains this information. The array size will be the unique mesh count here.
		struct InstancedBatch
		{
			uint32_t meshIndex;
			int instanceCount;
			uint32_t dataOffset;
			uint8_t userStencilRefOverride;
			uint8_t forceAlphatestForDithering; // padded bool
			AABB aabb;
		};
		InstancedBatch* instancedBatchArray = nullptr;
		int instancedBatchCount = 0;

		// The following loop is writing the instancing batches to a GPUBuffer:
		size_t prevMeshIndex = ~0;
		uint8_t prevUserStencilRefOverride = 0;
		uint32_t instanceCount = 0;
		for (uint32_t batchID = 0; batchID < renderQueue.batchCount; ++batchID) // Do not break out of this loop!
		{
			const RenderBatch& batch = renderQueue.batchArray[batchID];
			const uint32_t meshIndex = batch.GetMeshIndex();
			const uint32_t instanceIndex = batch.GetInstanceIndex();
			const ObjectComponent& instance = scene.objects[instanceIndex];
			const uint8_t userStencilRefOverride = instance.userStencilRef;

			// When we encounter a new mesh inside the global instance array, we begin a new InstancedBatch:
			if (meshIndex != prevMeshIndex || userStencilRefOverride != prevUserStencilRefOverride)
			{
				prevMeshIndex = meshIndex;
				prevUserStencilRefOverride = userStencilRefOverride;

				instancedBatchCount++;
				InstancedBatch* instancedBatch = (InstancedBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(InstancedBatch));
				instancedBatch->meshIndex = meshIndex;
				instancedBatch->instanceCount = 0;
				instancedBatch->dataOffset = instances.offset + instanceCount * instanceDataSize;
				instancedBatch->userStencilRefOverride = userStencilRefOverride;
				instancedBatch->forceAlphatestForDithering = 0;
				instancedBatch->aabb = AABB();
				if (instancedBatchArray == nullptr)
				{
					instancedBatchArray = instancedBatch;
				}
			}

			InstancedBatch& current_batch = instancedBatchArray[instancedBatchCount - 1];

			float dither = instance.GetTransparency();

			if (instance.IsImpostorPlacement())
			{
				float distance = batch.GetDistance();
				float swapDistance = instance.impostorSwapDistance;
				float fadeThreshold = instance.impostorFadeThresholdRadius;
				dither = std::max(0.0f, distance - swapDistance) / fadeThreshold;
			}

			if (dither > 0)
			{
				current_batch.forceAlphatestForDithering = 1;
			}

			const AABB& instanceAABB = scene.aabb_objects[instanceIndex];

			if (forwardLightmaskRequest)
			{
				current_batch.aabb = AABB::Merge(current_batch.aabb, instanceAABB);
			}

			const XMFLOAT4X4& worldMatrix = instance.transform_index >= 0 ? scene.transforms[instance.transform_index].world : IDENTITYMATRIX;

			for (uint32_t frustum_index = 0; frustum_index < frustum_count; ++frustum_index)
			{
				if (frusta != nullptr && !frusta[frustum_index].CheckBoxFast(instanceAABB))
				{
					// In case shadow cameras were provided and no intersection detected with frustum, we don't add the instance for the face:
					continue;
				}

				// Write into actual GPU-buffer:
				if (advancedVBRequest)
				{
					((volatile InstBuf*)instances.data)[instanceCount].instance.Create(worldMatrix, instance.color, dither, frustum_index);

					const XMFLOAT4X4& prev_worldMatrix = instance.prev_transform_index >= 0 ? scene.prev_transforms[instance.prev_transform_index].world_prev : IDENTITYMATRIX;
					((volatile InstBuf*)instances.data)[instanceCount].instancePrev.Create(prev_worldMatrix);
					((volatile InstBuf*)instances.data)[instanceCount].instanceAtlas.Create(instance.globalLightMapMulAdd);
				}
				else
				{
					((volatile Instance*)instances.data)[instanceCount].Create(worldMatrix, instance.color, dither, frustum_index);
				}

				current_batch.instanceCount++; // next instance in current InstancedBatch
				instanceCount++;
			}

		}

		// Render instanced batches:
		PRIMITIVETOPOLOGY prevTOPOLOGY = TRIANGLELIST;
		for (int instancedBatchID = 0; instancedBatchID < instancedBatchCount; ++instancedBatchID)
		{
			const InstancedBatch& instancedBatch = instancedBatchArray[instancedBatchID];
			const MeshComponent& mesh = scene.meshes[instancedBatch.meshIndex];
			const bool forceAlphaTestForDithering = instancedBatch.forceAlphatestForDithering != 0;
			const uint8_t userStencilRefOverride = instancedBatch.userStencilRefOverride;

			const float tessF = mesh.GetTessellationFactor();
			const bool tessellatorRequested = tessF > 0 && tessellation;
			const bool terrain = mesh.IsTerrain();

			if (tessellatorRequested)
			{
				TessellationCB tessCB;
				tessCB.g_f4TessFactors = XMFLOAT4(tessF, tessF, tessF, tessF);
				device->UpdateBuffer(&constantBuffers[CBTYPE_TESSELLATION], &tessCB, cmd);
				device->BindConstantBuffer(HS, &constantBuffers[CBTYPE_TESSELLATION], CBSLOT_RENDERER_TESSELLATION, cmd);
			}

			if (forwardLightmaskRequest)
			{
				const CameraComponent* camera = renderQueue.camera == nullptr ? &GetCamera() : renderQueue.camera;
				const FrameCulling& culling = frameCullings.at(camera);
				ForwardEntityMaskCB cb = ForwardEntityCullingCPU(culling, instancedBatch.aabb, renderPass);
				device->UpdateBuffer(&constantBuffers[CBTYPE_FORWARDENTITYMASK], &cb, cmd);
				device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_FORWARDENTITYMASK], CB_GETBINDSLOT(ForwardEntityMaskCB), cmd);
			}

			device->BindIndexBuffer(&mesh.indexBuffer, mesh.GetIndexFormat(), 0, cmd);

			enum class BOUNDVERTEXBUFFERTYPE
			{
				NOTHING,
				POSITION,
				POSITION_TEXCOORD,
				EVERYTHING,
			};
			BOUNDVERTEXBUFFERTYPE boundVBType_Prev = BOUNDVERTEXBUFFERTYPE::NOTHING;

			for (const MeshComponent::MeshSubset& subset : mesh.subsets)
			{
				if (subset.indexCount == 0)
				{
					continue;
				}
				const MaterialComponent& material = *scene.materials.GetComponent(subset.materialID);

				bool subsetRenderable = renderTypeFlags & material.GetRenderTypes();

				if (renderPass == RENDERPASS_SHADOW || renderPass == RENDERPASS_SHADOWCUBE)
				{
					subsetRenderable = subsetRenderable && material.IsCastingShadow();
				}

				if (!subsetRenderable)
				{
					continue;
				}

				const PipelineState* pso = GetObjectPSO(renderPass, renderTypeFlags, mesh, material, tessellatorRequested, forceAlphaTestForDithering);

				if (pso == nullptr || !pso->IsValid())
				{
					continue;
				}

				BOUNDVERTEXBUFFERTYPE boundVBType;
				if (advancedVBRequest || tessellatorRequested)
				{
					boundVBType = BOUNDVERTEXBUFFERTYPE::EVERYTHING;
				}
				else
				{
					// simple vertex buffers are used in some passes (note: tessellator requires more attributes)
					if ((renderPass == RENDERPASS_DEPTHONLY || renderPass == RENDERPASS_SHADOW || renderPass == RENDERPASS_SHADOWCUBE) && !material.IsAlphaTestEnabled() && !forceAlphaTestForDithering)
					{
						if (renderPass == RENDERPASS_SHADOW && (renderTypeFlags & RENDERTYPE_TRANSPARENT))
						{
							boundVBType = BOUNDVERTEXBUFFERTYPE::POSITION_TEXCOORD;
						}
						else
						{
							// bypass texcoord stream for non alphatested shadows and zprepass
							boundVBType = BOUNDVERTEXBUFFERTYPE::POSITION;
						}
					}
					else
					{
						boundVBType = BOUNDVERTEXBUFFERTYPE::POSITION_TEXCOORD;
					}
				}

				if (IsWireRender())
				{
					boundVBType = BOUNDVERTEXBUFFERTYPE::POSITION_TEXCOORD;
				}

				// Only bind vertex buffers when the layout changes
				if (boundVBType != boundVBType_Prev)
				{
					// Assemble the required vertex buffer:
					switch (boundVBType)
					{
					case BOUNDVERTEXBUFFERTYPE::POSITION:
					{
						const GPUBuffer* vbs[] = {
							mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
							instances.buffer
						};
						uint32_t strides[] = {
							sizeof(MeshComponent::Vertex_POS),
							instanceDataSize
						};
						uint32_t offsets[] = {
							0,
							instancedBatch.dataOffset
						};
						device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
					}
					break;
					case BOUNDVERTEXBUFFERTYPE::POSITION_TEXCOORD:
					{
						const GPUBuffer* vbs[] = {
							mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
							&mesh.vertexBuffer_UV0,
							&mesh.vertexBuffer_UV1,
							instances.buffer
						};
						uint32_t strides[] = {
							sizeof(MeshComponent::Vertex_POS),
							sizeof(MeshComponent::Vertex_TEX),
							sizeof(MeshComponent::Vertex_TEX),
							instanceDataSize
						};
						uint32_t offsets[] = {
							0,
							0,
							0,
							instancedBatch.dataOffset
						};
						device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
					}
					break;
					case BOUNDVERTEXBUFFERTYPE::EVERYTHING:
					{
						const GPUBuffer* vbs[] = {
							mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
							&mesh.vertexBuffer_UV0,
							&mesh.vertexBuffer_UV1,
							&mesh.vertexBuffer_ATL,
							&mesh.vertexBuffer_COL,
							mesh.streamoutBuffer_TAN.IsValid() ? &mesh.streamoutBuffer_TAN : &mesh.vertexBuffer_TAN,
							mesh.vertexBuffer_PRE.IsValid() ? &mesh.vertexBuffer_PRE : &mesh.vertexBuffer_POS,
							instances.buffer
						};
						uint32_t strides[] = {
							sizeof(MeshComponent::Vertex_POS),
							sizeof(MeshComponent::Vertex_TEX),
							sizeof(MeshComponent::Vertex_TEX),
							sizeof(MeshComponent::Vertex_TEX),
							sizeof(MeshComponent::Vertex_COL),
							sizeof(MeshComponent::Vertex_TAN),
							sizeof(MeshComponent::Vertex_POS),
							instanceDataSize
						};
						uint32_t offsets[] = {
							0,
							0,
							0,
							0,
							0,
							0,
							0,
							instancedBatch.dataOffset
						};
						device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
					}
					break;
					default:
						assert(0);
						break;
					}
				}
				boundVBType_Prev = boundVBType;

				SetAlphaRef(material.alphaRef, cmd);

				STENCILREF engineStencilRef = material.engineStencilRef;
				uint8_t userStencilRef = userStencilRefOverride > 0 ? userStencilRefOverride : material.userStencilRef;
				uint32_t stencilRef = CombineStencilrefs(engineStencilRef, userStencilRef);
				device->BindStencilRef(stencilRef, cmd);

				if (renderPass != RENDERPASS_DEPTHONLY) // depth only alpha test will be full res
				{
					device->BindShadingRate(material.shadingRate, cmd);
				}

				device->BindPipelineState(pso, cmd);

				device->BindConstantBuffer(VS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);
				device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);

				if (easyTextureBind)
				{
					const GPUResource* res[] = {
						material.GetBaseColorMap(),
					};
					device->BindResources(PS, res, TEXSLOT_RENDERER_BASECOLORMAP, arraysize(res), cmd);
				}
				else
				{
					const GPUResource* res[] = {
						material.GetBaseColorMap(),
						material.GetNormalMap(),
						material.GetSurfaceMap(),
						material.GetEmissiveMap(),
						material.GetDisplacementMap(),
						material.GetOcclusionMap(),
					};
					device->BindResources(PS, res, TEXSLOT_RENDERER_BASECOLORMAP, arraysize(res), cmd);
				}

				if (tessellatorRequested)
				{
					const GPUResource* res[] = {
						material.GetDisplacementMap(),
					};
					device->BindResources(DS, res, TEXSLOT_RENDERER_DISPLACEMENTMAP, arraysize(res), cmd);
					device->BindConstantBuffer(DS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);
				}

				if (terrain)
				{
					if (mesh.terrain_material1 == INVALID_ENTITY || !scene.materials.Contains(mesh.terrain_material1))
					{
						const GPUResource* res[] = {
							material.GetBaseColorMap(),
							material.GetNormalMap(),
							material.GetSurfaceMap(),
							material.GetEmissiveMap(),
						};
						device->BindResources(PS, res, TEXSLOT_RENDERER_BLEND1_BASECOLORMAP, arraysize(res), cmd);
						device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend1), cmd);
					}
					else
					{
						const MaterialComponent& blendmat = *scene.materials.GetComponent(mesh.terrain_material1);
						const GPUResource* res[] = {
							blendmat.GetBaseColorMap(),
							blendmat.GetNormalMap(),
							blendmat.GetSurfaceMap(),
							blendmat.GetEmissiveMap(),
						};
						device->BindResources(PS, res, TEXSLOT_RENDERER_BLEND1_BASECOLORMAP, arraysize(res), cmd);
						device->BindConstantBuffer(PS, &blendmat.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend1), cmd);
					}

					if (mesh.terrain_material2 == INVALID_ENTITY || !scene.materials.Contains(mesh.terrain_material2))
					{
						const GPUResource* res[] = {
							material.GetBaseColorMap(),
							material.GetNormalMap(),
							material.GetSurfaceMap(),
							material.GetEmissiveMap(),
						};
						device->BindResources(PS, res, TEXSLOT_RENDERER_BLEND2_BASECOLORMAP, arraysize(res), cmd);
						device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend2), cmd);
					}
					else
					{
						const MaterialComponent& blendmat = *scene.materials.GetComponent(mesh.terrain_material2);
						const GPUResource* res[] = {
							blendmat.GetBaseColorMap(),
							blendmat.GetNormalMap(),
							blendmat.GetSurfaceMap(),
							blendmat.GetEmissiveMap(),
						};
						device->BindResources(PS, res, TEXSLOT_RENDERER_BLEND2_BASECOLORMAP, arraysize(res), cmd);
						device->BindConstantBuffer(PS, &blendmat.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend2), cmd);
					}

					if (mesh.terrain_material3 == INVALID_ENTITY || !scene.materials.Contains(mesh.terrain_material3))
					{
						const GPUResource* res[] = {
							material.GetBaseColorMap(),
							material.GetNormalMap(),
							material.GetSurfaceMap(),
							material.GetEmissiveMap(),
						};
						device->BindResources(PS, res, TEXSLOT_RENDERER_BLEND3_BASECOLORMAP, arraysize(res), cmd);
						device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend3), cmd);
					}
					else
					{
						const MaterialComponent& blendmat = *scene.materials.GetComponent(mesh.terrain_material3);
						const GPUResource* res[] = {
							blendmat.GetBaseColorMap(),
							blendmat.GetNormalMap(),
							blendmat.GetSurfaceMap(),
							blendmat.GetEmissiveMap(),
						};
						device->BindResources(PS, res, TEXSLOT_RENDERER_BLEND3_BASECOLORMAP, arraysize(res), cmd);
						device->BindConstantBuffer(PS, &blendmat.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend3), cmd);
					}
				}

				device->DrawIndexedInstanced(subset.indexCount, instancedBatch.instanceCount, subset.indexOffset, 0, 0, cmd);
			}
		}

		ResetAlphaRef(cmd);

		GetRenderFrameAllocator(cmd).free(sizeof(InstancedBatch) * instancedBatchCount);

		device->EventEnd(cmd);
	}
}

void RenderImpostors(
	const CameraComponent& camera, 
	RENDERPASS renderPass, 
	CommandList cmd
)
{
	const Scene& scene = GetScene();
	const PipelineState* impostorRequest = GetImpostorPSO(renderPass);

	if (scene.impostors.GetCount() > 0 && impostorRequest != nullptr)
	{
		uint32_t instanceCount = 0;
		for (size_t impostorID = 0; impostorID < scene.impostors.GetCount(); ++impostorID)
		{
			const ImpostorComponent& impostor = scene.impostors[impostorID];
			if (camera.frustum.CheckBoxFast(impostor.aabb))
			{
				instanceCount += (uint32_t)impostor.instanceMatrices.size();
			}
		}

		if (instanceCount == 0)
		{
			return;
		}

		device->EventBegin("RenderImpostors", cmd);

		// Pre-allocate space for all the instances in GPU-buffer:
		const uint32_t instanceDataSize = sizeof(Instance);
		const size_t alloc_size = instanceCount * instanceDataSize;
		GraphicsDevice::GPUAllocation instances = device->AllocateGPU(alloc_size, cmd);

		uint32_t drawableInstanceCount = 0;
		for (size_t impostorID = 0; impostorID < scene.impostors.GetCount(); ++impostorID)
		{
			const ImpostorComponent& impostor = scene.impostors[impostorID];
			if (!camera.frustum.CheckBoxFast(impostor.aabb))
			{
				continue;
			}

			for (auto& mat : impostor.instanceMatrices)
			{
				const XMFLOAT3 center = *((XMFLOAT3*)&mat._41);
				float distance = wiMath::Distance(camera.Eye, center);

				if (distance < impostor.swapInDistance - impostor.fadeThresholdRadius)
				{
					continue;
				}

				float dither = std::max(0.0f, impostor.swapInDistance - distance) / impostor.fadeThresholdRadius;

				((volatile Instance*)instances.data)[drawableInstanceCount].Create(mat, impostor.color, dither, uint32_t(impostorID * impostorCaptureAngles * 3));

				drawableInstanceCount++;
			}
		}

		device->BindStencilRef(STENCILREF_DEFAULT, cmd);
		device->BindPipelineState(impostorRequest, cmd);
		SetAlphaRef(0.75f, cmd);

		MiscCB cb;
		cb.g_xColor.x = (float)instances.offset;
		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &cb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		device->BindResource(VS, instances.buffer, TEXSLOT_ONDEMAND21, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_IMPOSTORARRAY], TEXSLOT_ONDEMAND0, cmd);

		device->Draw(drawableInstanceCount * 6, 0, cmd);

		device->EventEnd(cmd);
	}
}

void ProcessDeferredMipGenRequests(CommandList cmd)
{
	deferredMIPGenLock.lock();
	for (auto& it : deferredMIPGens)
	{
		MIPGEN_OPTIONS mipopt;
		mipopt.preserve_coverage = it.second;
		GenerateMipChain(*it.first->texture, MIPGENFILTER_LINEAR, cmd, mipopt);
	}
	deferredMIPGens.clear();
	deferredMIPGenLock.unlock();
}

void UpdatePerFrameData(float dt, uint32_t layerMask)
{
	renderTime_Prev = renderTime;
	deltaTime = dt * GetGameSpeed();
	renderTime += deltaTime;

	Scene& scene = GetScene();

	scene.Update(deltaTime);

	wiJobSystem::context ctx;

	// Because main camera is not part of the scene, update it if it is attached to an entity here:
	if (cameraTransform != INVALID_ENTITY)
	{
		const TransformComponent* transform = scene.transforms.GetComponent(cameraTransform);
		if (transform != nullptr)
		{
			GetCamera().TransformCamera(*transform);
			GetCamera().UpdateCamera();
		}
		cameraTransform = INVALID_ENTITY; // but this is only active for the current frame
	}

	if (GetTemporalAAEnabled())
	{
		const XMFLOAT4& halton = wiMath::GetHaltonSequence(device->GetFrameCount() % 256);
		static float jitter = 1.0f;
		temporalAAJitterPrev = temporalAAJitter;
		temporalAAJitter.x = jitter * (halton.x * 2 - 1) / (float)GetInternalResolution().x;
		temporalAAJitter.y = jitter * (halton.y * 2 - 1) / (float)GetInternalResolution().y;
		GetCamera().jitter = temporalAAJitter;
	}
	else
	{
		temporalAAJitter = XMFLOAT2(0, 0);
		temporalAAJitterPrev = XMFLOAT2(0, 0);
	}

	GetCamera().UpdateCamera();
	GetRefCamera() = GetCamera();
	GetRefCamera().Reflect(waterPlane);

	// See which materials will need to update their GPU render data:
	wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
		for (size_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			MaterialComponent& material = scene.materials[i];

			if (material.IsDirty())
			{
				material.SetDirty(false);
				pendingMaterialUpdates.push_back(i);
			}
		}
	});

	// See which mesh morphs will need to update their GPU render data:
	wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
	    for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
	    {
			MeshComponent& mesh = scene.meshes[i];

			if (mesh.IsDirtyMorph())
			{
			    mesh.SetDirtyMorph(false);
				pendingMorphUpdates.push_back(i);
			}
	    }
	});

	// Need to swap prev and current vertex buffers for any dynamic meshes BEFORE render threads are kicked 
	//	and also create skinning bone buffers:
	wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
		const bool raytracing_api = device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING);

		for (size_t i = 0; i < scene.softbodies.GetCount(); ++i)
		{
			const SoftBodyPhysicsComponent& softbody = scene.softbodies[i];
			if (softbody.vertex_positions_simulation.empty())
			{
				continue;
			}

			Entity entity = scene.softbodies.GetEntity(i);
			MeshComponent& mesh = *scene.meshes.GetComponent(entity);

			if (raytracing_api && !mesh.BLAS_build_pending)
			{
				pendingBottomLevelBuilds[entity] = AS_UPDATE;
			}

			if (!mesh.vertexBuffer_PRE.IsValid())
			{
				device->CreateBuffer(&mesh.vertexBuffer_POS.desc, nullptr, &mesh.streamoutBuffer_POS);
				device->CreateBuffer(&mesh.vertexBuffer_POS.desc, nullptr, &mesh.vertexBuffer_PRE);

				GPUBufferDesc tandesc = mesh.vertexBuffer_TAN.desc;
				tandesc.Usage = USAGE_DEFAULT;
				device->CreateBuffer(&tandesc, nullptr, &mesh.streamoutBuffer_TAN);

				if (raytracing_api)
				{
					mesh.BLAS.desc._flags |= RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE;
					device->CreateRaytracingAccelerationStructure(&mesh.BLAS.desc, &mesh.BLAS);
				}
			}
			std::swap(mesh.streamoutBuffer_POS, mesh.vertexBuffer_PRE);
		}
		for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
		{
			Entity entity = scene.meshes.GetEntity(i);
			MeshComponent& mesh = scene.meshes[i];

			if (mesh.IsSkinned() && scene.armatures.Contains(mesh.armatureID))
			{
				const SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(entity);
				if (softbody == nullptr || softbody->vertex_positions_simulation.empty())
				{
					if (raytracing_api && !mesh.BLAS_build_pending)
					{
						pendingBottomLevelBuilds[entity] = AS_UPDATE;
					}

					ArmatureComponent& armature = *scene.armatures.GetComponent(mesh.armatureID);

					if (!armature.boneBuffer.IsValid())
					{
						GPUBufferDesc bd;
						bd.Usage = USAGE_DYNAMIC;
						bd.CPUAccessFlags = CPU_ACCESS_WRITE;

						bd.ByteWidth = sizeof(ArmatureComponent::ShaderBoneType) * (uint32_t)armature.boneCollection.size();
						bd.BindFlags = BIND_SHADER_RESOURCE;
						bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
						bd.StructureByteStride = sizeof(ArmatureComponent::ShaderBoneType);

						device->CreateBuffer(&bd, nullptr, &armature.boneBuffer);
					}
					if (!mesh.vertexBuffer_PRE.IsValid())
					{
						device->CreateBuffer(&mesh.streamoutBuffer_POS.GetDesc(), nullptr, &mesh.vertexBuffer_PRE);
					}
					std::swap(mesh.streamoutBuffer_POS, mesh.vertexBuffer_PRE);
				}
			}

			if (raytracing_api)
			{
				if (mesh.streamoutBuffer_POS.IsValid())
				{
					for (auto& geometry : mesh.BLAS.desc.bottomlevel.geometries)
					{
						// swapped dynamic vertex buffers in BLAS:
						geometry.triangles.vertexBuffer = mesh.streamoutBuffer_POS;
					}
				}

				if (mesh.BLAS_build_pending)
				{
					mesh.BLAS_build_pending = false;
					pendingBottomLevelBuilds[entity] = AS_REBUILD;
				}
			}

		}
	});

	// Update Voxelization parameters:
	if (scene.objects.GetCount() > 0)
	{
		wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
			// We don't update it if the scene is empty, this even makes it easier to debug
			const float f = 0.05f / voxelSceneData.voxelsize;
			XMFLOAT3 center = XMFLOAT3(floorf(GetCamera().Eye.x * f) / f, floorf(GetCamera().Eye.y * f) / f, floorf(GetCamera().Eye.z * f) / f);
			if (wiMath::DistanceSquared(center, voxelSceneData.center) > 0)
			{
				voxelSceneData.centerChangedThisFrame = true;
			}
			else
			{
				voxelSceneData.centerChangedThisFrame = false;
			}
			voxelSceneData.center = center;
			voxelSceneData.extents = XMFLOAT3(voxelSceneData.res * voxelSceneData.voxelsize, voxelSceneData.res * voxelSceneData.voxelsize, voxelSceneData.res * voxelSceneData.voxelsize);
		});
	}

	// Perform parallel frustum culling and obtain closest reflector:
	wiSpinLock locker;
	float closestRefPlane = FLT_MAX;
	requestReflectionRendering = false;
	requestVolumetricLightRendering = false;
	auto range = wiProfiler::BeginRangeCPU("Frustum Culling");
	{
		// The parallel frustum culling is first performed in shared memory, 
		//	then each group writes out it's local list to global memory
		//	The shared memory approach reduces atomics and helps the list to remain
		//	more coherent (less randomly organized compared to original order)
		const uint32_t groupSize = 64;
		const size_t sharedmemory_size = (groupSize + 1) * sizeof(uint32_t); // list + counter per group

		for (auto& x : frameCullings)
		{
			auto& camera = x.first;
			FrameCulling& culling = x.second;
			culling.Clear();

			if (!freezeCullingCamera)
			{
				culling.frustum = camera->frustum;
			}

			// Cull objects for each camera:
			culling.culledObjects.resize(scene.aabb_objects.GetCount());
			wiJobSystem::Dispatch(ctx, (uint32_t)scene.aabb_objects.GetCount(), groupSize, [&](wiJobArgs args) {

				Entity entity = scene.aabb_objects.GetEntity(args.jobIndex);
				const LayerComponent* layer = scene.layers.GetComponent(entity);
				if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
				{
					return;
				}

				const AABB& aabb = scene.aabb_objects[args.jobIndex];

				// Setup stream compaction:
				uint32_t& group_count = *(uint32_t*)args.sharedmemory;
				uint32_t* group_list = (uint32_t*)args.sharedmemory + 1;
				if (args.isFirstJobInGroup)
				{
					group_count = 0; // first thread initializes local counter
				}

				if (culling.frustum.CheckBoxFast(aabb))
				{
					// Local stream compaction:
					group_list[group_count++] = args.jobIndex;

					// Main camera can request reflection rendering:
					if (camera == &GetCamera())
					{
						const ObjectComponent& object = scene.objects[args.jobIndex];
						if (object.IsRequestPlanarReflection())
						{
							float dist = wiMath::DistanceEstimated(camera->Eye, object.center);
							locker.lock();
							if (dist < closestRefPlane)
							{
								closestRefPlane = dist;
								const TransformComponent& transform = scene.transforms[object.transform_index];
								XMVECTOR P = transform.GetPositionV();
								XMVECTOR N = XMVectorSet(0, 1, 0, 0);
								N = XMVector3TransformNormal(N, XMLoadFloat4x4(&transform.world));
								XMVECTOR _refPlane = XMPlaneFromPointNormal(P, N);
								XMStoreFloat4(&waterPlane, _refPlane);

								requestReflectionRendering = true;
							}
							locker.unlock();
						}
					}
				}

				// Global stream compaction:
				if (args.isLastJobInGroup && group_count > 0)
				{
					uint32_t prev_count = culling.object_counter.fetch_add(group_count);
					for (uint32_t i = 0; i < group_count; ++i)
					{
						culling.culledObjects[prev_count + i] = group_list[i];
					}
				}

			}, sharedmemory_size);

			// the following cullings will be only for the main camera:
			if (camera == &GetCamera())
			{
				culling.culledDecals.resize(scene.aabb_decals.GetCount());
				wiJobSystem::Dispatch(ctx, (uint32_t)scene.aabb_decals.GetCount(), groupSize, [&](wiJobArgs args) {

					Entity entity = scene.aabb_decals.GetEntity(args.jobIndex);
					const LayerComponent* layer = scene.layers.GetComponent(entity);
					if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
					{
						return;
					}

					const AABB& aabb = scene.aabb_decals[args.jobIndex];

					// Setup stream compaction:
					uint32_t& group_count = *(uint32_t*)args.sharedmemory;
					uint32_t* group_list = (uint32_t*)args.sharedmemory + 1;
					if (args.isFirstJobInGroup)
					{
						group_count = 0; // first thread initializes local counter
					}

					if (culling.frustum.CheckBoxFast(aabb))
					{
						// Local stream compaction:
						group_list[group_count++] = args.jobIndex;
					}

					// Global stream compaction:
					if (args.isLastJobInGroup && group_count > 0)
					{
						uint32_t prev_count = culling.decal_counter.fetch_add(group_count);
						for (uint32_t i = 0; i < group_count; ++i)
						{
							culling.culledDecals[prev_count + i] = group_list[i];
						}
					}

				}, sharedmemory_size);

				wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
					// Cull probes:
					for (size_t i = 0; i < scene.aabb_probes.GetCount(); ++i)
					{
						Entity entity = scene.aabb_probes.GetEntity(i);
						const LayerComponent* layer = scene.layers.GetComponent(entity);
						if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
						{
							continue;
						}

						const AABB& aabb = scene.aabb_probes[i];

						if (culling.frustum.CheckBoxFast(aabb))
						{
							culling.culledEnvProbes.push_back((uint32_t)i);
						}
					}
				});

				// Cull lights:
				culling.culledLights.resize(scene.aabb_lights.GetCount());
				wiJobSystem::Dispatch(ctx, (uint32_t)scene.aabb_lights.GetCount(), groupSize, [&](wiJobArgs args) {

					Entity entity = scene.aabb_lights.GetEntity(args.jobIndex);
					const LayerComponent* layer = scene.layers.GetComponent(entity);
					if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
					{
						return;
					}

					const AABB& aabb = scene.aabb_lights[args.jobIndex];

					// Setup stream compaction:
					uint32_t& group_count = *(uint32_t*)args.sharedmemory;
					uint32_t* group_list = (uint32_t*)args.sharedmemory + 1;
					if (args.isFirstJobInGroup)
					{
						group_count = 0; // first thread initializes local counter
					}

					if (culling.frustum.CheckBoxFast(aabb))
					{
						// Local stream compaction:
						group_list[group_count++] = args.jobIndex;
					}

					// Global stream compaction:
					if (args.isLastJobInGroup && group_count > 0)
					{
						uint32_t prev_count = culling.light_counter.fetch_add(group_count);
						for (uint32_t i = 0; i < group_count; ++i)
						{
							culling.culledLights[prev_count + i] = group_list[i];
						}
					}

				}, sharedmemory_size);

				wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
					// Cull emitters:
					for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
					{
						Entity entity = scene.emitters.GetEntity(i);
						const LayerComponent* layer = scene.layers.GetComponent(entity);
						if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
						{
							continue;
						}
						culling.culledEmitters.push_back((uint32_t)i);
					}
				});

				wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
					// Cull hairs:
					for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
					{
						Entity entity = scene.hairs.GetEntity(i);
						const LayerComponent* layer = scene.layers.GetComponent(entity);
						if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
						{
							continue;
						}

						const wiHairParticle& hair = scene.hairs[i];
						if (hair.meshID == INVALID_ENTITY || !culling.frustum.CheckBoxFast(hair.aabb))
						{
							continue;
						}
						culling.culledHairs.push_back((uint32_t)i);
					}
				});
			}

		}
	}
	wiJobSystem::Wait(ctx);
	// finalize stream compaction:
	for (auto& x : frameCullings)
	{
		FrameCulling& culling = x.second;
		culling.culledObjects.resize((size_t)culling.object_counter.load());
		culling.culledLights.resize((size_t)culling.light_counter.load());
		culling.culledDecals.resize((size_t)culling.decal_counter.load());
	}
	wiProfiler::EndRange(range); // Frustum Culling

	// Light sorting:
	for (auto& x : frameCullings)
	{
		FrameCulling& culling = x.second;

		if (!culling.culledLights.empty())
		{
			wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
				// Sort lights based on distance so that closer lights will receive shadow map priority:
				const CameraComponent* camera = x.first;
				const size_t lightCount = culling.culledLights.size();
				assert(lightCount < 0x0000FFFF); // watch out for sorting hash truncation!
				uint32_t* lightSortingHashes = (uint32_t*)GetUpdateFrameAllocator().allocate(sizeof(uint32_t) * lightCount);
				for (size_t i = 0; i < lightCount; ++i)
				{
					const uint32_t lightIndex = culling.culledLights[i];
					const LightComponent& light = scene.lights[lightIndex];
					float distance = wiMath::DistanceEstimated(light.position, camera->Eye);
					lightSortingHashes[i] = 0;
					lightSortingHashes[i] |= (uint32_t)i & 0x0000FFFF;
					lightSortingHashes[i] |= ((uint32_t)(distance * 10) & 0x0000FFFF) << 16;
				}
				std::sort(lightSortingHashes, lightSortingHashes + lightCount, std::less<uint32_t>());

				uint32_t shadowCounter_2D = 0;
				uint32_t shadowCounter_Cube = 0;
				for (size_t i = 0; i < lightCount; ++i)
				{
					const uint32_t lightIndex = culling.culledLights[lightSortingHashes[i] & 0x0000FFFF];
					LightComponent& light = scene.lights[lightIndex];
					requestVolumetricLightRendering |= light.IsVolumetricsEnabled();

					// Link shadowmaps to lights till there are free slots

					light.shadowMap_index = -1;

					if (light.IsCastingShadow())
					{
						switch (light.GetType())
						{
						case LightComponent::DIRECTIONAL:
							if ((shadowCounter_2D + CASCADE_COUNT - 1) < SHADOWCOUNT_2D)
							{
								light.shadowMap_index = shadowCounter_2D;
								shadowCounter_2D += CASCADE_COUNT;
							}
							break;
						case LightComponent::SPOT:
							if (shadowCounter_2D < SHADOWCOUNT_2D)
							{
								light.shadowMap_index = shadowCounter_2D;
								shadowCounter_2D++;
							}
							break;
						case LightComponent::POINT:
						case LightComponent::SPHERE:
						case LightComponent::DISC:
						case LightComponent::RECTANGLE:
						case LightComponent::TUBE:
							if (shadowCounter_Cube < SHADOWCOUNT_CUBE)
							{
								light.shadowMap_index = shadowCounter_Cube;
								shadowCounter_Cube++;
							}
							break;
						default:
							break;
						}
					}
				}
			});
		}
	}

	// Ocean will override any current reflectors
	if (scene.weather.IsOceanEnabled())
	{
		requestReflectionRendering = true; 
		XMVECTOR _refPlane = XMPlaneFromPointNormal(XMVectorSet(0, scene.weather.oceanParameters.waterHeight, 0, 0), XMVectorSet(0, 1, 0, 0));
		XMStoreFloat4(&waterPlane, _refPlane);

		if (ocean == nullptr)
		{
			ocean = std::make_unique<wiOcean>(scene.weather);
		}
	}
	else if (ocean != nullptr)
	{
		ocean.reset();
	}

	wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
		ManageDecalAtlas();
	});
	wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
		ManageLightmapAtlas();
	});
	wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
		ManageImpostors();
	});
	wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
		ManageEnvProbes();
	});
	wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
		for (auto& x : waterRipples)
		{
			x.Update(dt * 60);
		}
		ManageWaterRipples();
	});

	wiJobSystem::Wait(ctx);
}
void UpdateRenderData(CommandList cmd)
{
	const Scene& scene = GetScene();

	BindCommonResources(cmd);

	// Update material constant buffers:
	for (auto& materialIndex : pendingMaterialUpdates)
	{
		if (materialIndex < scene.materials.GetCount())
		{
			const MaterialComponent& material = scene.materials[materialIndex];
			ShaderMaterial materialGPUData;
			material.WriteShaderMaterial(&materialGPUData);
			device->UpdateBuffer(&material.constantBuffer, &materialGPUData, cmd);
		}
	}
	pendingMaterialUpdates.clear();

	// Update mesh morph buffers:
	for (auto& meshIndex : pendingMorphUpdates)
	{
	    if (meshIndex < scene.meshes.GetCount())
	    {
			const MeshComponent& mesh = scene.meshes[meshIndex];
			device->UpdateBuffer(&mesh.vertexBuffer_POS, mesh.vertex_positions_morphed.data(), cmd);
	    }
	}
	pendingMorphUpdates.clear();


	if (scene.weather.IsRealisticSky())
	{
		// Render Atmospheric Scattering textures for lighting and sky
		RenderAtmosphericScatteringTextures(cmd);
	}

	const FrameCulling& mainCameraCulling = frameCullings.at(&GetCamera());

	// Fill Entity Array with decals + envprobes + lights in the frustum:
	{
		// Reserve temporary entity array for GPU data upload:
		ShaderEntity* entityArray = (ShaderEntity*)GetRenderFrameAllocator(cmd).allocate(sizeof(ShaderEntity)*SHADER_ENTITY_COUNT);
		XMMATRIX* matrixArray = (XMMATRIX*)GetRenderFrameAllocator(cmd).allocate(sizeof(XMMATRIX)*MATRIXARRAY_COUNT);

		const XMMATRIX viewMatrix = GetCamera().GetView();

		uint32_t entityCounter = 0;
		uint32_t matrixCounter = 0;

		entityArrayOffset_Lights = 0;
		entityArrayCount_Lights = 0;
		entityArrayOffset_Decals = 0;
		entityArrayCount_Decals = 0;
		entityArrayOffset_ForceFields = 0;
		entityArrayCount_ForceFields = 0;
		entityArrayOffset_EnvProbes = 0;
		entityArrayCount_EnvProbes = 0;

		// Write decals into entity array:
		entityArrayOffset_Decals = entityCounter;
		for (size_t i = 0; i < mainCameraCulling.culledDecals.size(); ++i)
		{
			if (entityCounter == SHADER_ENTITY_COUNT)
			{
				assert(0); // too many entities!
				entityCounter--;
				break;
			}
			if (matrixCounter >= MATRIXARRAY_COUNT)
			{
				assert(0); // too many decals, can't upload the rest to matrixarray!
				matrixCounter--;
				break;
			}
			const uint32_t decalIndex = mainCameraCulling.culledDecals[mainCameraCulling.culledDecals.size() - 1 - i]; // note: reverse order, for correct blending!
			const DecalComponent& decal = scene.decals[decalIndex];

			entityArray[entityCounter].SetType(ENTITY_TYPE_DECAL);
			entityArray[entityCounter].positionWS = decal.position;
			XMStoreFloat3(&entityArray[entityCounter].positionVS, XMVector3TransformCoord(XMLoadFloat3(&decal.position), viewMatrix));
			entityArray[entityCounter].range = decal.range;
			entityArray[entityCounter].texMulAdd = decal.atlasMulAdd;
			entityArray[entityCounter].color = wiMath::CompressColor(XMFLOAT4(decal.color.x, decal.color.y, decal.color.z, decal.GetOpacity()));
			entityArray[entityCounter].energy = decal.emissive;

			entityArray[entityCounter].userdata = matrixCounter;
			matrixArray[matrixCounter] = XMMatrixInverse(nullptr, XMLoadFloat4x4(&decal.world));
			matrixCounter++;

			entityCounter++;
		}
		entityArrayCount_Decals = entityCounter - entityArrayOffset_Decals;

		// Write environment probes into entity array:
		entityArrayOffset_EnvProbes = entityCounter;
		for (size_t i = 0; i < mainCameraCulling.culledEnvProbes.size(); ++i)
		{
			if (entityCounter == SHADER_ENTITY_COUNT)
			{
				assert(0); // too many entities!
				entityCounter--;
				break;
			}
			if (matrixCounter >= MATRIXARRAY_COUNT)
			{
				assert(0); // too many probes, can't upload the rest to matrixarray!
				matrixCounter--;
				break;
			}

			const uint32_t probeIndex = mainCameraCulling.culledEnvProbes[mainCameraCulling.culledEnvProbes.size() - 1 - i]; // note: reverse order, for correct blending!
			const EnvironmentProbeComponent& probe = scene.probes[probeIndex];
			if (probe.textureIndex < 0)
			{
				continue;
			}

			entityArray[entityCounter].SetType(ENTITY_TYPE_ENVMAP);
			entityArray[entityCounter].positionWS = probe.position;
			XMStoreFloat3(&entityArray[entityCounter].positionVS, XMVector3TransformCoord(XMLoadFloat3(&probe.position), viewMatrix));
			entityArray[entityCounter].range = probe.range;
			entityArray[entityCounter].shadowBias = (float)probe.textureIndex;

			entityArray[entityCounter].userdata = matrixCounter;
			matrixArray[matrixCounter] = XMLoadFloat4x4(&probe.inverseMatrix);
			matrixCounter++;

			entityCounter++;
		}
		entityArrayCount_EnvProbes = entityCounter - entityArrayOffset_EnvProbes;

		// Write lights into entity array:
		entityArrayOffset_Lights = entityCounter;
		for (uint32_t lightIndex : mainCameraCulling.culledLights)
		{
			if (entityCounter == SHADER_ENTITY_COUNT)
			{
				assert(0); // too many entities!
				entityCounter--;
				break;
			}

			const LightComponent& light = scene.lights[lightIndex];

			entityArray[entityCounter].SetType(light.GetType());
			entityArray[entityCounter].positionWS = light.position;
			XMStoreFloat3(&entityArray[entityCounter].positionVS, XMVector3TransformCoord(XMLoadFloat3(&entityArray[entityCounter].positionWS), viewMatrix));
			entityArray[entityCounter].range = light.GetRange();
			entityArray[entityCounter].color = wiMath::CompressColor(light.color);
			entityArray[entityCounter].energy = light.energy;
			entityArray[entityCounter].shadowBias = light.shadowBias;
			entityArray[entityCounter].userdata = ~0;

			const bool shadow = light.IsCastingShadow() && light.shadowMap_index >= 0;
			if (shadow)
			{
				entityArray[entityCounter].SetShadowIndices(matrixCounter, (uint)light.shadowMap_index);
			}

			switch (light.GetType())
			{
			case LightComponent::DIRECTIONAL:
			{
				entityArray[entityCounter].directionWS = light.direction;
				entityArray[entityCounter].shadowKernel = 1.0f / SHADOWRES_2D;

				if (shadow)
				{
					std::array<SHCAM, CASCADE_COUNT> shcams;
					CreateDirLightShadowCams(light, GetCamera(), shcams);
					matrixArray[matrixCounter++] = shcams[0].VP;
					matrixArray[matrixCounter++] = shcams[1].VP;
					matrixArray[matrixCounter++] = shcams[2].VP;
				}
			}
			break;
			case LightComponent::SPOT:
			{
				entityArray[entityCounter].coneAngleCos = cosf(light.fov * 0.5f);
				entityArray[entityCounter].directionWS = light.direction;
				XMStoreFloat3(&entityArray[entityCounter].directionVS, XMVector3TransformNormal(XMLoadFloat3(&entityArray[entityCounter].directionWS), viewMatrix));
				entityArray[entityCounter].shadowKernel = 1.0f / SHADOWRES_2D;

				if (shadow)
				{
					SHCAM shcam;
					CreateSpotLightShadowCam(light, shcam);
					matrixArray[matrixCounter++] = shcam.VP;
				}
			}
			break;
			case LightComponent::POINT:
			{
				entityArray[entityCounter].shadowKernel = 1.0f / SHADOWRES_CUBE;
			}
			break;
			case LightComponent::SPHERE:
			case LightComponent::DISC:
			case LightComponent::RECTANGLE:
			case LightComponent::TUBE:
			{
				// Note: area lights are facing back by default
				entityArray[entityCounter].directionWS = light.right;
				entityArray[entityCounter].directionVS = light.direction;
				entityArray[entityCounter].positionVS = light.front;
				entityArray[entityCounter].texMulAdd = XMFLOAT4(light.radius, light.width, light.height, 0);
			}
			break;
			}

			switch (light.GetType())
			{
			case LightComponent::POINT:
			case LightComponent::SPHERE:
			case LightComponent::DISC:
			case LightComponent::RECTANGLE:
			case LightComponent::TUBE:
			{
				const float FarZ = 0.1f;	// watch out: reversed depth buffer! Also, light near plane is constant for simplicity, this should match on cpu side!
				const float NearZ = std::max(1.0f, entityArray[entityCounter].range); // watch out: reversed depth buffer!
				const float fRange = FarZ / (FarZ - NearZ);
				const float cubemapDepthRemapNear = fRange;
				const float cubemapDepthRemapFar = -fRange * NearZ;
				entityArray[entityCounter].texMulAdd.w = cubemapDepthRemapNear;
				entityArray[entityCounter].coneAngleCos = cubemapDepthRemapFar;
			}
			break;
			}

			if (light.IsStatic())
			{
				entityArray[entityCounter].SetFlags(ENTITY_FLAG_LIGHT_STATIC);
			}

			entityCounter++;
		}
		entityArrayCount_Lights = entityCounter - entityArrayOffset_Lights;

		// Write force fields into entity array:
		entityArrayOffset_ForceFields = entityCounter;
		for (size_t i = 0; i < scene.forces.GetCount(); ++i)
		{
			if (entityCounter == SHADER_ENTITY_COUNT)
			{
				assert(0); // too many entities!
				entityCounter--;
				break;
			}

			const ForceFieldComponent& force = scene.forces[i];

			entityArray[entityCounter].SetType(force.type);
			entityArray[entityCounter].positionWS = force.position;
			entityArray[entityCounter].energy = force.gravity;
			entityArray[entityCounter].range = 1.0f / std::max(0.0001f, force.GetRange()); // avoid division in shader
			entityArray[entityCounter].coneAngleCos = force.GetRange(); // this will be the real range in the less common shaders...
			// The default planar force field is facing upwards, and thus the pull direction is downwards:
			entityArray[entityCounter].directionWS = force.direction;

			entityCounter++;
		}
		entityArrayCount_ForceFields = entityCounter - entityArrayOffset_ForceFields;

		// Issue GPU entity array update:
		device->UpdateBuffer(&resourceBuffers[RBTYPE_ENTITYARRAY], entityArray, cmd, sizeof(ShaderEntity)*entityCounter);
		device->UpdateBuffer(&resourceBuffers[RBTYPE_MATRIXARRAY], matrixArray, cmd, sizeof(XMMATRIX)*matrixCounter);

		// Temporary array for GPU entities can be freed now:
		GetRenderFrameAllocator(cmd).free(sizeof(ShaderEntity)*SHADER_ENTITY_COUNT);
		GetRenderFrameAllocator(cmd).free(sizeof(XMMATRIX)*MATRIXARRAY_COUNT);
	}

	UpdateFrameCB(cmd);

	GetPrevCamera() = GetCamera();

	auto range = wiProfiler::BeginRangeGPU("Skinning", cmd);
	device->EventBegin("Skinning", cmd);
	{
		bool streamOutSetUp = false;
		SHADERTYPE lastCS = CSTYPE_SKINNING_LDS;

		for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
		{
			Entity entity = scene.meshes.GetEntity(i);
			const MeshComponent& mesh = scene.meshes[i];

			if (mesh.IsSkinned() && scene.armatures.Contains(mesh.armatureID))
			{
				const SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(entity);
				if (softbody != nullptr && softbody->physicsobject != nullptr)
				{
					// If soft body simulation is active, don't perform skinning.
					//	(Soft body animated vertices are skinned in simulation phase by physics system)
					continue;
				}

				const ArmatureComponent& armature = *scene.armatures.GetComponent(mesh.armatureID);

				if (!streamOutSetUp)
				{
					// Set up skinning shader
					streamOutSetUp = true;
					GPUBuffer* vbs[] = {
						nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
					};
					const uint32_t strides[] = {
						0,0,0,0,0,0,0,0
					};
					device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
					device->BindComputeShader(&shaders[CSTYPE_SKINNING_LDS], cmd);
				}

				SHADERTYPE targetCS = CSTYPE_SKINNING_LDS;

				if (!GetLDSSkinningEnabled() || armature.boneData.size() > SKINNING_COMPUTE_THREADCOUNT)
				{
					// If we have more bones that can fit into LDS, we switch to a skinning shader which loads from device memory:
					targetCS = CSTYPE_SKINNING;
				}

				if (targetCS != lastCS)
				{
					lastCS = targetCS;
					device->BindComputeShader(&shaders[targetCS], cmd);
				}

				// Upload bones for skinning to shader
				device->UpdateBuffer(&armature.boneBuffer, armature.boneData.data(), cmd, (int)(sizeof(ArmatureComponent::ShaderBoneType) * armature.boneData.size()));

				// Do the skinning
				const GPUResource* vbs[] = {
					&mesh.vertexBuffer_POS,
					&mesh.vertexBuffer_TAN,
					&mesh.vertexBuffer_BON,
					&armature.boneBuffer,
				};
				const GPUResource* so[] = {
					&mesh.streamoutBuffer_POS,
					&mesh.streamoutBuffer_TAN,
				};

				device->BindResources(CS, vbs, SKINNINGSLOT_IN_VERTEX_POS, arraysize(vbs), cmd);
				device->BindUAVs(CS, so, 0, arraysize(so), cmd);

				device->Dispatch(((uint32_t)mesh.vertex_positions.size() + SKINNING_COMPUTE_THREADCOUNT - 1) / SKINNING_COMPUTE_THREADCOUNT, 1, 1, cmd);
			}

		}

		if (streamOutSetUp)
		{
			// wait all skinning to finish (but they can overlap)
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
			device->UnbindUAVs(0, 2, cmd);
			device->UnbindResources(SKINNINGSLOT_IN_VERTEX_POS, 4, cmd);
		}

	}
	device->EventEnd(cmd);
	wiProfiler::EndRange(range); // skinning

	// Update soft body vertex buffers:
	for (size_t i = 0; i < scene.softbodies.GetCount(); ++i)
	{
		const SoftBodyPhysicsComponent& softbody = scene.softbodies[i];
		if (softbody.vertex_positions_simulation.empty())
		{
			continue;
		}

		Entity entity = scene.softbodies.GetEntity(i);
		const MeshComponent& mesh = *scene.meshes.GetComponent(entity);

		device->UpdateBuffer(&mesh.streamoutBuffer_POS, softbody.vertex_positions_simulation.data(), cmd, 
			(uint32_t)(sizeof(MeshComponent::Vertex_POS) * softbody.vertex_positions_simulation.size()));

		device->UpdateBuffer(&mesh.streamoutBuffer_TAN, softbody.vertex_tangents_simulation.data(), cmd,
			(uint32_t)(sizeof(MeshComponent::Vertex_TAN)* softbody.vertex_tangents_simulation.size()));
	}

	// GPU Particle systems simulation/sorting/culling:
	if (!mainCameraCulling.culledEmitters.empty())
	{
		range = wiProfiler::BeginRangeGPU("EmittedParticles - Simulate", cmd);
		for (uint32_t emitterIndex : mainCameraCulling.culledEmitters)
		{
			const wiEmittedParticle& emitter = scene.emitters[emitterIndex];
			Entity entity = scene.emitters.GetEntity(emitterIndex);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);
			const MaterialComponent& material = *scene.materials.GetComponent(entity);
			const MeshComponent* mesh = scene.meshes.GetComponent(emitter.meshID);

			emitter.UpdateGPU(transform, material, mesh, cmd);
		}
		wiProfiler::EndRange(range);
	}

	// Hair particle systems GPU simulation:
	if (!mainCameraCulling.culledHairs.empty())
	{
		range = wiProfiler::BeginRangeGPU("HairParticles - Simulate", cmd);
		for (uint32_t hairIndex : mainCameraCulling.culledHairs)
		{
			const wiHairParticle& hair = scene.hairs[hairIndex];
			const MeshComponent* mesh = scene.meshes.GetComponent(hair.meshID);

			if (mesh != nullptr)
			{
				Entity entity = scene.hairs.GetEntity(hairIndex);
				const MaterialComponent& material = *scene.materials.GetComponent(entity);

				hair.UpdateGPU(*mesh, material, cmd);
			}
		}
		wiProfiler::EndRange(range);
	}

	// Compute water simulation:
	if (scene.weather.IsOceanEnabled() && ocean != nullptr)
	{
		range = wiProfiler::BeginRangeGPU("Ocean - Simulate", cmd);
		ocean->UpdateDisplacementMap(scene.weather, renderTime, cmd);
		wiProfiler::EndRange(range);
	}
}
void UpdateRaytracingAccelerationStructures(CommandList cmd)
{
	const Scene& scene = GetScene();
	if (!device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING) || !scene.TLAS.IsValid())
	{
		return;
	}
	auto range = wiProfiler::BeginRangeGPU("Update Raytracing Acceleration Structures", cmd);
	device->EventBegin("Update Raytracing Acceleration Structures", cmd);

	bool bottomlevel_sync = false;

	// Build bottom level:
	for(auto& it : pendingBottomLevelBuilds)
	{
		AS_UPDATE_TYPE type = it.second;
		if (type == AS_COMPLETE)
			continue;

		Entity entity = it.first;
		const MeshComponent* mesh = scene.meshes.GetComponent(entity);

		if (mesh != nullptr)
		{
			// If src param is nullptr, rebuild happens, else update (if src == dst, then update happens in place)
			//device->BuildRaytracingAccelerationStructure(&mesh->BLAS, cmd, type == AS_REBUILD ? nullptr : &mesh->BLAS);
			device->BuildRaytracingAccelerationStructure(&mesh->BLAS, cmd, nullptr); // DX12 has some problem updating with new driver?
			bottomlevel_sync = true;
			it.second = AS_COMPLETE;
		}
	}

	// Gather all instances for top level:
	size_t instanceSize = device->GetTopLevelAccelerationStructureInstanceSize();
	size_t instanceArraySize = (size_t)scene.TLAS.desc.toplevel.count * instanceSize;
	void* instanceArray = GetRenderFrameAllocator(cmd).allocate(instanceArraySize);
	size_t instanceOffset = 0;
	for (size_t i = 0; i < scene.objects.GetCount(); ++i)
	{
		const ObjectComponent& object = scene.objects[i];

		if (object.meshID != INVALID_ENTITY)
		{
			const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);

			RaytracingAccelerationStructureDesc::TopLevel::Instance instance = {};
			const XMFLOAT4X4& transform = object.transform_index >= 0 ? scene.transforms[object.transform_index].world : IDENTITYMATRIX;
			instance = {};
			instance.transform = XMFLOAT3X4(
				transform._11, transform._21, transform._31, transform._41,
				transform._12, transform._22, transform._32, transform._42,
				transform._13, transform._23, transform._33, transform._43
			);
			instance.InstanceID = mesh.TLAS_geometryOffset;
			instance.InstanceMask = 1;
			instance.bottomlevel = mesh.BLAS;

			device->WriteTopLevelAccelerationStructureInstance(&instance, (void*)((size_t)instanceArray + instanceOffset));
			instanceOffset += instanceSize;
		}
	}
	device->UpdateBuffer(&scene.TLAS.desc.toplevel.instanceBuffer, instanceArray, cmd, (int)instanceArraySize);
	GetRenderFrameAllocator(cmd).free(instanceArraySize);

	// Sync with bottom level before building top level:
	if (bottomlevel_sync)
	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	// Build top level:
	device->BuildRaytracingAccelerationStructure(&scene.TLAS, cmd, nullptr);
	GPUBarrier barriers[] = {
		GPUBarrier::Memory(&scene.TLAS),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->EventEnd(cmd);
	wiProfiler::EndRange(range);
}
void OcclusionCulling_Render(CommandList cmd)
{
	if (!GetOcclusionCullingEnabled() || GetFreezeCullingCameraEnabled())
	{
		return;
	}

	const FrameCulling& culling = frameCullings.at(&GetCamera());

	auto range = wiProfiler::BeginRangeGPU("Occlusion Culling Render", cmd);

	int queryID = 0;

	if (!culling.culledObjects.empty())
	{
		device->EventBegin("Occlusion Culling Render", cmd);

		device->BindPipelineState(&PSO_occlusionquery, cmd);

		// TODO: This is not const, so not thread safe!
		Scene& scene = GetScene();

		int queryID = 0;

		MiscCB cb;

		for (uint32_t instanceIndex : culling.culledObjects)
		{
			ObjectComponent& object = scene.objects[instanceIndex];
			if (!object.IsRenderable())
			{
				continue;
			}

			if (queryID >= arraysize(occlusionQueries))
			{
				object.occlusionQueryID = -1; // assign an invalid id from the pool
				continue;
			}

			// If a query could be retrieved from the pool for the instance, the instance can be occluded, so render it
			GPUQuery* query = occlusionQueries[queryID].Get_GPU();
			if (query == nullptr || !query->IsValid())
			{
				continue;
			}

			const AABB& aabb = scene.aabb_objects[instanceIndex];

			if (aabb.intersects(GetCamera().Eye))
			{
				// camera is inside the instance, mark it as visible in this frame:
				object.occlusionHistory |= 1;
			}
			else
			{
				// only query for occlusion if the camera is outside the instance
				object.occlusionQueryID = queryID; // just assign the id from the pool
				queryID++;

				// previous frame view*projection because these are drawn against the previous depth buffer:
				XMStoreFloat4x4(&cb.g_xTransform, aabb.getAsBoxMatrix()*GetPrevCamera().GetViewProjection()); // todo: obb
				device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &cb, cmd);
				device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

				// render bounding box to later read the occlusion status
				device->QueryBegin(query, cmd);
				device->Draw(14, 0, cmd);
				device->QueryEnd(query, cmd);
			}
		}

		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range); // Occlusion Culling Render
}
void OcclusionCulling_Read()
{
	if (!GetOcclusionCullingEnabled() || GetFreezeCullingCameraEnabled())
	{
		return;
	}

	auto range = wiProfiler::BeginRangeCPU("Occlusion Culling Read");

	const FrameCulling& culling = frameCullings.at(&GetCamera());

	if (!culling.culledObjects.empty())
	{
		Scene& scene = GetScene();

		for (uint32_t instanceIndex : culling.culledObjects)
		{
			ObjectComponent& object = scene.objects[instanceIndex];
			if (!object.IsRenderable())
			{
				continue;
			}

			object.occlusionHistory <<= 1; // advance history by 1 frame
			if (object.occlusionQueryID < 0)
			{
				// object doesn't have an occlusion query
				object.occlusionHistory |= 1; // mark this frame as visible
				continue;
			}

			GPUQuery* query = occlusionQueries[object.occlusionQueryID].Get_CPU();
			if (query == nullptr || !query->IsValid())
			{
				// occlusion query not available for CPU read
				object.occlusionHistory |= 1; // mark this frame as visible
				continue;
			}

			GPUQueryResult query_result;
			bool query_ready = device->QueryRead(query, &query_result);

			if (query_result.result_passed_sample_count > 0 || !query_ready)
			{
				object.occlusionHistory |= 1; // mark this frame as visible
			}
			else
			{
				// leave this frame as occluded
			}
		}
	}

	wiProfiler::EndRange(range); // Occlusion Culling Read
}
void EndFrame()
{
	OcclusionCulling_Read();

	updateFrameAllocator.reset();
	for (int i = 0; i < COMMANDLIST_COUNT; ++i)
	{
		renderFrameAllocators[i].reset();
	}
}

void PutWaterRipple(const std::string& image, const XMFLOAT3& pos)
{
	wiSprite img(image);
	img.anim.fad = 0.01f;
	img.anim.scaleX = 0.2f;
	img.anim.scaleY = 0.2f;
	img.params.pos = pos;
	img.params.rotation = (wiRandom::getRandom(0, 1000)*0.001f) * 2 * 3.1415f;
	img.params.siz = XMFLOAT2(1, 1);
	img.params.typeFlag = WORLD;
	img.params.quality = QUALITY_ANISOTROPIC;
	img.params.pivot = XMFLOAT2(0.5f, 0.5f);
	img.params.lookAt = waterPlane;
	img.params.lookAt.w = 1;
	waterRipples.push_back(img);
}
void ManageWaterRipples(){
	while (
		!waterRipples.empty() &&
		(waterRipples.front().params.opacity <= 0 + FLT_EPSILON || waterRipples.front().params.fade == 1)
		)
	{
		waterRipples.pop_front();
	}
}
void DrawWaterRipples(CommandList cmd)
{
	device->EventBegin("Water Ripples", cmd);
	for(auto& x : waterRipples)
	{
		x.DrawNormal(cmd);
	}
	device->EventEnd(cmd);
}



void DrawSoftParticles(
	const CameraComponent& camera,
	const Texture& lineardepth,
	bool distortion, 
	CommandList cmd
)
{
	const Scene& scene = GetScene();
	const FrameCulling& culling = frameCullings.at(&camera);
	size_t emitterCount = culling.culledEmitters.size();
	if (emitterCount == 0)
	{
		return;
	}

	device->BindResource(PS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	// Sort emitters based on distance:
	assert(emitterCount < 0x0000FFFF); // watch out for sorting hash truncation!
	uint32_t* emitterSortingHashes = (uint32_t*)GetRenderFrameAllocator(cmd).allocate(sizeof(uint32_t) * emitterCount);
	for (size_t i = 0; i < emitterCount; ++i)
	{
		const uint32_t emitterIndex = culling.culledEmitters[i];
		const wiEmittedParticle& emitter = scene.emitters[emitterIndex];
		float distance = wiMath::DistanceEstimated(emitter.center, camera.Eye);
		emitterSortingHashes[i] = 0;
		emitterSortingHashes[i] |= (uint32_t)i & 0x0000FFFF;
		emitterSortingHashes[i] |= ((uint32_t)(distance * 10) & 0x0000FFFF) << 16;
	}
	std::sort(emitterSortingHashes, emitterSortingHashes + emitterCount, std::greater<uint32_t>());

	for (size_t i = 0; i < emitterCount; ++i)
	{
		const uint32_t emitterIndex = culling.culledEmitters[emitterSortingHashes[i] & 0x0000FFFF];
		const wiEmittedParticle& emitter = scene.emitters[emitterIndex];
		const Entity entity = scene.emitters.GetEntity(emitterIndex);
		const MaterialComponent& material = *scene.materials.GetComponent(entity);

		if (distortion && emitter.shaderType == wiEmittedParticle::SOFT_DISTORTION)
		{
			emitter.Draw(camera, material, cmd);
		}
		else if (!distortion && (emitter.shaderType == wiEmittedParticle::SOFT || emitter.shaderType == wiEmittedParticle::SOFT_LIGHTING || emitter.shaderType == wiEmittedParticle::SIMPLEST || IsWireRender()))
		{
			emitter.Draw(camera, material, cmd);
		}
	}

	GetRenderFrameAllocator(cmd).free(sizeof(uint32_t) * emitterCount);

}
void DrawLightVisualizers(
	const CameraComponent& camera, 
	CommandList cmd
)
{
	const FrameCulling& culling = frameCullings.at(&camera);

	if (!culling.culledLights.empty())
	{
		const Scene& scene = GetScene();

		device->EventBegin("Light Visualizer Render", cmd);

		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), cmd);

		XMMATRIX camrot = XMLoadFloat3x3(&camera.rotationMatrix);


		for (int type = LightComponent::POINT; type < LightComponent::LIGHTTYPE_COUNT; ++type)
		{
			device->BindPipelineState(&PSO_lightvisualizer[type], cmd);

			for (uint32_t lightIndex : culling.culledLights)
			{
				const LightComponent& light = scene.lights[lightIndex];

				if (light.GetType() == type && light.IsVisualizerEnabled())
				{

					VolumeLightCB lcb;
					lcb.lightColor = XMFLOAT4(light.color.x, light.color.y, light.color.z, 1);
					lcb.lightEnerdis = XMFLOAT4(light.energy, light.GetRange(), light.fov, light.energy);

					if (type == LightComponent::POINT)
					{
						lcb.lightEnerdis.w = light.GetRange()*light.energy*0.01f; // scale
						XMStoreFloat4x4(&lcb.lightWorld, 
							XMMatrixScaling(lcb.lightEnerdis.w, lcb.lightEnerdis.w, lcb.lightEnerdis.w)*
							camrot*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))
						);

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(108, 0, cmd); // circle
					}
					else if (type == LightComponent::SPOT)
					{
						float coneS = (float)(light.fov / 0.7853981852531433);
						lcb.lightEnerdis.w = light.GetRange()*light.energy*0.03f; // scale
						XMStoreFloat4x4(&lcb.lightWorld, 
							XMMatrixScaling(coneS*lcb.lightEnerdis.w, lcb.lightEnerdis.w, coneS*lcb.lightEnerdis.w)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))
						);

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(192, 0, cmd); // cone
					}
					else if (type == LightComponent::SPHERE)
					{
						XMStoreFloat4x4(&lcb.lightWorld, 
							XMMatrixScaling(light.radius, light.radius, light.radius)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))*
							camera.GetViewProjection()
						);

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(2880, 0, cmd); // uv-sphere
					}
					else if (type == LightComponent::DISC)
					{
						XMStoreFloat4x4(&lcb.lightWorld, 
							XMMatrixScaling(light.radius, light.radius, light.radius)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))*
							camera.GetViewProjection()
						);

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(108, 0, cmd); // circle
					}
					else if (type == LightComponent::RECTANGLE)
					{
						XMStoreFloat4x4(&lcb.lightWorld, 
							XMMatrixScaling(light.width * 0.5f * light.scale.x, light.height * 0.5f * light.scale.y, 0.5f)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))*
							camera.GetViewProjection()
						);

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(6, 0, cmd); // quad
					}
					else if (type == LightComponent::TUBE)
					{
						XMStoreFloat4x4(&lcb.lightWorld, 
							XMMatrixScaling(std::max(light.width * 0.5f, light.radius) * light.scale.x, light.radius, light.radius)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))*
							camera.GetViewProjection()
						);

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(384, 0, cmd); // cylinder
					}
				}
			}

		}

		device->EventEnd(cmd);

	}
}
void DrawVolumeLights(
	const CameraComponent& camera,
	const Texture& depthbuffer,
	CommandList cmd
)
{
	const FrameCulling& culling = frameCullings.at(&camera);

	if (!culling.culledLights.empty())
	{
		device->EventBegin("Volumetric Light Render", cmd);

		BindShadowmaps(PS, cmd);

		device->BindResource(PS, &depthbuffer, TEXSLOT_DEPTH, cmd);

		const Scene& scene = GetScene();

		for (int type = 0; type < LightComponent::LIGHTTYPE_COUNT; ++type)
		{
			const PipelineState& pso = PSO_volumetriclight[type];

			if (!pso.IsValid())
			{
				continue;
			}

			device->BindPipelineState(&pso, cmd);

			for (size_t i = 0; i < culling.culledLights.size(); ++i)
			{
				const uint32_t lightIndex = culling.culledLights[i];
				const LightComponent& light = scene.lights[lightIndex];
				if (light.GetType() == type && light.IsVolumetricsEnabled())
				{

					switch (type)
					{
					case LightComponent::DIRECTIONAL:
					case LightComponent::SPHERE:
					case LightComponent::DISC:
					case LightComponent::RECTANGLE:
					case LightComponent::TUBE:
					{
						MiscCB miscCb;
						miscCb.g_xColor.x = float(entityArrayOffset_Lights + i);
						device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &miscCb, cmd);
						device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
						device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

						device->Draw(3, 0, cmd); // full screen triangle
					}
					break;
					case LightComponent::POINT:
					{
						MiscCB miscCb;
						miscCb.g_xColor.x = float(entityArrayOffset_Lights + i);
						float sca = light.GetRange() + 1;
						XMStoreFloat4x4(&miscCb.g_xTransform, XMMatrixScaling(sca, sca, sca)*XMMatrixTranslationFromVector(XMLoadFloat3(&light.position)) * camera.GetViewProjection());
						device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &miscCb, cmd);
						device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
						device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

						device->Draw(240, 0, cmd); // icosphere
					}
					break;
					case LightComponent::SPOT:
					{
						MiscCB miscCb;
						miscCb.g_xColor.x = float(entityArrayOffset_Lights + i);
						const float coneS = (const float)(light.fov / XM_PIDIV4);
						XMStoreFloat4x4(&miscCb.g_xTransform, 
							XMMatrixScaling(coneS*light.GetRange(), light.GetRange(), coneS*light.GetRange())*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position)) *
							camera.GetViewProjection()
						);
						device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &miscCb, cmd);
						device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
						device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

						device->Draw(192, 0, cmd); // cone
					}
					break;
					}

				}
			}

		}

		device->EventEnd(cmd);
	}


}
void DrawLensFlares(
	const CameraComponent& camera,
	const Texture& depthbuffer,
	CommandList cmd
)
{
	if (IsWireRender())
		return;

	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("Lens Flares", cmd);

	const FrameCulling& culling = frameCullings.at(&camera);

	const Scene& scene = GetScene();

	device->BindResource(GS, &depthbuffer, TEXSLOT_DEPTH, cmd);

	for (uint32_t lightIndex : culling.culledLights)
	{
		const LightComponent& light = scene.lights[lightIndex];

		if (!light.lensFlareRimTextures.empty())
		{
			XMVECTOR POS;

			if (light.GetType() == LightComponent::POINT || light.GetType() == LightComponent::SPOT)
			{
				// point and spotlight flare will be placed to the source position:
				POS = XMLoadFloat3(&light.position);
			}
			else
			{
				// directional light flare will be placed at infinite position along direction vector:
				POS = 
					camera.GetEye() + 
					XMVector3Normalize(-XMVector3Transform(XMVectorSet(0, -1, 0, 1), XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation)))) * camera.zFarP;
			}

			if (XMVectorGetX(XMVector3Dot(XMVectorSubtract(POS, camera.GetEye()), camera.GetAt())) > 0) // check if the camera is facing towards the flare or not
			{
				device->BindPipelineState(&PSO_lensflare, cmd);

				// Get the screen position of the flare:
				XMVECTOR flarePos = XMVector3Project(POS, 0, 0, 1, 1, 1, 0, camera.GetProjection(), camera.GetView(), XMMatrixIdentity());
				LensFlareCB cb;
				XMStoreFloat4(&cb.xSunPos, flarePos);
				cb.xScreen = XMFLOAT4((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y, 0, 0);

				device->UpdateBuffer(&constantBuffers[CBTYPE_LENSFLARE], &cb, cmd);
				device->BindConstantBuffer(GS, &constantBuffers[CBTYPE_LENSFLARE], CB_GETBINDSLOT(LensFlareCB), cmd);

				uint32_t i = 0;
				for (auto& x : light.lensFlareRimTextures)
				{
					if (x != nullptr)
					{
						device->BindResource(PS, x->texture, TEXSLOT_ONDEMAND0 + i, cmd);
						device->BindResource(GS, x->texture, TEXSLOT_ONDEMAND0 + i, cmd);
						i++;
						if (i == 7)
						{
							break; // currently the pixel shader has hardcoded max amount of lens flare textures...
						}
					}
				}

				device->Draw(i, 0, cmd);
			}

		}

	}

	device->EventEnd(cmd);
}


void SetShadowProps2D(int resolution, int count, int softShadowQuality)
{
	if (resolution >= 0)
	{
		SHADOWRES_2D = resolution;
	}
	if (count >= 0)
	{
		SHADOWCOUNT_2D = count;
	}
	if (softShadowQuality >= 0)
	{
		SOFTSHADOWQUALITY_2D = softShadowQuality;
	}

	if (SHADOWCOUNT_2D > 0 && SHADOWRES_2D > 0)
	{
		TextureDesc desc;
		desc.Width = SHADOWRES_2D;
		desc.Height = SHADOWRES_2D;
		desc.MipLevels = 1;
		desc.ArraySize = SHADOWCOUNT_2D;
		desc.SampleCount = 1;
		desc.Usage = USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R16_TYPELESS;
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		device->CreateTexture(&desc, nullptr, &shadowMapArray_2D);

		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R8G8B8A8_UNORM;
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		// RGB: Shadow tint (multiplicative), A: Refraction caustics(additive)
		desc.clear.color[0] = 1;
		desc.clear.color[1] = 1;
		desc.clear.color[2] = 1;
		desc.clear.color[3] = 0;
		device->CreateTexture(&desc, nullptr, &shadowMapArray_Transparent);

		renderpasses_shadow2D.resize(SHADOWCOUNT_2D);
		renderpasses_shadow2DTransparent.resize(SHADOWCOUNT_2D);

		for (uint32_t i = 0; i < SHADOWCOUNT_2D; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&shadowMapArray_2D, DSV, i, 1, 0, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&shadowMapArray_Transparent, RTV, i, 1, 0, 1);
			assert(subresource_index == i);

			RenderPassDesc renderpassdesc;

			renderpassdesc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					&shadowMapArray_2D,
					RenderPassAttachment::LOADOP_CLEAR,
					RenderPassAttachment::STOREOP_STORE,
					IMAGE_LAYOUT_SHADER_RESOURCE,
					IMAGE_LAYOUT_DEPTHSTENCIL,
					IMAGE_LAYOUT_SHADER_RESOURCE
				)
			);
			renderpassdesc.attachments.back().subresource = subresource_index;
			
			device->CreateRenderPass(&renderpassdesc, &renderpasses_shadow2D[subresource_index]);

			renderpassdesc.attachments.back().loadop = RenderPassAttachment::LOADOP_LOAD;

			renderpassdesc.attachments.push_back(
				RenderPassAttachment::RenderTarget(
					&shadowMapArray_Transparent,
					RenderPassAttachment::LOADOP_CLEAR,
					RenderPassAttachment::STOREOP_STORE,
					IMAGE_LAYOUT_SHADER_RESOURCE,
					IMAGE_LAYOUT_RENDERTARGET,
					IMAGE_LAYOUT_SHADER_RESOURCE
				)
			);
			renderpassdesc.attachments.back().subresource = subresource_index;
			
			device->CreateRenderPass(&renderpassdesc, &renderpasses_shadow2DTransparent[subresource_index]);
		}
	}

}
void SetShadowPropsCube(int resolution, int count)
{
	if (resolution >= 0)
	{
		SHADOWRES_CUBE = resolution;
	}
	if (count >= 0)
	{
		SHADOWCOUNT_CUBE = count;
	}

	if (SHADOWCOUNT_CUBE > 0 && SHADOWRES_CUBE > 0)
	{
		TextureDesc desc;
		desc.Width = SHADOWRES_CUBE;
		desc.Height = SHADOWRES_CUBE;
		desc.MipLevels = 1;
		desc.ArraySize = 6 * SHADOWCOUNT_CUBE;
		desc.Format = FORMAT_R16_TYPELESS;
		desc.SampleCount = 1;
		desc.Usage = USAGE_DEFAULT;
		desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		device->CreateTexture(&desc, nullptr, &shadowMapArray_Cube);

		renderpasses_shadowCube.resize(SHADOWCOUNT_CUBE);

		for (uint32_t i = 0; i < SHADOWCOUNT_CUBE; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&shadowMapArray_Cube, DSV, i * 6, 6, 0, 1);
			assert(subresource_index == i);

			RenderPassDesc renderpassdesc;
			renderpassdesc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					&shadowMapArray_Cube,
					RenderPassAttachment::LOADOP_CLEAR,
					RenderPassAttachment::STOREOP_STORE,
					IMAGE_LAYOUT_SHADER_RESOURCE,
					IMAGE_LAYOUT_DEPTHSTENCIL,
					IMAGE_LAYOUT_SHADER_RESOURCE
				)
			);
			renderpassdesc.attachments.back().subresource = subresource_index;
			device->CreateRenderPass(&renderpassdesc, &renderpasses_shadowCube[subresource_index]);
		}
	}

}
void DrawShadowmaps(const CameraComponent& camera, CommandList cmd, uint32_t layerMask)
{
	if (IsWireRender())
		return;

	const FrameCulling& culling = frameCullings.at(&GetCamera());

	if (!culling.culledLights.empty())
	{
		device->EventBegin("DrawShadowmaps", cmd);
		auto range = wiProfiler::BeginRangeGPU("Shadow Rendering", cmd);

		BindCommonResources(cmd);
		BindConstantBuffers(VS, cmd);
		BindConstantBuffers(PS, cmd);

		BoundingFrustum cam_frustum;
		BoundingFrustum::CreateFromMatrix(cam_frustum, camera.GetProjection());
		std::swap(cam_frustum.Near, cam_frustum.Far);
		cam_frustum.Transform(cam_frustum, camera.GetInvView());
		XMStoreFloat4(&cam_frustum.Orientation, XMQuaternionNormalize(XMLoadFloat4(&cam_frustum.Orientation)));

		const Scene& scene = GetScene();

		device->UnbindResources(TEXSLOT_SHADOWARRAY_2D, 2, cmd);

		for (uint32_t lightIndex : culling.culledLights)
		{
			const LightComponent& light = scene.lights[lightIndex];
			if (light.shadowMap_index < 0 || !light.IsCastingShadow() || light.IsStatic())
			{
				continue;
			}

			switch (light.GetType())
			{
			case LightComponent::DIRECTIONAL:
			{
				std::array<SHCAM, CASCADE_COUNT> shcams;
				CreateDirLightShadowCams(light, camera, shcams);

				for (uint32_t cascade = 0; cascade < CASCADE_COUNT; ++cascade)
				{
					RenderQueue renderQueue;
					bool transparentShadowsRequested = false;
					for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
					{
						const AABB& aabb = scene.aabb_objects[i];
						if (shcams[cascade].frustum.CheckBoxFast(aabb))
						{
							const ObjectComponent& object = scene.objects[i];
							if (object.IsRenderable() && cascade >= object.cascadeMask && object.IsCastingShadow())
							{
								Entity cullable_entity = scene.aabb_objects.GetEntity(i);
								const LayerComponent* layer = scene.layers.GetComponent(cullable_entity);
								if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
								{
									continue;
								}

								RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
								size_t meshIndex = scene.meshes.GetIndex(object.meshID);
								batch->Create(meshIndex, i, 0);
								renderQueue.add(batch);

								if (object.GetRenderTypes() & RENDERTYPE_TRANSPARENT || object.GetRenderTypes() & RENDERTYPE_WATER)
								{
									transparentShadowsRequested = true;
								}
							}
						}
					}
					if (!renderQueue.empty())
					{
						CameraCB cb;
						XMStoreFloat4x4(&cb.g_xCamera_VP, shcams[cascade].VP);
						device->UpdateBuffer(&constantBuffers[CBTYPE_CAMERA], &cb, cmd);

						Viewport vp;
						vp.TopLeftX = 0;
						vp.TopLeftY = 0;
						vp.Width = (float)SHADOWRES_2D;
						vp.Height = (float)SHADOWRES_2D;
						vp.MinDepth = 0.0f;
						vp.MaxDepth = 1.0f;
						device->BindViewports(1, &vp, cmd);

						device->RenderPassBegin(&renderpasses_shadow2D[light.shadowMap_index + cascade], cmd);
						RenderMeshes(renderQueue, RENDERPASS_SHADOW, RENDERTYPE_OPAQUE, cmd);
						device->RenderPassEnd(cmd);

						// Transparent renderpass will always be started so that it is clear:
						device->RenderPassBegin(&renderpasses_shadow2DTransparent[light.shadowMap_index + cascade], cmd);
						if (GetTransparentShadowsEnabled() && transparentShadowsRequested)
						{
							RenderMeshes(renderQueue, RENDERPASS_SHADOW, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, cmd);
						}
						device->RenderPassEnd(cmd);

						GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
					}

				}
			}
			break;
			case LightComponent::SPOT:
			{
				SHCAM shcam;
				CreateSpotLightShadowCam(light, shcam);
				if (!cam_frustum.Intersects(shcam.boundingfrustum))
					break;

				RenderQueue renderQueue;
				bool transparentShadowsRequested = false;
				for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
				{
					const AABB& aabb = scene.aabb_objects[i];
					if (shcam.frustum.CheckBoxFast(aabb))
					{
						const ObjectComponent& object = scene.objects[i];
						if (object.IsRenderable() && object.IsCastingShadow())
						{
							Entity cullable_entity = scene.aabb_objects.GetEntity(i);
							const LayerComponent* layer = scene.layers.GetComponent(cullable_entity);
							if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
							{
								continue;
							}

							RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
							size_t meshIndex = scene.meshes.GetIndex(object.meshID);
							batch->Create(meshIndex, i, 0);
							renderQueue.add(batch);

							if (object.GetRenderTypes() & RENDERTYPE_TRANSPARENT || object.GetRenderTypes() & RENDERTYPE_WATER)
							{
								transparentShadowsRequested = true;
							}
						}
					}
				}
				if (!renderQueue.empty())
				{
					CameraCB cb;
					XMStoreFloat4x4(&cb.g_xCamera_VP, shcam.VP);
					device->UpdateBuffer(&constantBuffers[CBTYPE_CAMERA], &cb, cmd);

					Viewport vp;
					vp.TopLeftX = 0;
					vp.TopLeftY = 0;
					vp.Width = (float)SHADOWRES_2D;
					vp.Height = (float)SHADOWRES_2D;
					vp.MinDepth = 0.0f;
					vp.MaxDepth = 1.0f;
					device->BindViewports(1, &vp, cmd);

					device->RenderPassBegin(&renderpasses_shadow2D[light.shadowMap_index], cmd);
					RenderMeshes(renderQueue, RENDERPASS_SHADOW, RENDERTYPE_OPAQUE, cmd);
					device->RenderPassEnd(cmd);

					// Transparent renderpass will always be started so that it is clear:
					device->RenderPassBegin(&renderpasses_shadow2DTransparent[light.shadowMap_index], cmd);
					if (GetTransparentShadowsEnabled() && transparentShadowsRequested)
					{
						RenderMeshes(renderQueue, RENDERPASS_SHADOW, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, cmd);
					}
					device->RenderPassEnd(cmd);

					GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
				}

			}
			break;
			case LightComponent::POINT:
			case LightComponent::SPHERE:
			case LightComponent::DISC:
			case LightComponent::RECTANGLE:
			case LightComponent::TUBE:
			{
				SPHERE boundingsphere = SPHERE(light.position, light.GetRange());

				RenderQueue renderQueue;
				for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
				{
					const AABB& aabb = scene.aabb_objects[i];
					if (boundingsphere.intersects(aabb))
					{
						const ObjectComponent& object = scene.objects[i];
						if (object.IsRenderable() && object.IsCastingShadow() && object.GetRenderTypes() == RENDERTYPE_OPAQUE)
						{
							Entity cullable_entity = scene.aabb_objects.GetEntity(i);
							const LayerComponent* layer = scene.layers.GetComponent(cullable_entity);
							if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
							{
								continue;
							}

							RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
							size_t meshIndex = scene.meshes.GetIndex(object.meshID);
							batch->Create(meshIndex, i, 0);
							renderQueue.add(batch);
						}
					}
				}
				if (!renderQueue.empty())
				{
					MiscCB miscCb;
					miscCb.g_xColor = float4(light.position.x, light.position.y, light.position.z, 0);
					device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &miscCb, cmd);
					device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
					device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

					const float zNearP = 0.1f;
					const float zFarP = std::max(1.0f, light.GetRange());
					SHCAM cameras[] = {
						SHCAM(light.position, XMFLOAT4(0.5f, -0.5f, -0.5f, -0.5f), zNearP, zFarP, XM_PIDIV2), //+x
						SHCAM(light.position, XMFLOAT4(0.5f, 0.5f, 0.5f, -0.5f), zNearP, zFarP, XM_PIDIV2), //-x
						SHCAM(light.position, XMFLOAT4(1, 0, 0, -0), zNearP, zFarP, XM_PIDIV2), //+y
						SHCAM(light.position, XMFLOAT4(0, 0, 0, -1), zNearP, zFarP, XM_PIDIV2), //-y
						SHCAM(light.position, XMFLOAT4(0.707f, 0, 0, -0.707f), zNearP, zFarP, XM_PIDIV2), //+z
						SHCAM(light.position, XMFLOAT4(0, 0.707f, 0.707f, 0), zNearP, zFarP, XM_PIDIV2), //-z
					};
					Frustum frusta[arraysize(cameras)];
					uint32_t frustum_count = 0;

					CubemapRenderCB cb;
					for (uint32_t shcam = 0; shcam < arraysize(cameras); ++shcam)
					{
						if (cam_frustum.Intersects(cameras[shcam].boundingfrustum))
						{
							XMStoreFloat4x4(&cb.xCubemapRenderCams[frustum_count].VP, cameras[shcam].VP);
							cb.xCubemapRenderCams[frustum_count].properties = uint4(shcam, 0, 0, 0);
							frusta[frustum_count] = cameras[shcam].frustum;
							frustum_count++;
						}
					}
					device->UpdateBuffer(&constantBuffers[CBTYPE_CUBEMAPRENDER], &cb, cmd);
					device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubemapRenderCB), cmd);

					Viewport vp;
					vp.TopLeftX = 0;
					vp.TopLeftY = 0;
					vp.Width = (float)SHADOWRES_CUBE;
					vp.Height = (float)SHADOWRES_CUBE;
					vp.MinDepth = 0.0f;
					vp.MaxDepth = 1.0f;
					device->BindViewports(1, &vp, cmd);

					device->RenderPassBegin(&renderpasses_shadowCube[light.shadowMap_index], cmd);
					RenderMeshes(renderQueue, RENDERPASS_SHADOWCUBE, RENDERTYPE_OPAQUE, cmd, false, frusta, frustum_count);
					device->RenderPassEnd(cmd);

					GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
				}

			}
			break;
			} // terminate switch
		}

		wiProfiler::EndRange(range); // Shadow Rendering
		device->EventEnd(cmd);
	}
}

void DrawScene(
	const CameraComponent& camera,
	RENDERPASS renderPass,
	CommandList cmd,
	uint32_t flags
)
{
	const Scene& scene = GetScene();
	const FrameCulling& culling = frameCullings.at(&camera);

	const bool opaque = flags & RENDERTYPE_OPAQUE;
	const bool transparent = flags & DRAWSCENE_TRANSPARENT;
	const bool tessellation = (flags & DRAWSCENE_TESSELLATION) && GetTessellationEnabled();
	const bool hairparticle = flags & DRAWSCENE_HAIRPARTICLE;

	device->EventBegin("DrawScene", cmd);

	BindCommonResources(cmd);
	BindShadowmaps(PS, cmd);
	BindEnvironmentTextures(PS, cmd);
	BindConstantBuffers(VS, cmd);
	BindConstantBuffers(PS, cmd);

	device->BindResource(PS, GetGlobalLightmap(), TEXSLOT_GLOBALLIGHTMAP, cmd);
	if (decalAtlas.IsValid())
	{
		device->BindResource(PS, &decalAtlas, TEXSLOT_DECALATLAS, cmd);
	}

	if (transparent)
	{
		if (renderPass == RENDERPASS_MAIN)
		{
			device->BindResource(PS, &resourceBuffers[RBTYPE_ENTITYTILES_TRANSPARENT], SBSLOT_ENTITYTILES, cmd);
		}
		if (ocean != nullptr)
		{
			ocean->Render(camera, scene.weather, renderTime, cmd);
		}
	}
	else
	{
		if (renderPass == RENDERPASS_MAIN)
		{
			device->BindResource(PS, &resourceBuffers[RBTYPE_ENTITYTILES_OPAQUE], SBSLOT_ENTITYTILES, cmd);
		}
	}

	if (hairparticle)
	{
		if (!transparent)
		{
			// transparent pass only renders hair when alpha composition enabled
			for (uint32_t hairIndex : culling.culledHairs)
			{
				const wiHairParticle& hair = scene.hairs[hairIndex];
				Entity entity = scene.hairs.GetEntity(hairIndex);
				const MaterialComponent& material = *scene.materials.GetComponent(entity);

				hair.Draw(camera, material, renderPass, cmd);
			}
		}
	}

	RenderImpostors(camera, renderPass, cmd);

	uint32_t renderTypeFlags = 0;
	if (opaque)
	{
		renderTypeFlags |= RENDERTYPE_OPAQUE;
	}
	if (transparent)
	{
		renderTypeFlags |= RENDERTYPE_TRANSPARENT;
		renderTypeFlags |= RENDERTYPE_WATER;
	}

	RenderQueue renderQueue;
	renderQueue.camera = &camera;
	for (uint32_t instanceIndex : culling.culledObjects)
	{
		const ObjectComponent& object = scene.objects[instanceIndex];

		if (GetOcclusionCullingEnabled() && occlusionCulling && object.IsOccluded())
			continue;

		if (object.IsRenderable() && (object.GetRenderTypes() & renderTypeFlags))
		{
			const float distance = wiMath::Distance(camera.Eye, object.center);
			if (object.IsImpostorPlacement() && distance > object.impostorSwapDistance + object.impostorFadeThresholdRadius)
			{
				continue;
			}
			RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
			size_t meshIndex = scene.meshes.GetIndex(object.meshID);
			batch->Create(meshIndex, instanceIndex, distance);
			renderQueue.add(batch);
		}
	}
	if (!renderQueue.empty())
	{
		renderQueue.sort(transparent ? RenderQueue::SORT_BACK_TO_FRONT : RenderQueue::SORT_FRONT_TO_BACK);
		RenderMeshes(renderQueue, renderPass, renderTypeFlags, cmd, tessellation);

		GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
	}

	device->BindShadingRate(SHADING_RATE_1X1, cmd);
	device->EventEnd(cmd);

}

void DrawDebugWorld(const CameraComponent& camera, CommandList cmd)
{
	const Scene& scene = GetScene();

	static GPUBuffer wirecubeVB;
	static GPUBuffer wirecubeIB;
	if (!wirecubeVB.IsValid())
	{
		XMFLOAT4 min = XMFLOAT4(-1, -1, -1, 1);
		XMFLOAT4 max = XMFLOAT4(1, 1, 1, 1);

		XMFLOAT4 verts[] = {
			min,							XMFLOAT4(1,1,1,1),
			XMFLOAT4(min.x,max.y,min.z,1),	XMFLOAT4(1,1,1,1),
			XMFLOAT4(min.x,max.y,max.z,1),	XMFLOAT4(1,1,1,1),
			XMFLOAT4(min.x,min.y,max.z,1),	XMFLOAT4(1,1,1,1),
			XMFLOAT4(max.x,min.y,min.z,1),	XMFLOAT4(1,1,1,1),
			XMFLOAT4(max.x,max.y,min.z,1),	XMFLOAT4(1,1,1,1),
			max,							XMFLOAT4(1,1,1,1),
			XMFLOAT4(max.x,min.y,max.z,1),	XMFLOAT4(1,1,1,1),
		};

		GPUBufferDesc bd;
		bd.Usage = USAGE_DEFAULT;
		bd.ByteWidth = sizeof(verts);
		bd.BindFlags = BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		SubresourceData InitData;
		InitData.pSysMem = verts;
		device->CreateBuffer(&bd, &InitData, &wirecubeVB);

		uint16_t indices[] = {
			0,1,1,2,0,3,0,4,1,5,4,5,
			5,6,4,7,2,6,3,7,2,3,6,7
		};

		bd.Usage = USAGE_DEFAULT;
		bd.ByteWidth = sizeof(indices);
		bd.BindFlags = BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		InitData.pSysMem = indices;
		device->CreateBuffer(&bd, &InitData, &wirecubeIB);
	}

	device->EventBegin("DrawDebugWorld", cmd);

	BindCommonResources(cmd);
	BindConstantBuffers(VS, cmd);
	BindConstantBuffers(PS, cmd);

	if (debugPartitionTree)
	{
		// Actually, there is no SPTree any more, so this will just render all aabbs...
		device->EventBegin("DebugPartitionTree", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);

		const GPUBuffer* vbs[] = {
			&wirecubeVB,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(&wirecubeIB, INDEXFORMAT_16BIT, 0, cmd);

		MiscCB sb;

		for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
		{
			const AABB& aabb = scene.aabb_objects[i];

			XMStoreFloat4x4(&sb.g_xTransform, aabb.getAsBoxMatrix()*camera.GetViewProjection());
			sb.g_xColor = XMFLOAT4(1, 0, 0, 1);

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}

		for (size_t i = 0; i < scene.aabb_lights.GetCount(); ++i)
		{
			const AABB& aabb = scene.aabb_lights[i];

			XMStoreFloat4x4(&sb.g_xTransform, aabb.getAsBoxMatrix()*camera.GetViewProjection());
			sb.g_xColor = XMFLOAT4(1, 1, 0, 1);

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}

		for (size_t i = 0; i < scene.aabb_decals.GetCount(); ++i)
		{
			const AABB& aabb = scene.aabb_decals[i];

			XMStoreFloat4x4(&sb.g_xTransform, aabb.getAsBoxMatrix()*camera.GetViewProjection());
			sb.g_xColor = XMFLOAT4(1, 0, 1, 1);

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}

		for (size_t i = 0; i < scene.aabb_probes.GetCount(); ++i)
		{
			const AABB& aabb = scene.aabb_probes[i];

			XMStoreFloat4x4(&sb.g_xTransform, aabb.getAsBoxMatrix()*camera.GetViewProjection());
			sb.g_xColor = XMFLOAT4(0, 1, 1, 1);

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}

		device->EventEnd(cmd);
	}

	if (debugBoneLines)
	{
		device->EventBegin("DebugBoneLines", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_LINES], cmd);

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, camera.GetViewProjection());
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
		{
			const ArmatureComponent& armature = scene.armatures[i];

			if (armature.boneCollection.empty())
			{
				continue;
			}

			struct LineSegment
			{
				XMFLOAT4 a, colorA, b, colorB;
			};
			GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(LineSegment) * armature.boneCollection.size(), cmd);

			int j = 0;
			for (Entity entity : armature.boneCollection)
			{
				const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(entity);
				if (hierarchy == nullptr)
				{
					continue;
				}
				const TransformComponent& transform = *scene.transforms.GetComponent(entity);
				const TransformComponent& parent = *scene.transforms.GetComponent(hierarchy->parentID);

				XMMATRIX transform_world = XMLoadFloat4x4(&transform.world);
				XMMATRIX parent_world = XMLoadFloat4x4(&parent.world);

				XMVECTOR a = XMVector3Transform(XMVectorSet(0, 0, 0, 1), transform_world);
				XMVECTOR b = XMVector3Transform(XMVectorSet(0, 0, 0, 1), parent_world);

				LineSegment segment;
				XMStoreFloat4(&segment.a, a);
				XMStoreFloat4(&segment.b, b);
				segment.colorA = XMFLOAT4(1, 1, 1, 1);
				segment.colorB = XMFLOAT4(1, 0, 1, 1);

				memcpy((void*)((size_t)mem.data + j * sizeof(LineSegment)), &segment, sizeof(LineSegment));
				j++;
			}

			const GPUBuffer* vbs[] = {
				mem.buffer
			};
			const uint32_t strides[] = {
				sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
			};
			const uint32_t offsets[] = {
				mem.offset,
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

			device->Draw(2 * j, 0, cmd);

		}

		device->EventEnd(cmd);
	}

	if (!renderableTriangles_solid.empty())
	{
		device->EventBegin("DebugTriangles (Solid)", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_TRIANGLE_SOLID], cmd);

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, camera.GetViewProjection());
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		struct Vertex
		{
			XMFLOAT4 position;
			XMFLOAT4 color;
		};
		GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(Vertex) * renderableTriangles_solid.size() * 3, cmd);

		int i = 0;
		for (auto& triangle : renderableTriangles_solid)
		{
			Vertex vertices[3];
			vertices[0].position = XMFLOAT4(triangle.positionA.x, triangle.positionA.y, triangle.positionA.z, 1);
			vertices[0].color = triangle.colorA;
			vertices[1].position = XMFLOAT4(triangle.positionB.x, triangle.positionB.y, triangle.positionB.z, 1);
			vertices[1].color = triangle.colorB;
			vertices[2].position = XMFLOAT4(triangle.positionC.x, triangle.positionC.y, triangle.positionC.z, 1);
			vertices[2].color = triangle.colorC;

			memcpy((void*)((size_t)mem.data + i * sizeof(vertices)), vertices, sizeof(vertices));
			i++;
		}

		const GPUBuffer* vbs[] = {
			mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		const uint32_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		device->Draw(3 * i, 0, cmd);

		renderableTriangles_solid.clear();

		device->EventEnd(cmd);
	}

	if (!renderableTriangles_wireframe.empty())
	{
		device->EventBegin("DebugTriangles (Wireframe)", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_TRIANGLE_WIREFRAME], cmd);

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, camera.GetViewProjection());
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		struct Vertex
		{
			XMFLOAT4 position;
			XMFLOAT4 color;
		};
		GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(Vertex) * renderableTriangles_wireframe.size() * 3, cmd);

		int i = 0;
		for (auto& triangle : renderableTriangles_wireframe)
		{
			Vertex vertices[3];
			vertices[0].position = XMFLOAT4(triangle.positionA.x, triangle.positionA.y, triangle.positionA.z, 1);
			vertices[0].color = triangle.colorA;
			vertices[1].position = XMFLOAT4(triangle.positionB.x, triangle.positionB.y, triangle.positionB.z, 1);
			vertices[1].color = triangle.colorB;
			vertices[2].position = XMFLOAT4(triangle.positionC.x, triangle.positionC.y, triangle.positionC.z, 1);
			vertices[2].color = triangle.colorC;

			memcpy((void*)((size_t)mem.data + i * sizeof(vertices)), vertices, sizeof(vertices));
			i++;
		}

		const GPUBuffer* vbs[] = {
			mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		const uint32_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		device->Draw(3 * i, 0, cmd);

		renderableTriangles_wireframe.clear();

		device->EventEnd(cmd);
	}

	if (!renderableLines.empty())
	{
		device->EventBegin("DebugLines", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_LINES], cmd);

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, camera.GetViewProjection());
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		struct LineSegment
		{
			XMFLOAT4 a, colorA, b, colorB;
		};
		GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(LineSegment) * renderableLines.size(), cmd);

		int i = 0;
		for (auto& line : renderableLines)
		{
			LineSegment segment;
			segment.a = XMFLOAT4(line.start.x, line.start.y, line.start.z, 1);
			segment.b = XMFLOAT4(line.end.x, line.end.y, line.end.z, 1);
			segment.colorA = line.color_start;
			segment.colorB = line.color_end;

			memcpy((void*)((size_t)mem.data + i * sizeof(LineSegment)), &segment, sizeof(LineSegment));
			i++;
		}

		const GPUBuffer* vbs[] = {
			mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		const uint32_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		device->Draw(2 * i, 0, cmd);

		renderableLines.clear();

		device->EventEnd(cmd);
	}

	if (!renderableLines2D.empty())
	{
		device->EventBegin("DebugLines - 2D", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_LINES], cmd);

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, device->GetScreenProjection());
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		struct LineSegment
		{
			XMFLOAT4 a, colorA, b, colorB;
		};
		GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(LineSegment) * renderableLines2D.size(), cmd);

		int i = 0;
		for (auto& line : renderableLines2D)
		{
			LineSegment segment;
			segment.a = XMFLOAT4(line.start.x, line.start.y, 0, 1);
			segment.b = XMFLOAT4(line.end.x, line.end.y, 0, 1);
			segment.colorA = line.color_start;
			segment.colorB = line.color_end;

			memcpy((void*)((size_t)mem.data + i * sizeof(LineSegment)), &segment, sizeof(LineSegment));
			i++;
		}

		const GPUBuffer* vbs[] = {
			mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		const uint32_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		device->Draw(2 * i, 0, cmd);

		renderableLines2D.clear();

		device->EventEnd(cmd);
	}

	if (!renderablePoints.empty())
	{
		device->EventBegin("DebugPoints", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_LINES], cmd);

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, camera.GetProjection()); // only projection, we will expand in view space on CPU below to be camera facing!
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		// Will generate 2 line segments for each point forming a cross section:
		struct LineSegment
		{
			XMFLOAT4 a, colorA, b, colorB;
		};
		GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(LineSegment) * renderablePoints.size() * 2, cmd);

		XMMATRIX V = camera.GetView();

		int i = 0;
		for (auto& point : renderablePoints)
		{
			LineSegment segment;
			segment.colorA = segment.colorB = point.color;

			// the cross section will be transformed to view space and expanded here:
			XMVECTOR _c = XMLoadFloat3(&point.position);
			_c = XMVector3Transform(_c, V);

			XMVECTOR _a = _c + XMVectorSet(-1, -1, 0, 0) * point.size;
			XMVECTOR _b = _c + XMVectorSet(1, 1, 0, 0) * point.size;
			XMStoreFloat4(&segment.a, _a);
			XMStoreFloat4(&segment.b, _b);
			memcpy((void*)((size_t)mem.data + i * sizeof(LineSegment)), &segment, sizeof(LineSegment));
			i++;

			_a = _c + XMVectorSet(-1, 1, 0, 0) * point.size;
			_b = _c + XMVectorSet(1, -1, 0, 0) * point.size;
			XMStoreFloat4(&segment.a, _a);
			XMStoreFloat4(&segment.b, _b);
			memcpy((void*)((size_t)mem.data + i * sizeof(LineSegment)), &segment, sizeof(LineSegment));
			i++;
		}

		const GPUBuffer* vbs[] = {
			mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		const uint32_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		device->Draw(2 * i, 0, cmd);

		renderablePoints.clear();

		device->EventEnd(cmd);
	}

	if (!renderableBoxes.empty())
	{
		device->EventBegin("DebugBoxes", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);

		const GPUBuffer* vbs[] = {
			&wirecubeVB,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(&wirecubeIB, INDEXFORMAT_16BIT, 0, cmd);

		MiscCB sb;

		for (auto& x : renderableBoxes)
		{
			XMStoreFloat4x4(&sb.g_xTransform, XMLoadFloat4x4(&x.first)*camera.GetViewProjection());
			sb.g_xColor = x.second;

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}
		renderableBoxes.clear();

		device->EventEnd(cmd);
	}

	if (!renderableSpheres.empty())
	{
		device->EventBegin("DebugSpheres", cmd);

		static GPUBuffer wiresphereVB;
		static GPUBuffer wiresphereIB;
		if (!wiresphereVB.IsValid())
		{
			struct Vertex
			{
				XMFLOAT4 position;
				XMFLOAT4 color;
			};
			std::vector<Vertex> vertices;

			const int segmentcount = 36;
			Vertex vert;
			vert.color = XMFLOAT4(1, 1, 1, 1);

			// XY
			for (int i = 0; i <= segmentcount; ++i)
			{
				const float angle0 = (float)i / (float)segmentcount * XM_2PI;
				vert.position = XMFLOAT4(sinf(angle0), cosf(angle0), 0, 1);
				vertices.push_back(vert);
			}
			// XZ
			for (int i = 0; i <= segmentcount; ++i)
			{
				const float angle0 = (float)i / (float)segmentcount * XM_2PI;
				vert.position = XMFLOAT4(sinf(angle0), 0, cosf(angle0), 1);
				vertices.push_back(vert);
			}
			// YZ
			for (int i = 0; i <= segmentcount; ++i)
			{
				const float angle0 = (float)i / (float)segmentcount * XM_2PI;
				vert.position = XMFLOAT4(0, sinf(angle0), cosf(angle0), 1);
				vertices.push_back(vert);
			}

			GPUBufferDesc bd;
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = uint32_t(vertices.size() * sizeof(Vertex));
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			InitData.pSysMem = vertices.data();
			device->CreateBuffer(&bd, &InitData, &wiresphereVB);

			std::vector<uint16_t> indices;
			for (int i = 0; i < segmentcount; ++i)
			{
				indices.push_back(uint16_t(i));
				indices.push_back(uint16_t(i + 1));
			}
			for (int i = 0; i < segmentcount; ++i)
			{
				indices.push_back(uint16_t(i + segmentcount + 1));
				indices.push_back(uint16_t(i + segmentcount + 1 + 1));
			}
			for (int i = 0; i < segmentcount; ++i)
			{
				indices.push_back(uint16_t(i + (segmentcount + 1) * 2));
				indices.push_back(uint16_t(i + (segmentcount + 1) * 2 + 1));
			}

			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = uint32_t(indices.size() * sizeof(uint16_t));
			bd.BindFlags = BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;
			InitData.pSysMem = indices.data();
			device->CreateBuffer(&bd, &InitData, &wiresphereIB);
		}

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);

		const GPUBuffer* vbs[] = {
			&wiresphereVB,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(&wiresphereIB, INDEXFORMAT_16BIT, 0, cmd);

		MiscCB sb;

		for (auto& x : renderableSpheres)
		{
			const SPHERE& sphere = x.first;
			XMStoreFloat4x4(&sb.g_xTransform,
				XMMatrixScaling(sphere.radius, sphere.radius, sphere.radius) *
				XMMatrixTranslation(sphere.center.x, sphere.center.y, sphere.center.z) *
				camera.GetViewProjection()
			);
			sb.g_xColor = x.second;

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(wiresphereIB.GetDesc().ByteWidth / sizeof(uint16_t), 0, 0, cmd);
		}
		renderableSpheres.clear();

		device->EventEnd(cmd);
	}

	if (!renderableCapsules.empty())
	{
		device->EventBegin("DebugCapsules", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, camera.GetViewProjection());
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		const int segmentcount = 18 + 1 + 18 + 1;
		const int linecount = (int)renderableCapsules.size() * segmentcount;

		struct LineSegment
		{
			XMFLOAT4 a, colorA, b, colorB;
		};
		GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(LineSegment) * (size_t)linecount, cmd);

		XMVECTOR Eye = camera.GetEye();
		XMVECTOR Unit = XMVectorSet(0, 1, 0, 0);

		LineSegment* linearray = (LineSegment*)mem.data;
		int j = 0;
		for (auto& it : renderableCapsules)
		{
			const CAPSULE& capsule = it.first;
			const float radius = capsule.radius;

			XMVECTOR Base = XMLoadFloat3(&capsule.base);
			XMVECTOR Tip = XMLoadFloat3(&capsule.tip);
			XMVECTOR Radius = XMVectorReplicate(capsule.radius);
			XMVECTOR Normal = XMVector3Normalize(Tip - Base);
			XMVECTOR Tangent = XMVector3Normalize(XMVector3Cross(Normal, Base - Eye));
			XMVECTOR Binormal = XMVector3Normalize(XMVector3Cross(Tangent, Normal));
			XMVECTOR LineEndOffset = Normal * Radius;
			XMVECTOR A = Base + LineEndOffset;
			XMVECTOR B = Tip - LineEndOffset;
			XMVECTOR AB = Unit * XMVector3Length(B - A);
			XMMATRIX M = { Tangent,Normal,Binormal,XMVectorSetW(A, 1) };

			for (int i = 0; i < segmentcount; i += 1)
			{
				const float angle0 = XM_PIDIV2 + (float)i / (float)segmentcount * XM_2PI;
				const float angle1 = XM_PIDIV2 + (float)(i + 1) / (float)segmentcount * XM_2PI;
				XMVECTOR a, b;
				if (i < 18)
				{

					a = XMVectorSet(sinf(angle0) * radius, cosf(angle0) * radius, 0, 1);
					b = XMVectorSet(sinf(angle1) * radius, cosf(angle1) * radius, 0, 1);
				}
				else if (i == 18)
				{
					a = XMVectorSet(sinf(angle0) * radius, cosf(angle0) * radius, 0, 1);
					b = AB + XMVectorSet(sinf(angle1) * radius, cosf(angle1) * radius, 0, 1);
				}
				else if (i > 18 && i < 18 + 1 + 18)
				{
					a = AB + XMVectorSet(sinf(angle0) * radius, cosf(angle0) * radius, 0, 1);
					b = AB + XMVectorSet(sinf(angle1) * radius, cosf(angle1) * radius, 0, 1);
				}
				else
				{
					a = AB + XMVectorSet(sinf(angle0) * radius, cosf(angle0) * radius, 0, 1);
					b = XMVectorSet(sinf(angle1) * radius, cosf(angle1) * radius, 0, 1);
				}
				a = XMVector3Transform(a, M);
				b = XMVector3Transform(b, M);
				LineSegment& line = linearray[j++];
				XMStoreFloat4(&line.a, a);
				XMStoreFloat4(&line.b, b);
				line.colorA = line.colorB = it.second;
			}

		}

		const GPUBuffer* vbs[] = {
			mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		const uint32_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		device->Draw(linecount * 2, 0, cmd);

		renderableCapsules.clear();

		device->EventEnd(cmd);
	}


	if (debugEnvProbes && textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY].IsValid())
	{
		device->EventBegin("Debug EnvProbes", cmd);
		// Envmap spheres:

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_ENVPROBE], cmd);

		MiscCB sb;
		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			const EnvironmentProbeComponent& probe = scene.probes[i];

			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranslationFromVector(XMLoadFloat3(&probe.position)));
			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			if (probe.textureIndex < 0)
			{
				device->BindResource(PS, wiTextureHelper::getBlackCubeMap(), TEXSLOT_ONDEMAND0, cmd);
			}
			else
			{
				device->BindResource(PS, &textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], TEXSLOT_ONDEMAND0, cmd, textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY].GetDesc().MipLevels + probe.textureIndex);
			}

			device->Draw(2880, 0, cmd); // uv-sphere
		}


		// Local proxy boxes:

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);

		const GPUBuffer* vbs[] = {
			&wirecubeVB,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(&wirecubeIB, INDEXFORMAT_16BIT, 0, cmd);

		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			const EnvironmentProbeComponent& probe = scene.probes[i];

			if (probe.textureIndex < 0)
			{
				continue;
			}

			Entity entity = scene.probes.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			XMStoreFloat4x4(&sb.g_xTransform, XMLoadFloat4x4(&transform.world)*camera.GetViewProjection());
			sb.g_xColor = float4(0, 1, 1, 1);

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}

		device->EventEnd(cmd);
	}


	if (gridHelper)
	{
		device->EventBegin("GridHelper", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_GRID], cmd);

		static float col = 0.7f;
		static uint32_t gridVertexCount = 0;
		static GPUBuffer grid;
		if (!grid.IsValid())
		{
			const float h = 0.01f; // avoid z-fight with zero plane
			const int a = 20;
			XMFLOAT4 verts[((a + 1) * 2 + (a + 1) * 2) * 2];

			int count = 0;
			for (int i = 0; i <= a; ++i)
			{
				verts[count++] = XMFLOAT4(i - a * 0.5f, h, -a * 0.5f, 1);
				verts[count++] = (i == a / 2 ? XMFLOAT4(0, 0, 1, 1) : XMFLOAT4(col, col, col, 1));

				verts[count++] = XMFLOAT4(i - a * 0.5f, h, +a * 0.5f, 1);
				verts[count++] = (i == a / 2 ? XMFLOAT4(0, 0, 1, 1) : XMFLOAT4(col, col, col, 1));
			}
			for (int j = 0; j <= a; ++j)
			{
				verts[count++] = XMFLOAT4(-a * 0.5f, h, j - a * 0.5f, 1);
				verts[count++] = (j == a / 2 ? XMFLOAT4(1, 0, 0, 1) : XMFLOAT4(col, col, col, 1));

				verts[count++] = XMFLOAT4(+a * 0.5f, h, j - a * 0.5f, 1);
				verts[count++] = (j == a / 2 ? XMFLOAT4(1, 0, 0, 1) : XMFLOAT4(col, col, col, 1));
			}

			gridVertexCount = arraysize(verts) / 2;

			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			InitData.pSysMem = verts;
			device->CreateBuffer(&bd, &InitData, &grid);
		}

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, camera.GetViewProjection());
		sb.g_xColor = float4(1, 1, 1, 1);

		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		const GPUBuffer* vbs[] = {
			&grid,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->Draw(gridVertexCount, 0, cmd);

		device->EventEnd(cmd);
	}

	if (voxelHelper && textures[TEXTYPE_3D_VOXELRADIANCE].IsValid())
	{
		device->EventBegin("Debug Voxels", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_VOXEL], cmd);

		device->BindResource(VS, &textures[TEXTYPE_3D_VOXELRADIANCE], TEXSLOT_VOXELRADIANCE, cmd);


		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranslationFromVector(XMLoadFloat3(&voxelSceneData.center)) * camera.GetViewProjection());
		sb.g_xColor = float4(1, 1, 1, 1);

		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(GS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		device->Draw(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res, 0, cmd);

		device->EventEnd(cmd);
	}

	if (debugEmitters)
	{
		device->EventBegin("DebugEmitters", cmd);

		MiscCB sb;
		for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
		{
			const wiEmittedParticle& emitter = scene.emitters[i];
			Entity entity = scene.emitters.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);
			const MeshComponent* mesh = scene.meshes.GetComponent(emitter.meshID);

			XMStoreFloat4x4(&sb.g_xTransform, XMLoadFloat4x4(&transform.world)*camera.GetViewProjection());
			sb.g_xColor = float4(0, 1, 0, 1);
			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			if (mesh == nullptr)
			{
				// No mesh, just draw a box:
				device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);
				const GPUBuffer* vbs[] = {
					&wirecubeVB,
				};
				const uint32_t strides[] = {
					sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
				};
				device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
				device->BindIndexBuffer(&wirecubeIB, INDEXFORMAT_16BIT, 0, cmd);
				device->DrawIndexed(24, 0, 0, cmd);
			}
			else
			{
				// Draw mesh wireframe:
				device->BindPipelineState(&PSO_debug[DEBUGRENDERING_EMITTER], cmd);
				const GPUBuffer* vbs[] = {
					mesh->streamoutBuffer_POS.IsValid() ? &mesh->streamoutBuffer_POS : &mesh->vertexBuffer_POS,
				};
				const uint32_t strides[] = {
					sizeof(MeshComponent::Vertex_POS),
				};
				device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
				device->BindIndexBuffer(&mesh->indexBuffer, mesh->GetIndexFormat(), 0, cmd);

				device->DrawIndexed((uint32_t)mesh->indices.size(), 0, 0, cmd);
			}
		}

		device->EventEnd(cmd);
	}

	if (!paintrads.empty())
	{
		device->EventBegin("Paint Radiuses", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_PAINTRADIUS], cmd);

		for (auto& x : paintrads)
		{
			const ObjectComponent& object = *scene.objects.GetComponent(x.objectEntity);
			const TransformComponent& transform = *scene.transforms.GetComponent(x.objectEntity);
			const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);
			const MeshComponent::MeshSubset& subset = mesh.subsets[x.subset];
			const MaterialComponent& material = *scene.materials.GetComponent(subset.materialID);

			GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(Instance), cmd);
			volatile Instance* buff = (volatile Instance*)mem.data;
			buff->Create(transform.world);

			const GPUBuffer* vbs[] = {
				mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
				&mesh.vertexBuffer_UV0,
				&mesh.vertexBuffer_UV1,
				mem.buffer
			};
			uint32_t strides[] = {
				sizeof(MeshComponent::Vertex_POS),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(Instance)
			};
			uint32_t offsets[] = {
				0,
				0,
				0,
				mem.offset
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

			device->BindIndexBuffer(&mesh.indexBuffer, mesh.GetIndexFormat(), 0, cmd);

			device->BindConstantBuffer(VS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);
			device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);

			PaintRadiusCB cb;
			cb.xPaintRadResolution = x.dimensions;
			cb.xPaintRadCenter = x.center;
			cb.xPaintRadRadius = x.radius;
			cb.xPaintRadUVSET = x.uvset;
			device->UpdateBuffer(&constantBuffers[CBTYPE_PAINTRADIUS], &cb, cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_PAINTRADIUS], CB_GETBINDSLOT(PaintRadiusCB), cmd);

			device->DrawIndexedInstanced(subset.indexCount, 1, subset.indexOffset, 0, 0, cmd);
		}

		paintrads.clear();

		device->EventEnd(cmd);
	}


	if (debugForceFields)
	{
		device->EventBegin("DebugForceFields", cmd);

		MiscCB sb;
		for (size_t i = 0; i < scene.forces.GetCount(); ++i)
		{
			const ForceFieldComponent& force = scene.forces[i];

			XMStoreFloat4x4(&sb.g_xTransform, camera.GetViewProjection());
			sb.g_xColor = XMFLOAT4(camera.Eye.x, camera.Eye.y, camera.Eye.z, (float)i);
			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			switch (force.type)
			{
			case ENTITY_TYPE_FORCEFIELD_POINT:
				device->BindPipelineState(&PSO_debug[DEBUGRENDERING_FORCEFIELD_POINT], cmd);
				device->Draw(2880, 0, cmd); // uv-sphere
				break;
			case ENTITY_TYPE_FORCEFIELD_PLANE:
				device->BindPipelineState(&PSO_debug[DEBUGRENDERING_FORCEFIELD_PLANE], cmd);
				device->Draw(14, 0, cmd); // box
				break;
			}

			++i;
		}

		device->EventEnd(cmd);
	}


	if (debugCameras)
	{
		device->EventBegin("DebugCameras", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);

		const GPUBuffer* vbs[] = {
			&wirecubeVB,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(&wirecubeIB, INDEXFORMAT_16BIT, 0, cmd);

		MiscCB sb;
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);

		for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
		{
			const CameraComponent& cam = scene.cameras[i];
			Entity entity = scene.cameras.GetEntity(i);

			XMStoreFloat4x4(&sb.g_xTransform, cam.GetInvView()*camera.GetViewProjection());

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}

		device->EventEnd(cmd);
	}

	if (GetRaytraceDebugBVHVisualizerEnabled())
	{
		RayTraceSceneBVH(cmd);
	}

	device->EventEnd(cmd);
}


void RenderAtmosphericScatteringTextures(CommandList cmd)
{
	device->EventBegin("ComputeAtmosphericScatteringTextures", cmd);
	auto range = wiProfiler::BeginRangeGPU("Atmospheric Scattering Textures", cmd);

	GPUBarrier memory_barrier = GPUBarrier::Memory();

	if (!textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT].IsValid())
	{
		TextureDesc desc;
		desc.type = TextureDesc::TEXTURE_2D;
		desc.Width = 256;
		desc.Height = 64;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT]);
	}
	if (!textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT].IsValid())
	{
		TextureDesc desc;
		desc.type = TextureDesc::TEXTURE_2D;
		desc.Width = 32;
		desc.Height = 32;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT]);
	}
	if (!textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT].IsValid())
	{
		TextureDesc desc;
		desc.type = TextureDesc::TEXTURE_2D;
		desc.Width = 192;
		desc.Height = 104;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT]);
	}

	// Transmittance Lut pass:
	{
		device->EventBegin("TransmittanceLut", cmd);
		device->BindComputeShader(&shaders[CSTYPE_SKYATMOSPHERE_TRANSMITTANCELUT], cmd);

		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_ONDEMAND0, cmd); // empty
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_ONDEMAND1, cmd); // empty

		const GPUResource* uavs[] = {
			&textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		const int threadSize = 8;
		const int transmittanceLutWidth = textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT].GetDesc().Width;
		const int transmittanceLutHeight = textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT].GetDesc().Height;
		const int transmittanceLutThreadX = static_cast<uint32_t>(std::ceil(transmittanceLutWidth / threadSize));
		const int transmittanceLutThreadY = static_cast<uint32_t>(std::ceil(transmittanceLutHeight / threadSize));

		device->Dispatch(transmittanceLutThreadX, transmittanceLutThreadY, 1, cmd);

		device->Barrier(&memory_barrier, 1, cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// MultiScattered Luminance Lut pass:
	{
		device->EventBegin("MultiScatteredLuminanceLut", cmd);
		device->BindComputeShader(&shaders[CSTYPE_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], cmd);

		// Use transmittance from previous pass
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_ONDEMAND1, cmd);

		const GPUResource* uavs[] = {
			&textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		const int multiScatteredLutWidth = textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT].GetDesc().Width;
		const int multiScatteredLutHeight = textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT].GetDesc().Height;

		device->Dispatch(multiScatteredLutWidth, multiScatteredLutHeight, 1, cmd);

		device->Barrier(&memory_barrier, 1, cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	device->EventEnd(cmd);

	RefreshAtmosphericScatteringTextures(cmd);

	wiProfiler::EndRange(range);
}
void RefreshAtmosphericScatteringTextures(CommandList cmd)
{
	device->EventBegin("UpdateAtmosphericScatteringTextures", cmd);

	GPUBarrier memory_barrier = GPUBarrier::Memory();

	// Sky View Lut pass:
	{
		device->EventBegin("SkyViewLut", cmd);
		device->BindComputeShader(&shaders[CSTYPE_SKYATMOSPHERE_SKYVIEWLUT], cmd);

		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_ONDEMAND1, cmd);

		const GPUResource* uavs[] = {
			&textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		const int threadSize = 8;
		const int skyViewLutWidth = textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT].GetDesc().Width;
		const int skyViewLutHeight = textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT].GetDesc().Height;
		const int skyViewLutThreadX = static_cast<uint32_t>(std::ceil(skyViewLutWidth / threadSize));
		const int skyViewLutThreadY = static_cast<uint32_t>(std::ceil(skyViewLutHeight / threadSize));

		device->Dispatch(skyViewLutThreadX, skyViewLutThreadY, 1, cmd);

		device->Barrier(&memory_barrier, 1, cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	device->EventEnd(cmd);
}
void DrawSky(CommandList cmd)
{
	const Scene& scene = GetScene();

	device->EventBegin("DrawSky", cmd);
	
	if (scene.weather.skyMap != nullptr)
	{
		device->BindPipelineState(&PSO_sky[SKYRENDERING_STATIC], cmd);
		device->BindResource(PS, scene.weather.skyMap->texture, TEXSLOT_GLOBALENVMAP, cmd);
	}
	else
	{
		device->BindPipelineState(&PSO_sky[SKYRENDERING_DYNAMIC], cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);
	}

	BindConstantBuffers(VS, cmd);
	BindConstantBuffers(PS, cmd);

	device->Draw(3, 0, cmd);

	device->EventEnd(cmd);
}
void DrawSun(CommandList cmd)
{
	device->EventBegin("DrawSun", cmd);

	device->BindPipelineState(&PSO_sky[SKYRENDERING_SUN], cmd);

	device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
	device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
	device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);

	BindConstantBuffers(VS, cmd);
	BindConstantBuffers(PS, cmd);

	device->Draw(3, 0, cmd);

	device->EventEnd(cmd);
}


static const uint32_t envmapCount = 16;
static const uint32_t envmapRes = 128;
static const uint32_t envmapMIPs = 8;
static Texture envrenderingDepthBuffer;
static std::vector<RenderPass> renderpasses_envmap;
vector<uint32_t> probesToRefresh(envmapCount);
void ManageEnvProbes()
{
	probesToRefresh.clear();

	Scene& scene = GetScene();

	// reconstruct envmap array status:
	bool envmapTaken[envmapCount] = {};
	for (size_t i = 0; i < scene.probes.GetCount(); ++i)
	{
		const EnvironmentProbeComponent& probe = scene.probes[i];
		if (probe.textureIndex >= 0)
		{
			envmapTaken[probe.textureIndex] = true;
		}
	}

	for (size_t probeIndex = 0; probeIndex < scene.probes.GetCount(); ++probeIndex)
	{
		EnvironmentProbeComponent& probe = scene.probes[probeIndex];
		Entity entity = scene.probes.GetEntity(probeIndex);

		if (probe.textureIndex < 0)
		{
			// need to take a free envmap texture slot:
			bool found = false;
			for (int i = 0; i < arraysize(envmapTaken); ++i)
			{
				if (envmapTaken[i] == false)
				{
					envmapTaken[i] = true;
					probe.textureIndex = i;
					found = true;
					break;
				}
			}
			if (!found)
			{
				// could not find free slot in envmap array, so skip this probe:
				continue;
			}
		}

		if (!probe.IsDirty())
		{
			continue;
		}
		if (!probe.IsRealTime())
		{
			probe.SetDirty(false);
		}

		probesToRefresh.push_back((uint32_t)probeIndex);
	}

	if (!probesToRefresh.empty() && !textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY].IsValid())
	{
		TextureDesc desc;
		desc.ArraySize = 6;
		desc.BindFlags = BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_D16_UNORM;
		desc.Height = envmapRes;
		desc.Width = envmapRes;
		desc.MipLevels = 1;
		desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;
		desc.Usage = USAGE_DEFAULT;
		desc.layout = IMAGE_LAYOUT_DEPTHSTENCIL;

		device->CreateTexture(&desc, nullptr, &envrenderingDepthBuffer);

		desc.ArraySize = envmapCount * 6;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Height = envmapRes;
		desc.Width = envmapRes;
		desc.MipLevels = envmapMIPs;
		desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;
		desc.Usage = USAGE_DEFAULT;
		desc.layout = IMAGE_LAYOUT_GENERAL;

		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]);

		renderpasses_envmap.resize(envmapCount);

		for (uint32_t i = 0; i < envmapCount; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], RTV, i * 6, 6, 0, 1);
			assert(subresource_index == i);

			RenderPassDesc renderpassdesc;
			renderpassdesc.attachments.push_back(RenderPassAttachment::RenderTarget(&textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], RenderPassAttachment::LOADOP_DONTCARE));
			renderpassdesc.attachments.back().subresource = subresource_index; 
			
			renderpassdesc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					&envrenderingDepthBuffer,
					RenderPassAttachment::LOADOP_CLEAR
				)
			);
			
			device->CreateRenderPass(&renderpassdesc, &renderpasses_envmap[subresource_index]);
		}
		for (uint32_t i = 0; i < textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY].GetDesc().MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], SRV, 0, desc.ArraySize, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], UAV, 0, desc.ArraySize, i, 1);
			assert(subresource_index == i);
		}

		// debug probe views, individual cubes:
		for (uint32_t i = 0; i < envmapCount; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], SRV, i * 6, 6, 0, -1);
			assert(subresource_index == textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY].GetDesc().MipLevels + i);
		}
	}
}
void RefreshEnvProbes(CommandList cmd)
{
	if (probesToRefresh.empty())
	{
		return;
	}

	const Scene& scene = GetScene();

	device->EventBegin("EnvironmentProbe Refresh", cmd);

	Viewport vp;
	vp.Height = envmapRes;
	vp.Width = envmapRes;
	device->BindViewports(1, &vp, cmd);

	const float zNearP = GetCamera().zNearP;
	const float zFarP = GetCamera().zFarP;

	for (uint32_t probeIndex : probesToRefresh)
	{
		const EnvironmentProbeComponent& probe = scene.probes[probeIndex];
		Entity entity = scene.probes.GetEntity(probeIndex);

		const SHCAM cameras[] = {
			SHCAM(probe.position, XMFLOAT4(0.5f, -0.5f, -0.5f, -0.5f), zNearP, zFarP, XM_PIDIV2), //+x
			SHCAM(probe.position, XMFLOAT4(0.5f, 0.5f, 0.5f, -0.5f), zNearP, zFarP, XM_PIDIV2), //-x
			SHCAM(probe.position, XMFLOAT4(1, 0, 0, -0), zNearP, zFarP, XM_PIDIV2), //+y
			SHCAM(probe.position, XMFLOAT4(0, 0, 0, -1), zNearP, zFarP, XM_PIDIV2), //-y
			SHCAM(probe.position, XMFLOAT4(0.707f, 0, 0, -0.707f), zNearP, zFarP, XM_PIDIV2), //+z
			SHCAM(probe.position, XMFLOAT4(0, 0.707f, 0.707f, 0), zNearP, zFarP, XM_PIDIV2), //-z
		};
		Frustum frusta[arraysize(cameras)];

		CubemapRenderCB cb;
		for (uint32_t i = 0; i < arraysize(cameras); ++i)
		{
			frusta[i] = cameras[i].frustum;
			XMStoreFloat4x4(&cb.xCubemapRenderCams[i].VP, cameras[i].VP);
			cb.xCubemapRenderCams[i].properties = uint4(i, 0, 0, 0);
		}

		device->UpdateBuffer(&constantBuffers[CBTYPE_CUBEMAPRENDER], &cb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubemapRenderCB), cmd);


		CameraCB camcb;
		camcb.g_xCamera_CamPos = probe.position; // only this will be used by envprobe rendering shaders the rest is read from cubemaprenderCB
		device->UpdateBuffer(&constantBuffers[CBTYPE_CAMERA], &camcb, cmd);

		const LayerComponent* probe_layer = scene.layers.GetComponent(entity);
		const uint32_t layerMask = probe_layer == nullptr ?  ~0 : probe_layer->GetLayerMask();

		SPHERE culler = SPHERE(probe.position, zFarP);

		RenderQueue renderQueue;
		for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
		{
			const AABB& aabb = scene.aabb_objects[i];
			if (culler.intersects(aabb))
			{
				Entity cullable_entity = scene.aabb_objects.GetEntity(i);
				const LayerComponent* layer = scene.layers.GetComponent(cullable_entity);
				if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
				{
					continue;
				}

				const ObjectComponent& object = scene.objects[i];
				if (object.IsRenderable())
				{
					RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
					size_t meshIndex = scene.meshes.GetIndex(object.meshID);
					batch->Create(meshIndex, i, 0);
					renderQueue.add(batch);
				}
			}
		}

		BindConstantBuffers(VS, cmd);
		BindConstantBuffers(PS, cmd);

		device->RenderPassBegin(&renderpasses_envmap[probe.textureIndex], cmd);

		if (scene.weather.IsRealisticSky())
		{
			// Refresh atmospheric textures, since each probe has different positions
			RefreshAtmosphericScatteringTextures(cmd);
		}

		// Bind the atmospheric textures, as lighting and sky needs them
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);

		if (!renderQueue.empty())
		{
			BindShadowmaps(PS, cmd);
			device->BindResource(PS, GetGlobalLightmap(), TEXSLOT_GLOBALLIGHTMAP, cmd);

			RenderMeshes(renderQueue, RENDERPASS_ENVMAPCAPTURE, RENDERTYPE_ALL, cmd, false, frusta, arraysize(frusta));

			GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
		}

		// sky
		{
			if (scene.weather.skyMap != nullptr)
			{
				device->BindPipelineState(&PSO_sky[SKYRENDERING_ENVMAPCAPTURE_STATIC], cmd);
				device->BindResource(PS, scene.weather.skyMap->texture, TEXSLOT_ONDEMAND0, cmd);
			}
			else
			{
				device->BindPipelineState(&PSO_sky[SKYRENDERING_ENVMAPCAPTURE_DYNAMIC], cmd);
			}

			device->DrawInstanced(240, 6, 0, 0, cmd); // 6 instances so it will be replicated for every cubemap face
		}

		device->RenderPassEnd(cmd);

		MIPGEN_OPTIONS mipopt;
		mipopt.arrayIndex = probe.textureIndex;
		GenerateMipChain(textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], MIPGENFILTER_LINEAR, cmd, mipopt);

		// Filter the enviroment map mip chain according to BRDF:
		//	A bit similar to MIP chain generation, but its input is the MIP-mapped texture,
		//	and we generatethe filtered MIPs from bottom to top.
		device->EventBegin("FilterEnvMap", cmd);
		{
			TextureDesc desc = textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY].GetDesc();
			int arrayIndex = probe.textureIndex;

			device->BindComputeShader(&shaders[CSTYPE_FILTERENVMAP], cmd);

			desc.Width = 1;
			desc.Height = 1;
			for (uint32_t i = desc.MipLevels - 1; i > 0; --i)
			{
				device->BindUAV(CS, &textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], 0, cmd, i);
				device->BindResource(CS, &textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], TEXSLOT_UNIQUE0, cmd, std::max(0, (int)i - 2));

				FilterEnvmapCB cb;
				cb.filterResolution.x = desc.Width;
				cb.filterResolution.y = desc.Height;
				cb.filterResolution_rcp.x = 1.0f / cb.filterResolution.x;
				cb.filterResolution_rcp.y = 1.0f / cb.filterResolution.y;
				cb.filterArrayIndex = arrayIndex;
				cb.filterRoughness = (float)i / (float)desc.MipLevels;
				cb.filterRayCount = 128;
				device->UpdateBuffer(&constantBuffers[CBTYPE_FILTERENVMAP], &cb, cmd);
				device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_FILTERENVMAP], CB_GETBINDSLOT(FilterEnvmapCB), cmd);

				device->Dispatch(
					std::max(1u, (uint32_t)ceilf((float)desc.Width / GENERATEMIPCHAIN_2D_BLOCK_SIZE)),
					std::max(1u, (uint32_t)ceilf((float)desc.Height / GENERATEMIPCHAIN_2D_BLOCK_SIZE)),
					6,
					cmd);

				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);

				desc.Width *= 2;
				desc.Height *= 2;
			}
			device->UnbindUAVs(0, 1, cmd);
		}
		device->EventEnd(cmd);

	}

	device->EventEnd(cmd); // EnvironmentProbe Refresh
}

static const uint32_t maxImpostorCount = 8;
static const uint32_t impostorTextureArraySize = maxImpostorCount * impostorCaptureAngles * 3;
static const uint32_t impostorTextureDim = 128;
static Texture impostorDepthStencil;
static std::vector<RenderPass> renderpasses_impostor;
vector<uint32_t> impostorsToRefresh(maxImpostorCount);
void ManageImpostors()
{
	impostorsToRefresh.clear();

	Scene& scene = GetScene();

	for (size_t impostorIndex = 0; impostorIndex < std::min((size_t)maxImpostorCount, scene.impostors.GetCount()); ++impostorIndex)
	{
		ImpostorComponent& impostor = scene.impostors[impostorIndex];
		if (!impostor.IsDirty())
		{
			continue;
		}
		impostor.SetDirty(false);

		impostorsToRefresh.push_back((uint32_t)impostorIndex);
	}

	if (!impostorsToRefresh.empty() && !textures[TEXTYPE_2D_IMPOSTORARRAY].IsValid())
	{
		TextureDesc desc;
		desc.Width = impostorTextureDim;
		desc.Height = impostorTextureDim;

		desc.BindFlags = BIND_DEPTH_STENCIL;
		desc.ArraySize = 1;
		desc.Format = FORMAT_D16_UNORM;
		device->CreateTexture(&desc, nullptr, &impostorDepthStencil);
		device->SetName(&impostorDepthStencil, "impostorDepthStencil");

		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.ArraySize = impostorTextureArraySize;
		desc.Format = FORMAT_R8G8B8A8_UNORM;

		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_2D_IMPOSTORARRAY]);
		device->SetName(&textures[TEXTYPE_2D_IMPOSTORARRAY], "ImpostorArray");

		renderpasses_impostor.resize(desc.ArraySize);

		for (uint32_t i = 0; i < desc.ArraySize; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&textures[TEXTYPE_2D_IMPOSTORARRAY], RTV, i, 1, 0, 1);
			assert(subresource_index == i);

			RenderPassDesc renderpassdesc;
			renderpassdesc.attachments.push_back(RenderPassAttachment::RenderTarget(&textures[TEXTYPE_2D_IMPOSTORARRAY], RenderPassAttachment::LOADOP_CLEAR));
			renderpassdesc.attachments.back().subresource = subresource_index;

			renderpassdesc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					&impostorDepthStencil,
					RenderPassAttachment::LOADOP_CLEAR
				)
			);
			
			device->CreateRenderPass(&renderpassdesc, &renderpasses_impostor[subresource_index]);
		}
	}
}
void RefreshImpostors(CommandList cmd)
{
	if (impostorsToRefresh.empty())
	{
		return;
	}

	const Scene& scene = GetScene();

	device->EventBegin("Impostor Refresh", cmd);

	BindConstantBuffers(VS, cmd);
	BindConstantBuffers(PS, cmd);

	struct InstBuf
	{
		Instance instance;
		InstancePrev instancePrev;
		InstanceAtlas instanceAtlas;
	};
	GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(InstBuf), cmd);
	volatile InstBuf* buff = (volatile InstBuf*)mem.data;
	buff->instance.Create(IDENTITYMATRIX);
	buff->instancePrev.Create(IDENTITYMATRIX);
	buff->instanceAtlas.Create(XMFLOAT4(1, 1, 0, 0));

	for (uint32_t impostorIndex : impostorsToRefresh)
	{
		const ImpostorComponent& impostor = scene.impostors[impostorIndex];

		Entity entity = scene.impostors.GetEntity(impostorIndex);
		const MeshComponent& mesh = *scene.meshes.GetComponent(entity);

		// impostor camera will fit around mesh bounding sphere:
		const SPHERE boundingsphere = mesh.GetBoundingSphere();

		const GPUBuffer* vbs[] = {
			mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
			&mesh.vertexBuffer_UV0,
			&mesh.vertexBuffer_UV1,
			&mesh.vertexBuffer_ATL,
			&mesh.vertexBuffer_COL,
			&mesh.vertexBuffer_TAN,
			mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
			mem.buffer
		};
		uint32_t strides[] = {
			sizeof(MeshComponent::Vertex_POS),
			sizeof(MeshComponent::Vertex_TEX),
			sizeof(MeshComponent::Vertex_TEX),
			sizeof(MeshComponent::Vertex_TEX),
			sizeof(MeshComponent::Vertex_COL),
			sizeof(MeshComponent::Vertex_TAN),
			sizeof(MeshComponent::Vertex_POS),
			sizeof(InstBuf)
		};
		uint32_t offsets[] = {
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			mem.offset
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		device->BindIndexBuffer(&mesh.indexBuffer, mesh.GetIndexFormat(), 0, cmd);

		for (int prop = 0; prop < 3; ++prop)
		{
			switch (prop)
			{
			case 0:
				device->BindPipelineState(&PSO_captureimpostor_albedo, cmd);
				break;
			case 1:
				device->BindPipelineState(&PSO_captureimpostor_normal, cmd);
				break;
			case 2:
				device->BindPipelineState(&PSO_captureimpostor_surface, cmd);
				break;
			}

			for (size_t i = 0; i < impostorCaptureAngles; ++i)
			{
				int textureIndex = (int)(impostorIndex * impostorCaptureAngles * 3 + prop * impostorCaptureAngles + i);
				device->RenderPassBegin(&renderpasses_impostor[textureIndex], cmd);

				Viewport viewport;
				viewport.Height = (float)impostorTextureDim;
				viewport.Width = (float)impostorTextureDim;
				device->BindViewports(1, &viewport, cmd);


				CameraComponent impostorcamera;
				impostorcamera.SetCustomProjectionEnabled(true);
				TransformComponent camera_transform;

				camera_transform.ClearTransform();
				camera_transform.Translate(boundingsphere.center);

				XMMATRIX P = XMMatrixOrthographicOffCenterLH(-boundingsphere.radius, boundingsphere.radius, -boundingsphere.radius, boundingsphere.radius, -boundingsphere.radius, boundingsphere.radius);
				XMStoreFloat4x4(&impostorcamera.Projection, P);
				XMVECTOR Q = XMQuaternionNormalize(XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), XM_2PI * (float)i / (float)impostorCaptureAngles));
				XMStoreFloat4(&camera_transform.rotation_local, Q);

				camera_transform.UpdateTransform();
				impostorcamera.TransformCamera(camera_transform);
				impostorcamera.UpdateCamera();

				UpdateCameraCB(impostorcamera, cmd);

				for (auto& subset : mesh.subsets)
				{
					if (subset.indexCount == 0)
					{
						continue;
					}
					const MaterialComponent& material = *scene.materials.GetComponent(subset.materialID);

					device->BindConstantBuffer(VS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);
					device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);

					const GPUResource* res[] = {
						material.GetBaseColorMap(),
						material.GetNormalMap(),
						material.GetSurfaceMap(),
					};
					device->BindResources(PS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

					device->DrawIndexedInstanced(subset.indexCount, 1, subset.indexOffset, 0, 0, cmd);
				}

				device->RenderPassEnd(cmd);
			}
		}

	}

	UpdateCameraCB(GetCamera(), cmd);

	device->EventEnd(cmd);
}

void VoxelRadiance(CommandList cmd)
{
	if (!GetVoxelRadianceEnabled())
	{
		return;
	}

	device->EventBegin("Voxel Radiance", cmd);
	auto range = wiProfiler::BeginRangeGPU("Voxel Radiance", cmd);

	const Scene& scene = GetScene();

	static RenderPass renderpass_voxelize;

	if (!textures[TEXTYPE_3D_VOXELRADIANCE].IsValid())
	{
		TextureDesc desc;
		desc.type = TextureDesc::TEXTURE_3D;
		desc.Width = voxelSceneData.res;
		desc.Height = voxelSceneData.res;
		desc.Depth = voxelSceneData.res;
		desc.MipLevels = 0;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		desc.Usage = USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_3D_VOXELRADIANCE]);

		for (uint32_t i = 0; i < textures[TEXTYPE_3D_VOXELRADIANCE].GetDesc().MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&textures[TEXTYPE_3D_VOXELRADIANCE], SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&textures[TEXTYPE_3D_VOXELRADIANCE], UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}

		RenderPassDesc renderpassdesc;
		renderpassdesc._flags = RenderPassDesc::FLAG_ALLOW_UAV_WRITES;
		device->CreateRenderPass(&renderpassdesc, &renderpass_voxelize);
	}
	if (voxelSceneData.secondaryBounceEnabled && !textures[TEXTYPE_3D_VOXELRADIANCE_HELPER].IsValid())
	{
		const TextureDesc& desc = textures[TEXTYPE_3D_VOXELRADIANCE].GetDesc();
		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_3D_VOXELRADIANCE_HELPER]);

		for (uint32_t i = 0; i < desc.MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&textures[TEXTYPE_3D_VOXELRADIANCE_HELPER], SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&textures[TEXTYPE_3D_VOXELRADIANCE_HELPER], UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}
	}
	if (!resourceBuffers[RBTYPE_VOXELSCENE].IsValid())
	{
		GPUBufferDesc desc;
		desc.StructureByteStride = sizeof(uint32_t) * 2;
		desc.ByteWidth = desc.StructureByteStride * voxelSceneData.res * voxelSceneData.res * voxelSceneData.res;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;

		device->CreateBuffer(&desc, nullptr, &resourceBuffers[RBTYPE_VOXELSCENE]);
	}

	Texture* result = &textures[TEXTYPE_3D_VOXELRADIANCE];

	AABB bbox;
	XMFLOAT3 extents = voxelSceneData.extents;
	XMFLOAT3 center = voxelSceneData.center;
	bbox.createFromHalfWidth(center, extents);


	RenderQueue renderQueue;
	for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
	{
		const AABB& aabb = scene.aabb_objects[i];
		if (bbox.intersects(aabb))
		{
			const ObjectComponent& object = scene.objects[i];
			if (object.IsRenderable())
			{
				RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
				size_t meshIndex = scene.meshes.GetIndex(object.meshID);
				batch->Create(meshIndex, i, 0);
				renderQueue.add(batch);
			}
		}
	}

	if (!renderQueue.empty())
	{
		Viewport vp;
		vp.Width = (float)voxelSceneData.res;
		vp.Height = (float)voxelSceneData.res;
		device->BindViewports(1, &vp, cmd);

		GPUResource* UAVs[] = { &resourceBuffers[RBTYPE_VOXELSCENE] };
		device->BindUAVs(PS, UAVs, 0, 1, cmd);

		BindCommonResources(cmd);
		BindShadowmaps(PS, cmd);
		BindConstantBuffers(VS, cmd);
		BindConstantBuffers(PS, cmd);

		device->RenderPassBegin(&renderpass_voxelize, cmd);
		RenderMeshes(renderQueue, RENDERPASS_VOXELIZE, RENDERTYPE_OPAQUE, cmd);
		device->RenderPassEnd(cmd);

		GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);

		// Copy the packed voxel scene data to a 3D texture, then delete the voxel scene emission data. The cone tracing will operate on the 3D texture
		device->EventBegin("Voxel Scene Copy - Clear", cmd);
		device->BindUAV(CS, &resourceBuffers[RBTYPE_VOXELSCENE], 0, cmd);
		device->BindUAV(CS, &textures[TEXTYPE_3D_VOXELRADIANCE], 1, cmd);

		static bool smooth_copy = true;
		if (smooth_copy)
		{
			device->BindComputeShader(&shaders[CSTYPE_VOXELSCENECOPYCLEAR_TEMPORALSMOOTHING], cmd);
		}
		else
		{
			device->BindComputeShader(&shaders[CSTYPE_VOXELSCENECOPYCLEAR], cmd);
		}
		device->Dispatch((uint32_t)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 256), 1, 1, cmd);
		device->EventEnd(cmd);

		if (voxelSceneData.secondaryBounceEnabled)
		{
			device->EventBegin("Voxel Radiance Secondary Bounce", cmd);
			device->UnbindUAVs(1, 1, cmd);
			// Pre-integrate the voxel texture by creating blurred mip levels:
			GenerateMipChain(textures[TEXTYPE_3D_VOXELRADIANCE], MIPGENFILTER_LINEAR, cmd);
			device->BindUAV(CS, &textures[TEXTYPE_3D_VOXELRADIANCE_HELPER], 0, cmd);
			device->BindResource(CS, &textures[TEXTYPE_3D_VOXELRADIANCE], 0, cmd);
			device->BindResource(CS, &resourceBuffers[RBTYPE_VOXELSCENE], 1, cmd);
			device->BindComputeShader(&shaders[CSTYPE_VOXELRADIANCESECONDARYBOUNCE], cmd);
			device->Dispatch((uint32_t)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 64), 1, 1, cmd);
			device->EventEnd(cmd);

			device->EventBegin("Voxel Scene Clear Normals", cmd);
			device->UnbindResources(1, 1, cmd);
			device->BindUAV(CS, &resourceBuffers[RBTYPE_VOXELSCENE], 0, cmd);
			device->BindComputeShader(&shaders[CSTYPE_VOXELCLEARONLYNORMAL], cmd);
			device->Dispatch((uint32_t)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 256), 1, 1, cmd);
			device->EventEnd(cmd);

			result = &textures[TEXTYPE_3D_VOXELRADIANCE_HELPER];
		}

		device->UnbindUAVs(0, 2, cmd);


		// Pre-integrate the voxel texture by creating blurred mip levels:
		{
			GenerateMipChain(*result, MIPGENFILTER_LINEAR, cmd);
		}
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}



inline XMUINT3 GetEntityCullingTileCount()
{
	return XMUINT3(
		(GetInternalResolution().x + TILED_CULLING_BLOCKSIZE - 1) / TILED_CULLING_BLOCKSIZE,
		(GetInternalResolution().y + TILED_CULLING_BLOCKSIZE - 1) / TILED_CULLING_BLOCKSIZE,
		1);
}
void ComputeTiledLightCulling(
	const Texture& depthbuffer,
	CommandList cmd
)
{
	auto range = wiProfiler::BeginRangeGPU("Entity Culling", cmd);

	int _width = GetInternalResolution().x;
	int _height = GetInternalResolution().y;

	const XMUINT3 tileCount = GetEntityCullingTileCount();

	static int _savedWidth = 0;
	static int _savedHeight = 0;
	bool _resolutionChanged = false;
	if (_savedWidth != _width || _savedHeight != _height)
	{
		_resolutionChanged = true;
		_savedWidth = _width;
		_savedHeight = _height;
	}

	static GPUBuffer frustumBuffer;
	if (!frustumBuffer.IsValid() || _resolutionChanged)
	{
		GPUBufferDesc bd;
		bd.StructureByteStride = sizeof(XMFLOAT4) * 4; // storing 4 planes for every tile
		bd.ByteWidth = bd.StructureByteStride * tileCount.x * tileCount.y;
		bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.Usage = USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
		device->CreateBuffer(&bd, nullptr, &frustumBuffer);

		device->SetName(&frustumBuffer, "FrustumBuffer");
	}
	if (_resolutionChanged)
	{
		GPUBufferDesc bd;
		bd.StructureByteStride = sizeof(uint);
		bd.ByteWidth = tileCount.x * tileCount.y * bd.StructureByteStride * SHADER_ENTITY_TILE_BUCKET_COUNT;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		device->CreateBuffer(&bd, nullptr, &resourceBuffers[RBTYPE_ENTITYTILES_OPAQUE]);
		device->CreateBuffer(&bd, nullptr, &resourceBuffers[RBTYPE_ENTITYTILES_TRANSPARENT]);

		device->SetName(&resourceBuffers[RBTYPE_ENTITYTILES_OPAQUE], "EntityTiles_Opaque");
		device->SetName(&resourceBuffers[RBTYPE_ENTITYTILES_TRANSPARENT], "EntityTiles_Transparent");
	}

	BindCommonResources(cmd);

	// calculate the per-tile frustums once:
	static bool frustumsComplete = false;
	static XMFLOAT4X4 _savedProjection;
	if (memcmp(&_savedProjection, &GetCamera().Projection, sizeof(XMFLOAT4X4)) != 0)
	{
		_savedProjection = GetCamera().Projection;
		frustumsComplete = false;
	}
	if(!frustumsComplete || _resolutionChanged)
	{
		frustumsComplete = true;

		const GPUResource* uavs[] = { 
			&frustumBuffer 
		};

		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);
		device->BindComputeShader(&shaders[CSTYPE_TILEFRUSTUMS], cmd);

		device->Dispatch(
			(tileCount.x + TILED_CULLING_BLOCKSIZE - 1) / TILED_CULLING_BLOCKSIZE,
			(tileCount.y + TILED_CULLING_BLOCKSIZE - 1) / TILED_CULLING_BLOCKSIZE,
			1,
			cmd
		);
		device->UnbindUAVs(0, arraysize(uavs), cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	if (!textures[TEXTYPE_2D_DEBUGUAV].IsValid() || _resolutionChanged)
	{
		TextureDesc desc;
		desc.Width = (uint32_t)_width;
		desc.Height = (uint32_t)_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = FORMAT_R8G8B8A8_UNORM;
		desc.SampleCount = 1;
		desc.Usage = USAGE_DEFAULT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_2D_DEBUGUAV]);
	}

	// Perform the culling
	{
		device->EventBegin("Entity Culling", cmd);

		device->UnbindResources(SBSLOT_ENTITYTILES, 1, cmd);

		device->BindResource(CS, &frustumBuffer, SBSLOT_TILEFRUSTUMS, cmd);

		if (GetDebugLightCulling())
		{
		    device->BindComputeShader(&shaders[GetAdvancedLightCulling() ? CSTYPE_LIGHTCULLING_ADVANCED_DEBUG : CSTYPE_LIGHTCULLING_DEBUG], cmd);
			device->BindUAV(CS, &textures[TEXTYPE_2D_DEBUGUAV], 3, cmd);
		}
		else
		{
		    device->BindComputeShader(&shaders[GetAdvancedLightCulling() ? CSTYPE_LIGHTCULLING_ADVANCED : CSTYPE_LIGHTCULLING], cmd);
		}

		const FrameCulling& frameCulling = frameCullings.at(&GetCamera());

		BindConstantBuffers(CS, cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);

		GPUResource* uavs[] = {
			&resourceBuffers[RBTYPE_ENTITYTILES_TRANSPARENT],
			&resourceBuffers[RBTYPE_ENTITYTILES_OPAQUE],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(tileCount.x, tileCount.y, 1, cmd);
		
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, 8, cmd); // this unbinds pretty much every uav

		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range);
}


void ResolveMSAADepthBuffer(const Texture& dst, const Texture& src, CommandList cmd)
{
	device->EventBegin("ResolveMSAADepthBuffer", cmd);

	device->BindResource(CS, &src, TEXSLOT_ONDEMAND0, cmd);
	device->BindUAV(CS, &dst, 0, cmd);

	const TextureDesc& desc = src.GetDesc();

	device->BindComputeShader(&shaders[CSTYPE_RESOLVEMSAADEPTHSTENCIL], cmd);
	device->Dispatch((desc.Width + 7) / 8, (desc.Height + 7) / 8, 1, cmd);


	device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
	device->UnbindUAVs(0, 1, cmd);

	device->EventEnd(cmd);
}
void DownsampleDepthBuffer(const wiGraphics::Texture& src, wiGraphics::CommandList cmd)
{
	device->EventBegin("DownsampleDepthBuffer", cmd);

	device->BindPipelineState(&PSO_downsampledepthbuffer, cmd);

	device->BindResource(PS, &src, TEXSLOT_ONDEMAND0, cmd);

	device->Draw(3, 0, cmd);

	device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);

	device->EventEnd(cmd);
}

void GenerateMipChain(const Texture& texture, MIPGENFILTER filter, CommandList cmd, const MIPGEN_OPTIONS& options)
{
	TextureDesc desc = texture.GetDesc();

	if (desc.MipLevels < 2)
	{
		assert(0);
		return;
	}


	bool hdr = !device->IsFormatUnorm(desc.Format);

	if (desc.type == TextureDesc::TEXTURE_1D)
	{
		assert(0); // not implemented
	}
	else if (desc.type == TextureDesc::TEXTURE_2D)
	{

		if (desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
		{

			if (desc.ArraySize > 6)
			{
				// Cubearray
				assert(options.arrayIndex >= 0 && "You should only filter a specific cube in the array for now, so provide its index!");

				switch (filter)
				{
				case MIPGENFILTER_POINT:
					device->EventBegin("GenerateMipChain CubeArray - PointFilter", cmd);
					device->BindComputeShader(&shaders[hdr ? CSTYPE_GENERATEMIPCHAINCUBEARRAY_FLOAT4 : CSTYPE_GENERATEMIPCHAINCUBEARRAY_UNORM4], cmd);
					device->BindSampler(CS, &samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, cmd);
					break;
				case MIPGENFILTER_LINEAR:
					device->EventBegin("GenerateMipChain CubeArray - LinearFilter", cmd);
					device->BindComputeShader(&shaders[hdr ? CSTYPE_GENERATEMIPCHAINCUBEARRAY_FLOAT4 : CSTYPE_GENERATEMIPCHAINCUBEARRAY_UNORM4], cmd);
					device->BindSampler(CS, &samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, cmd);
					break;
				default:
					assert(0);
					break;
				}

				for (uint32_t i = 0; i < desc.MipLevels - 1; ++i)
				{
					device->BindUAV(CS, &texture, 0, cmd, i + 1);
					device->BindResource(CS, &texture, TEXSLOT_UNIQUE0, cmd, i);
					desc.Width = std::max(1u, desc.Width / 2);
					desc.Height = std::max(1u, desc.Height / 2);

					GenerateMIPChainCB cb;
					cb.outputResolution.x = desc.Width;
					cb.outputResolution.y = desc.Height;
					cb.outputResolution_rcp.x = 1.0f / cb.outputResolution.x;
					cb.outputResolution_rcp.y = 1.0f / cb.outputResolution.y;
					cb.arrayIndex = options.arrayIndex;
					cb.mipgen_options = 0;
					device->UpdateBuffer(&constantBuffers[CBTYPE_MIPGEN], &cb, cmd);
					device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_MIPGEN], CB_GETBINDSLOT(GenerateMIPChainCB), cmd);

					device->Dispatch(
						std::max(1u, (desc.Width + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
						std::max(1u, (desc.Height + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
						6,
						cmd);

					GPUBarrier barriers[] = {
						GPUBarrier::Memory(),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);
				}
			}
			else
			{
				// Cubemap
				switch (filter)
				{
				case MIPGENFILTER_POINT:
					device->EventBegin("GenerateMipChain Cube - PointFilter", cmd);
					device->BindComputeShader(&shaders[hdr ? CSTYPE_GENERATEMIPCHAINCUBE_FLOAT4 : CSTYPE_GENERATEMIPCHAINCUBE_UNORM4], cmd);
					device->BindSampler(CS, &samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, cmd);
					break;
				case MIPGENFILTER_LINEAR:
					device->EventBegin("GenerateMipChain Cube - LinearFilter", cmd);
					device->BindComputeShader(&shaders[hdr ? CSTYPE_GENERATEMIPCHAINCUBE_FLOAT4 : CSTYPE_GENERATEMIPCHAINCUBE_UNORM4], cmd);
					device->BindSampler(CS, &samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, cmd);
					break;
				default:
					assert(0); // not implemented
					break;
				}

				for (uint32_t i = 0; i < desc.MipLevels - 1; ++i)
				{
					device->BindUAV(CS, &texture, 0, cmd, i + 1);
					device->BindResource(CS, &texture, TEXSLOT_UNIQUE0, cmd, i);
					desc.Width = std::max(1u, desc.Width / 2);
					desc.Height = std::max(1u, desc.Height / 2);

					GenerateMIPChainCB cb;
					cb.outputResolution.x = desc.Width;
					cb.outputResolution.y = desc.Height;
					cb.outputResolution_rcp.x = 1.0f / cb.outputResolution.x;
					cb.outputResolution_rcp.y = 1.0f / cb.outputResolution.y;
					cb.arrayIndex = 0;
					cb.mipgen_options = 0;
					device->UpdateBuffer(&constantBuffers[CBTYPE_MIPGEN], &cb, cmd);
					device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_MIPGEN], CB_GETBINDSLOT(GenerateMIPChainCB), cmd);

					device->Dispatch(
						std::max(1u, (desc.Width + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
						std::max(1u, (desc.Height + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
						6,
						cmd);

					GPUBarrier barriers[] = {
						GPUBarrier::Memory(),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);
				}
			}

		}
		else
		{
			// Texture
			switch (filter)
			{
			case MIPGENFILTER_POINT:
				device->EventBegin("GenerateMipChain 2D - PointFilter", cmd);
				device->BindComputeShader(&shaders[hdr ? CSTYPE_GENERATEMIPCHAIN2D_FLOAT4 : CSTYPE_GENERATEMIPCHAIN2D_UNORM4], cmd);
				device->BindSampler(CS, &samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, cmd);
				break;
			case MIPGENFILTER_LINEAR:
				device->EventBegin("GenerateMipChain 2D - LinearFilter", cmd);
				device->BindComputeShader(&shaders[hdr ? CSTYPE_GENERATEMIPCHAIN2D_FLOAT4 : CSTYPE_GENERATEMIPCHAIN2D_UNORM4], cmd);
				device->BindSampler(CS, &samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, cmd);
				break;
			case MIPGENFILTER_GAUSSIAN:
			{
				assert(options.gaussian_temp != nullptr); // needed for separate filter!
				device->EventBegin("GenerateMipChain 2D - GaussianFilter", cmd);
				// Gaussian filter is a bit different as we do it in a separable way:
				for (uint32_t i = 0; i < desc.MipLevels - 1; ++i)
				{
					Postprocess_Blur_Gaussian(texture, *options.gaussian_temp, texture, cmd, i, i + 1 , options.wide_gauss);
				}
				device->EventEnd(cmd);
				return;
			}
				break;
			default:
				assert(0);
				break;
			}

			for (uint32_t i = 0; i < desc.MipLevels - 1; ++i)
			{
				device->BindUAV(CS, &texture, 0, cmd, i + 1);
				device->BindResource(CS, &texture, TEXSLOT_UNIQUE0, cmd, i);
				desc.Width = std::max(1u, desc.Width / 2);
				desc.Height = std::max(1u, desc.Height / 2);

				GenerateMIPChainCB cb;
				cb.outputResolution.x = desc.Width;
				cb.outputResolution.y = desc.Height;
				cb.outputResolution_rcp.x = 1.0f / cb.outputResolution.x;
				cb.outputResolution_rcp.y = 1.0f / cb.outputResolution.y;
				cb.arrayIndex = options.arrayIndex >= 0 ? (uint)options.arrayIndex : 0;
				cb.mipgen_options = 0;
				if (options.preserve_coverage)
				{
					cb.mipgen_options |= MIPGEN_OPTION_BIT_PRESERVE_COVERAGE;
				}
				device->UpdateBuffer(&constantBuffers[CBTYPE_MIPGEN], &cb, cmd);
				device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_MIPGEN], CB_GETBINDSLOT(GenerateMIPChainCB), cmd);

				device->Dispatch(
					std::max(1u, (desc.Width + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
					std::max(1u, (desc.Height + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
					1,
					cmd);

				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}
		}

		device->UnbindResources(TEXSLOT_UNIQUE0, 1, cmd);
		device->UnbindUAVs(0, 1, cmd);

		device->EventEnd(cmd);
	}
	else if (desc.type == TextureDesc::TEXTURE_3D)
	{
		switch (filter)
		{
		case MIPGENFILTER_POINT:
			device->EventBegin("GenerateMipChain 3D - PointFilter", cmd);
			device->BindComputeShader(&shaders[hdr ? CSTYPE_GENERATEMIPCHAIN3D_FLOAT4 : CSTYPE_GENERATEMIPCHAIN3D_UNORM4], cmd);
			device->BindSampler(CS, &samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, cmd);
			break;
		case MIPGENFILTER_LINEAR:
			device->EventBegin("GenerateMipChain 3D - LinearFilter", cmd);
			device->BindComputeShader(&shaders[hdr ? CSTYPE_GENERATEMIPCHAIN3D_FLOAT4 : CSTYPE_GENERATEMIPCHAIN3D_UNORM4], cmd);
			device->BindSampler(CS, &samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, cmd);
			break;
		default:
			assert(0); // not implemented
			break;
		}

		for (uint32_t i = 0; i < desc.MipLevels - 1; ++i)
		{
			device->BindUAV(CS, &texture, 0, cmd, i + 1);
			device->BindResource(CS, &texture, TEXSLOT_UNIQUE0, cmd, i);
			desc.Width = std::max(1u, desc.Width / 2);
			desc.Height = std::max(1u, desc.Height / 2);
			desc.Depth = std::max(1u, desc.Depth / 2);

			GenerateMIPChainCB cb;
			cb.outputResolution.x = desc.Width;
			cb.outputResolution.y = desc.Height;
			cb.outputResolution.z = desc.Depth;
			cb.outputResolution_rcp.x = 1.0f / cb.outputResolution.x;
			cb.outputResolution_rcp.y = 1.0f / cb.outputResolution.y;
			cb.outputResolution_rcp.z = 1.0f / cb.outputResolution.z;
			cb.arrayIndex = options.arrayIndex >= 0 ? (uint)options.arrayIndex : 0;
			cb.mipgen_options = 0;
			device->UpdateBuffer(&constantBuffers[CBTYPE_MIPGEN], &cb, cmd);
			device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_MIPGEN], CB_GETBINDSLOT(GenerateMIPChainCB), cmd);

			device->Dispatch(
				std::max(1u, (desc.Width + GENERATEMIPCHAIN_3D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_3D_BLOCK_SIZE),
				std::max(1u, (desc.Height + GENERATEMIPCHAIN_3D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_3D_BLOCK_SIZE),
				std::max(1u, (desc.Depth + GENERATEMIPCHAIN_3D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_3D_BLOCK_SIZE),
				cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindResources(TEXSLOT_UNIQUE0, 1, cmd);
		device->UnbindUAVs(0, 1, cmd);

		device->EventEnd(cmd);
	}
	else
	{
		assert(0);
	}
}

void CopyTexture2D(const Texture& dst, uint32_t DstMIP, uint32_t DstX, uint32_t DstY, const Texture& src, uint32_t SrcMIP, CommandList cmd, BORDEREXPANDSTYLE borderExpand)
{
	const TextureDesc& desc_dst = dst.GetDesc();
	const TextureDesc& desc_src = src.GetDesc();

	assert(desc_dst.BindFlags & BIND_UNORDERED_ACCESS);
	assert(desc_src.BindFlags & BIND_SHADER_RESOURCE);

	bool hdr = !device->IsFormatUnorm(desc_dst.Format);

	if (borderExpand == BORDEREXPAND_DISABLE)
	{
		if (hdr)
		{
			device->EventBegin("CopyTexture_FLOAT4", cmd);
			device->BindComputeShader(&shaders[CSTYPE_COPYTEXTURE2D_FLOAT4], cmd);
		}
		else
		{
			device->EventBegin("CopyTexture_UNORM4", cmd);
			device->BindComputeShader(&shaders[CSTYPE_COPYTEXTURE2D_UNORM4], cmd);
		}
	}
	else
	{
		if (hdr)
		{
			device->EventBegin("CopyTexture_BORDEREXPAND_FLOAT4", cmd);
			device->BindComputeShader(&shaders[CSTYPE_COPYTEXTURE2D_FLOAT4_BORDEREXPAND], cmd);
		}
		else
		{
			device->EventBegin("CopyTexture_BORDEREXPAND_UNORM4", cmd);
			device->BindComputeShader(&shaders[CSTYPE_COPYTEXTURE2D_UNORM4_BORDEREXPAND], cmd);
		}
	}

	CopyTextureCB cb;
	cb.xCopyDest.x = DstX;
	cb.xCopyDest.y = DstY;
	cb.xCopySrcSize.x = desc_src.Width >> SrcMIP;
	cb.xCopySrcSize.y = desc_src.Height >> SrcMIP;
	cb.xCopySrcMIP = SrcMIP;
	cb.xCopyBorderExpandStyle = (uint)borderExpand;
	device->UpdateBuffer(&constantBuffers[CBTYPE_COPYTEXTURE], &cb, cmd);

	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_COPYTEXTURE], CB_GETBINDSLOT(CopyTextureCB), cmd);

	device->BindResource(CS, &src, TEXSLOT_ONDEMAND0, cmd);

	if (DstMIP > 0)
	{
		assert(desc_dst.MipLevels > DstMIP);
		device->BindUAV(CS, &dst, 0, cmd, DstMIP);
	}
	else
	{
		device->BindUAV(CS, &dst, 0, cmd);
	}

	device->Dispatch((cb.xCopySrcSize.x + 7) / 8, (cb.xCopySrcSize.y + 7) / 8, 1, cmd);

	device->UnbindUAVs(0, 1, cmd);

	device->EventEnd(cmd);
}


void BuildSceneBVH(CommandList cmd)
{
	const Scene& scene = GetScene();

	sceneBVH.Build(scene, cmd);
}

void RayBuffers::Create(uint32_t newRayCapacity)
{
	rayCapacity = newRayCapacity;

	GPUBufferDesc desc;

	desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.Format = FORMAT_UNKNOWN;
	desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.Usage = USAGE_DEFAULT;

	desc.StructureByteStride = sizeof(uint);
	desc.ByteWidth = desc.StructureByteStride * rayCapacity;
	device->CreateBuffer(&desc, nullptr, &rayIndexBuffer[0]);
	device->SetName(&rayIndexBuffer[0], "rayIndexBuffer[0]");
	device->CreateBuffer(&desc, nullptr, &rayIndexBuffer[1]);
	device->SetName(&rayIndexBuffer[1], "rayIndexBuffer[1]");

#ifdef RAYTRACING_SORT_GLOBAL
	desc.StructureByteStride = sizeof(float); // sorting needs float now
	desc.ByteWidth = desc.StructureByteStride * rayCapacity;
	device->CreateBuffer(&desc, nullptr, &raySortBuffer);
	device->SetName(&raySortBuffer, "raySortBuffer");
#endif // RAYTRACING_SORT_GLOBAL

	desc.StructureByteStride = sizeof(RaytracingStoredRay);
	desc.ByteWidth = desc.StructureByteStride * rayCapacity;
	device->CreateBuffer(&desc, nullptr, &rayBuffer[0]);
	device->SetName(&rayBuffer[0], "rayBuffer[0]");
	device->CreateBuffer(&desc, nullptr, &rayBuffer[1]);
	device->SetName(&rayBuffer[1], "rayBuffer[1]");

	desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	desc.StructureByteStride = sizeof(uint);
	desc.ByteWidth = desc.StructureByteStride;
	device->CreateBuffer(&desc, nullptr, &rayCountBuffer[0]);
	device->SetName(&rayCountBuffer[0], "rayCountBuffer[0]");
	device->CreateBuffer(&desc, nullptr, &rayCountBuffer[1]);
	device->SetName(&rayCountBuffer[1], "rayCountBuffer[1]");
}

RayBuffers* GenerateScreenRayBuffers(const CameraComponent& camera, CommandList cmd)
{
	uint _width = GetInternalResolution().x;
	uint _height = GetInternalResolution().y;
	static uint RayCountPrev = 0;
	const uint _raycount = _width * _height;

	static RayBuffers screenRayBuffers;

	if (RayCountPrev != _raycount)
	{
		RayCountPrev = _raycount;
		screenRayBuffers.Create(_raycount);
	}


	device->EventBegin("Launch Screen Rays", cmd);
	{
		device->BindComputeShader(&shaders[CSTYPE_RAYTRACE_LAUNCH], cmd);

		const XMFLOAT4& halton = wiMath::GetHaltonSequence((int)device->GetFrameCount());
		RaytracingCB cb;
		cb.xTracePixelOffset = XMFLOAT2(halton.x, halton.y);
		cb.xTraceResolution.x = _width;
		cb.xTraceResolution.y = _height;
		cb.xTraceResolution_rcp.x = 1.0f / cb.xTraceResolution.x;
		cb.xTraceResolution_rcp.y = 1.0f / cb.xTraceResolution.y;
		device->UpdateBuffer(&constantBuffers[CBTYPE_RAYTRACE], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(RaytracingCB), cmd);

		GPUResource* uavs[] = {
			&screenRayBuffers.rayIndexBuffer[0],
			&screenRayBuffers.rayBuffer[0],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(_width + RAYTRACING_LAUNCH_BLOCKSIZE - 1) / RAYTRACING_LAUNCH_BLOCKSIZE,
			(_height + RAYTRACING_LAUNCH_BLOCKSIZE - 1) / RAYTRACING_LAUNCH_BLOCKSIZE,
			1, 
			cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);

		// write initial ray count:
		device->UpdateBuffer(&screenRayBuffers.rayCountBuffer[0], &screenRayBuffers.rayCapacity, cmd);
	}
	device->EventEnd(cmd);

	return &screenRayBuffers;
}
void RayTraceScene(
	const RayBuffers* rayBuffers, 
	const Texture* result, 
	int accumulation_sample, 
	CommandList cmd
)
{
	const Scene& scene = GetScene();

	device->EventBegin("RayTraceScene", cmd);


	static GPUBuffer indirectBuffer; // GPU job kicks
	if (!indirectBuffer.IsValid())
	{
		GPUBufferDesc desc;

		desc.BindFlags = BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(IndirectDispatchArgs) * 2;
		desc.ByteWidth = desc.StructureByteStride;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_INDIRECT_ARGS | RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		desc.Usage = USAGE_DEFAULT;
		device->CreateBuffer(&desc, nullptr, &indirectBuffer);
		device->SetName(&indirectBuffer, "raytrace_indirectBuffer");
	}

	const TextureDesc& result_desc = result->GetDesc();

	auto range = wiProfiler::BeginRangeGPU("RayTrace - ALL", cmd);

	// Set up tracing resources:
	sceneBVH.Bind(CS, cmd);

	if (scene.weather.skyMap != nullptr)
	{
		device->BindResource(CS, scene.weather.skyMap->texture, TEXSLOT_GLOBALENVMAP, cmd);
	}
	else
	{
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);
	}

	const XMFLOAT4& halton = wiMath::GetHaltonSequence((int)device->GetFrameCount());
	RaytracingCB cb;
	cb.xTracePixelOffset = XMFLOAT2(halton.x, halton.y);
	cb.xTraceAccumulationFactor = 1.0f / ((float)accumulation_sample + 1.0f);
	cb.xTraceResolution.x = result_desc.Width;
	cb.xTraceResolution.y = result_desc.Height;
	cb.xTraceResolution_rcp.x = 1.0f / cb.xTraceResolution.x;
	cb.xTraceResolution_rcp.y = 1.0f / cb.xTraceResolution.y;

	for (uint32_t bounce = 0; bounce < raytraceBounceCount + 1; ++bounce) // first contact + indirect bounces
	{
		const uint32_t __readBufferID = bounce % 2;
		const uint32_t __writeBufferID = (bounce + 1) % 2;

		cb.xTraceUserData.x = (bounce == 1 && accumulation_sample == 0) ? 1 : 0; // pre-clear result texture?
		cb.xTraceUserData.y = bounce == raytraceBounceCount ? 1 : 0; // accumulation step?
		cb.xTraceRandomSeed = renderTime * (float)(bounce + 1);
		device->UpdateBuffer(&constantBuffers[CBTYPE_RAYTRACE], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(RaytracingCB), cmd);


		// 1.) Kick off raytracing jobs for this bounce
		device->EventBegin("Kick Raytrace Jobs", cmd);
		{
			// Prepare indirect dispatch based on counter buffer value:
			device->BindComputeShader(&shaders[CSTYPE_RAYTRACE_KICKJOBS], cmd);

			const GPUResource* res[] = {
				&rayBuffers->rayCountBuffer[__readBufferID],
			};
			device->BindResources(CS, res, TEXSLOT_UNIQUE0, arraysize(res), cmd);
			const GPUResource* uavs[] = {
				&rayBuffers->rayCountBuffer[__writeBufferID],
				&indirectBuffer,
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

			device->Dispatch(1, 1, 1, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

			device->UnbindUAVs(0, arraysize(uavs), cmd);

			GPUBarrier barrier_uav_indirect = GPUBarrier::Buffer(&indirectBuffer, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_INDIRECT_ARGUMENT);
			device->Barrier(&barrier_uav_indirect, 1, cmd);
		}
		device->EventEnd(cmd);

		// Sorting and shading only after first bounce:
		if (bounce > 0)
		{
			// Sort rays to achieve more coherency:
			{
				device->EventBegin("Ray Sorting", cmd);

#ifdef RAYTRACING_SORT_GLOBAL
				wiGPUSortLib::Sort(rayBuffers->rayCapacity, rayBuffers->raySortBuffer, rayBuffers->rayCountBuffer[__readBufferID], 0, rayBuffers->rayIndexBuffer[__readBufferID], cmd);
#else
				device->BindComputeShader(&shaders[CSTYPE_RAYTRACE_TILESORT], cmd);

				const GPUResource* res[] = {
					&rayBuffers->rayCountBuffer[__readBufferID],
					&rayBuffers->rayBuffer[__readBufferID],
				};
				device->BindResources(CS, res, TEXSLOT_ONDEMAND7, arraysize(res), cmd);
				const GPUResource* uavs[] = {
					&rayBuffers->rayIndexBuffer[__readBufferID],
				};
				device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

				device->DispatchIndirect(&indirectBuffer, RAYTRACE_INDIRECT_OFFSET_TILESORT, cmd);

				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);

				device->UnbindUAVs(0, arraysize(uavs), cmd);
#endif // RAYTRACING_SORT_GLOBAL

				device->EventEnd(cmd);
			}

			// Shade
			{
				device->EventBegin("Shading Rays", cmd);

				wiProfiler::range_id range;
				if (bounce == 1)
				{
					range = wiProfiler::BeginRangeGPU("RayTrace - Shade", cmd);
				}

				device->BindComputeShader(&shaders[CSTYPE_RAYTRACE_SHADE], cmd);

				const GPUResource* res[] = {
					&rayBuffers->rayCountBuffer[__readBufferID],
					&rayBuffers->rayIndexBuffer[__readBufferID],
				};
				device->BindResources(CS, res, TEXSLOT_ONDEMAND7, arraysize(res), cmd);
				const GPUResource* uavs[] = {
					&rayBuffers->rayBuffer[__readBufferID],
					result,
				};
				device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

				device->DispatchIndirect(&indirectBuffer, RAYTRACE_INDIRECT_OFFSET_TRACE, cmd);

				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);

				device->UnbindUAVs(0, arraysize(uavs), cmd);

				if (bounce == 1)
				{
					wiProfiler::EndRange(range); // RayTrace - Shade
				}
				device->EventEnd(cmd);
			}
		}

		// Compute Closest hits (skip after last bounce)
		if(bounce < raytraceBounceCount)
		{
			device->EventBegin("Primary Rays", cmd);

			wiProfiler::range_id range;
			if (bounce == 0)
			{
				range = wiProfiler::BeginRangeGPU("RayTrace - First Contact", cmd);
			}
			else if (bounce == 1)
			{
				range = wiProfiler::BeginRangeGPU("RayTrace - First Bounce", cmd);
			}

			device->BindComputeShader(&shaders[CSTYPE_RAYTRACE_CLOSESTHIT], cmd);

			const GPUResource* res[] = {
				&rayBuffers->rayCountBuffer[__readBufferID],
				&rayBuffers->rayIndexBuffer[__readBufferID],
				&rayBuffers->rayBuffer[__readBufferID],
			};
			device->BindResources(CS, res, TEXSLOT_ONDEMAND7, arraysize(res), cmd);
			const GPUResource* uavs[] = {
				&rayBuffers->rayCountBuffer[__writeBufferID],
				&rayBuffers->rayBuffer[__writeBufferID],
#ifdef RAYTRACING_SORT_GLOBAL
				&rayBuffers->rayIndexBuffer[__writeBufferID],
				&rayBuffers->raySortBuffer,
#endif // RAYTRACING_SORT_GLOBAL
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

			device->DispatchIndirect(&indirectBuffer, RAYTRACE_INDIRECT_OFFSET_TRACE, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

			device->UnbindUAVs(0, arraysize(uavs), cmd);

			if (bounce == 0 || bounce == 1)
			{
				wiProfiler::EndRange(range); // RayTrace - First Contact/Bounce
			}
			device->EventEnd(cmd);
		}
	}

	wiProfiler::EndRange(range); // RayTrace - ALL




	device->EventEnd(cmd); // RayTraceScene
}
void RayTraceSceneBVH(CommandList cmd)
{
	device->EventBegin("RayTraceSceneBVH", cmd);
	device->BindPipelineState(&PSO_debug[DEBUGRENDERING_RAYTRACE_BVH], cmd);
	sceneBVH.Bind(PS, cmd);
	device->Draw(3, 0, cmd);
	device->EventEnd(cmd);
}


bool repackAtlas_Decal = false;
void ManageDecalAtlas()
{
	repackAtlas_Decal = false;

	Scene& scene = GetScene();

	using namespace wiRectPacker;

	// Gather all decal textures:
	for (size_t i = 0; i < scene.decals.GetCount(); ++i)
	{
		const DecalComponent& decal = scene.decals[i];

		if (decal.texture != nullptr)
		{
			if (packedDecals.find(decal.texture) == packedDecals.end())
			{
				// we need to pack this decal texture into the atlas
				rect_xywh newRect = rect_xywh(0, 0, decal.texture->texture->GetDesc().Width + atlasClampBorder * 2, decal.texture->texture->GetDesc().Height + atlasClampBorder * 2);
				packedDecals[decal.texture] = newRect;

				repackAtlas_Decal = true;
			}
		}
	}

	// Update atlas texture if it is invalidated:
	if (repackAtlas_Decal)
	{
		vector<rect_xywh*> out_rects(packedDecals.size());
		int i = 0;
		for (auto& it : packedDecals)
		{
			out_rects[i] = &it.second;
			i++;
		}

		std::vector<bin> bins;
		if (pack(out_rects.data(), (int)packedDecals.size(), 16384, bins))
		{
			assert(bins.size() == 1 && "The regions won't fit into the texture!");

			TextureDesc desc;
			desc.Width = (uint32_t)bins[0].size.w;
			desc.Height = (uint32_t)bins[0].size.h;
			desc.MipLevels = 0;
			desc.ArraySize = 1;
			desc.Format = FORMAT_R8G8B8A8_UNORM;
			desc.SampleCount = 1;
			desc.Usage = USAGE_DEFAULT;
			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			device->CreateTexture(&desc, nullptr, &decalAtlas);
			device->SetName(&decalAtlas, "decalAtlas");

			for (uint32_t i = 0; i < decalAtlas.GetDesc().MipLevels; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&decalAtlas, UAV, 0, 1, i, 1);
				assert(subresource_index == i);
			}
		}
		else
		{
			wiBackLog::post("Decal atlas packing failed!");
		}
	}

	// Assign atlas buckets to decals:
	for (size_t i = 0; i < scene.decals.GetCount(); ++i)
	{
		DecalComponent& decal = scene.decals[i];

		if (decal.texture != nullptr)
		{
			const TextureDesc& desc = decalAtlas.GetDesc();

			rect_xywh rect = packedDecals[decal.texture];

			// eliminate border expansion:
			rect.x += atlasClampBorder;
			rect.y += atlasClampBorder;
			rect.w -= atlasClampBorder * 2;
			rect.h -= atlasClampBorder * 2;

			decal.atlasMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height, (float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);
		}
		else
		{
			decal.atlasMulAdd = XMFLOAT4(0, 0, 0, 0);
		}

	}
}
void RefreshDecalAtlas(CommandList cmd)
{
	using namespace wiRectPacker;

	const Scene& scene = GetScene();

	if (repackAtlas_Decal)
	{
		for (uint32_t mip = 0; mip < decalAtlas.GetDesc().MipLevels; ++mip)
		{
			for (auto& it : packedDecals)
			{
				if (mip < it.first->texture->GetDesc().MipLevels)
				{
					CopyTexture2D(decalAtlas, mip, (it.second.x >> mip) + atlasClampBorder, (it.second.y >> mip) + atlasClampBorder, *it.first->texture, mip, cmd, BORDEREXPAND_CLAMP);
				}
			}
		}
	}
}

bool repackAtlas_Lightmap = false;
vector<uint32_t> lightmapsToRefresh;
void ManageLightmapAtlas()
{
	lightmapsToRefresh.clear(); 
	repackAtlas_Lightmap = false;

	Scene& scene = GetScene();

	using namespace wiRectPacker;

	// Gather all object lightmap textures:
	for (size_t objectIndex = 0; objectIndex < scene.objects.GetCount(); ++objectIndex)
	{
		ObjectComponent& object = scene.objects[objectIndex];
		bool refresh = false;

		if (object.lightmap.IsValid() && object.lightmapWidth == 0)
		{
			// If we get here, it means that the lightmap GPU texture contains the rendered lightmap, but the CPU-side data was erased.
			//	In this case, we delete the GPU side lightmap data from the object and the atlas too.
			packedLightmaps.erase(object.lightmap.internal_state.get());
			object.lightmap = Texture();
			repackAtlas_Lightmap = true;
			refresh = false;
		}

		if (object.IsLightmapRenderRequested())
		{
			refresh = true;

			if (object.lightmapIterationCount == 0)
			{
				if (object.lightmap.IsValid())
				{
					packedLightmaps.erase(object.lightmap.internal_state.get());
				}

				{
					// Unfortunately, fp128 format only correctly downloads from GPU if it is pow2 size:
					object.lightmapWidth = wiMath::GetNextPowerOfTwo(object.lightmapWidth + 1) / 2;
					object.lightmapHeight = wiMath::GetNextPowerOfTwo(object.lightmapHeight + 1) / 2;
				}

				TextureDesc desc;
				desc.Width = object.lightmapWidth;
				desc.Height = object.lightmapHeight;
				desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
				// Note: we need the full precision format to achieve correct accumulative blending! 
				//	But the global atlas will have less precision for good bandwidth for sampling
				desc.Format = FORMAT_R32G32B32A32_FLOAT;

				object.lightmap = Texture();
				device->CreateTexture(&desc, nullptr, &object.lightmap);
				device->SetName(&object.lightmap, "objectLightmap");

				RenderPassDesc renderpassdesc;

				renderpassdesc.attachments.push_back(RenderPassAttachment::RenderTarget(&object.lightmap, RenderPassAttachment::LOADOP_CLEAR));
				
				object.renderpass_lightmap_clear = RenderPass();
				device->CreateRenderPass(&renderpassdesc, &object.renderpass_lightmap_clear);

				renderpassdesc.attachments.back().loadop = RenderPassAttachment::LOADOP_LOAD;
				object.renderpass_lightmap_accumulate = RenderPass();
				device->CreateRenderPass(&renderpassdesc, &object.renderpass_lightmap_accumulate);
			}
			object.lightmapIterationCount++;
		}

		if (!object.lightmapTextureData.empty() && !object.lightmap.IsValid())
		{
			refresh = true;
			// Create a GPU-side per object lighmap if there is none yet, so that copying into atlas can be done efficiently:
			object.lightmap = Texture();
			wiTextureHelper::CreateTexture(object.lightmap, object.lightmapTextureData.data(), object.lightmapWidth, object.lightmapHeight, object.GetLightmapFormat());
		}

		if (object.lightmap.IsValid())
		{
			if (packedLightmaps.find(object.lightmap.internal_state.get()) == packedLightmaps.end())
			{
				// we need to pack this lightmap texture into the atlas
				rect_xywh newRect = rect_xywh(0, 0, object.lightmap.GetDesc().Width + atlasClampBorder * 2, object.lightmap.GetDesc().Height + atlasClampBorder * 2);
				packedLightmaps[object.lightmap.internal_state.get()] = newRect;

				repackAtlas_Lightmap = true;
				refresh = true;
			}
		}

		if (refresh)
		{
			// Push a new object whose lightmap should be copied over to the atlas:
			lightmapsToRefresh.push_back((uint32_t)objectIndex);
		}

	}

	// Update atlas texture if it is invalidated:
	using namespace wiRectPacker;
	if (repackAtlas_Lightmap && !packedLightmaps.empty())
	{
		vector<rect_xywh*> out_rects(packedLightmaps.size());
		int i = 0;
		for (auto& it : packedLightmaps)
		{
			out_rects[i] = &it.second;
			i++;
		}

		std::vector<bin> bins;
		if (pack(out_rects.data(), (int)packedLightmaps.size(), 16384, bins))
		{
			assert(bins.size() == 1 && "The regions won't fit into the texture!");

			TextureDesc desc;
			desc.Width = (uint32_t)bins[0].size.w;
			desc.Height = (uint32_t)bins[0].size.h;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = FORMAT_R11G11B10_FLOAT;
			desc.SampleCount = 1;
			desc.Usage = USAGE_DEFAULT;
			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			device->CreateTexture(&desc, nullptr, &globalLightmap);
			device->SetName(&globalLightmap, "globalLightmap");
		}
		else
		{
			wiBackLog::post("Global Lightmap atlas packing failed!");
		}
	}

	// Assign atlas buckets to objects:
	if (!packedLightmaps.empty())
	{
		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			ObjectComponent& object = scene.objects[i];

			if (object.lightmap.IsValid() && packedLightmaps.count(object.lightmap.internal_state.get()) > 0)
			{
				const TextureDesc& desc = globalLightmap.GetDesc();

				rect_xywh rect = packedLightmaps.at(object.lightmap.internal_state.get());

				// eliminate border expansion:
				rect.x += atlasClampBorder;
				rect.y += atlasClampBorder;
				rect.w -= atlasClampBorder * 2;
				rect.h -= atlasClampBorder * 2;

				object.globalLightMapMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height, (float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);
			}
			else
			{
				object.globalLightMapMulAdd = XMFLOAT4(0, 0, 0, 0);
			}
		}
	}
}
void RenderObjectLightMap(const ObjectComponent& object, CommandList cmd)
{
	const Scene& scene = GetScene();

	device->EventBegin("RenderObjectLightMap", cmd);

	const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);
	assert(!mesh.vertex_atlas.empty());
	assert(mesh.vertexBuffer_ATL.IsValid());

	const TextureDesc& desc = object.lightmap.GetDesc();

	const uint32_t lightmapIterationCount = std::max(1u, object.lightmapIterationCount) - 1; // ManageLightMapAtlas incremented before refresh

	if (lightmapIterationCount == 0)
	{
		device->RenderPassBegin(&object.renderpass_lightmap_clear, cmd);
	}
	else
	{
		device->RenderPassBegin(&object.renderpass_lightmap_accumulate, cmd);
	}

	Viewport vp;
	vp.Width = (float)desc.Width;
	vp.Height = (float)desc.Height;
	device->BindViewports(1, &vp, cmd);

	const TransformComponent& transform = scene.transforms[object.transform_index];

	// Note: using InstancePrev, because we just need the matrix, nothing else here...
	GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(InstancePrev), cmd);
	volatile InstancePrev* instance = (volatile InstancePrev*)mem.data;
	instance->Create(transform.world);

	const GPUBuffer* vbs[] = {
		&mesh.vertexBuffer_POS,
		&mesh.vertexBuffer_ATL,
		mem.buffer,
	};
	uint32_t strides[] = {
		sizeof(MeshComponent::Vertex_POS),
		sizeof(MeshComponent::Vertex_TEX),
		sizeof(InstancePrev),
	};
	uint32_t offsets[] = {
		0,
		0,
		mem.offset,
	};
	device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
	device->BindIndexBuffer(&mesh.indexBuffer, mesh.GetIndexFormat(), 0, cmd);

	RaytracingCB cb;
	cb.xTraceResolution.x = desc.Width;
	cb.xTraceResolution.y = desc.Height;
	cb.xTraceResolution_rcp.x = 1.0f / cb.xTraceResolution.x;
	cb.xTraceResolution_rcp.y = 1.0f / cb.xTraceResolution.y;
	XMFLOAT4 halton = wiMath::GetHaltonSequence(lightmapIterationCount); // for jittering the rasterization (good for eliminating atlas border artifacts)
	cb.xTracePixelOffset.x = (halton.x * 2 - 1) * cb.xTraceResolution_rcp.x;
	cb.xTracePixelOffset.y = (halton.y * 2 - 1) * cb.xTraceResolution_rcp.y;
	cb.xTracePixelOffset.x *= 1.4f;	// boost the jitter by a bit
	cb.xTracePixelOffset.y *= 1.4f;	// boost the jitter by a bit
	cb.xTraceRandomSeed = (float)lightmapIterationCount + 1.2345f; // random seed
	cb.xTraceAccumulationFactor = 1.0f / (lightmapIterationCount + 1.0f); // accumulation factor (alpha)
	cb.xTraceUserData.x = raytraceBounceCount;
	device->UpdateBuffer(&constantBuffers[CBTYPE_RAYTRACE], &cb, cmd);
	device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(RaytracingCB), cmd);
	device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(RaytracingCB), cmd);

	device->BindPipelineState(&PSO_renderlightmap, cmd);

	if (scene.weather.skyMap != nullptr)
	{
		device->BindResource(PS, scene.weather.skyMap->texture, TEXSLOT_GLOBALENVMAP, cmd);
	}
	else
	{
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);
	}

	device->DrawIndexedInstanced((uint32_t)mesh.indices.size(), 1, 0, 0, 0, cmd);

	device->RenderPassEnd(cmd);

	device->EventEnd(cmd);
}
void RefreshLightmapAtlas(CommandList cmd)
{
	const Scene& scene = GetScene();

	if (!lightmapsToRefresh.empty())
	{
		auto range = wiProfiler::BeginRangeGPU("Lightmap Processing", cmd);

		// Update GPU scene and BVH data:
		{
			if (scene_bvh_invalid)
			{
				scene_bvh_invalid = false;
				BuildSceneBVH(cmd);
			}
			sceneBVH.Bind(PS, cmd);
		}

		// Render lightmaps for each object:
		for (uint32_t objectIndex : lightmapsToRefresh)
		{
			const ObjectComponent& object = scene.objects[objectIndex];

			if (object.IsLightmapRenderRequested())
			{
				RenderObjectLightMap(object, cmd);
			}
		}

		if (!packedLightmaps.empty())
		{
			device->EventBegin("PackGlobalLightmap", cmd);
			if (repackAtlas_Lightmap)
			{
				// If atlas was repacked, we copy every object lightmap:
				for (size_t i = 0; i < scene.objects.GetCount(); ++i)
				{
					const ObjectComponent& object = scene.objects[i];
					if (object.lightmap.IsValid())
					{
						const auto& rec = packedLightmaps.at(object.lightmap.internal_state.get());
						CopyTexture2D(globalLightmap, 0, rec.x + atlasClampBorder, rec.y + atlasClampBorder, object.lightmap, 0, cmd);
					}
				}
			}
			else
			{
				// If atlas was not repacked, we only copy refreshed object lightmaps:
				for (uint32_t objectIndex : lightmapsToRefresh)
				{
					const ObjectComponent& object = scene.objects[objectIndex];
					const auto& rec = packedLightmaps.at(object.lightmap.internal_state.get());
					CopyTexture2D(globalLightmap, 0, rec.x + atlasClampBorder, rec.y + atlasClampBorder, object.lightmap, 0, cmd);
				}
			}
			device->EventEnd(cmd);

		}

		wiProfiler::EndRange(range);
	}
}

const Texture* GetGlobalLightmap()
{
	return &globalLightmap;
}

void BindCommonResources(CommandList cmd)
{
	ResetAlphaRef(cmd);

	for (int i = 0; i < SHADERSTAGE_COUNT; ++i)
	{
		SHADERSTAGE stage = (SHADERSTAGE)i;

		for (int i = 0; i < SSLOT_COUNT; ++i)
		{
			device->BindSampler(stage, &samplers[i], i, cmd);
		}

		BindConstantBuffers(stage, cmd);
	}

	// Bind the GPU entity array for all shaders that need it here:
	GPUResource* resources[] = {
		&resourceBuffers[RBTYPE_ENTITYARRAY],
		&resourceBuffers[RBTYPE_MATRIXARRAY],
	};
	device->BindResources(VS, resources, SBSLOT_ENTITYARRAY, arraysize(resources), cmd);
	device->BindResources(PS, resources, SBSLOT_ENTITYARRAY, arraysize(resources), cmd);
	device->BindResources(CS, resources, SBSLOT_ENTITYARRAY, arraysize(resources), cmd);

	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
	{
		const Scene& scene = GetScene();
		device->BindResource(PS, &scene.TLAS, TEXSLOT_ACCELERATION_STRUCTURE, cmd);
		device->BindResource(CS, &scene.TLAS, TEXSLOT_ACCELERATION_STRUCTURE, cmd);
	}
}

void UpdateFrameCB(CommandList cmd)
{
	const Scene& scene = GetScene();

	FrameCB cb;

	cb.g_xFrame_ConstantOne = 1;
	cb.g_xFrame_ScreenWidthHeight = float2((float)device->GetScreenWidth(), (float)device->GetScreenHeight());
	cb.g_xFrame_ScreenWidthHeight_rcp = float2(1.0f / cb.g_xFrame_ScreenWidthHeight.x, 1.0f / cb.g_xFrame_ScreenWidthHeight.y);
	cb.g_xFrame_InternalResolution = float2((float)GetInternalResolution().x, (float)GetInternalResolution().y);
	cb.g_xFrame_InternalResolution_rcp = float2(1.0f / cb.g_xFrame_InternalResolution.x, 1.0f / cb.g_xFrame_InternalResolution.y);
	cb.g_xFrame_Gamma = GetGamma();
	cb.g_xFrame_SunColor = scene.weather.sunColor;
	cb.g_xFrame_SunDirection = scene.weather.sunDirection;
	cb.g_xFrame_SunEnergy = scene.weather.sunEnergy;
	cb.g_xFrame_ShadowCascadeCount = CASCADE_COUNT;
	cb.g_xFrame_Ambient = scene.weather.ambient;
	cb.g_xFrame_Cloudiness = scene.weather.cloudiness;
	cb.g_xFrame_CloudScale = scene.weather.cloudScale;
	cb.g_xFrame_CloudSpeed = scene.weather.cloudSpeed;
	cb.g_xFrame_Fog = float3(scene.weather.fogStart, scene.weather.fogEnd, scene.weather.fogHeight);
	cb.g_xFrame_Horizon = scene.weather.horizon;
	cb.g_xFrame_Zenith = scene.weather.zenith;
	cb.g_xFrame_VoxelRadianceMaxDistance = voxelSceneData.maxDistance;
	cb.g_xFrame_VoxelRadianceDataSize = voxelSceneData.voxelsize;
	cb.g_xFrame_VoxelRadianceDataSize_rcp = 1.0f / (float)cb.g_xFrame_VoxelRadianceDataSize;
	cb.g_xFrame_VoxelRadianceDataRes = GetVoxelRadianceEnabled() ? (uint)voxelSceneData.res : 0;
	cb.g_xFrame_VoxelRadianceDataRes_rcp = 1.0f / (float)cb.g_xFrame_VoxelRadianceDataRes;
	cb.g_xFrame_VoxelRadianceDataMIPs = voxelSceneData.mips;
	cb.g_xFrame_VoxelRadianceNumCones = std::max(std::min(voxelSceneData.numCones, 16u), 1u);
	cb.g_xFrame_VoxelRadianceNumCones_rcp = 1.0f / (float)cb.g_xFrame_VoxelRadianceNumCones;
	cb.g_xFrame_VoxelRadianceRayStepSize = voxelSceneData.rayStepSize;
	cb.g_xFrame_VoxelRadianceDataCenter = voxelSceneData.center;
	cb.g_xFrame_EntityCullingTileCount = GetEntityCullingTileCount();
	cb.g_xFrame_GlobalEnvProbeIndex = -1;
	cb.g_xFrame_EnvProbeMipCount = 0;
	cb.g_xFrame_EnvProbeMipCount_rcp = 1.0f;
	if (scene.probes.GetCount() > 0)
	{
		cb.g_xFrame_GlobalEnvProbeIndex = 0; // for now, the global envprobe will be the first probe in the array. Easy change later on if required...
	}
	if (textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY].IsValid())
	{
		cb.g_xFrame_EnvProbeMipCount = textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY].GetDesc().MipLevels;
		cb.g_xFrame_EnvProbeMipCount_rcp = 1.0f / (float)cb.g_xFrame_EnvProbeMipCount;
	}

	cb.g_xFrame_Time = renderTime;
	cb.g_xFrame_TimePrev = renderTime_Prev;
	cb.g_xFrame_DeltaTime = deltaTime;
	cb.g_xFrame_LightArrayOffset = entityArrayOffset_Lights;
	cb.g_xFrame_LightArrayCount = entityArrayCount_Lights;
	cb.g_xFrame_DecalArrayOffset = entityArrayOffset_Decals;
	cb.g_xFrame_DecalArrayCount = entityArrayCount_Decals;
	cb.g_xFrame_ForceFieldArrayOffset = entityArrayOffset_ForceFields;
	cb.g_xFrame_ForceFieldArrayCount = entityArrayCount_ForceFields;
	cb.g_xFrame_EnvProbeArrayOffset = entityArrayOffset_EnvProbes;
	cb.g_xFrame_EnvProbeArrayCount = entityArrayCount_EnvProbes;
	cb.g_xFrame_WindSpeed = scene.weather.windSpeed;
	cb.g_xFrame_WindRandomness = scene.weather.windRandomness;
	cb.g_xFrame_WindWaveSize = scene.weather.windWaveSize;
	cb.g_xFrame_WindDirection = scene.weather.windDirection;
	cb.g_xFrame_StaticSkyGamma = 0.0f;
	if (scene.weather.skyMap != nullptr)
	{
		bool hdr = !device->IsFormatUnorm(scene.weather.skyMap->texture->GetDesc().Format);
		cb.g_xFrame_StaticSkyGamma = hdr ? 1.0f : cb.g_xFrame_Gamma;
	}
	cb.g_xFrame_FrameCount = (uint)device->GetFrameCount();
	cb.g_xFrame_TemporalAASampleRotation = 0;
	if (GetTemporalAAEnabled())
	{
		uint id = cb.g_xFrame_FrameCount % 4;
		uint x = 0;
		uint y = 0;
		switch (id)
		{
		case 1:
			x = 1;
			break;
		case 2:
			y = 1;
			break;
		case 3:
			x = 1;
			y = 1;
			break;
		default:
			break;
		}
		cb.g_xFrame_TemporalAASampleRotation = (x & 0x000000FF) | ((y & 0x000000FF) << 8);
	}
	cb.g_xFrame_TemporalAAJitter = temporalAAJitter;
	cb.g_xFrame_TemporalAAJitterPrev = temporalAAJitterPrev;

	const auto& prevCam = GetPrevCamera();
	const auto& reflCam = GetRefCamera();
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_PrevV, prevCam.GetView());
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_PrevP, prevCam.GetProjection());
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_PrevVP, prevCam.GetViewProjection());
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_PrevInvVP, prevCam.GetInvViewProjection());
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_ReflVP, reflCam.GetViewProjection());

	cb.g_xFrame_WorldBoundsMin = scene.bounds.getMin();
	cb.g_xFrame_WorldBoundsMax = scene.bounds.getMax();
	cb.g_xFrame_WorldBoundsExtents.x = abs(cb.g_xFrame_WorldBoundsMax.x - cb.g_xFrame_WorldBoundsMin.x);
	cb.g_xFrame_WorldBoundsExtents.y = abs(cb.g_xFrame_WorldBoundsMax.y - cb.g_xFrame_WorldBoundsMin.y);
	cb.g_xFrame_WorldBoundsExtents.z = abs(cb.g_xFrame_WorldBoundsMax.z - cb.g_xFrame_WorldBoundsMin.z);
	cb.g_xFrame_WorldBoundsExtents_rcp.x = 1.0f / cb.g_xFrame_WorldBoundsExtents.x;
	cb.g_xFrame_WorldBoundsExtents_rcp.y = 1.0f / cb.g_xFrame_WorldBoundsExtents.y;
	cb.g_xFrame_WorldBoundsExtents_rcp.z = 1.0f / cb.g_xFrame_WorldBoundsExtents.z;

	cb.g_xFrame_Options = 0;
	if (GetTemporalAAEnabled())
	{
		cb.g_xFrame_Options |= OPTION_BIT_TEMPORALAA_ENABLED;
	}
	if (GetTransparentShadowsEnabled())
	{
		cb.g_xFrame_Options |= OPTION_BIT_TRANSPARENTSHADOWS_ENABLED;
	}
	if (GetVoxelRadianceEnabled())
	{
		cb.g_xFrame_Options |= OPTION_BIT_VOXELGI_ENABLED;
	}
	if (GetVoxelRadianceReflectionsEnabled())
	{
		cb.g_xFrame_Options |= OPTION_BIT_VOXELGI_REFLECTIONS_ENABLED;
	}
	if (voxelSceneData.centerChangedThisFrame)
	{
		cb.g_xFrame_Options |= OPTION_BIT_VOXELGI_RETARGETTED;
	}
	if (scene.weather.IsSimpleSky())
	{
		cb.g_xFrame_Options |= OPTION_BIT_SIMPLE_SKY;
	}
	if (scene.weather.IsRealisticSky())
	{
		cb.g_xFrame_Options |= OPTION_BIT_REALISTIC_SKY;
	}
	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING) && GetRaytracedShadowsEnabled())
	{
		cb.g_xFrame_Options |= OPTION_BIT_RAYTRACED_SHADOWS;
	}

	device->UpdateBuffer(&constantBuffers[CBTYPE_FRAME], &cb, cmd);
}
void UpdateCameraCB(const CameraComponent& camera, CommandList cmd)
{
	CameraCB cb;

	XMStoreFloat4x4(&cb.g_xCamera_VP, camera.GetViewProjection());
	XMStoreFloat4x4(&cb.g_xCamera_View, camera.GetView());
	XMStoreFloat4x4(&cb.g_xCamera_Proj, camera.GetProjection());
	cb.g_xCamera_CamPos = camera.Eye;
	cb.g_xCamera_DistanceFromOrigin = XMVectorGetX(XMVector3Length(XMLoadFloat3(&cb.g_xCamera_CamPos)));
	XMStoreFloat4x4(&cb.g_xCamera_InvV, camera.GetInvView());
	XMStoreFloat4x4(&cb.g_xCamera_InvP, camera.GetInvProjection());
	XMStoreFloat4x4(&cb.g_xCamera_InvVP, camera.GetInvViewProjection());
	cb.g_xCamera_At = camera.At;
	cb.g_xCamera_Up = camera.Up;
	cb.g_xCamera_ZNearP = camera.zNearP;
	cb.g_xCamera_ZFarP = camera.zFarP;
	cb.g_xCamera_ZNearP_rcp = 1.0f / std::max(0.0001f, cb.g_xCamera_ZNearP);
	cb.g_xCamera_ZFarP_rcp = 1.0f / std::max(0.0001f, cb.g_xCamera_ZFarP);
	cb.g_xCamera_ZRange = abs(cb.g_xCamera_ZFarP - cb.g_xCamera_ZNearP);
	cb.g_xCamera_ZRange_rcp = 1.0f / std::max(0.0001f, cb.g_xCamera_ZRange);

	static_assert(arraysize(camera.frustum.planes) == arraysize(cb.g_xCamera_FrustumPlanes), "Mismatch!");
	for (int i = 0; i < arraysize(camera.frustum.planes); ++i)
	{
		cb.g_xCamera_FrustumPlanes[i] = camera.frustum.planes[i];
	}

	device->UpdateBuffer(&constantBuffers[CBTYPE_CAMERA], &cb, cmd);
}

APICB apiCB[COMMANDLIST_COUNT];
void SetClipPlane(const XMFLOAT4& clipPlane, CommandList cmd)
{
	apiCB[cmd].g_xClipPlane = clipPlane;
	device->UpdateBuffer(&constantBuffers[CBTYPE_API], &apiCB[cmd], cmd);
}
void SetAlphaRef(float alphaRef, CommandList cmd)
{
	if (alphaRef != apiCB[cmd].g_xAlphaRef)
	{
		apiCB[cmd].g_xAlphaRef = 1 - alphaRef + 1.0f / 256.0f; // 256 so that it is just about smaller than 1 unorm unit (1.0/255.0)
		device->UpdateBuffer(&constantBuffers[CBTYPE_API], &apiCB[cmd], cmd);
	}
}

const Texture* ComputeLuminance(const Texture& sourceImage, CommandList cmd)
{
	static Texture luminance_map;
	static std::vector<Texture> luminance_avg(0);
	if (!luminance_map.IsValid())
	{
		luminance_avg.clear();

		TextureDesc desc;
		desc.Width = 256;
		desc.Height = desc.Width;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = FORMAT_R32_FLOAT;
		desc.SampleCount = 1;
		desc.Usage = USAGE_DEFAULT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		device->CreateTexture(&desc, nullptr, &luminance_map);

		while (desc.Width > 1)
		{
			desc.Width = std::max(desc.Width / 16, 1u);
			desc.Height = desc.Width;

			Texture tex;
			device->CreateTexture(&desc, nullptr, &tex);

			luminance_avg.push_back(tex);
		}
	}
	if (luminance_map.IsValid())
	{
		device->EventBegin("Compute Luminance", cmd);

		// Pass 1 : Create luminance map from scene tex
		TextureDesc luminance_map_desc = luminance_map.GetDesc();
		device->BindComputeShader(&shaders[CSTYPE_LUMINANCE_PASS1], cmd);
		device->BindResource(CS, &sourceImage, TEXSLOT_ONDEMAND0, cmd);
		device->BindUAV(CS, &luminance_map, 0, cmd);
		device->Dispatch(luminance_map_desc.Width/16, luminance_map_desc.Height/16, 1, cmd);

		// Pass 2 : Reduce for average luminance until we got an 1x1 texture
		TextureDesc luminance_avg_desc;
		for (size_t i = 0; i < luminance_avg.size(); ++i)
		{
			luminance_avg_desc = luminance_avg[i].GetDesc();
			device->BindComputeShader(&shaders[CSTYPE_LUMINANCE_PASS2], cmd);

			const GPUResource* uavs[] = {
				&luminance_avg[i]
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

			if (i > 0)
			{
				device->BindResource(CS, &luminance_avg[i-1], TEXSLOT_ONDEMAND0, cmd);
			}
			else
			{
				device->BindResource(CS, &luminance_map, TEXSLOT_ONDEMAND0, cmd);
			}
			device->Dispatch(luminance_avg_desc.Width, luminance_avg_desc.Height, 1, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}


		device->UnbindUAVs(0, 1, cmd);

		device->EventEnd(cmd);

		return &luminance_avg.back();
	}

	return nullptr;
}

void ComputeShadingRateClassification(
	const Texture gbuffer[GBUFFER_COUNT],
	const Texture& lineardepth,
	const Texture& output,
	CommandList cmd
)
{
	device->EventBegin("ComputeShadingRateClassification", cmd);
	auto range = wiProfiler::BeginRangeGPU("ComputeShadingRateClassification", cmd);

	if (GetVariableRateShadingClassificationDebug())
	{
		device->BindUAV(CS, &textures[TEXTYPE_2D_DEBUGUAV], 1, cmd);
		device->BindComputeShader(&shaders[CSTYPE_SHADINGRATECLASSIFICATION_DEBUG], cmd);
	}
	else
	{
		device->BindComputeShader(&shaders[CSTYPE_SHADINGRATECLASSIFICATION], cmd);
	}

	device->BindResource(CS, &gbuffer[GBUFFER_NORMAL_VELOCITY], TEXSLOT_GBUFFER1, cmd);
	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	const TextureDesc& desc = output.GetDesc();

	ShadingRateClassificationCB cb = {}; // zero init the shading rates!
	cb.xShadingRateTileSize = device->GetVariableRateShadingTileSize();
	device->WriteShadingRateValue(SHADING_RATE_1X1, &cb.SHADING_RATE_1X1);
	device->WriteShadingRateValue(SHADING_RATE_1X2, &cb.SHADING_RATE_1X2);
	device->WriteShadingRateValue(SHADING_RATE_2X1, &cb.SHADING_RATE_2X1);
	device->WriteShadingRateValue(SHADING_RATE_2X2, &cb.SHADING_RATE_2X2);
	device->WriteShadingRateValue(SHADING_RATE_2X4, &cb.SHADING_RATE_2X4);
	device->WriteShadingRateValue(SHADING_RATE_4X2, &cb.SHADING_RATE_4X2);
	device->WriteShadingRateValue(SHADING_RATE_4X4, &cb.SHADING_RATE_4X4);
	device->UpdateBuffer(&constantBuffers[CBTYPE_SHADINGRATECLASSIFICATION], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_SHADINGRATECLASSIFICATION], CB_GETBINDSLOT(PostProcessCB), cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	// Whole threadgroup for each tile:
	device->Dispatch(desc.Width, desc.Height, 1, cmd);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
		GPUBarrier::Image(&output,IMAGE_LAYOUT_UNORDERED_ACCESS, IMAGE_LAYOUT_SHADING_RATE_SOURCE),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}

void DeferredComposition(
	const Texture gbuffer[GBUFFER_COUNT],
	const Texture& depth,
	CommandList cmd
)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("DeferredComposition", cmd);

	device->BindPipelineState(&PSO_deferredcomposition, cmd);

	device->BindResource(PS, &gbuffer[GBUFFER_ALBEDO_ROUGHNESS], TEXSLOT_GBUFFER0, cmd);
	device->BindResource(PS, &gbuffer[GBUFFER_LIGHTBUFFER_DIFFUSE], TEXSLOT_ONDEMAND0, cmd);
	device->BindResource(PS, &gbuffer[GBUFFER_LIGHTBUFFER_SPECULAR], TEXSLOT_ONDEMAND1, cmd);
	device->BindResource(PS, &depth, TEXSLOT_DEPTH, cmd);

	device->Draw(3, 0, cmd);

	device->EventEnd(cmd);
}

void Postprocess_Blur_Gaussian(
	const Texture& input,
	const Texture& temp,
	const Texture& output,
	CommandList cmd,
	int mip_src,
	int mip_dst,
	bool wide
)
{
	device->EventBegin("Postprocess_Blur_Gaussian", cmd);

	SHADERTYPE cs = CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT4;
	switch (output.GetDesc().Format)
	{
	case FORMAT_R16_UNORM:
	case FORMAT_R8_UNORM:
		cs = wide ? CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_UNORM1 : CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_UNORM1;
		break;
	case FORMAT_R16_FLOAT:
	case FORMAT_R32_FLOAT:
		cs = wide ? CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT1 : CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT1;
		break;
	case FORMAT_R16G16B16A16_UNORM:
	case FORMAT_R8G8B8A8_UNORM:
	case FORMAT_B8G8R8A8_UNORM:
	case FORMAT_R10G10B10A2_UNORM:
		cs = wide ? CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_UNORM4 : CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_UNORM4;
		break;
	case FORMAT_R11G11B10_FLOAT:
		cs = wide ? CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT3 : CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT3;
		break;
	case FORMAT_R16G16B16A16_FLOAT:
	case FORMAT_R32G32B32A32_FLOAT:
		cs = wide ? CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT4 : CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT4;
		break;
	default:
		assert(0); // implement format!
		break;
	}
	device->BindComputeShader(&shaders[cs], cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);
	
	// Horizontal:
	{
		const TextureDesc& desc = temp.GetDesc();

		PostProcessCB cb;
		cb.xPPResolution.x = desc.Width;
		cb.xPPResolution.y = desc.Height;
		if (mip_dst > 0)
		{
			cb.xPPResolution.x >>= mip_dst;
			cb.xPPResolution.y >>= mip_dst;
		}
		cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
		cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
		cb.xPPParams0.x = 1;
		cb.xPPParams0.y = 0;
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);

		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd, mip_src);
		device->BindUAV(CS, &temp, 0, cmd, mip_dst);

		device->Dispatch(
			(cb.xPPResolution.x + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT - 1) / POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT,
			cb.xPPResolution.y,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, 1, cmd);
	}

	// Vertical:
	{
		const TextureDesc& desc = output.GetDesc();

		PostProcessCB cb;
		cb.xPPResolution.x = desc.Width;
		cb.xPPResolution.y = desc.Height;
		if (mip_dst > 0)
		{
			cb.xPPResolution.x >>= mip_dst;
			cb.xPPResolution.y >>= mip_dst;
		}
		cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
		cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
		cb.xPPParams0.x = 0;
		cb.xPPParams0.y = 1;
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);

		device->BindResource(CS, &temp, TEXSLOT_ONDEMAND0, cmd, mip_dst); // <- also mip_dst because it's second pass!
		device->BindUAV(CS, &output, 0, cmd, mip_dst);

		device->Dispatch(
			cb.xPPResolution.x,
			(cb.xPPResolution.y + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT - 1) / POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, 1, cmd);
	}

	device->EventEnd(cmd);
}
void Postprocess_Blur_Bilateral(
	const Texture& input,
	const Texture& lineardepth,
	const Texture& temp,
	const Texture& output,
	CommandList cmd,
	float depth_threshold,
	int mip_src,
	int mip_dst,
	bool wide
)
{
	device->EventBegin("Postprocess_Blur_Bilateral", cmd);

	SHADERTYPE cs = CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT4;
	switch (output.GetDesc().Format)
	{
	case FORMAT_R16_UNORM:
	case FORMAT_R8_UNORM:
		cs = wide ? CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_UNORM1 : CSTYPE_POSTPROCESS_BLUR_BILATERAL_UNORM1;
		break;
	case FORMAT_R16_FLOAT:
	case FORMAT_R32_FLOAT:
		cs = wide ? CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT1 : CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT1;
		break;
	case FORMAT_R16G16B16A16_UNORM:
	case FORMAT_R8G8B8A8_UNORM:
	case FORMAT_B8G8R8A8_UNORM:
	case FORMAT_R10G10B10A2_UNORM:
		cs = wide ? CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_UNORM4 : CSTYPE_POSTPROCESS_BLUR_BILATERAL_UNORM4;
		break;
	case FORMAT_R11G11B10_FLOAT:
		cs = wide ? CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT3 : CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT3;
		break;
	case FORMAT_R16G16B16A16_FLOAT:
	case FORMAT_R32G32B32A32_FLOAT:
		cs = wide ? CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT4 : CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT4;
		break;
	default:
		assert(0); // implement format!
		break;
	}
	device->BindComputeShader(&shaders[cs], cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	// Horizontal:
	{
		const TextureDesc& desc = temp.GetDesc();

		PostProcessCB cb;
		cb.xPPResolution.x = desc.Width;
		cb.xPPResolution.y = desc.Height;
		if (mip_dst > 0)
		{
			cb.xPPResolution.x >>= mip_dst;
			cb.xPPResolution.y >>= mip_dst;
		}
		cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
		cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
		cb.xPPParams0.x = 1;
		cb.xPPParams0.y = 0;
		cb.xPPParams0.w = depth_threshold;
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);

		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd, mip_src);
		device->BindUAV(CS, &temp, 0, cmd, mip_dst);

		device->Dispatch(
			(cb.xPPResolution.x + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT - 1) / POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT,
			cb.xPPResolution.y,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, 1, cmd);
	}

	// Vertical:
	{
		const TextureDesc& desc = output.GetDesc();

		PostProcessCB cb;
		cb.xPPResolution.x = desc.Width;
		cb.xPPResolution.y = desc.Height;
		if (mip_dst > 0)
		{
			cb.xPPResolution.x >>= mip_dst;
			cb.xPPResolution.y >>= mip_dst;
		}
		cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
		cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
		cb.xPPParams0.x = 0;
		cb.xPPParams0.y = 1;
		cb.xPPParams0.w = depth_threshold;
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);

		device->BindResource(CS, &temp, TEXSLOT_ONDEMAND0, cmd, mip_dst); // <- also mip_dst because it's second pass!
		device->BindUAV(CS, &output, 0, cmd, mip_dst);

		device->Dispatch(
			cb.xPPResolution.x,
			(cb.xPPResolution.y + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT - 1) / POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, 1, cmd);
	}

	device->EventEnd(cmd);
}
void Postprocess_SSAO(
	const Texture& depthbuffer,
	const Texture& lineardepth,
	const Texture& output,
	CommandList cmd,
	float range,
	uint32_t samplecount,
	float power
)
{
	device->EventBegin("Postprocess_SSAO", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("SSAO", cmd);

	static TextureDesc saved_desc;
	static Texture temp0;
	static Texture temp1;

	const TextureDesc& lineardepth_desc = lineardepth.GetDesc();
	if (saved_desc.Width != lineardepth_desc.Width || saved_desc.Height != lineardepth_desc.Height)
	{
		saved_desc = lineardepth_desc; // <- this must already have SRV and UAV request flags set up!
		saved_desc.MipLevels = 1;

		TextureDesc desc = saved_desc;
		desc.Format = FORMAT_R8_UNORM;
		desc.Width = (desc.Width + 1) / 2;
		desc.Height = (desc.Height + 1) / 2;
		device->CreateTexture(&desc, nullptr, &temp0);
		device->CreateTexture(&desc, nullptr, &temp1);
	}

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSAO], cmd);

	device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	const TextureDesc& desc = temp0.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.ssao_range = range;
	cb.ssao_samplecount = (float)samplecount;
	cb.ssao_power = power;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	const GPUResource* uavs[] = {
		&temp0,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	Postprocess_Blur_Bilateral(temp0, lineardepth, temp1, temp0, cmd, 1.2f, -1, -1, true);
	Postprocess_Upsample_Bilateral(temp0, lineardepth, output, cmd);

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void Postprocess_HBAO(
	const Texture& lineardepth,
	const Texture& output,
	CommandList cmd,
	float power
)
{
	device->EventBegin("Postprocess_HBAO", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("HBAO", cmd);

	static TextureDesc saved_desc;
	static Texture temp0;
	static Texture temp1;

	const TextureDesc& lineardepth_desc = lineardepth.GetDesc();
	if (saved_desc.Width != lineardepth_desc.Width || saved_desc.Height != lineardepth_desc.Height)
	{
		saved_desc = lineardepth_desc; // <- this must already have SRV and UAV request flags set up!
		saved_desc.MipLevels = 1;

		TextureDesc desc = saved_desc;
		desc.Format = FORMAT_R8_UNORM;
		desc.Width = (desc.Width + 1) / 2;
		desc.Height = (desc.Height + 1) / 2;
		device->CreateTexture(&desc, nullptr, &temp0);
		device->CreateTexture(&desc, nullptr, &temp1);
	}

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_HBAO], cmd);

	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	const TextureDesc& desc = temp0.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = 1;
	cb.xPPParams0.y = 0;
	cb.hbao_power = power;

	const CameraComponent& camera = GetCamera();
	// Load first element of projection matrix which is the cotangent of the horizontal FOV divided by 2.
	const float TanHalfFovH = 1.0f / camera.Projection.m[0][0];
	const float FocalLenX = 1.0f / TanHalfFovH * ((float)cb.xPPResolution.y / (float)cb.xPPResolution.x);
	const float FocalLenY = 1.0f / TanHalfFovH;
	const float InvFocalLenX = 1.0f / FocalLenX;
	const float InvFocalLenY = 1.0f / FocalLenY;
	const float UVToViewAX = 2.0f * InvFocalLenX;
	const float UVToViewAY = -2.0f * InvFocalLenY;
	const float UVToViewBX = -1.0f * InvFocalLenX;
	const float UVToViewBY = 1.0f * InvFocalLenY;
	cb.xPPParams1.x = UVToViewAX;
	cb.xPPParams1.y = UVToViewAY;
	cb.xPPParams1.z = UVToViewBX;
	cb.xPPParams1.w = UVToViewBY;

	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// horizontal pass:
	{
		device->BindResource(CS, wiTextureHelper::getWhite(), TEXSLOT_ONDEMAND1, cmd);
		const GPUResource* uavs[] = {
			&temp1,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(cb.xPPResolution.x + POSTPROCESS_HBAO_THREADCOUNT - 1) / POSTPROCESS_HBAO_THREADCOUNT,
			cb.xPPResolution.y,
			1,
			cmd
			);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}

	// vertical pass:
	{
		cb.xPPParams0.x = 0;
		cb.xPPParams0.y = 1;
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

		device->BindResource(CS, &temp1, TEXSLOT_ONDEMAND1, cmd);
		const GPUResource* uavs[] = {
			&temp0,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			cb.xPPResolution.x,
			(cb.xPPResolution.y + POSTPROCESS_HBAO_THREADCOUNT - 1) / POSTPROCESS_HBAO_THREADCOUNT,
			1,
			cmd
			);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND1, 1, cmd);
	}

	Postprocess_Blur_Bilateral(temp0, lineardepth, temp1, temp0, cmd, 1.2f, -1, -1, true);
	Postprocess_Upsample_Bilateral(temp0, lineardepth, output, cmd);

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void Postprocess_MSAO(
	const Texture& lineardepth,
	const Texture& output,
	CommandList cmd,
	float power
	)
{
	device->EventBegin("Postprocess_MSAO", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("MSAO", cmd);

	static TextureDesc saved_desc;
	static Texture texture_lineardepth_downsize1;
	static Texture texture_lineardepth_tiled1;
	static Texture texture_lineardepth_downsize2;
	static Texture texture_lineardepth_tiled2;
	static Texture texture_lineardepth_downsize3;
	static Texture texture_lineardepth_tiled3;
	static Texture texture_lineardepth_downsize4;
	static Texture texture_lineardepth_tiled4;
	static Texture texture_ao_merged1;
	static Texture texture_ao_hq1;
	static Texture texture_ao_smooth1;
	static Texture texture_ao_merged2;
	static Texture texture_ao_hq2;
	static Texture texture_ao_smooth2;
	static Texture texture_ao_merged3;
	static Texture texture_ao_hq3;
	static Texture texture_ao_smooth3;
	static Texture texture_ao_merged4;
	static Texture texture_ao_hq4;

	const TextureDesc& lineardepth_desc = lineardepth.GetDesc();
	if (saved_desc.Width != lineardepth_desc.Width || saved_desc.Height != lineardepth_desc.Height)
	{
		saved_desc = lineardepth_desc; // <- this must already have SRV and UAV request flags set up!
		saved_desc.MipLevels = 1;

		const uint32_t bufferWidth = saved_desc.Width;
		const uint32_t bufferWidth1 = (bufferWidth + 1) / 2;
		const uint32_t bufferWidth2 = (bufferWidth + 3) / 4;
		const uint32_t bufferWidth3 = (bufferWidth + 7) / 8;
		const uint32_t bufferWidth4 = (bufferWidth + 15) / 16;
		const uint32_t bufferWidth5 = (bufferWidth + 31) / 32;
		const uint32_t bufferWidth6 = (bufferWidth + 63) / 64;
		const uint32_t bufferHeight = saved_desc.Height;
		const uint32_t bufferHeight1 = (bufferHeight + 1) / 2;
		const uint32_t bufferHeight2 = (bufferHeight + 3) / 4;
		const uint32_t bufferHeight3 = (bufferHeight + 7) / 8;
		const uint32_t bufferHeight4 = (bufferHeight + 15) / 16;
		const uint32_t bufferHeight5 = (bufferHeight + 31) / 32;
		const uint32_t bufferHeight6 = (bufferHeight + 63) / 64;

		TextureDesc desc = saved_desc;
		desc.Width = bufferWidth1;
		desc.Height = bufferHeight1;
		device->CreateTexture(&desc, nullptr, &texture_lineardepth_downsize1);
		desc.Width = bufferWidth3;
		desc.Height = bufferHeight3;
		desc.ArraySize = 16;
		desc.Format = FORMAT_R16_FLOAT;
		device->CreateTexture(&desc, nullptr, &texture_lineardepth_tiled1);

		desc = saved_desc;
		desc.Width = bufferWidth2;
		desc.Height = bufferHeight2;
		device->CreateTexture(&desc, nullptr, &texture_lineardepth_downsize2);
		desc.Width = bufferWidth4;
		desc.Height = bufferHeight4;
		desc.ArraySize = 16;
		desc.Format = FORMAT_R16_FLOAT;
		device->CreateTexture(&desc, nullptr, &texture_lineardepth_tiled2);

		desc = saved_desc;
		desc.Width = bufferWidth3;
		desc.Height = bufferHeight3;
		device->CreateTexture(&desc, nullptr, &texture_lineardepth_downsize3);
		desc.Width = bufferWidth5;
		desc.Height = bufferHeight5;
		desc.ArraySize = 16;
		desc.Format = FORMAT_R16_FLOAT;
		device->CreateTexture(&desc, nullptr, &texture_lineardepth_tiled3);

		desc = saved_desc;
		desc.Width = bufferWidth4;
		desc.Height = bufferHeight4;
		device->CreateTexture(&desc, nullptr, &texture_lineardepth_downsize4);
		desc.Width = bufferWidth6;
		desc.Height = bufferHeight6;
		desc.ArraySize = 16;
		desc.Format = FORMAT_R16_FLOAT;
		device->CreateTexture(&desc, nullptr, &texture_lineardepth_tiled4);

		desc = saved_desc;
		desc.Format = FORMAT_R8_UNORM;
		desc.Width = bufferWidth1;
		desc.Height = bufferHeight1;
		device->CreateTexture(&desc, nullptr, &texture_ao_merged1);
		device->CreateTexture(&desc, nullptr, &texture_ao_hq1);
		device->CreateTexture(&desc, nullptr, &texture_ao_smooth1);
		desc.Width = bufferWidth2;
		desc.Height = bufferHeight2;
		device->CreateTexture(&desc, nullptr, &texture_ao_merged2);
		device->CreateTexture(&desc, nullptr, &texture_ao_hq2);
		device->CreateTexture(&desc, nullptr, &texture_ao_smooth2);
		desc.Width = bufferWidth3;
		desc.Height = bufferHeight3;
		device->CreateTexture(&desc, nullptr, &texture_ao_merged3);
		device->CreateTexture(&desc, nullptr, &texture_ao_hq3);
		device->CreateTexture(&desc, nullptr, &texture_ao_smooth3);
		desc.Width = bufferWidth4;
		desc.Height = bufferHeight4;
		device->CreateTexture(&desc, nullptr, &texture_ao_merged4);
		device->CreateTexture(&desc, nullptr, &texture_ao_hq4);
	}

	// Depth downsampling + deinterleaving pass1:
	{
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MSAO_PREPAREDEPTHBUFFERS1], cmd);

		device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

		const GPUResource* uavs[] = {
			&texture_lineardepth_downsize1,
			&texture_lineardepth_tiled1,
			&texture_lineardepth_downsize2,
			&texture_lineardepth_tiled2,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		const TextureDesc& desc = texture_lineardepth_tiled2.GetDesc();
		device->Dispatch(desc.Width, desc.Height, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}

	// Depth downsampling + deinterleaving pass2:
	{
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MSAO_PREPAREDEPTHBUFFERS2], cmd);

		device->BindResource(CS, &texture_lineardepth_downsize2, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&texture_lineardepth_downsize3,
			&texture_lineardepth_tiled3,
			&texture_lineardepth_downsize4,
			&texture_lineardepth_tiled4,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		const TextureDesc& desc = texture_lineardepth_tiled4.GetDesc();
		device->Dispatch(desc.Width, desc.Height, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}


	float SampleThickness[12];
	SampleThickness[0] = sqrt(1.0f - 0.2f * 0.2f);
	SampleThickness[1] = sqrt(1.0f - 0.4f * 0.4f);
	SampleThickness[2] = sqrt(1.0f - 0.6f * 0.6f);
	SampleThickness[3] = sqrt(1.0f - 0.8f * 0.8f);
	SampleThickness[4] = sqrt(1.0f - 0.2f * 0.2f - 0.2f * 0.2f);
	SampleThickness[5] = sqrt(1.0f - 0.2f * 0.2f - 0.4f * 0.4f);
	SampleThickness[6] = sqrt(1.0f - 0.2f * 0.2f - 0.6f * 0.6f);
	SampleThickness[7] = sqrt(1.0f - 0.2f * 0.2f - 0.8f * 0.8f);
	SampleThickness[8] = sqrt(1.0f - 0.4f * 0.4f - 0.4f * 0.4f);
	SampleThickness[9] = sqrt(1.0f - 0.4f * 0.4f - 0.6f * 0.6f);
	SampleThickness[10] = sqrt(1.0f - 0.4f * 0.4f - 0.8f * 0.8f);
	SampleThickness[11] = sqrt(1.0f - 0.6f * 0.6f - 0.6f * 0.6f);
	static float RejectionFalloff = 2.0f;
	const float Accentuation = 0.1f * power;

	// The msao_compute will be called repeatedly, so create a local lambda for it:
	auto msao_compute = [&](const Texture& write_result, const Texture& read_depth) 
	{
		const TextureDesc& desc = read_depth.GetDesc();

		MSAOCB cb;

		const CameraComponent& camera = GetCamera();

		// Load first element of projection matrix which is the cotangent of the horizontal FOV divided by 2.
		const float TanHalfFovH = 1.0f / camera.Projection.m[0][0];

		// Here we compute multipliers that convert the center depth value into (the reciprocal of)
		// sphere thicknesses at each sample location.  This assumes a maximum sample radius of 5
		// units, but since a sphere has no thickness at its extent, we don't need to sample that far
		// out.  Only samples whole integer offsets with distance less than 25 are used.  This means
		// that there is no sample at (3, 4) because its distance is exactly 25 (and has a thickness of 0.)

		// The shaders are set up to sample a circular region within a 5-pixel radius.
		const float ScreenspaceDiameter = 10.0f;

		// SphereDiameter = CenterDepth * ThicknessMultiplier.  This will compute the thickness of a sphere centered
		// at a specific depth.  The ellipsoid scale can stretch a sphere into an ellipsoid, which changes the
		// characteristics of the AO.
		// TanHalfFovH:  Radius of sphere in depth units if its center lies at Z = 1
		// ScreenspaceDiameter:  Diameter of sample sphere in pixel units
		// ScreenspaceDiameter / BufferWidth:  Ratio of the screen width that the sphere actually covers
		// Note about the "2.0f * ":  Diameter = 2 * Radius
		float ThicknessMultiplier = 2.0f * TanHalfFovH * ScreenspaceDiameter / desc.Width;
		if (desc.ArraySize == 1)
		{
			ThicknessMultiplier *= 2.0f;
		}

		// This will transform a depth value from [0, thickness] to [0, 1].
		float InverseRangeFactor = 1.0f / ThicknessMultiplier;

		// The thicknesses are smaller for all off-center samples of the sphere.  Compute thicknesses relative
		// to the center sample.
		cb.xInvThicknessTable[0].x = InverseRangeFactor / SampleThickness[0];
		cb.xInvThicknessTable[0].y = InverseRangeFactor / SampleThickness[1];
		cb.xInvThicknessTable[0].z = InverseRangeFactor / SampleThickness[2];
		cb.xInvThicknessTable[0].w = InverseRangeFactor / SampleThickness[3];
		cb.xInvThicknessTable[1].x = InverseRangeFactor / SampleThickness[4];
		cb.xInvThicknessTable[1].y = InverseRangeFactor / SampleThickness[5];
		cb.xInvThicknessTable[1].z = InverseRangeFactor / SampleThickness[6];
		cb.xInvThicknessTable[1].w = InverseRangeFactor / SampleThickness[7];
		cb.xInvThicknessTable[2].x = InverseRangeFactor / SampleThickness[8];
		cb.xInvThicknessTable[2].y = InverseRangeFactor / SampleThickness[9];
		cb.xInvThicknessTable[2].z = InverseRangeFactor / SampleThickness[10];
		cb.xInvThicknessTable[2].w = InverseRangeFactor / SampleThickness[11];

		// These are the weights that are multiplied against the samples because not all samples are
		// equally important.  The farther the sample is from the center location, the less they matter.
		// We use the thickness of the sphere to determine the weight.  The scalars in front are the number
		// of samples with this weight because we sum the samples together before multiplying by the weight,
		// so as an aggregate all of those samples matter more.  After generating this table, the weights
		// are normalized.
		cb.xSampleWeightTable[0].x = 4.0f * SampleThickness[0];    // Axial
		cb.xSampleWeightTable[0].y = 4.0f * SampleThickness[1];    // Axial
		cb.xSampleWeightTable[0].z = 4.0f * SampleThickness[2];    // Axial
		cb.xSampleWeightTable[0].w = 4.0f * SampleThickness[3];    // Axial
		cb.xSampleWeightTable[1].x = 4.0f * SampleThickness[4];    // Diagonal
		cb.xSampleWeightTable[1].y = 8.0f * SampleThickness[5];    // L-shaped
		cb.xSampleWeightTable[1].z = 8.0f * SampleThickness[6];    // L-shaped
		cb.xSampleWeightTable[1].w = 8.0f * SampleThickness[7];    // L-shaped
		cb.xSampleWeightTable[2].x = 4.0f * SampleThickness[8];    // Diagonal
		cb.xSampleWeightTable[2].y = 8.0f * SampleThickness[9];    // L-shaped
		cb.xSampleWeightTable[2].z = 8.0f * SampleThickness[10];   // L-shaped
		cb.xSampleWeightTable[2].w = 4.0f * SampleThickness[11];   // Diagonal

		// If we aren't using all of the samples, delete their weights before we normalize.
#ifndef MSAO_SAMPLE_EXHAUSTIVELY
		cb.xSampleWeightTable[0].x = 0.0f;
		cb.xSampleWeightTable[0].z = 0.0f;
		cb.xSampleWeightTable[1].y = 0.0f;
		cb.xSampleWeightTable[1].w = 0.0f;
		cb.xSampleWeightTable[2].y = 0.0f;
#endif

		// Normalize the weights by dividing by the sum of all weights
		float totalWeight = 0.0f;
		totalWeight += cb.xSampleWeightTable[0].x;
		totalWeight += cb.xSampleWeightTable[0].y;
		totalWeight += cb.xSampleWeightTable[0].z;
		totalWeight += cb.xSampleWeightTable[0].w;
		totalWeight += cb.xSampleWeightTable[1].x;
		totalWeight += cb.xSampleWeightTable[1].y;
		totalWeight += cb.xSampleWeightTable[1].z;
		totalWeight += cb.xSampleWeightTable[1].w;
		totalWeight += cb.xSampleWeightTable[2].x;
		totalWeight += cb.xSampleWeightTable[2].y;
		totalWeight += cb.xSampleWeightTable[2].z;
		totalWeight += cb.xSampleWeightTable[2].w;
		cb.xSampleWeightTable[0].x /= totalWeight;
		cb.xSampleWeightTable[0].y /= totalWeight;
		cb.xSampleWeightTable[0].z /= totalWeight;
		cb.xSampleWeightTable[0].w /= totalWeight;
		cb.xSampleWeightTable[1].x /= totalWeight;
		cb.xSampleWeightTable[1].y /= totalWeight;
		cb.xSampleWeightTable[1].z /= totalWeight;
		cb.xSampleWeightTable[1].w /= totalWeight;
		cb.xSampleWeightTable[2].x /= totalWeight;
		cb.xSampleWeightTable[2].y /= totalWeight;
		cb.xSampleWeightTable[2].z /= totalWeight;
		cb.xSampleWeightTable[2].w /= totalWeight;

		cb.xInvSliceDimension.x = 1.0f / desc.Width;
		cb.xInvSliceDimension.y = 1.0f / desc.Height;
		cb.xRejectFadeoff = 1.0f / -RejectionFalloff;
		cb.xRcpAccentuation = 1.0f / (1.0f + Accentuation);

		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS_MSAO], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS_MSAO], CB_GETBINDSLOT(MSAOCB), cmd);

		device->BindResource(CS, &read_depth, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&write_result,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		if (desc.ArraySize == 1)
		{
			device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MSAO], cmd);
			device->Dispatch((desc.Width + 15) / 16, (desc.Height + 15) / 16, 1, cmd);
		}
		else
		{
			device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MSAO_INTERLEAVE], cmd);
			device->Dispatch((desc.Width + 7) / 8, (desc.Height + 7) / 8, desc.ArraySize, cmd);
		}

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}; // end of lambda: msao_compute

	msao_compute(texture_ao_merged4, texture_lineardepth_tiled4);
	msao_compute(texture_ao_hq4, texture_lineardepth_downsize4);

	msao_compute(texture_ao_merged3, texture_lineardepth_tiled3);
	msao_compute(texture_ao_hq3, texture_lineardepth_downsize3);

	msao_compute(texture_ao_merged2, texture_lineardepth_tiled2);
	msao_compute(texture_ao_hq2, texture_lineardepth_downsize2);

	msao_compute(texture_ao_merged1, texture_lineardepth_tiled1);
	msao_compute(texture_ao_hq1, texture_lineardepth_downsize1);

	auto blur_and_upsample = [&](const Texture& Destination, const Texture& HiResDepth, const Texture& LoResDepth,
		const Texture* InterleavedAO, const Texture* HighQualityAO, const Texture* HiResAO)
	{
		const uint32_t LoWidth = LoResDepth.GetDesc().Width;
		const uint32_t LoHeight = LoResDepth.GetDesc().Height;
		const uint32_t HiWidth = HiResDepth.GetDesc().Width;
		const uint32_t HiHeight = HiResDepth.GetDesc().Height;

		if (HiResAO == nullptr)
		{
			if (HighQualityAO == nullptr)
			{
				device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE], cmd);
			}
			else
			{
				device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_PREMIN], cmd);
			}
		}
		else
		{
			if (HighQualityAO == nullptr)
			{
				device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_BLENDOUT], cmd);
			}
			else
			{
				device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_PREMIN_BLENDOUT], cmd);
			}
		}

		static float g_NoiseFilterTolerance = -3.0f;
		static float g_BlurTolerance = -5.0f;
		static float g_UpsampleTolerance = -7.0f;

		float kBlurTolerance = 1.0f - powf(10.0f, g_BlurTolerance) * 1920.0f / (float)LoWidth;
		kBlurTolerance *= kBlurTolerance;
		float kUpsampleTolerance = powf(10.0f, g_UpsampleTolerance);
		float kNoiseFilterWeight = 1.0f / (powf(10.0f, g_NoiseFilterTolerance) + kUpsampleTolerance);

		MSAO_UPSAMPLECB cb;
		cb.InvLowResolution = float2(1.0f / LoWidth, 1.0f / LoHeight);
		cb.InvHighResolution = float2(1.0f / HiWidth, 1.0f / HiHeight);
		cb.NoiseFilterStrength = kNoiseFilterWeight;
		cb.StepSize = (float)lineardepth.GetDesc().Width / (float)LoWidth;
		cb.kBlurTolerance = kBlurTolerance;
		cb.kUpsampleTolerance = kUpsampleTolerance;

		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS_MSAO_UPSAMPLE], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS_MSAO_UPSAMPLE], CB_GETBINDSLOT(MSAO_UPSAMPLECB), cmd);
		
		device->BindUAV(CS, &Destination, 0, cmd);
		device->BindResource(CS, &LoResDepth, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &HiResDepth, TEXSLOT_ONDEMAND1, cmd);
		if (InterleavedAO != nullptr)
		{
			device->BindResource(CS, InterleavedAO, TEXSLOT_ONDEMAND2, cmd);
		}
		if (HighQualityAO != nullptr)
		{
			device->BindResource(CS, HighQualityAO, TEXSLOT_ONDEMAND3, cmd);
		}
		if (HiResAO != nullptr)
		{
			device->BindResource(CS, HiResAO, TEXSLOT_ONDEMAND4, cmd);
		}

		device->Dispatch((HiWidth + 2 + 15) / 16, (HiHeight + 2 + 15) / 16, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	};

	blur_and_upsample(texture_ao_smooth3, texture_lineardepth_downsize3, texture_lineardepth_downsize4, &texture_ao_merged4,
		&texture_ao_hq4, &texture_ao_merged3);

	blur_and_upsample(texture_ao_smooth2, texture_lineardepth_downsize2, texture_lineardepth_downsize3, &texture_ao_smooth3,
		&texture_ao_hq3, &texture_ao_merged2);

	blur_and_upsample(texture_ao_smooth1, texture_lineardepth_downsize1, texture_lineardepth_downsize2, &texture_ao_smooth2,
		&texture_ao_hq2, &texture_ao_merged1);

	blur_and_upsample(output, lineardepth, texture_lineardepth_downsize1, &texture_ao_smooth1,
		&texture_ao_hq1, nullptr);

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void Postprocess_RTAO(
	const Texture& depthbuffer,
	const Texture& lineardepth,
	const Texture& depth_history,
	const Texture& output,
	CommandList cmd,
	float range,
	uint32_t samplecount,
	float power
)
{
	if (!wiRenderer::device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
		return;
	if (!wiRenderer::device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_DESCRIPTOR_MANAGEMENT))
		return;

	const Scene& scene = wiScene::GetScene();
	if (scene.objects.GetCount() <= 0)
	{
		return;
	}

	device->EventBegin("Postprocess_RTAO", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("RTAO", cmd);

	static RaytracingPipelineState RTPSO;
	static DescriptorTable descriptorTable;
	static RootSignature rootSignature;

	auto load_shaders = [&scene](uint64_t userdata) {

		descriptorTable = DescriptorTable();
		descriptorTable.resources.push_back({ TEXTURE2D, TEXSLOT_DEPTH });
		descriptorTable.resources.push_back({ TEXTURE2D, TEXSLOT_ONDEMAND0 });
		descriptorTable.resources.push_back({ ACCELERATIONSTRUCTURE, TEXSLOT_ACCELERATION_STRUCTURE });
		descriptorTable.resources.push_back({ RWTEXTURE2D, 0 });
		descriptorTable.resources.push_back({ ROOT_CONSTANTBUFFER, CB_GETBINDSLOT(FrameCB) });
		descriptorTable.resources.push_back({ ROOT_CONSTANTBUFFER, CB_GETBINDSLOT(CameraCB) });
		descriptorTable.resources.push_back({ ROOT_CONSTANTBUFFER, CB_GETBINDSLOT(PostProcessCB) });
		descriptorTable.staticsamplers.push_back({ samplers[SSLOT_POINT_CLAMP], SSLOT_POINT_CLAMP });
		descriptorTable.staticsamplers.push_back({ samplers[SSLOT_LINEAR_CLAMP], SSLOT_LINEAR_CLAMP });
		descriptorTable.staticsamplers.push_back({ samplers[SSLOT_POINT_WRAP], SSLOT_POINT_WRAP });
		descriptorTable.staticsamplers.push_back({ samplers[SSLOT_LINEAR_WRAP], SSLOT_LINEAR_WRAP });
		device->CreateDescriptorTable(&descriptorTable);

		rootSignature = RootSignature();
		rootSignature.tables.push_back(descriptorTable);
		rootSignature.tables.push_back(scene.descriptorTable);
		device->CreateRootSignature(&rootSignature);

		shaders[RTTYPE_RTAO].rootSignature = &rootSignature;
		bool success = LoadShader(SHADERSTAGE_COUNT, shaders[RTTYPE_RTAO], "rtaoLIB.cso");
		assert(success);

		RaytracingPipelineStateDesc rtdesc;
		rtdesc.rootSignature = &rootSignature;

		rtdesc.shaderlibraries.emplace_back();
		rtdesc.shaderlibraries.back().shader = &shaders[RTTYPE_RTAO];
		rtdesc.shaderlibraries.back().function_name = "RTAO_Raygen";
		rtdesc.shaderlibraries.back().type = ShaderLibrary::RAYGENERATION;

		rtdesc.shaderlibraries.emplace_back();
		rtdesc.shaderlibraries.back().shader = &shaders[RTTYPE_RTAO];
		rtdesc.shaderlibraries.back().function_name = "RTAO_ClosestHit";
		rtdesc.shaderlibraries.back().type = ShaderLibrary::CLOSESTHIT;

		rtdesc.shaderlibraries.emplace_back();
		rtdesc.shaderlibraries.back().shader = &shaders[RTTYPE_RTAO];
		rtdesc.shaderlibraries.back().function_name = "RTAO_AnyHit";
		rtdesc.shaderlibraries.back().type = ShaderLibrary::ANYHIT;

		rtdesc.shaderlibraries.emplace_back();
		rtdesc.shaderlibraries.back().shader = &shaders[RTTYPE_RTAO];
		rtdesc.shaderlibraries.back().function_name = "RTAO_Miss";
		rtdesc.shaderlibraries.back().type = ShaderLibrary::MISS;

		rtdesc.hitgroups.emplace_back();
		rtdesc.hitgroups.back().type = ShaderHitGroup::GENERAL;
		rtdesc.hitgroups.back().name = "RTAO_Raygen";
		rtdesc.hitgroups.back().general_shader = 0;

		rtdesc.hitgroups.emplace_back();
		rtdesc.hitgroups.back().type = ShaderHitGroup::GENERAL;
		rtdesc.hitgroups.back().name = "RTAO_Miss";
		rtdesc.hitgroups.back().general_shader = 3;

		rtdesc.hitgroups.emplace_back();
		rtdesc.hitgroups.back().type = ShaderHitGroup::TRIANGLES;
		rtdesc.hitgroups.back().name = "RTAO_Hitgroup";
		rtdesc.hitgroups.back().closesthit_shader = 1;
		rtdesc.hitgroups.back().anyhit_shader = 2;

		rtdesc.max_trace_recursion_depth = 1;
		rtdesc.max_payload_size_in_bytes = sizeof(float);
		rtdesc.max_attribute_size_in_bytes = sizeof(XMFLOAT2); // bary
		success = device->CreateRaytracingPipelineState(&rtdesc, &RTPSO);
		assert(success);

		success = LoadShader(CS, shaders[CSTYPE_POSTPROCESS_RTAO_DENOISE_TEMPORAL], "rtao_denoise_temporalCS.cso");
		assert(success);
		success = LoadShader(CS, shaders[CSTYPE_POSTPROCESS_RTAO_DENOISE_BLUR], "rtao_denoise_blurCS.cso");
		assert(success);
	};

	static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, load_shaders);
	if (!RTPSO.IsValid())
	{
		load_shaders(0);
	}

	static TextureDesc saved_desc;
	static Texture temp;
	static Texture temporal[2];
	static Texture normals;

	const TextureDesc& lineardepth_desc = lineardepth.GetDesc();
	if (saved_desc.Width != lineardepth_desc.Width || saved_desc.Height != lineardepth_desc.Height)
	{
		saved_desc = lineardepth_desc; // <- this must already have SRV and UAV request flags set up!
		saved_desc.MipLevels = 1;

		TextureDesc desc = saved_desc;
		desc.Format = FORMAT_R8_UNORM;
		desc.Width = (desc.Width + 1) / 2;
		desc.Height = (desc.Height + 1) / 2;
		device->CreateTexture(&desc, nullptr, &temp);
		device->SetName(&temp, "rtao_temp");

		device->CreateTexture(&desc, nullptr, &temporal[0]);
		device->SetName(&temporal[0], "rtao_temporal[0]");
		device->CreateTexture(&desc, nullptr, &temporal[1]);
		device->SetName(&temporal[1], "rtao_temporal[1]");

		desc.Format = FORMAT_R11G11B10_FLOAT;
		device->CreateTexture(&desc, nullptr, &normals);
		device->SetName(&normals, "rtao_normals");
	}

	Postprocess_NormalsFromDepth(depthbuffer, normals, cmd);

	device->EventBegin("Raytrace", cmd);

	const TextureDesc& desc = temp.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.rtao_range = range;
	cb.rtao_samplecount = (float)samplecount;
	cb.rtao_power = power;
	cb.rtao_seed = renderTime;
	GraphicsDevice::GPUAllocation cb_alloc = device->AllocateGPU(sizeof(cb), cmd);
	memcpy(cb_alloc.data, &cb, sizeof(cb));

	device->BindRaytracingPipelineState(&RTPSO, cmd);
	device->WriteDescriptor(&descriptorTable, 0, 0, &depthbuffer);
	device->WriteDescriptor(&descriptorTable, 1, 0, &normals);
	device->WriteDescriptor(&descriptorTable, 2, 0, &scene.TLAS);
	device->WriteDescriptor(&descriptorTable, 3, 0, &temp);
	device->BindDescriptorTable(RAYTRACING, 0, &descriptorTable, cmd);
	device->BindDescriptorTable(RAYTRACING, 1, &scene.descriptorTable, cmd);
	device->BindRootDescriptor(RAYTRACING, 0, &constantBuffers[CBTYPE_FRAME], 0, cmd);
	device->BindRootDescriptor(RAYTRACING, 1, &constantBuffers[CBTYPE_CAMERA], 0, cmd);
	device->BindRootDescriptor(RAYTRACING, 2, cb_alloc.buffer, cb_alloc.offset, cmd);

	size_t shaderIdentifierSize = device->GetShaderIdentifierSize();
	GraphicsDevice::GPUAllocation shadertable_raygen = device->AllocateGPU(shaderIdentifierSize, cmd);
	GraphicsDevice::GPUAllocation shadertable_miss = device->AllocateGPU(shaderIdentifierSize, cmd);
	GraphicsDevice::GPUAllocation shadertable_hitgroup = device->AllocateGPU(shaderIdentifierSize, cmd);

	device->WriteShaderIdentifier(&RTPSO, 0, shadertable_raygen.data);
	device->WriteShaderIdentifier(&RTPSO, 1, shadertable_miss.data);
	device->WriteShaderIdentifier(&RTPSO, 2, shadertable_hitgroup.data);

	DispatchRaysDesc dispatchraysdesc;
	dispatchraysdesc.raygeneration.buffer = shadertable_raygen.buffer;
	dispatchraysdesc.raygeneration.offset = shadertable_raygen.offset;
	dispatchraysdesc.raygeneration.size = shaderIdentifierSize;

	dispatchraysdesc.miss.buffer = shadertable_miss.buffer;
	dispatchraysdesc.miss.offset = shadertable_miss.offset;
	dispatchraysdesc.miss.size = shaderIdentifierSize;
	dispatchraysdesc.miss.stride = shaderIdentifierSize;

	dispatchraysdesc.hitgroup.buffer = shadertable_hitgroup.buffer;
	dispatchraysdesc.hitgroup.offset = shadertable_hitgroup.offset;
	dispatchraysdesc.hitgroup.size = shaderIdentifierSize;
	dispatchraysdesc.hitgroup.stride = shaderIdentifierSize;

	dispatchraysdesc.Width = desc.Width;
	dispatchraysdesc.Height = desc.Height;

	device->DispatchRays(&dispatchraysdesc, cmd);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->EventEnd(cmd);

	int temporal_output = device->GetFrameCount() % 2;
	int temporal_history = 1 - temporal_output;

	// Temporal pass:
	{
		device->EventBegin("Temporal Denoise", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_RTAO_DENOISE_TEMPORAL], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &temp, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &temporal[temporal_history], TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &depth_history, TEXSLOT_ONDEMAND2, cmd);

		const GPUResource* uavs[] = {
			&temporal[temporal_output],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(temporal[temporal_output].GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(temporal[temporal_output].GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	{

		device->EventBegin("Blur Denoise", cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_RTAO_DENOISE_BLUR], cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

		device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);
		device->BindResource(CS, &normals, TEXSLOT_ONDEMAND1, cmd);

		static float depth_threshold = 1.2f;

		// Horizontal:
		{
			PostProcessCB cb;
			cb.xPPResolution.x = desc.Width;
			cb.xPPResolution.y = desc.Height;
			cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
			cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
			cb.xPPParams0.x = 1;
			cb.xPPParams0.y = 0;
			cb.xPPParams0.w = depth_threshold;
			device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);

			device->BindResource(CS, &temporal[temporal_output], TEXSLOT_ONDEMAND0, cmd);
			device->BindUAV(CS, &temp, 0, cmd);

			device->Dispatch(
				(cb.xPPResolution.x + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT - 1) / POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT,
				cb.xPPResolution.y,
				1,
				cmd
			);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

			device->UnbindUAVs(0, 1, cmd);
		}

		// Vertical:
		{
			PostProcessCB cb;
			cb.xPPResolution.x = desc.Width;
			cb.xPPResolution.y = desc.Height;
			cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
			cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
			cb.xPPParams0.x = 0;
			cb.xPPParams0.y = 1;
			cb.xPPParams0.w = depth_threshold;
			device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);

			device->BindResource(CS, &temp, TEXSLOT_ONDEMAND0, cmd);
			device->BindUAV(CS, &temporal[temporal_output], 0, cmd);

			device->Dispatch(
				cb.xPPResolution.x,
				(cb.xPPResolution.y + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT - 1) / POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT,
				1,
				cmd
			);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

			device->UnbindUAVs(0, 1, cmd);
		}

		device->EventEnd(cmd);
	}

	Postprocess_Upsample_Bilateral(temporal[temporal_output], lineardepth, output, cmd);

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void Postprocess_RTReflection(
	const Texture& depthbuffer,
	const Texture gbuffer[GBUFFER_COUNT],
	const Texture& output,
	CommandList cmd,
	float range
)
{
	if (!wiRenderer::device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
		return;
	if (!wiRenderer::device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_DESCRIPTOR_MANAGEMENT))
		return;

	const Scene& scene = wiScene::GetScene();
	if (scene.objects.GetCount() <= 0)
	{
		return;
	}

	device->EventBegin("Postprocess_RTReflection", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("RTReflection", cmd);

	static Texture temp;
	static Texture texture_temporal[2];

	if (temp.desc.Width != output.desc.Width / 2 || temp.desc.Height != output.desc.Height / 2)
	{
		TextureDesc desc = output.desc;
		desc.BindFlags |= BIND_UNORDERED_ACCESS;
		desc.Width /= 2;
		desc.Height /= 2;
		
		device->CreateTexture(&desc, nullptr, &temp);
		device->SetName(&temp, "rtreflection_temp");

		device->CreateTexture(&desc, nullptr, &texture_temporal[0]);
		device->CreateTexture(&desc, nullptr, &texture_temporal[1]);

	}

	static RaytracingPipelineState RTPSO;
	static DescriptorTable descriptorTable;
	static RootSignature rootSignature;

	auto load_shaders = [&scene](uint64_t userdata) {

		descriptorTable = DescriptorTable();
		descriptorTable.resources.push_back({ RWTEXTURE2D, 0 });
		descriptorTable.resources.push_back({ ACCELERATIONSTRUCTURE, TEXSLOT_ACCELERATION_STRUCTURE });
		descriptorTable.resources.push_back({ TEXTURE2D, TEXSLOT_DEPTH });
		descriptorTable.resources.push_back({ TEXTURE2D, TEXSLOT_GBUFFER0 });
		descriptorTable.resources.push_back({ TEXTURE2D, TEXSLOT_GBUFFER1 });
		descriptorTable.resources.push_back({ TEXTURECUBEARRAY, TEXSLOT_ENVMAPARRAY });
		descriptorTable.resources.push_back({ TEXTURE2DARRAY, TEXSLOT_SHADOWARRAY_2D });
		descriptorTable.resources.push_back({ TEXTURECUBEARRAY, TEXSLOT_SHADOWARRAY_CUBE });
		descriptorTable.resources.push_back({ TEXTURECUBEARRAY, TEXSLOT_SHADOWARRAY_TRANSPARENT });
		descriptorTable.resources.push_back({ STRUCTUREDBUFFER, SBSLOT_ENTITYARRAY });
		descriptorTable.resources.push_back({ STRUCTUREDBUFFER, SBSLOT_MATRIXARRAY });
		descriptorTable.resources.push_back({ TEXTURE2D, TEXSLOT_SKYVIEWLUT });
		descriptorTable.resources.push_back({ TEXTURE2D, TEXSLOT_TRANSMITTANCELUT });
		descriptorTable.resources.push_back({ TEXTURE2D, TEXSLOT_MULTISCATTERINGLUT });
		descriptorTable.resources.push_back({ ROOT_CONSTANTBUFFER, CB_GETBINDSLOT(FrameCB) });
		descriptorTable.resources.push_back({ ROOT_CONSTANTBUFFER, CB_GETBINDSLOT(CameraCB) });
		descriptorTable.resources.push_back({ ROOT_CONSTANTBUFFER, CB_GETBINDSLOT(PostProcessCB) });
		descriptorTable.staticsamplers.push_back({ samplers[SSLOT_POINT_CLAMP], SSLOT_POINT_CLAMP });
		descriptorTable.staticsamplers.push_back({ samplers[SSLOT_LINEAR_CLAMP], SSLOT_LINEAR_CLAMP });
		descriptorTable.staticsamplers.push_back({ samplers[SSLOT_POINT_WRAP], SSLOT_POINT_WRAP });
		descriptorTable.staticsamplers.push_back({ samplers[SSLOT_LINEAR_WRAP], SSLOT_LINEAR_WRAP });
		descriptorTable.staticsamplers.push_back({ samplers[SSLOT_CMP_DEPTH], SSLOT_CMP_DEPTH });
		device->CreateDescriptorTable(&descriptorTable);

		rootSignature = RootSignature();
		rootSignature.tables.push_back(descriptorTable);
		rootSignature.tables.push_back(scene.descriptorTable);
		device->CreateRootSignature(&rootSignature);

		shaders[RTTYPE_RTREFLECTION].rootSignature = &rootSignature;
		bool success = LoadShader(SHADERSTAGE_COUNT, shaders[RTTYPE_RTREFLECTION], "rtreflectionLIB.cso");
		assert(success);

		RaytracingPipelineStateDesc rtdesc;
		rtdesc.rootSignature = &rootSignature;

		rtdesc.shaderlibraries.emplace_back();
		rtdesc.shaderlibraries.back().shader = &shaders[RTTYPE_RTREFLECTION];
		rtdesc.shaderlibraries.back().function_name = "RTReflection_Raygen";
		rtdesc.shaderlibraries.back().type = ShaderLibrary::RAYGENERATION;

		rtdesc.shaderlibraries.emplace_back();
		rtdesc.shaderlibraries.back().shader = &shaders[RTTYPE_RTREFLECTION];
		rtdesc.shaderlibraries.back().function_name = "RTReflection_ClosestHit";
		rtdesc.shaderlibraries.back().type = ShaderLibrary::CLOSESTHIT;

		rtdesc.shaderlibraries.emplace_back();
		rtdesc.shaderlibraries.back().shader = &shaders[RTTYPE_RTREFLECTION];
		rtdesc.shaderlibraries.back().function_name = "RTReflection_AnyHit";
		rtdesc.shaderlibraries.back().type = ShaderLibrary::ANYHIT;

		rtdesc.shaderlibraries.emplace_back();
		rtdesc.shaderlibraries.back().shader = &shaders[RTTYPE_RTREFLECTION];
		rtdesc.shaderlibraries.back().function_name = "RTReflection_Miss";
		rtdesc.shaderlibraries.back().type = ShaderLibrary::MISS;

		rtdesc.hitgroups.emplace_back();
		rtdesc.hitgroups.back().type = ShaderHitGroup::GENERAL;
		rtdesc.hitgroups.back().name = "RTReflection_Raygen";
		rtdesc.hitgroups.back().general_shader = 0;

		rtdesc.hitgroups.emplace_back();
		rtdesc.hitgroups.back().type = ShaderHitGroup::GENERAL;
		rtdesc.hitgroups.back().name = "RTReflection_Miss";
		rtdesc.hitgroups.back().general_shader = 3;

		rtdesc.hitgroups.emplace_back();
		rtdesc.hitgroups.back().type = ShaderHitGroup::TRIANGLES;
		rtdesc.hitgroups.back().name = "RTReflection_Hitgroup";
		rtdesc.hitgroups.back().closesthit_shader = 1;
		rtdesc.hitgroups.back().anyhit_shader = 2;

		rtdesc.max_trace_recursion_depth = 1;
		rtdesc.max_payload_size_in_bytes = sizeof(float4);
		rtdesc.max_attribute_size_in_bytes = sizeof(float2); // bary
		success = device->CreateRaytracingPipelineState(&rtdesc, &RTPSO);
		assert(success);
	};

	static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, load_shaders);
	if (!RTPSO.IsValid())
	{
		load_shaders(0);
	}

	PostProcessCB cb;
	cb.xPPResolution.x = temp.desc.Width;
	cb.xPPResolution.y = temp.desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.rtreflection_range = range;
	cb.rtreflection_seed = renderTime;
	GraphicsDevice::GPUAllocation cb_alloc = device->AllocateGPU(sizeof(cb), cmd);
	memcpy(cb_alloc.data, &cb, sizeof(cb));

	device->BindRaytracingPipelineState(&RTPSO, cmd);
	device->WriteDescriptor(&descriptorTable, 0, 0, &temp);
	device->WriteDescriptor(&descriptorTable, 1, 0, &scene.TLAS);
	device->WriteDescriptor(&descriptorTable, 2, 0, &depthbuffer);
	device->WriteDescriptor(&descriptorTable, 3, 0, &gbuffer[GBUFFER_ALBEDO_ROUGHNESS]);
	device->WriteDescriptor(&descriptorTable, 4, 0, &gbuffer[GBUFFER_NORMAL_VELOCITY]);
	device->WriteDescriptor(&descriptorTable, 5, 0, &textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]);
	device->WriteDescriptor(&descriptorTable, 6, 0, &shadowMapArray_2D);
	device->WriteDescriptor(&descriptorTable, 7, 0, &shadowMapArray_Cube);
	device->WriteDescriptor(&descriptorTable, 8, 0, &shadowMapArray_Transparent);
	device->WriteDescriptor(&descriptorTable, 9, 0, &resourceBuffers[RBTYPE_ENTITYARRAY]);
	device->WriteDescriptor(&descriptorTable, 10, 0, &resourceBuffers[RBTYPE_MATRIXARRAY]);
	device->WriteDescriptor(&descriptorTable, 11, 0, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT]);
	device->WriteDescriptor(&descriptorTable, 12, 0, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT]);
	device->WriteDescriptor(&descriptorTable, 13, 0, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT]);
	device->BindDescriptorTable(RAYTRACING, 0, &descriptorTable, cmd);
	device->BindDescriptorTable(RAYTRACING, 1, &scene.descriptorTable, cmd);
	device->BindRootDescriptor(RAYTRACING, 0, &constantBuffers[CBTYPE_FRAME], 0, cmd);
	device->BindRootDescriptor(RAYTRACING, 1, &constantBuffers[CBTYPE_CAMERA], 0, cmd);
	device->BindRootDescriptor(RAYTRACING, 2, cb_alloc.buffer, cb_alloc.offset, cmd);

	size_t shaderIdentifierSize = device->GetShaderIdentifierSize();
	GraphicsDevice::GPUAllocation shadertable_raygen = device->AllocateGPU(shaderIdentifierSize, cmd);
	GraphicsDevice::GPUAllocation shadertable_miss = device->AllocateGPU(shaderIdentifierSize, cmd);
	GraphicsDevice::GPUAllocation shadertable_hitgroup = device->AllocateGPU(shaderIdentifierSize, cmd);

	device->WriteShaderIdentifier(&RTPSO, 0, shadertable_raygen.data);
	device->WriteShaderIdentifier(&RTPSO, 1, shadertable_miss.data);
	device->WriteShaderIdentifier(&RTPSO, 2, shadertable_hitgroup.data);

	DispatchRaysDesc dispatchraysdesc;
	dispatchraysdesc.raygeneration.buffer = shadertable_raygen.buffer;
	dispatchraysdesc.raygeneration.offset = shadertable_raygen.offset;
	dispatchraysdesc.raygeneration.size = shaderIdentifierSize;

	dispatchraysdesc.miss.buffer = shadertable_miss.buffer;
	dispatchraysdesc.miss.offset = shadertable_miss.offset;
	dispatchraysdesc.miss.size = shaderIdentifierSize;
	dispatchraysdesc.miss.stride = shaderIdentifierSize;

	dispatchraysdesc.hitgroup.buffer = shadertable_hitgroup.buffer;
	dispatchraysdesc.hitgroup.offset = shadertable_hitgroup.offset;
	dispatchraysdesc.hitgroup.size = shaderIdentifierSize;
	dispatchraysdesc.hitgroup.stride = shaderIdentifierSize;

	dispatchraysdesc.Width = temp.desc.Width;
	dispatchraysdesc.Height = temp.desc.Height;

	device->DispatchRays(&dispatchraysdesc, cmd);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	int temporal_output = device->GetFrameCount() % 2;
	int temporal_history = 1 - temporal_output;

	// Temporal pass:
	{
		device->EventBegin("Temporal pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_TEMPORAL], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &temp, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &texture_temporal[temporal_history], TEXSLOT_ONDEMAND1, cmd);

		const GPUResource* uavs[] = {
			&texture_temporal[temporal_output],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_temporal[temporal_output].GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_temporal[temporal_output].GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Switch to full res
	cb.xPPResolution.x = output.desc.Width;
	cb.xPPResolution.y = output.desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Median blur pass:
	{
		device->EventBegin("Median blur pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_MEDIAN], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &texture_temporal[temporal_output], TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(output.desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(output.desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void Postprocess_SSR(
	const Texture& input,
	const Texture& depthbuffer,
	const Texture& lineardepth,
	const Texture gbuffer[GBUFFER_COUNT],
	const Texture& output,
	CommandList cmd
)
{
	GraphicsDevice* device = GetDevice();

	device->EventBegin("Postprocess_SSR", cmd);
	auto range = wiProfiler::BeginRangeGPU("SSR", cmd);

	device->UnbindResources(TEXSLOT_RENDERPATH_SSR, 1, cmd);

	const TextureDesc& input_desc = input.GetDesc();
	const TextureDesc& desc = output.GetDesc();

	static TextureDesc initialized_desc;
	static Texture texture_raytrace;
	static Texture texture_mask;
	static Texture texture_resolve;
	static Texture texture_temporal[2];

	// Initialize once
	if (initialized_desc.Width != desc.Width || initialized_desc.Height != desc.Height)
	{
		initialized_desc = desc;

		TextureDesc cast_desc;
		cast_desc.type = TextureDesc::TEXTURE_2D;
		cast_desc.Width = desc.Width / 2;
		cast_desc.Height = desc.Height / 2;
		cast_desc.Format = FORMAT_R16G16B16A16_FLOAT;
		cast_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&cast_desc, nullptr, &texture_raytrace);
		cast_desc.Format = FORMAT_R16G16_FLOAT;
		device->CreateTexture(&cast_desc, nullptr, &texture_mask);

		TextureDesc buffer_desc;
		buffer_desc.type = TextureDesc::TEXTURE_2D;
		buffer_desc.Width = desc.Width;
		buffer_desc.Height = desc.Height;
		buffer_desc.Format = FORMAT_R16G16B16A16_FLOAT;
		buffer_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&buffer_desc, nullptr, &texture_resolve);
		device->CreateTexture(&buffer_desc, nullptr, &texture_temporal[0]);
		device->CreateTexture(&buffer_desc, nullptr, &texture_temporal[1]);
	}

	// Switch to half res
	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width / 2;
	cb.xPPResolution.y = desc.Height / 2;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.ssr_input_maxmip = float(input_desc.MipLevels - 1);
	cb.ssr_input_resolution_max = (float)std::max(input_desc.Width, input_desc.Height);
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Raytrace pass:
	{
		device->EventBegin("Stochastic Raytrace pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_RAYTRACE], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);
		device->BindResource(CS, &gbuffer[GBUFFER_ALBEDO_ROUGHNESS], TEXSLOT_GBUFFER0, cmd);
		device->BindResource(CS, &gbuffer[GBUFFER_NORMAL_VELOCITY], TEXSLOT_GBUFFER1, cmd);
		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&texture_raytrace,
			&texture_mask,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_raytrace.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_raytrace.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Switch to full res
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Resolve pass:
	{
		device->EventBegin("Resolve pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_RESOLVE], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &gbuffer[GBUFFER_NORMAL_VELOCITY], TEXSLOT_GBUFFER1, cmd);
		device->BindResource(CS, &texture_raytrace, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &texture_mask, TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &input, TEXSLOT_ONDEMAND2, cmd);

		const GPUResource* uavs[] = {
			&texture_resolve,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_resolve.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_resolve.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	int temporal_output = device->GetFrameCount() % 2;
	int temporal_history = 1 - temporal_output;

	// Temporal pass:
	{
		device->EventBegin("Temporal pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_TEMPORAL], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &texture_resolve, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &texture_temporal[temporal_history], TEXSLOT_ONDEMAND1, cmd);

		const GPUResource* uavs[] = {
			&texture_temporal[temporal_output],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_temporal[temporal_output].GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_temporal[temporal_output].GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Median blur pass:
	{
		device->EventBegin("Median blur pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_MEDIAN], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &texture_temporal[temporal_output], TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_SSS(
	const Texture& lineardepth,
	const Texture gbuffer[GBUFFER_COUNT],
	const RenderPass& input_output_lightbuffer_diffuse,
	const RenderPass& input_output_temp1,
	const RenderPass& input_output_temp2,
	CommandList cmd,
	float amount
)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("Postprocess_SSS", cmd);
	auto range = wiProfiler::BeginRangeGPU("SSS", cmd);

	device->BindResource(PS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	for (uint32_t stencilref = STENCILREF_SKIN; stencilref <= STENCILREF_SNOW; ++stencilref)
	{
		device->BindStencilRef(stencilref, cmd);

		switch (stencilref)
		{
		case STENCILREF_SKIN:
			device->BindPipelineState(&PSO_sss_skin, cmd);
			break;
		case STENCILREF_SNOW:
			device->BindPipelineState(&PSO_sss_snow, cmd);
			break;
		default:
			assert(0);
			break;
		}

		const RenderPass* rt_read = &input_output_lightbuffer_diffuse;
		const RenderPass* rt_write = &input_output_temp1;

		static int sssPassCount = 6;
		for (int i = 0; i < sssPassCount; ++i)
		{
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);

			if (i == sssPassCount - 1)
			{
				// last pass will write into light buffer, but still use the previous ping-pong result:
				rt_write = &input_output_lightbuffer_diffuse;
			}

			const TextureDesc& desc = rt_write->GetDesc().attachments[0].texture->GetDesc();

			device->RenderPassBegin(rt_write, cmd);

			Viewport vp;
			vp.Width = (float)desc.Width;
			vp.Height = (float)desc.Height;
			device->BindViewports(1, &vp, cmd);


			PostProcessCB cb;
			cb.xPPResolution.x = desc.Width;
			cb.xPPResolution.y = desc.Height;
			cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
			cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
			const float blur_strength = 400.0f * amount;
			if (i % 2 == 0)
			{
				cb.sss_step.x = blur_strength * cb.xPPResolution_rcp.x;
				cb.sss_step.y = 0;
			}
			else
			{
				cb.sss_step.x = 0;
				cb.sss_step.y = blur_strength * cb.xPPResolution_rcp.y;
			}
			device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

			if (rt_read->GetDesc().attachments.size() > 2)
			{
				// resolved input!
				device->BindResource(PS, rt_read->GetDesc().attachments[2].texture, TEXSLOT_ONDEMAND0, cmd);
			}
			else
			{
				device->BindResource(PS, rt_read->GetDesc().attachments[0].texture, TEXSLOT_ONDEMAND0, cmd);
			}

			device->Draw(3, 0, cmd);

			device->RenderPassEnd(cmd);

			if (i == 0)
			{
				// first pass was reading from lightbuffer, so correct here for next pass ping-pong:
				rt_read = &input_output_temp1;
				rt_write = &input_output_temp2;
			}
			else
			{
				// ping-pong between temp render targets:
				std::swap(rt_read, rt_write);
			}
		}
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_LightShafts(
	const Texture& input,
	const Texture& output,
	CommandList cmd,
	const XMFLOAT2& center
)
{
	device->EventBegin("Postprocess_LightShafts", cmd);
	auto range = wiProfiler::BeginRangeGPU("LightShafts", cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_LIGHTSHAFTS], cmd);

	device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = 0.65f;	// density
	cb.xPPParams0.y = 0.25f;	// weight
	cb.xPPParams0.z = 0.945f;	// decay
	cb.xPPParams0.w = 0.2f;		// exposure
	cb.xPPParams1.x = center.x;
	cb.xPPParams1.y = center.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);


	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_DepthOfField(
	const Texture& input,
	const Texture& output,
	const Texture& lineardepth,
	CommandList cmd,
	float focus,
	float scale,
	float aspect,
	float max_coc
)
{
	device->EventBegin("Postprocess_DepthOfField", cmd);
	auto range = wiProfiler::BeginRangeGPU("Depth of Field", cmd);

	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	const TextureDesc& desc = output.GetDesc();

	static TextureDesc initialized_desc;
	static Texture texture_tilemax_horizontal;
	static Texture texture_tilemin_horizontal;
	static Texture texture_tilemax;
	static Texture texture_tilemin;
	static Texture texture_neighborhoodmax;
	static Texture texture_presort;
	static Texture texture_prefilter;
	static Texture texture_main;
	static Texture texture_postfilter;
	static Texture texture_alpha1;
	static Texture texture_alpha2;
	static GPUBuffer buffer_tile_statistics;
	static GPUBuffer buffer_tiles_earlyexit;
	static GPUBuffer buffer_tiles_cheap;
	static GPUBuffer buffer_tiles_expensive;

	if (initialized_desc.Width != desc.Width || initialized_desc.Height != desc.Height)
	{
		initialized_desc = desc;

		TextureDesc tile_desc;
		tile_desc.type = TextureDesc::TEXTURE_2D;
		tile_desc.Width = (desc.Width + DEPTHOFFIELD_TILESIZE - 1) / DEPTHOFFIELD_TILESIZE;
		tile_desc.Height = (desc.Height + DEPTHOFFIELD_TILESIZE - 1) / DEPTHOFFIELD_TILESIZE;
		tile_desc.Format = FORMAT_R16G16_FLOAT;
		tile_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&tile_desc, nullptr, &texture_tilemax);
		device->CreateTexture(&tile_desc, nullptr, &texture_neighborhoodmax);
		tile_desc.Format = FORMAT_R16_FLOAT;
		device->CreateTexture(&tile_desc, nullptr, &texture_tilemin);

		tile_desc.Height = desc.Height;
		tile_desc.Format = FORMAT_R16G16_FLOAT;
		device->CreateTexture(&tile_desc, nullptr, &texture_tilemax_horizontal);
		tile_desc.Format = FORMAT_R16_FLOAT;
		device->CreateTexture(&tile_desc, nullptr, &texture_tilemin_horizontal);

		TextureDesc presort_desc;
		presort_desc.type = TextureDesc::TEXTURE_2D;
		presort_desc.Width = desc.Width / 2;
		presort_desc.Height = desc.Height / 2;
		presort_desc.Format = FORMAT_R11G11B10_FLOAT;
		presort_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&presort_desc, nullptr, &texture_presort);
		device->CreateTexture(&presort_desc, nullptr, &texture_prefilter);
		device->CreateTexture(&presort_desc, nullptr, &texture_main);
		device->CreateTexture(&presort_desc, nullptr, &texture_postfilter);
		presort_desc.Format = FORMAT_R8_UNORM;
		device->CreateTexture(&presort_desc, nullptr, &texture_alpha1);
		device->CreateTexture(&presort_desc, nullptr, &texture_alpha2);


		GPUBufferDesc bufferdesc;
		bufferdesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;

		bufferdesc.ByteWidth = TILE_STATISTICS_CAPACITY * sizeof(uint);
		bufferdesc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | RESOURCE_MISC_INDIRECT_ARGS;
		device->CreateBuffer(&bufferdesc, nullptr, &buffer_tile_statistics);

		bufferdesc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferdesc.StructureByteStride = sizeof(uint);
		bufferdesc.ByteWidth = tile_desc.Width * tile_desc.Height * bufferdesc.StructureByteStride;
		device->CreateBuffer(&bufferdesc, nullptr, &buffer_tiles_earlyexit);
		device->CreateBuffer(&bufferdesc, nullptr, &buffer_tiles_cheap);
		device->CreateBuffer(&bufferdesc, nullptr, &buffer_tiles_expensive);
	}

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.dof_focus = focus;
	cb.dof_scale = scale;
	cb.dof_aspect = aspect;
	cb.dof_maxcoc = max_coc;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Compute tile max COC (horizontal):
	{
		device->EventBegin("TileMax - Horizontal", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_TILEMAXCOC_HORIZONTAL], cmd);

		const GPUResource* uavs[] = {
			&texture_tilemax_horizontal,
			&texture_tilemin_horizontal,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_tilemax_horizontal.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_tilemax_horizontal.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Compute tile max COC (vertical):
	{
		device->EventBegin("TileMax - Vertical", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_TILEMAXCOC_VERTICAL], cmd);

		const GPUResource* res[] = {
			&texture_tilemax_horizontal,
			&texture_tilemin_horizontal
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		const GPUResource* uavs[] = {
			&texture_tilemax,
			&texture_tilemin,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_tilemax.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_tilemax.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Compute max COC for each tiles' neighborhood
	{
		device->EventBegin("NeighborhoodMax", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_NEIGHBORHOODMAXCOC], cmd);

		const GPUResource* res[] = {
			&texture_tilemax,
			&texture_tilemin
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		const GPUResource* uavs[] = {
			&buffer_tile_statistics,
			&buffer_tiles_earlyexit,
			&buffer_tiles_cheap,
			&buffer_tiles_expensive,
			&texture_neighborhoodmax
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_neighborhoodmax.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_neighborhoodmax.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Kick indirect tile jobs:
	{
		device->EventBegin("Kickjobs", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_KICKJOBS], cmd);

		device->BindResource(CS, &texture_tilemax, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&buffer_tile_statistics,
			&buffer_tiles_earlyexit,
			&buffer_tiles_cheap,
			&buffer_tiles_expensive
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(1, 1, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Buffer(&buffer_tile_statistics, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_INDIRECT_ARGUMENT),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Switch to half res:
	cb.xPPResolution.x = desc.Width / 2;
	cb.xPPResolution.y = desc.Height / 2;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Prepass:
	{
		device->EventBegin("Prepass", cmd);

		const GPUResource* res[] = {
			&input,
			&texture_neighborhoodmax,
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		const GPUResource* uavs[] = {
			&texture_presort,
			&texture_prefilter
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->BindResource(CS, &buffer_tiles_earlyexit, TEXSLOT_ONDEMAND2, cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS_EARLYEXIT], cmd);
		device->DispatchIndirect(&buffer_tile_statistics, INDIRECT_OFFSET_EARLYEXIT, cmd);

		device->BindResource(CS, &buffer_tiles_cheap, TEXSLOT_ONDEMAND2, cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS], cmd);
		device->DispatchIndirect(&buffer_tile_statistics, INDIRECT_OFFSET_CHEAP, cmd);

		device->BindResource(CS, &buffer_tiles_expensive, TEXSLOT_ONDEMAND2, cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS], cmd);
		device->DispatchIndirect(&buffer_tile_statistics, INDIRECT_OFFSET_EXPENSIVE, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Main pass:
	{
		device->EventBegin("Main pass", cmd);

		const GPUResource* res[] = {
			&texture_neighborhoodmax,
			&texture_presort,
			&texture_prefilter,
			&buffer_tiles_earlyexit,
			&buffer_tiles_cheap,
			&buffer_tiles_expensive
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		const GPUResource* uavs[] = {
			&texture_main,
			&texture_alpha1
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN_EARLYEXIT], cmd);
		device->DispatchIndirect(&buffer_tile_statistics, INDIRECT_OFFSET_EARLYEXIT, cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN_CHEAP], cmd);
		device->DispatchIndirect(&buffer_tile_statistics, INDIRECT_OFFSET_CHEAP, cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN], cmd);
		device->DispatchIndirect(&buffer_tile_statistics, INDIRECT_OFFSET_EXPENSIVE, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Post filter:
	{
		device->EventBegin("Post filter", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_POSTFILTER], cmd);

		const GPUResource* res[] = {
			&texture_main,
			&texture_alpha1
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		const GPUResource* uavs[] = {
			&texture_postfilter,
			&texture_alpha2
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_postfilter.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_postfilter.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Switch to full res:
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Upsample pass:
	{
		device->EventBegin("Upsample pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_UPSAMPLE], cmd);

		const GPUResource* res[] = {
			&input,
			&texture_postfilter,
			&texture_alpha2,
			&texture_neighborhoodmax
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_Outline(
	const Texture& input,
	CommandList cmd,
	float threshold,
	float thickness,
	const XMFLOAT4& color
)
{
	device->EventBegin("Postprocess_Outline", cmd);
	auto range = wiProfiler::BeginRangeGPU("Outline", cmd);

	device->BindPipelineState(&PSO_outline, cmd);

	device->BindResource(PS, &input, TEXSLOT_ONDEMAND0, cmd);

	PostProcessCB cb;
	cb.xPPResolution.x = (uint)input.GetDesc().Width;
	cb.xPPResolution.y = (uint)input.GetDesc().Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = threshold;
	cb.xPPParams0.y = thickness;
	cb.xPPParams1.x = color.x;
	cb.xPPParams1.y = color.y;
	cb.xPPParams1.z = color.z;
	cb.xPPParams1.w = color.w;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->Draw(3, 0, cmd);

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_MotionBlur(
	const Texture& input,
	const Texture& velocity,
	const Texture& lineardepth,
	const Texture& output,
	CommandList cmd,
	float strength
)
{
	device->EventBegin("Postprocess_MotionBlur", cmd);
	auto range = wiProfiler::BeginRangeGPU("MotionBlur", cmd);

	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);
	device->BindResource(CS, &velocity, TEXSLOT_GBUFFER1, cmd);

	const TextureDesc& desc = output.GetDesc();

	static TextureDesc initialized_desc;
	static Texture texture_tilemin_horizontal;
	static Texture texture_tilemax_horizontal;
	static Texture texture_tilemax;
	static Texture texture_tilemin;
	static Texture texture_neighborhoodmax;
	static GPUBuffer buffer_tile_statistics;
	static GPUBuffer buffer_tiles_earlyexit;
	static GPUBuffer buffer_tiles_cheap;
	static GPUBuffer buffer_tiles_expensive;

	if (initialized_desc.Width != desc.Width || initialized_desc.Height != desc.Height)
	{
		initialized_desc = desc;

		TextureDesc tile_desc;
		tile_desc.type = TextureDesc::TEXTURE_2D;
		tile_desc.Width = (desc.Width + MOTIONBLUR_TILESIZE - 1) / MOTIONBLUR_TILESIZE;
		tile_desc.Height = (desc.Height + MOTIONBLUR_TILESIZE - 1) / MOTIONBLUR_TILESIZE;
		tile_desc.Format = FORMAT_R16G16_FLOAT;
		tile_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&tile_desc, nullptr, &texture_tilemin);
		device->CreateTexture(&tile_desc, nullptr, &texture_tilemax);
		device->CreateTexture(&tile_desc, nullptr, &texture_neighborhoodmax);

		tile_desc.Height = desc.Height;
		device->CreateTexture(&tile_desc, nullptr, &texture_tilemax_horizontal);
		device->CreateTexture(&tile_desc, nullptr, &texture_tilemin_horizontal);


		GPUBufferDesc bufferdesc;
		bufferdesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;

		bufferdesc.ByteWidth = TILE_STATISTICS_CAPACITY * sizeof(uint);
		bufferdesc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | RESOURCE_MISC_INDIRECT_ARGS;
		device->CreateBuffer(&bufferdesc, nullptr, &buffer_tile_statistics);

		bufferdesc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferdesc.StructureByteStride = sizeof(uint);
		bufferdesc.ByteWidth = tile_desc.Width * tile_desc.Height * bufferdesc.StructureByteStride;
		device->CreateBuffer(&bufferdesc, nullptr, &buffer_tiles_earlyexit);
		device->CreateBuffer(&bufferdesc, nullptr, &buffer_tiles_cheap);
		device->CreateBuffer(&bufferdesc, nullptr, &buffer_tiles_expensive);
	}

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.motionblur_strength = strength;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Compute tile max velocities (horizontal):
	{
		device->EventBegin("TileMax - Horizontal", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_TILEMAXVELOCITY_HORIZONTAL], cmd);

		const GPUResource* uavs[] = {
			&texture_tilemax_horizontal,
			&texture_tilemin_horizontal,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_tilemax_horizontal.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_tilemax_horizontal.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Compute tile max velocities (vertical):
	{
		device->EventBegin("TileMax - Vertical", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_TILEMAXVELOCITY_VERTICAL], cmd);

		device->BindResource(CS, &texture_tilemax_horizontal, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&texture_tilemax,
			&texture_tilemin,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_tilemax.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_tilemax.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Compute max velocities for each tiles' neighborhood
	{
		device->EventBegin("NeighborhoodMax", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_NEIGHBORHOODMAXVELOCITY], cmd);

		const GPUResource* res[] = {
			&texture_tilemax,
			&texture_tilemin
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		const GPUResource* uavs[] = {
			&buffer_tile_statistics,
			&buffer_tiles_earlyexit,
			&buffer_tiles_cheap,
			&buffer_tiles_expensive,
			&texture_neighborhoodmax
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_neighborhoodmax.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_neighborhoodmax.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Kick indirect tile jobs:
	{
		device->EventBegin("Kickjobs", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_KICKJOBS], cmd);

		device->BindResource(CS, &texture_tilemax, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&buffer_tile_statistics,
			&buffer_tiles_earlyexit,
			&buffer_tiles_cheap,
			&buffer_tiles_expensive
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(1, 1, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Buffer(&buffer_tile_statistics, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_INDIRECT_ARGUMENT),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Tile jobs:
	{
		device->EventBegin("MotionBlur Jobs", cmd);

		const GPUResource* res[] = {
			&input,
			&texture_neighborhoodmax,
			&buffer_tiles_earlyexit,
			&buffer_tiles_cheap,
			&buffer_tiles_expensive,
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_EARLYEXIT], cmd);
		device->DispatchIndirect(&buffer_tile_statistics, INDIRECT_OFFSET_EARLYEXIT, cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_CHEAP], cmd);
		device->DispatchIndirect(&buffer_tile_statistics, INDIRECT_OFFSET_CHEAP, cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR], cmd);
		device->DispatchIndirect(&buffer_tile_statistics, INDIRECT_OFFSET_EXPENSIVE, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, arraysize(res), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_Bloom(
	const Texture& input,
	const Texture& bloom,
	const Texture& bloom_tmp,
	const Texture& output,
	CommandList cmd,
	float threshold
)
{
	device->EventBegin("Postprocess_Bloom", cmd);
	auto range = wiProfiler::BeginRangeGPU("Bloom", cmd);

	// Separate bright parts of image to bloom texture:
	{
		device->EventBegin("Bloom Separate", cmd);

		const TextureDesc& desc = bloom.GetDesc();

		PostProcessCB cb;
		cb.xPPResolution.x = desc.Width;
		cb.xPPResolution.y = desc.Height;
		cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
		cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
		cb.xPPParams0.x = threshold;
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_BLOOMSEPARATE], cmd);

		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&bloom,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	device->EventBegin("Bloom Mipchain", cmd);
	MIPGEN_OPTIONS mipopt;
	mipopt.gaussian_temp = &bloom_tmp;
	mipopt.wide_gauss = true;
	GenerateMipChain(bloom, wiRenderer::MIPGENFILTER_GAUSSIAN, cmd, mipopt);
	device->EventEnd(cmd);

	// Combine image with bloom
	{
		device->EventBegin("Bloom Combine", cmd);

		const TextureDesc& desc = output.GetDesc();

		PostProcessCB cb;
		cb.xPPResolution.x = desc.Width;
		cb.xPPResolution.y = desc.Height;
		cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
		cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_BLOOMCOMBINE], cmd);

		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &bloom, TEXSLOT_ONDEMAND1, cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_VolumetricClouds(
	const Texture& input,
	const Texture& output,
	const Texture& lightshaftoutput,
	const Texture& lineardepth,
	const Texture& depthbuffer,
	CommandList cmd
)
{
	device->EventBegin("Postprocess_VolumetricClouds", cmd);
	auto range = wiProfiler::BeginRangeGPU("Volumetric Clouds", cmd);

	GPUBarrier memory_barrier = GPUBarrier::Memory();

	const TextureDesc& desc = output.GetDesc();

	static TextureDesc initialized_desc;
	static Texture texture_shapeNoise;
	static Texture texture_detailNoise;
	static Texture texture_curlNoise;
	static Texture texture_weatherMap;

	static Texture texture_cloudRender;
	static Texture texture_cloudPositionShaft;
	static Texture texture_cloudRender_upsample;
	//static Texture texture_cloudRender_temp;
	static Texture texture_reproject[2];

	// Initialize and render once
	if (initialized_desc.Width != desc.Width || initialized_desc.Height != desc.Height)
	{
		initialized_desc = desc;

		// Initialize textures:
		TextureDesc shape_desc;
		shape_desc.type = TextureDesc::TEXTURE_3D;
		shape_desc.Width = 128;
		shape_desc.Height = 128;
		shape_desc.Depth = 32;
		shape_desc.MipLevels = 6;
		shape_desc.Format = FORMAT_R8G8B8A8_UNORM;
		shape_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&shape_desc, nullptr, &texture_shapeNoise);

		for (uint32_t i = 0; i < texture_shapeNoise.GetDesc().MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&texture_shapeNoise, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&texture_shapeNoise, UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}

		TextureDesc detail_desc;
		detail_desc.type = TextureDesc::TEXTURE_3D;
		detail_desc.Width = 32;
		detail_desc.Height = 32;
		detail_desc.Depth = 32;
		detail_desc.MipLevels = 6;
		detail_desc.Format = FORMAT_R8G8B8A8_UNORM;
		detail_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&detail_desc, nullptr, &texture_detailNoise);

		for (uint32_t i = 0; i < texture_detailNoise.GetDesc().MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&texture_detailNoise, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&texture_detailNoise, UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}

		TextureDesc texture_desc;
		texture_desc.type = TextureDesc::TEXTURE_2D;
		texture_desc.Width = 128;
		texture_desc.Height = 128;
		texture_desc.Format = FORMAT_R8G8B8A8_UNORM;
		texture_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&texture_desc, nullptr, &texture_curlNoise);

		texture_desc.Width = 1024;
		texture_desc.Height = 1024;
		texture_desc.Format = FORMAT_R8G8B8A8_UNORM;
		device->CreateTexture(&texture_desc, nullptr, &texture_weatherMap);

		texture_desc.Width = desc.Width / 2;
		texture_desc.Height = desc.Height / 2;
		texture_desc.Format = FORMAT_R16G16B16A16_FLOAT;
		device->CreateTexture(&texture_desc, nullptr, &texture_cloudRender);
		device->CreateTexture(&texture_desc, nullptr, &texture_cloudPositionShaft);

		texture_desc.Width = desc.Width;
		texture_desc.Height = desc.Height;
		texture_desc.Format = FORMAT_R16G16B16A16_FLOAT;
		device->CreateTexture(&texture_desc, nullptr, &texture_cloudRender_upsample);
		//device->CreateTexture(&texture_desc, nullptr, &texture_cloudRender_temp);
		device->CreateTexture(&texture_desc, nullptr, &texture_reproject[0]);
		device->CreateTexture(&texture_desc, nullptr, &texture_reproject[1]);

		// Shape Noise pass:
		{
			device->EventBegin("Shape Noise", cmd);
			device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_SHAPENOISE], cmd);

			const GPUResource* uavs[] = {
				&texture_shapeNoise,
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

			const int threadSize = 8;
			const int noiseThreadXY = static_cast<uint32_t>(std::ceil(texture_shapeNoise.GetDesc().Width / threadSize));
			const int noiseThreadZ = static_cast<uint32_t>(std::ceil(texture_shapeNoise.GetDesc().Depth / threadSize));

			device->Dispatch(noiseThreadXY, noiseThreadXY, noiseThreadZ, cmd);

			device->Barrier(&memory_barrier, 1, cmd);
			device->UnbindUAVs(0, arraysize(uavs), cmd);
			device->EventEnd(cmd);
		}

		// Detail Noise pass:
		{
			device->EventBegin("Detail Noise", cmd);
			device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_DETAILNOISE], cmd);

			const GPUResource* uavs[] = {
				&texture_detailNoise,
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

			const int threadSize = 8;
			const int noiseThreadXYZ = static_cast<uint32_t>(std::ceil(texture_detailNoise.GetDesc().Width / threadSize));

			device->Dispatch(noiseThreadXYZ, noiseThreadXYZ, noiseThreadXYZ, cmd);

			device->Barrier(&memory_barrier, 1, cmd);
			device->UnbindUAVs(0, arraysize(uavs), cmd);
			device->EventEnd(cmd);
		}

		// Generate mip chains for 3D textures:
		GenerateMipChain(texture_shapeNoise, MIPGENFILTER_LINEAR, cmd);
		GenerateMipChain(texture_detailNoise, MIPGENFILTER_LINEAR, cmd);

		// Curl Noise pass:
		{
			device->EventBegin("Curl Map", cmd);
			device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_CURLNOISE], cmd);

			const GPUResource* uavs[] = {
				&texture_curlNoise,
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

			const int threadSize = 16;
			const int curlRes = texture_curlNoise.GetDesc().Width;
			const int curlThread = static_cast<uint32_t>(std::ceil(curlRes / threadSize));

			device->Dispatch(curlThread, curlThread, 1, cmd);

			device->Barrier(&memory_barrier, 1, cmd);
			device->UnbindUAVs(0, arraysize(uavs), cmd);
			device->EventEnd(cmd);
		}

		// Weather Map pass:
		{
			device->EventBegin("Weather Map", cmd);
			device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_WEATHERMAP], cmd);

			const GPUResource* uavs[] = {
				&texture_weatherMap,
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

			const int threadSize = 16;
			const int weatherMapRes = texture_weatherMap.GetDesc().Width;
			const int weatherThread = static_cast<uint32_t>(std::ceil(weatherMapRes / threadSize));

			device->Dispatch(weatherThread, weatherThread, 1, cmd);

			device->Barrier(&memory_barrier, 1, cmd);
			device->UnbindUAVs(0, arraysize(uavs), cmd);
			device->EventEnd(cmd);
		}
	}

	// Switch to half res:
	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width / 2;
	cb.xPPResolution.y = desc.Height / 2;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;

	//const XMFLOAT4& halton = wiMath::GetHaltonSequence((int)device->GetFrameCount());
	//cb.xPPParams0 = halton;

	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Cloud pass:
	{
		device->EventBegin("Volumetric Cloud Rendering", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_RENDER], cmd);

		device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);
		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &texture_shapeNoise, TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &texture_detailNoise, TEXSLOT_ONDEMAND2, cmd);
		device->BindResource(CS, &texture_curlNoise, TEXSLOT_ONDEMAND3, cmd);
		device->BindResource(CS, &texture_weatherMap, TEXSLOT_ONDEMAND4, cmd);

		const GPUResource* uavs[] = {
			&texture_cloudRender,
			&texture_cloudPositionShaft,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_cloudRender.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_cloudRender.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		device->Barrier(&memory_barrier, 1, cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Upsample cloud render from half-res to full-res
	Postprocess_Upsample_Bilateral(texture_cloudRender, lineardepth, texture_cloudRender_upsample, cmd);
	//Postprocess_Blur_Gaussian(texture_cloudRender, texture_cloudRender_temp, texture_cloudRender_upsample, cmd);

	int temporal_output = device->GetFrameCount() % 2;
	int temporal_history = 1 - temporal_output;

	// Switch to full-res:
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Reprojection pass:
	{
		device->EventBegin("Volumetric Cloud Reproject", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_REPROJECT], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &texture_cloudRender_upsample, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &texture_cloudPositionShaft, TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &texture_reproject[temporal_history], TEXSLOT_ONDEMAND2, cmd);

		const GPUResource* uavs[] = {
			&texture_reproject[temporal_output],
			&lightshaftoutput,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(texture_reproject[temporal_output].GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(texture_reproject[temporal_output].GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		device->Barrier(&memory_barrier, 1, cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Final pass:
	{
		device->EventBegin("Volumetric Cloud Final", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_FINAL], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);
		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &texture_reproject[temporal_output], TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &texture_weatherMap, TEXSLOT_ONDEMAND2, cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		device->Barrier(&memory_barrier, 1, cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_FXAA(
	const Texture& input,
	const Texture& output,
	CommandList cmd
)
{
	device->EventBegin("Postprocess_FXAA", cmd);
	auto range = wiProfiler::BeginRangeGPU("FXAA", cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_FXAA], cmd);

	device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);


	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_TemporalAA(
	const Texture& input_current,
	const Texture& input_history,
	const Texture& velocity,
	const Texture& lineardepth,
	const Texture& depth_history,
	const Texture& output,
	CommandList cmd
)
{
	device->EventBegin("Postprocess_TemporalAA", cmd);
	auto range = wiProfiler::BeginRangeGPU("Temporal AA Resolve", cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_TEMPORALAA], cmd);

	device->BindResource(CS, &input_current, TEXSLOT_ONDEMAND0, cmd);
	device->BindResource(CS, &input_history, TEXSLOT_ONDEMAND1, cmd);
	device->BindResource(CS, &depth_history, TEXSLOT_ONDEMAND2, cmd);
	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);
	device->BindResource(CS, &velocity, TEXSLOT_GBUFFER1, cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);


	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_Lineardepth(
	const Texture& input,
	const Texture& output,
	CommandList cmd
)
{
	device->EventBegin("Postprocess_Lineardepth", cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width / 2;  // downsample res
	cb.xPPResolution.y = desc.Height / 2; // downsample res
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = (float)input.GetDesc().Width;
	cb.xPPParams0.y = (float)input.GetDesc().Height;
	cb.xPPParams0.z = 1.0f / cb.xPPParams0.x;
	cb.xPPParams0.w = 1.0f / cb.xPPParams0.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);

	device->BindUAV(CS, &output, 0, cmd, 0);
	device->BindUAV(CS, &output, 1, cmd, 1);
	device->BindUAV(CS, &output, 2, cmd, 2);
	device->BindUAV(CS, &output, 3, cmd, 3);
	device->BindUAV(CS, &output, 4, cmd, 4);
	device->BindUAV(CS, &output, 5, cmd, 5);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_LINEARDEPTH], cmd);
	device->Dispatch(
		(desc.Width + POSTPROCESS_LINEARDEPTH_BLOCKSIZE - 1) / POSTPROCESS_LINEARDEPTH_BLOCKSIZE,
		(desc.Height + POSTPROCESS_LINEARDEPTH_BLOCKSIZE - 1) / POSTPROCESS_LINEARDEPTH_BLOCKSIZE,
		1,
		cmd
	);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, 6, cmd);

	device->EventEnd(cmd);
}
void Postprocess_Sharpen(
	const Texture& input,
	const Texture& output,
	CommandList cmd,
	float amount
)
{
	device->EventBegin("Postprocess_Sharpen", cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SHARPEN], cmd);

	device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = amount;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);


	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	device->EventEnd(cmd);
}
void Postprocess_Tonemap(
	const Texture& input,
	const Texture& input_luminance,
	const Texture& input_distortion,
	const Texture& output,
	CommandList cmd,
	float exposure,
	bool dither,
	const Texture* colorgrade_lookuptable
)
{
	device->EventBegin("Postprocess_Tonemap", cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_TONEMAP], cmd);

	device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);
	device->BindResource(CS, &input_luminance, TEXSLOT_ONDEMAND1, cmd);
	device->BindResource(CS, &input_distortion, TEXSLOT_ONDEMAND2, cmd);
	device->BindResource(CS, colorgrade_lookuptable == nullptr ? wiTextureHelper::getColorGradeDefault() : colorgrade_lookuptable, TEXSLOT_ONDEMAND3, cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.tonemap_exposure = exposure;
	cb.tonemap_dither = dither ? 1.0f : 0.0f;
	cb.tonemap_colorgrading = colorgrade_lookuptable == nullptr ? 0.0f : 1.0f;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);


	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	device->EventEnd(cmd);
}
void Postprocess_Chromatic_Aberration(
	const Texture& input,
	const Texture& output,
	CommandList cmd,
	float amount
)
{
	device->EventBegin("Postprocess_Chromatic_Aberration", cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_CHROMATIC_ABERRATION], cmd);

	device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = amount;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);


	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	device->EventEnd(cmd);
}
void Postprocess_Upsample_Bilateral(
	const Texture& input,
	const Texture& lineardepth,
	const Texture& output,
	CommandList cmd,
	bool pixelshader,
	float threshold
)
{
	device->EventBegin("Postprocess_Upsample_Bilateral", cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = threshold;
	cb.xPPParams0.y = 1.0f / (float)input.GetDesc().Width;
	cb.xPPParams0.z = 1.0f / (float)input.GetDesc().Height;
	// select mip from lowres depth mipchain:
	cb.xPPParams0.w = floorf(std::max(1.0f, log2f(std::max((float)desc.Width / (float)input.GetDesc().Width, (float)desc.Height / (float)input.GetDesc().Height))));
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);

	if (pixelshader)
	{
		device->BindPipelineState(&PSO_upsample_bilateral, cmd);

		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

		device->BindResource(PS, &input, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(PS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

		device->Draw(3, 0, cmd);
	}
	else
	{
		SHADERTYPE cs = CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT4;
		switch (desc.Format)
		{
		case FORMAT_R16_UNORM:
		case FORMAT_R8_UNORM:
			cs = CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UNORM1;
			break;
		case FORMAT_R16_FLOAT:
		case FORMAT_R32_FLOAT:
			cs = CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT1;
			break;
		case FORMAT_R16G16B16A16_UNORM:
		case FORMAT_R8G8B8A8_UNORM:
		case FORMAT_B8G8R8A8_UNORM:
		case FORMAT_R10G10B10A2_UNORM:
			cs = CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UNORM4;
			break;
		case FORMAT_R11G11B10_FLOAT:
		case FORMAT_R16G16B16A16_FLOAT:
		case FORMAT_R32G32B32A32_FLOAT:
			cs = CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT4;
			break;
		default:
			assert(0); // implement format!
			break;
		}
		device->BindComputeShader(&shaders[cs], cmd);

		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}

	device->EventEnd(cmd);
}
void Postprocess_Downsample4x(
	const Texture& input,
	const Texture& output,
	CommandList cmd
)
{
	device->EventBegin("Postprocess_Downsample4x", cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DOWNSAMPLE4X], cmd);

	device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);
	device->EventEnd(cmd);
}
void Postprocess_NormalsFromDepth(
	const Texture& depthbuffer,
	const Texture& output,
	CommandList cmd
)
{
	device->EventBegin("Postprocess_NormalsFromDepth", cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_NORMALSFROMDEPTH], cmd);

	device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	GPUBarrier barriers[] = {
		GPUBarrier::Memory(),
	};
	device->Barrier(barriers, arraysize(barriers), cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);
	device->EventEnd(cmd);
}
void Postprocess_Denoise(
	const Texture& input_output_current,
	const Texture& temporal_history,
	const Texture& temporal_current,
	const Texture& velocity,
	const Texture& lineardepth,
	const Texture& depth_history,
	CommandList cmd
)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("Postprocess_Denoise", cmd);
	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DENOISE], cmd);

	const TextureDesc& desc = input_output_current.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->BindResource(CS, &velocity, TEXSLOT_GBUFFER1, cmd);

	{
		device->BindResource(CS, &input_output_current, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &temporal_history, TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &depth_history, TEXSLOT_ONDEMAND2, cmd);
		device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

		const GPUResource* uavs[] = {
			&temporal_current,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}
	{
		device->BindResource(CS, &temporal_current, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &temporal_history, TEXSLOT_ONDEMAND1, cmd);

		const GPUResource* uavs[] = {
			&input_output_current,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}

	device->EventEnd(cmd);
}

const XMFLOAT4& GetWaterPlane()
{
	return waterPlane;
}


RAY GetPickRay(long cursorX, long cursorY) 
{
	float screenW = device->GetScreenWidth();
	float screenH = device->GetScreenHeight();

	const CameraComponent& camera = GetCamera();
	XMMATRIX V = camera.GetView();
	XMMATRIX P = camera.GetProjection();
	XMMATRIX W = XMMatrixIdentity();
	XMVECTOR lineStart = XMVector3Unproject(XMVectorSet((float)cursorX, (float)cursorY, 1, 1), 0, 0, screenW, screenH, 0.0f, 1.0f, P, V, W);
	XMVECTOR lineEnd = XMVector3Unproject(XMVectorSet((float)cursorX, (float)cursorY, 0, 1), 0, 0, screenW, screenH, 0.0f, 1.0f, P, V, W);
	XMVECTOR rayDirection = XMVector3Normalize(XMVectorSubtract(lineEnd, lineStart));
	return RAY(lineStart, rayDirection);
}

void DrawBox(const XMFLOAT4X4& boxMatrix, const XMFLOAT4& color)
{
	renderableBoxes.push_back(std::make_pair(boxMatrix,color));
}
void DrawSphere(const SPHERE& sphere, const XMFLOAT4& color)
{
	renderableSpheres.push_back(std::make_pair(sphere, color));
}
void DrawCapsule(const CAPSULE& capsule, const XMFLOAT4& color)
{
	renderableCapsules.push_back(std::make_pair(capsule, color));
}
void DrawLine(const RenderableLine& line)
{
	renderableLines.push_back(line);
}
void DrawLine(const RenderableLine2D& line)
{
	renderableLines2D.push_back(line);
}
void DrawPoint(const RenderablePoint& point)
{
	renderablePoints.push_back(point);
}
void DrawTriangle(const RenderableTriangle& triangle, bool wireframe)
{
	if (wireframe)
	{
		renderableTriangles_wireframe.push_back(triangle);
	}
	else
	{
		renderableTriangles_solid.push_back(triangle);
	}
}
void DrawPaintRadius(const PaintRadius& paintrad)
{
	paintrads.push_back(paintrad);
}

void AddDeferredMIPGen(std::shared_ptr<wiResource> resource, bool preserve_coverage)
{
	deferredMIPGenLock.lock();
	deferredMIPGens.push_back(std::make_pair(resource, preserve_coverage));
	deferredMIPGenLock.unlock();
}




void SetResolutionScale(float value) 
{ 
	if (RESOLUTIONSCALE != value)
	{
		RESOLUTIONSCALE = value;
		union UserData
		{
			float fValue;
			uint64_t ullValue;
		} data;
		data.fValue = value;
		wiEvent::FireEvent(SYSTEM_EVENT_CHANGE_RESOLUTION_SCALE, data.ullValue);
	}
}
float GetResolutionScale() { return RESOLUTIONSCALE; }
int GetShadowRes2D() { return SHADOWRES_2D; }
int GetShadowResCube() { return SHADOWRES_CUBE; }
void SetTransparentShadowsEnabled(float value) { TRANSPARENTSHADOWSENABLED = value; }
float GetTransparentShadowsEnabled() { return TRANSPARENTSHADOWSENABLED; }
XMUINT2 GetInternalResolution() { return XMUINT2((uint32_t)ceilf(device->GetResolutionWidth()*GetResolutionScale()), (uint32_t)ceilf(device->GetResolutionHeight()*GetResolutionScale())); }
void SetGamma(float value) { GAMMA = value; }
float GetGamma() { return GAMMA; }
void SetWireRender(bool value) { wireRender = value; }
bool IsWireRender() { return wireRender; }
void SetToDrawDebugBoneLines(bool param) { debugBoneLines = param; }
bool GetToDrawDebugBoneLines() { return debugBoneLines; }
void SetToDrawDebugPartitionTree(bool param) { debugPartitionTree = param; }
bool GetToDrawDebugPartitionTree() { return debugPartitionTree; }
bool GetToDrawDebugEnvProbes() { return debugEnvProbes; }
void SetToDrawDebugEnvProbes(bool value) { debugEnvProbes = value; }
void SetToDrawDebugEmitters(bool param) { debugEmitters = param; }
bool GetToDrawDebugEmitters() { return debugEmitters; }
void SetToDrawDebugForceFields(bool param) { debugForceFields = param; }
bool GetToDrawDebugForceFields() { return debugForceFields; }
void SetToDrawDebugCameras(bool param) { debugCameras = param; }
bool GetToDrawDebugCameras() { return debugCameras; }
bool GetToDrawGridHelper() { return gridHelper; }
void SetToDrawGridHelper(bool value) { gridHelper = value; }
bool GetToDrawVoxelHelper() { return voxelHelper; }
void SetToDrawVoxelHelper(bool value) { voxelHelper = value; }
void SetDebugLightCulling(bool enabled) { debugLightCulling = enabled; }
bool GetDebugLightCulling() { return debugLightCulling; }
void SetAdvancedLightCulling(bool enabled) { advancedLightCulling = enabled; }
bool GetAdvancedLightCulling() { return advancedLightCulling; }
void SetVariableRateShadingClassification(bool enabled) { variableRateShadingClassification = enabled; }
bool GetVariableRateShadingClassification() { return variableRateShadingClassification; }
void SetVariableRateShadingClassificationDebug(bool enabled) { variableRateShadingClassificationDebug = enabled; }
bool GetVariableRateShadingClassificationDebug() { return variableRateShadingClassificationDebug; }
void SetOcclusionCullingEnabled(bool value)
{
	static bool initialized = false;

	if (!initialized && value == true)
	{
		initialized = true;

		GPUQueryDesc desc;
		desc.Type = GPU_QUERY_TYPE_OCCLUSION_PREDICATE;

		for (int i = 0; i < arraysize(occlusionQueries); ++i)
		{
			occlusionQueries[i].Create(GetDevice(), &desc);
		}
	}

	occlusionCulling = value;
}
bool GetOcclusionCullingEnabled() { return occlusionCulling; }
void SetLDSSkinningEnabled(bool enabled) { ldsSkinningEnabled = enabled; }
bool GetLDSSkinningEnabled() { return ldsSkinningEnabled; }
void SetTemporalAAEnabled(bool enabled) { temporalAA = enabled; }
bool GetTemporalAAEnabled() { return temporalAA; }
void SetTemporalAADebugEnabled(bool enabled) { temporalAADEBUG = enabled; }
bool GetTemporalAADebugEnabled() { return temporalAADEBUG; }
void SetFreezeCullingCameraEnabled(bool enabled) { freezeCullingCamera = enabled; }
bool GetFreezeCullingCameraEnabled() { return freezeCullingCamera; }
void SetVoxelRadianceEnabled(bool enabled) { voxelSceneData.enabled = enabled; }
bool GetVoxelRadianceEnabled() { return voxelSceneData.enabled; }
void SetVoxelRadianceSecondaryBounceEnabled(bool enabled) { voxelSceneData.secondaryBounceEnabled = enabled; }
bool GetVoxelRadianceSecondaryBounceEnabled() { return voxelSceneData.secondaryBounceEnabled; }
void SetVoxelRadianceReflectionsEnabled(bool enabled) { voxelSceneData.reflectionsEnabled = enabled; }
bool GetVoxelRadianceReflectionsEnabled() { return voxelSceneData.reflectionsEnabled; }
void SetVoxelRadianceVoxelSize(float value) { voxelSceneData.voxelsize = value; }
float GetVoxelRadianceVoxelSize() { return voxelSceneData.voxelsize; }
void SetVoxelRadianceMaxDistance(float value) { voxelSceneData.maxDistance = value; }
float GetVoxelRadianceMaxDistance() { return voxelSceneData.maxDistance; }
int GetVoxelRadianceResolution() { return voxelSceneData.res; }
void SetVoxelRadianceNumCones(int value) { voxelSceneData.numCones = value; }
int GetVoxelRadianceNumCones() { return voxelSceneData.numCones; }
float GetVoxelRadianceRayStepSize() { return voxelSceneData.rayStepSize; }
void SetVoxelRadianceRayStepSize(float value) { voxelSceneData.rayStepSize = value; }
bool IsRequestedReflectionRendering() { return requestReflectionRendering; }
bool IsRequestedVolumetricLightRendering() { return requestVolumetricLightRendering; }
void SetGameSpeed(float value) { GameSpeed = std::max(0.0f, value); }
float GetGameSpeed() { return GameSpeed; }
void OceanRegenerate() { if (ocean != nullptr) ocean = std::make_unique<wiOcean>(GetScene().weather); }
void InvalidateBVH() { scene_bvh_invalid = true; }
void SetRaytraceBounceCount(uint32_t bounces)
{
	raytraceBounceCount = bounces;
}
uint32_t GetRaytraceBounceCount()
{
	return raytraceBounceCount;
}
void SetRaytraceDebugBVHVisualizerEnabled(bool value)
{
	raytraceDebugVisualizer = value;
}
bool GetRaytraceDebugBVHVisualizerEnabled()
{
	return raytraceDebugVisualizer;
}
void SetRaytracedShadowsEnabled(bool value)
{
	raytracedShadows = value;
}
bool GetRaytracedShadowsEnabled()
{
	return raytracedShadows;
}
void SetTessellationEnabled(bool value)
{
	tessellationEnabled = value;
}
bool GetTessellationEnabled()
{
	return tessellationEnabled;
}

}
