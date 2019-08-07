#include "wiRenderer.h"
#include "wiHairParticle.h"
#include "wiEmittedParticle.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiSceneSystem.h"
#include "wiIntersect.h"
#include "wiHelper.h"
#include "wiMath.h"
#include "wiLensFlare.h"
#include "wiTextureHelper.h"
#include "wiCube.h"
#include "wiEnums.h"
#include "wiRandom.h"
#include "wiFont.h"
#include "wiRectPacker.h"
#include "wiBackLog.h"
#include "wiProfiler.h"
#include "wiOcean.h"
#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_CloudGenerator.h"
#include "ShaderInterop_Skinning.h"
#include "ShaderInterop_TracedRendering.h"
#include "ShaderInterop_BVH.h"
#include "ShaderInterop_Utility.h"
#include "wiWidget.h"
#include "wiGPUSortLib.h"
#include "wiAllocators.h"
#include "wiGPUBVH.h"
#include "wiJobSystem.h"
#include "wiSpinLock.h"

#include <algorithm>
#include <unordered_set>
#include <deque>
#include <array>

using namespace std;
using namespace wiGraphics;
using namespace wiSceneSystem;
using namespace wiECS;
using namespace wiAllocators;

namespace wiRenderer
{

GraphicsDevice*			graphicsDevice = nullptr;

const VertexShader*		vertexShaders[VSTYPE_LAST] = {};
const PixelShader*		pixelShaders[PSTYPE_LAST] = {};
const GeometryShader*	geometryShaders[GSTYPE_LAST] = {};
const HullShader*		hullShaders[HSTYPE_LAST] = {};
const DomainShader*		domainShaders[DSTYPE_LAST] = {};
const ComputeShader*	computeShaders[CSTYPE_LAST] = {};
Texture*				textures[TEXTYPE_LAST] = {};

VertexLayout			vertexLayouts[VLTYPE_LAST];
RasterizerState			rasterizers[RSTYPE_LAST];
DepthStencilState		depthStencils[DSSTYPE_LAST];
BlendState				blendStates[BSTYPE_LAST];
GPUBuffer				constantBuffers[CBTYPE_LAST];
GPUBuffer				resourceBuffers[RBTYPE_LAST];
Sampler					samplers[SSLOT_COUNT];

string SHADERPATH = "shaders/";

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
bool ALPHACOMPOSITIONENABLED = false;
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
bool advancedLightCulling = true;
bool advancedRefractions = false;
bool ldsSkinningEnabled = true;
bool scene_bvh_invalid = true;
float SPECULARAA = 0.0f;
float renderTime = 0;
float renderTime_Prev = 0;
float deltaTime = 0;
XMFLOAT2 temporalAAJitter = XMFLOAT2(0, 0);
XMFLOAT2 temporalAAJitterPrev = XMFLOAT2(0, 0);
float RESOLUTIONSCALE = 1.0f;
GPUQueryRing<2> occlusionQueries[256];
UINT entityArrayOffset_Lights = 0;
UINT entityArrayCount_Lights = 0;
UINT entityArrayOffset_Decals = 0;
UINT entityArrayCount_Decals = 0;
UINT entityArrayOffset_ForceFields = 0;
UINT entityArrayCount_ForceFields = 0;
UINT entityArrayOffset_EnvProbes = 0;
UINT entityArrayCount_EnvProbes = 0;
Texture2D* enviroMap = nullptr;
float GameSpeed = 1;
bool debugLightCulling = false;
bool occlusionCulling = false;
bool temporalAA = false;
bool temporalAADEBUG = false;
uint32_t raytraceBounceCount = 2;
bool raytraceDebugVisualizer = false;
Entity cameraTransform = INVALID_ENTITY;


struct VoxelizedSceneData
{
	bool enabled = false;
	UINT res = 128;
	float voxelsize = 1;
	XMFLOAT3 center = XMFLOAT3(0, 0, 0);
	XMFLOAT3 extents = XMFLOAT3(0, 0, 0);
	UINT numCones = 8;
	float rayStepSize = 0.5f;
	bool secondaryBounceEnabled = true;
	bool reflectionsEnabled = true;
	bool centerChangedThisFrame = true;
	UINT mips = 7;
} voxelSceneData;

std::unique_ptr<wiOcean> ocean;

Texture2D shadowMapArray_2D;
Texture2D shadowMapArray_Cube;
Texture2D shadowMapArray_Transparent;

deque<wiSprite*> waterRipples;

std::vector<pair<XMFLOAT4X4, XMFLOAT4>> renderableBoxes;
std::vector<RenderableLine> renderableLines;
std::vector<RenderablePoint> renderablePoints;

XMFLOAT4 waterPlane = XMFLOAT4(0, 1, 0, 0);

wiSpinLock deferredMIPGenLock;
unordered_set<const Texture2D*> deferredMIPGens;

wiGPUBVH sceneBVH;


static const int atlasClampBorder = 1;

static Texture2D decalAtlas;
static unordered_map<const Texture2D*, wiRectPacker::rect_xywh> packedDecals;

Texture2D globalLightmap;
unordered_map<const Texture2D*, wiRectPacker::rect_xywh> packedLightmaps;




void SetDevice(GraphicsDevice* newDevice)
{
	graphicsDevice = newDevice;
}
GraphicsDevice* GetDevice()
{
	return graphicsDevice;
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
		hash |= (uint32_t)(meshIndex & 0x00FFFFFF) << 8;
		hash |= ((uint32_t)(_distance)) & 0xFF;

		instance = (uint32_t)instanceIndex;
		distance = _distance;
	}

	inline uint32_t GetMeshIndex() const
	{
		return (hash >> 8) & 0x00FFFFFF;
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

	void Clear()
	{
		culledObjects.clear();
		culledLights.clear();
		culledDecals.clear();
		culledEnvProbes.clear();
		culledEmitters.clear();
		culledHairs.clear();
	}
};
unordered_map<const CameraComponent*, FrameCulling> frameCullings;

vector<uint32_t> pendingMaterialUpdates;

GFX_STRUCT Instance
{
	XMFLOAT4A mat0;
	XMFLOAT4A mat1;
	XMFLOAT4A mat2;
	XMFLOAT4A color;

	Instance(){}
	Instance(const XMFLOAT4X4& matIn, const XMFLOAT4& colorIn = XMFLOAT4(1, 1, 1, 1), float dither = 0){
		Create(matIn, colorIn, dither);
	}
	inline void Create(const XMFLOAT4X4& matIn, const XMFLOAT4& colorIn = XMFLOAT4(1, 1, 1, 1), float dither = 0) volatile
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

		color.x = colorIn.x;
		color.y = colorIn.y;
		color.z = colorIn.z;
		color.w = colorIn.w;

		color.w *= 1 - dither;
	}

	ALIGN_16
};
GFX_STRUCT InstancePrev
{
	XMFLOAT4A mat0;
	XMFLOAT4A mat1;
	XMFLOAT4A mat2;

	InstancePrev(){}
	InstancePrev(const XMFLOAT4X4& matIn)
	{
		Create(matIn);
	}
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

	ALIGN_16
};
GFX_STRUCT InstanceAtlas
{
	XMFLOAT4A atlasMulAdd;

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

	ALIGN_16
};


const Sampler* GetSampler(int slot)
{
	return &samplers[slot];
}
const VertexShader* GetVertexShader(VSTYPES id)
{
	return vertexShaders[id];
}
const HullShader* GetHullShader(VSTYPES id)
{
	return hullShaders[id];
}
const DomainShader* GetDomainShader(VSTYPES id)
{
	return domainShaders[id];
}
const GeometryShader* GetGeometryShader(VSTYPES id)
{
	return geometryShaders[id];
}
const PixelShader* GetPixelShader(PSTYPES id)
{
	return pixelShaders[id];
}
const ComputeShader* GetComputeShader(VSTYPES id)
{
	return computeShaders[id];
}
const VertexLayout* GetVertexLayout(VLTYPES id)
{
	return &vertexLayouts[id];
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
	return textures[id];
}

void ModifySampler(const SamplerDesc& desc, int slot)
{
	GetDevice()->CreateSamplerState(&desc, &samplers[slot]);
}

std::string& GetShaderPath()
{
	return SHADERPATH;
}
void ReloadShaders(const std::string& path)
{
	if (!path.empty())
	{
		GetShaderPath() = path;
	}

	GetDevice()->WaitForGPU();

	wiResourceManager::GetShaderManager().Clear();
	LoadShaders();
	wiHairParticle::LoadShaders();
	wiEmittedParticle::LoadShaders();
	wiFont::LoadShaders();
	wiImage::LoadShaders();
	wiLensFlare::LoadShaders();
	wiOcean::LoadShaders();
	CSFFT_512x512_Data_t::LoadShaders();
	wiWidget::LoadShaders();
	wiGPUSortLib::LoadShaders();
	wiGPUBVH::LoadShaders();
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
	GetCamera().CreatePerspective((float)GetInternalResolution().x, (float)GetInternalResolution().y, 0.1f, 800);

	frameCullings.insert(make_pair(&GetCamera(), FrameCulling()));
	frameCullings.insert(make_pair(&GetRefCamera(), FrameCulling()));

	SetUpStates();
	LoadBuffers();
	LoadShaders();

	SetShadowProps2D(SHADOWRES_2D, SHADOWCOUNT_2D, SOFTSHADOWQUALITY_2D);
	SetShadowPropsCube(SHADOWRES_CUBE, SHADOWCOUNT_CUBE);

	wiCube::Initialize();

	wiBackLog::post("wiRenderer Initialized");
}
void CleanUp()
{
	wiHairParticle::CleanUp();
	wiEmittedParticle::CleanUp();
	wiCube::CleanUp();

	for (int i = 0; i < TEXTYPE_LAST; ++i)
	{
		SAFE_DELETE(textures[i]);
	}

	SAFE_DELETE(graphicsDevice);
}
void ClearWorld()
{
	GetDevice()->WaitForGPU();

	enviroMap = nullptr;

	for (wiSprite* x : waterRipples)
		x->CleanUp();
	waterRipples.clear();

	GetScene().Clear();

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
enum OBJECTRENDERING_NORMALMAP
{
	OBJECTRENDERING_NORMALMAP_DISABLED,
	OBJECTRENDERING_NORMALMAP_ENABLED,
	OBJECTRENDERING_NORMALMAP_COUNT
};
enum OBJECTRENDERING_PLANARREFLECTION
{
	OBJECTRENDERING_PLANARREFLECTION_DISABLED,
	OBJECTRENDERING_PLANARREFLECTION_ENABLED,
	OBJECTRENDERING_PLANARREFLECTION_COUNT
};
enum OBJECTRENDERING_POM
{
	OBJECTRENDERING_POM_DISABLED,
	OBJECTRENDERING_POM_ENABLED,
	OBJECTRENDERING_POM_COUNT
};
PipelineState PSO_object[RENDERPASS_COUNT][BLENDMODE_COUNT][OBJECTRENDERING_DOUBLESIDED_COUNT][OBJECTRENDERING_TESSELLATION_COUNT][OBJECTRENDERING_ALPHATEST_COUNT][OBJECTRENDERING_NORMALMAP_COUNT][OBJECTRENDERING_PLANARREFLECTION_COUNT][OBJECTRENDERING_POM_COUNT];
PipelineState PSO_object_water[RENDERPASS_COUNT];
PipelineState PSO_object_wire;
inline const PipelineState* GetObjectPSO(RENDERPASS renderPass, bool doublesided, bool tessellation, const MaterialComponent& material, bool forceAlphaTestForDithering)
{
	if (IsWireRender())
	{
		switch (renderPass)
		{
		case RENDERPASS_TEXTURE:
		case RENDERPASS_DEFERRED:
		case RENDERPASS_FORWARD:
		case RENDERPASS_TILEDFORWARD:
			return &PSO_object_wire;
		}
		return nullptr;
	}

	if (material.IsWater())
	{
		return &PSO_object_water[renderPass];
	}

	const bool alphatest = material.IsAlphaTestEnabled() || forceAlphaTestForDithering;
	const bool normalmap = material.GetNormalMap() != nullptr;
	const bool planarreflection = material.HasPlanarReflection();
	const bool pom = material.parallaxOcclusionMapping > 0;
	const BLENDMODE blendMode = material.GetBlendMode();

	const PipelineState& pso = PSO_object[renderPass][blendMode][doublesided][tessellation][alphatest][normalmap][planarreflection][pom];
	assert(pso.IsValid());
	return &pso;
}

PipelineState PSO_object_hologram;
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
inline const PipelineState* GetCustomShaderPSO(RENDERPASS renderPass, uint32_t renderTypeFlags, int customShaderID)
{
	if (customShaderID >= 0 && customShaderID < (int)customShaders.size())
	{
		const CustomShader& customShader = customShaders[customShaderID];
		const CustomShader::Pass& customPass = customShader.passes[renderPass];
		if (customPass.renderTypeFlags & renderTypeFlags)
		{
			return customPass.pso;
		}
	}
	return nullptr;
}

VLTYPES GetVLTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest, bool transparent)
{
	VLTYPES realVL = VLTYPE_OBJECT_POS_TEX;

	switch (renderPass)
	{
	case RENDERPASS_TEXTURE:
		if (tessellation)
		{
			realVL = VLTYPE_OBJECT_ALL;
		}
		else
		{
			realVL = VLTYPE_OBJECT_POS_TEX;
		}
		break;
	case RENDERPASS_DEFERRED:
	case RENDERPASS_FORWARD:
	case RENDERPASS_TILEDFORWARD:
	case RENDERPASS_ENVMAPCAPTURE:
		realVL = VLTYPE_OBJECT_ALL;
		break;
	case RENDERPASS_DEPTHONLY:
		if (tessellation)
		{
			realVL = VLTYPE_OBJECT_ALL;
		}
		else
		{
			if (alphatest)
			{
				realVL = VLTYPE_OBJECT_POS_TEX;
			}
			else
			{
				realVL = VLTYPE_OBJECT_POS;
			}
		}
		break;
	case RENDERPASS_SHADOW:
	case RENDERPASS_SHADOWCUBE:
		if (alphatest || transparent)
		{
			realVL = VLTYPE_OBJECT_POS_TEX;
		}
		else
		{
			realVL = VLTYPE_OBJECT_POS;
		}
		break;
	case RENDERPASS_VOXELIZE:
		realVL = VLTYPE_OBJECT_POS_TEX;
		break;
	}

	return realVL;
}
VSTYPES GetVSTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest, bool transparent)
{
	VSTYPES realVS = VSTYPE_OBJECT_SIMPLE;

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
	case RENDERPASS_DEFERRED:
	case RENDERPASS_FORWARD:
	case RENDERPASS_TILEDFORWARD:
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
		break;
	case RENDERPASS_VOXELIZE:
		realVS = VSTYPE_VOXELIZER;
		break;
	}

	return realVS;
}
GSTYPES GetGSTYPE(RENDERPASS renderPass, bool alphatest)
{
	GSTYPES realGS = GSTYPE_NULL;

	switch (renderPass)
	{
	case RENDERPASS_TEXTURE:
		break;
	case RENDERPASS_DEFERRED:
		break;
	case RENDERPASS_FORWARD:
		break;
	case RENDERPASS_TILEDFORWARD:
		break;
	case RENDERPASS_DEPTHONLY:
		break;
	case RENDERPASS_ENVMAPCAPTURE:
		realGS = GSTYPE_ENVMAP;
		break;
	case RENDERPASS_SHADOW:
		break;
	case RENDERPASS_SHADOWCUBE:
		if (alphatest)
		{
			realGS = GSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST;
		}
		else
		{
			realGS = GSTYPE_SHADOWCUBEMAPRENDER;
		}
		break;
	case RENDERPASS_VOXELIZE:
		realGS = GSTYPE_VOXELIZER;
		break;
	}

	return realGS;
}
HSTYPES GetHSTYPE(RENDERPASS renderPass, bool tessellation)
{
	switch (renderPass)
	{
	case RENDERPASS_TEXTURE:
	case RENDERPASS_DEPTHONLY:
	case RENDERPASS_DEFERRED:
	case RENDERPASS_FORWARD:
	case RENDERPASS_TILEDFORWARD:
		return tessellation ? HSTYPE_OBJECT : HSTYPE_NULL;
		break;
	}

	return HSTYPE_NULL;
}
DSTYPES GetDSTYPE(RENDERPASS renderPass, bool tessellation)
{
	switch (renderPass)
	{
	case RENDERPASS_TEXTURE:
	case RENDERPASS_DEPTHONLY:
	case RENDERPASS_DEFERRED:
	case RENDERPASS_FORWARD:
	case RENDERPASS_TILEDFORWARD:
		return tessellation ? DSTYPE_OBJECT : DSTYPE_NULL;
		break;
	}

	return DSTYPE_NULL;
}
PSTYPES GetPSTYPE(RENDERPASS renderPass, bool alphatest, bool transparent, bool normalmap, bool planarreflection, bool pom)
{
	PSTYPES realPS = PSTYPE_OBJECT_SIMPLEST;

	switch (renderPass)
	{
	case RENDERPASS_DEFERRED:
		if (normalmap)
		{
			if (pom)
			{
				realPS = PSTYPE_OBJECT_DEFERRED_NORMALMAP_POM;
			}
			else
			{
				realPS = PSTYPE_OBJECT_DEFERRED_NORMALMAP;
			}
			if (planarreflection)
			{
				realPS = PSTYPE_OBJECT_DEFERRED_NORMALMAP_PLANARREFLECTION;
			}
		}
		else
		{
			if (pom)
			{
				realPS = PSTYPE_OBJECT_DEFERRED_POM;
			}
			else
			{
				realPS = PSTYPE_OBJECT_DEFERRED;
			}
			if (planarreflection)
			{
				realPS = PSTYPE_OBJECT_DEFERRED_PLANARREFLECTION;
			}
		}
		break;
	case RENDERPASS_FORWARD:
		if (transparent)
		{
			if (normalmap)
			{
				if (pom)
				{
					realPS = PSTYPE_OBJECT_FORWARD_TRANSPARENT_NORMALMAP_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_FORWARD_TRANSPARENT_NORMALMAP;
				}
				if (planarreflection)
				{
					realPS = PSTYPE_OBJECT_FORWARD_TRANSPARENT_NORMALMAP_PLANARREFLECTION;
				}
			}
			else
			{
				if (pom)
				{
					realPS = PSTYPE_OBJECT_FORWARD_TRANSPARENT_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_FORWARD_TRANSPARENT;
				}
				if (planarreflection)
				{
					realPS = PSTYPE_OBJECT_FORWARD_TRANSPARENT_PLANARREFLECTION;
				}
			}
		}
		else
		{
			if (normalmap)
			{
				if (pom)
				{
					realPS = PSTYPE_OBJECT_FORWARD_NORMALMAP_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_FORWARD_NORMALMAP;
				}
				if (planarreflection)
				{
					realPS = PSTYPE_OBJECT_FORWARD_NORMALMAP_PLANARREFLECTION;
				}
			}
			else
			{
				if (pom)
				{
					realPS = PSTYPE_OBJECT_FORWARD_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_FORWARD;
				}
				if (planarreflection)
				{
					realPS = PSTYPE_OBJECT_FORWARD_PLANARREFLECTION;
				}
			}
		}
		break;
	case RENDERPASS_TILEDFORWARD:
		if (transparent)
		{
			if (normalmap)
			{
				if (pom)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP;
				}
				if (planarreflection)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP_PLANARREFLECTION;
				}
			}
			else
			{
				if (pom)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT;
				}
				if (planarreflection)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_PLANARREFLECTION;
				}
			}
		}
		else
		{
			if (normalmap)
			{
				if (pom)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP;
				}
				if (planarreflection)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP_PLANARREFLECTION;
				}
			}
			else
			{
				if (pom)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD;
				}
				if (planarreflection)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_PLANARREFLECTION;
				}
			}
		}
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
				realPS = PSTYPE_NULL;
			}
		}
		break;
	case RENDERPASS_SHADOWCUBE:
		if (alphatest)
		{
			realPS = PSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST;
		}
		else
		{
			realPS = PSTYPE_SHADOWCUBEMAPRENDER;
		}
		break;
	case RENDERPASS_ENVMAPCAPTURE:
		realPS = PSTYPE_ENVMAP;
		break;
	case RENDERPASS_DEPTHONLY:
		if (alphatest)
		{
			realPS = PSTYPE_OBJECT_ALPHATESTONLY;
		}
		else
		{
			realPS = PSTYPE_NULL;
		}
		break;
	case RENDERPASS_VOXELIZE:
		realPS = PSTYPE_VOXELIZER;
		break;
	case RENDERPASS_TEXTURE:
		realPS = PSTYPE_OBJECT_TEXTUREONLY;
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
		case RENDERPASS_DEFERRED:
		case RENDERPASS_FORWARD:
		case RENDERPASS_TILEDFORWARD:
			return &PSO_impostor_wire;
		}
		return nullptr;
	}

	return &PSO_impostor[renderPass];
}

PipelineState PSO_deferredlight[LightComponent::LIGHTTYPE_COUNT];
PipelineState PSO_lightvisualizer[LightComponent::LIGHTTYPE_COUNT];
PipelineState PSO_volumetriclight[LightComponent::LIGHTTYPE_COUNT];
PipelineState PSO_enviromentallight;

PipelineState PSO_renderlightmap_indirect;
PipelineState PSO_renderlightmap_direct;

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
	DEBUGRENDERING_EMITTER,
	DEBUGRENDERING_VOXEL,
	DEBUGRENDERING_FORCEFIELD_POINT,
	DEBUGRENDERING_FORCEFIELD_PLANE,
	DEBUGRENDERING_RAYTRACE_BVH,
	DEBUGRENDERING_COUNT
};
PipelineState PSO_debug[DEBUGRENDERING_COUNT];

enum TILEDLIGHTING_TYPE
{
	TILEDLIGHTING_TYPE_FORWARD,
	TILEDLIGHTING_TYPE_DEFERRED,
	TILEDLIGHTING_TYPE_COUNT
};
enum TILEDLIGHTING_CULLING
{
	TILEDLIGHTING_CULLING_BASIC,
	TILEDLIGHTING_CULLING_ADVANCED,
	TILEDLIGHTING_CULLING_COUNT
};
enum TILEDLIGHTING_DEBUG
{
	TILEDLIGHTING_DEBUG_DISABLED,
	TILEDLIGHTING_DEBUG_ENABLED,
	TILEDLIGHTING_DEBUG_COUNT
};
const ComputeShader* tiledLightingCS[TILEDLIGHTING_TYPE_COUNT][TILEDLIGHTING_CULLING_COUNT][TILEDLIGHTING_DEBUG_COUNT] = {};


static const uint32_t CASCADE_COUNT = 3;
struct SHCAM
{
	XMFLOAT4X4 View;
	XMFLOAT4X4 Projection;
	AABB boundingbox;
	Frustum frustum;

	SHCAM() {}
	SHCAM(const XMVECTOR& eyePos, const XMVECTOR& rotation, float nearPlane, float farPlane, float fov) 
	{
		const XMMATRIX rot = XMMatrixRotationQuaternion(rotation);
		const XMVECTOR to = XMVector3TransformNormal(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), rot);
		const XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rot);
		const XMMATRIX V = XMMatrixLookToLH(eyePos, to, up);
		const XMMATRIX P = XMMatrixPerspectiveFovLH(fov, 1, farPlane, nearPlane);
		const XMMATRIX VP = XMMatrixMultiply(V, P);
		XMStoreFloat4x4(&View, V);
		XMStoreFloat4x4(&Projection, P);
		frustum.Create(VP);
	};
	XMMATRIX getVP() const {
		return XMMatrixTranspose(XMLoadFloat4x4(&View)*XMLoadFloat4x4(&Projection));
	}
};
inline void CreateSpotLightShadowCam(const LightComponent& light, SHCAM& shcam)
{
	shcam = SHCAM(XMLoadFloat3(&light.position), XMLoadFloat4(&light.rotation), 0.1f, light.GetRange(), light.fov);
}
inline void CreateDirLightShadowCams(const LightComponent& light, const CameraComponent& camera, std::array<SHCAM, CASCADE_COUNT>& shcams)
{
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
		for (int j = 0; j < ARRAYSIZE(corners); ++j)
		{
			center = XMVectorAdd(center, corners[j]);
		}
		center = center / float(ARRAYSIZE(corners));

		// Compute cascade bounding sphere radius:
		float radius = 0;
		for (int j = 0; j < ARRAYSIZE(corners); ++j)
		{
			radius = std::max(radius, XMVectorGetX(XMVector3Length(XMVectorSubtract(corners[j], center))));
		}

		// Fit AABB onto bounding sphere:
		XMVECTOR vMin = XMVectorAdd(center, XMVectorReplicate(-radius));
		XMVECTOR vMax = XMVectorAdd(center, XMVectorReplicate(radius));

		// Snap cascade to texel grid:
		const XMVECTOR extent = XMVectorSubtract(vMax, vMin);
		const XMVECTOR texelSize = extent / float(wiRenderer::GetShadowRes2D());
		vMin = XMVectorFloor(vMin / texelSize) * texelSize;
		vMax = XMVectorFloor(vMax / texelSize) * texelSize;

		XMFLOAT3 _min;
		XMFLOAT3 _max;
		XMStoreFloat3(&_min, vMin);
		XMStoreFloat3(&_max, vMax);

		// Extrude bounds to avoid early shadow clipping:
		_min.z = std::min(_min.z, -farPlane * 0.5f);
		_max.z = std::max(_max.z, farPlane * 0.5f);

		// Compute bounding box used for culling, conservatively transformed by the light transform:
		shcams[cascade].boundingbox = AABB(_min, _max).transform(XMMatrixInverse(nullptr, lightView));

		const XMMATRIX lightProjection = XMMatrixOrthographicOffCenterLH(_min.x, _max.x, _min.y, _max.y, _max.z, _min.z); // notice reversed Z!

		XMStoreFloat4x4(&shcams[cascade].View, lightView);
		XMStoreFloat4x4(&shcams[cascade].Projection, lightProjection);
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

	if (renderPass == RENDERPASS_FORWARD || renderPass == RENDERPASS_ENVMAPCAPTURE)
	{
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
	}

	if (renderPass == RENDERPASS_FORWARD)
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
	GraphicsDevice* device = GetDevice();

	device->BindConstantBuffer(stage, &constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), cmd);
	device->BindConstantBuffer(stage, &constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), cmd);
	device->BindConstantBuffer(stage, &constantBuffers[CBTYPE_API], CB_GETBINDSLOT(APICB), cmd);
}
void BindShadowmaps(SHADERSTAGE stage, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();

	device->BindResource(stage, &shadowMapArray_2D, TEXSLOT_SHADOWARRAY_2D, cmd);
	device->BindResource(stage, &shadowMapArray_Cube, TEXSLOT_SHADOWARRAY_CUBE, cmd);
	if (GetTransparentShadowsEnabled())
	{
		device->BindResource(stage, &shadowMapArray_Transparent, TEXSLOT_SHADOWARRAY_TRANSPARENT, cmd);
	}
}
void BindEnvironmentTextures(SHADERSTAGE stage, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();

	device->BindResource(stage, textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], TEXSLOT_ENVMAPARRAY, cmd);
	device->BindResource(stage, textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], TEXSLOT_ENVMAPARRAY, cmd);
	device->BindResource(stage, textures[TEXTYPE_3D_VOXELRADIANCE], TEXSLOT_VOXELRADIANCE, cmd);

	if (enviroMap != nullptr)
	{
		device->BindResource(stage, enviroMap, TEXSLOT_GLOBALENVMAP, cmd);
	}
}

void RenderMeshes(const RenderQueue& renderQueue, RENDERPASS renderPass, UINT renderTypeFlags, CommandList cmd, bool tessellation = false)
{
	if (!renderQueue.empty())
	{
		GraphicsDevice* device = GetDevice();
		const Scene& scene = GetScene();

		device->EventBegin("RenderMeshes", cmd);

		tessellation = tessellation && device->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION);
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
				renderPass == RENDERPASS_FORWARD ||
				renderPass == RENDERPASS_DEFERRED ||
				renderPass == RENDERPASS_TILEDFORWARD ||
				renderPass == RENDERPASS_ENVMAPCAPTURE
				);

		// Do we need to bind all textures or just a reduced amount for this pass?
		const bool easyTextureBind =
			renderPass == RENDERPASS_SHADOW ||
			renderPass == RENDERPASS_SHADOWCUBE ||
			renderPass == RENDERPASS_DEPTHONLY;

		// Do we need to compute a light mask for this pass on the CPU?
		const bool forwardLightmaskRequest = 
			renderPass == RENDERPASS_FORWARD ||
			renderPass == RENDERPASS_ENVMAPCAPTURE ||
			renderPass == RENDERPASS_VOXELIZE;


		// Pre-allocate space for all the instances in GPU-buffer:
		const UINT instanceDataSize = advancedVBRequest ? sizeof(InstBuf) : sizeof(Instance);
		const size_t alloc_size = renderQueue.batchCount * instanceDataSize;
		GraphicsDevice::GPUAllocation instances = device->AllocateGPU(alloc_size, cmd);

		// Purpose of InstancedBatch:
		//	The RenderQueue is sorted by meshIndex. There can be multiple instances for a single meshIndex,
		//	and the InstancedBatchArray contains this information. The array size will be the unique mesh count here.
		struct InstancedBatch
		{
			uint32_t meshIndex;
			int instanceCount;
			uint32_t dataOffset;
			int forceAlphatestForDithering;
			AABB aabb;
		};
		InstancedBatch* instancedBatchArray = nullptr;
		int instancedBatchCount = 0;

		size_t prevMeshIndex = ~0;
		for (uint32_t batchID = 0; batchID < renderQueue.batchCount; ++batchID) // Do not break out of this loop!
		{
			const RenderBatch& batch = renderQueue.batchArray[batchID];
			const uint32_t meshIndex = batch.GetMeshIndex();
			const uint32_t instanceIndex = batch.GetInstanceIndex();

			// When we encounter a new mesh inside the global instance array, we begin a new InstancedBatch:
			if (meshIndex != prevMeshIndex)
			{
				prevMeshIndex = meshIndex;
				instancedBatchCount++;
				InstancedBatch* instancedBatch = (InstancedBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(InstancedBatch));
				instancedBatch->meshIndex = meshIndex;
				instancedBatch->instanceCount = 0;
				instancedBatch->dataOffset = instances.offset + batchID * instanceDataSize;
				instancedBatch->forceAlphatestForDithering = 0;
				instancedBatch->aabb = AABB();
				if (instancedBatchArray == nullptr)
				{
					instancedBatchArray = instancedBatch;
				}
			}

			InstancedBatch& current_batch = instancedBatchArray[instancedBatchCount - 1];
			const ObjectComponent& instance = scene.objects[instanceIndex];

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

			if (forwardLightmaskRequest)
			{
				const AABB& instanceAABB = scene.aabb_objects[instanceIndex];
				current_batch.aabb = AABB::Merge(current_batch.aabb, instanceAABB);
			}

			const XMFLOAT4X4& worldMatrix = instance.transform_index >= 0 ? scene.transforms[instance.transform_index].world : IDENTITYMATRIX;

			// Write into actual GPU-buffer:
			if (advancedVBRequest)
			{
				((volatile InstBuf*)instances.data)[batchID].instance.Create(worldMatrix, instance.color, dither);

				const XMFLOAT4X4& prev_worldMatrix = instance.prev_transform_index >= 0 ? scene.prev_transforms[instance.prev_transform_index].world_prev : IDENTITYMATRIX;
				((volatile InstBuf*)instances.data)[batchID].instancePrev.Create(prev_worldMatrix);
				((volatile InstBuf*)instances.data)[batchID].instanceAtlas.Create(instance.globalLightMapMulAdd);
			}
			else
			{
				((volatile Instance*)instances.data)[batchID].Create(worldMatrix, instance.color, dither);
			}

			current_batch.instanceCount++; // next instance in current InstancedBatch
		}


		// Render instanced batches:
		PRIMITIVETOPOLOGY prevTOPOLOGY = TRIANGLELIST;
		for (int instancedBatchID = 0; instancedBatchID < instancedBatchCount; ++instancedBatchID)
		{
			const InstancedBatch& instancedBatch = instancedBatchArray[instancedBatchID];
			const MeshComponent& mesh = scene.meshes[instancedBatch.meshIndex];
			const bool forceAlphaTestForDithering = instancedBatch.forceAlphatestForDithering != 0;

			const float tessF = mesh.GetTessellationFactor();
			const bool tessellatorRequested = tessF > 0 && tessellation;

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

			device->BindIndexBuffer(mesh.indexBuffer.get(), mesh.GetIndexFormat(), 0, cmd);

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

				const PipelineState* pso = nullptr;
				if (material.IsCustomShader())
				{
					pso = GetCustomShaderPSO(renderPass, renderTypeFlags, material.GetCustomShaderID());
				}
				else
				{
					pso = GetObjectPSO(renderPass, mesh.IsDoubleSided(), tessellatorRequested, material, forceAlphaTestForDithering);
				}

				if (pso == nullptr)
				{
					continue;
				}

				bool subsetRenderable = false;

				if (renderTypeFlags & RENDERTYPE_OPAQUE)
				{
					subsetRenderable = subsetRenderable || (!material.IsTransparent() && !material.IsWater());
				}
				if (renderTypeFlags & RENDERTYPE_TRANSPARENT)
				{
					subsetRenderable = subsetRenderable || material.IsTransparent();
				}
				if (renderTypeFlags & RENDERTYPE_WATER)
				{
					subsetRenderable = subsetRenderable || material.IsWater();
				}
				if (renderPass == RENDERPASS_SHADOW || renderPass == RENDERPASS_SHADOWCUBE)
				{
					subsetRenderable = subsetRenderable && material.IsCastingShadow();
				}

				if (!subsetRenderable)
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
						if (renderPass == RENDERPASS_SHADOW && material.IsTransparent())
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

				if (material.IsWater())
				{
					boundVBType = BOUNDVERTEXBUFFERTYPE::POSITION_TEXCOORD;
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
							mesh.streamoutBuffer_POS.get() != nullptr ? mesh.streamoutBuffer_POS.get() : mesh.vertexBuffer_POS.get(),
							instances.buffer
						};
						UINT strides[] = {
							sizeof(MeshComponent::Vertex_POS),
							instanceDataSize
						};
						UINT offsets[] = {
							0,
							instancedBatch.dataOffset
						};
						device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, cmd);
					}
					break;
					case BOUNDVERTEXBUFFERTYPE::POSITION_TEXCOORD:
					{
						const GPUBuffer* vbs[] = {
							mesh.streamoutBuffer_POS.get() != nullptr ? mesh.streamoutBuffer_POS.get() : mesh.vertexBuffer_POS.get(),
							mesh.vertexBuffer_UV0.get(),
							mesh.vertexBuffer_UV1.get(),
							instances.buffer
						};
						UINT strides[] = {
							sizeof(MeshComponent::Vertex_POS),
							sizeof(MeshComponent::Vertex_TEX),
							sizeof(MeshComponent::Vertex_TEX),
							instanceDataSize
						};
						UINT offsets[] = {
							0,
							0,
							0,
							instancedBatch.dataOffset
						};
						device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, cmd);
					}
					break;
					case BOUNDVERTEXBUFFERTYPE::EVERYTHING:
					{
						const GPUBuffer* vbs[] = {
							mesh.streamoutBuffer_POS.get() != nullptr ? mesh.streamoutBuffer_POS.get() : mesh.vertexBuffer_POS.get(),
							mesh.vertexBuffer_UV0.get(),
							mesh.vertexBuffer_UV1.get(),
							mesh.vertexBuffer_ATL.get(),
							mesh.vertexBuffer_COL.get(),
							mesh.vertexBuffer_PRE.get() != nullptr ? mesh.vertexBuffer_PRE.get() : mesh.vertexBuffer_POS.get(),
							instances.buffer
						};
						UINT strides[] = {
							sizeof(MeshComponent::Vertex_POS),
							sizeof(MeshComponent::Vertex_TEX),
							sizeof(MeshComponent::Vertex_TEX),
							sizeof(MeshComponent::Vertex_TEX),
							sizeof(MeshComponent::Vertex_COL),
							sizeof(MeshComponent::Vertex_POS),
							instanceDataSize
						};
						UINT offsets[] = {
							0,
							0,
							0,
							0,
							0,
							0,
							instancedBatch.dataOffset
						};
						device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, cmd);
					}
					break;
					default:
						assert(0);
						break;
					}
				}
				boundVBType_Prev = boundVBType;

				SetAlphaRef(material.alphaRef, cmd);

				device->BindStencilRef(material.GetStencilRef(), cmd);
				device->BindPipelineState(pso, cmd);

				device->BindConstantBuffer(VS, material.constantBuffer.get(), CB_GETBINDSLOT(MaterialCB), cmd);
				device->BindConstantBuffer(PS, material.constantBuffer.get(), CB_GETBINDSLOT(MaterialCB), cmd);

				if (easyTextureBind)
				{
					const GPUResource* res[] = {
						material.GetBaseColorMap(),
					};
					device->BindResources(PS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), cmd);
				}
				else
				{
					const GPUResource* res[] = {
						material.GetBaseColorMap(),
						material.GetNormalMap(),
						material.GetSurfaceMap(),
						material.GetDisplacementMap(),
						material.GetEmissiveMap(),
						material.GetOcclusionMap(),
					};
					device->BindResources(PS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), cmd);
				}

				if (tessellatorRequested)
				{
					const GPUResource* res[] = {
						material.GetDisplacementMap(),
					};
					device->BindResources(DS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), cmd);
					device->BindConstantBuffer(DS, material.constantBuffer.get(), CB_GETBINDSLOT(MaterialCB), cmd);
				}

				device->DrawIndexedInstanced(subset.indexCount, instancedBatch.instanceCount, subset.indexOffset, 0, 0, cmd);
			}
		}

		ResetAlphaRef(cmd);

		GetRenderFrameAllocator(cmd).free(sizeof(InstancedBatch) * instancedBatchCount);

		device->EventEnd(cmd);
	}
}

void RenderImpostors(const CameraComponent& camera, RENDERPASS renderPass, CommandList cmd)
{
	const Scene& scene = GetScene();
	const PipelineState* impostorRequest = GetImpostorPSO(renderPass);

	if (scene.impostors.GetCount() > 0 && impostorRequest != nullptr)
	{
		GraphicsDevice* device = GetDevice();

		device->EventBegin("RenderImpostors", cmd);

		UINT instanceCount = 0;
		for (size_t impostorID = 0; impostorID < scene.impostors.GetCount(); ++impostorID)
		{
			const ImpostorComponent& impostor = scene.impostors[impostorID];
			if (camera.frustum.CheckBox(impostor.aabb))
			{
				instanceCount += (UINT)impostor.instanceMatrices.size();
			}
		}

		if (instanceCount == 0)
		{
			return;
		}

		// Pre-allocate space for all the instances in GPU-buffer:
		const UINT instanceDataSize = sizeof(Instance);
		const size_t alloc_size = instanceCount * instanceDataSize;
		GraphicsDevice::GPUAllocation instances = device->AllocateGPU(alloc_size, cmd);

		UINT drawableInstanceCount = 0;
		for (size_t impostorID = 0; impostorID < scene.impostors.GetCount(); ++impostorID)
		{
			const ImpostorComponent& impostor = scene.impostors[impostorID];
			if (!camera.frustum.CheckBox(impostor.aabb))
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

				((volatile Instance*)instances.data)[drawableInstanceCount].Create(mat, XMFLOAT4((float)impostorID * impostorCaptureAngles * 3, 1, 1, 1), dither);

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

		device->BindResource(VS, instances.buffer, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(PS, textures[TEXTYPE_2D_IMPOSTORARRAY], TEXSLOT_ONDEMAND0, cmd);

		device->Draw(drawableInstanceCount * 6, 0, cmd);

		device->EventEnd(cmd);
	}
}


void LoadShaders()
{
	GraphicsDevice* device = GetDevice();

	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_SUBSETINDEX",	0, MeshComponent::Vertex_POS::FORMAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		};
		vertexShaders[VSTYPE_OBJECT_DEBUG] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectVS_debug.cso", wiResourceManager::VERTEXSHADER));
		device->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShaders[VSTYPE_OBJECT_DEBUG]->code, &vertexLayouts[VLTYPE_OBJECT_DEBUG]);
	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_SUBSETINDEX",	0, MeshComponent::Vertex_POS::FORMAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					0, MeshComponent::Vertex_TEX::FORMAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					1, MeshComponent::Vertex_TEX::FORMAT, 2, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "ATLAS",					0, MeshComponent::Vertex_TEX::FORMAT, 3, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",					0, MeshComponent::Vertex_COL::FORMAT, 4, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "PREVPOS",				0, MeshComponent::Vertex_POS::FORMAT, 5, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, 6, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, 6, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, 6, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCECOLOR",			0, FORMAT_R32G32B32A32_FLOAT, 6, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIXPREV",		0, FORMAT_R32G32B32A32_FLOAT, 6, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIXPREV",		1, FORMAT_R32G32B32A32_FLOAT, 6, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIXPREV",		2, FORMAT_R32G32B32A32_FLOAT, 6, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEATLAS",			0, FORMAT_R32G32B32A32_FLOAT, 6, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		vertexShaders[VSTYPE_OBJECT_COMMON] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectVS_common.cso", wiResourceManager::VERTEXSHADER));
		device->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShaders[VSTYPE_OBJECT_COMMON]->code, &vertexLayouts[VLTYPE_OBJECT_ALL]);

	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_SUBSETINDEX",	0, MeshComponent::Vertex_POS::FORMAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCECOLOR",			0, FORMAT_R32G32B32A32_FLOAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		vertexShaders[VSTYPE_OBJECT_POSITIONSTREAM] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectVS_positionstream.cso", wiResourceManager::VERTEXSHADER));
		device->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShaders[VSTYPE_OBJECT_POSITIONSTREAM]->code, &vertexLayouts[VLTYPE_OBJECT_POS]);

	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_SUBSETINDEX",	0, MeshComponent::Vertex_POS::FORMAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					0, MeshComponent::Vertex_TEX::FORMAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					1, MeshComponent::Vertex_TEX::FORMAT, 2, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, 3, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, 3, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, 3, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCECOLOR",			0, FORMAT_R32G32B32A32_FLOAT, 3, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		vertexShaders[VSTYPE_OBJECT_SIMPLE] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectVS_simple.cso", wiResourceManager::VERTEXSHADER));
		device->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShaders[VSTYPE_OBJECT_SIMPLE]->code, &vertexLayouts[VLTYPE_OBJECT_POS_TEX]);

	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_SUBSETINDEX",	0, MeshComponent::Vertex_POS::FORMAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCECOLOR",			0, FORMAT_R32G32B32A32_FLOAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		vertexShaders[VSTYPE_SHADOW] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "shadowVS.cso", wiResourceManager::VERTEXSHADER));
		device->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShaders[VSTYPE_SHADOW]->code, &vertexLayouts[VLTYPE_SHADOW_POS]);

	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_SUBSETINDEX",	0, MeshComponent::Vertex_POS::FORMAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					0, MeshComponent::Vertex_TEX::FORMAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "UVSET",					1, MeshComponent::Vertex_TEX::FORMAT, 2, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, 3, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, 3, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, 3, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCECOLOR",			0, FORMAT_R32G32B32A32_FLOAT, 3, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		vertexShaders[VSTYPE_SHADOW_ALPHATEST] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "shadowVS_alphatest.cso", wiResourceManager::VERTEXSHADER));
		device->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShaders[VSTYPE_SHADOW_ALPHATEST]->code, &vertexLayouts[VLTYPE_SHADOW_POS_TEX]);


		vertexShaders[VSTYPE_SHADOW_TRANSPARENT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "shadowVS_transparent.cso", wiResourceManager::VERTEXSHADER));

	}

	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION", 0, FORMAT_R32G32B32A32_FLOAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, FORMAT_R32G32B32A32_FLOAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		};
		vertexShaders[VSTYPE_LINE] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "linesVS.cso", wiResourceManager::VERTEXSHADER));
		device->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShaders[VSTYPE_LINE]->code, &vertexLayouts[VLTYPE_LINE]);

	}

	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION", 0, FORMAT_R32G32B32_FLOAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, FORMAT_R32G32_FLOAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, FORMAT_R32G32B32A32_FLOAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		};
		vertexShaders[VSTYPE_TRAIL] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "trailVS.cso", wiResourceManager::VERTEXSHADER));
		device->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShaders[VSTYPE_TRAIL]->code, &vertexLayouts[VLTYPE_TRAIL]);

	}

	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_SUBSETINDEX",	0, MeshComponent::Vertex_POS::FORMAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "ATLAS",						0, MeshComponent::Vertex_TEX::FORMAT, 1, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "INSTANCEMATRIXPREV",			0, FORMAT_R32G32B32A32_FLOAT, 2, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIXPREV",			1, FORMAT_R32G32B32A32_FLOAT, 2, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "INSTANCEMATRIXPREV",			2, FORMAT_R32G32B32A32_FLOAT, 2, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		vertexShaders[VSTYPE_RENDERLIGHTMAP] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "renderlightmapVS.cso", wiResourceManager::VERTEXSHADER));
		device->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShaders[VSTYPE_RENDERLIGHTMAP]->code, &vertexLayouts[VLTYPE_RENDERLIGHTMAP]);

	}

	vertexShaders[VSTYPE_OBJECT_COMMON_TESSELLATION] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectVS_common_tessellation.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_OBJECT_SIMPLE_TESSELLATION] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectVS_simple_tessellation.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_IMPOSTOR] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "impostorVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_DIRLIGHT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "dirLightVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_POINTLIGHT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "pointLightVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_SPOTLIGHT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "spotLightVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_LIGHTVISUALIZER_SPOTLIGHT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "vSpotLightVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_LIGHTVISUALIZER_POINTLIGHT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "vPointLightVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_LIGHTVISUALIZER_SPHERELIGHT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "vSphereLightVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_LIGHTVISUALIZER_DISCLIGHT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "vDiscLightVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_LIGHTVISUALIZER_RECTANGLELIGHT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "vRectangleLightVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_LIGHTVISUALIZER_TUBELIGHT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "vTubeLightVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_DECAL] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "decalVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_ENVMAP] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "envMapVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_ENVMAP_SKY] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "envMap_skyVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_SPHERE] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "sphereVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_CUBE] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "cubeVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_SHADOWCUBEMAPRENDER] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "cubeShadowVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "cubeShadowVS_alphatest.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_SKY] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "skyVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_WATER] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "waterVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_VOXELIZER] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectVS_voxelizer.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_VOXEL] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "voxelVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_FORCEFIELDVISUALIZER_POINT] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "forceFieldPointVisualizerVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_FORCEFIELDVISUALIZER_PLANE] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "forceFieldPlaneVisualizerVS.cso", wiResourceManager::VERTEXSHADER));
	vertexShaders[VSTYPE_RAYTRACE_SCREEN] = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "raytrace_screenVS.cso", wiResourceManager::VERTEXSHADER));


	pixelShaders[PSTYPE_OBJECT_DEFERRED] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_deferred.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_DEFERRED_NORMALMAP] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_deferred_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_DEFERRED_POM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_deferred_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_DEFERRED_PLANARREFLECTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_deferred_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_DEFERRED_NORMALMAP_POM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_deferred_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_DEFERRED_NORMALMAP_PLANARREFLECTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_deferred_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_IMPOSTOR_DEFERRED] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "impostorPS_deferred.cso", wiResourceManager::PIXELSHADER));

	pixelShaders[PSTYPE_OBJECT_FORWARD] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_NORMALMAP] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_transparent.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT_NORMALMAP] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_transparent_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_PLANARREFLECTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_NORMALMAP_PLANARREFLECTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT_PLANARREFLECTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_transparent_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT_NORMALMAP_PLANARREFLECTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_transparent_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_POM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_NORMALMAP_POM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT_POM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_transparent_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT_NORMALMAP_POM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_transparent_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_WATER] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_forward_water.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_IMPOSTOR_FORWARD] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "impostorPS_forward.cso", wiResourceManager::PIXELSHADER));

	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_transparent.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_transparent_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_PLANARREFLECTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP_PLANARREFLECTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_PLANARREFLECTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_transparent_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP_PLANARREFLECTION] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_transparent_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_POM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP_POM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_POM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_transparent_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP_POM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_transparent_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_WATER] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_tiledforward_water.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_IMPOSTOR_TILEDFORWARD] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "impostorPS_tiledforward.cso", wiResourceManager::PIXELSHADER));

	pixelShaders[PSTYPE_OBJECT_HOLOGRAM] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_hologram.cso", wiResourceManager::PIXELSHADER));


	pixelShaders[PSTYPE_OBJECT_DEBUG] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_debug.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_SIMPLEST] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_simplest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_BLACKOUT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_blackout.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TEXTUREONLY] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_textureonly.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_ALPHATESTONLY] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_alphatestonly.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_IMPOSTOR_ALPHATESTONLY] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "impostorPS_alphatestonly.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_IMPOSTOR_SIMPLE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "impostorPS_simple.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_IMPOSTOR_WIRE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "impostorPS_wire.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVIRONMENTALLIGHT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "environmentalLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DIRLIGHT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "dirLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_POINTLIGHT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "pointLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SPOTLIGHT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "spotLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SPHERELIGHT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "sphereLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DISCLIGHT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "discLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_RECTANGLELIGHT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "rectangleLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_TUBELIGHT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "tubeLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_LIGHTVISUALIZER] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "lightVisualizerPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOLUMETRICLIGHT_DIRECTIONAL] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "volumetricLight_DirectionalPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOLUMETRICLIGHT_POINT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "volumetricLight_PointPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOLUMETRICLIGHT_SPOT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "volumetricLight_SpotPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DECAL] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "decalPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "envMapPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP_SKY_STATIC] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "envMap_skyPS_static.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP_SKY_DYNAMIC] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "envMap_skyPS_dynamic.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_CAPTUREIMPOSTOR_ALBEDO] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "captureImpostorPS_albedo.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_CAPTUREIMPOSTOR_NORMAL] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "captureImpostorPS_normal.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_CAPTUREIMPOSTOR_SURFACE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "captureImpostorPS_surface.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_CUBEMAP] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "cubemapPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_LINE] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "linesPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SKY_STATIC] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "skyPS_static.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SKY_DYNAMIC] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "skyPS_dynamic.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SUN] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "sunPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOW_ALPHATEST] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "shadowPS_alphatest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOW_TRANSPARENT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "shadowPS_transparent.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOW_WATER] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "shadowPS_water.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "cubeShadowPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "cubeShadowPS_alphatest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_TRAIL] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "trailPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOXELIZER] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectPS_voxelizer.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOXEL] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "voxelPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_FORCEFIELDVISUALIZER] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "forceFieldVisualizerPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_RENDERLIGHTMAP_INDIRECT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "renderlightmapPS_indirect.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_RENDERLIGHTMAP_DIRECT] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "renderlightmapPS_direct.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_RAYTRACE_DEBUGBVH] = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "raytrace_debugbvhPS.cso", wiResourceManager::PIXELSHADER));

	geometryShaders[GSTYPE_ENVMAP] = static_cast<const GeometryShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "envMapGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_ENVMAP_SKY] = static_cast<const GeometryShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "envMap_skyGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_SHADOWCUBEMAPRENDER] = static_cast<const GeometryShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "cubeShadowGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST] = static_cast<const GeometryShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "cubeShadowGS_alphatest.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_VOXELIZER] = static_cast<const GeometryShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectGS_voxelizer.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_VOXEL] = static_cast<const GeometryShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "voxelGS.cso", wiResourceManager::GEOMETRYSHADER));


	computeShaders[CSTYPE_LUMINANCE_PASS1] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "luminancePass1CS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_LUMINANCE_PASS2] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "luminancePass2CS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEFRUSTUMS] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "tileFrustumsCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_RESOLVEMSAADEPTHSTENCIL] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "resolveMSAADepthStencilCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELSCENECOPYCLEAR] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "voxelSceneCopyClearCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELSCENECOPYCLEAR_TEMPORALSMOOTHING] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "voxelSceneCopyClear_TemporalSmoothing.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELRADIANCESECONDARYBOUNCE] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "voxelRadianceSecondaryBounceCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELCLEARONLYNORMAL] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "voxelClearOnlyNormalCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN2D_UNORM4_SIMPLEFILTER] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChain2D_unorm4_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN2D_FLOAT4_SIMPLEFILTER] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChain2D_float4_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN2D_UNORM4_GAUSSIAN] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChain2D_unorm4_GaussianCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN2D_FLOAT4_GAUSSIAN] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChain2D_float4_GaussianCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN2D_UNORM4_BICUBIC] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChain2D_unorm4_BicubicCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN2D_FLOAT4_BICUBIC] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChain2D_float4_BicubicCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN3D_UNORM4_SIMPLEFILTER] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChain3D_unorm4_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN3D_FLOAT4_SIMPLEFILTER] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChain3D_float4_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN3D_UNORM4_GAUSSIAN] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChain3D_unorm4_GaussianCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN3D_FLOAT4_GAUSSIAN] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChain3D_float4_GaussianCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAINCUBE_UNORM4_SIMPLEFILTER] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChainCube_unorm4_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAINCUBE_FLOAT4_SIMPLEFILTER] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChainCube_float4_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAINCUBEARRAY_UNORM4_SIMPLEFILTER] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChainCubeArray_unorm4_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAINCUBEARRAY_FLOAT4_SIMPLEFILTER] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "generateMIPChainCubeArray_float4_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_FILTERENVMAP] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "filterEnvMapCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_COPYTEXTURE2D_UNORM4] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "copytexture2D_unorm4CS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_COPYTEXTURE2D_FLOAT4] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "copytexture2D_float4CS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_COPYTEXTURE2D_UNORM4_BORDEREXPAND] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "copytexture2D_unorm4_borderexpandCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_COPYTEXTURE2D_FLOAT4_BORDEREXPAND] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "copytexture2D_float4_borderexpandCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_SKINNING] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "skinningCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_SKINNING_LDS] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "skinningCS_LDS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_CLOUDGENERATOR] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "cloudGeneratorCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_RAYTRACE_LAUNCH] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "raytrace_launchCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_RAYTRACE_KICKJOBS] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "raytrace_kickjobsCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_RAYTRACE_PRIMARY] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "raytrace_primaryCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_RAYTRACE_LIGHTSAMPLING] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "raytrace_lightsamplingCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_RAYTRACE_ACCUMULATE] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "raytrace_accumulateCS.cso", wiResourceManager::COMPUTESHADER));


	hullShaders[HSTYPE_OBJECT] = static_cast<const HullShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectHS.cso", wiResourceManager::HULLSHADER));


	domainShaders[DSTYPE_OBJECT] = static_cast<const DomainShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + "objectDS.cso", wiResourceManager::DOMAINSHADER));

	wiJobSystem::context ctx;

	// default objectshaders:
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
						for (int normalmap = 0; normalmap < OBJECTRENDERING_NORMALMAP_COUNT; ++normalmap)
						{
							for (int planarreflection = 0; planarreflection < OBJECTRENDERING_PLANARREFLECTION_COUNT; ++planarreflection)
							{
								for (int pom = 0; pom < OBJECTRENDERING_POM_COUNT; ++pom)
								{
									wiJobSystem::Execute(ctx, [device, renderPass, blendMode, doublesided, tessellation, alphatest, normalmap, planarreflection,pom] {
										const bool transparency = blendMode != BLENDMODE_OPAQUE;
										VSTYPES realVS = GetVSTYPE((RENDERPASS)renderPass, tessellation, alphatest, transparency);
										VLTYPES realVL = GetVLTYPE((RENDERPASS)renderPass, tessellation, alphatest, transparency);
										HSTYPES realHS = GetHSTYPE((RENDERPASS)renderPass, tessellation);
										DSTYPES realDS = GetDSTYPE((RENDERPASS)renderPass, tessellation);
										GSTYPES realGS = GetGSTYPE((RENDERPASS)renderPass, alphatest);
										PSTYPES realPS = GetPSTYPE((RENDERPASS)renderPass, alphatest, transparency, normalmap, planarreflection, pom);

										if (tessellation && (realHS == HSTYPE_NULL || realDS == DSTYPE_NULL))
										{
											return; // if no job, this must be continue!!
										}

										PipelineStateDesc desc;
										desc.vs = vertexShaders[realVS];
										desc.il = &vertexLayouts[realVL];
										desc.hs = hullShaders[realHS];
										desc.ds = domainShaders[realDS];
										desc.gs = geometryShaders[realGS];
										desc.ps = pixelShaders[realPS];

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
										case RENDERPASS_TILEDFORWARD:
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

										switch (renderPass)
										{
										case RENDERPASS_TEXTURE:
											desc.numRTs = 1;
											desc.RTFormats[0] = RTFormat_hdr;
											desc.DSFormat = DSFormat_full;
											break;
										case RENDERPASS_DEFERRED:
											desc.numRTs = 5;
											desc.RTFormats[0] = RTFormat_gbuffer_0;
											desc.RTFormats[1] = RTFormat_gbuffer_1;
											desc.RTFormats[2] = RTFormat_gbuffer_2;
											desc.RTFormats[3] = RTFormat_deferred_lightbuffer;
											desc.RTFormats[4] = RTFormat_deferred_lightbuffer;
											desc.DSFormat = DSFormat_full;
											break;
										case RENDERPASS_FORWARD:
											if (transparency)
											{
												desc.numRTs = 1;
											}
											else
											{
												desc.numRTs = 2;
											}
											desc.RTFormats[0] = RTFormat_hdr;
											desc.RTFormats[1] = RTFormat_gbuffer_1;
											desc.DSFormat = DSFormat_full;
											break;
										case RENDERPASS_TILEDFORWARD:
											if (transparency)
											{
												desc.numRTs = 1;
											}
											else
											{
												desc.numRTs = 2;
											}
											desc.RTFormats[0] = RTFormat_hdr;
											desc.RTFormats[1] = RTFormat_gbuffer_1;
											desc.DSFormat = DSFormat_full;
											break;
										case RENDERPASS_DEPTHONLY:
											desc.numRTs = 0;
											desc.DSFormat = DSFormat_full;
											break;
										case RENDERPASS_ENVMAPCAPTURE:
											desc.numRTs = 1;
											desc.RTFormats[0] = RTFormat_envprobe;
											desc.DSFormat = DSFormat_small;
											break;
										case RENDERPASS_SHADOW:
											if (transparency)
											{
												desc.numRTs = 1;
												desc.RTFormats[0] = RTFormat_ldr;
											}
											else
											{
												desc.numRTs = 0;
											}
											desc.DSFormat = DSFormat_small;
											break;
										case RENDERPASS_SHADOWCUBE:
											desc.numRTs = 0;
											desc.DSFormat = DSFormat_small;
											break;
										case RENDERPASS_VOXELIZE:
											desc.numRTs = 0;
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

										device->CreatePipelineState(&desc, &PSO_object[renderPass][blendMode][doublesided][tessellation][alphatest][normalmap][planarreflection][pom]);
									});
								}
							}
						}
					}
				}
			}
		}
	}

	// Clear custom shaders (Custom shaders coming from user will need to be handled by the user in case of shader reload):
	customShaders.clear();

	// Hologram sample shader will be registered as custom shader:
	wiJobSystem::Execute(ctx, [device] {
		VSTYPES realVS = GetVSTYPE(RENDERPASS_FORWARD, false, false, true);
		VLTYPES realVL = GetVLTYPE(RENDERPASS_FORWARD, false, false, true);

		PipelineStateDesc desc;
		desc.vs = vertexShaders[realVS];
		desc.il = &vertexLayouts[realVL];
		desc.ps = pixelShaders[PSTYPE_OBJECT_HOLOGRAM];

		desc.bs = &blendStates[BSTYPE_ADDITIVE];
		desc.rs = &rasterizers[DSSTYPE_DEFAULT];
		desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
		desc.pt = TRIANGLELIST;

		desc.numRTs = 1;
		desc.RTFormats[0] = RTFormat_hdr;
		desc.DSFormat = DSFormat_full;

		device->CreatePipelineState(&desc, &PSO_object_hologram);

		CustomShader customShader;
		customShader.name = "Hologram";
		customShader.passes[RENDERPASS_FORWARD].pso = &PSO_object_hologram;
		customShader.passes[RENDERPASS_TILEDFORWARD].pso = &PSO_object_hologram;
		RegisterCustomShader(customShader);
	});


	wiJobSystem::Execute(ctx, [device] {
		PipelineStateDesc desc;
		desc.vs = vertexShaders[VSTYPE_WATER];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_TRANSPARENT];
		desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		desc.il = &vertexLayouts[VLTYPE_OBJECT_POS_TEX];

		desc.numRTs = 1;
		desc.RTFormats[0] = RTFormat_hdr;
		desc.DSFormat = DSFormat_full;

		desc.ps = pixelShaders[PSTYPE_OBJECT_FORWARD_WATER];
		device->CreatePipelineState(&desc, &PSO_object_water[RENDERPASS_FORWARD]);

		desc.ps = pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_WATER];
		device->CreatePipelineState(&desc, &PSO_object_water[RENDERPASS_TILEDFORWARD]);

		desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
		desc.rs = &rasterizers[RSTYPE_SHADOW];
		desc.bs = &blendStates[BSTYPE_TRANSPARENTSHADOWMAP];
		desc.vs = vertexShaders[VSTYPE_SHADOW_TRANSPARENT];
		desc.ps = pixelShaders[PSTYPE_SHADOW_WATER];

		device->CreatePipelineState(&desc, &PSO_object_water[RENDERPASS_SHADOW]);
	});
	wiJobSystem::Execute(ctx, [device] {
		PipelineStateDesc desc;
		desc.vs = vertexShaders[VSTYPE_OBJECT_SIMPLE];
		desc.ps = pixelShaders[PSTYPE_OBJECT_SIMPLEST];
		desc.rs = &rasterizers[RSTYPE_WIRE];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		desc.il = &vertexLayouts[VLTYPE_OBJECT_POS_TEX];

		desc.numRTs = 1;
		desc.RTFormats[0] = RTFormat_hdr;
		desc.DSFormat = DSFormat_full;

		device->CreatePipelineState(&desc, &PSO_object_wire);
	});
	wiJobSystem::Execute(ctx, [device] {
		PipelineStateDesc desc;
		desc.vs = vertexShaders[VSTYPE_DECAL];
		desc.ps = pixelShaders[PSTYPE_DECAL];
		desc.rs = &rasterizers[RSTYPE_FRONT];
		desc.bs = &blendStates[BSTYPE_DECAL];
		desc.dss = &depthStencils[DSSTYPE_DECAL];
		desc.pt = TRIANGLESTRIP;

		desc.numRTs = 1;
		desc.RTFormats[0] = RTFormat_gbuffer_0;
		//desc.RTFormats[1] = RTFormat_gbuffer_1;

		device->CreatePipelineState(&desc, &PSO_decal);
	});
	wiJobSystem::Execute(ctx, [device] {
		PipelineStateDesc desc;
		desc.vs = vertexShaders[VSTYPE_CUBE];
		desc.rs = &rasterizers[RSTYPE_OCCLUDEE];
		desc.bs = &blendStates[BSTYPE_COLORWRITEDISABLE];
		desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
		desc.pt = TRIANGLESTRIP;

		desc.DSFormat = DSFormat_small;

		device->CreatePipelineState(&desc, &PSO_occlusionquery);
	});
	wiJobSystem::Dispatch(ctx, RENDERPASS_COUNT, 1, [device](wiJobDispatchArgs args) {
		const bool impostorRequest =
			args.jobIndex != RENDERPASS_VOXELIZE &&
			args.jobIndex != RENDERPASS_SHADOW &&
			args.jobIndex != RENDERPASS_SHADOWCUBE &&
			args.jobIndex != RENDERPASS_ENVMAPCAPTURE;
		if (!impostorRequest)
		{
			return; // if no job, this must be continue!!
		}

		PipelineStateDesc desc;
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED]; // well, we don't need double sided impostors, but might be helpful if something breaks
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[args.jobIndex == RENDERPASS_TILEDFORWARD ? DSSTYPE_DEPTHREADEQUAL : DSSTYPE_DEFAULT];
		desc.il = nullptr;

		switch (args.jobIndex)
		{
		case RENDERPASS_DEFERRED:
			desc.vs = vertexShaders[VSTYPE_IMPOSTOR];
			desc.ps = pixelShaders[PSTYPE_IMPOSTOR_DEFERRED];
			desc.numRTs = 3;
			desc.RTFormats[0] = RTFormat_gbuffer_0;
			desc.RTFormats[1] = RTFormat_gbuffer_1;
			desc.RTFormats[2] = RTFormat_gbuffer_2;
			break;
		case RENDERPASS_FORWARD:
			desc.vs = vertexShaders[VSTYPE_IMPOSTOR];
			desc.ps = pixelShaders[PSTYPE_IMPOSTOR_FORWARD];
			desc.numRTs = 2;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.RTFormats[1] = RTFormat_gbuffer_1;
			break;
		case RENDERPASS_TILEDFORWARD:
			desc.vs = vertexShaders[VSTYPE_IMPOSTOR];
			desc.ps = pixelShaders[PSTYPE_IMPOSTOR_TILEDFORWARD];
			desc.numRTs = 2;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.RTFormats[1] = RTFormat_gbuffer_1;
			break;
		case RENDERPASS_DEPTHONLY:
			desc.vs = vertexShaders[VSTYPE_IMPOSTOR];
			desc.ps = pixelShaders[PSTYPE_IMPOSTOR_ALPHATESTONLY];
			break;
		default:
			desc.vs = vertexShaders[VSTYPE_IMPOSTOR];
			desc.ps = pixelShaders[PSTYPE_IMPOSTOR_SIMPLE];
			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_hdr;
			break;
		}
		desc.DSFormat = DSFormat_full;

		device->CreatePipelineState(&desc, &PSO_impostor[args.jobIndex]);
	});
	wiJobSystem::Execute(ctx, [device] {
		PipelineStateDesc desc;
		desc.vs = vertexShaders[VSTYPE_IMPOSTOR];
		desc.ps = pixelShaders[PSTYPE_IMPOSTOR_WIRE];
		desc.rs = &rasterizers[RSTYPE_WIRE];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		desc.il = nullptr;

		desc.numRTs = 1;
		desc.RTFormats[0] = RTFormat_hdr;
		desc.DSFormat = DSFormat_full;

		device->CreatePipelineState(&desc, &PSO_impostor_wire);
	});
	wiJobSystem::Execute(ctx, [device] {
		PipelineStateDesc desc;
		desc.vs = vertexShaders[VSTYPE_OBJECT_COMMON];
		desc.rs = &rasterizers[RSTYPE_FRONT];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_CAPTUREIMPOSTOR];
		desc.il = &vertexLayouts[VLTYPE_OBJECT_ALL];

		desc.numRTs = 1;
		desc.RTFormats[0] = RTFormat_impostor;
		desc.DSFormat = DSFormat_small;

		desc.ps = pixelShaders[PSTYPE_CAPTUREIMPOSTOR_ALBEDO];
		device->CreatePipelineState(&desc, &PSO_captureimpostor_albedo);

		desc.ps = pixelShaders[PSTYPE_CAPTUREIMPOSTOR_NORMAL];
		device->CreatePipelineState(&desc, &PSO_captureimpostor_normal);

		desc.ps = pixelShaders[PSTYPE_CAPTUREIMPOSTOR_SURFACE];
		device->CreatePipelineState(&desc, &PSO_captureimpostor_surface);
	});

	wiJobSystem::Dispatch(ctx, LightComponent::LIGHTTYPE_COUNT, 1, [device](wiJobDispatchArgs args) {
		PipelineStateDesc desc;

		// deferred lights:

		desc.pt = TRIANGLELIST;
		desc.rs = &rasterizers[RSTYPE_BACK];
		desc.bs = &blendStates[BSTYPE_DEFERREDLIGHT];

		switch (args.jobIndex)
		{
		case LightComponent::DIRECTIONAL:
			desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
			desc.ps = pixelShaders[PSTYPE_DIRLIGHT];
			desc.dss = &depthStencils[DSSTYPE_DIRLIGHT];
			break;
		case LightComponent::POINT:
			desc.vs = vertexShaders[VSTYPE_POINTLIGHT];
			desc.ps = pixelShaders[PSTYPE_POINTLIGHT];
			desc.dss = &depthStencils[DSSTYPE_LIGHT];
			break;
		case LightComponent::SPOT:
			desc.vs = vertexShaders[VSTYPE_SPOTLIGHT];
			desc.ps = pixelShaders[PSTYPE_SPOTLIGHT];
			desc.dss = &depthStencils[DSSTYPE_LIGHT];
			break;
		case LightComponent::SPHERE:
			desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
			desc.ps = pixelShaders[PSTYPE_SPHERELIGHT];
			desc.dss = &depthStencils[DSSTYPE_DIRLIGHT];
			break;
		case LightComponent::DISC:
			desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
			desc.ps = pixelShaders[PSTYPE_DISCLIGHT];
			desc.dss = &depthStencils[DSSTYPE_DIRLIGHT];
			break;
		case LightComponent::RECTANGLE:
			desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
			desc.ps = pixelShaders[PSTYPE_RECTANGLELIGHT];
			desc.dss = &depthStencils[DSSTYPE_DIRLIGHT];
			break;
		case LightComponent::TUBE:
			desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
			desc.ps = pixelShaders[PSTYPE_TUBELIGHT];
			desc.dss = &depthStencils[DSSTYPE_DIRLIGHT];
			break;
		}

		desc.numRTs = 2;
		desc.RTFormats[0] = RTFormat_deferred_lightbuffer;
		desc.RTFormats[1] = RTFormat_deferred_lightbuffer;
		desc.DSFormat = DSFormat_full;

		device->CreatePipelineState(&desc, &PSO_deferredlight[args.jobIndex]);



		// light visualizers:
		if (args.jobIndex != LightComponent::DIRECTIONAL)
		{

			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.ps = pixelShaders[PSTYPE_LIGHTVISUALIZER];

			switch (args.jobIndex)
			{
			case LightComponent::POINT:
				desc.bs = &blendStates[BSTYPE_ADDITIVE];
				desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_POINTLIGHT];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				break;
			case LightComponent::SPOT:
				desc.bs = &blendStates[BSTYPE_ADDITIVE];
				desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_SPOTLIGHT];
				desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
				break;
			case LightComponent::SPHERE:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_SPHERELIGHT];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				break;
			case LightComponent::DISC:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_DISCLIGHT];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				break;
			case LightComponent::RECTANGLE:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_RECTANGLELIGHT];
				desc.rs = &rasterizers[RSTYPE_BACK];
				break;
			case LightComponent::TUBE:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_TUBELIGHT];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				break;
			}

			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.DSFormat = DSFormat_full;

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
				desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
				desc.ps = pixelShaders[PSTYPE_VOLUMETRICLIGHT_DIRECTIONAL];
				break;
			case LightComponent::POINT:
				desc.vs = vertexShaders[VSTYPE_POINTLIGHT];
				desc.ps = pixelShaders[PSTYPE_VOLUMETRICLIGHT_POINT];
				break;
			case LightComponent::SPOT:
				desc.vs = vertexShaders[VSTYPE_SPOTLIGHT];
				desc.ps = pixelShaders[PSTYPE_VOLUMETRICLIGHT_SPOT];
				break;
			}

			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.DSFormat = FORMAT_UNKNOWN;

			device->CreatePipelineState(&desc, &PSO_volumetriclight[args.jobIndex]);
		}


	});
	wiJobSystem::Execute(ctx, [device] {
		PipelineStateDesc desc;
		desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
		desc.ps = pixelShaders[PSTYPE_ENVIRONMENTALLIGHT];
		desc.rs = &rasterizers[RSTYPE_BACK];
		desc.bs = &blendStates[BSTYPE_ENVIRONMENTALLIGHT];
		desc.dss = &depthStencils[DSSTYPE_DIRLIGHT];

		desc.numRTs = 2;
		desc.RTFormats[0] = RTFormat_deferred_lightbuffer;
		desc.RTFormats[1] = RTFormat_deferred_lightbuffer;
		desc.DSFormat = DSFormat_full;

		device->CreatePipelineState(&desc, &PSO_enviromentallight);
	});
	wiJobSystem::Execute(ctx, [device] {
		PipelineStateDesc desc;
		desc.il = &vertexLayouts[VLTYPE_RENDERLIGHTMAP];
		desc.vs = vertexShaders[VSTYPE_RENDERLIGHTMAP];
		desc.ps = pixelShaders[PSTYPE_RENDERLIGHTMAP_INDIRECT];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_TRANSPARENT];
		desc.dss = &depthStencils[DSSTYPE_XRAY];

		desc.numRTs = 1;
		desc.RTFormats[0] = RTFormat_lightmap_object;
		desc.DSFormat = FORMAT_UNKNOWN;

		device->CreatePipelineState(&desc, &PSO_renderlightmap_indirect);
	});
	wiJobSystem::Execute(ctx, [device] {
		PipelineStateDesc desc;
		desc.il = &vertexLayouts[VLTYPE_RENDERLIGHTMAP];
		desc.vs = vertexShaders[VSTYPE_RENDERLIGHTMAP];
		desc.ps = pixelShaders[PSTYPE_RENDERLIGHTMAP_DIRECT];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = &blendStates[BSTYPE_TRANSPARENT];
		desc.dss = &depthStencils[DSSTYPE_XRAY];

		desc.numRTs = 1;
		desc.RTFormats[0] = RTFormat_lightmap_object;
		desc.DSFormat = FORMAT_UNKNOWN;

		device->CreatePipelineState(&desc, &PSO_renderlightmap_direct);
	});
	wiJobSystem::Dispatch(ctx, SKYRENDERING_COUNT, 1, [device](wiJobDispatchArgs args) {
		PipelineStateDesc desc;
		desc.rs = &rasterizers[RSTYPE_SKY];
		desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];

		switch (args.jobIndex)
		{
		case SKYRENDERING_STATIC:
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.vs = vertexShaders[VSTYPE_SKY];
			desc.ps = pixelShaders[PSTYPE_SKY_STATIC];
			desc.numRTs = 2;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.RTFormats[1] = RTFormat_gbuffer_1;
			desc.DSFormat = DSFormat_full;
			break;
		case SKYRENDERING_DYNAMIC:
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.vs = vertexShaders[VSTYPE_SKY];
			desc.ps = pixelShaders[PSTYPE_SKY_DYNAMIC];
			desc.numRTs = 2;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.RTFormats[1] = RTFormat_gbuffer_1;
			desc.DSFormat = DSFormat_full;
			break;
		case SKYRENDERING_SUN:
			desc.bs = &blendStates[BSTYPE_ADDITIVE];
			desc.vs = vertexShaders[VSTYPE_SKY];
			desc.ps = pixelShaders[PSTYPE_SUN];
			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.DSFormat = DSFormat_full;
			break;
		case SKYRENDERING_ENVMAPCAPTURE_STATIC:
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.vs = vertexShaders[VSTYPE_ENVMAP_SKY];
			desc.ps = pixelShaders[PSTYPE_ENVMAP_SKY_STATIC];
			desc.gs = geometryShaders[GSTYPE_ENVMAP_SKY];
			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.DSFormat = DSFormat_small;
			break;
		case SKYRENDERING_ENVMAPCAPTURE_DYNAMIC:
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.vs = vertexShaders[VSTYPE_ENVMAP_SKY];
			desc.ps = pixelShaders[PSTYPE_ENVMAP_SKY_DYNAMIC];
			desc.gs = geometryShaders[GSTYPE_ENVMAP_SKY];
			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.DSFormat = DSFormat_small;
			break;
		}

		device->CreatePipelineState(&desc, &PSO_sky[args.jobIndex]);
	});
	wiJobSystem::Dispatch(ctx, DEBUGRENDERING_COUNT, 1, [device](wiJobDispatchArgs args) {
		PipelineStateDesc desc;

		desc.numRTs = 1;
		desc.RTFormats[0] = RTFormat_hdr;
		desc.DSFormat = DSFormat_full;

		switch (args.jobIndex)
		{
		case DEBUGRENDERING_ENVPROBE:
			desc.vs = vertexShaders[VSTYPE_SPHERE];
			desc.ps = pixelShaders[PSTYPE_CUBEMAP];
			desc.dss = &depthStencils[DSSTYPE_DEFAULT];
			desc.rs = &rasterizers[RSTYPE_FRONT];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.pt = TRIANGLELIST;
			break;
		case DEBUGRENDERING_GRID:
			desc.vs = vertexShaders[VSTYPE_LINE];
			desc.ps = pixelShaders[PSTYPE_LINE];
			desc.il = &vertexLayouts[VLTYPE_LINE];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = LINELIST;
			break;
		case DEBUGRENDERING_CUBE:
			desc.vs = vertexShaders[VSTYPE_LINE];
			desc.ps = pixelShaders[PSTYPE_LINE];
			desc.il = &vertexLayouts[VLTYPE_LINE];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = LINELIST;
			break;
		case DEBUGRENDERING_LINES:
			desc.vs = vertexShaders[VSTYPE_LINE];
			desc.ps = pixelShaders[PSTYPE_LINE];
			desc.il = &vertexLayouts[VLTYPE_LINE];
			desc.dss = &depthStencils[DSSTYPE_XRAY];
			desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = LINELIST;
			break;
		case DEBUGRENDERING_EMITTER:
			desc.vs = vertexShaders[VSTYPE_OBJECT_DEBUG];
			desc.ps = pixelShaders[PSTYPE_OBJECT_DEBUG];
			desc.il = &vertexLayouts[VLTYPE_OBJECT_DEBUG];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.pt = TRIANGLELIST;
			break;
		case DEBUGRENDERING_VOXEL:
			desc.vs = vertexShaders[VSTYPE_VOXEL];
			desc.ps = pixelShaders[PSTYPE_VOXEL];
			desc.gs = geometryShaders[GSTYPE_VOXEL];
			desc.dss = &depthStencils[DSSTYPE_DEFAULT];
			desc.rs = &rasterizers[RSTYPE_BACK];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.pt = POINTLIST;
			break;
		case DEBUGRENDERING_FORCEFIELD_POINT:
			desc.vs = vertexShaders[VSTYPE_FORCEFIELDVISUALIZER_POINT];
			desc.ps = pixelShaders[PSTYPE_FORCEFIELDVISUALIZER];
			desc.dss = &depthStencils[DSSTYPE_XRAY];
			desc.rs = &rasterizers[RSTYPE_BACK];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = TRIANGLELIST;
			break;
		case DEBUGRENDERING_FORCEFIELD_PLANE:
			desc.vs = vertexShaders[VSTYPE_FORCEFIELDVISUALIZER_PLANE];
			desc.ps = pixelShaders[PSTYPE_FORCEFIELDVISUALIZER];
			desc.dss = &depthStencils[DSSTYPE_XRAY];
			desc.rs = &rasterizers[RSTYPE_FRONT];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = TRIANGLESTRIP;
			break;
		case DEBUGRENDERING_RAYTRACE_BVH:
			desc.vs = vertexShaders[VSTYPE_RAYTRACE_SCREEN];
			desc.ps = pixelShaders[PSTYPE_RAYTRACE_DEBUGBVH];
			desc.dss = &depthStencils[DSSTYPE_XRAY];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.pt = TRIANGLELIST;
			desc.DSFormat = FORMAT_UNKNOWN;
			desc.RTFormats[0] = FORMAT_R32G32B32A32_FLOAT;
			break;
		}

		HRESULT hr = device->CreatePipelineState(&desc, &PSO_debug[args.jobIndex]);
		assert(SUCCEEDED(hr));
	});


	for (int i = 0; i < TILEDLIGHTING_TYPE_COUNT; ++i)
	{
		for (int j = 0; j < TILEDLIGHTING_CULLING_COUNT; ++j)
		{
			for (int k = 0; k < TILEDLIGHTING_DEBUG_COUNT; ++k)
			{
				wiJobSystem::Execute(ctx, [device, i, j, k] {
					string name = "lightCullingCS";
					if (i == TILEDLIGHTING_TYPE_DEFERRED)
					{
						name += "_DEFERRED";
					}
					if (j == TILEDLIGHTING_CULLING_ADVANCED)
					{
						name += "_ADVANCED";
					}
					if (k == TILEDLIGHTING_DEBUG_ENABLED)
					{
						name += "_DEBUG";
					}
					name += ".cso";

					tiledLightingCS[i][j][k] = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(SHADERPATH + name, wiResourceManager::COMPUTESHADER));
				});
			}
		}
	}


	wiJobSystem::Wait(ctx);

}
void LoadBuffers()
{
	GraphicsDevice* device = GetDevice();

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


	bd.ByteWidth = sizeof(ShaderEntityType) * SHADER_ENTITY_COUNT;
	bd.BindFlags = BIND_SHADER_RESOURCE;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(ShaderEntityType);
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

	bd.ByteWidth = sizeof(DecalCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_DECAL]);
	device->SetName(&constantBuffers[CBTYPE_DECAL], "DecalCB");

	bd.ByteWidth = sizeof(CubemapRenderCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_CUBEMAPRENDER]);
	device->SetName(&constantBuffers[CBTYPE_CUBEMAPRENDER], "CubemapRenderCB");

	bd.ByteWidth = sizeof(TessellationCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_TESSELLATION]);
	device->SetName(&constantBuffers[CBTYPE_TESSELLATION], "TessellationCB");

	bd.ByteWidth = sizeof(DispatchParamsCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_DISPATCHPARAMS]);
	device->SetName(&constantBuffers[CBTYPE_DISPATCHPARAMS], "DispatchParamsCB");

	bd.ByteWidth = sizeof(CloudGeneratorCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_CLOUDGENERATOR]);
	device->SetName(&constantBuffers[CBTYPE_CLOUDGENERATOR], "CloudGeneratorCB");

	bd.ByteWidth = sizeof(TracedRenderingCB);
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


}
void SetUpStates()
{
	GraphicsDevice* device = GetDevice();

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
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_LINEAR_MIRROR]);

	samplerDesc.Filter = FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_LINEAR_CLAMP]);

	samplerDesc.Filter = FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_LINEAR_WRAP]);

	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_POINT_MIRROR]);

	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_POINT_WRAP]);


	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_POINT_CLAMP]);

	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MaxAnisotropy = 16;
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_ANISO_CLAMP]);

	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 16;
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_ANISO_WRAP]);

	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MaxAnisotropy = 16;
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_ANISO_MIRROR]);

	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 16;
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_OBJECTSHADER]);

	samplerDesc.Filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = COMPARISON_GREATER_EQUAL;
	device->CreateSamplerState(&samplerDesc, &samplers[SSLOT_CMP_DEPTH]);



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
	dsd.StencilReadMask = 0xFF;
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


	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthEnable = false;
	dsd.DepthFunc = COMPARISON_LESS;
	dsd.StencilEnable = false;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;
	dsd.FrontFace.StencilFunc = COMPARISON_LESS;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_LESS_EQUAL;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DIRLIGHT]);


	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthEnable = true;
	dsd.DepthFunc = COMPARISON_LESS;
	dsd.StencilEnable = false;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;
	dsd.FrontFace.StencilFunc = COMPARISON_LESS_EQUAL;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_LESS_EQUAL;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_LIGHT]);


	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthEnable = true;
	dsd.StencilEnable = true;
	dsd.DepthFunc = COMPARISON_LESS;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0x00;
	dsd.FrontFace.StencilFunc = COMPARISON_EQUAL;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_EQUAL;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DECAL]);


	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthEnable = false;
	dsd.StencilEnable = true;
	dsd.DepthFunc = COMPARISON_GREATER;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0x00;
	dsd.FrontFace.StencilFunc = COMPARISON_EQUAL;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_EQUAL;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	device->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_STENCILREAD_MATCH]);


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
	device->CreateBlendState(&bd, &blendStates[BSTYPE_DEFERREDLIGHT]);
	 
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


void UpdatePerFrameData(float dt, uint32_t layerMask)
{
	renderTime_Prev = renderTime;
	deltaTime = dt * GetGameSpeed();
	renderTime += dt;

	GraphicsDevice* device = GetDevice();
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

	// See which materials will need to update their GPU render data:
	wiJobSystem::Execute(ctx, [&] {
		pendingMaterialUpdates.clear();
		for (size_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			MaterialComponent& material = scene.materials[i];

			if (material.IsDirty())
			{
				material.SetDirty(false);
				pendingMaterialUpdates.push_back(uint32_t(i));

				if (material.constantBuffer == nullptr)
				{
					GPUBufferDesc desc;
					desc.Usage = USAGE_DEFAULT;
					desc.BindFlags = BIND_CONSTANT_BUFFER;
					desc.ByteWidth = sizeof(MaterialCB);

					material.constantBuffer.reset(new GPUBuffer);
					HRESULT hr = device->CreateBuffer(&desc, nullptr, material.constantBuffer.get());
					assert(SUCCEEDED(hr));
				}
			}
		}
	});

	// Need to swap prev and current vertex buffers for any dynamic meshes BEFORE render threads are kicked 
	//	and also create skinning bone buffers:
	wiJobSystem::Execute(ctx, [&] {
		for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
		{
			MeshComponent& mesh = scene.meshes[i];

			if (mesh.IsSkinned() && scene.armatures.Contains(mesh.armatureID))
			{
				ArmatureComponent& armature = *scene.armatures.GetComponent(mesh.armatureID);

				if (armature.boneBuffer == nullptr)
				{
					GPUBufferDesc bd;
					bd.Usage = USAGE_DYNAMIC;
					bd.CPUAccessFlags = CPU_ACCESS_WRITE;

					bd.ByteWidth = sizeof(ArmatureComponent::ShaderBoneType) * (UINT)armature.boneCollection.size();
					bd.BindFlags = BIND_SHADER_RESOURCE;
					bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
					bd.StructureByteStride = sizeof(ArmatureComponent::ShaderBoneType);

					armature.boneBuffer.reset(new GPUBuffer);
					HRESULT hr = device->CreateBuffer(&bd, nullptr, armature.boneBuffer.get());
					assert(SUCCEEDED(hr));
				}
				if (mesh.vertexBuffer_PRE == nullptr)
				{
					mesh.vertexBuffer_PRE.reset(new GPUBuffer);
					HRESULT hr = device->CreateBuffer(&mesh.streamoutBuffer_POS->GetDesc(), nullptr, mesh.vertexBuffer_PRE.get());
					assert(SUCCEEDED(hr));
				}
				mesh.streamoutBuffer_POS.swap(mesh.vertexBuffer_PRE);
			}
		}
		for (size_t i = 0; i < scene.softbodies.GetCount(); ++i)
		{
			Entity entity = scene.softbodies.GetEntity(i);
			MeshComponent& mesh = *scene.meshes.GetComponent(entity);

			if (mesh.vertexBuffer_PRE == nullptr)
			{
				mesh.vertexBuffer_PRE.reset(new GPUBuffer);
				HRESULT hr = device->CreateBuffer(&mesh.vertexBuffer_POS->GetDesc(), nullptr, mesh.vertexBuffer_PRE.get());
				assert(SUCCEEDED(hr));
			}
			mesh.vertexBuffer_POS.swap(mesh.vertexBuffer_PRE);
		}
	});

	// Update Voxelization parameters:
	if (scene.objects.GetCount() > 0)
	{
		wiJobSystem::Execute(ctx, [&] {
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

	// Perform culling and obtain closest reflector:
	requestReflectionRendering = false;
	auto range = wiProfiler::BeginRangeCPU("Frustum Culling");
	{
		for (auto& x : frameCullings)
		{
			const CameraComponent* camera = x.first;
			FrameCulling& culling = x.second;
			culling.Clear();

			if (!freezeCullingCamera)
			{
				culling.frustum = camera->frustum;
			}

			// Cull objects for each camera:
			wiJobSystem::Execute(ctx, [&] {
				for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
				{
					Entity entity = scene.aabb_objects.GetEntity(i);
					const LayerComponent* layer = scene.layers.GetComponent(entity);
					if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
					{
						continue;
					}

					const AABB& aabb = scene.aabb_objects[i];

					if (culling.frustum.CheckBox(aabb))
					{
						culling.culledObjects.push_back((uint32_t)i);

						// Main camera can request reflection rendering:
						if (camera == &GetCamera())
						{
							const ObjectComponent& object = scene.objects[i];
							if (object.IsRequestPlanarReflection())
							{
								requestReflectionRendering = true;
							}
						}
					}
				}
			});

			// the following cullings will be only for the main camera:
			if (camera == &GetCamera())
			{
				wiJobSystem::Execute(ctx, [&] {
					// Cull decals:
					for (size_t i = 0; i < scene.aabb_decals.GetCount(); ++i)
					{
						Entity entity = scene.aabb_decals.GetEntity(i);
						const LayerComponent* layer = scene.layers.GetComponent(entity);
						if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
						{
							continue;
						}

						const AABB& aabb = scene.aabb_decals[i];

						if (culling.frustum.CheckBox(aabb))
						{
							culling.culledDecals.push_back((uint32_t)i);
						}
					}
				});

				wiJobSystem::Execute(ctx, [&] {
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

						if (culling.frustum.CheckBox(aabb))
						{
							culling.culledEnvProbes.push_back((uint32_t)i);
						}
					}
				});

				wiJobSystem::Execute(ctx, [&] {
					// Cull lights:
					for (size_t i = 0; i < scene.aabb_lights.GetCount(); ++i)
					{
						Entity entity = scene.aabb_lights.GetEntity(i);
						const LayerComponent* layer = scene.layers.GetComponent(entity);
						if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
						{
							continue;
						}

						const AABB& aabb = scene.aabb_lights[i];

						if (culling.frustum.CheckBox(aabb))
						{
							culling.culledLights.push_back((uint32_t)i);
						}
					}
				});

				wiJobSystem::Execute(ctx, [&] {
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

				wiJobSystem::Execute(ctx, [&] {
					// Cull hairs:
					for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
					{
						Entity entity = scene.hairs.GetEntity(i);
						const LayerComponent* layer = scene.layers.GetComponent(entity);
						if (layer != nullptr && !(layer->GetLayerMask() & layerMask))
						{
							continue;
						}
						culling.culledHairs.push_back((uint32_t)i);
					}
				});

				wiJobSystem::Wait(ctx);

				// Sort lights based on distance so that closer lights will receive shadow map priority:
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

			}

		}
	}
	wiProfiler::EndRange(range); // Frustum Culling

	// Ocean will override any current reflectors
	waterPlane = scene.waterPlane;
	if (ocean != nullptr)
	{
		requestReflectionRendering = true; 
		XMVECTOR _refPlane = XMPlaneFromPointNormal(XMVectorSet(0, scene.weather.oceanParameters.waterHeight, 0, 0), XMVectorSet(0, 1, 0, 0));
		XMStoreFloat4(&waterPlane, _refPlane);
	}

	if (GetTemporalAAEnabled())
	{
		const XMFLOAT4& halton = wiMath::GetHaltonSequence(GetDevice()->GetFrameCount() % 256);
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

	wiJobSystem::Execute(ctx, [&] {
		ManageDecalAtlas();
	});
	wiJobSystem::Execute(ctx, [&] {
		ManageLightmapAtlas();
	});
	wiJobSystem::Execute(ctx, [&] {
		ManageImpostors();
	});
	wiJobSystem::Execute(ctx, [&] {
		ManageEnvProbes();
	});
	wiJobSystem::Execute(ctx, [&] {
		for (auto& x : waterRipples)
		{
			x->Update(dt * 60);
		}
		ManageWaterRipples();
	});

	wiJobSystem::Wait(ctx);
}
void UpdateRenderData(CommandList cmd)
{
	GraphicsDevice* device = GetDevice();
	const Scene& scene = GetScene();

	BindCommonResources(cmd);

	// Process deferred MIP generation:
	deferredMIPGenLock.lock();
	for (auto& it : deferredMIPGens)
	{
		GenerateMipChain(it, MIPGENFILTER_LINEAR, cmd);
	}
	deferredMIPGens.clear();
	deferredMIPGenLock.unlock();

	// Update material constant buffers:
	MaterialCB materialGPUData;
	for (auto& materialIndex : pendingMaterialUpdates)
	{
		const MaterialComponent& material = scene.materials[materialIndex];

		materialGPUData.g_xMat_baseColor = material.baseColor;
		materialGPUData.g_xMat_emissiveColor = material.emissiveColor;
		materialGPUData.g_xMat_texMulAdd = material.texMulAdd;
		materialGPUData.g_xMat_roughness = material.roughness;
		materialGPUData.g_xMat_reflectance = material.reflectance;
		materialGPUData.g_xMat_metalness = material.metalness;
		materialGPUData.g_xMat_refractionIndex = material.refractionIndex;
		materialGPUData.g_xMat_subsurfaceScattering = material.subsurfaceScattering;
		materialGPUData.g_xMat_normalMapStrength = (material.normalMap == nullptr ? 0 : material.normalMapStrength);
		materialGPUData.g_xMat_normalMapFlip = (material._flags & MaterialComponent::FLIP_NORMALMAP ? -1.0f : 1.0f);
		materialGPUData.g_xMat_parallaxOcclusionMapping = material.parallaxOcclusionMapping;
		materialGPUData.g_xMat_displacementMapping = material.displacementMapping;
		materialGPUData.g_xMat_useVertexColors = material.IsUsingVertexColors() ? 1 : 0;
		materialGPUData.g_xMat_uvset_baseColorMap = material.baseColorMap == nullptr ? -1 : (int)material.uvset_baseColorMap;
		materialGPUData.g_xMat_uvset_surfaceMap = material.surfaceMap == nullptr ? -1 : (int)material.uvset_surfaceMap;
		materialGPUData.g_xMat_uvset_normalMap = material.normalMap == nullptr ? -1 : (int)material.uvset_normalMap;
		materialGPUData.g_xMat_uvset_displacementMap = material.displacementMap == nullptr ? -1 : (int)material.uvset_displacementMap;
		materialGPUData.g_xMat_uvset_emissiveMap = material.emissiveMap == nullptr ? -1 : (int)material.uvset_emissiveMap;
		materialGPUData.g_xMat_uvset_occlusionMap = material.occlusionMap == nullptr ? -1 : (int)material.uvset_occlusionMap;
		materialGPUData.g_xMat_specularGlossinessWorkflow = material.IsUsingSpecularGlossinessWorkflow() ? 1 : 0;
		materialGPUData.g_xMat_occlusion_primary = material.IsOcclusionEnabled_Primary() ? 1 : 0;
		materialGPUData.g_xMat_occlusion_secondary = material.IsOcclusionEnabled_Secondary() ? 1 : 0;

		device->UpdateBuffer(material.constantBuffer.get(), &materialGPUData, cmd);
	}


	const FrameCulling& mainCameraCulling = frameCullings.at(&GetCamera());

	// Fill Entity Array with decals + envprobes + lights in the frustum:
	{
		// Reserve temporary entity array for GPU data upload:
		ShaderEntityType* entityArray = (ShaderEntityType*)GetRenderFrameAllocator(cmd).allocate(sizeof(ShaderEntityType)*SHADER_ENTITY_COUNT);
		XMMATRIX* matrixArray = (XMMATRIX*)GetRenderFrameAllocator(cmd).allocate(sizeof(XMMATRIX)*MATRIXARRAY_COUNT);

		const XMMATRIX viewMatrix = GetCamera().GetView();

		UINT entityCounter = 0;
		UINT matrixCounter = 0;

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
			matrixArray[matrixCounter] = XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&decal.world)));
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
			matrixArray[matrixCounter] = XMMatrixTranspose(XMLoadFloat4x4(&probe.inverseMatrix));
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
					matrixArray[matrixCounter++] = shcams[0].getVP();
					matrixArray[matrixCounter++] = shcams[1].getVP();
					matrixArray[matrixCounter++] = shcams[2].getVP();
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
					matrixArray[matrixCounter++] = shcam.getVP();
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
		device->UpdateBuffer(&resourceBuffers[RBTYPE_ENTITYARRAY], entityArray, cmd, sizeof(ShaderEntityType)*entityCounter);
		device->UpdateBuffer(&resourceBuffers[RBTYPE_MATRIXARRAY], matrixArray, cmd, sizeof(XMMATRIX)*matrixCounter);

		// Temporary array for GPU entities can be freed now:
		GetRenderFrameAllocator(cmd).free(sizeof(ShaderEntityType)*SHADER_ENTITY_COUNT);
		GetRenderFrameAllocator(cmd).free(sizeof(XMMATRIX)*MATRIXARRAY_COUNT);
	}

	UpdateFrameCB(cmd);

	GetPrevCamera() = GetCamera();

	auto range = wiProfiler::BeginRangeGPU("Skinning", cmd);
	device->EventBegin("Skinning", cmd);
	{
		bool streamOutSetUp = false;
		CSTYPES lastCS = CSTYPE_SKINNING_LDS;

		for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
		{
			const MeshComponent& mesh = scene.meshes[i];

			if (mesh.IsSkinned() && scene.armatures.Contains(mesh.armatureID))
			{
				const ArmatureComponent& armature = *scene.armatures.GetComponent(mesh.armatureID);

				if (!streamOutSetUp)
				{
					// Set up skinning shader
					streamOutSetUp = true;
					GPUBuffer* vbs[] = {
						nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
					};
					const UINT strides[] = {
						0,0,0,0,0,0,0,0
					};
					device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, cmd);
					device->BindComputeShader(computeShaders[CSTYPE_SKINNING_LDS], cmd);
				}

				CSTYPES targetCS = CSTYPE_SKINNING_LDS;

				if (!GetLDSSkinningEnabled() || armature.boneData.size() > SKINNING_COMPUTE_THREADCOUNT)
				{
					// If we have more bones that can fit into LDS, we switch to a skinning shader which loads from device memory:
					targetCS = CSTYPE_SKINNING;
				}

				if (targetCS != lastCS)
				{
					lastCS = targetCS;
					device->BindComputeShader(computeShaders[targetCS], cmd);
				}

				// Upload bones for skinning to shader
				device->UpdateBuffer(armature.boneBuffer.get(), armature.boneData.data(), cmd, (int)(sizeof(ArmatureComponent::ShaderBoneType) * armature.boneData.size()));
				device->BindResource(CS, armature.boneBuffer.get(), SKINNINGSLOT_IN_BONEBUFFER, cmd);

				// Do the skinning
				GPUResource* vbs[] = {
					mesh.vertexBuffer_POS.get(),
					mesh.vertexBuffer_BON.get(),
				};
				GPUResource* so[] = {
					mesh.streamoutBuffer_POS.get(),
				};

				device->BindResources(CS, vbs, SKINNINGSLOT_IN_VERTEX_POS, ARRAYSIZE(vbs), cmd);
				device->BindUAVs(CS, so, 0, ARRAYSIZE(so), cmd);

				device->Dispatch((UINT)ceilf((float)mesh.vertex_positions.size() / SKINNING_COMPUTE_THREADCOUNT), 1, 1, cmd);
				device->UAVBarrier(so, ARRAYSIZE(so), cmd); // todo: defer, to gain from async compute
			}

		}

		if (streamOutSetUp)
		{
			device->UnbindUAVs(0, 2, cmd);
			device->UnbindResources(SKINNINGSLOT_IN_VERTEX_POS, 2, cmd);
		}

	}
	device->EventEnd(cmd);
	wiProfiler::EndRange(range); // skinning

	// Update soft body vertex buffers:
	for (size_t i = 0; i < scene.softbodies.GetCount(); ++i)
	{
		Entity entity = scene.softbodies.GetEntity(i);
		const MeshComponent& mesh = *scene.meshes.GetComponent(entity);

		// Copy new simulation data to vertex buffer
		const size_t vb_size = sizeof(MeshComponent::Vertex_POS) * mesh.vertex_positions.size();
		MeshComponent::Vertex_POS* vb = (MeshComponent::Vertex_POS*)GetRenderFrameAllocator(cmd).allocate(vb_size);

		if (mesh.vertex_normals.empty())
		{
			for (size_t ind = 0; ind < mesh.vertex_positions.size(); ++ind)
			{
				vb[ind].FromFULL(mesh.vertex_positions[ind], XMFLOAT3(0, 0, 0), 0); // subsetindex??
			}
		}
		else
		{
			for (size_t ind = 0; ind < mesh.vertex_positions.size(); ++ind)
			{
				vb[ind].FromFULL(mesh.vertex_positions[ind], mesh.vertex_normals[ind], 0); // subsetindex??
			}
		}

		device->UpdateBuffer(mesh.vertexBuffer_POS.get(), vb, cmd, (UINT)vb_size);

		GetRenderFrameAllocator(cmd).free(vb_size);
	}

	// GPU Particle systems simulation/sorting/culling:
	for (uint32_t emitterIndex : mainCameraCulling.culledEmitters)
	{
		const wiEmittedParticle& emitter = scene.emitters[emitterIndex];
		Entity entity = scene.emitters.GetEntity(emitterIndex);
		const TransformComponent& transform = *scene.transforms.GetComponent(entity);
		const MaterialComponent& material = *scene.materials.GetComponent(entity);
		const MeshComponent* mesh = scene.meshes.GetComponent(emitter.meshID);

		emitter.UpdateGPU(transform, material, mesh, cmd);
	}

	// Hair particle systems GPU simulation:
	for (uint32_t hairIndex : mainCameraCulling.culledHairs)
	{
		const wiHairParticle& hair = scene.hairs[hairIndex];

		if (hair.meshID != INVALID_ENTITY && GetCamera().frustum.CheckBox(hair.aabb))
		{
			const MeshComponent* mesh = scene.meshes.GetComponent(hair.meshID);

			if (mesh != nullptr)
			{
				Entity entity = scene.hairs.GetEntity(hairIndex);
				const MaterialComponent& material = *scene.materials.GetComponent(entity);

				hair.UpdateGPU(*mesh, material, cmd);
			}
		}
	}

	// Compute water simulation:
	if (ocean != nullptr)
	{
		ocean->UpdateDisplacementMap(scene.weather, renderTime, cmd);
	}

	// Generate cloud layer:
	if(enviroMap == nullptr && scene.weather.cloudiness > 0) // generate only when sky is dynamic
	{
		if (textures[TEXTYPE_2D_CLOUDS] == nullptr)
		{
			TextureDesc desc;
			desc.ArraySize = 1;
			desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.Format = FORMAT_R8G8B8A8_UNORM;
			desc.Height = 128;
			desc.Width = 128;
			desc.MipLevels = 1;
			desc.MiscFlags = 0;
			desc.Usage = USAGE_DEFAULT;

			textures[TEXTYPE_2D_CLOUDS] = new Texture2D;
			device->CreateTexture2D(&desc, nullptr, (Texture2D*)textures[TEXTYPE_2D_CLOUDS]);
		}

		float cloudPhase = renderTime * scene.weather.cloudSpeed;
		GenerateClouds((Texture2D*)textures[TEXTYPE_2D_CLOUDS], 5, cloudPhase, cmd);
	}

	RefreshDecalAtlas(cmd);
	RefreshLightmapAtlas(cmd);
	RefreshEnvProbes(cmd);
	RefreshImpostors(cmd);
}
void OcclusionCulling_Render(CommandList cmd)
{
	if (!GetOcclusionCullingEnabled() || GetFreezeCullingCameraEnabled())
	{
		return;
	}

	GraphicsDevice* device = GetDevice();
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

			if (queryID >= ARRAYSIZE(occlusionQueries))
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
				XMStoreFloat4x4(&cb.g_xTransform, XMMatrixTranspose(aabb.getAsBoxMatrix()*GetPrevCamera().GetViewProjection())); // todo: obb
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

	GraphicsDevice* device = GetDevice();
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
			while (!device->QueryRead(query, &query_result)) {}

			if (query_result.result_passed == TRUE)
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
	wiSprite* img=new wiSprite("","",image);
	img->anim.fad=0.01f;
	img->anim.scaleX=0.2f;
	img->anim.scaleY=0.2f;
	img->params.pos=pos;
	img->params.rotation=(wiRandom::getRandom(0,1000)*0.001f)*2*3.1415f;
	img->params.siz=XMFLOAT2(1,1);
	img->params.typeFlag=WORLD;
	img->params.quality=QUALITY_ANISOTROPIC;
	img->params.pivot = XMFLOAT2(0.5f, 0.5f);
	img->params.lookAt=waterPlane;
	img->params.lookAt.w=1;
	waterRipples.push_back(img);
}
void ManageWaterRipples(){
	while(	
		!waterRipples.empty() && 
			(waterRipples.front()->params.opacity <= 0 + FLT_EPSILON || waterRipples.front()->params.fade==1)
		)
		waterRipples.pop_front();
}
void DrawWaterRipples(CommandList cmd)
{
	GetDevice()->EventBegin("Water Ripples", cmd);
	for(wiSprite* i:waterRipples){
		i->DrawNormal(cmd);
	}
	GetDevice()->EventEnd(cmd);
}



void DrawSoftParticles(const CameraComponent& camera, bool distortion, CommandList cmd)
{
	const Scene& scene = GetScene();
	const FrameCulling& culling = frameCullings.at(&camera);
	size_t emitterCount = culling.culledEmitters.size();

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
		else if (!distortion && (emitter.shaderType == wiEmittedParticle::SOFT || emitter.shaderType == wiEmittedParticle::SIMPLEST || IsWireRender()))
		{
			emitter.Draw(camera, material, cmd);
		}
	}

	GetRenderFrameAllocator(cmd).free(sizeof(uint32_t) * emitterCount);

}
void DrawLights(const CameraComponent& camera, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();
	const FrameCulling& culling = frameCullings.at(&camera);

	const Scene& scene = GetScene();

	device->EventBegin("Light Render", cmd);
	auto range = wiProfiler::BeginRangeGPU("Light Render", cmd);

	BindShadowmaps(PS, cmd);
	BindEnvironmentTextures(PS, cmd);

	// Environmental light (envmap + voxelGI) is always drawn
	{
		device->BindPipelineState(&PSO_enviromentallight, cmd);
		device->Draw(3, 0, cmd); // full screen triangle
	}

	for (int type = 0; type < LightComponent::LIGHTTYPE_COUNT; ++type)
	{
		device->BindPipelineState(&PSO_deferredlight[type], cmd);

		for (size_t i = 0; i < culling.culledLights.size(); ++i)
		{
			const uint32_t lightIndex = culling.culledLights[i];
			const LightComponent& light = scene.lights[lightIndex];
			if (light.GetType() != type || light.IsStatic())
				continue;

			switch (type)
			{
			case LightComponent::DIRECTIONAL:
			case LightComponent::SPHERE:
			case LightComponent::DISC:
			case LightComponent::RECTANGLE:
			case LightComponent::TUBE:
				{
					MiscCB miscCb;
					miscCb.g_xColor.x =  float(entityArrayOffset_Lights + i);
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
					XMStoreFloat4x4(&miscCb.g_xTransform, XMMatrixTranspose(XMMatrixScaling(sca, sca, sca)*XMMatrixTranslationFromVector(XMLoadFloat3(&light.position)) * camera.GetViewProjection()));
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
					XMStoreFloat4x4(&miscCb.g_xTransform, XMMatrixTranspose(
						XMMatrixScaling(coneS*light.GetRange(), light.GetRange(), coneS*light.GetRange())*
						XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
						XMMatrixTranslationFromVector(XMLoadFloat3(&light.position)) *
						camera.GetViewProjection()
					));
					device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &miscCb, cmd);
					device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
					device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

					device->Draw(192, 0, cmd); // cone
				}
				break;
			}
		}


	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void DrawLightVisualizers(const CameraComponent& camera, CommandList cmd)
{
	const FrameCulling& culling = frameCullings.at(&camera);

	if (!culling.culledLights.empty())
	{
		GraphicsDevice* device = GetDevice();
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
						XMStoreFloat4x4(&lcb.lightWorld, XMMatrixTranspose(
							XMMatrixScaling(lcb.lightEnerdis.w, lcb.lightEnerdis.w, lcb.lightEnerdis.w)*
							camrot*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))
						));

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(108, 0, cmd); // circle
					}
					else if (type == LightComponent::SPOT)
					{
						float coneS = (float)(light.fov / 0.7853981852531433);
						lcb.lightEnerdis.w = light.GetRange()*light.energy*0.03f; // scale
						XMStoreFloat4x4(&lcb.lightWorld, XMMatrixTranspose(
							XMMatrixScaling(coneS*lcb.lightEnerdis.w, lcb.lightEnerdis.w, coneS*lcb.lightEnerdis.w)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))
						));

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(192, 0, cmd); // cone
					}
					else if (type == LightComponent::SPHERE)
					{
						XMStoreFloat4x4(&lcb.lightWorld, XMMatrixTranspose(
							XMMatrixScaling(light.radius, light.radius, light.radius)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))*
							camera.GetViewProjection()
						));

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(2880, 0, cmd); // uv-sphere
					}
					else if (type == LightComponent::DISC)
					{
						XMStoreFloat4x4(&lcb.lightWorld, XMMatrixTranspose(
							XMMatrixScaling(light.radius, light.radius, light.radius)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))*
							camera.GetViewProjection()
						));

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(108, 0, cmd); // circle
					}
					else if (type == LightComponent::RECTANGLE)
					{
						XMStoreFloat4x4(&lcb.lightWorld, XMMatrixTranspose(
							XMMatrixScaling(light.width * 0.5f * light.scale.x, light.height * 0.5f * light.scale.y, 0.5f)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))*
							camera.GetViewProjection()
						));

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(6, 0, cmd); // quad
					}
					else if (type == LightComponent::TUBE)
					{
						XMStoreFloat4x4(&lcb.lightWorld, XMMatrixTranspose(
							XMMatrixScaling(std::max(light.width * 0.5f, light.radius) * light.scale.x, light.radius, light.radius)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position))*
							camera.GetViewProjection()
						));

						device->UpdateBuffer(&constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, cmd);

						device->Draw(384, 0, cmd); // cylinder
					}
				}
			}

		}

		device->EventEnd(cmd);

	}
}
void DrawVolumeLights(const CameraComponent& camera, CommandList cmd)
{
	const FrameCulling& culling = frameCullings.at(&camera);

	if (!culling.culledLights.empty())
	{
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Volumetric Light Render", cmd);

		BindShadowmaps(PS, cmd);

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
						XMStoreFloat4x4(&miscCb.g_xTransform, XMMatrixTranspose(XMMatrixScaling(sca, sca, sca)*XMMatrixTranslationFromVector(XMLoadFloat3(&light.position)) * camera.GetViewProjection()));
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
						XMStoreFloat4x4(&miscCb.g_xTransform, XMMatrixTranspose(
							XMMatrixScaling(coneS*light.GetRange(), light.GetRange(), coneS*light.GetRange())*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position)) *
							camera.GetViewProjection()
						));
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
void DrawLensFlares(const CameraComponent& camera, CommandList cmd)
{
	if (IsWireRender())
		return;

	const FrameCulling& culling = frameCullings.at(&camera);

	const Scene& scene = GetScene();

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
				POS = XMVector3Normalize(
					-XMVector3Transform(XMVectorSet(0, -1, 0, 1), XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation)))
				) * 100000;
			}

			if (XMVectorGetX(XMVector3Dot(XMVectorSubtract(POS, camera.GetEye()), camera.GetAt())) > 0) // check if the camera is facing towards the flare or not
			{
				// Get the screen position of the flare:
				XMVECTOR flarePos = XMVector3Project(POS, 0.f, 0.f, (float)GetInternalResolution().x, (float)GetInternalResolution().y, 0.0f, 1.0f, camera.GetProjection(), camera.GetView(), XMMatrixIdentity());
				wiLensFlare::Draw(cmd, flarePos, light.lensFlareRimTextures);
			}

		}

	}
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
		shadowMapArray_2D.RequestIndependentRenderTargetArraySlices(true);
		shadowMapArray_Transparent.RequestIndependentRenderTargetArraySlices(true);

		TextureDesc desc;
		desc.Width = SHADOWRES_2D;
		desc.Height = SHADOWRES_2D;
		desc.MipLevels = 1;
		desc.ArraySize = SHADOWCOUNT_2D;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
		desc.Format = DSFormat_small_alias;
		GetDevice()->CreateTexture2D(&desc, nullptr, &shadowMapArray_2D);

		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = RTFormat_ldr;
		GetDevice()->CreateTexture2D(&desc, nullptr, &shadowMapArray_Transparent);
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
		shadowMapArray_Cube.RequestIndependentRenderTargetArraySlices(true);
		shadowMapArray_Cube.RequestIndependentRenderTargetCubemapFaces(false);

		TextureDesc desc;
		desc.Width = SHADOWRES_CUBE;
		desc.Height = SHADOWRES_CUBE;
		desc.MipLevels = 1;
		desc.ArraySize = 6 * SHADOWCOUNT_CUBE;
		desc.Format = DSFormat_small_alias;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = USAGE_DEFAULT;
		desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;
		GetDevice()->CreateTexture2D(&desc, nullptr, &shadowMapArray_Cube);
	}

}
void DrawForShadowMap(const CameraComponent& camera, CommandList cmd, uint32_t layerMask)
{
	if (IsWireRender())
		return;

	GraphicsDevice* device = GetDevice();
	const FrameCulling& culling = frameCullings.at(&GetCamera());

	if (!culling.culledLights.empty())
	{
		device->EventBegin("ShadowMap Render", cmd);
		auto range = wiProfiler::BeginRangeGPU("Shadow Rendering", cmd);

		BindCommonResources(cmd);
		BindConstantBuffers(VS, cmd);
		BindConstantBuffers(PS, cmd);


		ViewPort vp;

		// RGB: Shadow tint (multiplicative), A: Refraction caustics(additive)
		const float transparentShadowClearColor[] = { 1,1,1,0 };


		const Scene& scene = GetScene();

		device->UnbindResources(TEXSLOT_SHADOWARRAY_2D, 2, cmd);

		for (int type = 0; type < LightComponent::LIGHTTYPE_COUNT; ++type)
		{
			switch (type)
			{
			case LightComponent::DIRECTIONAL:
			case LightComponent::SPOT:
			{
				vp.TopLeftX = 0;
				vp.TopLeftY = 0;
				vp.Width = (float)SHADOWRES_2D;
				vp.Height = (float)SHADOWRES_2D;
				vp.MinDepth = 0.0f;
				vp.MaxDepth = 1.0f;
				device->BindViewports(1, &vp, cmd);
				break;
			}
			break;
			case LightComponent::POINT:
			case LightComponent::SPHERE:
			case LightComponent::DISC:
			case LightComponent::RECTANGLE:
			case LightComponent::TUBE:
			{
				vp.TopLeftX = 0;
				vp.TopLeftY = 0;
				vp.Width = (float)SHADOWRES_CUBE;
				vp.Height = (float)SHADOWRES_CUBE;
				vp.MinDepth = 0.0f;
				vp.MaxDepth = 1.0f;
				device->BindViewports(1, &vp, cmd);

				device->BindConstantBuffer(GS, &constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubemapRenderCB), cmd);
				break;
			}
			break;
			default:
				break;
			}

			for (uint32_t lightIndex : culling.culledLights)
			{
				const LightComponent& light = scene.lights[lightIndex];
				if (light.shadowMap_index < 0 || light.GetType() != type || !light.IsCastingShadow() || light.IsStatic())
				{
					continue;
				}

				switch (type)
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
							if (shcams[cascade].boundingbox.intersects(aabb))
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
							XMStoreFloat4x4(&cb.g_xCamera_VP, shcams[cascade].getVP());
							device->UpdateBuffer(&constantBuffers[CBTYPE_CAMERA], &cb, cmd);

							device->ClearDepthStencil(&shadowMapArray_2D, CLEAR_DEPTH, 0.0f, 0, cmd, light.shadowMap_index + cascade);

							// unfortunately we will always have to clear the associated transparent shadowmap to avoid discrepancy with shadowmap indexing changes across frames
							device->ClearRenderTarget(&shadowMapArray_Transparent, transparentShadowClearColor, cmd, light.shadowMap_index + cascade);

							// render opaque shadowmap:
							device->BindRenderTargets(0, nullptr, &shadowMapArray_2D, cmd, light.shadowMap_index + cascade);
							RenderMeshes(renderQueue, RENDERPASS_SHADOW, RENDERTYPE_OPAQUE, cmd);

							if (GetTransparentShadowsEnabled() && transparentShadowsRequested)
							{
								// render transparent shadowmap:
								const Texture2D* rts[] = {
									&shadowMapArray_Transparent
								};
								device->BindRenderTargets(ARRAYSIZE(rts), rts, &shadowMapArray_2D, cmd, light.shadowMap_index + cascade);
								RenderMeshes(renderQueue, RENDERPASS_SHADOW, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, cmd);
							}
							GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
						}

					}
				}
				break;
				case LightComponent::SPOT:
				{
					SHCAM shcam;
					CreateSpotLightShadowCam(light, shcam);

					RenderQueue renderQueue;
					bool transparentShadowsRequested = false;
					for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
					{
						const AABB& aabb = scene.aabb_objects[i];
						if (shcam.frustum.CheckBox(aabb))
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
						XMStoreFloat4x4(&cb.g_xCamera_VP, shcam.getVP());
						device->UpdateBuffer(&constantBuffers[CBTYPE_CAMERA], &cb, cmd);

						device->ClearDepthStencil(&shadowMapArray_2D, CLEAR_DEPTH, 0.0f, 0, cmd, light.shadowMap_index);

						// unfortunately we will always have to clear the associated transparent shadowmap to avoid discrepancy with shadowmap indexing changes across frames
						device->ClearRenderTarget(&shadowMapArray_Transparent, transparentShadowClearColor, cmd, light.shadowMap_index);

						// render opaque shadowmap:
						device->BindRenderTargets(0, nullptr, &shadowMapArray_2D, cmd, light.shadowMap_index);
						RenderMeshes(renderQueue, RENDERPASS_SHADOW, RENDERTYPE_OPAQUE, cmd);

						if (GetTransparentShadowsEnabled() && transparentShadowsRequested)
						{
							// render transparent shadowmap:
							const Texture2D* rts[] = {
								&shadowMapArray_Transparent
							};
							device->BindRenderTargets(ARRAYSIZE(rts), rts, &shadowMapArray_2D, cmd, light.shadowMap_index);
							RenderMeshes(renderQueue, RENDERPASS_SHADOW, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, cmd);
						}
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
						device->BindRenderTargets(0, nullptr, &shadowMapArray_Cube, cmd, light.shadowMap_index);
						device->ClearDepthStencil(&shadowMapArray_Cube, CLEAR_DEPTH, 0.0f, 0, cmd, light.shadowMap_index);

						MiscCB miscCb;
						miscCb.g_xColor = float4(light.position.x, light.position.y, light.position.z, 1.0f / light.GetRange()); // reciprocal range, to avoid division in shader
						device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &miscCb, cmd);
						device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
						device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

						const float zNearP = 0.1f;
						const float zFarP = std::max(1.0f, light.GetRange());
						const XMVECTOR lightPos = XMLoadFloat3(&light.position);
						const SHCAM cameras[] = {
							SHCAM(lightPos, XMVectorSet(0.5f, -0.5f, -0.5f, -0.5f), zNearP, zFarP, XM_PIDIV2), //+x
							SHCAM(lightPos, XMVectorSet(0.5f, 0.5f, 0.5f, -0.5f), zNearP, zFarP, XM_PIDIV2), //-x
							SHCAM(lightPos, XMVectorSet(1, 0, 0, -0), zNearP, zFarP, XM_PIDIV2), //+y
							SHCAM(lightPos, XMVectorSet(0, 0, 0, -1), zNearP, zFarP, XM_PIDIV2), //-y
							SHCAM(lightPos, XMVectorSet(0.707f, 0, 0, -0.707f), zNearP, zFarP, XM_PIDIV2), //+z
							SHCAM(lightPos, XMVectorSet(0, 0.707f, 0.707f, 0), zNearP, zFarP, XM_PIDIV2), //-z
						};

						CubemapRenderCB cb;
						for (int shcam = 0; shcam < ARRAYSIZE(cameras); ++shcam)
						{
							XMStoreFloat4x4(&cb.xCubeShadowVP[shcam], cameras[shcam].getVP());
						}
						device->UpdateBuffer(&constantBuffers[CBTYPE_CUBEMAPRENDER], &cb, cmd);

						RenderMeshes(renderQueue, RENDERPASS_SHADOWCUBE, RENDERTYPE_OPAQUE, cmd);

						GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
					}

				}
				break;
				} // terminate switch
			}

		}

		device->BindRenderTargets(0, nullptr, nullptr, cmd);


		wiProfiler::EndRange(range); // Shadow Rendering
		device->EventEnd(cmd);
	}
}

void DrawScene(const CameraComponent& camera, bool tessellation, CommandList cmd, RENDERPASS renderPass, bool grass, bool occlusionCulling)
{
	GraphicsDevice* device = GetDevice();
	const Scene& scene = GetScene();
	const FrameCulling& culling = frameCullings.at(&camera);

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

	if (renderPass == RENDERPASS_TILEDFORWARD)
	{
		device->BindResource(PS, &resourceBuffers[RBTYPE_ENTITYTILES_OPAQUE], SBSLOT_ENTITYTILES, cmd);
	}

	if (grass)
	{
		if (GetAlphaCompositionEnabled())
		{
			// cut off most transparent areas
			SetAlphaRef(0.25f, cmd);
		}

		for (uint32_t hairIndex : culling.culledHairs)
		{
			const wiHairParticle& hair = scene.hairs[hairIndex];

			if (camera.frustum.CheckBox(hair.aabb))
			{
				Entity entity = scene.hairs.GetEntity(hairIndex);
				const MaterialComponent& material = *scene.materials.GetComponent(entity);

				if (renderPass == RENDERPASS_FORWARD)
				{
					ForwardEntityMaskCB cb = ForwardEntityCullingCPU(culling, hair.aabb, renderPass);
					device->UpdateBuffer(&constantBuffers[CBTYPE_FORWARDENTITYMASK], &cb, cmd);
					device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_FORWARDENTITYMASK], CB_GETBINDSLOT(ForwardEntityMaskCB), cmd);
				}

				hair.Draw(camera, material, renderPass, false, cmd);
			}
		}
	}

	RenderImpostors(camera, renderPass, cmd);

	RenderQueue renderQueue;
	renderQueue.camera = &camera;
	for (uint32_t instanceIndex : culling.culledObjects)
	{
		const ObjectComponent& object = scene.objects[instanceIndex];

		if (GetOcclusionCullingEnabled() && occlusionCulling && object.IsOccluded())
			continue;

		if (object.IsRenderable() && object.GetRenderTypes() & RENDERTYPE_OPAQUE)
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
		renderQueue.sort(RenderQueue::SORT_FRONT_TO_BACK);
		RenderMeshes(renderQueue, renderPass, RENDERTYPE_OPAQUE, cmd, tessellation);

		GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
	}

	device->EventEnd(cmd);

}

void DrawScene_Transparent(const CameraComponent& camera, RENDERPASS renderPass, CommandList cmd, bool grass, bool occlusionCulling)
{
	GraphicsDevice* device = GetDevice();
	const Scene& scene = GetScene();
	const FrameCulling& culling = frameCullings.at(&camera);

	device->EventBegin("DrawScene_Transparent", cmd);

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

	if (renderPass == RENDERPASS_TILEDFORWARD)
	{
		device->BindResource(PS, &resourceBuffers[RBTYPE_ENTITYTILES_TRANSPARENT], SBSLOT_ENTITYTILES, cmd);
	}

	if (ocean != nullptr)
	{
		ocean->Render(camera, scene.weather, renderTime, cmd);
	}

	if (grass && GetAlphaCompositionEnabled())
	{
		// transparent passes can only render hair when alpha composition is enabled

		for (uint32_t hairIndex : culling.culledHairs)
		{
			const wiHairParticle& hair = scene.hairs[hairIndex];

			if (camera.frustum.CheckBox(hair.aabb))
			{
				Entity entity = scene.hairs.GetEntity(hairIndex);
				const MaterialComponent& material = *scene.materials.GetComponent(entity);

				if (renderPass == RENDERPASS_FORWARD)
				{
					ForwardEntityMaskCB cb = ForwardEntityCullingCPU(culling, hair.aabb, renderPass);
					device->UpdateBuffer(&constantBuffers[CBTYPE_FORWARDENTITYMASK], &cb, cmd);
					device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_FORWARDENTITYMASK], CB_GETBINDSLOT(ForwardEntityMaskCB), cmd);
				}

				hair.Draw(camera, material, renderPass, true, cmd);
			}
		}
	}

	RenderQueue renderQueue;
	renderQueue.camera = &camera;
	for (uint32_t instanceIndex : culling.culledObjects)
	{
		const ObjectComponent& object = scene.objects[instanceIndex];

		if (GetOcclusionCullingEnabled() && occlusionCulling && object.IsOccluded())
			continue;

		if (object.IsRenderable() && object.GetRenderTypes() & RENDERTYPE_TRANSPARENT)
		{
			RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
			size_t meshIndex = scene.meshes.GetIndex(object.meshID);
			batch->Create(meshIndex, instanceIndex, wiMath::DistanceEstimated(camera.Eye, object.center));
			renderQueue.add(batch);
		}
	}
	if (!renderQueue.empty())
	{
		renderQueue.sort(RenderQueue::SORT_BACK_TO_FRONT);
		RenderMeshes(renderQueue, renderPass, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, cmd, false);

		GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
	}

	device->EventEnd(cmd);
}

void DrawDebugWorld(const CameraComponent& camera, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();
	const Scene& scene = GetScene();

	device->EventBegin("DrawDebugWorld", cmd);

	BindCommonResources(cmd);
	BindConstantBuffers(VS, cmd);
	BindConstantBuffers(PS, cmd);

	if (debugPartitionTree)
	{
		// Actually, there is no SPTree any more, so this will just render all aabbs...
		device->EventBegin("DebugPartitionTree", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);

		GPUBuffer* vbs[] = {
			wiCube::GetVertexBuffer(),
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(wiCube::GetIndexBuffer(), INDEXFORMAT_16BIT, 0, cmd);

		MiscCB sb;

		for (size_t i = 0; i < scene.aabb_objects.GetCount(); ++i)
		{
			const AABB& aabb = scene.aabb_objects[i];

			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(aabb.getAsBoxMatrix()*camera.GetViewProjection()));
			sb.g_xColor = XMFLOAT4(1, 0, 0, 1);

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}

		for (size_t i = 0; i < scene.aabb_lights.GetCount(); ++i)
		{
			const AABB& aabb = scene.aabb_lights[i];

			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(aabb.getAsBoxMatrix()*camera.GetViewProjection()));
			sb.g_xColor = XMFLOAT4(1, 1, 0, 1);

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}

		for (size_t i = 0; i < scene.aabb_decals.GetCount(); ++i)
		{
			const AABB& aabb = scene.aabb_decals[i];

			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(aabb.getAsBoxMatrix()*camera.GetViewProjection()));
			sb.g_xColor = XMFLOAT4(1, 0, 1, 1);

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}

		for (size_t i = 0; i < scene.aabb_probes.GetCount(); ++i)
		{
			const AABB& aabb = scene.aabb_probes[i];

			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(aabb.getAsBoxMatrix()*camera.GetViewProjection()));
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
		XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(camera.GetViewProjection()));
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
			const UINT strides[] = {
				sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
			};
			const UINT offsets[] = {
				mem.offset,
			};
			device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, cmd);

			device->Draw(2 * j, 0, cmd);

		}

		device->EventEnd(cmd);
	}

	if (!renderableLines.empty())
	{
		device->EventBegin("DebugLines", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_LINES], cmd);

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(camera.GetViewProjection()));
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
			segment.colorA = segment.colorB = line.color;

			memcpy((void*)((size_t)mem.data + i * sizeof(LineSegment)), &segment, sizeof(LineSegment));
			i++;
		}

		const GPUBuffer* vbs[] = {
			mem.buffer,
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		const UINT offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, cmd);

		device->Draw(2 * i, 0, cmd);

		renderableLines.clear();

		device->EventEnd(cmd);
	}

	if (!renderablePoints.empty())
	{
		device->EventBegin("DebugPoints", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_LINES], cmd);

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(camera.GetProjection())); // only projection, we will expand in view space on CPU below to be camera facing!
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
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		const UINT offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, cmd);

		device->Draw(2 * i, 0, cmd);

		renderablePoints.clear();

		device->EventEnd(cmd);
	}

	if (!renderableBoxes.empty())
	{
		device->EventBegin("DebugBoxes", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);

		GPUBuffer* vbs[] = {
			wiCube::GetVertexBuffer(),
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(wiCube::GetIndexBuffer(), INDEXFORMAT_16BIT, 0, cmd);

		MiscCB sb;

		for (auto& x : renderableBoxes)
		{
			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(XMLoadFloat4x4(&x.first)*camera.GetViewProjection()));
			sb.g_xColor = x.second;

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(24, 0, 0, cmd);
		}
		renderableBoxes.clear();

		device->EventEnd(cmd);
	}


	if (debugEnvProbes)
	{
		device->EventBegin("Debug EnvProbes", cmd);
		// Envmap spheres:

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_ENVPROBE], cmd);

		MiscCB sb;
		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			const EnvironmentProbeComponent& probe = scene.probes[i];

			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(XMMatrixTranslationFromVector(XMLoadFloat3(&probe.position))));
			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			if (probe.textureIndex < 0)
			{
				device->BindResource(PS, wiTextureHelper::getBlackCubeMap(), TEXSLOT_ONDEMAND0, cmd);
			}
			else
			{
				device->BindResource(PS, textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], TEXSLOT_ONDEMAND0, cmd, textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]->GetDesc().MipLevels + probe.textureIndex);
			}

			device->Draw(2880, 0, cmd); // uv-sphere
		}


		// Local proxy boxes:

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);

		GPUBuffer* vbs[] = {
			wiCube::GetVertexBuffer(),
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(wiCube::GetIndexBuffer(), INDEXFORMAT_16BIT, 0, cmd);

		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			const EnvironmentProbeComponent& probe = scene.probes[i];

			if (probe.textureIndex < 0)
			{
				continue;
			}

			Entity entity = scene.probes.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(XMLoadFloat4x4(&transform.world)*camera.GetViewProjection()));
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
		static UINT gridVertexCount = 0;
		static GPUBuffer* grid = nullptr;
		if (grid == nullptr)
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

			gridVertexCount = ARRAYSIZE(verts) / 2;

			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			InitData.pSysMem = verts;
			grid = new GPUBuffer;
			device->CreateBuffer(&bd, &InitData, grid);
		}

		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(camera.GetViewProjection()));
		sb.g_xColor = float4(1, 1, 1, 1);

		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		GPUBuffer* vbs[] = {
			grid,
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, cmd);
		device->Draw(gridVertexCount, 0, cmd);

		device->EventEnd(cmd);
	}

	if (voxelHelper && textures[TEXTYPE_3D_VOXELRADIANCE] != nullptr)
	{
		device->EventBegin("Debug Voxels", cmd);

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_VOXEL], cmd);

		device->BindResource(VS, textures[TEXTYPE_3D_VOXELRADIANCE], TEXSLOT_VOXELRADIANCE, cmd);


		MiscCB sb;
		XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(XMMatrixTranslationFromVector(XMLoadFloat3(&voxelSceneData.center)) * camera.GetViewProjection()));
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

			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(XMLoadFloat4x4(&transform.world)*camera.GetViewProjection()));
			sb.g_xColor = float4(0, 1, 0, 1);
			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			if (mesh == nullptr)
			{
				// No mesh, just draw a box:
				device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);
				GPUBuffer* vbs[] = {
					wiCube::GetVertexBuffer(),
				};
				const UINT strides[] = {
					sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
				};
				device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, cmd);
				device->BindIndexBuffer(wiCube::GetIndexBuffer(), INDEXFORMAT_16BIT, 0, cmd);
				device->DrawIndexed(24, 0, 0, cmd);
			}
			else
			{
				// Draw mesh wireframe:
				device->BindPipelineState(&PSO_debug[DEBUGRENDERING_EMITTER], cmd);
				GPUBuffer* vbs[] = {
					mesh->streamoutBuffer_POS != nullptr ? mesh->streamoutBuffer_POS.get() : mesh->vertexBuffer_POS.get(),
				};
				const UINT strides[] = {
					sizeof(MeshComponent::Vertex_POS),
				};
				device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, cmd);
				device->BindIndexBuffer(mesh->indexBuffer.get(), mesh->GetIndexFormat(), 0, cmd);

				device->DrawIndexed((UINT)mesh->indices.size(), 0, 0, cmd);
			}
		}

		device->EventEnd(cmd);
	}


	if (debugForceFields)
	{
		device->EventBegin("DebugForceFields", cmd);

		MiscCB sb;
		for (size_t i = 0; i < scene.forces.GetCount(); ++i)
		{
			const ForceFieldComponent& force = scene.forces[i];

			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(camera.GetViewProjection()));
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

		GPUBuffer* vbs[] = {
			wiCube::GetVertexBuffer(),
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(wiCube::GetIndexBuffer(), INDEXFORMAT_16BIT, 0, cmd);

		MiscCB sb;
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);

		for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
		{
			const CameraComponent& cam = scene.cameras[i];
			Entity entity = scene.cameras.GetEntity(i);

			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranspose(cam.GetInvView()*camera.GetViewProjection()));

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

void DrawSky(CommandList cmd)
{
	GraphicsDevice* device = GetDevice();
	const Scene& scene = GetScene();

	device->EventBegin("DrawSky", cmd);
	
	if (enviroMap != nullptr)
	{
		device->BindPipelineState(&PSO_sky[SKYRENDERING_STATIC], cmd);
		device->BindResource(PS, enviroMap, TEXSLOT_GLOBALENVMAP, cmd);
	}
	else
	{
		device->BindPipelineState(&PSO_sky[SKYRENDERING_DYNAMIC], cmd);
		if (scene.weather.cloudiness > 0)
		{
			device->BindResource(PS, textures[TEXTYPE_2D_CLOUDS], TEXSLOT_ONDEMAND0, cmd);
		}
		else
		{
			device->BindResource(PS, wiTextureHelper::getBlack(), TEXSLOT_ONDEMAND0, cmd);
		}
	}

	BindConstantBuffers(VS, cmd);
	BindConstantBuffers(PS, cmd);

	device->Draw(240, 0, cmd); // icosphere

	device->EventEnd(cmd);
}
void DrawSun(CommandList cmd)
{
	GraphicsDevice* device = GetDevice();

	device->EventBegin("DrawSun", cmd);

	device->BindPipelineState(&PSO_sky[SKYRENDERING_SUN], cmd);

	if (enviroMap != nullptr)
	{
		device->BindResource(PS, wiTextureHelper::getBlack(), TEXSLOT_ONDEMAND0, cmd);
	}
	else
	{
		device->BindResource(PS, textures[TEXTYPE_2D_CLOUDS], TEXSLOT_ONDEMAND0, cmd);
	}

	BindConstantBuffers(VS, cmd);
	BindConstantBuffers(PS, cmd);

	device->Draw(240, 0, cmd);

	device->EventEnd(cmd);
}

void DrawDecals(const CameraComponent& camera, CommandList cmd)
{
	const FrameCulling& culling = frameCullings.at(&camera);

	if(!culling.culledDecals.empty())
	{
		GraphicsDevice* device = GetDevice();

		device->EventBegin("Decals", cmd);

		const Scene& scene = GetScene();

		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_DECAL], CB_GETBINDSLOT(DecalCB),cmd);

		device->BindStencilRef(STENCILREF_DEFAULT, cmd);

		device->BindPipelineState(&PSO_decal, cmd);

		for (size_t decalIndex : culling.culledDecals) 
		{
			const DecalComponent& decal = scene.decals[decalIndex];
			const AABB& aabb = scene.aabb_decals[decalIndex];

			if ((decal.texture != nullptr || decal.normal != nullptr) && camera.frustum.CheckBox(aabb)) 
			{

				device->BindResource(PS, decal.texture, TEXSLOT_ONDEMAND0, cmd);
				device->BindResource(PS, decal.normal, TEXSLOT_ONDEMAND1, cmd);

				XMMATRIX decalWorld = XMLoadFloat4x4(&decal.world);

				MiscCB dcbvs;
				XMStoreFloat4x4(&dcbvs.g_xTransform, XMMatrixTranspose(decalWorld*camera.GetViewProjection()));
				device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &dcbvs, cmd);
				device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

				DecalCB dcbps;
				XMStoreFloat4x4(&dcbps.xDecalVP, XMMatrixTranspose(XMMatrixInverse(nullptr, decalWorld))); // todo: cache the inverse!
				dcbps.hasTexNor = 0;
				if (decal.texture != nullptr)
					dcbps.hasTexNor |= 0x0000001;
				if (decal.normal != nullptr)
					dcbps.hasTexNor |= 0x0000010;
				XMStoreFloat3(&dcbps.eye, camera.GetEye());
				dcbps.opacity = decal.GetOpacity();
				dcbps.front = decal.front;
				device->UpdateBuffer(&constantBuffers[CBTYPE_DECAL], &dcbps, cmd);

				device->Draw(14, 0, cmd);

			}

		}

		device->EventEnd(cmd);
	}
}

static const UINT envmapCount = 16;
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
			for (int i = 0; i < ARRAYSIZE(envmapTaken); ++i)
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
}
void RefreshEnvProbes(CommandList cmd)
{
	const Scene& scene = GetScene();

	GraphicsDevice* device = GetDevice();
	device->EventBegin("EnvironmentProbe Refresh", cmd);

	static const UINT envmapRes = 128;
	static const UINT envmapMIPs = 8;

	if (textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY] == nullptr)
	{
		TextureDesc desc;
		desc.ArraySize = envmapCount * 6;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.Format = RTFormat_envprobe;
		desc.Height = envmapRes;
		desc.Width = envmapRes;
		desc.MipLevels = envmapMIPs;
		desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE /*| RESOURCE_MISC_GENERATE_MIPS*/;
		desc.Usage = USAGE_DEFAULT;

		textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY] = new Texture2D;
		textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]->RequestIndependentRenderTargetArraySlices(true);
		textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]->RequestIndependentShaderResourceArraySlices(true);
		textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]->RequestIndependentShaderResourcesForMIPs(true);
		textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]->RequestIndependentUnorderedAccessResourcesForMIPs(true);
		HRESULT hr = device->CreateTexture2D(&desc, nullptr, (Texture2D*)textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]);
		assert(SUCCEEDED(hr));
	}

	static std::unique_ptr<Texture2D> envrenderingDepthBuffer;
	if (envrenderingDepthBuffer == nullptr)
	{
		TextureDesc desc;
		desc.ArraySize = 6;
		desc.BindFlags = BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.Format = DSFormat_small;
		desc.Height = envmapRes;
		desc.Width = envmapRes;
		desc.MipLevels = 1;
		desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;
		desc.Usage = USAGE_DEFAULT;

		envrenderingDepthBuffer.reset(new Texture2D);
		HRESULT hr = device->CreateTexture2D(&desc, nullptr, envrenderingDepthBuffer.get());
		assert(SUCCEEDED(hr));
	}

	ViewPort VP;
	VP.Height = envmapRes;
	VP.Width = envmapRes;
	VP.TopLeftX = 0;
	VP.TopLeftY = 0;
	VP.MinDepth = 0.0f;
	VP.MaxDepth = 1.0f;
	device->BindViewports(1, &VP, cmd);

	const float zNearP = GetCamera().zNearP;
	const float zFarP = GetCamera().zFarP;


	for (uint32_t probeIndex : probesToRefresh)
	{
		const EnvironmentProbeComponent& probe = scene.probes[probeIndex];
		Entity entity = scene.probes.GetEntity(probeIndex);

		device->BindRenderTargets(1, (Texture2D**)&textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], envrenderingDepthBuffer.get(), cmd, probe.textureIndex);
		const float clearColor[4] = { 0,0,0,1 };
		device->ClearRenderTarget(textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], clearColor, cmd, probe.textureIndex);
		device->ClearDepthStencil(envrenderingDepthBuffer.get(), CLEAR_DEPTH, 0.0f, 0, cmd);

		const XMVECTOR probePos = XMLoadFloat3(&probe.position);
		const SHCAM cameras[] = {
			SHCAM(probePos, XMVectorSet(0.5f, -0.5f, -0.5f, -0.5f), zNearP, zFarP, XM_PIDIV2), //+x
			SHCAM(probePos, XMVectorSet(0.5f, 0.5f, 0.5f, -0.5f), zNearP, zFarP, XM_PIDIV2), //-x
			SHCAM(probePos, XMVectorSet(1, 0, 0, -0), zNearP, zFarP, XM_PIDIV2), //+y
			SHCAM(probePos, XMVectorSet(0, 0, 0, -1), zNearP, zFarP, XM_PIDIV2), //-y
			SHCAM(probePos, XMVectorSet(0.707f, 0, 0, -0.707f), zNearP, zFarP, XM_PIDIV2), //+z
			SHCAM(probePos, XMVectorSet(0, 0.707f, 0.707f, 0), zNearP, zFarP, XM_PIDIV2), //-z
		};

		CubemapRenderCB cb;
		for (int i = 0; i < ARRAYSIZE(cameras); ++i)
		{
			XMStoreFloat4x4(&cb.xCubeShadowVP[i], cameras[i].getVP());
		}

		device->UpdateBuffer(&constantBuffers[CBTYPE_CUBEMAPRENDER], &cb, cmd);
		device->BindConstantBuffer(GS, &constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubemapRenderCB), cmd);


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

		if (!renderQueue.empty())
		{
			BindShadowmaps(PS, cmd);

			RenderMeshes(renderQueue, RENDERPASS_ENVMAPCAPTURE, RENDERTYPE_OPAQUE | RENDERTYPE_TRANSPARENT, cmd);

			GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
		}

		// sky
		{
			if (enviroMap != nullptr)
			{
				device->BindPipelineState(&PSO_sky[SKYRENDERING_ENVMAPCAPTURE_STATIC], cmd);
				device->BindResource(PS, enviroMap, TEXSLOT_ONDEMAND0, cmd);
			}
			else
			{
				device->BindPipelineState(&PSO_sky[SKYRENDERING_ENVMAPCAPTURE_DYNAMIC], cmd);
				device->BindResource(PS, textures[TEXTYPE_2D_CLOUDS], TEXSLOT_ONDEMAND0, cmd);
			}

			device->Draw(240, 0, cmd);
		}

		device->BindRenderTargets(0, nullptr, nullptr, cmd);
		GenerateMipChain((Texture2D*)textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], MIPGENFILTER_LINEAR, cmd, probe.textureIndex);

		// Filter the enviroment map mip chain according to BRDF:
		//	A bit similar to MIP chain generation, but its input is the MIP-mapped texture,
		//	and we generatethe filtered MIPs from bottom to top.
		device->EventBegin("FilterEnvMap", cmd);
		{
			Texture* texture = textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY];
			TextureDesc desc = texture->GetDesc();
			int arrayIndex = probe.textureIndex;

			device->BindComputeShader(computeShaders[CSTYPE_FILTERENVMAP], cmd);

			desc.Width = 1;
			desc.Height = 1;
			for (UINT i = desc.MipLevels - 1; i > 0; --i)
			{
				device->BindUAV(CS, texture, 0, cmd, i);
				device->BindResource(CS, texture, TEXSLOT_UNIQUE0, cmd, std::max(0, (int)i - 2));

				FilterEnvmapCB cb;
				cb.filterResolution.x = desc.Width;
				cb.filterResolution.y = desc.Height;
				cb.filterArrayIndex = arrayIndex;
				cb.filterRoughness = (float)i / (float)desc.MipLevels;
				cb.filterRayCount = 128;
				device->UpdateBuffer(&constantBuffers[CBTYPE_FILTERENVMAP], &cb, cmd);
				device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_FILTERENVMAP], CB_GETBINDSLOT(FilterEnvmapCB), cmd);

				device->Dispatch(
					std::max(1u, (UINT)ceilf((float)desc.Width / GENERATEMIPCHAIN_2D_BLOCK_SIZE)),
					std::max(1u, (UINT)ceilf((float)desc.Height / GENERATEMIPCHAIN_2D_BLOCK_SIZE)),
					6,
					cmd);

				device->UAVBarrier((GPUResource**)&texture, 1, cmd);

				desc.Width *= 2;
				desc.Height *= 2;
			}
			device->UnbindUAVs(0, 1, cmd);
		}
		device->EventEnd(cmd);

	}

	device->EventEnd(cmd); // EnvironmentProbe Refresh
}

static const UINT maxImpostorCount = 8;
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
}
void RefreshImpostors(CommandList cmd)
{
	const Scene& scene = GetScene();

	if (!impostorsToRefresh.empty())
	{
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Impostor Refresh", cmd);

		BindConstantBuffers(VS, cmd);
		BindConstantBuffers(PS, cmd);

		static const UINT textureArraySize = maxImpostorCount * impostorCaptureAngles * 3;
		static const UINT textureDim = 128;
		static Texture2D depthStencil;

		if (textures[TEXTYPE_2D_IMPOSTORARRAY] == nullptr)
		{
			TextureDesc desc;
			desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			desc.Usage = USAGE_DEFAULT;
			desc.CPUAccessFlags = 0;
			desc.ArraySize = textureArraySize;
			desc.Width = textureDim;
			desc.Height = textureDim;
			desc.Depth = 1;
			desc.MipLevels = 1;
			desc.Format = RTFormat_impostor;
			desc.MiscFlags = 0;

			textures[TEXTYPE_2D_IMPOSTORARRAY] = new Texture2D;
			textures[TEXTYPE_2D_IMPOSTORARRAY]->RequestIndependentRenderTargetArraySlices(true);
			HRESULT hr = device->CreateTexture2D(&desc, nullptr, (Texture2D*)textures[TEXTYPE_2D_IMPOSTORARRAY]);
			assert(SUCCEEDED(hr));
			device->SetName(textures[TEXTYPE_2D_IMPOSTORARRAY], "ImpostorTarget");

			desc.BindFlags = BIND_DEPTH_STENCIL;
			desc.ArraySize = 1;
			desc.Format = DSFormat_small;
			hr = device->CreateTexture2D(&desc, nullptr, &depthStencil);
			assert(SUCCEEDED(hr));
			device->SetName(&depthStencil, "ImpostorDepthTarget");
		}

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

			const AABB& bbox = mesh.aabb;
			const XMFLOAT3 extents = bbox.getHalfWidth();

			const GPUBuffer* vbs[] = {
				mesh.IsSkinned() ? mesh.streamoutBuffer_POS.get() : mesh.vertexBuffer_POS.get(),
				mesh.vertexBuffer_UV0.get(),
				mesh.vertexBuffer_UV1.get(),
				mesh.vertexBuffer_ATL.get(),
				mesh.vertexBuffer_COL.get(),
				mesh.IsSkinned() ? mesh.streamoutBuffer_POS.get() : mesh.vertexBuffer_POS.get(),
				mem.buffer
			};
			UINT strides[] = {
				sizeof(MeshComponent::Vertex_POS),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(MeshComponent::Vertex_COL),
				sizeof(MeshComponent::Vertex_POS),
				sizeof(InstBuf)
			};
			UINT offsets[] = {
				0,
				0,
				0,
				0,
				0,
				0,
				mem.offset
			};
			device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, cmd);

			device->BindIndexBuffer(mesh.indexBuffer.get(), mesh.GetIndexFormat(), 0, cmd);

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
					device->BindRenderTargets(1, (Texture2D**)&textures[TEXTYPE_2D_IMPOSTORARRAY], &depthStencil, cmd, textureIndex);
					const float clearColor[4] = { 0,0,0,0 };
					device->ClearRenderTarget(textures[TEXTYPE_2D_IMPOSTORARRAY], clearColor, cmd, textureIndex);
					device->ClearDepthStencil(&depthStencil, CLEAR_DEPTH, 0.0f, 0, cmd);

					ViewPort viewPort;
					viewPort.Height = (float)textureDim;
					viewPort.Width = (float)textureDim;
					viewPort.TopLeftX = 0;
					viewPort.TopLeftY = 0;
					viewPort.MinDepth = 0;
					viewPort.MaxDepth = 1;
					device->BindViewports(1, &viewPort, cmd);


					CameraComponent impostorcamera;
					TransformComponent camera_transform;

					camera_transform.ClearTransform();
					camera_transform.Translate(bbox.getCenter());

					XMMATRIX P = XMMatrixOrthographicOffCenterLH(-extents.x, extents.x, -extents.y, extents.y, -extents.z, extents.z);
					XMStoreFloat4x4(&impostorcamera.Projection, P);
					camera_transform.RotateRollPitchYaw(XMFLOAT3(0, XM_2PI * (float)i / (float)impostorCaptureAngles, 0));

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

						device->BindConstantBuffer(VS, material.constantBuffer.get(), CB_GETBINDSLOT(MaterialCB), cmd);
						device->BindConstantBuffer(PS, material.constantBuffer.get(), CB_GETBINDSLOT(MaterialCB), cmd);

						const GPUResource* res[] = {
							material.GetBaseColorMap(),
							material.GetNormalMap(),
							material.GetSurfaceMap(),
						};
						device->BindResources(PS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), cmd);

						device->DrawIndexedInstanced(subset.indexCount, 1, subset.indexOffset, 0, 0, cmd);
					}

				}
			}

		}

		UpdateCameraCB(GetCamera(), cmd);

		device->EventEnd(cmd);
	}
}

void VoxelRadiance(CommandList cmd)
{
	if (!GetVoxelRadianceEnabled())
	{
		return;
	}

	GraphicsDevice* device = GetDevice();

	device->EventBegin("Voxel Radiance", cmd);
	auto range = wiProfiler::BeginRangeGPU("Voxel Radiance", cmd);

	const Scene& scene = GetScene();

	if (textures[TEXTYPE_3D_VOXELRADIANCE] == nullptr)
	{
		TextureDesc desc;
		desc.Width = voxelSceneData.res;
		desc.Height = voxelSceneData.res;
		desc.Depth = voxelSceneData.res;
		desc.MipLevels = 0;
		desc.Format = RTFormat_voxelradiance;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		desc.Usage = USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		textures[TEXTYPE_3D_VOXELRADIANCE] = new Texture3D;
		textures[TEXTYPE_3D_VOXELRADIANCE]->RequestIndependentShaderResourcesForMIPs(true);
		textures[TEXTYPE_3D_VOXELRADIANCE]->RequestIndependentUnorderedAccessResourcesForMIPs(true);
		HRESULT hr = device->CreateTexture3D(&desc, nullptr, (Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE]);
		assert(SUCCEEDED(hr));
	}
	if (voxelSceneData.secondaryBounceEnabled && textures[TEXTYPE_3D_VOXELRADIANCE_HELPER] == nullptr)
	{
		TextureDesc desc = ((Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE])->GetDesc();
		textures[TEXTYPE_3D_VOXELRADIANCE_HELPER] = new Texture3D;
		textures[TEXTYPE_3D_VOXELRADIANCE_HELPER]->RequestIndependentShaderResourcesForMIPs(true);
		textures[TEXTYPE_3D_VOXELRADIANCE_HELPER]->RequestIndependentUnorderedAccessResourcesForMIPs(true);
		HRESULT hr = device->CreateTexture3D(&desc, nullptr, (Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE_HELPER]);
		assert(SUCCEEDED(hr));
	}
	if (!resourceBuffers[RBTYPE_VOXELSCENE].IsValid())
	{
		GPUBufferDesc desc;
		desc.StructureByteStride = sizeof(UINT) * 2;
		desc.ByteWidth = desc.StructureByteStride * voxelSceneData.res * voxelSceneData.res * voxelSceneData.res;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;

		HRESULT hr = device->CreateBuffer(&desc, nullptr, &resourceBuffers[RBTYPE_VOXELSCENE]);
		assert(SUCCEEDED(hr));
	}

	Texture3D* result = (Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE];

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
		ViewPort VP;
		VP.TopLeftX = 0;
		VP.TopLeftY = 0;
		VP.Width = (float)voxelSceneData.res;
		VP.Height = (float)voxelSceneData.res;
		VP.MinDepth = 0.0f;
		VP.MaxDepth = 1.0f;
		device->BindViewports(1, &VP, cmd);

		GPUResource* UAVs[] = { &resourceBuffers[RBTYPE_VOXELSCENE] };
		device->BindUAVs(PS, UAVs, 0, 1, cmd);

		BindShadowmaps(PS, cmd);
		BindConstantBuffers(VS, cmd);
		BindConstantBuffers(PS, cmd);

		RenderMeshes(renderQueue, RENDERPASS_VOXELIZE, RENDERTYPE_OPAQUE, cmd);
		GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);

		// Copy the packed voxel scene data to a 3D texture, then delete the voxel scene emission data. The cone tracing will operate on the 3D texture
		device->EventBegin("Voxel Scene Copy - Clear", cmd);
		device->BindRenderTargets(0, nullptr, nullptr, cmd);
		device->BindUAV(CS, &resourceBuffers[RBTYPE_VOXELSCENE], 0, cmd);
		device->BindUAV(CS, textures[TEXTYPE_3D_VOXELRADIANCE], 1, cmd);

		if (device->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_UNORDEREDACCESSTEXTURE_LOAD_FORMAT_EXT))
		{
			device->BindComputeShader(computeShaders[CSTYPE_VOXELSCENECOPYCLEAR_TEMPORALSMOOTHING], cmd);
		}
		else
		{
			device->BindComputeShader(computeShaders[CSTYPE_VOXELSCENECOPYCLEAR], cmd);
		}
		device->Dispatch((UINT)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 256), 1, 1, cmd);
		device->EventEnd(cmd);

		if (voxelSceneData.secondaryBounceEnabled)
		{
			device->EventBegin("Voxel Radiance Secondary Bounce", cmd);
			device->UnbindUAVs(1, 1, cmd);
			// Pre-integrate the voxel texture by creating blurred mip levels:
			GenerateMipChain((Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE], MIPGENFILTER_LINEAR, cmd);
			device->BindUAV(CS, textures[TEXTYPE_3D_VOXELRADIANCE_HELPER], 0, cmd);
			device->BindResource(CS, textures[TEXTYPE_3D_VOXELRADIANCE], 0, cmd);
			device->BindResource(CS, &resourceBuffers[RBTYPE_VOXELSCENE], 1, cmd);
			device->BindComputeShader(computeShaders[CSTYPE_VOXELRADIANCESECONDARYBOUNCE], cmd);
			device->Dispatch((UINT)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 64), 1, 1, cmd);
			device->EventEnd(cmd);

			device->EventBegin("Voxel Scene Clear Normals", cmd);
			device->UnbindResources(1, 1, cmd);
			device->BindUAV(CS, &resourceBuffers[RBTYPE_VOXELSCENE], 0, cmd);
			device->BindComputeShader(computeShaders[CSTYPE_VOXELCLEARONLYNORMAL], cmd);
			device->Dispatch((UINT)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 256), 1, 1, cmd);
			device->EventEnd(cmd);

			result = (Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE_HELPER];
		}

		device->UnbindUAVs(0, 2, cmd);


		// Pre-integrate the voxel texture by creating blurred mip levels:
		{
			GenerateMipChain(result, MIPGENFILTER_LINEAR, cmd);
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
void ComputeTiledLightCulling(CommandList cmd, const Texture2D* lightbuffer_diffuse, const Texture2D* lightbuffer_specular)
{
	const bool deferred = lightbuffer_diffuse != nullptr && lightbuffer_specular != nullptr;
	auto range = wiProfiler::BeginRangeGPU("Entity Culling", cmd);
	GraphicsDevice* device = GetDevice();

	int _width = GetInternalResolution().x;
	int _height = GetInternalResolution().y;

	const XMUINT3 tileCount = GetEntityCullingTileCount();

	static int _savedWidth = 0;
	static int _savedHeight = 0;
	bool _resolutionChanged = device->ResolutionChanged();
	if (_savedWidth != _width || _savedHeight != _height)
	{
		_resolutionChanged = true;
		_savedWidth = _width;
		_savedHeight = _height;
	}

	static GPUBuffer* frustumBuffer = nullptr;
	if (frustumBuffer == nullptr || _resolutionChanged)
	{
		SAFE_DELETE(frustumBuffer);

		frustumBuffer = new GPUBuffer;

		GPUBufferDesc bd;
		bd.StructureByteStride = sizeof(XMFLOAT4) * 4; // storing 4 planes for every tile
		bd.ByteWidth = bd.StructureByteStride * tileCount.x * tileCount.y;
		bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.Usage = USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
		device->CreateBuffer(&bd, nullptr, frustumBuffer);

		device->SetName(frustumBuffer, "FrustumBuffer");
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

		GPUResource* uavs[] = { frustumBuffer };

		device->BindUAVs(CS, uavs, UAVSLOT_TILEFRUSTUMS, ARRAYSIZE(uavs), cmd);
		device->BindComputeShader(computeShaders[CSTYPE_TILEFRUSTUMS], cmd);

		DispatchParamsCB dispatchParams;
		dispatchParams.xDispatchParams_numThreads.x = tileCount.x;
		dispatchParams.xDispatchParams_numThreads.y = tileCount.y;
		dispatchParams.xDispatchParams_numThreads.z = 1;
		dispatchParams.xDispatchParams_numThreadGroups.x = (dispatchParams.xDispatchParams_numThreads.x + TILED_CULLING_BLOCKSIZE - 1) / TILED_CULLING_BLOCKSIZE;
		dispatchParams.xDispatchParams_numThreadGroups.y = (dispatchParams.xDispatchParams_numThreads.y + TILED_CULLING_BLOCKSIZE - 1) / TILED_CULLING_BLOCKSIZE;
		dispatchParams.xDispatchParams_numThreadGroups.z = 1;
		device->UpdateBuffer(&constantBuffers[CBTYPE_DISPATCHPARAMS], &dispatchParams, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_DISPATCHPARAMS], CB_GETBINDSLOT(DispatchParamsCB), cmd);

		device->Dispatch(dispatchParams.xDispatchParams_numThreadGroups.x, dispatchParams.xDispatchParams_numThreadGroups.y, dispatchParams.xDispatchParams_numThreadGroups.z, cmd);
		device->UnbindUAVs(UAVSLOT_TILEFRUSTUMS, 1, cmd);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), cmd);
	}

	if (textures[TEXTYPE_2D_DEBUGUAV] == nullptr || _resolutionChanged)
	{
		SAFE_DELETE(textures[TEXTYPE_2D_DEBUGUAV]);

		TextureDesc desc;
		desc.Width = (UINT)_width;
		desc.Height = (UINT)_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = USAGE_DEFAULT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		textures[TEXTYPE_2D_DEBUGUAV] = new Texture2D;
		device->CreateTexture2D(&desc, nullptr, (Texture2D*)textures[TEXTYPE_2D_DEBUGUAV]);
	}

	// Perform the culling
	{
		device->EventBegin("Entity Culling", cmd);

		device->UnbindResources(SBSLOT_ENTITYTILES, 1, cmd);

		device->BindResource(CS, frustumBuffer, SBSLOT_TILEFRUSTUMS, cmd);

		device->BindComputeShader(tiledLightingCS[deferred][GetAdvancedLightCulling()][GetDebugLightCulling()], cmd);

		if (GetDebugLightCulling())
		{
			device->BindUAV(CS, textures[TEXTYPE_2D_DEBUGUAV], UAVSLOT_DEBUGTEXTURE, cmd);
		}


		const FrameCulling& frameCulling = frameCullings.at(&GetCamera());


		DispatchParamsCB dispatchParams;
		dispatchParams.xDispatchParams_numThreadGroups.x = tileCount.x;
		dispatchParams.xDispatchParams_numThreadGroups.y = tileCount.y;
		dispatchParams.xDispatchParams_numThreadGroups.z = 1;
		dispatchParams.xDispatchParams_numThreads.x = dispatchParams.xDispatchParams_numThreadGroups.x * TILED_CULLING_BLOCKSIZE;
		dispatchParams.xDispatchParams_numThreads.y = dispatchParams.xDispatchParams_numThreadGroups.y * TILED_CULLING_BLOCKSIZE;
		dispatchParams.xDispatchParams_numThreads.z = 1;
		dispatchParams.xDispatchParams_value0 = (UINT)(frameCulling.culledLights.size() + frameCulling.culledEnvProbes.size() + frameCulling.culledDecals.size());
		device->UpdateBuffer(&constantBuffers[CBTYPE_DISPATCHPARAMS], &dispatchParams, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_DISPATCHPARAMS], CB_GETBINDSLOT(DispatchParamsCB), cmd);

		BindConstantBuffers(CS, cmd);

		if (deferred)
		{
			const GPUResource* uavs[] = {
				lightbuffer_diffuse,
				&resourceBuffers[RBTYPE_ENTITYTILES_TRANSPARENT],
				lightbuffer_specular,
			};
			device->BindUAVs(CS, uavs, UAVSLOT_TILEDDEFERRED_DIFFUSE, ARRAYSIZE(uavs), cmd);

			BindShadowmaps(CS, cmd);
			BindEnvironmentTextures(CS, cmd);

			device->Dispatch(dispatchParams.xDispatchParams_numThreadGroups.x, dispatchParams.xDispatchParams_numThreadGroups.y, dispatchParams.xDispatchParams_numThreadGroups.z, cmd);
			device->UAVBarrier(uavs, ARRAYSIZE(uavs), cmd);
		}
		else
		{
			GPUResource* uavs[] = {
				&resourceBuffers[RBTYPE_ENTITYTILES_OPAQUE],
				&resourceBuffers[RBTYPE_ENTITYTILES_TRANSPARENT],
			};
			device->BindUAVs(CS, uavs, UAVSLOT_ENTITYTILES_OPAQUE, ARRAYSIZE(uavs), cmd);

			device->Dispatch(dispatchParams.xDispatchParams_numThreadGroups.x, dispatchParams.xDispatchParams_numThreadGroups.y, dispatchParams.xDispatchParams_numThreadGroups.z, cmd);
			device->UAVBarrier(uavs, ARRAYSIZE(uavs), cmd);
		}

		device->UnbindUAVs(0, 8, cmd); // this unbinds pretty much every uav

		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range);
}


void ResolveMSAADepthBuffer(const Texture2D* dst, const Texture2D* src, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();
	device->EventBegin("Resolve MSAA DepthBuffer", cmd);

	device->BindResource(CS, src, TEXSLOT_ONDEMAND0, cmd);
	device->BindUAV(CS, dst, 0, cmd);

	TextureDesc desc = src->GetDesc();

	device->BindComputeShader(computeShaders[CSTYPE_RESOLVEMSAADEPTHSTENCIL], cmd);
	device->Dispatch((desc.Width + 7) / 8, (desc.Height + 7) / 8, 1, cmd);


	device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
	device->UnbindUAVs(0, 1, cmd);

	device->EventEnd(cmd);
}
void GenerateMipChain(const Texture1D* texture, MIPGENFILTER filter, CommandList cmd, int arrayIndex)
{
	assert(0 && "Not implemented!");
}
void GenerateMipChain(const Texture2D* texture, MIPGENFILTER filter, CommandList cmd, int arrayIndex)
{
	GraphicsDevice* device = GetDevice();
	TextureDesc desc = texture->GetDesc();

	if (desc.MipLevels < 2)
	{
		assert(0);
		return;
	}


	bool hdr = !device->IsFormatUnorm(desc.Format);

	device->BindRenderTargets(0, nullptr, nullptr, cmd);

	if (desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
	{

		if (desc.ArraySize > 6)
		{
			// Cubearray
			assert(arrayIndex >= 0 && "You should only filter a specific cube in the array for now, so provide its index!");

			switch (filter)
			{
			case MIPGENFILTER_POINT:
				device->EventBegin("GenerateMipChain CubeArray - PointFilter", cmd);
				device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAINCUBEARRAY_FLOAT4_SIMPLEFILTER : CSTYPE_GENERATEMIPCHAINCUBEARRAY_UNORM4_SIMPLEFILTER], cmd);
				device->BindSampler(CS, &samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, cmd);
				break;
			case MIPGENFILTER_LINEAR:
				device->EventBegin("GenerateMipChain CubeArray - LinearFilter", cmd);
				device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAINCUBEARRAY_FLOAT4_SIMPLEFILTER : CSTYPE_GENERATEMIPCHAINCUBEARRAY_UNORM4_SIMPLEFILTER], cmd);
				device->BindSampler(CS, &samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, cmd);
				break;
			default:
				assert(0);
				break;
			}

			for (UINT i = 0; i < desc.MipLevels - 1; ++i)
			{
				device->BindUAV(CS, texture, 0, cmd, i + 1);
				device->BindResource(CS, texture, TEXSLOT_UNIQUE0, cmd, i);
				desc.Width = std::max(1u, desc.Width / 2);
				desc.Height = std::max(1u, desc.Height / 2);

				GenerateMIPChainCB cb;
				cb.outputResolution.x = desc.Width;
				cb.outputResolution.y = desc.Height;
				cb.arrayIndex = arrayIndex;
				device->UpdateBuffer(&constantBuffers[CBTYPE_MIPGEN], &cb, cmd);
				device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_MIPGEN], CB_GETBINDSLOT(GenerateMIPChainCB), cmd);

				device->Dispatch(
					std::max(1u, (desc.Width + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
					std::max(1u, (desc.Height + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
					6,
					cmd);

				device->UAVBarrier((GPUResource**)&texture, 1, cmd);
			}
		}
		else
		{
			// Cubemap
			switch (filter)
			{
			case MIPGENFILTER_POINT:
				device->EventBegin("GenerateMipChain Cube - PointFilter", cmd);
				device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAINCUBE_FLOAT4_SIMPLEFILTER : CSTYPE_GENERATEMIPCHAINCUBE_UNORM4_SIMPLEFILTER], cmd);
				device->BindSampler(CS, &samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, cmd);
				break;
			case MIPGENFILTER_LINEAR:
				device->EventBegin("GenerateMipChain Cube - LinearFilter", cmd);
				device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAINCUBE_FLOAT4_SIMPLEFILTER : CSTYPE_GENERATEMIPCHAINCUBE_UNORM4_SIMPLEFILTER], cmd);
				device->BindSampler(CS, &samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, cmd);
				break;
			default:
				assert(0); // not implemented
				break;
			}

			for (UINT i = 0; i < desc.MipLevels - 1; ++i)
			{
				device->BindUAV(CS, texture, 0, cmd, i + 1);
				device->BindResource(CS, texture, TEXSLOT_UNIQUE0, cmd, i);
				desc.Width = std::max(1u, desc.Width / 2);
				desc.Height = std::max(1u, desc.Height / 2);

				GenerateMIPChainCB cb;
				cb.outputResolution.x = desc.Width;
				cb.outputResolution.y = desc.Height;
				cb.arrayIndex = 0;
				device->UpdateBuffer(&constantBuffers[CBTYPE_MIPGEN], &cb, cmd);
				device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_MIPGEN], CB_GETBINDSLOT(GenerateMIPChainCB), cmd);

				device->Dispatch(
					std::max(1u, (desc.Width + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
					std::max(1u, (desc.Height + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
					6,
					cmd);

				device->UAVBarrier((GPUResource**)&texture, 1, cmd);
			}
		}

	}
	else
	{
		// Texture2D
		switch (filter)
		{
		case MIPGENFILTER_POINT:
			device->EventBegin("GenerateMipChain 2D - PointFilter", cmd);
			device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAIN2D_FLOAT4_SIMPLEFILTER : CSTYPE_GENERATEMIPCHAIN2D_UNORM4_SIMPLEFILTER], cmd);
			device->BindSampler(CS, &samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, cmd);
			break;
		case MIPGENFILTER_LINEAR:
			device->EventBegin("GenerateMipChain 2D - LinearFilter", cmd);
			device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAIN2D_FLOAT4_SIMPLEFILTER : CSTYPE_GENERATEMIPCHAIN2D_UNORM4_SIMPLEFILTER], cmd);
			device->BindSampler(CS, &samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, cmd);
			break;
		case MIPGENFILTER_GAUSSIAN:
			device->EventBegin("GenerateMipChain 2D - GaussianFilter", cmd);
			device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAIN2D_FLOAT4_GAUSSIAN : CSTYPE_GENERATEMIPCHAIN2D_UNORM4_GAUSSIAN], cmd);
			break;
		case MIPGENFILTER_BICUBIC:
			device->EventBegin("GenerateMipChain 2D - BicubicFilter", cmd);
			device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAIN2D_FLOAT4_BICUBIC : CSTYPE_GENERATEMIPCHAIN2D_UNORM4_BICUBIC], cmd);
			break;
		default:
			assert(0);
			break;
		}

		for (UINT i = 0; i < desc.MipLevels - 1; ++i)
		{
			device->BindUAV(CS, texture, 0, cmd, i + 1);
			device->BindResource(CS, texture, TEXSLOT_UNIQUE0, cmd, i);
			desc.Width = std::max(1u, desc.Width / 2);
			desc.Height = std::max(1u, desc.Height / 2);

			GenerateMIPChainCB cb;
			cb.outputResolution.x = desc.Width;
			cb.outputResolution.y = desc.Height;
			cb.arrayIndex = arrayIndex >= 0 ? (uint)arrayIndex : 0;
			device->UpdateBuffer(&constantBuffers[CBTYPE_MIPGEN], &cb, cmd);
			device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_MIPGEN], CB_GETBINDSLOT(GenerateMIPChainCB), cmd);

			device->Dispatch(
				std::max(1u, (desc.Width + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
				std::max(1u, (desc.Height + GENERATEMIPCHAIN_2D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_2D_BLOCK_SIZE),
				1,
				cmd);

			device->UAVBarrier((GPUResource**)&texture, 1, cmd);
		}
	}

	device->UnbindResources(TEXSLOT_UNIQUE0, 1, cmd);
	device->UnbindUAVs(0, 1, cmd);

	device->EventEnd(cmd);
}
void GenerateMipChain(const Texture3D* texture, MIPGENFILTER filter, CommandList cmd, int arrayIndex)
{
	GraphicsDevice* device = GetDevice();
	TextureDesc desc = texture->GetDesc();

	if (desc.MipLevels < 2)
	{
		assert(0);
		return;
	}

	bool hdr = !device->IsFormatUnorm(desc.Format);

	device->BindRenderTargets(0, nullptr, nullptr, cmd);

	switch (filter)
	{
	case MIPGENFILTER_POINT:
		device->EventBegin("GenerateMipChain 3D - PointFilter", cmd);
		device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAIN3D_FLOAT4_SIMPLEFILTER : CSTYPE_GENERATEMIPCHAIN3D_UNORM4_SIMPLEFILTER], cmd);
		device->BindSampler(CS, &samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, cmd);
		break;
	case MIPGENFILTER_LINEAR:
		device->EventBegin("GenerateMipChain 3D - LinearFilter", cmd);
		device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAIN3D_FLOAT4_SIMPLEFILTER : CSTYPE_GENERATEMIPCHAIN3D_UNORM4_SIMPLEFILTER], cmd);
		device->BindSampler(CS, &samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, cmd);
		break;
	case MIPGENFILTER_GAUSSIAN:
		device->EventBegin("GenerateMipChain 3D - GaussianFilter", cmd);
		device->BindComputeShader(computeShaders[hdr ? CSTYPE_GENERATEMIPCHAIN3D_FLOAT4_GAUSSIAN : CSTYPE_GENERATEMIPCHAIN3D_UNORM4_GAUSSIAN], cmd);
		break;
	default:
		assert(0); // not implemented
		break;
	}

	for (UINT i = 0; i < desc.MipLevels - 1; ++i)
	{
		device->BindUAV(CS, texture, 0, cmd, i + 1);
		device->BindResource(CS, texture, TEXSLOT_UNIQUE0, cmd, i);
		desc.Width = std::max(1u, desc.Width / 2);
		desc.Height = std::max(1u, desc.Height / 2);
		desc.Depth = std::max(1u, desc.Depth / 2);

		GenerateMIPChainCB cb;
		cb.outputResolution.x = desc.Width;
		cb.outputResolution.y = desc.Height;
		cb.outputResolution.z = desc.Depth;
		cb.arrayIndex = arrayIndex >= 0 ? (uint)arrayIndex : 0;
		device->UpdateBuffer(&constantBuffers[CBTYPE_MIPGEN], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_MIPGEN], CB_GETBINDSLOT(GenerateMIPChainCB), cmd);

		device->Dispatch(
			std::max(1u, (desc.Width + GENERATEMIPCHAIN_3D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_3D_BLOCK_SIZE), 
			std::max(1u, (desc.Height + GENERATEMIPCHAIN_3D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_3D_BLOCK_SIZE),
			std::max(1u, (desc.Depth + GENERATEMIPCHAIN_3D_BLOCK_SIZE - 1) / GENERATEMIPCHAIN_3D_BLOCK_SIZE),
			cmd);
	}

	device->UnbindResources(TEXSLOT_UNIQUE0, 1, cmd);
	device->UnbindUAVs(0, 1, cmd);

	device->EventEnd(cmd);
}

void CopyTexture2D(const Texture2D* dst, UINT DstMIP, UINT DstX, UINT DstY, const Texture2D* src, UINT SrcMIP, CommandList cmd, BORDEREXPANDSTYLE borderExpand)
{
	GraphicsDevice* device = GetDevice();

	const TextureDesc& desc_dst = dst->GetDesc();
	const TextureDesc& desc_src = src->GetDesc();

	assert(desc_dst.BindFlags & BIND_UNORDERED_ACCESS);
	assert(desc_src.BindFlags & BIND_SHADER_RESOURCE);

	bool hdr = !device->IsFormatUnorm(desc_dst.Format);

	if (borderExpand == BORDEREXPAND_DISABLE)
	{
		if (hdr)
		{
			device->EventBegin("CopyTexture2D_FLOAT4", cmd);
			device->BindComputeShader(computeShaders[CSTYPE_COPYTEXTURE2D_FLOAT4], cmd);
		}
		else
		{
			device->EventBegin("CopyTexture2D_UNORM4", cmd);
			device->BindComputeShader(computeShaders[CSTYPE_COPYTEXTURE2D_UNORM4], cmd);
		}
	}
	else
	{
		if (hdr)
		{
			device->EventBegin("CopyTexture2D_BORDEREXPAND_FLOAT4", cmd);
			device->BindComputeShader(computeShaders[CSTYPE_COPYTEXTURE2D_FLOAT4_BORDEREXPAND], cmd);
		}
		else
		{
			device->EventBegin("CopyTexture2D_BORDEREXPAND_UNORM4", cmd);
			device->BindComputeShader(computeShaders[CSTYPE_COPYTEXTURE2D_UNORM4_BORDEREXPAND], cmd);
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

	device->BindResource(CS, src, TEXSLOT_ONDEMAND0, cmd);

	if (DstMIP > 0)
	{
		assert(desc_dst.MipLevels > DstMIP);
		device->BindUAV(CS, dst, 0, cmd, DstMIP);
	}
	else
	{
		device->BindUAV(CS, dst, 0, cmd);
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

void RayBuffers::Create(GraphicsDevice* device, uint32_t newRayCapacity)
{
	rayCapacity = newRayCapacity;

	GPUBufferDesc desc;
	HRESULT hr;

	desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	desc.StructureByteStride = sizeof(uint);
	desc.ByteWidth = desc.StructureByteStride * rayCapacity;
	desc.CPUAccessFlags = 0;
	desc.Format = FORMAT_UNKNOWN;
	desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.Usage = USAGE_DEFAULT;
	hr = device->CreateBuffer(&desc, nullptr, &rayIndexBuffer[0]);
	assert(SUCCEEDED(hr));
	device->SetName(&rayIndexBuffer[0], "rayIndexBuffer[0]");
	hr = device->CreateBuffer(&desc, nullptr, &rayIndexBuffer[1]);
	assert(SUCCEEDED(hr));
	device->SetName(&rayIndexBuffer[1], "rayIndexBuffer[1]");

	desc.StructureByteStride = sizeof(float); // sorting needs float now
	desc.ByteWidth = desc.StructureByteStride * rayCapacity;
	hr = device->CreateBuffer(&desc, nullptr, &raySortBuffer);
	assert(SUCCEEDED(hr));
	device->SetName(&raySortBuffer, "raySortBuffer");

	desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	desc.StructureByteStride = sizeof(TracedRenderingStoredRay);
	desc.ByteWidth = desc.StructureByteStride * rayCapacity;
	desc.CPUAccessFlags = 0;
	desc.Format = FORMAT_UNKNOWN;
	desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.Usage = USAGE_DEFAULT;
	hr = device->CreateBuffer(&desc, nullptr, &rayBuffer[0]);
	assert(SUCCEEDED(hr));
	device->SetName(&rayBuffer[0], "rayBuffer[0]");
	hr = device->CreateBuffer(&desc, nullptr, &rayBuffer[1]);
	assert(SUCCEEDED(hr));
	device->SetName(&rayBuffer[1], "rayBuffer[1]");
}

RayBuffers* GenerateScreenRayBuffers(const CameraComponent& camera, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();

	uint _width = GetInternalResolution().x;
	uint _height = GetInternalResolution().y;
	static uint RayCountPrev = 0;
	const uint _raycount = _width * _height;

	static RayBuffers screenRayBuffers;

	if (RayCountPrev != _raycount)
	{
		RayCountPrev = _raycount;
		screenRayBuffers.Create(device, _raycount);
	}


	device->EventBegin("Launch Screen Rays", cmd);
	{
		device->BindComputeShader(computeShaders[CSTYPE_RAYTRACE_LAUNCH], cmd);

		const XMFLOAT4& halton = wiMath::GetHaltonSequence((int)GetDevice()->GetFrameCount());
		TracedRenderingCB cb;
		cb.xTracePixelOffset = XMFLOAT2(halton.x, halton.y);
		cb.xTraceResolution.x = _width;
		cb.xTraceResolution.y = _height;
		cb.xTraceResolution_Inverse.x = 1.0f / cb.xTraceResolution.x;
		cb.xTraceResolution_Inverse.y = 1.0f / cb.xTraceResolution.y;
		device->UpdateBuffer(&constantBuffers[CBTYPE_RAYTRACE], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(TracedRenderingCB), cmd);

		GPUResource* uavs[] = {
			&screenRayBuffers.rayIndexBuffer[0],
			&screenRayBuffers.raySortBuffer,
			&screenRayBuffers.rayBuffer[0],
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), cmd);

		device->Dispatch(
			(_width + TRACEDRENDERING_LAUNCH_BLOCKSIZE - 1) / TRACEDRENDERING_LAUNCH_BLOCKSIZE,
			(_height + TRACEDRENDERING_LAUNCH_BLOCKSIZE - 1) / TRACEDRENDERING_LAUNCH_BLOCKSIZE,
			1, 
			cmd);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), cmd);

		device->UnbindUAVs(0, ARRAYSIZE(uavs), cmd);
	}
	device->EventEnd(cmd);

	return &screenRayBuffers;
}
void RayTraceScene(const RayBuffers* rayBuffers, const Texture2D* result, int accumulation_sample, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();
	const Scene& scene = GetScene();

	device->EventBegin("RayTraceScene", cmd);


	// Misc buffers:
	static bool initialized = false;
	static GPUBuffer indirectBuffer; // GPU job kicks
	static GPUBuffer counterBuffer[2]; // Active ray counter

	if (!initialized)
	{
		initialized = true;
		GPUBufferDesc desc;
		HRESULT hr;

		desc.BindFlags = BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(IndirectDispatchArgs);
		desc.ByteWidth = desc.StructureByteStride;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_DRAWINDIRECT_ARGS | RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &indirectBuffer);
		assert(SUCCEEDED(hr));
		device->SetName(&indirectBuffer, "raytrace_indirectBuffer");

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = sizeof(uint);
		desc.ByteWidth = desc.StructureByteStride;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_UNKNOWN;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		desc.Usage = USAGE_DEFAULT;
		hr = device->CreateBuffer(&desc, nullptr, &counterBuffer[0]);
		assert(SUCCEEDED(hr));
		device->SetName(&counterBuffer[0], "raytrace_counterBuffer[0]");
		hr = device->CreateBuffer(&desc, nullptr, &counterBuffer[1]);
		assert(SUCCEEDED(hr));
		device->SetName(&counterBuffer[1], "raytrace_counterBuffer[1]");
	}

	const TextureDesc& result_desc = result->GetDesc();
	static TextureDesc temp_desc;
	static Texture2D temp_texture;
	if (temp_desc.Width < result_desc.Width || temp_desc.Height < result_desc.Height)
	{
		temp_desc.Width = std::max(temp_desc.Width, result_desc.Width);
		temp_desc.Height = std::max(temp_desc.Height, result_desc.Height);
		temp_desc.Format = FORMAT_R16G16B16A16_FLOAT;
		temp_desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		device->CreateTexture2D(&temp_desc, nullptr, &temp_texture);
		device->SetName(&temp_texture, "raytrace_temp_texture");
	}

	// Begin raytrace

	auto range = wiProfiler::BeginRangeGPU("RayTrace - ALL", cmd);

	// write initial ray count to RW-counter:
	device->UpdateBuffer(&counterBuffer[0], &rayBuffers->rayCapacity, cmd);

	// Set up tracing resources:
	sceneBVH.Bind(CS, cmd);

	if (enviroMap != nullptr)
	{
		device->BindResource(CS, enviroMap, TEXSLOT_GLOBALENVMAP, cmd);
	}

	const XMFLOAT4& halton = wiMath::GetHaltonSequence((int)GetDevice()->GetFrameCount());
	TracedRenderingCB cb;
	cb.xTracePixelOffset = XMFLOAT2(halton.x, halton.y);
	cb.xTraceAccumulationFactor = 1.0f / ((float)accumulation_sample + 1.0f);
	cb.xTraceResolution.x = result_desc.Width;
	cb.xTraceResolution.y = result_desc.Height;
	cb.xTraceResolution_Inverse.x = 1.0f / cb.xTraceResolution.x;
	cb.xTraceResolution_Inverse.y = 1.0f / cb.xTraceResolution.y;

	for (uint32_t bounce = 0; bounce < raytraceBounceCount + 1; ++bounce) // first contact + indirect bounces
	{
		uint32_t __readBufferID = bounce % 2;
		uint32_t __writeBufferID = (bounce + 1) % 2;

		cb.xTraceUserData.x = bounce;
		cb.xTraceRandomSeed = renderTime + (float)bounce;
		device->UpdateBuffer(&constantBuffers[CBTYPE_RAYTRACE], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(TracedRenderingCB), cmd);


		// 1.) Kick off raytracing jobs for this bounce
		device->EventBegin("Kick Raytrace Jobs", cmd);
		{
			// Prepare indirect dispatch based on counter buffer value:
			device->BindComputeShader(computeShaders[CSTYPE_RAYTRACE_KICKJOBS], cmd);

			GPUResource* res[] = {
				&counterBuffer[__readBufferID],
			};
			device->BindResources(CS, res, TEXSLOT_UNIQUE0, ARRAYSIZE(res), cmd);
			GPUResource* uavs[] = {
				&counterBuffer[__writeBufferID],
				&indirectBuffer,
			};
			device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), cmd);

			device->Dispatch(1, 1, 1, cmd);

			device->UAVBarrier(uavs, ARRAYSIZE(uavs), cmd);
			device->UnbindUAVs(0, ARRAYSIZE(uavs), cmd);

			GPUResource* trans[] = {
				&indirectBuffer
			};
			device->TransitionBarrier(trans, 1, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_INDIRECT_ARGUMENT, cmd);
		}
		device->EventEnd(cmd);

		// 1.) Compute Primary Trace (closest hit)
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

			device->BindComputeShader(computeShaders[CSTYPE_RAYTRACE_PRIMARY], cmd);

			const GPUResource* res[] = {
				&counterBuffer[__readBufferID],
				&rayBuffers->rayIndexBuffer[__readBufferID],
				&rayBuffers->rayBuffer[__readBufferID],
			};
			device->BindResources(CS, res, TEXSLOT_ONDEMAND7, ARRAYSIZE(res), cmd);
			const GPUResource* uavs[] = {
				&counterBuffer[__writeBufferID],
				&rayBuffers->rayIndexBuffer[__writeBufferID],
				&rayBuffers->raySortBuffer,
				&rayBuffers->rayBuffer[__writeBufferID],
				&temp_texture,
			};
			device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), cmd);

			device->DispatchIndirect(&indirectBuffer, 0, cmd);

			device->UAVBarrier(uavs, ARRAYSIZE(uavs), cmd);
			device->UnbindUAVs(0, ARRAYSIZE(uavs), cmd);

			if (bounce == 0 || bounce == 1)
			{
				wiProfiler::EndRange(range); // RayTrace - First Bounce
			}
			device->EventEnd(cmd);
		}

		// Primary trace has written new alive ray buffer, so light sampling will use that:
		std::swap(__readBufferID, __writeBufferID);

		// 2.) Sort rays to achieve more coherency:
		device->EventBegin("Ray Sorting", cmd);
		wiGPUSortLib::Sort(rayBuffers->rayCapacity, rayBuffers->raySortBuffer, counterBuffer[__readBufferID], 0, rayBuffers->rayIndexBuffer[__readBufferID], cmd);
		device->EventEnd(cmd);


		// 3.) Light sampling (any hit) <- only after first bounce has occured
		{
			device->EventBegin("Light Sampling Rays", cmd);

			wiProfiler::range_id range;
			if (bounce == 1)
			{
				range = wiProfiler::BeginRangeGPU("RayTrace - First Light Sampling", cmd);
			}

			device->BindComputeShader(computeShaders[CSTYPE_RAYTRACE_LIGHTSAMPLING], cmd);

			const GPUResource* res[] = {
				&counterBuffer[__readBufferID],
				&rayBuffers->rayIndexBuffer[__readBufferID],
				&rayBuffers->rayBuffer[__readBufferID],
			};
			device->BindResources(CS, res, TEXSLOT_ONDEMAND7, ARRAYSIZE(res), cmd);
			const GPUResource* uavs[] = {
				&temp_texture,
			};
			device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), cmd);

			device->DispatchIndirect(&indirectBuffer, 0, cmd);

			device->UAVBarrier(uavs, ARRAYSIZE(uavs), cmd);
			device->UnbindUAVs(0, ARRAYSIZE(uavs), cmd);

			if (bounce == 1)
			{
				wiProfiler::EndRange(range); // RayTrace - First Light Sampling
			}
			device->EventEnd(cmd);
		}

	}

	device->EventBegin("Accumulate", cmd);
	{
		device->BindComputeShader(computeShaders[CSTYPE_RAYTRACE_ACCUMULATE], cmd);

		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(TracedRenderingCB), cmd);

		const GPUResource* res[] = {
			&temp_texture
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), cmd);
		const GPUResource* uavs[] = {
			result,
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), cmd);

		device->Dispatch(
			(result_desc.Width + TRACEDRENDERING_ACCUMULATE_BLOCKSIZE - 1) / TRACEDRENDERING_ACCUMULATE_BLOCKSIZE, 
			(result_desc.Height + TRACEDRENDERING_ACCUMULATE_BLOCKSIZE - 1) / TRACEDRENDERING_ACCUMULATE_BLOCKSIZE, 
			1, 
			cmd);

		device->UAVBarrier(uavs, ARRAYSIZE(uavs), cmd);

		device->UnbindUAVs(0, ARRAYSIZE(uavs), cmd);
	}
	device->EventEnd(cmd);

	wiProfiler::EndRange(range); // RayTrace - ALL




	device->EventEnd(cmd); // RayTraceScene
}
void RayTraceSceneBVH(CommandList cmd)
{
	GraphicsDevice* device = GetDevice();

	device->EventBegin("RayTraceSceneBVH", cmd);
	device->BindPipelineState(&PSO_debug[DEBUGRENDERING_RAYTRACE_BVH], cmd);
	sceneBVH.Bind(PS, cmd);
	device->Draw(3, 0, cmd);
	device->EventEnd(cmd);
}

void GenerateClouds(const Texture2D* dst, UINT refinementCount, float randomness, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();
	device->EventBegin("Cloud Generator", cmd);

	TextureDesc src_desc = wiTextureHelper::getRandom64x64()->GetDesc();

	TextureDesc dst_desc = dst->GetDesc();
	assert(dst_desc.BindFlags & BIND_UNORDERED_ACCESS);

	device->BindResource(CS, wiTextureHelper::getRandom64x64(), TEXSLOT_ONDEMAND0, cmd);
	device->BindUAV(CS, dst, 0, cmd);

	CloudGeneratorCB cb;
	cb.xNoiseTexDim = XMFLOAT2((float)src_desc.Width, (float)src_desc.Height);
	cb.xRandomness = randomness;
	if (refinementCount == 0)
	{
		cb.xRefinementCount = std::max(1u, (UINT)log2(dst_desc.Width));
	}
	else
	{
		cb.xRefinementCount = refinementCount;
	}
	device->UpdateBuffer(&constantBuffers[CBTYPE_CLOUDGENERATOR], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_CLOUDGENERATOR], CB_GETBINDSLOT(CloudGeneratorCB), cmd);

	device->BindComputeShader(computeShaders[CSTYPE_CLOUDGENERATOR], cmd);
	device->Dispatch((UINT)ceilf(dst_desc.Width / (float)CLOUDGENERATOR_BLOCKSIZE), (UINT)ceilf(dst_desc.Height / (float)CLOUDGENERATOR_BLOCKSIZE), 1, cmd);

	device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
	device->UnbindUAVs(0, 1, cmd);


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
				rect_xywh newRect = rect_xywh(0, 0, decal.texture->GetDesc().Width + atlasClampBorder * 2, decal.texture->GetDesc().Height + atlasClampBorder * 2);
				packedDecals[decal.texture] = newRect;

				repackAtlas_Decal = true;
			}
		}
	}

	// Update atlas texture if it is invalidated:
	GraphicsDevice* device = GetDevice();
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
			desc.Width = (UINT)bins[0].size.w;
			desc.Height = (UINT)bins[0].size.h;
			desc.MipLevels = 0;
			desc.ArraySize = 1;
			desc.Format = FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = USAGE_DEFAULT;
			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			decalAtlas.RequestIndependentUnorderedAccessResourcesForMIPs(true);

			device->CreateTexture2D(&desc, nullptr, &decalAtlas);
			device->SetName(&decalAtlas, "decalAtlas");
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
	GraphicsDevice* device = GetDevice();

	using namespace wiRectPacker;

	const Scene& scene = GetScene();

	if (repackAtlas_Decal)
	{
		for (UINT mip = 0; mip < decalAtlas.GetDesc().MipLevels; ++mip)
		{
			for (auto& it : packedDecals)
			{
				if (mip < it.first->GetDesc().MipLevels)
				{
					CopyTexture2D(&decalAtlas, mip, (it.second.x >> mip) + atlasClampBorder, (it.second.y >> mip) + atlasClampBorder, it.first, mip, cmd, BORDEREXPAND_CLAMP);
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
	GraphicsDevice* device = GetDevice();

	using namespace wiRectPacker;

	// Gather all object lightmap textures:
	for (size_t objectIndex = 0; objectIndex < scene.objects.GetCount(); ++objectIndex)
	{
		ObjectComponent& object = scene.objects[objectIndex];
		bool refresh = false;

		if (object.lightmap != nullptr && object.lightmapWidth == 0)
		{
			// If we get here, it means that the lightmap GPU texture contains the rendered lightmap, but the CPU-side data was erased.
			//	In this case, we delete the GPU side lightmap data from the object and the atlas too.
			packedLightmaps.erase(object.lightmap.get());
			object.lightmap.reset(nullptr);
			repackAtlas_Lightmap = true;
			refresh = false;
		}

		if (object.IsLightmapRenderRequested())
		{
			refresh = true;

			if (object.lightmapIterationCount == 0)
			{
				if (object.lightmap != nullptr)
				{
					packedLightmaps.erase(object.lightmap.get());
				}

				if (RTFormat_lightmap_object == FORMAT_R32G32B32A32_FLOAT)
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
				desc.Format = RTFormat_lightmap_object;

				object.lightmap.reset(new Texture2D);
				HRESULT hr = device->CreateTexture2D(&desc, nullptr, object.lightmap.get());
				assert(SUCCEEDED(hr));
				device->SetName(object.lightmap.get(), "objectLightmap");
			}
			object.lightmapIterationCount++;
		}

		if (!object.lightmapTextureData.empty() && object.lightmap == nullptr)
		{
			refresh = true;
			// Create a GPU-side per object lighmap if there is none yet, so that copying into atlas can be done efficiently:
			object.lightmap.reset(new Texture2D);
			wiTextureHelper::CreateTexture(*object.lightmap.get(), object.lightmapTextureData.data(), object.lightmapWidth, object.lightmapHeight, object.GetLightmapFormat());
		}

		if (object.lightmap != nullptr)
		{
			if (packedLightmaps.find(object.lightmap.get()) == packedLightmaps.end())
			{
				// we need to pack this lightmap texture into the atlas
				rect_xywh newRect = rect_xywh(0, 0, object.lightmap->GetDesc().Width + atlasClampBorder * 2, object.lightmap->GetDesc().Height + atlasClampBorder * 2);
				packedLightmaps[object.lightmap.get()] = newRect;

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
			desc.Width = (UINT)bins[0].size.w;
			desc.Height = (UINT)bins[0].size.h;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = RTFormat_lightmap_global;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = USAGE_DEFAULT;
			desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			device->CreateTexture2D(&desc, nullptr, &globalLightmap);
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

			if (object.lightmap != nullptr && packedLightmaps.count(object.lightmap.get()) > 0)
			{
				const TextureDesc& desc = globalLightmap.GetDesc();

				rect_xywh rect = packedLightmaps.at(object.lightmap.get());

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
	GraphicsDevice* device = GetDevice();
	const Scene& scene = GetScene();

	device->EventBegin("RenderObjectLightMap", cmd);

	const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);
	assert(!mesh.vertex_atlas.empty());
	assert(mesh.vertexBuffer_ATL != nullptr);

	const TextureDesc& desc = object.lightmap->GetDesc();

	const Texture2D* rts[] = { object.lightmap.get() };
	device->BindRenderTargets(1, rts, nullptr, cmd);

	const uint32_t lightmapIterationCount = std::max(1u, object.lightmapIterationCount) - 1; // ManageLightMapAtlas incremented before refresh

	if (lightmapIterationCount == 0)
	{
		float clearColor[4] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[0], clearColor, cmd);
	}

	ViewPort vp;
	vp.Width = (float)desc.Width;
	vp.Height = (float)desc.Height;
	device->BindViewports(1, &vp, cmd);

	const TransformComponent& transform = scene.transforms[object.transform_index];

	// Note: using InstancePrev, because we just need the matrix, nothing else here...
	GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(InstancePrev), cmd);
	volatile InstancePrev* instance = (volatile InstancePrev*)mem.data;
	instance->Create(transform.world);

	const GPUBuffer* vbs[] = {
		mesh.vertexBuffer_POS.get(),
		mesh.vertexBuffer_ATL.get(),
		mem.buffer,
	};
	UINT strides[] = {
		sizeof(MeshComponent::Vertex_POS),
		sizeof(MeshComponent::Vertex_TEX),
		sizeof(InstancePrev),
	};
	UINT offsets[] = {
		0,
		0,
		mem.offset,
	};
	device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, cmd);
	device->BindIndexBuffer(mesh.indexBuffer.get(), mesh.GetIndexFormat(), 0, cmd);

	TracedRenderingCB cb;
	XMFLOAT4 halton = wiMath::GetHaltonSequence(lightmapIterationCount); // for jittering the rasterization (good for eliminating atlas border artifacts)
	cb.xTracePixelOffset.x = (halton.x * 2 - 1) / vp.Width;
	cb.xTracePixelOffset.y = (halton.y * 2 - 1) / vp.Height;
	cb.xTracePixelOffset.x *= 1.4f;	// boost the jitter by a bit
	cb.xTracePixelOffset.y *= 1.4f;	// boost the jitter by a bit
	cb.xTraceRandomSeed = renderTime; // random seed
	cb.xTraceAccumulationFactor = 1.0f / (lightmapIterationCount + 1.0f); // accumulation factor (alpha)
	cb.xTraceUserData.x = raytraceBounceCount;
	device->UpdateBuffer(&constantBuffers[CBTYPE_RAYTRACE], &cb, cmd);
	device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(TracedRenderingCB), cmd);
	device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(TracedRenderingCB), cmd);

	// Render direct lighting part:
	device->BindPipelineState(&PSO_renderlightmap_direct, cmd);
	device->DrawIndexedInstanced((UINT)mesh.indices.size(), 1, 0, 0, 0, cmd);

	if (raytraceBounceCount > 0)
	{
		// Render indirect lighting part:
		device->BindPipelineState(&PSO_renderlightmap_indirect, cmd);
		device->DrawIndexedInstanced((UINT)mesh.indices.size(), 1, 0, 0, 0, cmd);
	}

	device->BindRenderTargets(0, nullptr, nullptr, cmd);

	device->EventEnd(cmd);
}
void RefreshLightmapAtlas(CommandList cmd)
{
	const Scene& scene = GetScene();
	GraphicsDevice* device = GetDevice();

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
					if (object.lightmap != nullptr)
					{
						const auto& rec = packedLightmaps.at(object.lightmap.get());
						CopyTexture2D(&globalLightmap, 0, rec.x + atlasClampBorder, rec.y + atlasClampBorder, object.lightmap.get(), 0, cmd);
					}
				}
			}
			else
			{
				// If atlas was not repacked, we only copy refreshed object lightmaps:
				for (uint32_t objectIndex : lightmapsToRefresh)
				{
					const ObjectComponent& object = scene.objects[objectIndex];
					const auto& rec = packedLightmaps.at(object.lightmap.get());
					CopyTexture2D(&globalLightmap, 0, rec.x + atlasClampBorder, rec.y + atlasClampBorder, object.lightmap.get(), 0, cmd);
				}
			}
			device->EventEnd(cmd);

		}

		wiProfiler::EndRange(range);
	}
}

const Texture2D* GetGlobalLightmap()
{
	if (globalLightmap.IsValid())
	{
		return &globalLightmap;
	}
	return wiTextureHelper::getTransparent();
}

void BindCommonResources(CommandList cmd)
{
	GraphicsDevice* device = GetDevice();

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
	device->BindResources(VS, resources, SBSLOT_ENTITYARRAY, ARRAYSIZE(resources), cmd);
	device->BindResources(PS, resources, SBSLOT_ENTITYARRAY, ARRAYSIZE(resources), cmd);
	device->BindResources(CS, resources, SBSLOT_ENTITYARRAY, ARRAYSIZE(resources), cmd);
}

void UpdateFrameCB(CommandList cmd)
{
	const Scene& scene = GetScene();

	FrameCB cb;

	cb.g_xFrame_ConstantOne = 1;
	cb.g_xFrame_ScreenWidthHeight = float2((float)GetDevice()->GetScreenWidth(), (float)GetDevice()->GetScreenHeight());
	cb.g_xFrame_ScreenWidthHeight_Inverse = float2(1.0f / cb.g_xFrame_ScreenWidthHeight.x, 1.0f / cb.g_xFrame_ScreenWidthHeight.y);
	cb.g_xFrame_InternalResolution = float2((float)GetInternalResolution().x, (float)GetInternalResolution().y);
	cb.g_xFrame_InternalResolution_Inverse = float2(1.0f / cb.g_xFrame_InternalResolution.x, 1.0f / cb.g_xFrame_InternalResolution.y);
	cb.g_xFrame_Gamma = GetGamma();
	cb.g_xFrame_SunColor = scene.weather.sunColor;
	cb.g_xFrame_SunDirection = scene.weather.sunDirection;
	cb.g_xFrame_ShadowCascadeCount = CASCADE_COUNT;
	cb.g_xFrame_Ambient = scene.weather.ambient;
	cb.g_xFrame_Cloudiness = scene.weather.cloudiness;
	cb.g_xFrame_CloudScale = scene.weather.cloudScale;
	cb.g_xFrame_Fog = float3(scene.weather.fogStart, scene.weather.fogEnd, scene.weather.fogHeight);
	cb.g_xFrame_Horizon = scene.weather.horizon;
	cb.g_xFrame_Zenith = scene.weather.zenith;
	cb.g_xFrame_SpecularAA = SPECULARAA;
	cb.g_xFrame_VoxelRadianceDataSize = voxelSceneData.voxelsize;
	cb.g_xFrame_VoxelRadianceDataSize_Inverse = 1.0f / (float)cb.g_xFrame_VoxelRadianceDataSize;
	cb.g_xFrame_VoxelRadianceDataRes = GetVoxelRadianceEnabled() ? (uint)voxelSceneData.res : 0;
	cb.g_xFrame_VoxelRadianceDataRes_Inverse = 1.0f / (float)cb.g_xFrame_VoxelRadianceDataRes;
	cb.g_xFrame_VoxelRadianceDataMIPs = voxelSceneData.mips;
	cb.g_xFrame_VoxelRadianceNumCones = std::max(std::min(voxelSceneData.numCones, 16u), 1u);
	cb.g_xFrame_VoxelRadianceNumCones_Inverse = 1.0f / (float)cb.g_xFrame_VoxelRadianceNumCones;
	cb.g_xFrame_VoxelRadianceRayStepSize = voxelSceneData.rayStepSize;
	cb.g_xFrame_VoxelRadianceReflectionsEnabled = voxelSceneData.reflectionsEnabled;
	cb.g_xFrame_VoxelRadianceDataCenter = voxelSceneData.center;
	cb.g_xFrame_AdvancedRefractions = GetAdvancedRefractionsEnabled() ? 1 : 0;
	cb.g_xFrame_EntityCullingTileCount = GetEntityCullingTileCount();
	cb.g_xFrame_TransparentShadowsEnabled = TRANSPARENTSHADOWSENABLED;
	cb.g_xFrame_GlobalEnvProbeIndex = -1;
	cb.g_xFrame_EnvProbeMipCount = 0;
	cb.g_xFrame_EnvProbeMipCount_Inverse = 1.0f;
	if (scene.probes.GetCount() > 0)
	{
		cb.g_xFrame_GlobalEnvProbeIndex = 0; // for now, the global envprobe will be the first probe in the array. Easy change later on if required...
	}
	if (textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY] != nullptr)
	{
		cb.g_xFrame_EnvProbeMipCount = static_cast<Texture2D*>(textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY])->GetDesc().MipLevels;
		cb.g_xFrame_EnvProbeMipCount_Inverse = 1.0f / (float)cb.g_xFrame_EnvProbeMipCount;
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
	cb.g_xFrame_VoxelRadianceRetargetted = voxelSceneData.centerChangedThisFrame ? 1 : 0;
	cb.g_xFrame_WindRandomness = scene.weather.windRandomness;
	cb.g_xFrame_WindWaveSize = scene.weather.windWaveSize;
	cb.g_xFrame_WindDirection = scene.weather.windDirection;
	cb.g_xFrame_StaticSkyGamma = 0.0f;
	if (enviroMap != nullptr)
	{
		bool hdr = !GetDevice()->IsFormatUnorm(enviroMap->GetDesc().Format);
		cb.g_xFrame_StaticSkyGamma = hdr ? 1.0f : cb.g_xFrame_Gamma;
	}
	cb.g_xFrame_FrameCount = (uint)GetDevice()->GetFrameCount();
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

	const auto& camera = GetCamera();
	const auto& prevCam = GetPrevCamera();
	const auto& reflCam = GetRefCamera();

	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_VP, XMMatrixTranspose(camera.GetViewProjection()));
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_View, XMMatrixTranspose(camera.GetView()));
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_Proj, XMMatrixTranspose(camera.GetProjection()));
	cb.g_xFrame_MainCamera_CamPos = camera.Eye;
	cb.g_xFrame_MainCamera_DistanceFromOrigin = XMVectorGetX(XMVector3Length(XMLoadFloat3(&cb.g_xFrame_MainCamera_CamPos)));
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_PrevV, XMMatrixTranspose(prevCam.GetView()));
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_PrevP, XMMatrixTranspose(prevCam.GetProjection()));
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_PrevVP, XMMatrixTranspose(prevCam.GetViewProjection()));
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_PrevInvVP, XMMatrixTranspose(prevCam.GetInvViewProjection()));
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_ReflVP, XMMatrixTranspose(reflCam.GetViewProjection()));
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_InvV, XMMatrixTranspose(camera.GetInvView()));
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_InvP, XMMatrixTranspose(camera.GetInvProjection()));
	XMStoreFloat4x4(&cb.g_xFrame_MainCamera_InvVP, XMMatrixTranspose(camera.GetInvViewProjection()));
	cb.g_xFrame_MainCamera_At = camera.At;
	cb.g_xFrame_MainCamera_Up = camera.Up;
	cb.g_xFrame_MainCamera_ZNearP = camera.zNearP;
	cb.g_xFrame_MainCamera_ZFarP = camera.zFarP;
	cb.g_xFrame_MainCamera_ZNearP_Recip = 1.0f / std::max(0.0001f, cb.g_xFrame_MainCamera_ZNearP);
	cb.g_xFrame_MainCamera_ZFarP_Recip = 1.0f / std::max(0.0001f, cb.g_xFrame_MainCamera_ZFarP);
	cb.g_xFrame_MainCamera_ZRange = abs(cb.g_xFrame_MainCamera_ZFarP - cb.g_xFrame_MainCamera_ZNearP);
	cb.g_xFrame_MainCamera_ZRange_Recip = 1.0f / std::max(0.0001f, cb.g_xFrame_MainCamera_ZRange);
	cb.g_xFrame_FrustumPlanesWS[0] = camera.frustum.getLeftPlane();
	cb.g_xFrame_FrustumPlanesWS[1] = camera.frustum.getRightPlane();
	cb.g_xFrame_FrustumPlanesWS[2] = camera.frustum.getTopPlane();
	cb.g_xFrame_FrustumPlanesWS[3] = camera.frustum.getBottomPlane();
	cb.g_xFrame_FrustumPlanesWS[4] = camera.frustum.getNearPlane();
	cb.g_xFrame_FrustumPlanesWS[5] = camera.frustum.getFarPlane();

	cb.g_xFrame_WorldBoundsMin = scene.bounds.getMin();
	cb.g_xFrame_WorldBoundsMax = scene.bounds.getMax();
	cb.g_xFrame_WorldBoundsExtents.x = abs(cb.g_xFrame_WorldBoundsMax.x - cb.g_xFrame_WorldBoundsMin.x);
	cb.g_xFrame_WorldBoundsExtents.y = abs(cb.g_xFrame_WorldBoundsMax.y - cb.g_xFrame_WorldBoundsMin.y);
	cb.g_xFrame_WorldBoundsExtents.z = abs(cb.g_xFrame_WorldBoundsMax.z - cb.g_xFrame_WorldBoundsMin.z);
	cb.g_xFrame_WorldBoundsExtents_Inverse.x = 1.0f / cb.g_xFrame_WorldBoundsExtents.x;
	cb.g_xFrame_WorldBoundsExtents_Inverse.y = 1.0f / cb.g_xFrame_WorldBoundsExtents.y;
	cb.g_xFrame_WorldBoundsExtents_Inverse.z = 1.0f / cb.g_xFrame_WorldBoundsExtents.z;

	GetDevice()->UpdateBuffer(&constantBuffers[CBTYPE_FRAME], &cb, cmd);
}
void UpdateCameraCB(const CameraComponent& camera, CommandList cmd)
{
	CameraCB cb;

	XMStoreFloat4x4(&cb.g_xCamera_VP, XMMatrixTranspose(camera.GetViewProjection()));
	XMStoreFloat4x4(&cb.g_xCamera_View, XMMatrixTranspose(camera.GetView()));
	XMStoreFloat4x4(&cb.g_xCamera_Proj, XMMatrixTranspose(camera.GetProjection()));
	cb.g_xCamera_CamPos = camera.Eye;

	GetDevice()->UpdateBuffer(&constantBuffers[CBTYPE_CAMERA], &cb, cmd);
}

APICB apiCB[COMMANDLIST_COUNT];
void SetClipPlane(const XMFLOAT4& clipPlane, CommandList cmd)
{
	apiCB[cmd].g_xClipPlane = clipPlane;
	GetDevice()->UpdateBuffer(&constantBuffers[CBTYPE_API], &apiCB[cmd], cmd);
}
void SetAlphaRef(float alphaRef, CommandList cmd)
{
	if (alphaRef != apiCB[cmd].g_xAlphaRef)
	{
		apiCB[cmd].g_xAlphaRef = alphaRef;
		GetDevice()->UpdateBuffer(&constantBuffers[CBTYPE_API], &apiCB[cmd], cmd);
	}
}
void BindGBufferTextures(const Texture2D* slot0, const Texture2D* slot1, const Texture2D* slot2, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();

	device->BindResource(PS, slot0, TEXSLOT_GBUFFER0, cmd);
	device->BindResource(PS, slot1, TEXSLOT_GBUFFER1, cmd);
	device->BindResource(PS, slot2, TEXSLOT_GBUFFER2, cmd);

	device->BindResource(CS, slot0, TEXSLOT_GBUFFER0, cmd);
	device->BindResource(CS, slot1, TEXSLOT_GBUFFER1, cmd);
	device->BindResource(CS, slot2, TEXSLOT_GBUFFER2, cmd);
}
void BindDepthTextures(const Texture2D* depth, const Texture2D* linearDepth, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();

	device->BindResource(PS, depth, TEXSLOT_DEPTH, cmd);
	device->BindResource(VS, depth, TEXSLOT_DEPTH, cmd);
	device->BindResource(GS, depth, TEXSLOT_DEPTH, cmd);
	device->BindResource(CS, depth, TEXSLOT_DEPTH, cmd);

	device->BindResource(PS, linearDepth, TEXSLOT_LINEARDEPTH, cmd);
	device->BindResource(VS, linearDepth, TEXSLOT_LINEARDEPTH, cmd);
	device->BindResource(GS, linearDepth, TEXSLOT_LINEARDEPTH, cmd);
	device->BindResource(CS, linearDepth, TEXSLOT_LINEARDEPTH, cmd);
}


const Texture2D* ComputeLuminance(const Texture2D* sourceImage, CommandList cmd)
{
	GraphicsDevice* device = GetDevice();

	static std::unique_ptr<Texture2D> luminance_map;
	static std::vector<Texture2D*> luminance_avg(0);
	if (luminance_map == nullptr)
	{
		for (auto& x : luminance_avg)
		{
			SAFE_DELETE(x);
		}
		luminance_avg.clear();

		TextureDesc desc;
		desc.Width = 256;
		desc.Height = desc.Width;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = FORMAT_R32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = USAGE_DEFAULT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		luminance_map.reset(new Texture2D);
		device->CreateTexture2D(&desc, nullptr, luminance_map.get());

		while (desc.Width > 1)
		{
			desc.Width = std::max(desc.Width / 16, 1u);
			desc.Height = desc.Width;

			Texture2D* tex = new Texture2D;
			device->CreateTexture2D(&desc, nullptr, tex);

			luminance_avg.push_back(tex);
		}
	}
	if (luminance_map != nullptr)
	{
		device->EventBegin("Compute Luminance", cmd);

		// Pass 1 : Create luminance map from scene tex
		TextureDesc luminance_map_desc = luminance_map->GetDesc();
		device->BindComputeShader(computeShaders[CSTYPE_LUMINANCE_PASS1], cmd);
		device->BindResource(CS, sourceImage, TEXSLOT_ONDEMAND0, cmd);
		device->BindUAV(CS, luminance_map.get(), 0, cmd);
		device->Dispatch(luminance_map_desc.Width/16, luminance_map_desc.Height/16, 1, cmd);

		// Pass 2 : Reduce for average luminance until we got an 1x1 texture
		TextureDesc luminance_avg_desc;
		for (size_t i = 0; i < luminance_avg.size(); ++i)
		{
			luminance_avg_desc = luminance_avg[i]->GetDesc();
			device->BindComputeShader(computeShaders[CSTYPE_LUMINANCE_PASS2], cmd);
			device->BindUAV(CS, luminance_avg[i], 0, cmd);
			if (i > 0)
			{
				device->BindResource(CS, luminance_avg[i-1], TEXSLOT_ONDEMAND0, cmd);
			}
			else
			{
				device->BindResource(CS, luminance_map.get(), TEXSLOT_ONDEMAND0, cmd);
			}
			device->Dispatch(luminance_avg_desc.Width, luminance_avg_desc.Height, 1, cmd);
		}


		device->UnbindUAVs(0, 1, cmd);

		device->EventEnd(cmd);

		return luminance_avg.back();
	}

	return nullptr;
}

const XMFLOAT4& GetWaterPlane()
{
	return waterPlane;
}


RAY GetPickRay(long cursorX, long cursorY) 
{
	const CameraComponent& camera = GetCamera();
	XMMATRIX V = camera.GetView();
	XMMATRIX P = camera.GetProjection();
	XMMATRIX W = XMMatrixIdentity();
	XMVECTOR& lineStart = XMVector3Unproject(XMVectorSet((float)cursorX, (float)cursorY, 1, 1), 0, 0, camera.width, camera.height, 0.0f, 1.0f, P, V, W);
	XMVECTOR& lineEnd = XMVector3Unproject(XMVectorSet((float)cursorX, (float)cursorY, 0, 1), 0, 0, camera.width, camera.height, 0.0f, 1.0f, P, V, W);
	XMVECTOR& rayDirection = XMVector3Normalize(XMVectorSubtract(lineEnd, lineStart));
	return RAY(lineStart, rayDirection);
}

void AddRenderableBox(const XMFLOAT4X4& boxMatrix, const XMFLOAT4& color)
{
	renderableBoxes.push_back(pair<XMFLOAT4X4,XMFLOAT4>(boxMatrix,color));
}
void AddRenderableLine(const RenderableLine& line)
{
	renderableLines.push_back(line);
}
void AddRenderablePoint(const RenderablePoint& point)
{
	renderablePoints.push_back(point);
}

void AddDeferredMIPGen(const Texture2D* tex)
{
	deferredMIPGenLock.lock();
	deferredMIPGens.insert(tex);
	deferredMIPGenLock.unlock();
}




void SetResolutionScale(float value) { RESOLUTIONSCALE = value; }
float GetResolutionScale() { return RESOLUTIONSCALE; }
int GetShadowRes2D() { return SHADOWRES_2D; }
int GetShadowResCube() { return SHADOWRES_CUBE; }
void SetTransparentShadowsEnabled(float value) { TRANSPARENTSHADOWSENABLED = value; }
float GetTransparentShadowsEnabled() { return TRANSPARENTSHADOWSENABLED; }
XMUINT2 GetInternalResolution() { return XMUINT2((UINT)ceilf(GetDevice()->GetScreenWidth()*GetResolutionScale()), (UINT)ceilf(GetDevice()->GetScreenHeight()*GetResolutionScale())); }
bool ResolutionChanged()
{
	//detect internal resolution change:
	static float _savedresscale = GetResolutionScale();
	static uint64_t lastFrameInternalResChange = 0;
	if (_savedresscale != GetResolutionScale() || lastFrameInternalResChange == GetDevice()->GetFrameCount())
	{
		_savedresscale = GetResolutionScale();
		lastFrameInternalResChange = GetDevice()->GetFrameCount();
		return true;
	}

	// detect device resolution change:
	return GetDevice()->ResolutionChanged();
}
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
void SetAlphaCompositionEnabled(bool enabled) { ALPHACOMPOSITIONENABLED = enabled; }
bool GetAlphaCompositionEnabled() { return ALPHACOMPOSITIONENABLED; }
void SetOcclusionCullingEnabled(bool value)
{
	static bool initialized = false;

	if (!initialized && value == true)
	{
		initialized = true;

		GPUQueryDesc desc;
		desc.Type = GPU_QUERY_TYPE_OCCLUSION_PREDICATE;

		for (int i = 0; i < ARRAYSIZE(occlusionQueries); ++i)
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
int GetVoxelRadianceResolution() { return voxelSceneData.res; }
void SetVoxelRadianceNumCones(int value) { voxelSceneData.numCones = value; }
int GetVoxelRadianceNumCones() { return voxelSceneData.numCones; }
float GetVoxelRadianceRayStepSize() { return voxelSceneData.rayStepSize; }
void SetVoxelRadianceRayStepSize(float value) { voxelSceneData.rayStepSize = value; }
void SetSpecularAAParam(float value) { SPECULARAA = value; }
float GetSpecularAAParam() { return SPECULARAA; }
void SetAdvancedRefractionsEnabled(bool value) { advancedRefractions = value; }
bool GetAdvancedRefractionsEnabled() { return advancedRefractions; }
bool IsRequestedReflectionRendering() { return requestReflectionRendering; }
void SetEnvironmentMap(wiGraphics::Texture2D* tex) { enviroMap = tex; }
const Texture2D* GetEnvironmentMap() { return enviroMap; }
void SetGameSpeed(float value) { GameSpeed = std::max(0.0f, value); }
float GetGameSpeed() { return GameSpeed; }
void SetOceanEnabled(bool enabled)
{
	if (enabled)
	{
		const Scene& scene = GetScene();
		ocean.reset(new wiOcean(scene.weather));
	}
	else
	{
		ocean.reset();
	}
}
bool GetOceanEnabled() { return ocean != nullptr; }
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

}
