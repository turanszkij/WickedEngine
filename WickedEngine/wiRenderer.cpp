#include "wiRenderer.h"
#include "wiHairParticle.h"
#include "wiEmittedParticle.h"
#include "wiSprite.h"
#include "wiScene.h"
#include "wiHelper.h"
#include "wiMath.h"
#include "wiTextureHelper.h"
#include "wiEnums.h"
#include "wiRectPacker.h"
#include "wiBackLog.h"
#include "wiProfiler.h"
#include "wiOcean.h"
#include "wiGPUSortLib.h"
#include "wiAllocators.h"
#include "wiGPUBVH.h"
#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiEvent.h"
#include "wiPlatform.h"
#include "wiSheenLUT.h"
#include "wiShaderCompiler.h"

#include "shaders/ShaderInterop_Postprocess.h"
#include "shaders/ShaderInterop_Skinning.h"
#include "shaders/ShaderInterop_Raytracing.h"
#include "shaders/ShaderInterop_BVH.h"
#include "shaders/ShaderInterop_Utility.h"
#include "shaders/ShaderInterop_Paint.h"

#include <algorithm>
#include <array>

using namespace wiGraphics;
using namespace wiScene;
using namespace wiECS;
using namespace wiAllocators;

namespace wiRenderer
{

std::shared_ptr<GraphicsDevice> device;

Shader				shaders[SHADERTYPE_COUNT];
Texture				textures[TEXTYPE_COUNT];
InputLayout			inputLayouts[ILTYPE_COUNT];
RasterizerState		rasterizers[RSTYPE_COUNT];
DepthStencilState	depthStencils[DSSTYPE_COUNT];
BlendState			blendStates[BSTYPE_COUNT];
GPUBuffer			constantBuffers[CBTYPE_COUNT];
GPUBuffer			resourceBuffers[RBTYPE_COUNT];
Sampler				samplers[SSLOT_COUNT];

std::string SHADERPATH = "shaders/";
std::string SHADERSOURCEPATH = "../WickedEngine/shaders/";

LinearAllocator renderFrameAllocators[COMMANDLIST_COUNT]; // can be used by graphics threads
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
bool TRANSPARENTSHADOWSENABLED = true;
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
bool advancedLightCulling = true;
bool variableRateShadingClassification = false;
bool variableRateShadingClassificationDebug = false;
bool ldsSkinningEnabled = true;
float GameSpeed = 1;
bool debugLightCulling = false;
bool occlusionCulling = false;
bool temporalAA = false;
bool temporalAADEBUG = false;
uint32_t raytraceBounceCount = 2;
bool raytraceDebugVisualizer = false;
bool raytracedShadows = false;
bool tessellationEnabled = true;
bool disableAlbedoMaps = false;
bool SCREENSPACESHADOWS = false;


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

Texture shadowMapArray_2D;
Texture shadowMapArray_Cube;
Texture shadowMapArray_Transparent_2D;
Texture shadowMapArray_Transparent_Cube;
std::vector<RenderPass> renderpasses_shadow2D;
std::vector<RenderPass> renderpasses_shadowCube;

std::vector<std::pair<XMFLOAT4X4, XMFLOAT4>> renderableBoxes;
std::vector<std::pair<SPHERE, XMFLOAT4>> renderableSpheres;
std::vector<std::pair<CAPSULE, XMFLOAT4>> renderableCapsules;
std::vector<RenderableLine> renderableLines;
std::vector<RenderableLine2D> renderableLines2D;
std::vector<RenderablePoint> renderablePoints;
std::vector<RenderableTriangle> renderableTriangles_solid;
std::vector<RenderableTriangle> renderableTriangles_wireframe;
std::vector<PaintRadius> paintrads;

wiSpinLock deferredMIPGenLock;
std::vector<std::pair<std::shared_ptr<wiResource>, bool>> deferredMIPGens;


bool volumetric_clouds_precomputed = false;
Texture texture_shapeNoise;
Texture texture_detailNoise;
Texture texture_curlNoise;
Texture texture_weatherMap;


void SetDevice(std::shared_ptr<GraphicsDevice> newDevice)
{
	device = newDevice;

	SamplerDesc samplerDesc;
	samplerDesc.Filter = FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = COMPARISON_NEVER;
	samplerDesc.BorderColor = SAMPLER_BORDER_COLOR_TRANSPARENT_BLACK;
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


	StaticSampler sam;

	sam.sampler = samplers[SSLOT_CMP_DEPTH];
	sam.slot = SSLOT_CMP_DEPTH;
	device->SetCommonSampler(&sam);

	sam.sampler = samplers[SSLOT_LINEAR_MIRROR];
	sam.slot = SSLOT_LINEAR_MIRROR;
	device->SetCommonSampler(&sam);

	sam.sampler = samplers[SSLOT_LINEAR_CLAMP];
	sam.slot = SSLOT_LINEAR_CLAMP;
	device->SetCommonSampler(&sam);

	sam.sampler = samplers[SSLOT_LINEAR_WRAP];
	sam.slot = SSLOT_LINEAR_WRAP;
	device->SetCommonSampler(&sam);

	sam.sampler = samplers[SSLOT_POINT_MIRROR];
	sam.slot = SSLOT_POINT_MIRROR;
	device->SetCommonSampler(&sam);

	sam.sampler = samplers[SSLOT_POINT_WRAP];
	sam.slot = SSLOT_POINT_WRAP;
	device->SetCommonSampler(&sam);

	sam.sampler = samplers[SSLOT_POINT_CLAMP];
	sam.slot = SSLOT_POINT_CLAMP;
	device->SetCommonSampler(&sam);

	sam.sampler = samplers[SSLOT_ANISO_CLAMP];
	sam.slot = SSLOT_ANISO_CLAMP;
	device->SetCommonSampler(&sam);

	sam.sampler = samplers[SSLOT_ANISO_WRAP];
	sam.slot = SSLOT_ANISO_WRAP;
	device->SetCommonSampler(&sam);

	sam.sampler = samplers[SSLOT_ANISO_MIRROR];
	sam.slot = SSLOT_ANISO_MIRROR;
	device->SetCommonSampler(&sam);
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

	inline void Create(size_t meshIndex, size_t instanceIndex, float distance)
	{
		hash = 0;

		assert(meshIndex < 0x00FFFFFF);
		hash |= (uint32_t)(meshIndex & 0x00FFFFFF);
		hash |= ((uint32_t)distance & 0xFF) << 24;

		instance = (uint32_t)instanceIndex;
	}

	inline uint32_t GetMeshIndex() const
	{
		return hash & 0x00FFFFFF;
	}
	inline uint32_t GetInstanceIndex() const
	{
		return instance;
	}
};

// This is just a utility that points to a linear array of render batches:
struct RenderQueue
{
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

struct Instance
{
	XMFLOAT4 mat0;
	XMFLOAT4 mat1;
	XMFLOAT4 mat2;
	XMUINT4 userdata;

	inline void Create(
		const XMFLOAT4X4& matIn,
		const XMFLOAT4& colorIn = XMFLOAT4(1, 1, 1, 1),
		float dither = 0,
		uint32_t subInstance = 0,
		const XMFLOAT4& emissiveColor = XMFLOAT4(1, 1, 1, 1)
	) volatile
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

		userdata.z = wiMath::CompressColor(emissiveColor);
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
	OBJECTRENDERING_DOUBLESIDED_BACKSIDE,
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
PipelineState PSO_object_wire_tessellation;

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


ILTYPES GetILTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest, bool transparent)
{
	ILTYPES realVL = ILTYPE_OBJECT_POS_TEX;

	switch (renderPass)
	{
	case RENDERPASS_MAIN:
	case RENDERPASS_ENVMAPCAPTURE:
	case RENDERPASS_VOXELIZE:
		realVL = ILTYPE_OBJECT_COMMON;
		break;
	case RENDERPASS_PREPASS:
		if (tessellation)
		{
			realVL = ILTYPE_OBJECT_POS_PREVPOS_TEX; // tessellation needs tex because displacement mapping
		}
		else
		{
			if (alphatest)
			{
				realVL = ILTYPE_OBJECT_POS_PREVPOS_TEX;
			}
			else
			{
				realVL = ILTYPE_OBJECT_POS_PREVPOS;
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
	case RENDERPASS_PREPASS:
		if (tessellation)
		{
			if (alphatest)
			{
				realVS = VSTYPE_OBJECT_PREPASS_ALPHATEST_TESSELLATION;
			}
			else
			{
				realVS = VSTYPE_OBJECT_PREPASS_TESSELLATION;
			}
		}
		else
		{
			if (alphatest)
			{
				realVS = VSTYPE_OBJECT_PREPASS_ALPHATEST;
			}
			else
			{
				realVS = VSTYPE_OBJECT_PREPASS;
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
		if (transparent)
		{
			realVS = VSTYPE_SHADOWCUBEMAPRENDER_TRANSPARENT;
		}
		else
		{
			if (alphatest)
			{
				realVS = VSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST;
			}
			else
			{
				realVS = VSTYPE_SHADOWCUBEMAPRENDER;
			}
		}
		break;
	case RENDERPASS_VOXELIZE:
		realVS = VSTYPE_VOXELIZER;
		break;
	}

	return realVS;
}
SHADERTYPE GetGSTYPE(RENDERPASS renderPass, bool alphatest, bool transparent)
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
		if (transparent)
		{
			realGS = GSTYPE_SHADOWCUBEMAPRENDER_TRANSPARENT_EMULATION;
		}
		else
		{
			if (alphatest)
			{
				realGS = GSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST_EMULATION;
			}
			else
			{
				realGS = GSTYPE_SHADOWCUBEMAPRENDER_EMULATION;
			}
		}
		break;
	}

	return realGS;
}
SHADERTYPE GetHSTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest)
{
	if (tessellation)
	{
		switch (renderPass)
		{
		case RENDERPASS_PREPASS:
			if (alphatest)
			{
				return HSTYPE_OBJECT_PREPASS_ALPHATEST;
			}
			else
			{
				return HSTYPE_OBJECT_PREPASS;
			}
			break;
		case RENDERPASS_MAIN:
			return HSTYPE_OBJECT;
			break;
		}
	}

	return SHADERTYPE_COUNT;
}
SHADERTYPE GetDSTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest)
{
	if (tessellation)
	{
		switch (renderPass)
		{
		case RENDERPASS_PREPASS:
			if (alphatest)
			{
				return DSTYPE_OBJECT_PREPASS_ALPHATEST;
			}
			else
			{
				return DSTYPE_OBJECT_PREPASS;
			}
		case RENDERPASS_MAIN:
			return DSTYPE_OBJECT;
		}
	}

	return SHADERTYPE_COUNT;
}
SHADERTYPE GetPSTYPE(RENDERPASS renderPass, bool alphatest, bool transparent, MaterialComponent::SHADERTYPE shaderType)
{
	SHADERTYPE realPS = SHADERTYPE_COUNT;

	switch (renderPass)
	{
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
		case wiScene::MaterialComponent::SHADERTYPE_PBR_CLOTH:
			realPS = transparent ? PSTYPE_OBJECT_TRANSPARENT_CLOTH : PSTYPE_OBJECT_CLOTH;
			break;
		case wiScene::MaterialComponent::SHADERTYPE_PBR_CLEARCOAT:
			realPS = transparent ? PSTYPE_OBJECT_TRANSPARENT_CLEARCOAT : PSTYPE_OBJECT_CLEARCOAT;
			break;
		case wiScene::MaterialComponent::SHADERTYPE_PBR_CLOTH_CLEARCOAT:
			realPS = transparent ? PSTYPE_OBJECT_TRANSPARENT_CLOTH_CLEARCOAT : PSTYPE_OBJECT_CLOTH_CLEARCOAT;
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
	case RENDERPASS_PREPASS:
		if (alphatest)
		{
			realPS = PSTYPE_OBJECT_PREPASS_ALPHATEST;
		}
		else
		{
			realPS = PSTYPE_OBJECT_PREPASS;
		}
		break;
	case RENDERPASS_ENVMAPCAPTURE:
		realPS = PSTYPE_ENVMAP;
		break;
	case RENDERPASS_SHADOW:
	case RENDERPASS_SHADOWCUBE:
		if (transparent)
		{
			realPS = shaderType == MaterialComponent::SHADERTYPE_WATER ? PSTYPE_SHADOW_WATER : PSTYPE_SHADOW_TRANSPARENT;
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


RaytracingPipelineState RTPSO_ao;
RaytracingPipelineState RTPSO_reflection;
RaytracingPipelineState RTPSO_shadow;

enum SKYRENDERING
{
	SKYRENDERING_STATIC,
	SKYRENDERING_DYNAMIC,
	SKYRENDERING_SUN,
	SKYRENDERING_VELOCITY,
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


#if __has_include("wiShaderDump.h")
// In this case, wiShaderDump.h contains precompiled shader binary data
#include "wiShaderdump.h"
#define SHADERDUMP_ENABLED
size_t GetShaderDumpCount()
{
	return wiShaderDump::shaderdump.size();
}
#else
// In this case, shaders can only be loaded from file
size_t GetShaderDumpCount()
{
	return 0;
}
#endif // SHADERDUMP

bool LoadShader(SHADERSTAGE stage, Shader& shader, const std::string& filename, SHADERMODEL minshadermodel)
{
	std::string shaderbinaryfilename = SHADERPATH + filename;

#ifdef SHADERDUMP_ENABLED
	
	// Loading shader from precompiled dump:
	auto it = wiShaderDump::shaderdump.find(shaderbinaryfilename);
	if (it != wiShaderDump::shaderdump.end())
	{
		return device->CreateShader(stage, it->second.data, it->second.size, &shader);
	}
	else
	{
		wiBackLog::post(("shader dump doesn't contain shader: " + shaderbinaryfilename).c_str());
	}
#endif // SHADERDUMP_ENABLED

	wiShaderCompiler::RegisterShader(shaderbinaryfilename);

	if (wiShaderCompiler::IsShaderOutdated(shaderbinaryfilename))
	{
		wiShaderCompiler::CompilerInput input;
		input.format = device->GetShaderFormat();
		input.stage = stage;
		input.minshadermodel = minshadermodel;

		std::string sourcedir = SHADERSOURCEPATH;
		wiHelper::MakePathAbsolute(sourcedir);
		input.include_directories.push_back(sourcedir);

		input.shadersourcefilename = wiHelper::ReplaceExtension(sourcedir + filename, "hlsl");

		wiShaderCompiler::CompilerOutput output;
		wiShaderCompiler::Compile(input, output);

		if (output.IsValid())
		{
			wiShaderCompiler::SaveShaderAndMetadata(shaderbinaryfilename, output);

			if (!output.error_message.empty())
			{
				wiBackLog::post(output.error_message.c_str());
			}
			wiBackLog::post(("shader compiled: " + shaderbinaryfilename).c_str());
			return device->CreateShader(stage, output.shaderdata, output.shadersize, &shader);
		}
		else
		{
			wiBackLog::post(("shader compile FAILED: " + shaderbinaryfilename + "\n" + output.error_message).c_str());
		}
	}

	std::vector<uint8_t> buffer;
	if (wiHelper::FileRead(shaderbinaryfilename, buffer))
	{
		return device->CreateShader(stage, buffer.data(), buffer.size(), &shader);
	}

	return false;
}


void LoadShaders()
{
	wiJobSystem::context ctx;

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		inputLayouts[ILTYPE_OBJECT_DEBUG].elements =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, INPUT_SLOT_POSITION_NORMAL_WIND, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
		};
		LoadShader(VS, shaders[VSTYPE_OBJECT_DEBUG], "objectVS_debug.cso");
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		inputLayouts[ILTYPE_OBJECT_COMMON].elements =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, INPUT_SLOT_POSITION_NORMAL_WIND, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "UVSET",					0, MeshComponent::Vertex_TEX::FORMAT, INPUT_SLOT_UV0, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "UVSET",					1, MeshComponent::Vertex_TEX::FORMAT, INPUT_SLOT_UV1, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "ATLAS",					0, MeshComponent::Vertex_TEX::FORMAT, INPUT_SLOT_ATLAS, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "COLOR",					0, MeshComponent::Vertex_COL::FORMAT, INPUT_SLOT_COLOR, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "TANGENT",				0, MeshComponent::Vertex_TAN::FORMAT, INPUT_SLOT_TANGENT, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEUSERDATA",		0, FORMAT_R32G32B32A32_UINT,  INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEATLAS",			0, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
		};
		LoadShader(VS, shaders[VSTYPE_OBJECT_COMMON], "objectVS_common.cso");
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		inputLayouts[ILTYPE_OBJECT_POS_PREVPOS].elements =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, INPUT_SLOT_POSITION_NORMAL_WIND, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "PREVPOS",				0, MeshComponent::Vertex_POS::FORMAT, INPUT_SLOT_PREVPOS, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEUSERDATA",		0, FORMAT_R32G32B32A32_UINT,  INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIXPREV",		0, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIXPREV",		1, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIXPREV",		2, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
		};
		LoadShader(VS, shaders[VSTYPE_OBJECT_PREPASS], "objectVS_prepass.cso");
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		inputLayouts[ILTYPE_OBJECT_POS_PREVPOS_TEX].elements =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, INPUT_SLOT_POSITION_NORMAL_WIND, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "PREVPOS",				0, MeshComponent::Vertex_POS::FORMAT, INPUT_SLOT_PREVPOS, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "UVSET",					0, MeshComponent::Vertex_TEX::FORMAT, INPUT_SLOT_UV0, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "UVSET",					1, MeshComponent::Vertex_TEX::FORMAT, INPUT_SLOT_UV1, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEUSERDATA",		0, FORMAT_R32G32B32A32_UINT,  INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIXPREV",		0, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIXPREV",		1, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIXPREV",		2, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
		};
		LoadShader(VS, shaders[VSTYPE_OBJECT_PREPASS_ALPHATEST], "objectVS_prepass_alphatest.cso");
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		inputLayouts[ILTYPE_OBJECT_POS].elements =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, INPUT_SLOT_POSITION_NORMAL_WIND, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEUSERDATA",		0, FORMAT_R32G32B32A32_UINT,  INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
		};
		LoadShader(VS, shaders[VSTYPE_SHADOW], "shadowVS.cso");
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		inputLayouts[ILTYPE_OBJECT_POS_TEX].elements =
		{
			{ "POSITION_NORMAL_WIND",	0, MeshComponent::Vertex_POS::FORMAT, INPUT_SLOT_POSITION_NORMAL_WIND, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "UVSET",					0, MeshComponent::Vertex_TEX::FORMAT, INPUT_SLOT_UV0, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "UVSET",					1, MeshComponent::Vertex_TEX::FORMAT, INPUT_SLOT_UV1, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },

			{ "INSTANCEMATRIX",			0, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIX",			1, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEMATRIX",			2, FORMAT_R32G32B32A32_FLOAT, INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
			{ "INSTANCEUSERDATA",		0, FORMAT_R32G32B32A32_UINT,  INPUT_SLOT_INSTANCEDATA, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA },
		};
		LoadShader(VS, shaders[VSTYPE_OBJECT_SIMPLE], "objectVS_simple.cso");
		LoadShader(VS, shaders[VSTYPE_SHADOW_ALPHATEST], "shadowVS_alphatest.cso");
		LoadShader(VS, shaders[VSTYPE_SHADOW_TRANSPARENT], "shadowVS_transparent.cso");
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		inputLayouts[ILTYPE_VERTEXCOLOR].elements =
		{
			{ "POSITION", 0, FORMAT_R32G32B32A32_FLOAT, 0, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "TEXCOORD", 0, FORMAT_R32G32B32A32_FLOAT, 0, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
		};
		LoadShader(VS, shaders[VSTYPE_VERTEXCOLOR], "vertexcolorVS.cso");
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) {
		inputLayouts[ILTYPE_RENDERLIGHTMAP].elements =
		{
			{ "POSITION_NORMAL_WIND",		0, MeshComponent::Vertex_POS::FORMAT, 0, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
			{ "ATLAS",						0, MeshComponent::Vertex_TEX::FORMAT, 1, InputLayout::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA },
		};
		LoadShader(VS, shaders[VSTYPE_RENDERLIGHTMAP], "renderlightmapVS.cso");
		});

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_OBJECT_COMMON_TESSELLATION], "objectVS_common_tessellation.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_OBJECT_PREPASS_TESSELLATION], "objectVS_prepass_tessellation.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_OBJECT_PREPASS_ALPHATEST_TESSELLATION], "objectVS_prepass_alphatest_tessellation.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_OBJECT_SIMPLE_TESSELLATION], "objectVS_simple_tessellation.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_IMPOSTOR], "impostorVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_VOLUMETRICLIGHT_DIRECTIONAL], "volumetriclight_directionalVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_VOLUMETRICLIGHT_POINT], "volumetriclight_pointVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_VOLUMETRICLIGHT_SPOT], "volumetriclight_spotVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_LIGHTVISUALIZER_SPOTLIGHT], "vSpotLightVS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_LIGHTVISUALIZER_POINTLIGHT], "vPointLightVS.cso"); });
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
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SHADOWCUBEMAPRENDER_TRANSPARENT], "cubeShadowVS_transparent.cso"); });
	}
	else
	{
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_ENVMAP], "envMapVS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_ENVMAP_SKY], "envMap_skyVS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SHADOWCUBEMAPRENDER], "cubeShadowVS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST], "cubeShadowVS_alphatest_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(VS, shaders[VSTYPE_SHADOWCUBEMAPRENDER_TRANSPARENT], "cubeShadowVS_transparent_emulation.cso"); });

		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_ENVMAP_EMULATION], "envMapGS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_ENVMAP_SKY_EMULATION], "envMap_skyGS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_SHADOWCUBEMAPRENDER_EMULATION], "cubeShadowGS_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST_EMULATION], "cubeShadowGS_alphatest_emulation.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_SHADOWCUBEMAPRENDER_TRANSPARENT_EMULATION], "cubeShadowGS_transparent_emulation.cso"); });
	}

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT], "objectPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT], "objectPS_transparent.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_PLANARREFLECTION], "objectPS_planarreflection.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_PLANARREFLECTION], "objectPS_transparent_planarreflection.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_POM], "objectPS_pom.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_POM], "objectPS_transparent_pom.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_ANISOTROPIC], "objectPS_anisotropic.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_ANISOTROPIC], "objectPS_transparent_anisotropic.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_CLOTH], "objectPS_cloth.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_CLOTH], "objectPS_transparent_cloth.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_CLEARCOAT], "objectPS_clearcoat.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_CLEARCOAT], "objectPS_transparent_clearcoat.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_CLOTH_CLEARCOAT], "objectPS_cloth_clearcoat.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_TRANSPARENT_CLOTH_CLEARCOAT], "objectPS_transparent_cloth_clearcoat.cso"); });
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
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_SIMPLE], "objectPS_simple.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_PREPASS], "objectPS_prepass.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_OBJECT_PREPASS_ALPHATEST], "objectPS_prepass_alphatest.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_IMPOSTOR_PREPASS], "impostorPS_prepass.cso"); });
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
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SKY_VELOCITY], "skyPS_velocity.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SUN], "sunPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SHADOW_ALPHATEST], "shadowPS_alphatest.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SHADOW_TRANSPARENT], "shadowPS_transparent.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_SHADOW_WATER], "shadowPS_water.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_VOXELIZER], "objectPS_voxelizer.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_VOXELIZER_TERRAIN], "objectPS_voxelizer_terrain.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_VOXEL], "voxelPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_FORCEFIELDVISUALIZER], "forceFieldVisualizerPS.cso"); });
	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
	{
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_RENDERLIGHTMAP], "renderlightmapPS_rtapi.cso", SHADERMODEL_6_5); });
	}
	else
	{
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_RENDERLIGHTMAP], "renderlightmapPS.cso"); });
	}
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_RAYTRACE_DEBUGBVH], "raytrace_debugbvhPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_DOWNSAMPLEDEPTHBUFFER], "downsampleDepthBuffer4xPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL], "upsample_bilateralPS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_POSTPROCESS_OUTLINE], "outlinePS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(PS, shaders[PSTYPE_LENSFLARE], "lensFlarePS.cso"); });

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_VOXELIZER], "objectGS_voxelizer.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(GS, shaders[GSTYPE_VOXEL], "voxelGS.cso"); });


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
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_SKYATMOSPHERE_SKYLUMINANCELUT], "skyAtmosphere_skyLuminanceLutCS.cso"); });
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
	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
	{
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_RAYTRACE], "raytraceCS_rtapi.cso", SHADERMODEL_6_5); });
	}
	else
	{
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_RAYTRACE], "raytraceCS.cso"); });
	}

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
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_TEMPORAL], "volumetricCloud_temporalCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_FXAA], "fxaaCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_TEMPORALAA], "temporalaaCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_LINEARDEPTH], "lineardepthCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_SHARPEN], "sharpenCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_TONEMAP], "tonemapCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_FSR_UPSCALING], "fsr_upscalingCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_FSR_SHARPEN], "fsr_sharpenCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_CHROMATIC_ABERRATION], "chromatic_aberrationCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT1], "upsample_bilateral_float1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UNORM1], "upsample_bilateral_unorm1CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT4], "upsample_bilateral_float4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UNORM4], "upsample_bilateral_unorm4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UINT4], "upsample_bilateral_uint4CS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_DOWNSAMPLE4X], "downsample4xCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_NORMALSFROMDEPTH], "normalsfromdepthCS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_SCREENSPACESHADOW], "screenspaceshadowCS.cso"); });

	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
	{
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_RTSHADOW], "rtshadowCS.cso", SHADERMODEL_6_5); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_TILECLASSIFICATION], "rtshadow_denoise_tileclassificationCS.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_FILTER], "rtshadow_denoise_filterCS.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_TEMPORAL], "rtshadow_denoise_temporalCS.cso"); });

		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_RTAO], "rtaoCS.cso", SHADERMODEL_6_5); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_RTAO_DENOISE_TILECLASSIFICATION], "rtao_denoise_tileclassificationCS.cso"); });
		wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(CS, shaders[CSTYPE_POSTPROCESS_RTAO_DENOISE_FILTER], "rtao_denoise_filterCS.cso"); });

	}

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(HS, shaders[HSTYPE_OBJECT], "objectHS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(HS, shaders[HSTYPE_OBJECT_PREPASS], "objectHS_prepass.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(HS, shaders[HSTYPE_OBJECT_PREPASS_ALPHATEST], "objectHS_prepass_alphatest.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(HS, shaders[HSTYPE_OBJECT_SIMPLE], "objectHS_simple.cso"); });

	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(DS, shaders[DSTYPE_OBJECT], "objectDS.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(DS, shaders[DSTYPE_OBJECT_PREPASS], "objectDS_prepass.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(DS, shaders[DSTYPE_OBJECT_PREPASS_ALPHATEST], "objectDS_prepass_alphatest.cso"); });
	wiJobSystem::Execute(ctx, [](wiJobArgs args) { LoadShader(DS, shaders[DSTYPE_OBJECT_SIMPLE], "objectDS_simple.cso"); });

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
							SHADERTYPE realHS = GetHSTYPE((RENDERPASS)renderPass, tessellation, alphatest);
							SHADERTYPE realDS = GetDSTYPE((RENDERPASS)renderPass, tessellation, alphatest);
							SHADERTYPE realGS = GetGSTYPE((RENDERPASS)renderPass, alphatest, transparency);
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

							if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_BINDLESS_DESCRIPTORS))
							{
								desc.il = nullptr;
							}

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
							case BLENDMODE_MULTIPLY:
								desc.bs = &blendStates[BSTYPE_MULTIPLY];
								break;
							default:
								assert(0);
								break;
							}

							switch (renderPass)
							{
							case RENDERPASS_SHADOW:
							case RENDERPASS_SHADOWCUBE:
								desc.bs = &blendStates[transparency ? BSTYPE_TRANSPARENTSHADOW : BSTYPE_COLORWRITEDISABLE];
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
								switch (doublesided)
								{
								default:
								case OBJECTRENDERING_DOUBLESIDED_DISABLED:
									desc.rs = &rasterizers[RSTYPE_FRONT];
									break;
								case OBJECTRENDERING_DOUBLESIDED_ENABLED:
									desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
									break;
								case OBJECTRENDERING_DOUBLESIDED_BACKSIDE:
									desc.rs = &rasterizers[RSTYPE_BACK];
									break;
								}
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
		case RENDERPASS_MAIN:
			desc.dss = &depthStencils[DSSTYPE_DEPTHREADEQUAL];
			desc.ps = &shaders[PSTYPE_OBJECT_TERRAIN];
			break;
		case RENDERPASS_PREPASS:
			desc.ps = &shaders[PSTYPE_OBJECT_PREPASS];
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
		desc.dss = &depthStencils[DSSTYPE_HOLOGRAM];
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
		desc.ps = &shaders[PSTYPE_OBJECT_SIMPLE];
		desc.rs = &rasterizers[RSTYPE_WIRE];
		desc.bs = &blendStates[BSTYPE_OPAQUE];
		desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		desc.il = &inputLayouts[ILTYPE_OBJECT_POS_TEX];

		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_BINDLESS_DESCRIPTORS))
		{
			desc.il = nullptr;
		}

		device->CreatePipelineState(&desc, &PSO_object_wire);

		desc.pt = PATCHLIST;
		desc.vs = &shaders[VSTYPE_OBJECT_SIMPLE_TESSELLATION];
		desc.hs = &shaders[HSTYPE_OBJECT_SIMPLE];
		desc.ds = &shaders[DSTYPE_OBJECT_SIMPLE];
		device->CreatePipelineState(&desc, &PSO_object_wire_tessellation);
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
		case RENDERPASS_PREPASS:
			desc.vs = &shaders[VSTYPE_IMPOSTOR];
			desc.ps = &shaders[PSTYPE_IMPOSTOR_PREPASS];
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
		desc.il = &inputLayouts[ILTYPE_OBJECT_COMMON];

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
		desc.bs = &blendStates[BSTYPE_ADDITIVE];
		desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		desc.dss = &depthStencils[DSSTYPE_XRAY];
		desc.pt = TRIANGLESTRIP;

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
		case SKYRENDERING_VELOCITY:
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.vs = &shaders[VSTYPE_SKY];
			desc.ps = &shaders[PSTYPE_SKY_VELOCITY];
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

	if(device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_PIPELINE) &&
		device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_GEOMETRYINDEX))
	{
		wiJobSystem::Execute(ctx, [](wiJobArgs args) {

			bool success = LoadShader(LIB, shaders[RTTYPE_RTREFLECTION], "rtreflectionLIB.cso");
			assert(success);

			RaytracingPipelineStateDesc rtdesc;
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
			success = device->CreateRaytracingPipelineState(&rtdesc, &RTPSO_reflection);


			});
	};

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

	bd.ByteWidth = sizeof(CameraCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_CAMERA]);
	device->SetName(&constantBuffers[CBTYPE_CAMERA], "CameraCB");

	bd.ByteWidth = sizeof(MiscCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_MISC]);
	device->SetName(&constantBuffers[CBTYPE_MISC], "MiscCB");

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

	bd.ByteWidth = sizeof(FSRCB);
	device->CreateBuffer(&bd, nullptr, &constantBuffers[CBTYPE_POSTPROCESS_FSR]);
	device->SetName(&constantBuffers[CBTYPE_POSTPROCESS_FSR], "FSRCB");

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

	{
		TextureDesc desc;
		desc.BindFlags = BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R8_UNORM;
		desc.Height = 16;
		desc.Width = 16;
		SubresourceData InitData;
		InitData.pSysMem = sheenLUTdata;
		InitData.SysMemPitch = desc.Width;
		device->CreateTexture(&desc, &InitData, &textures[TEXTYPE_2D_SHEENLUT]);
	}

	{
		TextureDesc desc;
		desc.type = TextureDesc::TEXTURE_2D;
		desc.Width = 256;
		desc.Height = 64;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT]);
	}
	{
		TextureDesc desc;
		desc.type = TextureDesc::TEXTURE_2D;
		desc.Width = 32;
		desc.Height = 32;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT]);
	}
	{
		TextureDesc desc;
		desc.type = TextureDesc::TEXTURE_2D;
		desc.Width = 192;
		desc.Height = 104;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT]);
	}
	{
		TextureDesc desc;
		desc.type = TextureDesc::TEXTURE_2D;
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT]);
	}
}
void SetUpStates()
{
	RasterizerState rs;
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
	rasterizers[RSTYPE_FRONT] = rs;


	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_BACK;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = -1;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = -4.0f;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	rasterizers[RSTYPE_SHADOW] = rs;
	rs.CullMode = CULL_NONE;
	rasterizers[RSTYPE_SHADOW_DOUBLESIDED] = rs;

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
	rasterizers[RSTYPE_WIRE] = rs;
	rs.AntialiasedLineEnable = true;
	rasterizers[RSTYPE_WIRE_SMOOTH] = rs;

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
	rasterizers[RSTYPE_DOUBLESIDED] = rs;

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
	rasterizers[RSTYPE_WIRE_DOUBLESIDED] = rs;
	rs.AntialiasedLineEnable = true;
	rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH] = rs;

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
	rasterizers[RSTYPE_BACK] = rs;

	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_NONE;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 2;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 2;
	rs.DepthClipEnable = true;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	rasterizers[RSTYPE_OCCLUDEE] = rs;

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
	rasterizers[RSTYPE_SKY] = rs;

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
	rs.ForcedSampleCount = 8;
	rasterizers[RSTYPE_VOXELIZE] = rs;




	DepthStencilState dsd;
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
	depthStencils[DSSTYPE_DEFAULT] = dsd;

	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	depthStencils[DSSTYPE_HOLOGRAM] = dsd;

	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_GREATER;
	dsd.StencilEnable = false;
	depthStencils[DSSTYPE_SHADOW] = dsd;

	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_GREATER;
	dsd.StencilEnable = false;
	depthStencils[DSSTYPE_CAPTUREIMPOSTOR] = dsd;


	dsd.DepthEnable = true;
	dsd.StencilEnable = false;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_GREATER_EQUAL;
	depthStencils[DSSTYPE_DEPTHREAD] = dsd;

	dsd.DepthEnable = false;
	dsd.StencilEnable = false;
	depthStencils[DSSTYPE_XRAY] = dsd;


	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_EQUAL;
	depthStencils[DSSTYPE_DEPTHREADEQUAL] = dsd;


	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_GREATER;
	depthStencils[DSSTYPE_ENVMAP] = dsd;

	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_ALWAYS;
	dsd.StencilEnable = false;
	depthStencils[DSSTYPE_WRITEONLY] = dsd;


	BlendState bd;
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
	blendStates[BSTYPE_OPAQUE] = bd;

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
	blendStates[BSTYPE_TRANSPARENT] = bd;

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
	blendStates[BSTYPE_PREMULTIPLIED] = bd;


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
	blendStates[BSTYPE_ADDITIVE] = bd;


	bd.RenderTarget[0].BlendEnable = false;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_DISABLE;
	bd.IndependentBlendEnable = false,
		bd.AlphaToCoverageEnable = false;
	blendStates[BSTYPE_COLORWRITEDISABLE] = bd;


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
	blendStates[BSTYPE_ENVIRONMENTALLIGHT] = bd;

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
	blendStates[BSTYPE_INVERSE] = bd;


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
	blendStates[BSTYPE_DECAL] = bd;


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
	blendStates[BSTYPE_MULTIPLY] = bd;

	bd.RenderTarget[0].SrcBlend = BLEND_ZERO;
	bd.RenderTarget[0].DestBlend = BLEND_SRC_COLOR;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_MAX;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.AlphaToCoverageEnable = false;
	bd.IndependentBlendEnable = false;
	blendStates[BSTYPE_TRANSPARENTSHADOW] = bd;
}

void ModifyObjectSampler(const SamplerDesc& desc)
{
	device->CreateSampler(&desc, &samplers[SSLOT_OBJECTSHADER]);
}

const std::string& GetShaderPath()
{
	return SHADERPATH;
}
void SetShaderPath(const std::string& path)
{
	SHADERPATH = path;
}
const std::string& GetShaderSourcePath()
{
	return SHADERSOURCEPATH;
}
void SetShaderSourcePath(const std::string& path)
{
	SHADERSOURCEPATH = path;
}
void ReloadShaders()
{
	device->ClearPipelineStateCache();

	wiEvent::FireEvent(SYSTEM_EVENT_RELOAD_SHADERS, 0);
}

void Initialize()
{
	SetUpStates();
	LoadBuffers();

	static wiEvent::Handle handle2 = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
	LoadShaders();

	SetShadowProps2D(SHADOWRES_2D, SHADOWCOUNT_2D);
	SetShadowPropsCube(SHADOWRES_CUBE, SHADOWCOUNT_CUBE);

	wiBackLog::post("wiRenderer Initialized");
}
void ClearWorld(Scene& scene)
{
	scene.Clear();

	deferredMIPGenLock.lock();
	deferredMIPGens.clear();
	deferredMIPGenLock.unlock();

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


ForwardEntityMaskCB ForwardEntityCullingCPU(const Visibility& vis, const AABB& batch_aabb, RENDERPASS renderPass)
{
	// Performs CPU light culling for a renderable batch:
	//	Similar to GPU-based tiled light culling, but this is only for simple forward passes (drawcall-granularity)

	ForwardEntityMaskCB cb;
	cb.xForwardLightMask.x = 0;
	cb.xForwardLightMask.y = 0;
	cb.xForwardDecalMask = 0;
	cb.xForwardEnvProbeMask = 0;

	uint32_t buckets[2] = { 0,0 };
	for (size_t i = 0; i < std::min(size_t(64), vis.visibleLights.size()); ++i) // only support indexing 64 lights at max for now
	{
		const uint16_t lightIndex = vis.visibleLights[i].index;
		const AABB& light_aabb = vis.scene->aabb_lights[lightIndex];
		if (light_aabb.intersects(batch_aabb))
		{
			const uint8_t bucket_index = uint8_t(i / 32);
			const uint8_t bucket_place = uint8_t(i % 32);
			buckets[bucket_index] |= 1 << bucket_place;
		}
	}
	cb.xForwardLightMask.x = buckets[0];
	cb.xForwardLightMask.y = buckets[1];

	for (size_t i = 0; i < std::min(size_t(32), vis.visibleDecals.size()); ++i)
	{
		const uint32_t decalIndex = vis.visibleDecals[vis.visibleDecals.size() - 1 - i]; // note: reverse order, for correct blending!
		const AABB& decal_aabb = vis.scene->aabb_decals[decalIndex];
		if (decal_aabb.intersects(batch_aabb))
		{
			const uint8_t bucket_place = uint8_t(i % 32);
			cb.xForwardDecalMask |= 1 << bucket_place;
		}
	}

	if (renderPass != RENDERPASS_ENVMAPCAPTURE)
	{
		for (size_t i = 0; i < std::min(size_t(32), vis.visibleEnvProbes.size()); ++i)
		{
			const uint32_t probeIndex = vis.visibleEnvProbes[vis.visibleEnvProbes.size() - 1 - i]; // note: reverse order, for correct blending!
			const AABB& probe_aabb = vis.scene->aabb_probes[probeIndex];
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
}
void BindShadowmaps(SHADERSTAGE stage, CommandList cmd)
{
	device->BindResource(stage, &shadowMapArray_2D, TEXSLOT_SHADOWARRAY_2D, cmd);
	device->BindResource(stage, &shadowMapArray_Cube, TEXSLOT_SHADOWARRAY_CUBE, cmd);
	if (GetTransparentShadowsEnabled())
	{
		device->BindResource(stage, &shadowMapArray_Transparent_2D, TEXSLOT_SHADOWARRAY_TRANSPARENT_2D, cmd);
		device->BindResource(stage, &shadowMapArray_Transparent_Cube, TEXSLOT_SHADOWARRAY_TRANSPARENT_CUBE, cmd);
	}
}

void RenderMeshes(
	const Visibility& vis,
	const RenderQueue& renderQueue,
	RENDERPASS renderPass,
	uint32_t renderTypeFlags,
	CommandList cmd,
	bool tessellation = false,
	const Frustum* frusta = nullptr,
	uint32_t frustum_count = 1
)
{
	if (renderQueue.empty())
		return;

	device->EventBegin("RenderMeshes", cmd);
	const bool bindless = device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_BINDLESS_DESCRIPTORS);

	tessellation = tessellation && device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_TESSELLATION);
	if (tessellation)
	{
		BindConstantBuffers(DS, cmd);
	}

	const TextureDesc& lightmap_desc = vis.scene->lightmap.GetDesc();
	const float lightmap_width_rcp = 1.0f / lightmap_desc.Width;
	const float lightmap_height_rcp = 1.0f / lightmap_desc.Height;

	// Do we need to compute a light mask for this pass on the CPU?
	const bool forwardLightmaskRequest =
		renderPass == RENDERPASS_ENVMAPCAPTURE ||
		renderPass == RENDERPASS_VOXELIZE;

	const INSTANCETYPE instanceRequest = instanceTypes[renderPass];
	struct Instance_MATRIX_USERDATA
	{
		Instance instance;
	};
	struct Instance_MATRIX_USERDATA_ATLAS
	{
		Instance instance;
		InstanceAtlas instanceAtlas;
	};
	struct Instance_MATRIX_USERDATA_MATRIXPREV
	{
		Instance instance;
		InstancePrev instancePrev;
	};

	// Pre-allocate space for all the instances in GPU-buffer:
	uint32_t instanceDataSize = 0;
	switch (instanceRequest)
	{
	default:
	case INSTANCETYPE_MATRIX_USERDATA:
		instanceDataSize = sizeof(Instance_MATRIX_USERDATA);
		break;
	case INSTANCETYPE_MATRIX_USERDATA_ATLAS:
		instanceDataSize = sizeof(Instance_MATRIX_USERDATA_ATLAS);
		break;
	case INSTANCETYPE_MATRIX_USERDATA_MATRIXPREV:
		instanceDataSize = sizeof(Instance_MATRIX_USERDATA_MATRIXPREV);
		break;
	}
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
		uint16_t padding;
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
		const ObjectComponent& instance = vis.scene->objects[instanceIndex];
		const AABB& instanceAABB = vis.scene->aabb_objects[instanceIndex];
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
			float distance = wiMath::Distance(instanceAABB.getCenter(), vis.camera->Eye);
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
			current_batch.aabb = AABB::Merge(current_batch.aabb, instanceAABB);
		}

		const XMFLOAT4X4& worldMatrix = instance.transform_index >= 0 ? vis.scene->transforms[instance.transform_index].world : IDENTITYMATRIX;

		for (uint32_t frustum_index = 0; frustum_index < frustum_count; ++frustum_index)
		{
			if (frusta != nullptr && !frusta[frustum_index].CheckBoxFast(instanceAABB))
			{
				// In case multiple cameras were provided and no intersection detected with frustum, we don't add the instance for the face:
				continue;
			}

			// Write into actual GPU-buffer:
			switch (instanceRequest)
			{
			default:
			case INSTANCETYPE_MATRIX_USERDATA:
				((volatile Instance_MATRIX_USERDATA*)instances.data)[instanceCount].instance.Create(worldMatrix, instance.color, dither, frustum_index, instance.emissiveColor);
				break;
			case INSTANCETYPE_MATRIX_USERDATA_ATLAS:
				((volatile Instance_MATRIX_USERDATA_ATLAS*)instances.data)[instanceCount].instance.Create(worldMatrix, instance.color, dither, frustum_index, instance.emissiveColor);
				{
					XMFLOAT4 lightMapMulAdd;
					if (instance.lightmap.IsValid())
					{
						auto rect = instance.lightmap_rect;

						// eliminate border expansion:
						rect.x += Scene::atlasClampBorder;
						rect.y += Scene::atlasClampBorder;
						rect.w -= Scene::atlasClampBorder * 2;
						rect.h -= Scene::atlasClampBorder * 2;

						lightMapMulAdd = XMFLOAT4(
							(float)rect.w / (float)lightmap_desc.Width,
							(float)rect.h / (float)lightmap_desc.Height,
							(float)rect.x / (float)lightmap_desc.Width,
							(float)rect.y / (float)lightmap_desc.Height
						);
					}
					else
					{
						lightMapMulAdd = XMFLOAT4(0, 0, 0, 0);
					}
					((volatile Instance_MATRIX_USERDATA_ATLAS*)instances.data)[instanceCount].instanceAtlas.Create(lightMapMulAdd);
				}
				break;
			case INSTANCETYPE_MATRIX_USERDATA_MATRIXPREV:
				((volatile Instance_MATRIX_USERDATA_MATRIXPREV*)instances.data)[instanceCount].instance.Create(worldMatrix, instance.color, dither, frustum_index, instance.emissiveColor);
				((volatile Instance_MATRIX_USERDATA_MATRIXPREV*)instances.data)[instanceCount].instancePrev.Create(instance.prev_transform_index >= 0 ? vis.scene->prev_transforms[instance.prev_transform_index].world_prev : IDENTITYMATRIX);
				break;
			}

			current_batch.instanceCount++; // next instance in current InstancedBatch
			instanceCount++;
		}

	}

	// Render instanced batches:
	for (int instancedBatchID = 0; instancedBatchID < instancedBatchCount; ++instancedBatchID)
	{
		const InstancedBatch& instancedBatch = instancedBatchArray[instancedBatchID];
		const MeshComponent& mesh = vis.scene->meshes[instancedBatch.meshIndex];
		const bool forceAlphaTestForDithering = instancedBatch.forceAlphatestForDithering != 0;
		const uint8_t userStencilRefOverride = instancedBatch.userStencilRefOverride;

		const float tessF = mesh.GetTessellationFactor();
		const bool tessellatorRequested = tessF > 0 && tessellation;
		const bool terrain = mesh.IsTerrain();

		if (tessellatorRequested)
		{
			TessellationCB tessCB;
			tessCB.xTessellationFactors = XMFLOAT4(tessF, tessF, tessF, tessF);
			device->UpdateBuffer(&constantBuffers[CBTYPE_TESSELLATION], &tessCB, cmd);
			device->BindConstantBuffer(HS, &constantBuffers[CBTYPE_TESSELLATION], CBSLOT_RENDERER_TESSELLATION, cmd);
		}

		if (forwardLightmaskRequest)
		{
			ForwardEntityMaskCB cb = ForwardEntityCullingCPU(vis, instancedBatch.aabb, renderPass);
			device->UpdateBuffer(&constantBuffers[CBTYPE_FORWARDENTITYMASK], &cb, cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_FORWARDENTITYMASK], CB_GETBINDSLOT(ForwardEntityMaskCB), cmd);
		}

		device->BindIndexBuffer(&mesh.indexBuffer, mesh.GetIndexFormat(), 0, cmd);

		ObjectPushConstants push; // used with bindless model only

		if (bindless)
		{
			push.mesh = device->GetDescriptorIndex(&mesh.descriptor, SRV);
			push.instances = device->GetDescriptorIndex(instances.buffer, SRV);
			push.instance_offset = instancedBatch.dataOffset;
		}
		else
		{
			const GPUBuffer* vbs[] = {
				mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
				mesh.vertexBuffer_PRE.IsValid() ? &mesh.vertexBuffer_PRE : &mesh.vertexBuffer_POS,
				&mesh.vertexBuffer_UV0,
				&mesh.vertexBuffer_UV1,
				&mesh.vertexBuffer_ATL,
				&mesh.vertexBuffer_COL,
				mesh.streamoutBuffer_TAN.IsValid() ? &mesh.streamoutBuffer_TAN : &mesh.vertexBuffer_TAN,
				instances.buffer
			};
			uint32_t strides[] = {
				sizeof(MeshComponent::Vertex_POS),
				sizeof(MeshComponent::Vertex_POS),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(MeshComponent::Vertex_COL),
				sizeof(MeshComponent::Vertex_TAN),
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
			static_assert(arraysize(vbs) == INPUT_SLOT_COUNT, "This layout must conform to OBJECT_VERTEXINPUT enum!");
			static_assert(arraysize(vbs) == arraysize(strides), "Mismatch between vertex buffers and strides!");
			static_assert(arraysize(vbs) == arraysize(offsets), "Mismatch between vertex buffers and offsets!");
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
		}

		for (const MeshComponent::MeshSubset& subset : mesh.subsets)
		{
			if (subset.indexCount == 0)
			{
				continue;
			}
			const MaterialComponent& material = vis.scene->materials[subset.materialIndex];

			bool subsetRenderable = renderTypeFlags & material.GetRenderTypes();

			if (renderPass == RENDERPASS_SHADOW || renderPass == RENDERPASS_SHADOWCUBE)
			{
				subsetRenderable = subsetRenderable && material.IsCastingShadow();
			}

			if (!subsetRenderable)
			{
				continue;
			}

			const PipelineState* pso = nullptr;
			const PipelineState* pso_backside = nullptr; // only when separate backside rendering is required (transparent doublesided)
			{
				if (IsWireRender())
				{
					switch (renderPass)
					{
					case RENDERPASS_MAIN:
						pso = tessellatorRequested ? &PSO_object_wire_tessellation : &PSO_object_wire;
					}
				}
				else if (mesh.IsTerrain())
				{
					pso = &PSO_object_terrain[renderPass];
				}
				else if (material.customShaderID >= 0 && material.customShaderID < (int)customShaders.size())
				{
					const CustomShader& customShader = customShaders[material.customShaderID];
					if (renderTypeFlags & customShader.renderTypeFlags)
					{
						pso = &customShader.pso[renderPass];
					}
				}
				else
				{
					const BLENDMODE blendMode = material.GetBlendMode();
					const bool alphatest = material.IsAlphaTestEnabled() || forceAlphaTestForDithering;
					OBJECTRENDERING_DOUBLESIDED doublesided = (mesh.IsDoubleSided() || material.IsDoubleSided()) ? OBJECTRENDERING_DOUBLESIDED_ENABLED : OBJECTRENDERING_DOUBLESIDED_DISABLED;

					pso = &PSO_object[material.shaderType][renderPass][blendMode][doublesided][tessellatorRequested][alphatest];
					assert(pso->IsValid());

					if ((renderTypeFlags & RENDERTYPE_TRANSPARENT) && doublesided == OBJECTRENDERING_DOUBLESIDED_ENABLED)
					{
						doublesided = OBJECTRENDERING_DOUBLESIDED_BACKSIDE;
						pso_backside = &PSO_object[material.shaderType][renderPass][blendMode][doublesided][tessellatorRequested][alphatest];
					}
				}
			}

			if (pso == nullptr || !pso->IsValid())
			{
				continue;
			}

			STENCILREF engineStencilRef = material.engineStencilRef;
			uint8_t userStencilRef = userStencilRefOverride > 0 ? userStencilRefOverride : material.userStencilRef;
			uint32_t stencilRef = CombineStencilrefs(engineStencilRef, userStencilRef);
			device->BindStencilRef(stencilRef, cmd);

			if (renderPass != RENDERPASS_PREPASS && renderPass != RENDERPASS_VOXELIZE) // depth only alpha test will be full res
			{
				device->BindShadingRate(material.shadingRate, cmd);
			}

			if (bindless)
			{
				push.material = device->GetDescriptorIndex(&material.constantBuffer, SRV);
				device->PushConstants(&push, sizeof(push), cmd);
			}
			else
			{
				device->BindConstantBuffer(VS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);
				device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);

				// Bind all material textures:
				const GPUResource* materialtextures[MaterialComponent::TEXTURESLOT_COUNT];
				material.WriteTextures(materialtextures, arraysize(materialtextures));
				device->BindResources(PS, materialtextures, TEXSLOT_RENDERER_BASECOLORMAP, arraysize(materialtextures), cmd);

				if (tessellatorRequested)
				{
					device->BindConstantBuffer(DS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);
					device->BindResources(DS, materialtextures, TEXSLOT_RENDERER_BASECOLORMAP, arraysize(materialtextures), cmd);
				}

				if (terrain)
				{
					if (mesh.terrain_material1 == INVALID_ENTITY || !vis.scene->materials.Contains(mesh.terrain_material1))
					{
						device->BindResources(PS, materialtextures, TEXSLOT_RENDERER_BLEND1_BASECOLORMAP, 4, cmd);
						device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend1), cmd);
					}
					else
					{
						const MaterialComponent& blendmat = *vis.scene->materials.GetComponent(mesh.terrain_material1);
						const GPUResource* res[4];
						blendmat.WriteTextures(res, arraysize(res));
						device->BindResources(PS, res, TEXSLOT_RENDERER_BLEND1_BASECOLORMAP, arraysize(res), cmd);
						device->BindConstantBuffer(PS, &blendmat.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend1), cmd);
					}

					if (mesh.terrain_material2 == INVALID_ENTITY || !vis.scene->materials.Contains(mesh.terrain_material2))
					{
						device->BindResources(PS, materialtextures, TEXSLOT_RENDERER_BLEND2_BASECOLORMAP, 4, cmd);
						device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend2), cmd);
					}
					else
					{
						const MaterialComponent& blendmat = *vis.scene->materials.GetComponent(mesh.terrain_material2);
						const GPUResource* res[4];
						blendmat.WriteTextures(res, arraysize(res));
						device->BindResources(PS, res, TEXSLOT_RENDERER_BLEND2_BASECOLORMAP, arraysize(res), cmd);
						device->BindConstantBuffer(PS, &blendmat.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend2), cmd);
					}

					if (mesh.terrain_material3 == INVALID_ENTITY || !vis.scene->materials.Contains(mesh.terrain_material3))
					{
						device->BindResources(PS, materialtextures, TEXSLOT_RENDERER_BLEND3_BASECOLORMAP, 4, cmd);
						device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend3), cmd);
					}
					else
					{
						const MaterialComponent& blendmat = *vis.scene->materials.GetComponent(mesh.terrain_material3);
						const GPUResource* res[4];
						blendmat.WriteTextures(res, arraysize(res));
						device->BindResources(PS, res, TEXSLOT_RENDERER_BLEND3_BASECOLORMAP, arraysize(res), cmd);
						device->BindConstantBuffer(PS, &blendmat.constantBuffer, CB_GETBINDSLOT(MaterialCB_Blend3), cmd);
					}
				}
			}

			if (pso_backside != nullptr)
			{
				device->BindPipelineState(pso_backside, cmd);
				device->DrawIndexedInstanced(subset.indexCount, instancedBatch.instanceCount, subset.indexOffset, 0, 0, cmd);
			}

			device->BindPipelineState(pso, cmd);
			device->DrawIndexedInstanced(subset.indexCount, instancedBatch.instanceCount, subset.indexOffset, 0, 0, cmd);
		}
	}

	GetRenderFrameAllocator(cmd).free(sizeof(InstancedBatch) * instancedBatchCount);

	device->EventEnd(cmd);
}

void RenderImpostors(
	const Visibility& vis,
	RENDERPASS renderPass, 
	CommandList cmd
)
{
	const PipelineState* impostorRequest = GetImpostorPSO(renderPass);

	if (vis.scene->impostors.GetCount() > 0 && impostorRequest != nullptr)
	{
		uint32_t instanceCount = 0;
		for (size_t impostorID = 0; impostorID < vis.scene->impostors.GetCount(); ++impostorID)
		{
			const ImpostorComponent& impostor = vis.scene->impostors[impostorID];
			if (vis.camera->frustum.CheckBoxFast(impostor.aabb))
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
		for (size_t impostorID = 0; impostorID < vis.scene->impostors.GetCount(); ++impostorID)
		{
			const ImpostorComponent& impostor = vis.scene->impostors[impostorID];
			if (!vis.camera->frustum.CheckBoxFast(impostor.aabb))
			{
				continue;
			}

			for (auto& mat : impostor.instanceMatrices)
			{
				const XMFLOAT3 center = *((XMFLOAT3*)&mat._41);
				float distance = wiMath::Distance(vis.camera->Eye, center);

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

		MiscCB cb;
		cb.g_xColor.x = (float)instances.offset;
		device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &cb, cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

		device->BindResource(VS, instances.buffer, TEXSLOT_ONDEMAND21, cmd);
		device->BindResource(PS, &vis.scene->impostorArray, TEXSLOT_ONDEMAND0, cmd);

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
		GenerateMipChain(it.first->texture, MIPGENFILTER_LINEAR, cmd, mipopt);
	}
	deferredMIPGens.clear();
	deferredMIPGenLock.unlock();
}

void UpdateVisibility(Visibility& vis)
{
	// Perform parallel frustum culling and obtain closest reflector:
	wiJobSystem::context ctx;
	wiJobSystem::context ctx_lights;
	auto range = wiProfiler::BeginRangeCPU("Frustum Culling");

	assert(vis.scene != nullptr); // User must provide a scene!
	assert(vis.camera != nullptr); // User must provide a camera!

	// The parallel frustum culling is first performed in shared memory, 
	//	then each group writes out it's local list to global memory
	//	The shared memory approach reduces atomics and helps the list to remain
	//	more coherent (less randomly organized compared to original order)
	static const uint32_t groupSize = 64;
	static const size_t sharedmemory_size = (groupSize + 1) * sizeof(uint32_t); // list + counter per group

	// Initialize visible indices:
	vis.Clear();

	if (!freezeCullingCamera)
	{
		vis.frustum = vis.camera->frustum;
	}

	if (vis.flags & Visibility::ALLOW_LIGHTS)
	{
		// Cull lights:
		vis.visibleLights.resize(vis.scene->aabb_lights.GetCount());
		wiJobSystem::Dispatch(ctx_lights, (uint32_t)vis.scene->aabb_lights.GetCount(), groupSize, [&](wiJobArgs args) {

			// Setup stream compaction:
			uint32_t& group_count = *(uint32_t*)args.sharedmemory;
			Visibility::VisibleLight* group_list = (Visibility::VisibleLight*)args.sharedmemory + 1;
			if (args.isFirstJobInGroup)
			{
				group_count = 0; // first thread initializes local counter
			}

			const AABB& aabb = vis.scene->aabb_lights[args.jobIndex];

			if ((aabb.layerMask & vis.layerMask) && vis.frustum.CheckBoxFast(aabb))
			{
				// Local stream compaction:
				//	(also compute light distance for shadow priority sorting)
				assert(args.jobIndex < 0xFFFF);
				group_list[group_count].index = (uint16_t)args.jobIndex;
				const LightComponent& lightcomponent = vis.scene->lights[args.jobIndex];
				float distance = 0;
				if (lightcomponent.type != LightComponent::DIRECTIONAL)
				{
					distance = wiMath::DistanceEstimated(lightcomponent.position, vis.camera->Eye);
				}
				group_list[group_count].distance = uint16_t(distance * 10);
				group_count++;
				if (lightcomponent.IsVolumetricsEnabled())
				{
					vis.volumetriclight_request.store(true);
				}
			}

			// Global stream compaction:
			if (args.isLastJobInGroup && group_count > 0)
			{
				uint32_t prev_count = vis.light_counter.fetch_add(group_count);
				for (uint32_t i = 0; i < group_count; ++i)
				{
					vis.visibleLights[prev_count + i] = group_list[i];
				}
			}

			}, sharedmemory_size);
	}

	if (vis.flags & Visibility::ALLOW_OBJECTS)
	{
		// Cull objects:
		vis.visibleObjects.resize(vis.scene->aabb_objects.GetCount());
		wiJobSystem::Dispatch(ctx, (uint32_t)vis.scene->aabb_objects.GetCount(), groupSize, [&](wiJobArgs args) {

			// Setup stream compaction:
			uint32_t& group_count = *(uint32_t*)args.sharedmemory;
			uint32_t* group_list = (uint32_t*)args.sharedmemory + 1;
			if (args.isFirstJobInGroup)
			{
				group_count = 0; // first thread initializes local counter
			}

			const AABB& aabb = vis.scene->aabb_objects[args.jobIndex];

			if ((aabb.layerMask & vis.layerMask) && vis.frustum.CheckBoxFast(aabb))
			{
				// Local stream compaction:
				group_list[group_count++] = args.jobIndex;

				if (vis.flags & Visibility::ALLOW_REQUEST_REFLECTION)
				{
					const ObjectComponent& object = vis.scene->objects[args.jobIndex];
					if (object.IsRequestPlanarReflection())
					{
						float dist = wiMath::DistanceEstimated(vis.camera->Eye, object.center);
						vis.locker.lock();
						if (dist < vis.closestRefPlane)
						{
							vis.closestRefPlane = dist;
							const TransformComponent& transform = vis.scene->transforms[object.transform_index];
							XMVECTOR P = transform.GetPositionV();
							XMVECTOR N = XMVectorSet(0, 1, 0, 0);
							N = XMVector3TransformNormal(N, XMLoadFloat4x4(&transform.world));
							XMVECTOR _refPlane = XMPlaneFromPointNormal(P, N);
							XMStoreFloat4(&vis.reflectionPlane, _refPlane);

							vis.planar_reflection_visible = true;
						}
						vis.locker.unlock();
					}
				}
			}

			// Global stream compaction:
			if (args.isLastJobInGroup && group_count > 0)
			{
				uint32_t prev_count = vis.object_counter.fetch_add(group_count);
				for (uint32_t i = 0; i < group_count; ++i)
				{
					vis.visibleObjects[prev_count + i] = group_list[i];
				}
			}

			}, sharedmemory_size);
	}

	if (vis.flags & Visibility::ALLOW_DECALS)
	{
		vis.visibleDecals.resize(vis.scene->aabb_decals.GetCount());
		wiJobSystem::Dispatch(ctx, (uint32_t)vis.scene->aabb_decals.GetCount(), groupSize, [&](wiJobArgs args) {

			// Setup stream compaction:
			uint32_t& group_count = *(uint32_t*)args.sharedmemory;
			uint32_t* group_list = (uint32_t*)args.sharedmemory + 1;
			if (args.isFirstJobInGroup)
			{
				group_count = 0; // first thread initializes local counter
			}

			const AABB& aabb = vis.scene->aabb_decals[args.jobIndex];

			if ((aabb.layerMask & vis.layerMask) && vis.frustum.CheckBoxFast(aabb))
			{
				// Local stream compaction:
				group_list[group_count++] = args.jobIndex;
			}

			// Global stream compaction:
			if (args.isLastJobInGroup && group_count > 0)
			{
				uint32_t prev_count = vis.decal_counter.fetch_add(group_count);
				for (uint32_t i = 0; i < group_count; ++i)
				{
					vis.visibleDecals[prev_count + i] = group_list[i];
				}
			}

			}, sharedmemory_size);
	}

	if (vis.flags & Visibility::ALLOW_ENVPROBES)
	{
		wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
			// Cull probes:
			for (size_t i = 0; i < vis.scene->aabb_probes.GetCount(); ++i)
			{
				const AABB& aabb = vis.scene->aabb_probes[i];

				if ((aabb.layerMask & vis.layerMask) && vis.frustum.CheckBoxFast(aabb))
				{
					vis.visibleEnvProbes.push_back((uint32_t)i);
				}
			}
			});
	}

	if (vis.flags & Visibility::ALLOW_EMITTERS)
	{
		wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
			// Cull emitters:
			for (size_t i = 0; i < vis.scene->emitters.GetCount(); ++i)
			{
				const wiEmittedParticle& emitter = vis.scene->emitters[i];
				if (!(emitter.layerMask & vis.layerMask))
				{
					continue;
				}
				vis.visibleEmitters.push_back((uint32_t)i);
			}
			});
	}

	if (vis.flags & Visibility::ALLOW_HAIRS)
	{
		wiJobSystem::Execute(ctx, [&](wiJobArgs args) {
			// Cull hairs:
			for (size_t i = 0; i < vis.scene->hairs.GetCount(); ++i)
			{
				const wiHairParticle& hair = vis.scene->hairs[i];
				if (!(hair.layerMask & vis.layerMask))
				{
					continue;
				}
				if (hair.meshID == INVALID_ENTITY || !vis.frustum.CheckBoxFast(hair.aabb))
				{
					continue;
				}
				vis.visibleHairs.push_back((uint32_t)i);
			}
			});
	}

	if (vis.flags & Visibility::ALLOW_LIGHTS)
	{
		wiJobSystem::Wait(ctx_lights);
		vis.visibleLights.resize((size_t)vis.light_counter.load());
		// Sort lights based on distance so that closer lights will receive shadow map priority:
		std::sort(vis.visibleLights.begin(), vis.visibleLights.end());
	}

	wiJobSystem::Wait(ctx);

	// finalize stream compaction:
	vis.visibleObjects.resize((size_t)vis.object_counter.load());
	vis.visibleDecals.resize((size_t)vis.decal_counter.load());

	if ((vis.flags & Visibility::ALLOW_REQUEST_REFLECTION) && vis.scene->weather.IsOceanEnabled())
	{
		// Ocean will override any current reflectors
		vis.planar_reflection_visible = true;
		XMVECTOR _refPlane = XMPlaneFromPointNormal(XMVectorSet(0, vis.scene->weather.oceanParameters.waterHeight, 0, 0), XMVectorSet(0, 1, 0, 0));
		XMStoreFloat4(&vis.reflectionPlane, _refPlane);
	}

	wiProfiler::EndRange(range); // Frustum Culling
}
void UpdatePerFrameData(
	Scene& scene,
	const Visibility& vis,
	FrameCB& frameCB,
	XMUINT2 internalResolution,
	const wiCanvas& canvas,
	float dt
)
{
	for (int i = 0; i < COMMANDLIST_COUNT; ++i)
	{
		renderFrameAllocators[i].reset();
	}

	wiJobSystem::context ctx;

	// Occlusion query allocation:
	if (GetOcclusionCullingEnabled() && !GetFreezeCullingCameraEnabled())
	{
		wiJobSystem::Dispatch(ctx, (uint32_t)vis.visibleObjects.size(), 64, [&](wiJobArgs args) {

			ObjectComponent& object = scene.objects[args.jobIndex];
			if (!object.IsRenderable())
			{
				return;
			}

			const AABB& aabb = scene.aabb_objects[args.jobIndex];

			if (aabb.intersects(vis.camera->Eye))
			{
				// camera is inside the instance, mark it as visible in this frame:
				object.occlusionHistory |= 1;
			}
			else
			{
				const uint32_t writeQuery = scene.queryAllocator.fetch_add(1); // allocate new occlusion query from heap
				if (writeQuery < scene.queryHeap[scene.queryheap_idx].desc.queryCount)
				{
					object.occlusionQueries[scene.queryheap_idx] = writeQuery;
				}
			}
		});
	}

	// Update Voxelization parameters:
	if (scene.objects.GetCount() > 0)
	{
		// We don't update it if the scene is empty, this even makes it easier to debug
		const float f = 0.05f / voxelSceneData.voxelsize;
		XMFLOAT3 center = XMFLOAT3(floorf(vis.camera->Eye.x * f) / f, floorf(vis.camera->Eye.y * f) / f, floorf(vis.camera->Eye.z * f) / f);
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
	}

	wiJobSystem::Wait(ctx);

	if (!device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_PIPELINE) && !device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE) && scene.IsAccelerationStructureUpdateRequested())
	{
		scene.BVH.Update(scene);
	}

	// Update CPU-side frame constant buffer:
	frameCB.g_xFrame_ConstantOne = 1;
	frameCB.g_xFrame_CanvasSize = float2(canvas.GetLogicalWidth(), canvas.GetLogicalHeight());
	frameCB.g_xFrame_CanvasSize_rcp = float2(1.0f / frameCB.g_xFrame_CanvasSize.x, 1.0f / frameCB.g_xFrame_CanvasSize.y);
	frameCB.g_xFrame_InternalResolution = float2((float)internalResolution.x, (float)internalResolution.y);
	frameCB.g_xFrame_InternalResolution_rcp = float2(1.0f / frameCB.g_xFrame_InternalResolution.x, 1.0f / frameCB.g_xFrame_InternalResolution.y);
	frameCB.g_xFrame_Gamma = GetGamma();
	frameCB.g_xFrame_SunColor = vis.scene->weather.sunColor;
	frameCB.g_xFrame_SunDirection = vis.scene->weather.sunDirection;
	frameCB.g_xFrame_SunEnergy = vis.scene->weather.sunEnergy;
	frameCB.g_xFrame_ShadowCascadeCount = CASCADE_COUNT;
	frameCB.g_xFrame_Ambient = vis.scene->weather.ambient;
	frameCB.g_xFrame_Cloudiness = vis.scene->weather.cloudiness;
	frameCB.g_xFrame_CloudScale = vis.scene->weather.cloudScale;
	frameCB.g_xFrame_CloudSpeed = vis.scene->weather.cloudSpeed;
	frameCB.g_xFrame_Fog = float4(vis.scene->weather.fogStart, vis.scene->weather.fogEnd, vis.scene->weather.fogHeightStart, vis.scene->weather.fogHeightEnd);
	frameCB.g_xFrame_FogHeightSky = vis.scene->weather.fogHeightSky;
	frameCB.g_xFrame_Horizon = vis.scene->weather.horizon;
	frameCB.g_xFrame_Zenith = vis.scene->weather.zenith;
	frameCB.g_xFrame_SkyExposure = vis.scene->weather.skyExposure;
	frameCB.g_xFrame_VoxelRadianceMaxDistance = voxelSceneData.maxDistance;
	frameCB.g_xFrame_VoxelRadianceDataSize = voxelSceneData.voxelsize;
	frameCB.g_xFrame_VoxelRadianceDataSize_rcp = 1.0f / (float)frameCB.g_xFrame_VoxelRadianceDataSize;
	frameCB.g_xFrame_VoxelRadianceDataRes = GetVoxelRadianceEnabled() ? (uint)voxelSceneData.res : 0;
	frameCB.g_xFrame_VoxelRadianceDataRes_rcp = 1.0f / (float)frameCB.g_xFrame_VoxelRadianceDataRes;
	frameCB.g_xFrame_VoxelRadianceDataMIPs = voxelSceneData.mips;
	frameCB.g_xFrame_VoxelRadianceNumCones = std::max(std::min(voxelSceneData.numCones, 16u), 1u);
	frameCB.g_xFrame_VoxelRadianceNumCones_rcp = 1.0f / (float)frameCB.g_xFrame_VoxelRadianceNumCones;
	frameCB.g_xFrame_VoxelRadianceRayStepSize = voxelSceneData.rayStepSize;
	frameCB.g_xFrame_VoxelRadianceDataCenter = voxelSceneData.center;
	frameCB.g_xFrame_EntityCullingTileCount = GetEntityCullingTileCount(internalResolution);
	frameCB.g_xFrame_ObjectShaderSamplerIndex = device->GetDescriptorIndex(&samplers[SSLOT_OBJECTSHADER]);

	// The order is very important here:
	frameCB.g_xFrame_DecalArrayOffset = 0;
	frameCB.g_xFrame_DecalArrayCount = (uint)vis.visibleDecals.size();
	frameCB.g_xFrame_EnvProbeArrayOffset = frameCB.g_xFrame_DecalArrayCount;
	frameCB.g_xFrame_EnvProbeArrayCount = std::min(vis.scene->envmapCount, (uint)vis.visibleEnvProbes.size());
	frameCB.g_xFrame_LightArrayOffset = frameCB.g_xFrame_EnvProbeArrayOffset + frameCB.g_xFrame_EnvProbeArrayCount;
	frameCB.g_xFrame_LightArrayCount = (uint)vis.visibleLights.size();
	frameCB.g_xFrame_ForceFieldArrayOffset = frameCB.g_xFrame_LightArrayOffset + frameCB.g_xFrame_LightArrayCount;
	frameCB.g_xFrame_ForceFieldArrayCount = (uint)vis.scene->forces.GetCount();

	frameCB.g_xFrame_GlobalEnvProbeIndex = 0;
	frameCB.g_xFrame_EnvProbeMipCount = 0;
	frameCB.g_xFrame_EnvProbeMipCount_rcp = 1.0f;
	if (vis.scene->envmapArray.IsValid())
	{
		frameCB.g_xFrame_EnvProbeMipCount = vis.scene->envmapArray.GetDesc().MipLevels;
		frameCB.g_xFrame_EnvProbeMipCount_rcp = 1.0f / (float)frameCB.g_xFrame_EnvProbeMipCount;
	}

	frameCB.g_xFrame_DeltaTime = dt * GetGameSpeed();
	frameCB.g_xFrame_TimePrev = frameCB.g_xFrame_Time;
	frameCB.g_xFrame_Time += frameCB.g_xFrame_DeltaTime;
	frameCB.g_xFrame_WindSpeed = vis.scene->weather.windSpeed;
	frameCB.g_xFrame_WindRandomness = vis.scene->weather.windRandomness;
	frameCB.g_xFrame_WindWaveSize = vis.scene->weather.windWaveSize;
	frameCB.g_xFrame_WindDirection = vis.scene->weather.windDirection;
	frameCB.g_xFrame_StaticSkyGamma = 0.0f;
	if (vis.scene->weather.skyMap != nullptr)
	{
		bool hdr = !device->IsFormatUnorm(vis.scene->weather.skyMap->texture.desc.Format);
		frameCB.g_xFrame_StaticSkyGamma = hdr ? 1.0f : frameCB.g_xFrame_Gamma;
	}
	frameCB.g_xFrame_FrameCount = (uint)device->GetFrameCount();
	frameCB.g_xFrame_TemporalAASampleRotation = 0;
	if (GetTemporalAAEnabled())
	{
		uint id = frameCB.g_xFrame_FrameCount % 4;
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
		frameCB.g_xFrame_TemporalAASampleRotation = (x & 0x000000FF) | ((y & 0x000000FF) << 8);
	}
	frameCB.g_xFrame_ShadowKernel2D = 1.0f / SHADOWRES_2D;
	frameCB.g_xFrame_ShadowKernelCube = 1.0f / SHADOWRES_CUBE;

	frameCB.g_xFrame_WorldBoundsMin = vis.scene->bounds.getMin();
	frameCB.g_xFrame_WorldBoundsMax = vis.scene->bounds.getMax();
	frameCB.g_xFrame_WorldBoundsExtents.x = abs(frameCB.g_xFrame_WorldBoundsMax.x - frameCB.g_xFrame_WorldBoundsMin.x);
	frameCB.g_xFrame_WorldBoundsExtents.y = abs(frameCB.g_xFrame_WorldBoundsMax.y - frameCB.g_xFrame_WorldBoundsMin.y);
	frameCB.g_xFrame_WorldBoundsExtents.z = abs(frameCB.g_xFrame_WorldBoundsMax.z - frameCB.g_xFrame_WorldBoundsMin.z);
	frameCB.g_xFrame_WorldBoundsExtents_rcp.x = 1.0f / frameCB.g_xFrame_WorldBoundsExtents.x;
	frameCB.g_xFrame_WorldBoundsExtents_rcp.y = 1.0f / frameCB.g_xFrame_WorldBoundsExtents.y;
	frameCB.g_xFrame_WorldBoundsExtents_rcp.z = 1.0f / frameCB.g_xFrame_WorldBoundsExtents.z;

	frameCB.g_xFrame_BlueNoisePhase = (frameCB.g_xFrame_FrameCount & 0xFF) * 1.6180339887f;

	frameCB.g_xFrame_Options = 0;
	if (GetTemporalAAEnabled())
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_TEMPORALAA_ENABLED;
	}
	if (GetTransparentShadowsEnabled())
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_TRANSPARENTSHADOWS_ENABLED;
	}
	if (GetVoxelRadianceEnabled())
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_VOXELGI_ENABLED;
	}
	if (GetVoxelRadianceReflectionsEnabled())
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_VOXELGI_REFLECTIONS_ENABLED;
	}
	if (voxelSceneData.centerChangedThisFrame)
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_VOXELGI_RETARGETTED;
	}
	if (vis.scene->weather.IsSimpleSky())
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_SIMPLE_SKY;
	}
	if (vis.scene->weather.IsRealisticSky())
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_REALISTIC_SKY;
	}
	if (vis.scene->weather.IsHeightFog())
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_HEIGHT_FOG;
	}
	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE) && GetRaytracedShadowsEnabled())
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_RAYTRACED_SHADOWS;
		frameCB.g_xFrame_Options |= OPTION_BIT_SHADOW_MASK;
	}
	if (IsDisableAlbedoMaps())
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_DISABLE_ALBEDO_MAPS;
	}
	if (GetScreenSpaceShadowsEnabled())
	{
		frameCB.g_xFrame_Options |= OPTION_BIT_SHADOW_MASK;
	}

	frameCB.g_xFrame_Atmosphere = vis.scene->weather.atmosphereParameters;
	frameCB.g_xFrame_VolumetricClouds = vis.scene->weather.volumetricCloudParameters;



	// Create volumetric cloud static resources if needed:
	if (scene.weather.IsVolumetricClouds() && !texture_shapeNoise.IsValid())
	{
		TextureDesc shape_desc;
		shape_desc.type = TextureDesc::TEXTURE_3D;
		shape_desc.Width = 64;
		shape_desc.Height = 64;
		shape_desc.Depth = 64;
		shape_desc.MipLevels = 6;
		shape_desc.Format = FORMAT_R8G8B8A8_UNORM;
		shape_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		shape_desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE_COMPUTE;
		device->CreateTexture(&shape_desc, nullptr, &texture_shapeNoise);
		device->SetName(&texture_shapeNoise, "texture_shapeNoise");

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
		detail_desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE_COMPUTE;
		device->CreateTexture(&detail_desc, nullptr, &texture_detailNoise);
		device->SetName(&texture_detailNoise, "texture_detailNoise");

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
		texture_desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE_COMPUTE;
		device->CreateTexture(&texture_desc, nullptr, &texture_curlNoise);
		device->SetName(&texture_curlNoise, "texture_curlNoise");

		texture_desc.Width = 1024;
		texture_desc.Height = 1024;
		texture_desc.Format = FORMAT_R8G8B8A8_UNORM;
		device->CreateTexture(&texture_desc, nullptr, &texture_weatherMap);
		device->SetName(&texture_weatherMap, "texture_weatherMap");
	}
}
void UpdateRenderData(
	const Visibility& vis,
	const FrameCB& frameCB,
	CommandList cmd
)
{
	device->UpdateBuffer(&constantBuffers[CBTYPE_FRAME], &frameCB, cmd);
	UpdateCameraCB(
		*vis.camera,
		*vis.camera,
		*vis.camera,
		cmd
	);

	BindCommonResources(cmd);

	// Update dirty material constant buffers:
	for (size_t i = 0; i < vis.scene->materials.GetCount(); ++i)
	{
		const MaterialComponent& material = vis.scene->materials[i];
		if (material.dirty_buffer)
		{
			material.dirty_buffer = false;

			ShaderMaterial shadermaterial;
			material.WriteShaderMaterial(&shadermaterial);
			device->UpdateBuffer(&material.constantBuffer, &shadermaterial, cmd);
		}
	}

	// Fill Entity Array with decals + envprobes + lights in the frustum:
	{
		// Reserve temporary entity array for GPU data upload:
		ShaderEntity* entityArray = (ShaderEntity*)GetRenderFrameAllocator(cmd).allocate(sizeof(ShaderEntity)*SHADER_ENTITY_COUNT);
		XMMATRIX* matrixArray = (XMMATRIX*)GetRenderFrameAllocator(cmd).allocate(sizeof(XMMATRIX)*MATRIXARRAY_COUNT);

		const XMMATRIX viewMatrix = vis.camera->GetView();

		uint32_t entityCounter = 0;
		uint32_t matrixCounter = 0;

		// Write decals into entity array:
		for (size_t i = 0; i < vis.visibleDecals.size(); ++i)
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
			const uint32_t decalIndex = vis.visibleDecals[vis.visibleDecals.size() - 1 - i]; // note: reverse order, for correct blending!
			const DecalComponent& decal = vis.scene->decals[decalIndex];

			entityArray[entityCounter] = {}; // zero out!
			entityArray[entityCounter].layerMask = ~0u;

			Entity entity = vis.scene->decals.GetEntity(decalIndex);
			const LayerComponent* layer = vis.scene->layers.GetComponent(entity);
			if (layer != nullptr)
			{
				entityArray[entityCounter].layerMask = layer->layerMask;
			}

			entityArray[entityCounter].SetType(ENTITY_TYPE_DECAL);
			entityArray[entityCounter].position = decal.position;
			entityArray[entityCounter].SetRange(decal.range);
			entityArray[entityCounter].color = wiMath::CompressColor(XMFLOAT4(decal.color.x, decal.color.y, decal.color.z, decal.GetOpacity()));
			entityArray[entityCounter].SetEnergy(decal.emissive);

			entityArray[entityCounter].SetIndices(matrixCounter, 0);
			matrixArray[matrixCounter] = XMMatrixInverse(nullptr, XMLoadFloat4x4(&decal.world));

			XMFLOAT4 atlasMulAdd;
			if (decal.texture != nullptr)
			{
				const TextureDesc& desc = vis.scene->decalAtlas.GetDesc();

				wiRectPacker::rect_xywh rect = vis.scene->packedDecals.at(decal.texture);

				// eliminate border expansion:
				rect.x += Scene::atlasClampBorder;
				rect.y += Scene::atlasClampBorder;
				rect.w -= Scene::atlasClampBorder * 2;
				rect.h -= Scene::atlasClampBorder * 2;

				atlasMulAdd = XMFLOAT4(
					(float)rect.w / (float)desc.Width,
					(float)rect.h / (float)desc.Height,
					(float)rect.x / (float)desc.Width,
					(float)rect.y / (float)desc.Height
				);
			}
			else
			{
				atlasMulAdd = XMFLOAT4(0, 0, 0, 0);
			}
			matrixArray[matrixCounter].r[0] = XMVectorSetW(matrixArray[matrixCounter].r[0], atlasMulAdd.x);
			matrixArray[matrixCounter].r[1] = XMVectorSetW(matrixArray[matrixCounter].r[1], atlasMulAdd.y);
			matrixArray[matrixCounter].r[2] = XMVectorSetW(matrixArray[matrixCounter].r[2], atlasMulAdd.z);
			matrixArray[matrixCounter].r[3] = XMVectorSetW(matrixArray[matrixCounter].r[3], atlasMulAdd.w);
			matrixCounter++;

			entityCounter++;
		}

		// Write environment probes into entity array:
		for (size_t i = 0; i < std::min((size_t)vis.scene->envmapCount, vis.visibleEnvProbes.size()); ++i)
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

			const uint32_t probeIndex = vis.visibleEnvProbes[vis.visibleEnvProbes.size() - 1 - i]; // note: reverse order, for correct blending!
			const EnvironmentProbeComponent& probe = vis.scene->probes[probeIndex];

			entityArray[entityCounter] = {}; // zero out!
			entityArray[entityCounter].layerMask = ~0u;

			Entity entity = vis.scene->probes.GetEntity(probeIndex);
			const LayerComponent* layer = vis.scene->layers.GetComponent(entity);
			if (layer != nullptr)
			{
				entityArray[entityCounter].layerMask = layer->layerMask;
			}

			entityArray[entityCounter].SetType(ENTITY_TYPE_ENVMAP);
			entityArray[entityCounter].position = probe.position;
			entityArray[entityCounter].SetRange(probe.range);

			entityArray[entityCounter].SetIndices(matrixCounter, (uint32_t)probe.textureIndex);
			matrixArray[matrixCounter] = XMLoadFloat4x4(&probe.inverseMatrix);
			matrixCounter++;

			entityCounter++;
		}

		// Write lights into entity array:
		uint32_t shadowCounter_2D = SHADOWRES_2D > 0 ? 0 : SHADOWCOUNT_2D;
		uint32_t shadowCounter_Cube = SHADOWRES_CUBE > 0 ? 0 : SHADOWCOUNT_CUBE;
		for (auto visibleLight : vis.visibleLights)
		{
			if (entityCounter == SHADER_ENTITY_COUNT)
			{
				assert(0); // too many entities!
				entityCounter--;
				break;
			}

			uint16_t lightIndex = visibleLight.index;
			const LightComponent& light = vis.scene->lights[lightIndex];

			entityArray[entityCounter] = {}; // zero out!
			entityArray[entityCounter].layerMask = ~0u;

			Entity entity = vis.scene->lights.GetEntity(lightIndex);
			const LayerComponent* layer = vis.scene->layers.GetComponent(entity);
			if (layer != nullptr)
			{
				entityArray[entityCounter].layerMask = layer->layerMask;
			}

			entityArray[entityCounter].SetType(light.GetType());
			entityArray[entityCounter].position = light.position;
			entityArray[entityCounter].SetRange(light.GetRange());
			entityArray[entityCounter].color = wiMath::CompressColor(light.color);
			entityArray[entityCounter].SetEnergy(light.energy);

			// mark as no shadow by default:
			entityArray[entityCounter].indices = ~0;

			bool shadow = light.IsCastingShadow() && !light.IsStatic();

			if (GetRaytracedShadowsEnabled() && shadow)
			{
				entityArray[entityCounter].SetIndices(matrixCounter, 0);
			}
			else if(shadow)
			{
				shadow = light.IsCastingShadow() && !light.IsStatic();
				if (shadow)
				{
					switch (light.GetType())
					{
					case LightComponent::DIRECTIONAL:
						if (shadowCounter_2D < SHADOWCOUNT_2D - CASCADE_COUNT + 1)
						{
							entityArray[entityCounter].SetIndices(matrixCounter, shadowCounter_2D);
							shadowCounter_2D += CASCADE_COUNT;
						}
						break;
					case LightComponent::SPOT:
						if (shadowCounter_2D < SHADOWCOUNT_2D)
						{
							entityArray[entityCounter].SetIndices(matrixCounter, shadowCounter_2D);
							shadowCounter_2D += 1;
						}
						break;
					default:
						if (shadowCounter_Cube < SHADOWCOUNT_CUBE)
						{
							entityArray[entityCounter].SetIndices(matrixCounter, shadowCounter_Cube);
							shadowCounter_Cube += 1;
						}
						break;
					}
				}
			}

			switch (light.GetType())
			{
			case LightComponent::DIRECTIONAL:
			{
				entityArray[entityCounter].SetDirection(light.direction);

				if (shadow)
				{
					std::array<SHCAM, CASCADE_COUNT> shcams;
					CreateDirLightShadowCams(light, *vis.camera, shcams);
					matrixArray[matrixCounter++] = shcams[0].VP;
					matrixArray[matrixCounter++] = shcams[1].VP;
					matrixArray[matrixCounter++] = shcams[2].VP;
				}
			}
			break;
			case LightComponent::POINT:
			{
				if (shadow)
				{
					const float FarZ = 0.1f;	// watch out: reversed depth buffer! Also, light near plane is constant for simplicity, this should match on cpu side!
					const float NearZ = std::max(1.0f, light.GetRange()); // watch out: reversed depth buffer!
					const float fRange = FarZ / (FarZ - NearZ);
					const float cubemapDepthRemapNear = fRange;
					const float cubemapDepthRemapFar = -fRange * NearZ;
					entityArray[entityCounter].SetCubeRemapNear(cubemapDepthRemapNear);
					entityArray[entityCounter].SetCubeRemapFar(cubemapDepthRemapFar);
				}
			}
			break;
			case LightComponent::SPOT:
			{
				entityArray[entityCounter].SetConeAngleCos(cosf(light.fov * 0.5f));
				entityArray[entityCounter].SetDirection(light.direction);

				if (shadow)
				{
					SHCAM shcam;
					CreateSpotLightShadowCam(light, shcam);
					matrixArray[matrixCounter++] = shcam.VP;
				}
			}
			break;
			}

			if (light.IsStatic())
			{
				entityArray[entityCounter].SetFlags(ENTITY_FLAG_LIGHT_STATIC);
			}

			entityCounter++;
		}

		// Write force fields into entity array:
		for (size_t i = 0; i < vis.scene->forces.GetCount(); ++i)
		{
			if (entityCounter == SHADER_ENTITY_COUNT)
			{
				assert(0); // too many entities!
				entityCounter--;
				break;
			}

			const ForceFieldComponent& force = vis.scene->forces[i];

			entityArray[entityCounter] = {}; // zero out!
			entityArray[entityCounter].layerMask = ~0u;

			Entity entity = vis.scene->forces.GetEntity(i);
			const LayerComponent* layer = vis.scene->layers.GetComponent(entity);
			if (layer != nullptr)
			{
				entityArray[entityCounter].layerMask = layer->layerMask;
			}

			entityArray[entityCounter].SetType(force.type);
			entityArray[entityCounter].position = force.position;
			entityArray[entityCounter].SetEnergy(force.gravity);
			entityArray[entityCounter].SetRange(1.0f / std::max(0.0001f, force.GetRange())); // avoid division in shader
			entityArray[entityCounter].SetConeAngleCos(force.GetRange()); // this will be the real range in the less common shaders...
			// The default planar force field is facing upwards, and thus the pull direction is downwards:
			entityArray[entityCounter].SetDirection(force.direction);

			entityCounter++;
		}

		// Issue GPU entity array update:
		device->UpdateBuffer(&resourceBuffers[RBTYPE_ENTITYARRAY], entityArray, cmd, sizeof(ShaderEntity)*entityCounter);
		device->UpdateBuffer(&resourceBuffers[RBTYPE_MATRIXARRAY], matrixArray, cmd, sizeof(XMMATRIX)*matrixCounter);

		// Temporary array for GPU entities can be freed now:
		GetRenderFrameAllocator(cmd).free(sizeof(ShaderEntity)*SHADER_ENTITY_COUNT);
		GetRenderFrameAllocator(cmd).free(sizeof(XMMATRIX)*MATRIXARRAY_COUNT);
	}

	auto range = wiProfiler::BeginRangeGPU("Skinning", cmd);
	device->EventBegin("Skinning", cmd);
	{
		bool streamOutSetUp = false;
		SHADERTYPE lastCS = CSTYPE_SKINNING_LDS;
		GPUBarrier* barriers_start = (GPUBarrier*)GetRenderFrameAllocator(cmd).top();
		uint32_t numBarriers = 0;

		for (size_t i = 0; i < vis.scene->meshes.GetCount(); ++i)
		{
			Entity entity = vis.scene->meshes.GetEntity(i);
			const MeshComponent& mesh = vis.scene->meshes[i];

			if (mesh.dirty_bindless && device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_BINDLESS_DESCRIPTORS))
			{
				mesh.dirty_bindless = false;

				ShaderMesh shadermesh;
				mesh.WriteShaderMesh(&shadermesh);

				int mesh_descriptor = device->GetDescriptorIndex(&mesh.descriptor, SRV);

				size_t tmp_alloc = sizeof(ShaderMeshSubset) * mesh.subsets.size();
				ShaderMeshSubset* subsetarray = (ShaderMeshSubset*)GetRenderFrameAllocator(cmd).allocate(tmp_alloc);
				int j = 0;
				for (auto& x : mesh.subsets)
				{
					ShaderMeshSubset& shadersubset = subsetarray[j++];
					shadersubset.indexOffset = x.indexOffset;
					shadersubset.indexCount = x.indexCount;
					shadersubset.mesh = mesh_descriptor;

					const MaterialComponent* material = vis.scene->materials.GetComponent(x.materialID);
					if (material != nullptr)
					{
						shadersubset.material = device->GetDescriptorIndex(&material->constantBuffer, SRV);
					}
				}

				device->UpdateBuffer(&mesh.descriptor, &shadermesh, cmd);
				device->UpdateBuffer(&mesh.subsetBuffer, subsetarray, cmd);
				GetRenderFrameAllocator(cmd).free(tmp_alloc);
			}

			if (mesh.dirty_morph)
			{
				mesh.dirty_morph = false;
				wiRenderer::GetDevice()->UpdateBuffer(&mesh.vertexBuffer_POS, mesh.vertex_positions_morphed.data(), cmd);
			}

			if (mesh.IsSkinned() && vis.scene->armatures.Contains(mesh.armatureID))
			{
				const SoftBodyPhysicsComponent* softbody = vis.scene->softbodies.GetComponent(entity);
				if (softbody != nullptr && softbody->physicsobject != nullptr)
				{
					// If soft body simulation is active, don't perform skinning.
					//	(Soft body animated vertices are skinned in simulation phase by physics system)
					continue;
				}

				const ArmatureComponent& armature = *vis.scene->armatures.GetComponent(mesh.armatureID);

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
				const GPUResource* uavs[] = {
					&mesh.streamoutBuffer_POS,
					&mesh.streamoutBuffer_TAN,
				};

				numBarriers += arraysize(uavs);
				GPUBarrier* barriers = (GPUBarrier*)GetRenderFrameAllocator(cmd).allocate(sizeof(GPUBarrier) * arraysize(uavs));
				barriers[0] = GPUBarrier::Buffer(&mesh.streamoutBuffer_POS, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE);
				barriers[1] = GPUBarrier::Buffer(&mesh.streamoutBuffer_TAN, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE);
				// Barriers will be issued later...

				device->BindResources(CS, vbs, SKINNINGSLOT_IN_VERTEX_POS, arraysize(vbs), cmd);
				device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

				device->Dispatch(((uint32_t)mesh.vertex_positions.size() + SKINNING_COMPUTE_THREADCOUNT - 1) / SKINNING_COMPUTE_THREADCOUNT, 1, 1, cmd);
			}

		}

		if (streamOutSetUp)
		{
			numBarriers++;
			GPUBarrier* barriers = (GPUBarrier*)GetRenderFrameAllocator(cmd).allocate(sizeof(GPUBarrier));
			barriers[0] = GPUBarrier::Memory();
			device->Barrier(barriers_start, numBarriers, cmd);
			GetRenderFrameAllocator(cmd).free(sizeof(GPUBarrier) * numBarriers);

			device->UnbindUAVs(0, 2, cmd);
			device->UnbindResources(SKINNINGSLOT_IN_VERTEX_POS, 4, cmd);
		}

	}
	device->EventEnd(cmd);
	wiProfiler::EndRange(range); // skinning

	// Soft body updates:
	for (size_t i = 0; i < vis.scene->softbodies.GetCount(); ++i)
	{
		Entity entity = vis.scene->softbodies.GetEntity(i);
		const SoftBodyPhysicsComponent& softbody = vis.scene->softbodies[i];

		const MeshComponent* mesh = vis.scene->meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			device->UpdateBuffer(&mesh->streamoutBuffer_POS, softbody.vertex_positions_simulation.data(), cmd);
			device->UpdateBuffer(&mesh->streamoutBuffer_TAN, softbody.vertex_tangents_simulation.data(), cmd);
		}
	}

	// GPU Particle systems simulation/sorting/culling:
	if (!vis.visibleEmitters.empty())
	{
		range = wiProfiler::BeginRangeGPU("EmittedParticles - Simulate", cmd);
		for (uint32_t emitterIndex : vis.visibleEmitters)
		{
			const wiEmittedParticle& emitter = vis.scene->emitters[emitterIndex];
			Entity entity = vis.scene->emitters.GetEntity(emitterIndex);
			const TransformComponent& transform = *vis.scene->transforms.GetComponent(entity);
			const MaterialComponent& material = *vis.scene->materials.GetComponent(entity);
			const MeshComponent* mesh = vis.scene->meshes.GetComponent(emitter.meshID);

			emitter.UpdateGPU(transform, material, mesh, cmd);
		}
		wiProfiler::EndRange(range);
	}

	// Hair particle systems GPU simulation:
	if (!vis.visibleHairs.empty())
	{
		range = wiProfiler::BeginRangeGPU("HairParticles - Simulate", cmd);
		for (uint32_t hairIndex : vis.visibleHairs)
		{
			const wiHairParticle& hair = vis.scene->hairs[hairIndex];
			const MeshComponent* mesh = vis.scene->meshes.GetComponent(hair.meshID);

			if (mesh != nullptr)
			{
				Entity entity = vis.scene->hairs.GetEntity(hairIndex);
				const MaterialComponent& material = *vis.scene->materials.GetComponent(entity);

				hair.UpdateGPU(*mesh, material, cmd);
			}
		}
		wiProfiler::EndRange(range);
	}

	// Compute water simulation:
	if (vis.scene->weather.IsOceanEnabled())
	{
		range = wiProfiler::BeginRangeGPU("Ocean - Simulate", cmd);
		vis.scene->ocean.UpdateDisplacementMap(vis.scene->weather.oceanParameters, cmd);
		wiProfiler::EndRange(range);
	}

	if (vis.scene->weather.IsRealisticSky())
	{
		// Render Atmospheric Scattering textures for lighting and sky
		RenderAtmosphericScatteringTextures(cmd);
	}


	// Precompute static volumetric cloud textures:
	if (!volumetric_clouds_precomputed && vis.scene->weather.IsVolumetricClouds())
	{
		// Shape Noise pass:
		{
			device->EventBegin("Shape Noise", cmd);
			device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_SHAPENOISE], cmd);

			const GPUResource* uavs[] = {
				&texture_shapeNoise,
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&texture_shapeNoise, texture_shapeNoise.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			const int threadSize = 8;
			const int noiseThreadXY = static_cast<uint32_t>(std::ceil(texture_shapeNoise.GetDesc().Width / threadSize));
			const int noiseThreadZ = static_cast<uint32_t>(std::ceil(texture_shapeNoise.GetDesc().Depth / threadSize));

			device->Dispatch(noiseThreadXY, noiseThreadXY, noiseThreadZ, cmd);

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

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&texture_detailNoise, texture_detailNoise.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			const int threadSize = 8;
			const int noiseThreadXYZ = static_cast<uint32_t>(std::ceil(texture_detailNoise.GetDesc().Width / threadSize));

			device->Dispatch(noiseThreadXYZ, noiseThreadXYZ, noiseThreadXYZ, cmd);

			device->UnbindUAVs(0, arraysize(uavs), cmd);
			device->EventEnd(cmd);
		}

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&texture_shapeNoise, IMAGE_LAYOUT_UNORDERED_ACCESS, texture_shapeNoise.desc.layout),
				GPUBarrier::Image(&texture_detailNoise, IMAGE_LAYOUT_UNORDERED_ACCESS, texture_detailNoise.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
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

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&texture_curlNoise, texture_curlNoise.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			const int threadSize = 16;
			const int curlRes = texture_curlNoise.GetDesc().Width;
			const int curlThread = static_cast<uint32_t>(std::ceil(curlRes / threadSize));

			device->Dispatch(curlThread, curlThread, 1, cmd);

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

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&texture_weatherMap, texture_weatherMap.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			const int threadSize = 16;
			const int weatherMapRes = texture_weatherMap.GetDesc().Width;
			const int weatherThread = static_cast<uint32_t>(std::ceil(weatherMapRes / threadSize));

			device->Dispatch(weatherThread, weatherThread, 1, cmd);
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
					GPUBarrier::Image(&texture_weatherMap, IMAGE_LAYOUT_UNORDERED_ACCESS, texture_weatherMap.desc.layout),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->UnbindUAVs(0, arraysize(uavs), cmd);
			device->EventEnd(cmd);
		}
		volumetric_clouds_precomputed = true;
	}
}
void UpdateRaytracingAccelerationStructures(const Scene& scene, CommandList cmd)
{
	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_PIPELINE) || device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
	{

		if (!scene.TLAS.IsValid())
			return;

		// BLAS:
		{
			auto rangeCPU = wiProfiler::BeginRangeCPU("BLAS Update (CPU)");
			auto range = wiProfiler::BeginRangeGPU("BLAS Update (GPU)", cmd);
			device->EventBegin("BLAS Update", cmd);

			for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
			{
				const MeshComponent& mesh = scene.meshes[i];
				if (mesh.BLAS.IsValid())
				{
					switch (mesh.BLAS_state)
					{
					default:
					case MeshComponent::BLAS_STATE_COMPLETE:
						break;
					case MeshComponent::BLAS_STATE_NEEDS_REBUILD:
						device->BuildRaytracingAccelerationStructure(&mesh.BLAS, cmd, nullptr);
						break;
					case MeshComponent::BLAS_STATE_NEEDS_REFIT:
						device->BuildRaytracingAccelerationStructure(&mesh.BLAS, cmd, &mesh.BLAS);
						break;
					}
					mesh.BLAS_state = MeshComponent::BLAS_STATE_COMPLETE;
				}
			}

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->EventEnd(cmd);
			wiProfiler::EndRange(range);
			wiProfiler::EndRange(rangeCPU);
		}

		// TLAS:
		{
			auto rangeCPU = wiProfiler::BeginRangeCPU("TLAS Update (CPU)");
			auto range = wiProfiler::BeginRangeGPU("TLAS Update (GPU)", cmd);
			device->EventBegin("TLAS Update", cmd);

			device->UpdateBuffer(&scene.TLAS.desc.toplevel.instanceBuffer, scene.TLAS_instances.data(), cmd);
			device->BuildRaytracingAccelerationStructure(&scene.TLAS, cmd, nullptr);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(&scene.TLAS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->EventEnd(cmd);
			wiProfiler::EndRange(range);
			wiProfiler::EndRange(rangeCPU);
		}
	}
	else
	{
		scene.BVH.Build(scene, cmd);
	}

	scene.acceleration_structure_update_requested = false;
}
void OcclusionCulling_Render(const CameraComponent& camera_previous, const Visibility& vis, CommandList cmd)
{
	if (!GetOcclusionCullingEnabled() || GetFreezeCullingCameraEnabled())
	{
		return;
	}

	auto range = wiProfiler::BeginRangeGPU("Occlusion Culling Render", cmd);

	if (!vis.visibleObjects.empty())
	{
		device->EventBegin("Occlusion Culling Render", cmd);

		int query_write = vis.scene->queryheap_idx;
		const GPUQueryHeap& queryHeap = vis.scene->queryHeap[query_write];

		device->BindPipelineState(&PSO_occlusionquery, cmd);
		const bool bindless = device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_BINDLESS_DESCRIPTORS);

		XMMATRIX VP = camera_previous.GetViewProjection();

		for (uint32_t instanceIndex : vis.visibleObjects)
		{
			const ObjectComponent& object = vis.scene->objects[instanceIndex];
			if (!object.IsRenderable())
			{
				continue;
			}

			int queryIndex = object.occlusionQueries[query_write];
			if (queryIndex >= 0)
			{
				const AABB& aabb = vis.scene->aabb_objects[instanceIndex];

				const XMMATRIX transform = aabb.getAsBoxMatrix() * VP;

				if (bindless)
				{
					device->PushConstants(&transform, sizeof(transform), cmd);
				}
				else
				{
					MiscCB cb;
					XMStoreFloat4x4(&cb.g_xTransform, transform);
					device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &cb, cmd);
					device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

				}

				// render bounding box to later read the occlusion status
				device->QueryBegin(&queryHeap, queryIndex, cmd);
				device->Draw(14, 0, cmd);
				device->QueryEnd(&queryHeap, queryIndex, cmd);
			}
		}

		device->QueryResolve(
			&queryHeap,
			0,
			vis.scene->writtenQueries[query_write],
			cmd
		);

		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range); // Occlusion Culling Render
}

void DrawWaterRipples(const Visibility& vis, CommandList cmd)
{
	// remove camera jittering
	CameraComponent cam = *vis.camera;
	cam.jitter = XMFLOAT2(0, 0);
	cam.UpdateCamera();
	const XMMATRIX VP = cam.GetViewProjection();

	XMVECTOR vvv = abs(vis.reflectionPlane.x) > 0.99f ? XMVectorSet(0, 1, 0, 0) : XMVectorSet(1, 0, 0, 0);
	XMVECTOR dir = XMLoadFloat4(&vis.reflectionPlane);
	XMMATRIX R = XMMatrixLookToLH(XMVectorZero(), dir, XMVector3Cross(vvv, dir));

	device->EventBegin("Water Ripples", cmd);
	for(auto& x : vis.scene->waterRipples)
	{
		x.params.customRotation = &R;
		x.params.customProjection = &VP;
		x.Draw(cmd);
	}
	device->EventEnd(cmd);
}



void DrawSoftParticles(
	const Visibility& vis,
	const Texture& lineardepth,
	bool distortion, 
	CommandList cmd
)
{
	size_t emitterCount = vis.visibleEmitters.size();
	if (emitterCount == 0)
	{
		return;
	}
	auto range = distortion ?
		wiProfiler::BeginRangeGPU("EmittedParticles - Render (Distortion)", cmd) :
		wiProfiler::BeginRangeGPU("EmittedParticles - Render", cmd);

	device->BindResource(PS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	// Sort emitters based on distance:
	assert(emitterCount < 0x0000FFFF); // watch out for sorting hash truncation!
	uint32_t* emitterSortingHashes = (uint32_t*)GetRenderFrameAllocator(cmd).allocate(sizeof(uint32_t) * emitterCount);
	for (size_t i = 0; i < emitterCount; ++i)
	{
		const uint32_t emitterIndex = vis.visibleEmitters[i];
		const wiEmittedParticle& emitter = vis.scene->emitters[emitterIndex];
		float distance = wiMath::DistanceEstimated(emitter.center, vis.camera->Eye);
		emitterSortingHashes[i] = 0;
		emitterSortingHashes[i] |= (uint32_t)i & 0x0000FFFF;
		emitterSortingHashes[i] |= ((uint32_t)(distance * 10) & 0x0000FFFF) << 16;
	}
	std::sort(emitterSortingHashes, emitterSortingHashes + emitterCount, std::greater<uint32_t>());

	for (size_t i = 0; i < emitterCount; ++i)
	{
		const uint32_t emitterIndex = vis.visibleEmitters[emitterSortingHashes[i] & 0x0000FFFF];
		const wiEmittedParticle& emitter = vis.scene->emitters[emitterIndex];
		const Entity entity = vis.scene->emitters.GetEntity(emitterIndex);
		const MaterialComponent& material = *vis.scene->materials.GetComponent(entity);

		if (distortion && emitter.shaderType == wiEmittedParticle::SOFT_DISTORTION)
		{
			emitter.Draw(*vis.camera, material, cmd);
		}
		else if (!distortion && (emitter.shaderType == wiEmittedParticle::SOFT || emitter.shaderType == wiEmittedParticle::SOFT_LIGHTING || emitter.shaderType == wiEmittedParticle::SIMPLE || IsWireRender()))
		{
			emitter.Draw(*vis.camera, material, cmd);
		}
	}

	GetRenderFrameAllocator(cmd).free(sizeof(uint32_t) * emitterCount);

	wiProfiler::EndRange(range);
}
void DrawLightVisualizers(
	const Visibility& vis,
	CommandList cmd
)
{
	if (!vis.visibleLights.empty())
	{
		device->EventBegin("Light Visualizer Render", cmd);

		device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), cmd);
		device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), cmd);

		XMMATRIX camrot = XMLoadFloat3x3(&vis.camera->rotationMatrix);
		XMMATRIX VP = vis.camera->GetViewProjection();

		for (int type = LightComponent::POINT; type < LightComponent::LIGHTTYPE_COUNT; ++type)
		{
			device->BindPipelineState(&PSO_lightvisualizer[type], cmd);

			for (auto visibleLight : vis.visibleLights)
			{
				uint16_t lightIndex = visibleLight.index;
				const LightComponent& light = vis.scene->lights[lightIndex];

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
				}
			}

		}

		device->EventEnd(cmd);

	}
}
void DrawVolumeLights(
	const Visibility& vis,
	const Texture& depthbuffer,
	CommandList cmd
)
{

	if (!vis.visibleLights.empty())
	{
		device->EventBegin("Volumetric Light Render", cmd);

		BindShadowmaps(PS, cmd);

		device->BindResource(PS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT], TEXSLOT_SKYLUMINANCELUT, cmd);

		XMMATRIX VP = vis.camera->GetViewProjection();

		for (int type = 0; type < LightComponent::LIGHTTYPE_COUNT; ++type)
		{
			const PipelineState& pso = PSO_volumetriclight[type];

			if (!pso.IsValid())
			{
				continue;
			}

			device->BindPipelineState(&pso, cmd);

			for (size_t i = 0; i < vis.visibleLights.size(); ++i)
			{
				const uint32_t lightIndex = vis.visibleLights[i].index;
				const LightComponent& light = vis.scene->lights[lightIndex];
				if (light.GetType() == type && light.IsVolumetricsEnabled())
				{

					switch (type)
					{
					case LightComponent::DIRECTIONAL:
					{
						MiscCB miscCb;
						miscCb.g_xColor.x = float(i);
						device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &miscCb, cmd);
						device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
						device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

						device->Draw(3, 0, cmd); // full screen triangle
					}
					break;
					case LightComponent::POINT:
					{
						MiscCB miscCb;
						miscCb.g_xColor.x = float(i);
						float sca = light.GetRange() + 1;
						XMStoreFloat4x4(&miscCb.g_xTransform, XMMatrixScaling(sca, sca, sca)*XMMatrixTranslationFromVector(XMLoadFloat3(&light.position)) * VP);
						device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &miscCb, cmd);
						device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
						device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

						device->Draw(240, 0, cmd); // icosphere
					}
					break;
					case LightComponent::SPOT:
					{
						MiscCB miscCb;
						miscCb.g_xColor.x = float(i);
						const float coneS = (const float)(light.fov / XM_PIDIV4);
						XMStoreFloat4x4(&miscCb.g_xTransform, 
							XMMatrixScaling(coneS*light.GetRange(), light.GetRange(), coneS*light.GetRange())*
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&light.position)) *
							VP
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
	const Visibility& vis,
	const Texture& depthbuffer,
	CommandList cmd,
	const Texture* texture_directional_occlusion
)
{
	if (IsWireRender())
		return;

	device->EventBegin("Lens Flares", cmd);

	device->BindResource(VS, &depthbuffer, TEXSLOT_DEPTH, cmd);

	for (auto visibleLight : vis.visibleLights)
	{
		uint16_t lightIndex = visibleLight.index;
		const LightComponent& light = vis.scene->lights[lightIndex];

		if (!light.lensFlareRimTextures.empty())
		{
			XMVECTOR POS;

			if (light.GetType() == LightComponent::DIRECTIONAL)
			{
				// directional light flare will be placed at infinite position along direction vector:
				XMVECTOR D = XMVector3Normalize(-XMVector3Transform(XMVectorSet(0, 1, 0, 1), XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation))));
				if (XMVectorGetX(XMVector3Dot(D, XMVectorSet(0, -1, 0, 0))) < 0)
					continue; // sun below horizon, skip lensflare
				POS = vis.camera->GetEye() + D * -vis.camera->zFarP;

				// Directional light can use occlusion texture (eg. clouds):
				if (texture_directional_occlusion == nullptr)
				{
					device->BindResource(VS, wiTextureHelper::getWhite(), TEXSLOT_ONDEMAND0, cmd);
				}
				else
				{
					device->BindResource(VS, texture_directional_occlusion, TEXSLOT_ONDEMAND0, cmd);
				}
			}
			else
			{
				// point and spotlight flare will be placed to the source position:
				POS = XMLoadFloat3(&light.position);

				// not using occlusion texture
				device->BindResource(VS, wiTextureHelper::getWhite(), TEXSLOT_ONDEMAND0, cmd);
			}

			if (XMVectorGetX(XMVector3Dot(XMVectorSubtract(POS, vis.camera->GetEye()), vis.camera->GetAt())) > 0) // check if the camera is facing towards the flare or not
			{
				device->BindPipelineState(&PSO_lensflare, cmd);
				device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_LENSFLARE], CB_GETBINDSLOT(LensFlareCB), cmd);

				// Get the screen position of the flare:
				XMVECTOR flarePos = XMVector3Project(POS, 0, 0, 1, 1, 1, 0, vis.camera->GetProjection(), vis.camera->GetView(), XMMatrixIdentity());

				LensFlareCB cb;
				XMStoreFloat3(&cb.xLensFlarePos, flarePos);

				uint32_t i = 0;
				for (auto& x : light.lensFlareRimTextures)
				{
					if (x != nullptr)
					{
						// pre-baked offsets
						// These values work well for me, but should be tweakable
						static const float mods[] = { 1.0f,0.55f,0.4f,0.1f,-0.1f,-0.3f,-0.5f };
						if (i >= arraysize(mods))
							break;

						cb.xLensFlareOffset = mods[i];
						cb.xLensFlareSize.x = (float)x->texture.desc.Width;
						cb.xLensFlareSize.y = (float)x->texture.desc.Height;

						device->UpdateBuffer(&constantBuffers[CBTYPE_LENSFLARE], &cb, cmd);

						device->BindResource(PS, &x->texture, TEXSLOT_ONDEMAND1, cmd);
						device->Draw(4, 0, cmd);
						i++;
					}
				}
			}

		}

	}

	device->EventEnd(cmd);
}


void SetShadowProps2D(int resolution, int count)
{
	if (resolution >= 0)
	{
		SHADOWRES_2D = resolution;
	}
	if (count >= 0)
	{
		SHADOWCOUNT_2D = count;
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
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		desc.clear.color[0] = 1;
		desc.clear.color[1] = 1;
		desc.clear.color[2] = 1;
		desc.clear.color[3] = 0;
		device->CreateTexture(&desc, nullptr, &shadowMapArray_Transparent_2D);

		renderpasses_shadow2D.resize(SHADOWCOUNT_2D);

		for (uint32_t i = 0; i < SHADOWCOUNT_2D; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&shadowMapArray_2D, DSV, i, 1, 0, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&shadowMapArray_Transparent_2D, RTV, i, 1, 0, 1);
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

			renderpassdesc.attachments.push_back(
				RenderPassAttachment::RenderTarget(
					&shadowMapArray_Transparent_2D,
					RenderPassAttachment::LOADOP_CLEAR,
					RenderPassAttachment::STOREOP_STORE,
					IMAGE_LAYOUT_SHADER_RESOURCE,
					IMAGE_LAYOUT_RENDERTARGET,
					IMAGE_LAYOUT_SHADER_RESOURCE
				)
			);
			renderpassdesc.attachments.back().subresource = subresource_index;

			device->CreateRenderPass(&renderpassdesc, &renderpasses_shadow2D[subresource_index]);
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
		desc.SampleCount = 1;
		desc.Usage = USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;

		desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R16_TYPELESS;
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		device->CreateTexture(&desc, nullptr, &shadowMapArray_Cube);

		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		desc.clear.color[0] = 1;
		desc.clear.color[1] = 1;
		desc.clear.color[2] = 1;
		desc.clear.color[3] = 0;
		device->CreateTexture(&desc, nullptr, &shadowMapArray_Transparent_Cube);

		renderpasses_shadowCube.resize(SHADOWCOUNT_CUBE);

		for (uint32_t i = 0; i < SHADOWCOUNT_CUBE; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(&shadowMapArray_Cube, DSV, i * 6, 6, 0, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(&shadowMapArray_Transparent_Cube, RTV, i * 6, 6, 0, 1);
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

			renderpassdesc.attachments.push_back(
				RenderPassAttachment::RenderTarget(
					&shadowMapArray_Transparent_Cube,
					RenderPassAttachment::LOADOP_CLEAR,
					RenderPassAttachment::STOREOP_STORE,
					IMAGE_LAYOUT_SHADER_RESOURCE,
					IMAGE_LAYOUT_RENDERTARGET,
					IMAGE_LAYOUT_SHADER_RESOURCE
				)
			);
			renderpassdesc.attachments.back().subresource = subresource_index;

			device->CreateRenderPass(&renderpassdesc, &renderpasses_shadowCube[subresource_index]);
		}
	}

}
void DrawShadowmaps(
	const Visibility& vis,
	CommandList cmd
)
{
	if (IsWireRender())
		return;

	if (!vis.visibleLights.empty())
	{
		device->EventBegin("DrawShadowmaps", cmd);
		auto range = wiProfiler::BeginRangeGPU("Shadow Rendering", cmd);

		BindCommonResources(cmd);
		BindConstantBuffers(VS, cmd);
		BindConstantBuffers(PS, cmd);

		BoundingFrustum cam_frustum;
		BoundingFrustum::CreateFromMatrix(cam_frustum, vis.camera->GetProjection());
		std::swap(cam_frustum.Near, cam_frustum.Far);
		cam_frustum.Transform(cam_frustum, vis.camera->GetInvView());
		XMStoreFloat4(&cam_frustum.Orientation, XMQuaternionNormalize(XMLoadFloat4(&cam_frustum.Orientation)));

		device->UnbindResources(TEXSLOT_SHADOWARRAY_2D, 2, cmd);

		uint32_t shadowCounter_2D = SHADOWRES_2D > 0 ? 0 : SHADOWCOUNT_2D;
		uint32_t shadowCounter_Cube = SHADOWRES_CUBE > 0 ? 0 : SHADOWCOUNT_CUBE;

		for (const auto& visibleLight : vis.visibleLights)
		{
			if (shadowCounter_2D >= SHADOWCOUNT_2D && shadowCounter_Cube >= SHADOWCOUNT_CUBE)
			{
				break;
			}

			uint16_t lightIndex = visibleLight.index;
			const LightComponent& light = vis.scene->lights[lightIndex];
			
			bool shadow = light.IsCastingShadow() && !light.IsStatic();
			if (!shadow)
			{
				continue;
			}

			switch (light.GetType())
			{
			case LightComponent::DIRECTIONAL:
			{
				if (shadowCounter_2D >= SHADOWCOUNT_2D - CASCADE_COUNT + 1)
					break;
				uint32_t slice = shadowCounter_2D;
				shadowCounter_2D += CASCADE_COUNT;

				std::array<SHCAM, CASCADE_COUNT> shcams;
				CreateDirLightShadowCams(light, *vis.camera, shcams);

				for (uint32_t cascade = 0; cascade < CASCADE_COUNT; ++cascade)
				{
					RenderQueue renderQueue;
					bool transparentShadowsRequested = false;
					for (size_t i = 0; i < vis.scene->aabb_objects.GetCount(); ++i)
					{
						const AABB& aabb = vis.scene->aabb_objects[i];
						if ((aabb.layerMask & vis.layerMask) && shcams[cascade].frustum.CheckBoxFast(aabb))
						{
							const ObjectComponent& object = vis.scene->objects[i];
							if (object.IsRenderable() && object.IsCastingShadow() && (cascade < (CASCADE_COUNT - object.cascadeMask)))
							{
								Entity cullable_entity = vis.scene->aabb_objects.GetEntity(i);

								RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
								size_t meshIndex = vis.scene->meshes.GetIndex(object.meshID);
								batch->Create(meshIndex, i, 0);
								renderQueue.add(batch);

								if (object.GetRenderTypes() & RENDERTYPE_TRANSPARENT || object.GetRenderTypes() & RENDERTYPE_WATER)
								{
									transparentShadowsRequested = true;
								}
							}
						}
					}

					device->RenderPassBegin(&renderpasses_shadow2D[slice + cascade], cmd);
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

						RenderMeshes(vis, renderQueue, RENDERPASS_SHADOW, RENDERTYPE_OPAQUE, cmd);
						if (GetTransparentShadowsEnabled() && transparentShadowsRequested)
						{
							RenderMeshes(vis, renderQueue, RENDERPASS_SHADOW, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, cmd);
						}

						GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
					}
					device->RenderPassEnd(cmd);

				}
			}
			break;
			case LightComponent::SPOT:
			{
				if (shadowCounter_2D >= SHADOWCOUNT_2D)
					break;
				uint32_t slice = shadowCounter_2D;
				shadowCounter_2D += 1;

				SHCAM shcam;
				CreateSpotLightShadowCam(light, shcam);
				if (!cam_frustum.Intersects(shcam.boundingfrustum))
					break;

				RenderQueue renderQueue;
				bool transparentShadowsRequested = false;
				for (size_t i = 0; i < vis.scene->aabb_objects.GetCount(); ++i)
				{
					const AABB& aabb = vis.scene->aabb_objects[i];
					if ((aabb.layerMask & vis.layerMask) && shcam.frustum.CheckBoxFast(aabb))
					{
						const ObjectComponent& object = vis.scene->objects[i];
						if (object.IsRenderable() && object.IsCastingShadow())
						{
							Entity cullable_entity = vis.scene->aabb_objects.GetEntity(i);

							RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
							size_t meshIndex = vis.scene->meshes.GetIndex(object.meshID);
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

					device->RenderPassBegin(&renderpasses_shadow2D[slice], cmd);
					RenderMeshes(vis, renderQueue, RENDERPASS_SHADOW, RENDERTYPE_OPAQUE, cmd);
					if (GetTransparentShadowsEnabled() && transparentShadowsRequested)
					{
						RenderMeshes(vis, renderQueue, RENDERPASS_SHADOW, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, cmd);
					}
					device->RenderPassEnd(cmd);

					GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
				}

			}
			break;
			case LightComponent::POINT:
			{
				if (shadowCounter_Cube >= SHADOWCOUNT_CUBE)
					break;
				uint32_t slice = shadowCounter_Cube;
				shadowCounter_Cube += 1;

				SPHERE boundingsphere = SPHERE(light.position, light.GetRange());

				RenderQueue renderQueue;
				bool transparentShadowsRequested = false;
				for (size_t i = 0; i < vis.scene->aabb_objects.GetCount(); ++i)
				{
					const AABB& aabb = vis.scene->aabb_objects[i];
					if ((aabb.layerMask & vis.layerMask) && boundingsphere.intersects(aabb))
					{
						const ObjectComponent& object = vis.scene->objects[i];
						if (object.IsRenderable() && object.IsCastingShadow())
						{
							Entity cullable_entity = vis.scene->aabb_objects.GetEntity(i);

							RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
							size_t meshIndex = vis.scene->meshes.GetIndex(object.meshID);
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

					device->RenderPassBegin(&renderpasses_shadowCube[slice], cmd);
					RenderMeshes(vis, renderQueue, RENDERPASS_SHADOWCUBE, RENDERTYPE_OPAQUE, cmd, false, frusta, frustum_count);
					if (GetTransparentShadowsEnabled() && transparentShadowsRequested)
					{
						RenderMeshes(vis, renderQueue, RENDERPASS_SHADOWCUBE, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, cmd, false, frusta, frustum_count);
					}
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
	const Visibility& vis,
	RENDERPASS renderPass,
	CommandList cmd,
	uint32_t flags
)
{
	const bool opaque = flags & RENDERTYPE_OPAQUE;
	const bool transparent = flags & DRAWSCENE_TRANSPARENT;
	const bool tessellation = (flags & DRAWSCENE_TESSELLATION) && GetTessellationEnabled();
	const bool hairparticle = flags & DRAWSCENE_HAIRPARTICLE;
	const bool occlusion = flags & DRAWSCENE_OCCLUSIONCULLING;

	device->EventBegin("DrawScene", cmd);
	device->BindShadingRate(SHADING_RATE_1X1, cmd);

	BindCommonResources(cmd);
	BindShadowmaps(PS, cmd);
	BindConstantBuffers(VS, cmd);
	BindConstantBuffers(PS, cmd);

	device->BindResource(PS, &vis.scene->envmapArray, TEXSLOT_ENVMAPARRAY, cmd);
	device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
	device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
	device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);
	device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT], TEXSLOT_SKYLUMINANCELUT, cmd);
	device->BindResource(PS, GetVoxelRadianceSecondaryBounceEnabled() ? &textures[TEXTYPE_3D_VOXELRADIANCE_HELPER] : &textures[TEXTYPE_3D_VOXELRADIANCE], TEXSLOT_VOXELRADIANCE, cmd);

	if (vis.scene->weather.skyMap != nullptr)
	{
		device->BindResource(PS, &vis.scene->weather.skyMap->texture, TEXSLOT_GLOBALENVMAP, cmd);
	}

	device->BindResource(PS, &vis.scene->lightmap, TEXSLOT_GLOBALLIGHTMAP, cmd);
	if (vis.scene->decalAtlas.IsValid())
	{
		device->BindResource(PS, &vis.scene->decalAtlas, TEXSLOT_DECALATLAS, cmd);
	}

	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_PIPELINE) || device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
	{
		device->BindResource(PS, &vis.scene->TLAS, TEXSLOT_ACCELERATION_STRUCTURE, cmd);
		device->BindResource(CS, &vis.scene->TLAS, TEXSLOT_ACCELERATION_STRUCTURE, cmd);
	}

	if (transparent && vis.scene->weather.IsOceanEnabled())
	{
		vis.scene->ocean.Render(*vis.camera, vis.scene->weather.oceanParameters, cmd);
	}

	if (hairparticle)
	{
		if (!transparent)
		{
			for (uint32_t hairIndex : vis.visibleHairs)
			{
				const wiHairParticle& hair = vis.scene->hairs[hairIndex];
				Entity entity = vis.scene->hairs.GetEntity(hairIndex);
				const MaterialComponent& material = *vis.scene->materials.GetComponent(entity);

				hair.Draw(*vis.camera, material, renderPass, cmd);
			}
		}
	}

	if (IsWireRender() && !transparent)
		return;

	RenderImpostors(vis, renderPass, cmd);

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

	if (IsWireRender())
	{
		renderTypeFlags = RENDERTYPE_ALL;
	}

	RenderQueue renderQueue;
	for (uint32_t instanceIndex : vis.visibleObjects)
	{
		const ObjectComponent& object = vis.scene->objects[instanceIndex];

		if (GetOcclusionCullingEnabled() && occlusion && object.IsOccluded())
			continue;

		if (object.IsRenderable() && (object.GetRenderTypes() & renderTypeFlags))
		{
			const float distance = wiMath::Distance(vis.camera->Eye, object.center);
			if (object.IsImpostorPlacement() && distance > object.impostorSwapDistance + object.impostorFadeThresholdRadius)
			{
				continue;
			}
			RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
			size_t meshIndex = vis.scene->meshes.GetIndex(object.meshID);
			batch->Create(meshIndex, instanceIndex, distance);
			renderQueue.add(batch);
		}
	}
	if (!renderQueue.empty())
	{
		renderQueue.sort(transparent ? RenderQueue::SORT_BACK_TO_FRONT : RenderQueue::SORT_FRONT_TO_BACK);
		RenderMeshes(vis, renderQueue, renderPass, renderTypeFlags, cmd, tessellation);

		GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
	}

	device->BindShadingRate(SHADING_RATE_1X1, cmd);
	device->EventEnd(cmd);

}

void DrawDebugWorld(
	const Scene& scene,
	const CameraComponent& camera,
	const wiCanvas& canvas,
	CommandList cmd
)
{
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
		XMStoreFloat4x4(&sb.g_xTransform, canvas.GetProjection());
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


	if (debugEnvProbes && scene.envmapArray.IsValid())
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
				device->BindResource(PS, &scene.envmapArray, TEXSLOT_ONDEMAND0, cmd, scene.envmapArray.GetDesc().MipLevels + probe.textureIndex);
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

		device->BindResource(VS, GetVoxelRadianceSecondaryBounceEnabled() ? &textures[TEXTYPE_3D_VOXELRADIANCE_HELPER] : &textures[TEXTYPE_3D_VOXELRADIANCE], TEXSLOT_VOXELRADIANCE, cmd);


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

			if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_BINDLESS_DESCRIPTORS))
			{
				ObjectPushConstants push;
				push.material = device->GetDescriptorIndex(&material.constantBuffer, SRV);
				push.mesh = device->GetDescriptorIndex(&mesh.descriptor, SRV);
				push.instances = device->GetDescriptorIndex(mem.buffer, SRV);
				push.instance_offset = mem.offset;
				device->PushConstants(&push, sizeof(push), cmd);
			}
			else
			{
				const GPUBuffer* vbs[] = {
					mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
					nullptr,
					&mesh.vertexBuffer_UV0,
					&mesh.vertexBuffer_UV1,
					nullptr,
					nullptr,
					nullptr,
					mem.buffer
				};
				uint32_t strides[] = {
					sizeof(MeshComponent::Vertex_POS),
					sizeof(MeshComponent::Vertex_POS),
					sizeof(MeshComponent::Vertex_TEX),
					sizeof(MeshComponent::Vertex_TEX),
					sizeof(MeshComponent::Vertex_TEX),
					sizeof(MeshComponent::Vertex_COL),
					sizeof(MeshComponent::Vertex_TAN),
					sizeof(Instance)
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

				device->BindConstantBuffer(VS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);
				device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);
			}

			device->BindIndexBuffer(&mesh.indexBuffer, mesh.GetIndexFormat(), 0, cmd);

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

		static GPUBuffer wirecamVB;
		static GPUBuffer wirecamIB;
		if (!wirecamVB.IsValid())
		{
			XMFLOAT4 verts[] = {
				XMFLOAT4(-0.1f,-0.1f,-1,1),	XMFLOAT4(1,1,1,1),
				XMFLOAT4(0.1f,-0.1f,-1,1),	XMFLOAT4(1,1,1,1),
				XMFLOAT4(0.1f,0.1f,-1,1),	XMFLOAT4(1,1,1,1),
				XMFLOAT4(-0.1f,0.1f,-1,1),	XMFLOAT4(1,1,1,1),
				XMFLOAT4(-1,-1,1,1),	XMFLOAT4(1,1,1,1),
				XMFLOAT4(1,-1,1,1),	XMFLOAT4(1,1,1,1),
				XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1),
				XMFLOAT4(-1,1,1,1),	XMFLOAT4(1,1,1,1),
				XMFLOAT4(0,1.5f,1,1),	XMFLOAT4(1,1,1,1),
				XMFLOAT4(0,0,-1,1),	XMFLOAT4(1,1,1,1),
			};

			GPUBufferDesc bd;
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			InitData.pSysMem = verts;
			device->CreateBuffer(&bd, &InitData, &wirecamVB);

			uint16_t indices[] = {
				0,1,1,2,0,3,0,4,1,5,4,5,
				5,6,4,7,2,6,3,7,2,3,6,7,
				6,8,7,8,
				0,2,1,3
			};

			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(indices);
			bd.BindFlags = BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;
			InitData.pSysMem = indices;
			device->CreateBuffer(&bd, &InitData, &wirecamIB);
		}

		device->BindPipelineState(&PSO_debug[DEBUGRENDERING_CUBE], cmd);

		const GPUBuffer* vbs[] = {
			&wirecamVB,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		device->BindIndexBuffer(&wirecamIB, INDEXFORMAT_16BIT, 0, cmd);

		MiscCB sb;
		sb.g_xColor = XMFLOAT4(1, 1, 1, 1);

		for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
		{
			const CameraComponent& cam = scene.cameras[i];
			Entity entity = scene.cameras.GetEntity(i);

			const float aspect = cam.width / cam.height;
			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixScaling(aspect * 0.5f, 0.5f, 0.5f) * cam.GetInvView()*camera.GetViewProjection());

			device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &sb, cmd);
			device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);
			device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

			device->DrawIndexed(32, 0, 0, cmd);
		}

		device->EventEnd(cmd);
	}

	if (GetRaytraceDebugBVHVisualizerEnabled())
	{
		RayTraceSceneBVH(scene, cmd);
	}

	device->EventEnd(cmd);
}


void RenderAtmosphericScatteringTextures(CommandList cmd)
{
	device->EventBegin("ComputeAtmosphericScatteringTextures", cmd);
	auto range = wiProfiler::BeginRangeGPU("Atmospheric Scattering Textures", cmd);

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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		const int threadSize = 8;
		const int transmittanceLutWidth = textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT].GetDesc().Width;
		const int transmittanceLutHeight = textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT].GetDesc().Height;
		const int transmittanceLutThreadX = static_cast<uint32_t>(std::ceil(transmittanceLutWidth / threadSize));
		const int transmittanceLutThreadY = static_cast<uint32_t>(std::ceil(transmittanceLutHeight / threadSize));

		device->Dispatch(transmittanceLutThreadX, transmittanceLutThreadY, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], IMAGE_LAYOUT_UNORDERED_ACCESS, textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT].desc.layout)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		const int multiScatteredLutWidth = textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT].GetDesc().Width;
		const int multiScatteredLutHeight = textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT].GetDesc().Height;

		device->Dispatch(multiScatteredLutWidth, multiScatteredLutHeight, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], IMAGE_LAYOUT_UNORDERED_ACCESS, textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT].desc.layout)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Environment Luminance Lut pass:
	{
		device->EventBegin("EnvironmentLuminanceLut", cmd);
		device->BindComputeShader(&shaders[CSTYPE_SKYATMOSPHERE_SKYLUMINANCELUT], cmd);

		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_ONDEMAND1, cmd);

		const GPUResource* uavs[] = {
			&textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT], textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		const int environmentLutWidth = textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT].GetDesc().Width;
		const int environmentLutHeight = textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT].GetDesc().Height;

		device->Dispatch(environmentLutWidth, environmentLutHeight, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT], IMAGE_LAYOUT_UNORDERED_ACCESS, textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT].desc.layout)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		const int threadSize = 8;
		const int skyViewLutWidth = textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT].GetDesc().Width;
		const int skyViewLutHeight = textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT].GetDesc().Height;
		const int skyViewLutThreadX = static_cast<uint32_t>(std::ceil(skyViewLutWidth / threadSize));
		const int skyViewLutThreadY = static_cast<uint32_t>(std::ceil(skyViewLutHeight / threadSize));

		device->Dispatch(skyViewLutThreadX, skyViewLutThreadY, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], IMAGE_LAYOUT_UNORDERED_ACCESS, textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT].desc.layout)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	device->EventEnd(cmd);
}
void DrawSky(const Scene& scene, CommandList cmd)
{
	device->EventBegin("DrawSky", cmd);
	
	if (scene.weather.skyMap != nullptr)
	{
		device->BindPipelineState(&PSO_sky[SKYRENDERING_STATIC], cmd);
		device->BindResource(PS, &scene.weather.skyMap->texture, TEXSLOT_GLOBALENVMAP, cmd);
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
void DrawSkyVelocity(CommandList cmd)
{
	device->EventBegin("DrawSkyVelocity", cmd);

	device->BindPipelineState(&PSO_sky[SKYRENDERING_VELOCITY], cmd);

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


void RefreshEnvProbes(const Visibility& vis, CommandList cmd)
{
	if (!vis.scene->envmapArray.IsValid())
		return;

	device->EventBegin("EnvironmentProbe Refresh", cmd);
	auto range = wiProfiler::BeginRangeGPU("Environment Probe Refresh", cmd);

	BindCommonResources(cmd);

	Viewport vp;
	vp.Height = vp.Width = (float)vis.scene->envmapArray.desc.Width;
	device->BindViewports(1, &vp, cmd);

	const float zNearP = vis.camera->zNearP;
	const float zFarP = vis.camera->zFarP;

	auto render_probe = [&](const EnvironmentProbeComponent& probe, const AABB& probe_aabb) {

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

		if (vis.scene->weather.IsRealisticSky())
		{
			// Refresh atmospheric textures, since each probe has different positions
			RefreshAtmosphericScatteringTextures(cmd);
		}

		// Bind the atmospheric textures, as lighting and sky needs them
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT], TEXSLOT_SKYLUMINANCELUT, cmd);


		device->RenderPassBegin(&vis.scene->renderpasses_envmap[probe.textureIndex], cmd);

		// Scene will only be rendered if this is a real probe entity:
		if (probe_aabb.layerMask & vis.layerMask)
		{
			SPHERE culler = SPHERE(probe.position, zFarP);

			RenderQueue renderQueue;
			for (size_t i = 0; i < vis.scene->aabb_objects.GetCount(); ++i)
			{
				const AABB& aabb = vis.scene->aabb_objects[i];
				if ((aabb.layerMask & vis.layerMask) && (aabb.layerMask & probe_aabb.layerMask) && culler.intersects(aabb))
				{
					const ObjectComponent& object = vis.scene->objects[i];
					if (object.IsRenderable())
					{
						RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
						size_t meshIndex = vis.scene->meshes.GetIndex(object.meshID);
						batch->Create(meshIndex, i, 0);
						renderQueue.add(batch);
					}
				}
			}

			if (!renderQueue.empty())
			{
				BindShadowmaps(PS, cmd);
				device->BindResource(PS, &vis.scene->lightmap, TEXSLOT_GLOBALLIGHTMAP, cmd);

				RenderMeshes(vis, renderQueue, RENDERPASS_ENVMAPCAPTURE, RENDERTYPE_ALL, cmd, false, frusta, arraysize(frusta));

				GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);
			}
		}

		// sky
		{
			if (vis.scene->weather.skyMap != nullptr)
			{
				device->BindPipelineState(&PSO_sky[SKYRENDERING_ENVMAPCAPTURE_STATIC], cmd);
				device->BindResource(PS, &vis.scene->weather.skyMap->texture, TEXSLOT_ONDEMAND0, cmd);
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
		GenerateMipChain(vis.scene->envmapArray, MIPGENFILTER_LINEAR, cmd, mipopt);

		// Filter the enviroment map mip chain according to BRDF:
		//	A bit similar to MIP chain generation, but its input is the MIP-mapped texture,
		//	and we generatethe filtered MIPs from bottom to top.
		device->EventBegin("FilterEnvMap", cmd);
		{
			TextureDesc desc = vis.scene->envmapArray.GetDesc();
			int arrayIndex = probe.textureIndex;

			device->BindComputeShader(&shaders[CSTYPE_FILTERENVMAP], cmd);

			desc.Width = 1;
			desc.Height = 1;
			for (uint32_t i = desc.MipLevels - 1; i > 0; --i)
			{
				{
					GPUBarrier barriers[] = {
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_UNORDERED_ACCESS, i, arrayIndex * 6 + 0),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_UNORDERED_ACCESS, i, arrayIndex * 6 + 1),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_UNORDERED_ACCESS, i, arrayIndex * 6 + 2),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_UNORDERED_ACCESS, i, arrayIndex * 6 + 3),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_UNORDERED_ACCESS, i, arrayIndex * 6 + 4),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_UNORDERED_ACCESS, i, arrayIndex * 6 + 5),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);
				}

				device->BindUAV(CS, &vis.scene->envmapArray, 0, cmd, i);
				device->BindResource(CS, &vis.scene->envmapArray, TEXSLOT_ONDEMAND0, cmd, std::max(0, (int)i - 2));

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

				{
					GPUBarrier barriers[] = {
						GPUBarrier::Memory(),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_UNORDERED_ACCESS, IMAGE_LAYOUT_SHADER_RESOURCE, i, arrayIndex * 6 + 0),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_UNORDERED_ACCESS, IMAGE_LAYOUT_SHADER_RESOURCE, i, arrayIndex * 6 + 1),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_UNORDERED_ACCESS, IMAGE_LAYOUT_SHADER_RESOURCE, i, arrayIndex * 6 + 2),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_UNORDERED_ACCESS, IMAGE_LAYOUT_SHADER_RESOURCE, i, arrayIndex * 6 + 3),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_UNORDERED_ACCESS, IMAGE_LAYOUT_SHADER_RESOURCE, i, arrayIndex * 6 + 4),
						GPUBarrier::Image(&vis.scene->envmapArray, IMAGE_LAYOUT_UNORDERED_ACCESS, IMAGE_LAYOUT_SHADER_RESOURCE, i, arrayIndex * 6 + 5),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);
				}

				desc.Width *= 2;
				desc.Height *= 2;
			}
			device->UnbindUAVs(0, 1, cmd);
		}
		device->EventEnd(cmd);
	};

	if (vis.scene->probes.GetCount() == 0)
	{
		// In this case, there are no probes, so the sky will be rendered to first envmap:
		EnvironmentProbeComponent probe;
		probe.textureIndex = 0;
		probe.position = vis.camera->Eye;

		AABB probe_aabb;
		probe_aabb.layerMask = 0;
		render_probe(probe, probe_aabb);
	}
	else
	{
		for (size_t i = 0; i < vis.scene->probes.GetCount(); ++i)
		{
			const EnvironmentProbeComponent& probe = vis.scene->probes[i];
			const AABB& probe_aabb = vis.scene->aabb_probes[i];

			if ((probe_aabb.layerMask & vis.layerMask) && probe.render_dirty && probe.textureIndex >= 0 && probe.textureIndex < vis.scene->envmapCount)
			{
				probe.render_dirty = false;
				render_probe(probe, probe_aabb);
			}
		}
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd); // EnvironmentProbe Refresh
}

void RefreshImpostors(const Scene& scene, CommandList cmd)
{
	if (!scene.impostorArray.IsValid())
		return;

	device->EventBegin("Impostor Refresh", cmd);

	BindCommonResources(cmd);

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

	for (uint32_t impostorIndex = 0; impostorIndex < scene.impostors.GetCount(); ++impostorIndex)
	{
		const ImpostorComponent& impostor = scene.impostors[impostorIndex];
		if (!impostor.render_dirty)
			continue;
		impostor.render_dirty = false;

		Entity entity = scene.impostors.GetEntity(impostorIndex);
		const MeshComponent& mesh = *scene.meshes.GetComponent(entity);

		// impostor camera will fit around mesh bounding sphere:
		const SPHERE boundingsphere = mesh.GetBoundingSphere();

		ObjectPushConstants push; // used with bindless model only

		const bool bindless = device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_BINDLESS_DESCRIPTORS);
		if (bindless)
		{
			push.mesh = device->GetDescriptorIndex(&mesh.descriptor, SRV);
			push.instances = device->GetDescriptorIndex(mem.buffer, SRV);
			push.instance_offset = mem.offset;
		}
		else
		{
			const GPUBuffer* vbs[] = {
				mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
				mesh.vertexBuffer_PRE.IsValid() ? &mesh.vertexBuffer_PRE : &mesh.vertexBuffer_POS,
				&mesh.vertexBuffer_UV0,
				&mesh.vertexBuffer_UV1,
				&mesh.vertexBuffer_ATL,
				&mesh.vertexBuffer_COL,
				mesh.streamoutBuffer_TAN.IsValid() ? &mesh.streamoutBuffer_TAN : &mesh.vertexBuffer_TAN,
				mem.buffer
			};
			uint32_t strides[] = {
				sizeof(MeshComponent::Vertex_POS),
				sizeof(MeshComponent::Vertex_POS),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(MeshComponent::Vertex_TEX),
				sizeof(MeshComponent::Vertex_COL),
				sizeof(MeshComponent::Vertex_TAN),
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
		}

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

				UpdateCameraCB(impostorcamera, impostorcamera, impostorcamera, cmd);


				int textureIndex = (int)(impostorIndex * impostorCaptureAngles * 3 + prop * impostorCaptureAngles + i);
				device->RenderPassBegin(&scene.renderpasses_impostor[textureIndex], cmd);

				Viewport viewport;
				viewport.Height = (float)scene.impostorTextureDim;
				viewport.Width = (float)scene.impostorTextureDim;
				device->BindViewports(1, &viewport, cmd);

				for (auto& subset : mesh.subsets)
				{
					if (subset.indexCount == 0)
					{
						continue;
					}
					const MaterialComponent& material = *scene.materials.GetComponent(subset.materialID);

					if (bindless)
					{
						push.material = device->GetDescriptorIndex(&material.constantBuffer, SRV);
						device->PushConstants(&push, sizeof(push), cmd);
					}
					else
					{
						device->BindConstantBuffer(VS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);
						device->BindConstantBuffer(PS, &material.constantBuffer, CB_GETBINDSLOT(MaterialCB), cmd);

						const GPUResource* res[4];
						material.WriteTextures(res, arraysize(res));
						device->BindResources(PS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);
					}

					device->DrawIndexedInstanced(subset.indexCount, 1, subset.indexOffset, 0, 0, cmd);
				}

				device->RenderPassEnd(cmd);
			}
		}

	}

	device->EventEnd(cmd);
}

void VoxelRadiance(const Visibility& vis, CommandList cmd)
{
	if (!GetVoxelRadianceEnabled())
	{
		return;
	}

	device->EventBegin("Voxel Radiance", cmd);
	auto range = wiProfiler::BeginRangeGPU("Voxel Radiance", cmd);

	static RenderPass renderpass_voxelize;

	if (!renderpass_voxelize.IsValid())
	{
		RenderPassDesc renderpassdesc;
		renderpassdesc._flags = RenderPassDesc::FLAG_ALLOW_UAV_WRITES;
		device->CreateRenderPass(&renderpassdesc, &renderpass_voxelize);
	}

	Texture* result = &textures[TEXTYPE_3D_VOXELRADIANCE];

	AABB bbox;
	XMFLOAT3 extents = voxelSceneData.extents;
	XMFLOAT3 center = voxelSceneData.center;
	bbox.createFromHalfWidth(center, extents);


	RenderQueue renderQueue;
	for (size_t i = 0; i < vis.scene->aabb_objects.GetCount(); ++i)
	{
		const AABB& aabb = vis.scene->aabb_objects[i];
		if (bbox.intersects(aabb))
		{
			const ObjectComponent& object = vis.scene->objects[i];
			if (object.IsRenderable())
			{
				RenderBatch* batch = (RenderBatch*)GetRenderFrameAllocator(cmd).allocate(sizeof(RenderBatch));
				size_t meshIndex = vis.scene->meshes.GetIndex(object.meshID);
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

		device->RenderPassBegin(&renderpass_voxelize, cmd);
		RenderMeshes(vis, renderQueue, RENDERPASS_VOXELIZE, RENDERTYPE_OPAQUE, cmd, false, nullptr, 1);
		device->RenderPassEnd(cmd);

		GetRenderFrameAllocator(cmd).free(sizeof(RenderBatch) * renderQueue.batchCount);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		if (voxelSceneData.secondaryBounceEnabled)
		{
			device->EventBegin("Voxel Radiance Secondary Bounce", cmd);
			device->UnbindUAVs(1, 1, cmd);
			// Pre-integrate the voxel texture by creating blurred mip levels:
			GenerateMipChain(textures[TEXTYPE_3D_VOXELRADIANCE], MIPGENFILTER_LINEAR, cmd);

			device->BindResource(CS, &textures[TEXTYPE_3D_VOXELRADIANCE], 0, cmd);
			device->BindResource(CS, &resourceBuffers[RBTYPE_VOXELSCENE], 1, cmd);
			device->BindUAV(CS, &textures[TEXTYPE_3D_VOXELRADIANCE_HELPER], 0, cmd);
			device->BindComputeShader(&shaders[CSTYPE_VOXELRADIANCESECONDARYBOUNCE], cmd);
			device->Dispatch(voxelSceneData.res / 8, voxelSceneData.res / 8, voxelSceneData.res / 8, cmd);
			device->EventEnd(cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->EventBegin("Voxel Scene Clear Normals", cmd);
			device->UnbindResources(1, 1, cmd);
			device->BindUAV(CS, &resourceBuffers[RBTYPE_VOXELSCENE], 0, cmd);
			device->BindComputeShader(&shaders[CSTYPE_VOXELCLEARONLYNORMAL], cmd);
			device->Dispatch((uint32_t)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 256), 1, 1, cmd);
			device->EventEnd(cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

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


void CreateTiledLightResources(TiledLightResources& res, XMUINT2 resolution)
{
	const XMUINT3 tileCount = wiRenderer::GetEntityCullingTileCount(resolution);

	{
		GPUBufferDesc bd;
		bd.StructureByteStride = sizeof(XMFLOAT4) * 4; // storing 4 planes for every tile
		bd.ByteWidth = bd.StructureByteStride * tileCount.x * tileCount.y;
		bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.Usage = USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
		device->CreateBuffer(&bd, nullptr, &res.tileFrustums);

		device->SetName(&res.tileFrustums, "tileFrustums");
	}
	{
		GPUBufferDesc bd;
		bd.StructureByteStride = sizeof(uint);
		bd.ByteWidth = tileCount.x * tileCount.y * bd.StructureByteStride * SHADER_ENTITY_TILE_BUCKET_COUNT;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		device->CreateBuffer(&bd, nullptr, &res.entityTiles_Opaque);
		device->CreateBuffer(&bd, nullptr, &res.entityTiles_Transparent);

		device->SetName(&res.entityTiles_Opaque, "entityTiles_Opaque");
		device->SetName(&res.entityTiles_Transparent, "entityTiles_Transparent");
	}
}
void ComputeTiledLightCulling(
	const TiledLightResources& res,
	const Texture& depthbuffer,
	const Texture& debugUAV,
	CommandList cmd
)
{
	const XMUINT3 tileCount = GetEntityCullingTileCount(XMUINT2(depthbuffer.desc.Width, depthbuffer.desc.Height));

	BindCommonResources(cmd);

	device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);

	// Frustum computation
	{
		device->EventBegin("Tile Frustums", cmd);
		device->BindComputeShader(&shaders[CSTYPE_TILEFRUSTUMS], cmd);

		const GPUResource* uavs[] = { 
			&res.tileFrustums 
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(&res.tileFrustums),
				GPUBarrier::Buffer(&res.tileFrustums, BUFFER_STATE_SHADER_RESOURCE, BUFFER_STATE_UNORDERED_ACCESS)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(tileCount.x + TILED_CULLING_BLOCKSIZE - 1) / TILED_CULLING_BLOCKSIZE,
			(tileCount.y + TILED_CULLING_BLOCKSIZE - 1) / TILED_CULLING_BLOCKSIZE,
			1,
			cmd
		);
		device->UnbindUAVs(0, arraysize(uavs), cmd);

		device->EventEnd(cmd);
	}

	// Perform the culling
	{
		device->EventBegin("Entity Culling", cmd);

		device->BindResource(CS, &res.tileFrustums, TEXSLOT_ONDEMAND0, cmd);

		if (GetDebugLightCulling() && debugUAV.IsValid())
		{
			device->BindComputeShader(&shaders[GetAdvancedLightCulling() ? CSTYPE_LIGHTCULLING_ADVANCED_DEBUG : CSTYPE_LIGHTCULLING_DEBUG], cmd);
			device->BindUAV(CS, &debugUAV, 3, cmd);
		}
		else
		{
			device->BindComputeShader(&shaders[GetAdvancedLightCulling() ? CSTYPE_LIGHTCULLING_ADVANCED : CSTYPE_LIGHTCULLING], cmd);
		}

		const GPUResource* uavs[] = {
			&res.entityTiles_Transparent,
			&res.entityTiles_Opaque,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(&res.tileFrustums),
				GPUBarrier::Buffer(&res.tileFrustums, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
				GPUBarrier::Buffer(&res.entityTiles_Transparent, BUFFER_STATE_SHADER_RESOURCE, BUFFER_STATE_UNORDERED_ACCESS),
				GPUBarrier::Buffer(&res.entityTiles_Opaque, BUFFER_STATE_SHADER_RESOURCE, BUFFER_STATE_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(tileCount.x, tileCount.y, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(&res.entityTiles_Opaque),
				GPUBarrier::Memory(&res.entityTiles_Transparent),
				GPUBarrier::Buffer(&res.entityTiles_Opaque, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
				GPUBarrier::Buffer(&res.entityTiles_Transparent, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);

		device->EventEnd(cmd);
	}
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
					{
						GPUBarrier barriers[] = {
							GPUBarrier::Image(&texture, texture.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, i + 1, options.arrayIndex * 6 + 0),
							GPUBarrier::Image(&texture, texture.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, i + 1, options.arrayIndex * 6 + 1),
							GPUBarrier::Image(&texture, texture.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, i + 1, options.arrayIndex * 6 + 2),
							GPUBarrier::Image(&texture, texture.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, i + 1, options.arrayIndex * 6 + 3),
							GPUBarrier::Image(&texture, texture.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, i + 1, options.arrayIndex * 6 + 4),
							GPUBarrier::Image(&texture, texture.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, i + 1, options.arrayIndex * 6 + 5),
						};
						device->Barrier(barriers, arraysize(barriers), cmd);
					}

					device->BindUAV(CS, &texture, 0, cmd, i + 1);
					device->BindResource(CS, &texture, TEXSLOT_ONDEMAND0, cmd, i);
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

					{
						GPUBarrier barriers[] = {
							GPUBarrier::Memory(),
							GPUBarrier::Image(&texture, IMAGE_LAYOUT_UNORDERED_ACCESS, texture.desc.layout, i + 1, options.arrayIndex * 6 + 0),
							GPUBarrier::Image(&texture, IMAGE_LAYOUT_UNORDERED_ACCESS, texture.desc.layout, i + 1, options.arrayIndex * 6 + 1),
							GPUBarrier::Image(&texture, IMAGE_LAYOUT_UNORDERED_ACCESS, texture.desc.layout, i + 1, options.arrayIndex * 6 + 2),
							GPUBarrier::Image(&texture, IMAGE_LAYOUT_UNORDERED_ACCESS, texture.desc.layout, i + 1, options.arrayIndex * 6 + 3),
							GPUBarrier::Image(&texture, IMAGE_LAYOUT_UNORDERED_ACCESS, texture.desc.layout, i + 1, options.arrayIndex * 6 + 4),
							GPUBarrier::Image(&texture, IMAGE_LAYOUT_UNORDERED_ACCESS, texture.desc.layout, i + 1, options.arrayIndex * 6 + 5),
						};
						device->Barrier(barriers, arraysize(barriers), cmd);
					}
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
					device->BindResource(CS, &texture, TEXSLOT_ONDEMAND0, cmd, i);
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
				{
					GPUBarrier barriers[] = {
						GPUBarrier::Image(&texture,texture.desc.layout,IMAGE_LAYOUT_UNORDERED_ACCESS,i + 1),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);
				}

				device->BindUAV(CS, &texture, 0, cmd, i + 1);
				device->BindResource(CS, &texture, TEXSLOT_ONDEMAND0, cmd, i);
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

				{
					GPUBarrier barriers[] = {
						GPUBarrier::Memory(),
						GPUBarrier::Image(&texture,IMAGE_LAYOUT_UNORDERED_ACCESS,texture.desc.layout,i + 1),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);
				}
			}
		}

		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
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
			device->BindResource(CS, &texture, TEXSLOT_ONDEMAND0, cmd, i);
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

		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
		device->UnbindUAVs(0, 1, cmd);

		device->EventEnd(cmd);
	}
	else
	{
		assert(0);
	}
}

void CopyTexture2D(const Texture& dst, int DstMIP, int DstX, int DstY, const Texture& src, int SrcMIP, CommandList cmd, BORDEREXPANDSTYLE borderExpand)
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

	device->BindUAV(CS, &dst, 0, cmd, DstMIP);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&dst,dst.desc.layout,IMAGE_LAYOUT_UNORDERED_ACCESS, DstMIP),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch((cb.xCopySrcSize.x + 7) / 8, (cb.xCopySrcSize.y + 7) / 8, 1, cmd);

	device->UnbindUAVs(0, 1, cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&dst,IMAGE_LAYOUT_UNORDERED_ACCESS,dst.desc.layout, DstMIP),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->EventEnd(cmd);
}


void RayTraceScene(
	const Scene& scene,
	const Texture& output,
	int accumulation_sample, 
	CommandList cmd,
	const Texture* output_albedo,
	const Texture* output_normal
)
{
	// Set up tracing resources:
	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
	{
		if (!scene.TLAS.IsValid())
		{
			return;
		}
		device->BindResource(CS, &scene.TLAS, TEXSLOT_ACCELERATION_STRUCTURE, cmd);
	}
	else
	{
		scene.BVH.Bind(CS, cmd);
	}

	device->EventBegin("RayTraceScene", cmd);
	auto range = wiProfiler::BeginRangeGPU("RayTraceScene", cmd);

	const TextureDesc& desc = output.GetDesc();

	if (scene.weather.skyMap != nullptr)
	{
		device->BindResource(CS, &scene.weather.skyMap->texture, TEXSLOT_GLOBALENVMAP, cmd);
	}
	else
	{
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);
	}

	const XMFLOAT4& halton = wiMath::GetHaltonSequence(accumulation_sample);
	RaytracingCB cb;
	cb.xTracePixelOffset = XMFLOAT2(halton.x, halton.y);
	cb.xTraceAccumulationFactor = 1.0f / ((float)accumulation_sample + 1.0f);
	cb.xTraceResolution.x = desc.Width;
	cb.xTraceResolution.y = desc.Height;
	cb.xTraceResolution_rcp.x = 1.0f / cb.xTraceResolution.x;
	cb.xTraceResolution_rcp.y = 1.0f / cb.xTraceResolution.y;
	cb.xTraceUserData.x = raytraceBounceCount;
	cb.xTraceSampleIndex = (uint32_t)accumulation_sample;
	device->UpdateBuffer(&constantBuffers[CBTYPE_RAYTRACE], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(RaytracingCB), cmd);

	device->BindComputeShader(&shaders[CSTYPE_RAYTRACE], cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	if (output_albedo != nullptr)
	{
		device->BindUAV(CS, output_albedo, 1, cmd);
	}
	if (output_normal != nullptr)
	{
		device->BindUAV(CS, output_normal, 2, cmd);
	}

	device->Dispatch(
		(desc.Width + RAYTRACING_LAUNCH_BLOCKSIZE - 1) / RAYTRACING_LAUNCH_BLOCKSIZE,
		(desc.Height + RAYTRACING_LAUNCH_BLOCKSIZE - 1) / RAYTRACING_LAUNCH_BLOCKSIZE,
		1,
		cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, 3, cmd);

	wiProfiler::EndRange(range);
	device->EventEnd(cmd); // RayTraceScene
}
void RayTraceSceneBVH(const Scene& scene, CommandList cmd)
{
	device->EventBegin("RayTraceSceneBVH", cmd);
	device->BindPipelineState(&PSO_debug[DEBUGRENDERING_RAYTRACE_BVH], cmd);
	scene.BVH.Bind(PS, cmd);
	device->Draw(3, 0, cmd);
	device->EventEnd(cmd);
}


void RefreshDecalAtlas(const Scene& scene, CommandList cmd)
{
	using namespace wiRectPacker;

	if (scene.decal_repack_needed)
	{
		for (uint32_t mip = 0; mip < scene.decalAtlas.GetDesc().MipLevels; ++mip)
		{
			for (auto& it : scene.packedDecals)
			{
				if (mip < it.first->texture.desc.MipLevels)
				{
					CopyTexture2D(scene.decalAtlas, mip, (it.second.x >> mip) + Scene::atlasClampBorder, (it.second.y >> mip) + Scene::atlasClampBorder, it.first->texture, mip, cmd, BORDEREXPAND_CLAMP);
				}
			}
		}
		scene.decal_repack_needed = false;
	}
}

void RenderObjectLightMap(const Scene& scene, const ObjectComponent& object, CommandList cmd)
{
	device->EventBegin("RenderObjectLightMap", cmd);

	const MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);
	assert(!mesh.vertex_atlas.empty());
	assert(mesh.vertexBuffer_ATL.IsValid());

	const TextureDesc& desc = object.lightmap.GetDesc();

	if (object.lightmapIterationCount == 0)
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

	MiscCB misccb;
	misccb.g_xTransform = transform.world;
	device->UpdateBuffer(&constantBuffers[CBTYPE_MISC], &misccb, cmd);
	device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), cmd);

	const GPUBuffer* vbs[] = {
		&mesh.vertexBuffer_POS,
		&mesh.vertexBuffer_ATL,
	};
	uint32_t strides[] = {
		sizeof(MeshComponent::Vertex_POS),
		sizeof(MeshComponent::Vertex_TEX),
	};
	uint32_t offsets[] = {
		0,
		0,
	};
	device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
	device->BindIndexBuffer(&mesh.indexBuffer, mesh.GetIndexFormat(), 0, cmd);

	RaytracingCB cb;
	cb.xTraceResolution.x = desc.Width;
	cb.xTraceResolution.y = desc.Height;
	cb.xTraceResolution_rcp.x = 1.0f / cb.xTraceResolution.x;
	cb.xTraceResolution_rcp.y = 1.0f / cb.xTraceResolution.y;
	XMFLOAT4 halton = wiMath::GetHaltonSequence(object.lightmapIterationCount); // for jittering the rasterization (good for eliminating atlas border artifacts)
	cb.xTracePixelOffset.x = (halton.x * 2 - 1) * cb.xTraceResolution_rcp.x;
	cb.xTracePixelOffset.y = (halton.y * 2 - 1) * cb.xTraceResolution_rcp.y;
	cb.xTracePixelOffset.x *= 1.4f;	// boost the jitter by a bit
	cb.xTracePixelOffset.y *= 1.4f;	// boost the jitter by a bit
	cb.xTraceAccumulationFactor = 1.0f / (object.lightmapIterationCount + 1.0f); // accumulation factor (alpha)
	cb.xTraceUserData.x = raytraceBounceCount;
	cb.xTraceSampleIndex = object.lightmapIterationCount;
	device->UpdateBuffer(&constantBuffers[CBTYPE_RAYTRACE], &cb, cmd);
	device->BindConstantBuffer(VS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(RaytracingCB), cmd);
	device->BindConstantBuffer(PS, &constantBuffers[CBTYPE_RAYTRACE], CB_GETBINDSLOT(RaytracingCB), cmd);

	device->BindPipelineState(&PSO_renderlightmap, cmd);

	if (scene.weather.skyMap != nullptr)
	{
		device->BindResource(PS, &scene.weather.skyMap->texture, TEXSLOT_GLOBALENVMAP, cmd);
	}
	else
	{
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
		device->BindResource(PS, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);
	}

	device->DrawIndexedInstanced((uint32_t)mesh.indices.size(), 1, 0, 0, 0, cmd);
	object.lightmapIterationCount++;

	device->RenderPassEnd(cmd);

	device->EventEnd(cmd);
}
void RefreshLightmapAtlas(const Scene& scene, CommandList cmd)
{
	if (scene.lightmap_refresh_needed.load() || scene.lightmap_repack_needed.load())
	{
		auto range = wiProfiler::BeginRangeGPU("Lightmap Processing", cmd);

		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
		{
			device->BindResource(PS, &scene.TLAS, TEXSLOT_ACCELERATION_STRUCTURE, cmd);
		}
		else
		{
			scene.BVH.Bind(PS, cmd);
		}

		// Render lightmaps for each object:
		for (uint32_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			const ObjectComponent& object = scene.objects[i];
			if (!object.lightmap.IsValid())
				continue;

			if (object.IsLightmapRenderRequested())
			{
				RenderObjectLightMap(scene, object, cmd);
			}

			if (object.IsLightmapRenderRequested() || scene.lightmap_repack_needed)
			{
				// copy object' lightmap into scene's lightmap atlas
				CopyTexture2D(scene.lightmap, -1, object.lightmap_rect.x + Scene::atlasClampBorder, object.lightmap_rect.y + Scene::atlasClampBorder, object.lightmap, 0, cmd);
			}
		}

		scene.lightmap_repack_needed.store(false);
		scene.lightmap_refresh_needed.store(false);
		wiProfiler::EndRange(range);
	}
}

void BindCommonResources(CommandList cmd)
{
	for (int i = 0; i < SHADERSTAGE_COUNT; ++i)
	{
		SHADERSTAGE stage = (SHADERSTAGE)i;

		device->BindSampler(stage, &samplers[SSLOT_OBJECTSHADER], SSLOT_OBJECTSHADER, cmd);

		BindConstantBuffers(stage, cmd);

		device->BindResource(stage, wiTextureHelper::getRandom64x64(), TEXSLOT_RANDOM64X64, cmd);
		device->BindResource(stage, wiTextureHelper::getBlueNoise(), TEXSLOT_BLUENOISE, cmd);
	}

	device->BindResource(PS, &textures[TEXTYPE_2D_SHEENLUT], TEXSLOT_SHEENLUT, cmd);
	device->BindResource(CS, &textures[TEXTYPE_2D_SHEENLUT], TEXSLOT_SHEENLUT, cmd);

	// Bind the GPU entity array for all shaders that need it here:
	GPUResource* resources[] = {
		&resourceBuffers[RBTYPE_ENTITYARRAY],
		&resourceBuffers[RBTYPE_MATRIXARRAY],
	};
	device->BindResources(VS, resources, SBSLOT_ENTITYARRAY, arraysize(resources), cmd);
	device->BindResources(PS, resources, SBSLOT_ENTITYARRAY, arraysize(resources), cmd);
	device->BindResources(CS, resources, SBSLOT_ENTITYARRAY, arraysize(resources), cmd);
}

void UpdateCameraCB(
	const CameraComponent& camera,
	const CameraComponent& camera_previous,
	const CameraComponent& camera_reflection,
	CommandList cmd
)
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
	cb.g_xCamera_ClipPlane = camera.clipPlane;

	static_assert(arraysize(camera.frustum.planes) == arraysize(cb.g_xCamera_FrustumPlanes), "Mismatch!");
	for (int i = 0; i < arraysize(camera.frustum.planes); ++i)
	{
		cb.g_xCamera_FrustumPlanes[i] = camera.frustum.planes[i];
	}

	cb.g_xFrame_TemporalAAJitter = camera.jitter;
	cb.g_xFrame_TemporalAAJitterPrev = camera_previous.jitter;

	XMStoreFloat4x4(&cb.g_xCamera_PrevV, camera_previous.GetView());
	XMStoreFloat4x4(&cb.g_xCamera_PrevP, camera_previous.GetProjection());
	XMStoreFloat4x4(&cb.g_xCamera_PrevVP, camera_previous.GetViewProjection());
	XMStoreFloat4x4(&cb.g_xCamera_PrevInvVP, camera_previous.GetInvViewProjection());
	XMStoreFloat4x4(&cb.g_xCamera_ReflVP, camera_reflection.GetViewProjection());
	XMStoreFloat4x4(&cb.g_xCamera_Reprojection, camera.GetInvViewProjection() * camera_previous.GetViewProjection());

	cb.g_xCamera_FocalLength = camera.focal_length;
	cb.g_xCamera_ApertureSize = camera.aperture_size;
	cb.g_xCamera_ApertureShape = camera.aperture_shape;

	device->UpdateBuffer(&constantBuffers[CBTYPE_CAMERA], &cb, cmd);
}

void CreateLuminanceResources(LuminanceResources& res, XMUINT2 resolution)
{
	TextureDesc desc;
	desc.Width = 32;
	desc.Height = desc.Width;
	desc.Format = FORMAT_R16_FLOAT;
	desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	device->CreateTexture(&desc, nullptr, &res.reductiontex);

	desc.Width = 1;
	desc.Height = desc.Width;
	device->CreateTexture(&desc, nullptr, &res.luminance);
}
const Texture* ComputeLuminance(
	const LuminanceResources& res,
	const Texture& sourceImage,
	CommandList cmd,
	float adaption_rate
)
{
	device->EventBegin("Compute Luminance", cmd);
	auto range = wiProfiler::BeginRangeGPU("Luminance", cmd);

	PostProcessCB cb;
	cb.luminance_adaptionrate = adaption_rate;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Pass 1 : Compute log luminance and reduction
	{
		device->BindComputeShader(&shaders[CSTYPE_LUMINANCE_PASS1], cmd);
		device->BindResource(CS, &sourceImage, TEXSLOT_ONDEMAND0, cmd);
		device->BindUAV(CS, &res.reductiontex, 0, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.reductiontex, res.reductiontex.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(32, 32, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.reductiontex, IMAGE_LAYOUT_UNORDERED_ACCESS, res.reductiontex.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}
	}

	// Pass 2 : Reduce into 1x1 texture
	{
		device->BindComputeShader(&shaders[CSTYPE_LUMINANCE_PASS2], cmd);

		device->BindUAV(CS, &res.luminance, 0, cmd);
		device->BindResource(CS, &res.reductiontex, TEXSLOT_ONDEMAND0, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.luminance, res.luminance.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(1, 1, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.luminance, IMAGE_LAYOUT_UNORDERED_ACCESS, res.luminance.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, 1, cmd);
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);

	return &res.luminance;
}

void ComputeShadingRateClassification(
	const Texture gbuffer[GBUFFER_COUNT],
	const Texture& lineardepth,
	const Texture& output,
	const Texture& debugUAV,
	CommandList cmd
)
{
	device->EventBegin("ComputeShadingRateClassification", cmd);
	auto range = wiProfiler::BeginRangeGPU("ComputeShadingRateClassification", cmd);

	if (GetVariableRateShadingClassificationDebug())
	{
		device->BindUAV(CS, &debugUAV, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&debugUAV, debugUAV.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->BindComputeShader(&shaders[CSTYPE_SHADINGRATECLASSIFICATION_DEBUG], cmd);
	}
	else
	{
		device->BindComputeShader(&shaders[CSTYPE_SHADINGRATECLASSIFICATION], cmd);
	}

	device->BindResource(CS, &gbuffer[GBUFFER_VELOCITY], TEXSLOT_GBUFFER2, cmd);
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

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	// Whole threadgroup for each tile:
	device->Dispatch(desc.Width, desc.Height, 1, cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	if (GetVariableRateShadingClassificationDebug())
	{
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&debugUAV, IMAGE_LAYOUT_UNORDERED_ACCESS, debugUAV.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}
	}

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	wiProfiler::EndRange(range);
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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&temp, temp.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, mip_dst),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(cb.xPPResolution.x + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT - 1) / POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT,
			cb.xPPResolution.y,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&temp, IMAGE_LAYOUT_UNORDERED_ACCESS, temp.desc.layout, mip_dst),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, mip_dst),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			cb.xPPResolution.x,
			(cb.xPPResolution.y + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT - 1) / POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout, mip_dst),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&temp, temp.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, mip_dst),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(cb.xPPResolution.x + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT - 1) / POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT,
			cb.xPPResolution.y,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&temp, IMAGE_LAYOUT_UNORDERED_ACCESS, temp.desc.layout, mip_dst),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, mip_dst),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			cb.xPPResolution.x,
			(cb.xPPResolution.y + POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT - 1) / POSTPROCESS_BLUR_GAUSSIAN_THREADCOUNT,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout, mip_dst),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, 1, cmd);
	}

	device->EventEnd(cmd);
}
void CreateSSAOResources(SSAOResources& res, XMUINT2 resolution)
{
	TextureDesc desc;
	desc.Format = FORMAT_R8_UNORM;
	desc.Width = resolution.x / 2;
	desc.Height = resolution.y / 2;
	desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE_COMPUTE;
	device->CreateTexture(&desc, nullptr, &res.temp);
}
void Postprocess_SSAO(
	const SSAOResources& res,
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

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSAO], cmd);

	device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	const TextureDesc& desc = output.GetDesc();

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
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	Postprocess_Blur_Bilateral(output, lineardepth, res.temp, output, cmd, 1.2f, -1, -1, true);

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void Postprocess_HBAO(
	const SSAOResources& res,
	const CameraComponent& camera,
	const Texture& lineardepth,
	const Texture& output,
	CommandList cmd,
	float power
)
{
	device->EventBegin("Postprocess_HBAO", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("HBAO", cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_HBAO], cmd);

	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = 1;
	cb.xPPParams0.y = 0;
	cb.hbao_power = power;

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
		device->BindResource(CS, wiTextureHelper::getWhite(), TEXSLOT_ONDEMAND0, cmd);
		const GPUResource* uavs[] = {
			&res.temp,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.temp, res.temp.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(cb.xPPResolution.x + POSTPROCESS_HBAO_THREADCOUNT - 1) / POSTPROCESS_HBAO_THREADCOUNT,
			cb.xPPResolution.y,
			1,
			cmd
			);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.temp, IMAGE_LAYOUT_UNORDERED_ACCESS, res.temp.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}

	// vertical pass:
	{
		cb.xPPParams0.x = 0;
		cb.xPPParams0.y = 1;
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

		device->BindResource(CS, &res.temp, TEXSLOT_ONDEMAND0, cmd);
		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			cb.xPPResolution.x,
			(cb.xPPResolution.y + POSTPROCESS_HBAO_THREADCOUNT - 1) / POSTPROCESS_HBAO_THREADCOUNT,
			1,
			cmd
			);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND1, 1, cmd);
	}

	Postprocess_Blur_Bilateral(output, lineardepth, res.temp, output, cmd, 1.2f, -1, -1, true);

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void CreateMSAOResources(MSAOResources& res, XMUINT2 resolution)
{
	TextureDesc saved_desc;
	saved_desc.Format = FORMAT_R32_FLOAT;
	saved_desc.Width = resolution.x;
	saved_desc.Height = resolution.y;
	saved_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	saved_desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE_COMPUTE;

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
	device->CreateTexture(&desc, nullptr, &res.texture_lineardepth_downsize1);
	desc.Width = bufferWidth3;
	desc.Height = bufferHeight3;
	desc.ArraySize = 16;
	desc.Format = FORMAT_R16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.texture_lineardepth_tiled1);

	desc = saved_desc;
	desc.Width = bufferWidth2;
	desc.Height = bufferHeight2;
	device->CreateTexture(&desc, nullptr, &res.texture_lineardepth_downsize2);
	desc.Width = bufferWidth4;
	desc.Height = bufferHeight4;
	desc.ArraySize = 16;
	desc.Format = FORMAT_R16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.texture_lineardepth_tiled2);

	desc = saved_desc;
	desc.Width = bufferWidth3;
	desc.Height = bufferHeight3;
	device->CreateTexture(&desc, nullptr, &res.texture_lineardepth_downsize3);
	desc.Width = bufferWidth5;
	desc.Height = bufferHeight5;
	desc.ArraySize = 16;
	desc.Format = FORMAT_R16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.texture_lineardepth_tiled3);

	desc = saved_desc;
	desc.Width = bufferWidth4;
	desc.Height = bufferHeight4;
	device->CreateTexture(&desc, nullptr, &res.texture_lineardepth_downsize4);
	desc.Width = bufferWidth6;
	desc.Height = bufferHeight6;
	desc.ArraySize = 16;
	desc.Format = FORMAT_R16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.texture_lineardepth_tiled4);

	desc = saved_desc;
	desc.Format = FORMAT_R8_UNORM;
	desc.Width = bufferWidth1;
	desc.Height = bufferHeight1;
	device->CreateTexture(&desc, nullptr, &res.texture_ao_merged1);
	device->CreateTexture(&desc, nullptr, &res.texture_ao_hq1);
	device->CreateTexture(&desc, nullptr, &res.texture_ao_smooth1);
	desc.Width = bufferWidth2;
	desc.Height = bufferHeight2;
	device->CreateTexture(&desc, nullptr, &res.texture_ao_merged2);
	device->CreateTexture(&desc, nullptr, &res.texture_ao_hq2);
	device->CreateTexture(&desc, nullptr, &res.texture_ao_smooth2);
	desc.Width = bufferWidth3;
	desc.Height = bufferHeight3;
	device->CreateTexture(&desc, nullptr, &res.texture_ao_merged3);
	device->CreateTexture(&desc, nullptr, &res.texture_ao_hq3);
	device->CreateTexture(&desc, nullptr, &res.texture_ao_smooth3);
	desc.Width = bufferWidth4;
	desc.Height = bufferHeight4;
	device->CreateTexture(&desc, nullptr, &res.texture_ao_merged4);
	device->CreateTexture(&desc, nullptr, &res.texture_ao_hq4);
}
void Postprocess_MSAO(
	const MSAOResources& res,
	const CameraComponent& camera,
	const Texture& lineardepth,
	const Texture& output,
	CommandList cmd,
	float power
	)
{
	device->EventBegin("Postprocess_MSAO", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("MSAO", cmd);

	// Depth downsampling + deinterleaving pass1:
	{
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MSAO_PREPAREDEPTHBUFFERS1], cmd);

		device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

		const GPUResource* uavs[] = {
			&res.texture_lineardepth_downsize1,
			&res.texture_lineardepth_tiled1,
			&res.texture_lineardepth_downsize2,
			&res.texture_lineardepth_tiled2,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_lineardepth_downsize1, res.texture_lineardepth_downsize1.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_lineardepth_tiled1, res.texture_lineardepth_tiled1.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_lineardepth_downsize2, res.texture_lineardepth_downsize2.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_lineardepth_tiled2, res.texture_lineardepth_tiled2.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		const TextureDesc& desc = res.texture_lineardepth_tiled2.GetDesc();
		device->Dispatch(desc.Width, desc.Height, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_lineardepth_downsize1, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_lineardepth_downsize1.desc.layout),
				GPUBarrier::Image(&res.texture_lineardepth_tiled1, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_lineardepth_tiled1.desc.layout),
				GPUBarrier::Image(&res.texture_lineardepth_downsize2, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_lineardepth_downsize2.desc.layout),
				GPUBarrier::Image(&res.texture_lineardepth_tiled2, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_lineardepth_tiled2.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}

	// Depth downsampling + deinterleaving pass2:
	{
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MSAO_PREPAREDEPTHBUFFERS2], cmd);

		device->BindResource(CS, &res.texture_lineardepth_downsize2, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&res.texture_lineardepth_downsize3,
			&res.texture_lineardepth_tiled3,
			&res.texture_lineardepth_downsize4,
			&res.texture_lineardepth_tiled4,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_lineardepth_downsize3, res.texture_lineardepth_downsize3.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_lineardepth_tiled3, res.texture_lineardepth_tiled3.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_lineardepth_downsize4, res.texture_lineardepth_downsize4.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_lineardepth_tiled4, res.texture_lineardepth_tiled4.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		const TextureDesc& desc = res.texture_lineardepth_tiled4.GetDesc();
		device->Dispatch(desc.Width, desc.Height, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_lineardepth_downsize3, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_lineardepth_downsize3.desc.layout),
				GPUBarrier::Image(&res.texture_lineardepth_tiled3, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_lineardepth_tiled3.desc.layout),
				GPUBarrier::Image(&res.texture_lineardepth_downsize4, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_lineardepth_downsize4.desc.layout),
				GPUBarrier::Image(&res.texture_lineardepth_tiled4, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_lineardepth_tiled4.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&write_result, write_result.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&write_result, IMAGE_LAYOUT_UNORDERED_ACCESS, write_result.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}; // end of lambda: msao_compute

	msao_compute(res.texture_ao_merged4, res.texture_lineardepth_tiled4);
	msao_compute(res.texture_ao_hq4, res.texture_lineardepth_downsize4);

	msao_compute(res.texture_ao_merged3, res.texture_lineardepth_tiled3);
	msao_compute(res.texture_ao_hq3, res.texture_lineardepth_downsize3);

	msao_compute(res.texture_ao_merged2, res.texture_lineardepth_tiled2);
	msao_compute(res.texture_ao_hq2, res.texture_lineardepth_downsize2);

	msao_compute(res.texture_ao_merged1, res.texture_lineardepth_tiled1);
	msao_compute(res.texture_ao_hq1, res.texture_lineardepth_downsize1);

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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&Destination, Destination.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch((HiWidth + 2 + 15) / 16, (HiHeight + 2 + 15) / 16, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&Destination, IMAGE_LAYOUT_UNORDERED_ACCESS, Destination.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}
	};

	blur_and_upsample(
		res.texture_ao_smooth3,
		res.texture_lineardepth_downsize3,
		res.texture_lineardepth_downsize4,
		&res.texture_ao_merged4,
		&res.texture_ao_hq4,
		&res.texture_ao_merged3
	);

	blur_and_upsample(
		res.texture_ao_smooth2,
		res.texture_lineardepth_downsize2,
		res.texture_lineardepth_downsize3,
		&res.texture_ao_smooth3,
		&res.texture_ao_hq3,
		&res.texture_ao_merged2
	);

	blur_and_upsample(
		res.texture_ao_smooth1,
		res.texture_lineardepth_downsize1,
		res.texture_lineardepth_downsize2,
		&res.texture_ao_smooth2,
		&res.texture_ao_hq2,
		&res.texture_ao_merged1
	);

	blur_and_upsample(
		output,
		lineardepth,
		res.texture_lineardepth_downsize1,
		&res.texture_ao_smooth1,
		&res.texture_ao_hq1,
		nullptr
	);

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void CreateRTAOResources(RTAOResources& res, XMUINT2 resolution)
{
	TextureDesc desc;
	desc.Width = resolution.x / 2;
	desc.Height = resolution.y / 2;
	desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE_COMPUTE;

	desc.Format = FORMAT_R11G11B10_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.normals);
	device->SetName(&res.normals, "rtao_normals");

	GPUBufferDesc bd;
	bd.StructureByteStride = sizeof(uint);
	bd.ByteWidth = bd.StructureByteStride *
		((desc.Width + 7) / 8) *
		((desc.Height + 3) / 4);
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	device->CreateBuffer(&bd, nullptr, &res.tiles);
	device->SetName(&res.tiles, "rtshadow_tiles");
	device->CreateBuffer(&bd, nullptr, &res.metadata);
	device->SetName(&res.metadata, "rtshadow_metadata");

	desc.Format = FORMAT_R16G16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.scratch[0]);
	device->SetName(&res.scratch[0], "rtshadow_scratch[0]");
	device->CreateTexture(&desc, nullptr, &res.scratch[1]);
	device->SetName(&res.scratch[1], "rtshadow_scratch[1]");

	desc.Format = FORMAT_R11G11B10_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.moments[0]);
	device->SetName(&res.moments[0], "rtshadow_moments[0]");
	device->CreateTexture(&desc, nullptr, &res.moments[1]);
	device->SetName(&res.moments[1], "rtshadow_moments[1]");
}
void Postprocess_RTAO(
	const RTAOResources& res,
	const Scene& scene,
	const Texture& depthbuffer,
	const Texture& lineardepth,
	const Texture& depth_history,
	const Texture gbuffer[GBUFFER_COUNT],
	const Texture& output,
	CommandList cmd,
	float range,
	float power
)
{
	if (!wiRenderer::device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
		return;

	if (scene.objects.GetCount() <= 0)
	{
		return;
	}

	device->EventBegin("Postprocess_RTAO", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("RTAO", cmd);

	BindCommonResources(cmd);

	device->EventBegin("Raytrace", cmd);

	const TextureDesc& desc = output.GetDesc();

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_RTAO], cmd);

	device->BindResource(CS, &scene.TLAS, TEXSLOT_ACCELERATION_STRUCTURE, cmd);
	device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	device->BindResource(CS, &gbuffer[GBUFFER_COLOR], TEXSLOT_GBUFFER0, cmd);
	device->BindResource(CS, &gbuffer[GBUFFER_NORMAL_ROUGHNESS], TEXSLOT_GBUFFER1, cmd);
	device->BindResource(CS, &gbuffer[GBUFFER_VELOCITY], TEXSLOT_GBUFFER2, cmd);

	const GPUResource* uavs[] = {
		&output,
		&res.normals,
		&res.tiles
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.rtao_range = range;
	cb.rtao_power = power;
	cb.xPPParams0.w = (float)res.frame;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			GPUBarrier::Image(&res.normals, res.normals.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			GPUBarrier::Buffer(&res.tiles, BUFFER_STATE_SHADER_RESOURCE_COMPUTE, BUFFER_STATE_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&res.normals, IMAGE_LAYOUT_UNORDERED_ACCESS, res.normals.desc.layout),
			GPUBarrier::Buffer(&res.tiles, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE_COMPUTE),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->EventEnd(cmd);

	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	int temporal_output = res.frame % 2;
	int temporal_history = 1 - temporal_output;

	// Denoise - Tile Classification:
	{
		device->EventBegin("Denoise - Tile Classification", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_RTAO_DENOISE_TILECLASSIFICATION], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &res.normals, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &res.tiles, TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &res.moments[temporal_history], TEXSLOT_ONDEMAND2, cmd);
		device->BindResource(CS, &res.scratch[1], TEXSLOT_ONDEMAND3, cmd);
		device->BindResource(CS, &depth_history, TEXSLOT_ONDEMAND4, cmd);

		const GPUResource* uavs[] = {
			&res.scratch[0],
			&res.moments[temporal_output],
			&res.metadata
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.scratch[0], res.scratch[0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.moments[temporal_output], res.moments[temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Buffer(&res.metadata, BUFFER_STATE_SHADER_RESOURCE_COMPUTE, BUFFER_STATE_UNORDERED_ACCESS)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.scratch[0], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[0].desc.layout),
				GPUBarrier::Image(&res.moments[temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.moments[temporal_output].desc.layout),
				GPUBarrier::Buffer(&res.metadata, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE_COMPUTE)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->EventEnd(cmd);
	}

	// Denoise - Spatial filtering:
	{
		device->EventBegin("Denoise - Filter", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_RTAO_DENOISE_FILTER], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &res.normals, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &res.metadata, TEXSLOT_ONDEMAND1, cmd);

		// pass0:
		{
			device->BindResource(CS, &res.scratch[0], TEXSLOT_ONDEMAND2, cmd);
			const GPUResource* uavs[] = {
				&res.scratch[1],
				&output
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&res.scratch[1], res.scratch[1].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			cb.xPPParams1.x = 0;
			cb.xPPParams1.y = 1;
			device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
			device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

			device->Dispatch(
				(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				1,
				cmd
			);
		}

		// pass1:
		{
			device->BindResource(CS, &res.scratch[1], TEXSLOT_ONDEMAND2, cmd);
			const GPUResource* uavs[] = {
				&res.scratch[0],
				&output
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
					GPUBarrier::Image(&res.scratch[1], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[1].desc.layout),
					GPUBarrier::Image(&res.scratch[0], res.scratch[0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			cb.xPPParams1.x = 1;
			cb.xPPParams1.y = 2;
			device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
			device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

			device->Dispatch(
				(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				1,
				cmd
			);
		}

		// pass2:
		{
			device->BindResource(CS, &res.scratch[0], TEXSLOT_ONDEMAND2, cmd);
			const GPUResource* uavs[] = {
				&res.scratch[1],
				&output
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
					GPUBarrier::Image(&res.scratch[0], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[0].desc.layout),
					GPUBarrier::Image(&res.scratch[1], res.scratch[0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			cb.xPPParams1.x = 2;
			cb.xPPParams1.y = 4;
			device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
			device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

			device->Dispatch(
				(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				1,
				cmd
			);
		}

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.scratch[1], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[1].desc.layout),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->EventEnd(cmd);
	}
	res.frame++;

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void CreateRTReflectionResources(RTReflectionResources& res, XMUINT2 resolution)
{
	TextureDesc desc;
	desc.Width = resolution.x / 2;
	desc.Height = resolution.y / 2;
	desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE_COMPUTE;

	desc.Format = FORMAT_R11G11B10_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.temporal[0]);
	device->SetName(&res.temporal[0], "rtreflection_temporal[0]");
	device->CreateTexture(&desc, nullptr, &res.temporal[1]);
	device->SetName(&res.temporal[1], "rtreflection_temporal[1]");

	desc.Format = FORMAT_R16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.rayLengths);
	device->SetName(&res.rayLengths, "rtreflection_rayLengths");
}
void Postprocess_RTReflection(
	const RTReflectionResources& res,
	const Scene& scene,
	const Texture& depthbuffer,
	const Texture& depth_history,
	const Texture gbuffer[GBUFFER_COUNT],
	const Texture& output,
	CommandList cmd,
	float range
)
{
	if (!wiRenderer::device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_PIPELINE))
		return;
	if (!wiRenderer::device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_GEOMETRYINDEX))
		return;

	if (scene.objects.GetCount() <= 0)
	{
		return;
	}

	device->EventBegin("Postprocess_RTReflection", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("RTReflection", cmd);

	const TextureDesc& desc = output.desc;

	device->BindRaytracingPipelineState(&RTPSO_reflection, cmd);

	BindCommonResources(cmd);
	BindShadowmaps(LIB, cmd);

	device->BindResource(LIB, &scene.TLAS, TEXSLOT_ACCELERATION_STRUCTURE, cmd);
	device->BindResource(LIB, &depthbuffer, TEXSLOT_DEPTH, cmd);
	device->BindResource(LIB, &gbuffer[GBUFFER_NORMAL_ROUGHNESS], TEXSLOT_GBUFFER1, cmd);
	device->BindResource(LIB, &gbuffer[GBUFFER_VELOCITY], TEXSLOT_GBUFFER2, cmd);

	device->BindResource(LIB, &depth_history, TEXSLOT_ONDEMAND2, cmd);

	device->BindResource(LIB, &resourceBuffers[RBTYPE_ENTITYARRAY], SBSLOT_ENTITYARRAY, cmd);
	device->BindResource(LIB, &resourceBuffers[RBTYPE_MATRIXARRAY], SBSLOT_MATRIXARRAY, cmd);
	device->BindResource(LIB, GetVoxelRadianceSecondaryBounceEnabled() ? &textures[TEXTYPE_3D_VOXELRADIANCE_HELPER] : &textures[TEXTYPE_3D_VOXELRADIANCE], TEXSLOT_VOXELRADIANCE, cmd);
	device->BindResource(LIB, &scene.envmapArray, TEXSLOT_ENVMAPARRAY, cmd);
	device->BindResource(LIB, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
	device->BindResource(LIB, &textures[TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], TEXSLOT_MULTISCATTERINGLUT, cmd);
	device->BindResource(LIB, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT], TEXSLOT_SKYVIEWLUT, cmd);
	device->BindResource(LIB, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT], TEXSLOT_SKYLUMINANCELUT, cmd);

	device->BindResource(LIB, &scene.lightmap, TEXSLOT_GLOBALLIGHTMAP, cmd);
	if (scene.decalAtlas.IsValid())
	{
		device->BindResource(LIB, &scene.decalAtlas, TEXSLOT_DECALATLAS, cmd);
	}

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.rtreflection_range = range;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(LIB, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	size_t shaderIdentifierSize = device->GetShaderIdentifierSize();
	GraphicsDevice::GPUAllocation shadertable_raygen = device->AllocateGPU(shaderIdentifierSize, cmd);
	GraphicsDevice::GPUAllocation shadertable_miss = device->AllocateGPU(shaderIdentifierSize, cmd);
	GraphicsDevice::GPUAllocation shadertable_hitgroup = device->AllocateGPU(shaderIdentifierSize, cmd);

	device->WriteShaderIdentifier(&RTPSO_reflection, 0, shadertable_raygen.data);
	device->WriteShaderIdentifier(&RTPSO_reflection, 1, shadertable_miss.data);
	device->WriteShaderIdentifier(&RTPSO_reflection, 2, shadertable_hitgroup.data);

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

	const GPUResource* uavs[] = {
		&output,
		&res.rayLengths
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			GPUBarrier::Image(&res.rayLengths, res.rayLengths.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->DispatchRays(&dispatchraysdesc, cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			GPUBarrier::Image(&res.rayLengths, IMAGE_LAYOUT_UNORDERED_ACCESS, res.rayLengths.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->BindResource(CS, &gbuffer[GBUFFER_COLOR], TEXSLOT_GBUFFER0, cmd);
	device->BindResource(CS, &gbuffer[GBUFFER_NORMAL_ROUGHNESS], TEXSLOT_GBUFFER1, cmd);
	device->BindResource(CS, &gbuffer[GBUFFER_VELOCITY], TEXSLOT_GBUFFER2, cmd);

	int temporal_output = device->GetFrameCount() % 2;
	int temporal_history = 1 - temporal_output;

	// Temporal pass:
	{
		device->EventBegin("Temporal pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_TEMPORAL], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &output, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &res.temporal[temporal_history], TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &depth_history, TEXSLOT_ONDEMAND2, cmd);
		device->BindResource(CS, &res.rayLengths, TEXSLOT_ONDEMAND3, cmd);

		const GPUResource* uavs[] = {
			&res.temporal[temporal_output],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.temporal[temporal_output], res.temporal[temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.temporal[temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.temporal[temporal_output].desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Median blur pass:
	{
		device->EventBegin("Median blur pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_MEDIAN], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &res.temporal[temporal_output], TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(output.desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(output.desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void CreateSSRResources(SSRResources& res, XMUINT2 resolution)
{
	TextureDesc desc;
	desc.type = TextureDesc::TEXTURE_2D;
	desc.Width = resolution.x / 2;
	desc.Height = resolution.y / 2;
	desc.Format = FORMAT_R16G16B16A16_FLOAT;
	desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE_COMPUTE;
	device->CreateTexture(&desc, nullptr, &res.texture_raytrace);
	device->CreateTexture(&desc, nullptr, &res.texture_temporal[0]);
	device->CreateTexture(&desc, nullptr, &res.texture_temporal[1]);

	desc.Format = FORMAT_R16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.rayLengths);
	device->SetName(&res.rayLengths, "ssr_rayLengths");
}
void Postprocess_SSR(
	const SSRResources& res,
	const Texture& input,
	const Texture& depthbuffer,
	const Texture& lineardepth,
	const Texture& depth_history,
	const Texture gbuffer[GBUFFER_COUNT],
	const Texture& output,
	CommandList cmd
)
{
	device->EventBegin("Postprocess_SSR", cmd);
	auto range = wiProfiler::BeginRangeGPU("SSR", cmd);

	device->UnbindResources(TEXSLOT_RENDERPATH_SSR, 1, cmd);

	BindCommonResources(cmd);

	const TextureDesc& input_desc = input.GetDesc();
	const TextureDesc& desc = output.GetDesc();

	device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);
	device->BindResource(CS, &gbuffer[GBUFFER_COLOR], TEXSLOT_GBUFFER0, cmd);
	device->BindResource(CS, &gbuffer[GBUFFER_NORMAL_ROUGHNESS], TEXSLOT_GBUFFER1, cmd);
	device->BindResource(CS, &gbuffer[GBUFFER_VELOCITY], TEXSLOT_GBUFFER2, cmd);

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
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

		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&res.texture_raytrace,
			&res.rayLengths
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_raytrace, res.texture_raytrace.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_raytrace, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_raytrace.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Resolve pass:
	{
		device->EventBegin("Resolve pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_RESOLVE], cmd);

		device->BindResource(CS, &res.texture_raytrace, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &input, TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &depth_history, TEXSLOT_ONDEMAND2, cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	int temporal_output = device->GetFrameCount() % 2;
	int temporal_history = 1 - temporal_output;

	// Temporal pass:
	{
		device->EventBegin("Temporal pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_TEMPORAL], cmd);

		device->BindResource(CS, &output, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &res.texture_temporal[temporal_history], TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &depth_history, TEXSLOT_ONDEMAND2, cmd);
		device->BindResource(CS, &res.rayLengths, TEXSLOT_ONDEMAND3, cmd);

		const GPUResource* uavs[] = {
			&res.texture_temporal[temporal_output],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_temporal[temporal_output], res.texture_temporal[temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_temporal[temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_temporal[temporal_output].desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Median blur pass:
	{
		device->EventBegin("Median blur pass", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SSR_MEDIAN], cmd);

		device->BindResource(CS, &res.texture_temporal[temporal_output], TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void CreateRTShadowResources(RTShadowResources& res, XMUINT2 resolution)
{
	TextureDesc desc;
	desc.Width = resolution.x / 2;
	desc.Height = resolution.y / 2;
	desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE_COMPUTE;

	desc.Format = FORMAT_R32G32B32A32_UINT;
	device->CreateTexture(&desc, nullptr, &res.temp);
	device->SetName(&res.temp, "rtshadow_temp");
	device->CreateTexture(&desc, nullptr, &res.temporal[0]);
	device->SetName(&res.temporal[0], "rtshadow_temporal[0]");
	device->CreateTexture(&desc, nullptr, &res.temporal[1]);
	device->SetName(&res.temporal[1], "rtshadow_temporal[1]");

	desc.Format = FORMAT_R11G11B10_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.normals);
	device->SetName(&res.normals, "rtshadow_normals");

	GPUBufferDesc bd;
	bd.StructureByteStride = sizeof(uint4);
	bd.ByteWidth = bd.StructureByteStride *
		((desc.Width + 7) / 8) *
		((desc.Height + 3) / 4);
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	device->CreateBuffer(&bd, nullptr, &res.tiles);
	device->SetName(&res.tiles, "rtshadow_tiles");
	device->CreateBuffer(&bd, nullptr, &res.metadata);
	device->SetName(&res.metadata, "rtshadow_metadata");

	for (int i = 0; i < 4; ++i)
	{
		desc.Format = FORMAT_R16G16_FLOAT;
		device->CreateTexture(&desc, nullptr, &res.scratch[i][0]);
		device->SetName(&res.scratch[i][0], "rtshadow_scratch[i][0]");
		device->CreateTexture(&desc, nullptr, &res.scratch[i][1]);
		device->SetName(&res.scratch[i][1], "rtshadow_scratch[i][1]");

		desc.Format = FORMAT_R11G11B10_FLOAT;
		device->CreateTexture(&desc, nullptr, &res.moments[i][0]);
		device->SetName(&res.moments[i][0], "rtshadow_moments[i][0]");
		device->CreateTexture(&desc, nullptr, &res.moments[i][1]);
		device->SetName(&res.moments[i][1], "rtshadow_moments[i][1]");
	}

	desc.Format = FORMAT_R8G8B8A8_UNORM;
	device->CreateTexture(&desc, nullptr, &res.denoised);
	device->SetName(&res.denoised, "rtshadow_denoised");
}
void Postprocess_RTShadow(
	const RTShadowResources& res,
	const Scene& scene,
	const Texture& depthbuffer,
	const Texture& lineardepth,
	const Texture& depth_history,
	const GPUBuffer& entityTiles_Opaque,
	const Texture gbuffer[GBUFFER_COUNT],
	const Texture& output,
	CommandList cmd
)
{
	if (!wiRenderer::device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
		return;

	if (scene.objects.GetCount() <= 0)
	{
		return;
	}

	device->EventBegin("Postprocess_RTShadow", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("RTShadow", cmd);

	const TextureDesc& desc = res.temp.GetDesc();

	BindCommonResources(cmd);

	device->EventBegin("Raytrace", cmd);

	device->BindResource(CS, &scene.TLAS, TEXSLOT_ACCELERATION_STRUCTURE, cmd);

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.w = (float)res.frame;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_RTSHADOW], cmd);

	device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	device->BindResource(CS, &entityTiles_Opaque, TEXSLOT_RENDERPATH_ENTITYTILES, cmd);

	device->BindResource(CS, &gbuffer[GBUFFER_COLOR], TEXSLOT_GBUFFER0, cmd);
	device->BindResource(CS, &gbuffer[GBUFFER_NORMAL_ROUGHNESS], TEXSLOT_GBUFFER1, cmd);
	device->BindResource(CS, &gbuffer[GBUFFER_VELOCITY], TEXSLOT_GBUFFER2, cmd);

	const GPUResource* uavs[] = {
		&res.temp,
		&res.normals,
		&res.tiles
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&res.temp, res.temp.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			GPUBarrier::Image(&res.normals, res.normals.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			GPUBarrier::Buffer(&res.tiles, BUFFER_STATE_SHADER_RESOURCE_COMPUTE, BUFFER_STATE_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&res.normals, IMAGE_LAYOUT_UNORDERED_ACCESS, res.normals.desc.layout),
			GPUBarrier::Buffer(&res.tiles, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE_COMPUTE),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, arraysize(uavs), cmd);
	device->EventEnd(cmd);

	int temporal_output = res.frame % 2;
	int temporal_history = 1 - temporal_output;

	// Denoise - Tile Classification:
	{
		device->EventBegin("Denoise - Tile Classification", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_TILECLASSIFICATION], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &res.normals, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &depth_history, TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &res.tiles, TEXSLOT_ONDEMAND2, cmd);
		device->BindResource(CS, &res.moments[0][temporal_history], TEXSLOT_ONDEMAND3, cmd);
		device->BindResource(CS, &res.moments[1][temporal_history], TEXSLOT_ONDEMAND4, cmd);
		device->BindResource(CS, &res.moments[2][temporal_history], TEXSLOT_ONDEMAND5, cmd);
		device->BindResource(CS, &res.moments[3][temporal_history], TEXSLOT_ONDEMAND6, cmd);
		device->BindResource(CS, &res.scratch[0][1], TEXSLOT_ONDEMAND7, cmd);
		device->BindResource(CS, &res.scratch[1][1], TEXSLOT_ONDEMAND8, cmd);
		device->BindResource(CS, &res.scratch[2][1], TEXSLOT_ONDEMAND9, cmd);
		device->BindResource(CS, &res.scratch[3][1], TEXSLOT_ONDEMAND10, cmd);

		const GPUResource* uavs[] = {
			&res.metadata,
			&res.scratch[0][0],
			&res.scratch[1][0],
			&res.scratch[2][0],
			&res.scratch[3][0],
			&res.moments[0][temporal_output],
			&res.moments[1][temporal_output],
			&res.moments[2][temporal_output],
			&res.moments[3][temporal_output],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&res.metadata, BUFFER_STATE_SHADER_RESOURCE_COMPUTE, BUFFER_STATE_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.scratch[0][0], res.scratch[0][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.scratch[1][0], res.scratch[1][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.scratch[2][0], res.scratch[2][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.scratch[3][0], res.scratch[3][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.moments[0][temporal_output], res.moments[0][temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.moments[1][temporal_output], res.moments[1][temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.moments[2][temporal_output], res.moments[2][temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.moments[3][temporal_output], res.moments[3][temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			4, // 4 lights
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Buffer(&res.metadata, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE_COMPUTE),
				GPUBarrier::Image(&res.scratch[0][0], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[0][0].desc.layout),
				GPUBarrier::Image(&res.scratch[1][0], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[1][0].desc.layout),
				GPUBarrier::Image(&res.scratch[2][0], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[2][0].desc.layout),
				GPUBarrier::Image(&res.scratch[3][0], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[3][0].desc.layout),
				GPUBarrier::Image(&res.moments[0][temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.moments[0][temporal_output].desc.layout),
				GPUBarrier::Image(&res.moments[1][temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.moments[1][temporal_output].desc.layout),
				GPUBarrier::Image(&res.moments[2][temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.moments[2][temporal_output].desc.layout),
				GPUBarrier::Image(&res.moments[3][temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.moments[3][temporal_output].desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Denoise - Spatial filtering:
	{
		device->EventBegin("Denoise - Filter", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_FILTER], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &res.normals, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &res.metadata, TEXSLOT_ONDEMAND1, cmd);

		// pass0:
		{
			device->BindResource(CS, &res.scratch[0][0], TEXSLOT_ONDEMAND2, cmd);
			device->BindResource(CS, &res.scratch[1][0], TEXSLOT_ONDEMAND3, cmd);
			device->BindResource(CS, &res.scratch[2][0], TEXSLOT_ONDEMAND4, cmd);
			device->BindResource(CS, &res.scratch[3][0], TEXSLOT_ONDEMAND5, cmd);
			const GPUResource* uavs[] = {
				&res.scratch[0][1],
				&res.scratch[1][1],
				&res.scratch[2][1],
				&res.scratch[3][1],
				&res.denoised
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&res.scratch[0][1], res.scratch[0][1].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
					GPUBarrier::Image(&res.scratch[1][1], res.scratch[1][1].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
					GPUBarrier::Image(&res.scratch[2][1], res.scratch[2][1].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
					GPUBarrier::Image(&res.scratch[3][1], res.scratch[3][1].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
					GPUBarrier::Image(&res.denoised, res.denoised.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			cb.xPPParams1.x = 0;
			cb.xPPParams1.y = 1;
			device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
			device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

			device->Dispatch(
				(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				4, // 4 lights
				cmd
			);
		}

		// pass1:
		{
			device->BindResource(CS, &res.scratch[0][1], TEXSLOT_ONDEMAND2, cmd);
			device->BindResource(CS, &res.scratch[1][1], TEXSLOT_ONDEMAND3, cmd);
			device->BindResource(CS, &res.scratch[2][1], TEXSLOT_ONDEMAND4, cmd);
			device->BindResource(CS, &res.scratch[3][1], TEXSLOT_ONDEMAND5, cmd);
			const GPUResource* uavs[] = {
				&res.scratch[0][0],
				&res.scratch[1][0],
				&res.scratch[2][0],
				&res.scratch[3][0],
				&res.denoised
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
					GPUBarrier::Image(&res.scratch[0][1], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[0][1].desc.layout),
					GPUBarrier::Image(&res.scratch[1][1], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[1][1].desc.layout),
					GPUBarrier::Image(&res.scratch[2][1], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[2][1].desc.layout),
					GPUBarrier::Image(&res.scratch[3][1], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[3][1].desc.layout),
					GPUBarrier::Image(&res.scratch[0][0], res.scratch[0][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
					GPUBarrier::Image(&res.scratch[1][0], res.scratch[1][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
					GPUBarrier::Image(&res.scratch[2][0], res.scratch[2][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
					GPUBarrier::Image(&res.scratch[3][0], res.scratch[3][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			cb.xPPParams1.x = 1;
			cb.xPPParams1.y = 2;
			device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
			device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

			device->Dispatch(
				(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				4, // 4 lights
				cmd
			);
		}

		// pass2:
		{
			device->BindResource(CS, &res.scratch[0][0], TEXSLOT_ONDEMAND2, cmd);
			device->BindResource(CS, &res.scratch[1][0], TEXSLOT_ONDEMAND3, cmd);
			device->BindResource(CS, &res.scratch[2][0], TEXSLOT_ONDEMAND4, cmd);
			device->BindResource(CS, &res.scratch[3][0], TEXSLOT_ONDEMAND5, cmd);
			const GPUResource* uavs[] = {
				&res.scratch[0][1],
				&res.scratch[1][1],
				&res.scratch[2][1],
				&res.scratch[3][1],
				&res.denoised
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
					GPUBarrier::Image(&res.scratch[0][0], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[0][0].desc.layout),
					GPUBarrier::Image(&res.scratch[1][0], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[1][0].desc.layout),
					GPUBarrier::Image(&res.scratch[2][0], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[2][0].desc.layout),
					GPUBarrier::Image(&res.scratch[3][0], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[3][0].desc.layout),
					GPUBarrier::Image(&res.scratch[0][1], res.scratch[0][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
					GPUBarrier::Image(&res.scratch[1][1], res.scratch[1][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
					GPUBarrier::Image(&res.scratch[2][1], res.scratch[2][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
					GPUBarrier::Image(&res.scratch[3][1], res.scratch[3][0].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			cb.xPPParams1.x = 2;
			cb.xPPParams1.y = 4;
			device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
			device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

			device->Dispatch(
				(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
				4, // 4 lights
				cmd
			);
		}

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.scratch[0][1], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[0][1].desc.layout),
				GPUBarrier::Image(&res.scratch[1][1], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[1][1].desc.layout),
				GPUBarrier::Image(&res.scratch[2][1], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[2][1].desc.layout),
				GPUBarrier::Image(&res.scratch[3][1], IMAGE_LAYOUT_UNORDERED_ACCESS, res.scratch[3][1].desc.layout),
				GPUBarrier::Image(&res.denoised, IMAGE_LAYOUT_UNORDERED_ACCESS, res.denoised.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}
	res.frame++;

	// Temporal pass:
	{
		device->EventBegin("Temporal Denoise", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_TEMPORAL], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &res.temp, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &res.temporal[temporal_history], TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &depth_history, TEXSLOT_ONDEMAND2, cmd);
		device->BindResource(CS, &res.denoised, TEXSLOT_ONDEMAND3, cmd);

		const GPUResource* uavs[] = {
			&res.temporal[temporal_output],
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.temp, IMAGE_LAYOUT_UNORDERED_ACCESS, res.temp.desc.layout),
				GPUBarrier::Image(&res.temporal[temporal_output], res.temporal[temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.temporal[temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.temporal[temporal_output].desc.layout),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(prof_range);
	device->EventEnd(cmd);
}
void CreateScreenSpaceShadowResources(ScreenSpaceShadowResources& res, XMUINT2 resolution)
{
}
void Postprocess_ScreenSpaceShadow(
	const ScreenSpaceShadowResources& res,
	const Texture& depthbuffer,
	const Texture& lineardepth,
	const GPUBuffer& entityTiles_Opaque,
	const Texture& output,
	CommandList cmd,
	float range,
	uint32_t samplecount
)
{
	device->EventBegin("Postprocess_ScreenSpaceShadow", cmd);
	auto prof_range = wiProfiler::BeginRangeGPU("ScreenSpaceShadow", cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = range;
	cb.xPPParams0.y = (float)samplecount;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_SCREENSPACESHADOW], cmd);

	device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	device->BindResource(CS, &entityTiles_Opaque, TEXSLOT_RENDERPATH_ENTITYTILES, cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	wiProfiler::EndRange(prof_range);
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

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void CreateDepthOfFieldResources(DepthOfFieldResources& res, XMUINT2 resolution)
{
	TextureDesc tile_desc;
	tile_desc.type = TextureDesc::TEXTURE_2D;
	tile_desc.Width = (resolution.x + DEPTHOFFIELD_TILESIZE - 1) / DEPTHOFFIELD_TILESIZE;
	tile_desc.Height = (resolution.y + DEPTHOFFIELD_TILESIZE - 1) / DEPTHOFFIELD_TILESIZE;
	tile_desc.Format = FORMAT_R16G16_FLOAT;
	tile_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	device->CreateTexture(&tile_desc, nullptr, &res.texture_tilemax);
	device->CreateTexture(&tile_desc, nullptr, &res.texture_neighborhoodmax);
	tile_desc.Format = FORMAT_R16_FLOAT;
	device->CreateTexture(&tile_desc, nullptr, &res.texture_tilemin);

	tile_desc.Height = resolution.x;
	tile_desc.Format = FORMAT_R16G16_FLOAT;
	device->CreateTexture(&tile_desc, nullptr, &res.texture_tilemax_horizontal);
	tile_desc.Format = FORMAT_R16_FLOAT;
	device->CreateTexture(&tile_desc, nullptr, &res.texture_tilemin_horizontal);

	TextureDesc presort_desc;
	presort_desc.type = TextureDesc::TEXTURE_2D;
	presort_desc.Width = resolution.x / 2;
	presort_desc.Height = resolution.y / 2;
	presort_desc.Format = FORMAT_R11G11B10_FLOAT;
	presort_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	device->CreateTexture(&presort_desc, nullptr, &res.texture_presort);
	device->CreateTexture(&presort_desc, nullptr, &res.texture_prefilter);
	device->CreateTexture(&presort_desc, nullptr, &res.texture_main);
	device->CreateTexture(&presort_desc, nullptr, &res.texture_postfilter);
	presort_desc.Format = FORMAT_R8_UNORM;
	device->CreateTexture(&presort_desc, nullptr, &res.texture_alpha1);
	device->CreateTexture(&presort_desc, nullptr, &res.texture_alpha2);


	GPUBufferDesc bufferdesc;
	bufferdesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;

	bufferdesc.ByteWidth = TILE_STATISTICS_CAPACITY * sizeof(uint);
	bufferdesc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | RESOURCE_MISC_INDIRECT_ARGS;
	device->CreateBuffer(&bufferdesc, nullptr, &res.buffer_tile_statistics);

	bufferdesc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferdesc.StructureByteStride = sizeof(uint);
	bufferdesc.ByteWidth = tile_desc.Width * tile_desc.Height * bufferdesc.StructureByteStride;
	device->CreateBuffer(&bufferdesc, nullptr, &res.buffer_tiles_earlyexit);
	device->CreateBuffer(&bufferdesc, nullptr, &res.buffer_tiles_cheap);
	device->CreateBuffer(&bufferdesc, nullptr, &res.buffer_tiles_expensive);
}
void Postprocess_DepthOfField(
	const DepthOfFieldResources& res,
	const Texture& input,
	const Texture& output,
	const Texture& lineardepth,
	CommandList cmd,
	float coc_scale,
	float max_coc
)
{
	device->EventBegin("Postprocess_DepthOfField", cmd);
	auto range = wiProfiler::BeginRangeGPU("Depth of Field", cmd);

	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.dof_cocscale = coc_scale;
	cb.dof_maxcoc = max_coc;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Compute tile max COC (horizontal):
	{
		device->EventBegin("TileMax - Horizontal", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_TILEMAXCOC_HORIZONTAL], cmd);

		const GPUResource* uavs[] = {
			&res.texture_tilemax_horizontal,
			&res.texture_tilemin_horizontal,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_tilemax_horizontal, res.texture_tilemax_horizontal.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_tilemin_horizontal, res.texture_tilemin_horizontal.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(res.texture_tilemax_horizontal.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(res.texture_tilemax_horizontal.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_tilemax_horizontal, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_tilemax_horizontal.desc.layout),
				GPUBarrier::Image(&res.texture_tilemin_horizontal, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_tilemin_horizontal.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Compute tile max COC (vertical):
	{
		device->EventBegin("TileMax - Vertical", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_TILEMAXCOC_VERTICAL], cmd);

		const GPUResource* resarray[] = {
			&res.texture_tilemax_horizontal,
			&res.texture_tilemin_horizontal
		};
		device->BindResources(CS, resarray, TEXSLOT_ONDEMAND0, arraysize(resarray), cmd);

		const GPUResource* uavs[] = {
			&res.texture_tilemax,
			&res.texture_tilemin,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_tilemax, res.texture_tilemax.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_tilemin, res.texture_tilemin.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(res.texture_tilemax.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(res.texture_tilemax.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_tilemax, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_tilemax.desc.layout),
				GPUBarrier::Image(&res.texture_tilemin, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_tilemin.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Compute max COC for each tiles' neighborhood
	{
		device->EventBegin("NeighborhoodMax", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_NEIGHBORHOODMAXCOC], cmd);

		const GPUResource* resarray[] = {
			&res.texture_tilemax,
			&res.texture_tilemin
		};
		device->BindResources(CS, resarray, TEXSLOT_ONDEMAND0, arraysize(resarray), cmd);

		const GPUResource* uavs[] = {
			&res.buffer_tile_statistics,
			&res.buffer_tiles_earlyexit,
			&res.buffer_tiles_cheap,
			&res.buffer_tiles_expensive,
			&res.texture_neighborhoodmax
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_neighborhoodmax, res.texture_neighborhoodmax.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(res.texture_neighborhoodmax.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(res.texture_neighborhoodmax.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_neighborhoodmax, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_neighborhoodmax.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Kick indirect tile jobs:
	{
		device->EventBegin("Kickjobs", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_KICKJOBS], cmd);

		device->BindResource(CS, &res.texture_tilemax, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&res.buffer_tile_statistics,
			&res.buffer_tiles_earlyexit,
			&res.buffer_tiles_cheap,
			&res.buffer_tiles_expensive
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(1, 1, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Buffer(&res.buffer_tile_statistics, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_INDIRECT_ARGUMENT),
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

		const GPUResource* resarray[] = {
			&input,
			&res.texture_neighborhoodmax,
		};
		device->BindResources(CS, resarray, TEXSLOT_ONDEMAND0, arraysize(resarray), cmd);

		const GPUResource* uavs[] = {
			&res.texture_presort,
			&res.texture_prefilter
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&res.buffer_tiles_earlyexit, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
				GPUBarrier::Buffer(&res.buffer_tiles_cheap, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
				GPUBarrier::Buffer(&res.buffer_tiles_expensive, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
				GPUBarrier::Image(&res.texture_presort, res.texture_presort.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_prefilter, res.texture_prefilter.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->BindResource(CS, &res.buffer_tiles_earlyexit, TEXSLOT_ONDEMAND2, cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS_EARLYEXIT], cmd);
		device->DispatchIndirect(&res.buffer_tile_statistics, INDIRECT_OFFSET_EARLYEXIT, cmd);

		device->BindResource(CS, &res.buffer_tiles_cheap, TEXSLOT_ONDEMAND2, cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS], cmd);
		device->DispatchIndirect(&res.buffer_tile_statistics, INDIRECT_OFFSET_CHEAP, cmd);

		device->BindResource(CS, &res.buffer_tiles_expensive, TEXSLOT_ONDEMAND2, cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS], cmd);
		device->DispatchIndirect(&res.buffer_tile_statistics, INDIRECT_OFFSET_EXPENSIVE, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_presort, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_presort.desc.layout),
				GPUBarrier::Image(&res.texture_prefilter, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_prefilter.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Main pass:
	{
		device->EventBegin("Main pass", cmd);

		const GPUResource* resarray[] = {
			&res.texture_neighborhoodmax,
			&res.texture_presort,
			&res.texture_prefilter,
			&res.buffer_tiles_earlyexit,
			&res.buffer_tiles_cheap,
			&res.buffer_tiles_expensive
		};
		device->BindResources(CS, resarray, TEXSLOT_ONDEMAND0, arraysize(resarray), cmd);

		const GPUResource* uavs[] = {
			&res.texture_main,
			&res.texture_alpha1
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_main, res.texture_main.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_alpha1, res.texture_alpha1.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN_EARLYEXIT], cmd);
		device->DispatchIndirect(&res.buffer_tile_statistics, INDIRECT_OFFSET_EARLYEXIT, cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN_CHEAP], cmd);
		device->DispatchIndirect(&res.buffer_tile_statistics, INDIRECT_OFFSET_CHEAP, cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN], cmd);
		device->DispatchIndirect(&res.buffer_tile_statistics, INDIRECT_OFFSET_EXPENSIVE, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_main, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_main.desc.layout),
				GPUBarrier::Image(&res.texture_alpha1, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_alpha1.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Post filter:
	{
		device->EventBegin("Post filter", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_POSTFILTER], cmd);

		const GPUResource* resarray[] = {
			&res.texture_main,
			&res.texture_alpha1
		};
		device->BindResources(CS, resarray, TEXSLOT_ONDEMAND0, arraysize(resarray), cmd);

		const GPUResource* uavs[] = {
			&res.texture_postfilter,
			&res.texture_alpha2
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_postfilter, res.texture_postfilter.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_alpha2, res.texture_alpha2.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(res.texture_postfilter.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(res.texture_postfilter.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_postfilter, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_postfilter.desc.layout),
				GPUBarrier::Image(&res.texture_alpha2, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_alpha2.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

		const GPUResource* resarray[] = {
			&input,
			&res.texture_postfilter,
			&res.texture_alpha2,
			&res.texture_neighborhoodmax
		};
		device->BindResources(CS, resarray, TEXSLOT_ONDEMAND0, arraysize(resarray), cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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
void CreateMotionBlurResources(MotionBlurResources& res, XMUINT2 resolution)
{
	TextureDesc tile_desc;
	tile_desc.type = TextureDesc::TEXTURE_2D;
	tile_desc.Width = (resolution.x + MOTIONBLUR_TILESIZE - 1) / MOTIONBLUR_TILESIZE;
	tile_desc.Height = (resolution.y + MOTIONBLUR_TILESIZE - 1) / MOTIONBLUR_TILESIZE;
	tile_desc.Format = FORMAT_R16G16_FLOAT;
	tile_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	device->CreateTexture(&tile_desc, nullptr, &res.texture_tilemin);
	device->CreateTexture(&tile_desc, nullptr, &res.texture_tilemax);
	device->CreateTexture(&tile_desc, nullptr, &res.texture_neighborhoodmax);

	tile_desc.Height = resolution.y;
	device->CreateTexture(&tile_desc, nullptr, &res.texture_tilemax_horizontal);
	device->CreateTexture(&tile_desc, nullptr, &res.texture_tilemin_horizontal);


	GPUBufferDesc bufferdesc;
	bufferdesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;

	bufferdesc.ByteWidth = TILE_STATISTICS_CAPACITY * sizeof(uint);
	bufferdesc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | RESOURCE_MISC_INDIRECT_ARGS;
	device->CreateBuffer(&bufferdesc, nullptr, &res.buffer_tile_statistics);

	bufferdesc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferdesc.StructureByteStride = sizeof(uint);
	bufferdesc.ByteWidth = tile_desc.Width * tile_desc.Height * bufferdesc.StructureByteStride;
	device->CreateBuffer(&bufferdesc, nullptr, &res.buffer_tiles_earlyexit);
	device->CreateBuffer(&bufferdesc, nullptr, &res.buffer_tiles_cheap);
	device->CreateBuffer(&bufferdesc, nullptr, &res.buffer_tiles_expensive);
}
void Postprocess_MotionBlur(
	const MotionBlurResources& res,
	const Texture& input,
	const Texture& lineardepth,
	const Texture gbuffer[GBUFFER_COUNT],
	const Texture& output,
	CommandList cmd,
	float strength
)
{
	device->EventBegin("Postprocess_MotionBlur", cmd);
	auto range = wiProfiler::BeginRangeGPU("MotionBlur", cmd);

	device->BindResource(CS, &lineardepth, TEXSLOT_LINEARDEPTH, cmd);
	device->BindResource(CS, &gbuffer[GBUFFER_VELOCITY], TEXSLOT_GBUFFER2, cmd);

	const TextureDesc& desc = output.GetDesc();

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
			&res.texture_tilemax_horizontal,
			&res.texture_tilemin_horizontal,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_tilemax_horizontal, res.texture_tilemax_horizontal.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_tilemin_horizontal, res.texture_tilemin_horizontal.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(res.texture_tilemax_horizontal.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(res.texture_tilemax_horizontal.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_tilemax_horizontal, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_tilemax_horizontal.desc.layout),
				GPUBarrier::Image(&res.texture_tilemin_horizontal, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_tilemin_horizontal.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Compute tile max velocities (vertical):
	{
		device->EventBegin("TileMax - Vertical", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_TILEMAXVELOCITY_VERTICAL], cmd);

		device->BindResource(CS, &res.texture_tilemax_horizontal, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&res.texture_tilemax,
			&res.texture_tilemin,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_tilemax, res.texture_tilemax.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_tilemin, res.texture_tilemin.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(res.texture_tilemax.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(res.texture_tilemax.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_tilemax, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_tilemax.desc.layout),
				GPUBarrier::Image(&res.texture_tilemin, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_tilemin.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Compute max velocities for each tiles' neighborhood
	{
		device->EventBegin("NeighborhoodMax", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_NEIGHBORHOODMAXVELOCITY], cmd);

		const GPUResource* resarray[] = {
			&res.texture_tilemax,
			&res.texture_tilemin
		};
		device->BindResources(CS, resarray, TEXSLOT_ONDEMAND0, arraysize(resarray), cmd);

		const GPUResource* uavs[] = {
			&res.buffer_tile_statistics,
			&res.buffer_tiles_earlyexit,
			&res.buffer_tiles_cheap,
			&res.buffer_tiles_expensive,
			&res.texture_neighborhoodmax
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_neighborhoodmax, res.texture_neighborhoodmax.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(res.texture_neighborhoodmax.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(res.texture_neighborhoodmax.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_neighborhoodmax, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_neighborhoodmax.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Kick indirect tile jobs:
	{
		device->EventBegin("Kickjobs", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_KICKJOBS], cmd);

		device->BindResource(CS, &res.texture_tilemax, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&res.buffer_tile_statistics,
			&res.buffer_tiles_earlyexit,
			&res.buffer_tiles_cheap,
			&res.buffer_tiles_expensive
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(1, 1, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Buffer(&res.buffer_tile_statistics, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_INDIRECT_ARGUMENT),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Tile jobs:
	{
		device->EventBegin("MotionBlur Jobs", cmd);

		const GPUResource* resarray[] = {
			&input,
			&res.texture_neighborhoodmax,
			&res.buffer_tiles_earlyexit,
			&res.buffer_tiles_cheap,
			&res.buffer_tiles_expensive,
		};
		device->BindResources(CS, resarray, TEXSLOT_ONDEMAND0, arraysize(resarray), cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&res.buffer_tiles_earlyexit, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
				GPUBarrier::Buffer(&res.buffer_tiles_cheap, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
				GPUBarrier::Buffer(&res.buffer_tiles_expensive, BUFFER_STATE_UNORDERED_ACCESS, BUFFER_STATE_SHADER_RESOURCE),
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_EARLYEXIT], cmd);
		device->DispatchIndirect(&res.buffer_tile_statistics, INDIRECT_OFFSET_EARLYEXIT, cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_CHEAP], cmd);
		device->DispatchIndirect(&res.buffer_tile_statistics, INDIRECT_OFFSET_CHEAP, cmd);

		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_MOTIONBLUR], cmd);
		device->DispatchIndirect(&res.buffer_tile_statistics, INDIRECT_OFFSET_EXPENSIVE, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, arraysize(resarray), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void CreateBloomResources(BloomResources& res, XMUINT2 resolution)
{
	TextureDesc desc;
	desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	desc.Format = FORMAT_R11G11B10_FLOAT;
	desc.Width = resolution.x / 4;
	desc.Height = resolution.y / 4;
	desc.MipLevels = std::min(5u, (uint32_t)std::log2(std::max(desc.Width, desc.Height)));
	device->CreateTexture(&desc, nullptr, &res.texture_bloom);
	device->SetName(&res.texture_bloom, "bloom.texture_bloom");
	device->CreateTexture(&desc, nullptr, &res.texture_temp);
	device->SetName(&res.texture_temp, "bloom.texture_temp");

	for (uint32_t i = 0; i < res.texture_bloom.desc.MipLevels; ++i)
	{
		int subresource_index;
		subresource_index = device->CreateSubresource(&res.texture_bloom, SRV, 0, 1, i, 1);
		assert(subresource_index == i);
		subresource_index = device->CreateSubresource(&res.texture_temp, SRV, 0, 1, i, 1);
		assert(subresource_index == i);
		subresource_index = device->CreateSubresource(&res.texture_bloom, UAV, 0, 1, i, 1);
		assert(subresource_index == i);
		subresource_index = device->CreateSubresource(&res.texture_temp, UAV, 0, 1, i, 1);
		assert(subresource_index == i);
	}
}
void Postprocess_Bloom(
	const BloomResources& res,
	const Texture& input,
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

		const TextureDesc& desc = res.texture_bloom.GetDesc();

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
			&res.texture_bloom,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_bloom, res.texture_bloom.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_bloom, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_bloom.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	device->EventBegin("Bloom Mipchain", cmd);
	MIPGEN_OPTIONS mipopt;
	mipopt.gaussian_temp = &res.texture_temp;
	mipopt.wide_gauss = true;
	GenerateMipChain(res.texture_bloom, wiRenderer::MIPGENFILTER_GAUSSIAN, cmd, mipopt);
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
		device->BindResource(CS, &res.texture_bloom, TEXSLOT_ONDEMAND1, cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void CreateVolumetricCloudResources(VolumetricCloudResources& res, XMUINT2 resolution)
{
	XMUINT2 renderResolution = XMUINT2(resolution.x / 4, resolution.y / 4);
	XMUINT2 reprojectionResolution = XMUINT2(resolution.x / 2, resolution.y / 2);
	XMUINT2 maskResolution = XMUINT2(resolution.x / 4, resolution.y / 4); // Needs to be half of final cloud output

	TextureDesc desc;
	desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	desc.Width = renderResolution.x;
	desc.Height = renderResolution.y;
	desc.Format = FORMAT_R16G16B16A16_FLOAT;
	desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE_COMPUTE;
	device->CreateTexture(&desc, nullptr, &res.texture_cloudRender);
	device->SetName(&res.texture_cloudRender, "texture_cloudRender");
	desc.Format = FORMAT_R16G16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.texture_cloudDepth);
	device->SetName(&res.texture_cloudDepth, "texture_cloudDepth");

	desc.Width = reprojectionResolution.x;
	desc.Height = reprojectionResolution.y;
	desc.Format = FORMAT_R16G16B16A16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.texture_reproject[0]);
	device->SetName(&res.texture_reproject[0], "texture_reproject[0]");
	device->CreateTexture(&desc, nullptr, &res.texture_reproject[1]);
	device->SetName(&res.texture_reproject[1], "texture_reproject[1]");
	desc.Format = FORMAT_R16G16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.texture_reproject_depth[0]);
	device->SetName(&res.texture_reproject_depth[0], "texture_reproject_depth[0]");
	device->CreateTexture(&desc, nullptr, &res.texture_reproject_depth[1]);
	device->SetName(&res.texture_reproject_depth[1], "texture_reproject_depth[1]");

	desc.Format = FORMAT_R16G16B16A16_FLOAT;
	device->CreateTexture(&desc, nullptr, &res.texture_temporal[0]);
	device->SetName(&res.texture_temporal[0], "texture_temporal[0]");
	device->CreateTexture(&desc, nullptr, &res.texture_temporal[1]);
	device->SetName(&res.texture_temporal[1], "texture_temporal[1]");

	desc.Width = maskResolution.x;
	desc.Height = maskResolution.y;
	desc.Format = FORMAT_R8G8B8A8_UNORM;
	device->CreateTexture(&desc, nullptr, &res.texture_cloudMask);
	device->SetName(&res.texture_cloudMask, "texture_cloudMask");
}
void Postprocess_VolumetricClouds(
	const VolumetricCloudResources& res,
	const Texture& depthbuffer,
	CommandList cmd
)
{
	device->EventBegin("Postprocess_VolumetricClouds", cmd);
	auto range = wiProfiler::BeginRangeGPU("Volumetric Clouds", cmd);

	BindCommonResources(cmd);

	const TextureDesc& desc = res.texture_reproject[0].GetDesc();
	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = (float)res.texture_reproject[0].GetDesc().Width;
	cb.xPPParams0.y = (float)res.texture_reproject[0].GetDesc().Height;
	cb.xPPParams0.z = 1.0f / cb.xPPParams0.x;
	cb.xPPParams0.w = 1.0f / cb.xPPParams0.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	// Cloud pass:
	{
		device->EventBegin("Volumetric Cloud Rendering", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_RENDER], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &texture_shapeNoise, TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &texture_detailNoise, TEXSLOT_ONDEMAND2, cmd);
		device->BindResource(CS, &texture_curlNoise, TEXSLOT_ONDEMAND3, cmd);
		device->BindResource(CS, &texture_weatherMap, TEXSLOT_ONDEMAND4, cmd);
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT], TEXSLOT_TRANSMITTANCELUT, cmd);
		device->BindResource(CS, &textures[TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT], TEXSLOT_SKYLUMINANCELUT, cmd);

		const GPUResource* uavs[] = {
			&res.texture_cloudRender,
			&res.texture_cloudDepth,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_cloudRender, res.texture_cloudRender.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_cloudDepth, res.texture_cloudDepth.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(res.texture_cloudRender.GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(res.texture_cloudRender.GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_cloudRender, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_cloudRender.desc.layout),
				GPUBarrier::Image(&res.texture_cloudDepth, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_cloudDepth.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	const TextureDesc& reprojection_desc = res.texture_reproject[0].GetDesc();
	cb.xPPResolution.x = reprojection_desc.Width;
	cb.xPPResolution.y = reprojection_desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);
	
	int temporal_output = device->GetFrameCount() % 2;
	int temporal_history = 1 - temporal_output;

	// Reprojection pass:
	{
		device->EventBegin("Volumetric Cloud Reproject", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_REPROJECT], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &res.texture_cloudRender, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &res.texture_cloudDepth, TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &res.texture_reproject[temporal_history], TEXSLOT_ONDEMAND2, cmd);
		device->BindResource(CS, &res.texture_reproject_depth[temporal_history], TEXSLOT_ONDEMAND3, cmd);

		const GPUResource* uavs[] = {
			&res.texture_reproject[temporal_output],
			&res.texture_reproject_depth[temporal_output],
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_reproject[temporal_output], res.texture_reproject[temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_reproject_depth[temporal_output], res.texture_reproject_depth[temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(res.texture_reproject[temporal_output].GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(res.texture_reproject[temporal_output].GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_reproject[temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_reproject[temporal_output].desc.layout),
				GPUBarrier::Image(&res.texture_reproject_depth[temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_reproject_depth[temporal_output].desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->EventEnd(cmd);
	}

	// Temporal pass:
	{
		device->EventBegin("Volumetric Cloud Temporal", cmd);
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_TEMPORAL], cmd);

		device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);
		device->BindResource(CS, &res.texture_reproject[temporal_output], TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, &res.texture_reproject_depth[temporal_output], TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, &res.texture_temporal[temporal_history], TEXSLOT_ONDEMAND2, cmd);

		const GPUResource* uavs[] = {
			&res.texture_temporal[temporal_output],
			&res.texture_cloudMask,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&res.texture_temporal[temporal_output], res.texture_temporal[temporal_output].desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
				GPUBarrier::Image(&res.texture_cloudMask, res.texture_cloudMask.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(res.texture_temporal[temporal_output].GetDesc().Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(res.texture_temporal[temporal_output].GetDesc().Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&res.texture_temporal[temporal_output], IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_temporal[temporal_output].desc.layout),
				GPUBarrier::Image(&res.texture_cloudMask, IMAGE_LAYOUT_UNORDERED_ACCESS, res.texture_cloudMask.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_TemporalAA(
	const Texture& input_current,
	const Texture& input_history,
	const Texture& lineardepth,
	const Texture& depth_history,
	const Texture gbuffer[GBUFFER_COUNT],
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
	device->BindResource(CS, &gbuffer[GBUFFER_VELOCITY], TEXSLOT_GBUFFER2, cmd);

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

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	wiProfiler::EndRange(range);
	device->EventEnd(cmd);
}
void Postprocess_DepthPyramid(
	const Texture& depthbuffer,
	const Texture& lineardepth,
	CommandList cmd
)
{
	device->EventBegin("Postprocess_DepthPyramid", cmd);
	auto range = wiProfiler::BeginRangeGPU("Depth Pyramid", cmd);

	BindCommonResources(cmd);

	const TextureDesc& desc = lineardepth.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width / 2;  // downsample res
	cb.xPPResolution.y = desc.Height / 2; // downsample res
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.xPPParams0.x = (float)depthbuffer.GetDesc().Width;
	cb.xPPParams0.y = (float)depthbuffer.GetDesc().Height;
	cb.xPPParams0.z = 1.0f / cb.xPPParams0.x;
	cb.xPPParams0.w = 1.0f / cb.xPPParams0.y;
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd, 0); // full res native depth

	device->BindUAV(CS, &lineardepth, 0, cmd, 0); // full res linear depth
	device->BindUAV(CS, &lineardepth, 1, cmd, 1); // 2x downsample linear depth
	device->BindUAV(CS, &lineardepth, 2, cmd, 2); // 4x downsample linear depth
	device->BindUAV(CS, &lineardepth, 3, cmd, 3); // 8x downsample linear depth
	device->BindUAV(CS, &lineardepth, 4, cmd, 4); // 16x downsample linear depth
	device->BindUAV(CS, &lineardepth, 5, cmd, 5); // 32x downsample linear depth

	device->BindUAV(CS, &depthbuffer, 6, cmd, 1); // 2x downsample native depth
	device->BindUAV(CS, &depthbuffer, 7, cmd, 2); // 4x downsample native depth

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&lineardepth, lineardepth.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			GPUBarrier::Image(&depthbuffer, depthbuffer.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, 1),
			GPUBarrier::Image(&depthbuffer, depthbuffer.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS, 2),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_LINEARDEPTH], cmd);
	device->Dispatch(
		(desc.Width + POSTPROCESS_LINEARDEPTH_BLOCKSIZE - 1) / POSTPROCESS_LINEARDEPTH_BLOCKSIZE,
		(desc.Height + POSTPROCESS_LINEARDEPTH_BLOCKSIZE - 1) / POSTPROCESS_LINEARDEPTH_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&lineardepth, IMAGE_LAYOUT_UNORDERED_ACCESS, lineardepth.desc.layout),
			GPUBarrier::Image(&depthbuffer, IMAGE_LAYOUT_UNORDERED_ACCESS, depthbuffer.desc.layout, 1),
			GPUBarrier::Image(&depthbuffer, IMAGE_LAYOUT_UNORDERED_ACCESS, depthbuffer.desc.layout, 2),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, 8, cmd);

	wiProfiler::EndRange(range);
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

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, arraysize(uavs), cmd);

	device->EventEnd(cmd);
}
void Postprocess_Tonemap(
	const Texture& input,
	const Texture& output,
	CommandList cmd,
	float exposure,
	bool dither,
	const Texture* texture_colorgradinglut,
	const Texture* texture_distortion,
	const Texture* texture_luminance,
	float eyeadaptionkey
)
{
	device->EventBegin("Postprocess_Tonemap", cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_TONEMAP], cmd);

	const TextureDesc& desc = output.GetDesc();

	PostProcessCB cb;
	cb.xPPResolution.x = desc.Width;
	cb.xPPResolution.y = desc.Height;
	cb.xPPResolution_rcp.x = 1.0f / cb.xPPResolution.x;
	cb.xPPResolution_rcp.y = 1.0f / cb.xPPResolution.y;
	cb.tonemap_exposure = exposure;
	cb.tonemap_dither = dither ? 1.0f : 0.0f;
	cb.tonemap_colorgrading = texture_colorgradinglut == nullptr ? 0.0f : 1.0f;
	cb.tonemap_eyeadaption = texture_luminance == nullptr ? 0.0f : 1.0f;
	cb.tonemap_distortion = texture_distortion == nullptr ? 0.0f : 1.0f;
	cb.tonemap_eyeadaptionkey = eyeadaptionkey;

	assert(texture_colorgradinglut == nullptr || texture_colorgradinglut->desc.type == TextureDesc::TEXTURE_3D); // This must be a 3D lut

	if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_BINDLESS_DESCRIPTORS))
	{
		PushConstantsTonemap push = {};
		push.xPPResolution_rcp = cb.xPPResolution_rcp;
		push.exposure = cb.tonemap_exposure;
		push.dither = cb.tonemap_dither;
		push.eyeadaptionkey = cb.tonemap_eyeadaptionkey;
		push.texture_input = device->GetDescriptorIndex(&input, SRV);
		push.texture_input_luminance = device->GetDescriptorIndex(texture_luminance, SRV);
		push.texture_input_distortion = device->GetDescriptorIndex(texture_distortion, SRV);
		push.texture_colorgrade_lookuptable = device->GetDescriptorIndex(texture_colorgradinglut, SRV);
		push.texture_output = device->GetDescriptorIndex(&output, UAV);
		device->PushConstants(&push, sizeof(push), cmd);
	}
	else
	{
		device->UnbindResources(TEXSLOT_ONDEMAND0, 4, cmd);

		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);
		device->BindResource(CS, texture_luminance, TEXSLOT_ONDEMAND1, cmd);
		device->BindResource(CS, texture_distortion, TEXSLOT_ONDEMAND2, cmd);

		if (texture_colorgradinglut != nullptr)
		{
			device->BindResource(CS, texture_colorgradinglut, TEXSLOT_ONDEMAND3, cmd);
		}
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);
	}

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, 1, cmd);

	device->EventEnd(cmd);
}

#define A_CPU
#include "shaders/ffx-fsr/ffx_a.h"
#include "shaders/ffx-fsr/ffx_fsr1.h"
void Postprocess_FSR(
	const Texture& input,
	const Texture& temp,
	const Texture& output,
	CommandList cmd,
	float sharpness
)
{
	device->EventBegin("Postprocess_FSR", cmd);
	auto range = wiProfiler::BeginRangeGPU("Postprocess_FSR", cmd);

	const TextureDesc& desc = output.GetDesc();

	struct FSRCB
	{
		AU1 const0[4];
		AU1 const1[4];
		AU1 const2[4];
		AU1 const3[4];
	} cb;

	// Upscaling:
	{
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_FSR_UPSCALING], cmd);

		FsrEasuCon(
			cb.const0,
			cb.const1,
			cb.const2,
			cb.const3,

			// current frame render resolution:
			static_cast<AF1>(input.desc.Width),
			static_cast<AF1>(input.desc.Height),

			// input container resolution:
			static_cast<AF1>(input.desc.Width),
			static_cast<AF1>(input.desc.Height),

			// upscaled-to-resolution:
			static_cast<AF1>(temp.desc.Width),
			static_cast<AF1>(temp.desc.Height)

		);
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS_FSR], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS_FSR], CB_GETBINDSLOT(FSRCB), cmd);

		device->BindResource(CS, &input, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&temp,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&temp, temp.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch((desc.Width + 15) / 16, (desc.Height + 15) / 16, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&temp, IMAGE_LAYOUT_UNORDERED_ACCESS, temp.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}

	// Sharpen:
	{
		device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_FSR_SHARPEN], cmd);

		FsrRcasCon(cb.const0, sharpness);
		device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS_FSR], &cb, cmd);
		device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS_FSR], CB_GETBINDSLOT(FSRCB), cmd);

		device->BindResource(CS, &temp, TEXSLOT_ONDEMAND0, cmd);

		const GPUResource* uavs[] = {
			&output,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch((desc.Width + 15) / 16, (desc.Height + 15) / 16, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}

	wiProfiler::EndRange(range);
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

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

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
	cb.xPPParams1.x = (float)input.GetDesc().Width;
	cb.xPPParams1.y = (float)input.GetDesc().Height;
	cb.xPPParams1.z = 1.0f / cb.xPPParams1.x;
	cb.xPPParams1.w = 1.0f / cb.xPPParams1.y;
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
		case FORMAT_R32G32B32A32_UINT:
			cs = CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UINT4;
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

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->Dispatch(
			(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
			1,
			cmd
		);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
				GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

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

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

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
	cb.xPPParams0.x = floorf(std::max(1.0f, log2f(std::max((float)desc.Width / (float)depthbuffer.GetDesc().Width, (float)desc.Height / (float)depthbuffer.GetDesc().Height))));
	device->UpdateBuffer(&constantBuffers[CBTYPE_POSTPROCESS], &cb, cmd);
	device->BindConstantBuffer(CS, &constantBuffers[CBTYPE_POSTPROCESS], CB_GETBINDSLOT(PostProcessCB), cmd);

	device->BindComputeShader(&shaders[CSTYPE_POSTPROCESS_NORMALSFROMDEPTH], cmd);

	device->BindResource(CS, &depthbuffer, TEXSLOT_DEPTH, cmd);

	const GPUResource* uavs[] = {
		&output,
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Image(&output, output.desc.layout, IMAGE_LAYOUT_UNORDERED_ACCESS),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->Dispatch(
		(desc.Width + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		(desc.Height + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE,
		1,
		cmd
	);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Image(&output, IMAGE_LAYOUT_UNORDERED_ACCESS, output.desc.layout),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	device->UnbindUAVs(0, arraysize(uavs), cmd);
	device->EventEnd(cmd);
}


RAY GetPickRay(long cursorX, long cursorY, const wiCanvas& canvas, const CameraComponent& camera)
{
	float screenW = canvas.GetLogicalWidth();
	float screenH = canvas.GetLogicalHeight();

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



int GetShadowRes2D() { return SHADOWRES_2D; }
int GetShadowResCube() { return SHADOWRES_CUBE; }
void SetTransparentShadowsEnabled(float value) { TRANSPARENTSHADOWSENABLED = value; }
float GetTransparentShadowsEnabled() { return TRANSPARENTSHADOWSENABLED; }
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
void SetVoxelRadianceEnabled(bool enabled)
{
	voxelSceneData.enabled = enabled;
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
	}
	if (!textures[TEXTYPE_3D_VOXELRADIANCE_HELPER].IsValid())
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
}
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
void SetGameSpeed(float value) { GameSpeed = std::max(0.0f, value); }
float GetGameSpeed() { return GameSpeed; }
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
void SetDisableAlbedoMaps(bool value)
{
	disableAlbedoMaps = value;
}
bool IsDisableAlbedoMaps()
{
	return disableAlbedoMaps;
}
void SetScreenSpaceShadowsEnabled(bool value)
{
	SCREENSPACESHADOWS = value;
}
bool GetScreenSpaceShadowsEnabled()
{
	return SCREENSPACESHADOWS;
}

}
