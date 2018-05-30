#include "wiRenderer.h"
#include "wiFrameRate.h"
#include "wiHairParticle.h"
#include "wiEmittedParticle.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiLoader.h"
#include "wiFrustum.h"
#include "wiRenderTarget.h"
#include "wiDepthTarget.h"
#include "wiHelper.h"
#include "wiMath.h"
#include "wiLensFlare.h"
#include "wiTextureHelper.h"
#include "wiPHYSICS.h"
#include "wiLines.h"
#include "wiCube.h"
#include "wiWaterPlane.h"
#include "wiEnums.h"
#include "wiRandom.h"
#include "wiFont.h"
#include "wiTranslator.h"
#include "wiRectPacker.h"
#include "wiBackLog.h"
#include "wiProfiler.h"
#include "wiOcean.h"
#include "ShaderInterop_CloudGenerator.h"
#include "ShaderInterop_Skinning.h"
#include "wiWidget.h"
#include "wiGPUSortLib.h"

#include <algorithm>

#include <DirectXCollision.h>

using namespace std;
using namespace wiGraphicsTypes;

#pragma region STATICS
GraphicsDevice* wiRenderer::graphicsDevice = nullptr;
Sampler				*wiRenderer::samplers[SSLOT_COUNT];
VertexShader		*wiRenderer::vertexShaders[VSTYPE_LAST];
PixelShader			*wiRenderer::pixelShaders[PSTYPE_LAST];
GeometryShader		*wiRenderer::geometryShaders[GSTYPE_LAST];
HullShader			*wiRenderer::hullShaders[HSTYPE_LAST];
DomainShader		*wiRenderer::domainShaders[DSTYPE_LAST];
ComputeShader		*wiRenderer::computeShaders[CSTYPE_LAST];
VertexLayout		*wiRenderer::vertexLayouts[VLTYPE_LAST];
RasterizerState		*wiRenderer::rasterizers[RSTYPE_LAST];
DepthStencilState	*wiRenderer::depthStencils[DSSTYPE_LAST];
BlendState			*wiRenderer::blendStates[BSTYPE_LAST];
GPUBuffer			*wiRenderer::constantBuffers[CBTYPE_LAST];
GPUBuffer			*wiRenderer::resourceBuffers[RBTYPE_LAST];
Texture				*wiRenderer::textures[TEXTYPE_LAST];
Sampler				*wiRenderer::customsamplers[SSTYPE_LAST];

GPURingBuffer		*wiRenderer::dynamicVertexBufferPool;

float wiRenderer::GAMMA = 2.2f;
int wiRenderer::SHADOWRES_2D = 1024, wiRenderer::SHADOWRES_CUBE = 256, wiRenderer::SHADOWCOUNT_2D = 5 + 3 + 3, wiRenderer::SHADOWCOUNT_CUBE = 5, wiRenderer::SOFTSHADOWQUALITY_2D = 2;
bool wiRenderer::HAIRPARTICLEENABLED=true,wiRenderer::EMITTERSENABLED=true;
bool wiRenderer::TRANSPARENTSHADOWSENABLED = true;
bool wiRenderer::ALPHACOMPOSITIONENABLED = false;
bool wiRenderer::wireRender = false, wiRenderer::debugSpheres = false, wiRenderer::debugBoneLines = false, wiRenderer::debugPartitionTree = false, wiRenderer::debugEmitters = false, wiRenderer::freezeCullingCamera = false
, wiRenderer::debugEnvProbes = false, wiRenderer::debugForceFields = false, wiRenderer::debugCameras = false, wiRenderer::gridHelper = false, wiRenderer::voxelHelper = false, wiRenderer::requestReflectionRendering = false, wiRenderer::advancedLightCulling = true
, wiRenderer::advancedRefractions = false;
bool wiRenderer::ldsSkinningEnabled = true;
float wiRenderer::SPECULARAA = 0.0f;
float wiRenderer::renderTime = 0, wiRenderer::renderTime_Prev = 0, wiRenderer::deltaTime = 0;
XMFLOAT2 wiRenderer::temporalAAJitter = XMFLOAT2(0, 0), wiRenderer::temporalAAJitterPrev = XMFLOAT2(0, 0);
float wiRenderer::RESOLUTIONSCALE = 1.0f;
GPUQuery wiRenderer::occlusionQueries[];
UINT wiRenderer::entityArrayOffset_Lights = 0, wiRenderer::entityArrayCount_Lights = 0;
UINT wiRenderer::entityArrayOffset_Decals = 0, wiRenderer::entityArrayCount_Decals = 0;
UINT wiRenderer::entityArrayOffset_ForceFields = 0, wiRenderer::entityArrayCount_ForceFields = 0;
UINT wiRenderer::entityArrayOffset_EnvProbes = 0, wiRenderer::entityArrayCount_EnvProbes = 0;

Texture2D* wiRenderer::enviroMap,*wiRenderer::colorGrading;
float wiRenderer::GameSpeed=1,wiRenderer::overrideGameSpeed=1;
bool wiRenderer::debugLightCulling = false;
bool wiRenderer::occlusionCulling = false;
bool wiRenderer::temporalAA = false, wiRenderer::temporalAADEBUG = false;
wiRenderer::VoxelizedSceneData wiRenderer::voxelSceneData = VoxelizedSceneData();
int wiRenderer::visibleCount;
wiRenderTarget wiRenderer::normalMapRT, wiRenderer::imagesRT, wiRenderer::imagesRTAdd;
Camera *wiRenderer::cam = nullptr, *wiRenderer::refCam = nullptr, *wiRenderer::prevFrameCam = nullptr;
PHYSICS* wiRenderer::physicsEngine = nullptr;
wiOcean* wiRenderer::ocean = nullptr;

string wiRenderer::SHADERPATH = "shaders/";
#pragma endregion

#pragma region STATIC TEMP

int wiRenderer::vertexCount;
deque<wiSprite*> wiRenderer::images;
deque<wiSprite*> wiRenderer::waterRipples;

wiSPTree* wiRenderer::spTree = nullptr;
wiSPTree* wiRenderer::spTree_lights = nullptr;

Scene* wiRenderer::scene = nullptr;

unordered_set<Object*>			  wiRenderer::objectsWithTrails;
unordered_set<wiEmittedParticle*> wiRenderer::emitterSystems;

std::vector<Lines*>	wiRenderer::boneLines;
std::vector<Lines*>	wiRenderer::linesTemp;
std::vector<Cube>	wiRenderer::cubes;

std::vector<wiTranslator*> wiRenderer::renderableTranslators;
std::vector<pair<XMFLOAT4X4, XMFLOAT4>> wiRenderer::renderableBoxes;

std::unordered_map<Camera*, wiRenderer::FrameCulling> wiRenderer::frameCullings;

wiWaterPlane wiRenderer::waterPlane;

#pragma endregion


wiRenderer::wiRenderer()
{
}

void wiRenderer::Present(function<void()> drawToScreen1,function<void()> drawToScreen2,function<void()> drawToScreen3)
{
	GetDevice()->PresentBegin();
	
	if(drawToScreen1!=nullptr)
		drawToScreen1();
	if(drawToScreen2!=nullptr)
		drawToScreen2();
	if(drawToScreen3!=nullptr)
		drawToScreen3();

	GetDevice()->PresentEnd();

	OcclusionCulling_Read();

	*prevFrameCam = *cam;

	wiFrameRate::Frame();

}


void wiRenderer::CleanUp()
{
}

void wiRenderer::SetUpStaticComponents()
{
	for (int i = 0; i < VSTYPE_LAST; ++i)
	{
		SAFE_INIT(vertexShaders[i]);
	}
	for (int i = 0; i < PSTYPE_LAST; ++i)
	{
		SAFE_INIT(pixelShaders[i]);
	}
	for (int i = 0; i < GSTYPE_LAST; ++i)
	{
		SAFE_INIT(geometryShaders[i]);
	}
	for (int i = 0; i < HSTYPE_LAST; ++i)
	{
		SAFE_INIT(hullShaders[i]);
	}
	for (int i = 0; i < DSTYPE_LAST; ++i)
	{
		SAFE_INIT(domainShaders[i]);
	}
	for (int i = 0; i < CSTYPE_LAST; ++i)
	{
		SAFE_INIT(computeShaders[i]);
	}
	for (int i = 0; i < VLTYPE_LAST; ++i)
	{
		SAFE_INIT(vertexLayouts[i]);
	}
	for (int i = 0; i < RSTYPE_LAST; ++i)
	{
		SAFE_INIT(rasterizers[i]);
	}
	for (int i = 0; i < DSSTYPE_LAST; ++i)
	{
		SAFE_INIT(depthStencils[i]);
	}
	for (int i = 0; i < CBTYPE_LAST; ++i)
	{
		SAFE_INIT(constantBuffers[i]);
	}
	for (int i = 0; i < RBTYPE_LAST; ++i)
	{
		SAFE_INIT(resourceBuffers[i]);
	}
	for (int i = 0; i < TEXTYPE_LAST; ++i)
	{
		SAFE_INIT(textures[i]);
	}
	for (int i = 0; i < SSLOT_COUNT_PERSISTENT; ++i)
	{
		SAFE_INIT(samplers[i]);
	}
	for (int i = 0; i < SSTYPE_LAST; ++i)
	{
		SAFE_INIT(customsamplers[i]);
	}

	cam = new Camera();
	cam->SetUp((float)GetInternalResolution().x, (float)GetInternalResolution().y, 0.1f, 800);
	refCam = new Camera();
	refCam->SetUp((float)GetInternalResolution().x, (float)GetInternalResolution().y, 0.1f, 800);
	prevFrameCam = new Camera;
	

	wireRender=false;
	debugSpheres=false;

	SetUpStates();
	LoadBuffers();
	LoadShaders();
	
	wiHairParticle::SetUpStatic();
	wiEmittedParticle::SetUpStatic();

	GameSpeed=1;

	resetVertexCount();
	resetVisibleObjectCount();

	GetScene().wind=Wind();

	Cube::LoadStatic();

	spTree_lights=nullptr;


	waterRipples.resize(0);

	
	normalMapRT.Initialize(
		GetInternalResolution().x
		, GetInternalResolution().y
		,false, RTFormat_normalmaps
		);
	imagesRTAdd.Initialize(
		GetInternalResolution().x
		, GetInternalResolution().y
		,false
		);
	imagesRT.Initialize(
		GetInternalResolution().x
		, GetInternalResolution().y
		,false
		);

	SetShadowProps2D(SHADOWRES_2D, SHADOWCOUNT_2D, SOFTSHADOWQUALITY_2D);
	SetShadowPropsCube(SHADOWRES_CUBE, SHADOWCOUNT_CUBE);

	Material::CreateImpostorMaterialCB();
}
void wiRenderer::CleanUpStatic()
{

	wiHairParticle::CleanUpStatic();
	wiEmittedParticle::CleanUpStatic();
	Cube::CleanUpStatic();


	for (int i = 0; i < VSTYPE_LAST; ++i)
	{
		SAFE_DELETE(vertexShaders[i]);
	}
	for (int i = 0; i < PSTYPE_LAST; ++i)
	{
		SAFE_DELETE(pixelShaders[i]);
	}
	for (int i = 0; i < GSTYPE_LAST; ++i)
	{
		SAFE_DELETE(geometryShaders[i]);
	}
	for (int i = 0; i < HSTYPE_LAST; ++i)
	{
		SAFE_DELETE(hullShaders[i]);
	}
	for (int i = 0; i < DSTYPE_LAST; ++i)
	{
		SAFE_DELETE(domainShaders[i]);
	}
	for (int i = 0; i < CSTYPE_LAST; ++i)
	{
		SAFE_DELETE(computeShaders[i]);
	}
	for (int i = 0; i < VLTYPE_LAST; ++i)
	{
		SAFE_DELETE(vertexLayouts[i]);
	}
	for (int i = 0; i < RSTYPE_LAST; ++i)
	{
		SAFE_DELETE(rasterizers[i]);
	}
	for (int i = 0; i < DSSTYPE_LAST; ++i)
	{
		SAFE_DELETE(depthStencils[i]);
	}
	for (int i = 0; i < BSTYPE_LAST; ++i)
	{
		SAFE_DELETE(blendStates[i]);
	}
	for (int i = 0; i < CBTYPE_LAST; ++i)
	{
		SAFE_DELETE(constantBuffers[i]);
	}
	for (int i = 0; i < RBTYPE_LAST; ++i)
	{
		SAFE_DELETE(resourceBuffers[i]);
	}
	for (int i = 0; i < TEXTYPE_LAST; ++i)
	{
		SAFE_DELETE(textures[i]);
	}
	for (int i = 0; i < SSLOT_COUNT_PERSISTENT; ++i)
	{
		SAFE_DELETE(samplers[i]);
	}
	for (int i = 0; i < SSTYPE_LAST; ++i)
	{
		SAFE_DELETE(customsamplers[i]);
	}

	SAFE_DELETE(dynamicVertexBufferPool);

	if (physicsEngine) physicsEngine->CleanUp();

	SAFE_DELETE(graphicsDevice);
}
void wiRenderer::ClearWorld()
{
	emitterSystems.clear();
	
	if (physicsEngine)
		physicsEngine->ClearWorld();

	enviroMap = nullptr;
	colorGrading = nullptr;

	wiRenderer::resetVertexCount();
	
	for (wiSprite* x : images)
		x->CleanUp();
	images.clear();
	for (wiSprite* x : waterRipples)
		x->CleanUp();
	waterRipples.clear();

	SAFE_DELETE(spTree);
	SAFE_DELETE(spTree_lights);

	for (auto& x : frameCullings)
	{
		FrameCulling& culling = x.second;
		culling.Clear();
	}

	cam->detach();

	GetScene().ClearWorld();
}
XMVECTOR wiRenderer::GetSunPosition()
{
	for (Model* model : GetScene().models)
	{
		for (Light* l : model->lights)
			if (l->GetType() == Light::DIRECTIONAL)
				return -XMVector3Transform(XMVectorSet(0, -1, 0, 1), XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation)));
	}
	return XMVectorSet(0, 1, 0, 1);
}
XMFLOAT4 wiRenderer::GetSunColor()
{
	for (Model* model : GetScene().models)
	{
		for (Light* l : model->lights)
			if (l->GetType() == Light::DIRECTIONAL)
				return l->color;
	}
	return XMFLOAT4(1,1,1,1);
}
int wiRenderer::GetSunArrayIndex()
{
	for (Model* model : GetScene().models)
	{
		for (Light* l : model->lights)
			if (l->GetType() == Light::DIRECTIONAL)
				return l->entityArray_index;
	}
	return -1;
}
float wiRenderer::GetGameSpeed() { return GameSpeed*overrideGameSpeed; }

void wiRenderer::SetUpBoneLines()
{
	boneLines.clear();
	for (Model* model : GetScene().models)
	{
		//for (unsigned int i = 0; i < model->armatures.size(); i++) {
		//	for (unsigned int j = 0; j < model->armatures[i]->boneCollection.size(); j++) {
		//		boneLines.push_back(new Lines(model->armatures[i]->boneCollection[j]->length, XMFLOAT4A(1, 1, 1, 1), i, j));
		//	}
		//}
		int i = 0;
		for (auto& a : model->armatures)
		{
			int j = 0;
			for (auto& b : a->boneCollection)
			{
				boneLines.push_back(new Lines(b->length, XMFLOAT4A(1, 1, 1, 1), i, j));
				j++;
			}
			i++;
		}
	}
}
void wiRenderer::UpdateBoneLines()
{
	if (debugBoneLines)
	{
		for (unsigned int i = 0; i < boneLines.size(); i++) {
			int armatureI = boneLines[i]->parentArmature;
			int boneI = boneLines[i]->parentBone;

			int arm = 0;
			for (Model* model : GetScene().models)
			{
				for (Armature* armature : model->armatures)
				{
					if (arm == armatureI)
					{
						int bonI = 0;
						for (Bone* b : armature->boneCollection)
						{
							if (boneI == bonI)
							{
								boneLines[i]->Transform(b->world);
							}
							bonI++;
						}
					}
					arm++;
				}
			}
		}
	}
}
void iterateSPTree2(wiSPTree::Node* n, std::vector<Cube>& cubes, const XMFLOAT4A& col);
void iterateSPTree(wiSPTree::Node* n, std::vector<Cube>& cubes, const XMFLOAT4A& col){
	if(!n) return;
	if(n->count){
		for (unsigned int i = 0; i<n->children.size(); ++i)
			iterateSPTree(n->children[i],cubes,col);
	}
	if(!n->objects.empty()){
		cubes.push_back(Cube(n->box.getCenter(),n->box.getHalfWidth(),col));
		for(Cullable* object:n->objects){
			cubes.push_back(Cube(object->bounds.getCenter(),object->bounds.getHalfWidth(),XMFLOAT4A(1,0,0,1)));
			//Object* o = (Object*)object;
			//for(wiHairParticle& hps : o->hParticleSystems)
			//	iterateSPTree2(hps.spTree->root,cubes,XMFLOAT4A(0,1,0,1));
		}
	}
}
void iterateSPTree2(wiSPTree::Node* n, std::vector<Cube>& cubes, const XMFLOAT4A& col){
	if(!n) return;
	if(n->count){
		for (unsigned int i = 0; i<n->children.size(); ++i)
			iterateSPTree2(n->children[i],cubes,col);
	}
	if(!n->objects.empty()){
		cubes.push_back(Cube(n->box.getCenter(),n->box.getHalfWidth(),col));
	}
}
void wiRenderer::SetUpCubes(){
	/*if(debugBoxes){
		cubes.resize(0);
		iterateSPTree(spTree->root,cubes);
		for(Object* object:objects)
			cubes.push_back(Cube(XMFLOAT3(0,0,0),XMFLOAT3(1,1,1),XMFLOAT4A(1,0,0,1)));
	}*/
	cubes.clear();
}
void wiRenderer::UpdateCubes(){
	if(debugPartitionTree && spTree && spTree->root){
		/*int num=0;
		iterateSPTreeUpdate(spTree->root,cubes,num);
		for(Object* object:objects){
			AABB b=object->frameBB;
			XMFLOAT3 c = b.getCenter();
			XMFLOAT3 hw = b.getHalfWidth();
			cubes[num].Transform( XMMatrixScaling(hw.x,hw.y,hw.z) * XMMatrixTranslation(c.x,c.y,c.z) );
			num+=1;
		}*/
		cubes.clear();
		if(spTree) iterateSPTree(spTree->root,cubes,XMFLOAT4A(1,1,0,1));
		if(spTree_lights) iterateSPTree(spTree_lights->root,cubes,XMFLOAT4A(1,1,1,1));
	}
	//if(debugBoxes){
	//	for(Decal* decal : decals){
	//		cubes.push_back(Cube(decal->bounds.getCenter(),decal->bounds.getHalfWidth(),XMFLOAT4A(1,0,1,1)));
	//	}
	//}
}

bool wiRenderer::ResolutionChanged()
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


void wiRenderer::LoadBuffers()
{
	GPUBufferDesc bd;

	// Ring buffer allows fast allocation of dynamic buffers for one frame:
	dynamicVertexBufferPool = new GPURingBuffer;
	bd.BindFlags = BIND_VERTEX_BUFFER;
	bd.ByteWidth = 1024 * 1024 * 64;
	bd.Usage = USAGE_DYNAMIC;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	GetDevice()->CreateBuffer(&bd, nullptr, dynamicVertexBufferPool);


	for (int i = 0; i < CBTYPE_LAST; ++i)
	{
		constantBuffers[i] = new GPUBuffer;
	}

	ZeroMemory( &bd, sizeof(bd) );
	bd.BindFlags = BIND_CONSTANT_BUFFER;

	//Persistent buffers...

	// Per World Constant buffer will be updated occasionally, so it should reside in DEFAULT GPU memory!
	bd.CPUAccessFlags = 0;
	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WorldCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_WORLD]);

	bd.ByteWidth = sizeof(FrameCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_FRAME]);

	// The other constant buffers will be updated frequently (> per frame) so they should reside in DYNAMIC GPU memory!
	bd.Usage = USAGE_DYNAMIC;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;

	bd.ByteWidth = sizeof(CameraCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_CAMERA]);

	bd.ByteWidth = sizeof(MiscCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_MISC]);

	bd.ByteWidth = sizeof(APICB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_API]);


	// On demand buffers...

	bd.ByteWidth = sizeof(VolumeLightCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_VOLUMELIGHT]);

	bd.ByteWidth = sizeof(DecalCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_DECAL]);

	bd.ByteWidth = sizeof(CubeMapRenderCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_CUBEMAPRENDER]);

	bd.ByteWidth = sizeof(TessellationCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_TESSELLATION]);

	bd.ByteWidth = sizeof(DispatchParamsCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_DISPATCHPARAMS]);

	bd.ByteWidth = sizeof(CloudGeneratorCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_CLOUDGENERATOR]);





	// Resource Buffers:

	for (int i = 0; i < RBTYPE_LAST; ++i)
	{
		resourceBuffers[i] = new GPUBuffer;
	}

	bd.Usage = USAGE_DEFAULT;
	bd.CPUAccessFlags = 0;


	bd.ByteWidth = sizeof(ShaderEntityType) * MAX_SHADER_ENTITY_COUNT;
	bd.BindFlags = BIND_SHADER_RESOURCE;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(ShaderEntityType);
	GetDevice()->CreateBuffer(&bd, nullptr, resourceBuffers[RBTYPE_ENTITYARRAY]);

	bd.ByteWidth = sizeof(XMMATRIX) * MATRIXARRAY_COUNT;
	bd.BindFlags = BIND_SHADER_RESOURCE;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(XMMATRIX);
	GetDevice()->CreateBuffer(&bd, nullptr, resourceBuffers[RBTYPE_MATRIXARRAY]);

	SAFE_DELETE(resourceBuffers[RBTYPE_VOXELSCENE]); // lazy init on request
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
enum OBJECTRENDERING_TRANSPARENCY
{
	OBJECTRENDERING_TRANSPARENCY_DISABLED,
	OBJECTRENDERING_TRANSPARENCY_ENABLED,
	OBJECTRENDERING_TRANSPARENCY_COUNT
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
GraphicsPSO* PSO_object[SHADERTYPE_COUNT][OBJECTRENDERING_DOUBLESIDED_COUNT][OBJECTRENDERING_TESSELLATION_COUNT][OBJECTRENDERING_ALPHATEST_COUNT][OBJECTRENDERING_TRANSPARENCY_COUNT][OBJECTRENDERING_NORMALMAP_COUNT][OBJECTRENDERING_PLANARREFLECTION_COUNT][OBJECTRENDERING_POM_COUNT] = {};
GraphicsPSO* PSO_object_water[SHADERTYPE_COUNT] = {};
GraphicsPSO* PSO_object_wire = nullptr;
GraphicsPSO* GetObjectPSO(SHADERTYPE shaderType, bool doublesided, bool tessellation, Material* material, bool forceAlphaTestForDithering)
{
	if (wiRenderer::IsWireRender())
	{
		switch (shaderType)
		{
		case SHADERTYPE_TEXTURE:
		case SHADERTYPE_DEFERRED:
		case SHADERTYPE_FORWARD:
		case SHADERTYPE_TILEDFORWARD:
			return PSO_object_wire;
		}
		return nullptr;
	}

	if (material->IsWater())
	{
		return PSO_object_water[shaderType];
	}

	bool alphatest = material->IsAlphaTestEnabled() || forceAlphaTestForDithering;
	bool transparent = material->IsTransparent();
	bool normalmap = material->GetNormalMap() != nullptr;
	bool planarreflection = material->HasPlanarReflection();
	bool pom = material->parallaxOcclusionMapping > 0;

	return PSO_object[shaderType][doublesided][tessellation][alphatest][transparent][normalmap][planarreflection][pom];
}


VLTYPES GetVLTYPE(SHADERTYPE shaderType, bool tessellation, bool alphatest, bool transparent)
{
	VLTYPES realVL = VLTYPE_OBJECT_POS_TEX;

	switch (shaderType)
	{
	case SHADERTYPE_TEXTURE:
		if (tessellation)
		{
			realVL = VLTYPE_OBJECT_ALL;
		}
		else
		{
			realVL = VLTYPE_OBJECT_POS_TEX;
		}
		break;
	case SHADERTYPE_DEFERRED:
	case SHADERTYPE_FORWARD:
	case SHADERTYPE_TILEDFORWARD:
		realVL = VLTYPE_OBJECT_ALL;
		break;
	case SHADERTYPE_ENVMAPCAPTURE:
		realVL = VLTYPE_OBJECT_POS_TEX;
		break;
	case SHADERTYPE_DEPTHONLY:
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
	case SHADERTYPE_SHADOW:
	case SHADERTYPE_SHADOWCUBE:
		if (alphatest || transparent)
		{
			realVL = VLTYPE_OBJECT_POS_TEX;
		}
		else
		{
			realVL = VLTYPE_OBJECT_POS;
		}
		break;
	case SHADERTYPE_VOXELIZE:
		realVL = VLTYPE_OBJECT_POS_TEX;
		break;
	}

	return realVL;
}
VSTYPES GetVSTYPE(SHADERTYPE shaderType, bool tessellation, bool alphatest, bool transparent)
{
	VSTYPES realVS = VSTYPE_OBJECT_SIMPLE;

	switch (shaderType)
	{
	case SHADERTYPE_TEXTURE:
		if (tessellation)
		{
			realVS = VSTYPE_OBJECT_SIMPLE_TESSELLATION;
		}
		else
		{
			realVS = VSTYPE_OBJECT_SIMPLE;
		}
		break;
	case SHADERTYPE_DEFERRED:
	case SHADERTYPE_FORWARD:
	case SHADERTYPE_TILEDFORWARD:
		if (tessellation)
		{
			realVS = VSTYPE_OBJECT_COMMON_TESSELLATION;
		}
		else
		{
			realVS = VSTYPE_OBJECT_COMMON;
		}
		break;
	case SHADERTYPE_DEPTHONLY:
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
	case SHADERTYPE_ENVMAPCAPTURE:
		realVS = VSTYPE_ENVMAP;
		break;
	case SHADERTYPE_SHADOW:
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
	case SHADERTYPE_SHADOWCUBE:
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
	case SHADERTYPE_VOXELIZE:
		realVS = VSTYPE_VOXELIZER;
		break;
	}

	return realVS;
}
GSTYPES GetGSTYPE(SHADERTYPE shaderType, bool alphatest)
{
	GSTYPES realGS = GSTYPE_NULL;

	switch (shaderType)
	{
	case SHADERTYPE_TEXTURE:
		break;
	case SHADERTYPE_DEFERRED:
		break;
	case SHADERTYPE_FORWARD:
		break;
	case SHADERTYPE_TILEDFORWARD:
		break;
	case SHADERTYPE_DEPTHONLY:
		break;
	case SHADERTYPE_ENVMAPCAPTURE:
		realGS = GSTYPE_ENVMAP;
		break;
	case SHADERTYPE_SHADOW:
		break;
	case SHADERTYPE_SHADOWCUBE:
		if (alphatest)
		{
			realGS = GSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST;
		}
		else
		{
			realGS = GSTYPE_SHADOWCUBEMAPRENDER;
		}
		break;
	case SHADERTYPE_VOXELIZE:
		realGS = GSTYPE_VOXELIZER;
		break;
	}

	return realGS;
}
HSTYPES GetHSTYPE(SHADERTYPE shaderType, bool tessellation)
{
	switch (shaderType)
	{
	case SHADERTYPE_TEXTURE:
	case SHADERTYPE_DEPTHONLY:
	case SHADERTYPE_DEFERRED:
	case SHADERTYPE_FORWARD:
	case SHADERTYPE_TILEDFORWARD:
		return tessellation ? HSTYPE_OBJECT : HSTYPE_NULL;
		break;
	}

	return HSTYPE_NULL;
}
DSTYPES GetDSTYPE(SHADERTYPE shaderType, bool tessellation)
{
	switch (shaderType)
	{
	case SHADERTYPE_TEXTURE:
	case SHADERTYPE_DEPTHONLY:
	case SHADERTYPE_DEFERRED:
	case SHADERTYPE_FORWARD:
	case SHADERTYPE_TILEDFORWARD:
		return tessellation ? DSTYPE_OBJECT : DSTYPE_NULL;
		break;
	}

	return DSTYPE_NULL;
}
PSTYPES GetPSTYPE(SHADERTYPE shaderType, bool alphatest, bool transparent, bool normalmap, bool planarreflection, bool pom)
{
	PSTYPES realPS = PSTYPE_OBJECT_SIMPLEST;

	switch (shaderType)
	{
	case SHADERTYPE_DEFERRED:
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
		}
		break;
	case SHADERTYPE_FORWARD:
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
	case SHADERTYPE_TILEDFORWARD:
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
	case SHADERTYPE_SHADOW:
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
	case SHADERTYPE_SHADOWCUBE:
		if (alphatest)
		{
			realPS = PSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST;
		}
		else
		{
			realPS = PSTYPE_SHADOWCUBEMAPRENDER;
		}
		break;
	case SHADERTYPE_ENVMAPCAPTURE:
		realPS = PSTYPE_ENVMAP;
		break;
	case SHADERTYPE_DEPTHONLY:
		if (alphatest)
		{
			realPS = PSTYPE_OBJECT_ALPHATESTONLY;
		}
		else
		{
			realPS = PSTYPE_NULL;
		}
		break;
	case SHADERTYPE_VOXELIZE:
		realPS = PSTYPE_VOXELIZER;
		break;
	case SHADERTYPE_TEXTURE:
		realPS = PSTYPE_OBJECT_TEXTUREONLY;
		break;
	}

	return realPS;
}

GraphicsPSO* PSO_decal = nullptr;
GraphicsPSO* PSO_occlusionquery = nullptr;
GraphicsPSO* PSO_impostor[SHADERTYPE_COUNT] = {};
GraphicsPSO* PSO_captureimpostor = nullptr;
GraphicsPSO* GetImpostorPSO(SHADERTYPE shaderType)
{
	if (wiRenderer::IsWireRender())
	{
		switch (shaderType)
		{
		case SHADERTYPE_TEXTURE:
		case SHADERTYPE_DEFERRED:
		case SHADERTYPE_FORWARD:
		case SHADERTYPE_TILEDFORWARD:
			return PSO_object_wire;
		}
		return nullptr;
	}

	return PSO_impostor[shaderType];
}

GraphicsPSO* PSO_deferredlight[Light::LIGHTTYPE_COUNT] = {};
GraphicsPSO* PSO_lightvisualizer[Light::LIGHTTYPE_COUNT] = {};
GraphicsPSO* PSO_volumetriclight[Light::LIGHTTYPE_COUNT] = {};
GraphicsPSO* PSO_enviromentallight = nullptr;

enum SKYRENDERING
{
	SKYRENDERING_STATIC,
	SKYRENDERING_DYNAMIC,
	SKYRENDERING_SUN,
	SKYRENDERING_ENVMAPCAPTURE_STATIC,
	SKYRENDERING_ENVMAPCAPTURE_DYNAMIC,
	SKYRENDERING_COUNT
};
GraphicsPSO* PSO_sky[SKYRENDERING_COUNT] = {};

enum DEBUGRENDERING
{
	DEBUGRENDERING_TRANSLATOR_WIREPART,
	DEBUGRENDERING_TRANSLATOR_SOLIDPART,
	DEBUGRENDERING_ENVPROBE,
	DEBUGRENDERING_GRID,
	DEBUGRENDERING_CUBE,
	DEBUGRENDERING_LINES,
	DEBUGRENDERING_BONELINES,
	DEBUGRENDERING_EMITTER,
	DEBUGRENDERING_VOXEL,
	DEBUGRENDERING_FORCEFIELD_POINT,
	DEBUGRENDERING_FORCEFIELD_PLANE,
	DEBUGRENDERING_COUNT
};
GraphicsPSO* PSO_debug[DEBUGRENDERING_COUNT] = {};

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
ComputePSO* CPSO_tiledlighting[TILEDLIGHTING_TYPE_COUNT][TILEDLIGHTING_CULLING_COUNT][TILEDLIGHTING_DEBUG_COUNT] = {};
ComputePSO* CPSO[CSTYPE_LAST] = {};

void wiRenderer::LoadShaders()
{

	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_debug.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			vertexShaders[VSTYPE_OBJECT_DEBUG] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_OBJECT_DEBUG] = vsinfo->vertexLayout;
		}
	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",				0, Mesh::Vertex_TEX::FORMAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",				1, Mesh::Vertex_POS::FORMAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "MATI",			0, FORMAT_R32G32B32A32_FLOAT, 3, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			1, FORMAT_R32G32B32A32_FLOAT, 3, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			2, FORMAT_R32G32B32A32_FLOAT, 3, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR_DITHER",	0, FORMAT_R32G32B32A32_FLOAT, 3, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATIPREV",		0, FORMAT_R32G32B32A32_FLOAT, 3, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATIPREV",		1, FORMAT_R32G32B32A32_FLOAT, 3, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATIPREV",		2, FORMAT_R32G32B32A32_FLOAT, 3, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_common.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			vertexShaders[VSTYPE_OBJECT_COMMON] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_OBJECT_ALL] = vsinfo->vertexLayout;
		}
	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "MATI",			0, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			1, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			2, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR_DITHER",	0, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_positionstream.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			vertexShaders[VSTYPE_OBJECT_POSITIONSTREAM] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_OBJECT_POS] = vsinfo->vertexLayout;
		}
	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",				0, Mesh::Vertex_TEX::FORMAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "MATI",			0, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			1, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			2, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR_DITHER",	0, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_simple.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			vertexShaders[VSTYPE_OBJECT_SIMPLE] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_OBJECT_POS_TEX] = vsinfo->vertexLayout;
		}
	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "MATI",			0, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			1, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			2, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR_DITHER",	0, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			vertexShaders[VSTYPE_SHADOW] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_SHADOW_POS] = vsinfo->vertexLayout;
		}
	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION_NORMAL_WIND",	0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",				0, Mesh::Vertex_TEX::FORMAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "MATI",			0, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			1, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			2, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR_DITHER",	0, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowVS_alphatest.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			vertexShaders[VSTYPE_SHADOW_ALPHATEST] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_SHADOW_POS_TEX] = vsinfo->vertexLayout;
		}

		vertexShaders[VSTYPE_SHADOW_TRANSPARENT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowVS_transparent.cso", wiResourceManager::VERTEXSHADER))->vertexShader;

	}

	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);

		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "linesVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			vertexShaders[VSTYPE_LINE] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_LINE] = vsinfo->vertexLayout;
		}
	}

	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION", 0, FORMAT_R32G32B32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, FORMAT_R32G32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);

		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "trailVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			vertexShaders[VSTYPE_TRAIL] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_TRAIL] = vsinfo->vertexLayout;
		}
	}

	vertexShaders[VSTYPE_OBJECT_COMMON_TESSELLATION] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_common_tessellation.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_OBJECT_SIMPLE_TESSELLATION] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_simple_tessellation.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_DIRLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_POINTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "pointLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_SPOTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "spotLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_LIGHTVISUALIZER_SPOTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vSpotLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_LIGHTVISUALIZER_POINTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vPointLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_LIGHTVISUALIZER_SPHERELIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vSphereLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_LIGHTVISUALIZER_DISCLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vDiscLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_LIGHTVISUALIZER_RECTANGLELIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vRectangleLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_LIGHTVISUALIZER_TUBELIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vTubeLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_DECAL] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "decalVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_ENVMAP] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_ENVMAP_SKY] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMap_skyVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_SPHERE] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "sphereVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_CUBE] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_SHADOWCUBEMAPRENDER] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowVS_alphatest.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_SKY] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_WATER] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "waterVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOXELIZER] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_voxelizer.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOXEL] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_FORCEFIELDVISUALIZER_POINT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "forceFieldPointVisualizerVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_FORCEFIELDVISUALIZER_PLANE] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "forceFieldPlaneVisualizerVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;


	pixelShaders[PSTYPE_OBJECT_DEFERRED] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_deferred.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_DEFERRED_NORMALMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_deferred_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_DEFERRED_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_deferred_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_DEFERRED_NORMALMAP_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_deferred_normalmap_pom.cso", wiResourceManager::PIXELSHADER));

	pixelShaders[PSTYPE_OBJECT_FORWARD] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_NORMALMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_transparent.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT_NORMALMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_transparent_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_NORMALMAP_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_transparent_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT_NORMALMAP_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_transparent_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_NORMALMAP_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_transparent_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_TRANSPARENT_NORMALMAP_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_transparent_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_WATER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_water.cso", wiResourceManager::PIXELSHADER));

	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_transparent.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_transparent_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_transparent_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_transparent_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_transparent_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_transparent_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_WATER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_tiledforward_water.cso", wiResourceManager::PIXELSHADER));

	pixelShaders[PSTYPE_OBJECT_HOLOGRAM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_hologram.cso", wiResourceManager::PIXELSHADER));


	pixelShaders[PSTYPE_OBJECT_DEBUG] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_debug.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_SIMPLEST] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_simplest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_BLACKOUT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_blackout.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TEXTUREONLY] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_textureonly.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_ALPHATESTONLY] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_alphatestonly.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVIRONMENTALLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "environmentalLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DIRLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_POINTLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "pointLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SPOTLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "spotLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SPHERELIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "sphereLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DISCLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "discLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_RECTANGLELIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "rectangleLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_TUBELIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "tubeLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_LIGHTVISUALIZER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightVisualizerPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOLUMETRICLIGHT_DIRECTIONAL] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "volumetricLight_DirectionalPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOLUMETRICLIGHT_POINT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "volumetricLight_PointPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOLUMETRICLIGHT_SPOT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "volumetricLight_SpotPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DECAL] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "decalPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP_SKY_STATIC] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMap_skyPS_static.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP_SKY_DYNAMIC] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMap_skyPS_dynamic.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_CAPTUREIMPOSTOR] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "captureImpostorPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_CUBEMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubemapPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_LINE] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "linesPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SKY_STATIC] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyPS_static.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SKY_DYNAMIC] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyPS_dynamic.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SUN] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "sunPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOW_ALPHATEST] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowPS_alphatest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOW_TRANSPARENT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowPS_transparent.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOW_WATER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowPS_water.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowPS_alphatest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_TRAIL] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "trailPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOXELIZER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_voxelizer.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOXEL] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_FORCEFIELDVISUALIZER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "forceFieldVisualizerPS.cso", wiResourceManager::PIXELSHADER));

	geometryShaders[GSTYPE_ENVMAP] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_ENVMAP_SKY] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMap_skyGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_SHADOWCUBEMAPRENDER] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowGS_alphatest.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_VOXELIZER] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectGS_voxelizer.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_VOXEL] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelGS.cso", wiResourceManager::GEOMETRYSHADER));


	computeShaders[CSTYPE_LUMINANCE_PASS1] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "luminancePass1CS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_LUMINANCE_PASS2] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "luminancePass2CS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEFRUSTUMS] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "tileFrustumsCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_RESOLVEMSAADEPTHSTENCIL] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "resolveMSAADepthStencilCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELSCENECOPYCLEAR] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelSceneCopyClearCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELSCENECOPYCLEAR_TEMPORALSMOOTHING] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelSceneCopyClear_TemporalSmoothing.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELRADIANCESECONDARYBOUNCE] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelRadianceSecondaryBounceCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELCLEARONLYNORMAL] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelClearOnlyNormalCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN2D_SIMPLEFILTER] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "generateMIPChain2D_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN2D_GAUSSIAN] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "generateMIPChain2D_GaussianCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN3D_SIMPLEFILTER] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "generateMIPChain3D_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN3D_GAUSSIAN] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "generateMIPChain3D_GaussianCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_SKINNING] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skinningCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_SKINNING_LDS] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skinningCS_LDS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_CLOUDGENERATOR] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cloudGeneratorCS.cso", wiResourceManager::COMPUTESHADER));


	hullShaders[HSTYPE_OBJECT] = static_cast<HullShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectHS.cso", wiResourceManager::HULLSHADER));


	domainShaders[DSTYPE_OBJECT] = static_cast<DomainShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectDS.cso", wiResourceManager::DOMAINSHADER));


	GraphicsDevice* device = GetDevice();

	vector<thread> thread_pool(0);

	thread_pool.push_back(thread([&] {
		// default objectshaders:
		for (int shaderType = 0; shaderType < SHADERTYPE_COUNT; ++shaderType)
		{
			for (int doublesided = 0; doublesided < OBJECTRENDERING_DOUBLESIDED_COUNT; ++doublesided)
			{
				for (int tessellation = 0; tessellation < OBJECTRENDERING_TESSELLATION_COUNT; ++tessellation)
				{
					for (int alphatest = 0; alphatest < OBJECTRENDERING_ALPHATEST_COUNT; ++alphatest)
					{
						for (int transparency = 0; transparency < OBJECTRENDERING_TRANSPARENCY_COUNT; ++transparency)
						{
							for (int normalmap = 0; normalmap < OBJECTRENDERING_NORMALMAP_COUNT; ++normalmap)
							{
								for (int planarreflection = 0; planarreflection < OBJECTRENDERING_PLANARREFLECTION_COUNT; ++planarreflection)
								{
									for (int pom = 0; pom < OBJECTRENDERING_POM_COUNT; ++pom)
									{
										VSTYPES realVS = GetVSTYPE((SHADERTYPE)shaderType, tessellation, alphatest, transparency);
										VLTYPES realVL = GetVLTYPE((SHADERTYPE)shaderType, tessellation, alphatest, transparency);
										HSTYPES realHS = GetHSTYPE((SHADERTYPE)shaderType, tessellation);
										DSTYPES realDS = GetDSTYPE((SHADERTYPE)shaderType, tessellation);
										GSTYPES realGS = GetGSTYPE((SHADERTYPE)shaderType, alphatest);
										PSTYPES realPS = GetPSTYPE((SHADERTYPE)shaderType, alphatest, transparency, normalmap, planarreflection, pom);

										if (tessellation && (realHS == HSTYPE_NULL || realDS == DSTYPE_NULL))
										{
											continue;
										}

										GraphicsPSODesc desc;
										desc.vs = vertexShaders[realVS];
										desc.il = vertexLayouts[realVL];
										desc.hs = hullShaders[realHS];
										desc.ds = domainShaders[realDS];
										desc.gs = geometryShaders[realGS];
										desc.ps = pixelShaders[realPS];

										switch (shaderType)
										{
										case SHADERTYPE_DEPTHONLY:
										case SHADERTYPE_SHADOW:
										case SHADERTYPE_SHADOWCUBE:
											desc.bs = blendStates[transparency ? BSTYPE_TRANSPARENTSHADOWMAP : BSTYPE_COLORWRITEDISABLE];
											break;
										default:
											desc.bs = blendStates[transparency ? BSTYPE_TRANSPARENT : BSTYPE_OPAQUE];
											break;
										}

										switch (shaderType)
										{
										case SHADERTYPE_SHADOW:
										case SHADERTYPE_SHADOWCUBE:
											desc.dss = depthStencils[transparency ? DSSTYPE_DEPTHREAD : DSSTYPE_SHADOW];
											break;
										case SHADERTYPE_TILEDFORWARD:
											desc.dss = depthStencils[transparency ? DSSTYPE_DEFAULT : DSSTYPE_DEPTHREADEQUAL];
											break;
										case SHADERTYPE_ENVMAPCAPTURE:
											desc.dss = depthStencils[DSSTYPE_ENVMAP];
											break;
										case SHADERTYPE_VOXELIZE:
											desc.dss = depthStencils[DSSTYPE_XRAY];
											break;
										default:
											desc.dss = depthStencils[DSSTYPE_DEFAULT];
											break;
										}

										switch (shaderType)
										{
										case SHADERTYPE_SHADOW:
										case SHADERTYPE_SHADOWCUBE:
											desc.rs = rasterizers[doublesided ? RSTYPE_SHADOW_DOUBLESIDED : RSTYPE_SHADOW];
											break;
										case SHADERTYPE_VOXELIZE:
											desc.rs = rasterizers[RSTYPE_VOXELIZE];
											break;
										default:
											desc.rs = rasterizers[doublesided ? RSTYPE_DOUBLESIDED : RSTYPE_FRONT];
											break;
										}

										switch (shaderType)
										{
										case SHADERTYPE_TEXTURE:
											desc.numRTs = 1;
											desc.RTFormats[0] = RTFormat_hdr;
											desc.DSFormat = DSFormat_full;
											break;
										case SHADERTYPE_DEFERRED:
											desc.numRTs = 4;
											desc.RTFormats[0] = RTFormat_gbuffer_0;
											desc.RTFormats[1] = RTFormat_gbuffer_1;
											desc.RTFormats[2] = RTFormat_gbuffer_2;
											desc.RTFormats[3] = RTFormat_gbuffer_3;
											desc.DSFormat = DSFormat_full;
											break;
										case SHADERTYPE_FORWARD:
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
										case SHADERTYPE_TILEDFORWARD:
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
										case SHADERTYPE_DEPTHONLY:
											desc.numRTs = 0;
											desc.DSFormat = DSFormat_full;
											break;
										case SHADERTYPE_ENVMAPCAPTURE:
											desc.numRTs = 1;
											desc.RTFormats[0] = RTFormat_envprobe;
											desc.DSFormat = DSFormat_small;
											break;
										case SHADERTYPE_SHADOW:
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
										case SHADERTYPE_SHADOWCUBE:
											desc.numRTs = 0;
											desc.DSFormat = DSFormat_small;
											break;
										case SHADERTYPE_VOXELIZE:
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

										RECREATE(PSO_object[shaderType][doublesided][tessellation][alphatest][transparency][normalmap][planarreflection][pom]);
										device->CreateGraphicsPSO(&desc, PSO_object[shaderType][doublesided][tessellation][alphatest][transparency][normalmap][planarreflection][pom]);
									}
								}
							}
						}
					}
				}
			}
		}

		// Custom objectshader presets:
		for (auto& x : Material::customShaderPresets)
		{
			SAFE_DELETE(x);
		}
		Material::customShaderPresets.clear();

		// Hologram:
		{
			VSTYPES realVS = GetVSTYPE(SHADERTYPE_FORWARD, false, false, true);
			VLTYPES realVL = GetVLTYPE(SHADERTYPE_FORWARD, false, false, true);

			GraphicsPSODesc desc;
			desc.vs = vertexShaders[realVS];
			desc.il = vertexLayouts[realVL];
			desc.ps = pixelShaders[PSTYPE_OBJECT_HOLOGRAM];

			desc.bs = blendStates[BSTYPE_ADDITIVE];
			desc.rs = rasterizers[DSSTYPE_DEFAULT];
			desc.dss = depthStencils[DSSTYPE_DEPTHREAD];
			desc.pt = TRIANGLELIST;

			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.DSFormat = DSFormat_full;

			Material::CustomShader* customShader = new Material::CustomShader;
			customShader->name = "Hologram";
			customShader->passes[SHADERTYPE_FORWARD].pso = new GraphicsPSO;
			device->CreateGraphicsPSO(&desc, customShader->passes[SHADERTYPE_FORWARD].pso);
			customShader->passes[SHADERTYPE_TILEDFORWARD].pso = new GraphicsPSO;
			device->CreateGraphicsPSO(&desc, customShader->passes[SHADERTYPE_TILEDFORWARD].pso);
			Material::customShaderPresets.push_back(customShader);
		}
	}));

	thread_pool.push_back(thread([&] {
		{
			GraphicsPSODesc desc;
			desc.vs = vertexShaders[VSTYPE_WATER];
			desc.rs = rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = blendStates[BSTYPE_TRANSPARENT];
			desc.dss = depthStencils[DSSTYPE_DEFAULT];
			desc.il = vertexLayouts[VLTYPE_OBJECT_POS_TEX];

			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.DSFormat = DSFormat_full;

			desc.ps = pixelShaders[PSTYPE_OBJECT_FORWARD_WATER];
			RECREATE(PSO_object_water[SHADERTYPE_FORWARD]);
			device->CreateGraphicsPSO(&desc, PSO_object_water[SHADERTYPE_FORWARD]);

			desc.ps = pixelShaders[PSTYPE_OBJECT_TILEDFORWARD_WATER];
			RECREATE(PSO_object_water[SHADERTYPE_TILEDFORWARD]);
			device->CreateGraphicsPSO(&desc, PSO_object_water[SHADERTYPE_TILEDFORWARD]);

			desc.dss = depthStencils[DSSTYPE_DEPTHREAD];
			desc.rs = rasterizers[RSTYPE_SHADOW];
			desc.bs = blendStates[BSTYPE_TRANSPARENTSHADOWMAP];
			desc.vs = vertexShaders[VSTYPE_SHADOW_TRANSPARENT];
			desc.ps = pixelShaders[PSTYPE_SHADOW_WATER];
			RECREATE(PSO_object_water[SHADERTYPE_SHADOW]);
			device->CreateGraphicsPSO(&desc, PSO_object_water[SHADERTYPE_SHADOW]);
		}
		{
			GraphicsPSODesc desc;
			desc.vs = vertexShaders[VSTYPE_OBJECT_SIMPLE];
			desc.ps = pixelShaders[PSTYPE_OBJECT_SIMPLEST];
			desc.rs = rasterizers[RSTYPE_WIRE];
			desc.bs = blendStates[BSTYPE_OPAQUE];
			desc.dss = depthStencils[DSSTYPE_DEFAULT];
			desc.il = vertexLayouts[VLTYPE_OBJECT_POS_TEX];

			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.DSFormat = DSFormat_full;

			RECREATE(PSO_object_wire);
			device->CreateGraphicsPSO(&desc, PSO_object_wire);
		}
		{
			GraphicsPSODesc desc;
			desc.vs = vertexShaders[VSTYPE_DECAL];
			desc.ps = pixelShaders[PSTYPE_DECAL];
			desc.rs = rasterizers[RSTYPE_FRONT];
			desc.bs = blendStates[BSTYPE_DECAL];
			desc.dss = depthStencils[DSSTYPE_DECAL];

			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_gbuffer_0;
			//desc.RTFormats[1] = RTFormat_gbuffer_1;

			RECREATE(PSO_decal);
			device->CreateGraphicsPSO(&desc, PSO_decal);
		}
		{
			GraphicsPSODesc desc;
			desc.vs = vertexShaders[VSTYPE_CUBE];
			desc.rs = rasterizers[RSTYPE_OCCLUDEE];
			desc.bs = blendStates[BSTYPE_COLORWRITEDISABLE];
			desc.dss = depthStencils[DSSTYPE_DEPTHREAD];
			desc.pt = TRIANGLESTRIP;

			desc.DSFormat = DSFormat_small;

			RECREATE(PSO_occlusionquery);
			device->CreateGraphicsPSO(&desc, PSO_occlusionquery);
		}
		for (int shaderType = 0; shaderType < SHADERTYPE_COUNT; ++shaderType)
		{
			const bool impostorRequest =
				shaderType != SHADERTYPE_VOXELIZE &&
				shaderType != SHADERTYPE_SHADOW &&
				shaderType != SHADERTYPE_SHADOWCUBE &&
				shaderType != SHADERTYPE_ENVMAPCAPTURE;
			if (!impostorRequest)
			{
				continue;
			}

			GraphicsPSODesc desc;
			desc.rs = rasterizers[RSTYPE_FRONT];
			desc.bs = blendStates[BSTYPE_OPAQUE];
			desc.dss = depthStencils[shaderType == SHADERTYPE_TILEDFORWARD ? DSSTYPE_DEPTHREADEQUAL : DSSTYPE_DEFAULT];

			VLTYPES realVL = (shaderType == SHADERTYPE_DEPTHONLY || shaderType == SHADERTYPE_TEXTURE) ? VLTYPE_OBJECT_POS_TEX : VLTYPE_OBJECT_ALL;
			VSTYPES realVS = realVL == VLTYPE_OBJECT_POS_TEX ? VSTYPE_OBJECT_SIMPLE : VSTYPE_OBJECT_COMMON;
			PSTYPES realPS;
			switch (shaderType)
			{
			case SHADERTYPE_DEFERRED:
				realPS = PSTYPE_OBJECT_DEFERRED_NORMALMAP;
				desc.numRTs = 4;
				desc.RTFormats[0] = RTFormat_gbuffer_0;
				desc.RTFormats[1] = RTFormat_gbuffer_1;
				desc.RTFormats[2] = RTFormat_gbuffer_2;
				desc.RTFormats[3] = RTFormat_gbuffer_3;
				break;
			case SHADERTYPE_FORWARD:
				realPS = PSTYPE_OBJECT_FORWARD_NORMALMAP;
				desc.numRTs = 2;
				desc.RTFormats[0] = RTFormat_hdr;
				desc.RTFormats[1] = RTFormat_gbuffer_1;
				break;
			case SHADERTYPE_TILEDFORWARD:
				realPS = PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP;
				desc.numRTs = 2;
				desc.RTFormats[0] = RTFormat_hdr;
				desc.RTFormats[1] = RTFormat_gbuffer_1;
				break;
			case SHADERTYPE_DEPTHONLY:
				realPS = PSTYPE_OBJECT_ALPHATESTONLY;
				break;
			default:
				realPS = PSTYPE_OBJECT_TEXTUREONLY;
				desc.numRTs = 1;
				desc.RTFormats[0] = RTFormat_hdr;
				break;
			}
			desc.DSFormat = DSFormat_full;

			desc.vs = vertexShaders[realVS];
			desc.il = vertexLayouts[realVL];
			desc.ps = pixelShaders[realPS];

			RECREATE(PSO_impostor[shaderType]);
			device->CreateGraphicsPSO(&desc, PSO_impostor[shaderType]);
		}
		{
			GraphicsPSODesc desc;
			desc.vs = vertexShaders[VSTYPE_OBJECT_COMMON];
			desc.ps = pixelShaders[PSTYPE_CAPTUREIMPOSTOR];
			desc.rs = rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = blendStates[BSTYPE_OPAQUE];
			desc.dss = depthStencils[DSSTYPE_DEFAULT];
			desc.il = vertexLayouts[VLTYPE_OBJECT_ALL];

			desc.numRTs = 3;
			desc.RTFormats[0] = RTFormat_impostor_albedo;
			desc.RTFormats[1] = RTFormat_impostor_normal;
			desc.RTFormats[2] = RTFormat_impostor_surface;
			desc.DSFormat = DSFormat_full;

			RECREATE(PSO_captureimpostor);
			device->CreateGraphicsPSO(&desc, PSO_captureimpostor);
		}
	}));

	thread_pool.push_back(thread([&] {
		for (int type = 0; type < Light::LIGHTTYPE_COUNT; ++type)
		{
			GraphicsPSODesc desc;

			// deferred lights:

			desc.pt = TRIANGLELIST;
			desc.rs = rasterizers[RSTYPE_BACK];
			desc.bs = blendStates[BSTYPE_DEFERREDLIGHT];

			switch (type)
			{
			case Light::DIRECTIONAL:
				desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
				desc.ps = pixelShaders[PSTYPE_DIRLIGHT];
				desc.dss = depthStencils[DSSTYPE_DIRLIGHT];
				break;
			case Light::POINT:
				desc.vs = vertexShaders[VSTYPE_POINTLIGHT];
				desc.ps = pixelShaders[PSTYPE_POINTLIGHT];
				desc.dss = depthStencils[DSSTYPE_LIGHT];
				break;
			case Light::SPOT:
				desc.vs = vertexShaders[VSTYPE_SPOTLIGHT];
				desc.ps = pixelShaders[PSTYPE_SPOTLIGHT];
				desc.dss = depthStencils[DSSTYPE_LIGHT];
				break;
			case Light::SPHERE:
				desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
				desc.ps = pixelShaders[PSTYPE_SPHERELIGHT];
				desc.dss = depthStencils[DSSTYPE_DIRLIGHT];
				break;
			case Light::DISC:
				desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
				desc.ps = pixelShaders[PSTYPE_DISCLIGHT];
				desc.dss = depthStencils[DSSTYPE_DIRLIGHT];
				break;
			case Light::RECTANGLE:
				desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
				desc.ps = pixelShaders[PSTYPE_RECTANGLELIGHT];
				desc.dss = depthStencils[DSSTYPE_DIRLIGHT];
				break;
			case Light::TUBE:
				desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
				desc.ps = pixelShaders[PSTYPE_TUBELIGHT];
				desc.dss = depthStencils[DSSTYPE_DIRLIGHT];
				break;
			}

			desc.numRTs = 2;
			desc.RTFormats[0] = RTFormat_deferred_lightbuffer;
			desc.RTFormats[1] = RTFormat_deferred_lightbuffer;
			desc.DSFormat = DSFormat_full;

			RECREATE(PSO_deferredlight[type]);
			device->CreateGraphicsPSO(&desc, PSO_deferredlight[type]);



			// light visualizers:
			if (type != Light::DIRECTIONAL)
			{

				desc.dss = depthStencils[DSSTYPE_DEPTHREAD];
				desc.ps = pixelShaders[PSTYPE_LIGHTVISUALIZER];

				switch (type)
				{
				case Light::POINT:
					desc.bs = blendStates[BSTYPE_ADDITIVE];
					desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_POINTLIGHT];
					desc.rs = rasterizers[RSTYPE_FRONT];
					break;
				case Light::SPOT:
					desc.bs = blendStates[BSTYPE_ADDITIVE];
					desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_SPOTLIGHT];
					desc.rs = rasterizers[RSTYPE_DOUBLESIDED];
					break;
				case Light::SPHERE:
					desc.bs = blendStates[BSTYPE_OPAQUE];
					desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_SPHERELIGHT];
					desc.rs = rasterizers[RSTYPE_FRONT];
					break;
				case Light::DISC:
					desc.bs = blendStates[BSTYPE_OPAQUE];
					desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_DISCLIGHT];
					desc.rs = rasterizers[RSTYPE_FRONT];
					break;
				case Light::RECTANGLE:
					desc.bs = blendStates[BSTYPE_OPAQUE];
					desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_RECTANGLELIGHT];
					desc.rs = rasterizers[RSTYPE_BACK];
					break;
				case Light::TUBE:
					desc.bs = blendStates[BSTYPE_OPAQUE];
					desc.vs = vertexShaders[VSTYPE_LIGHTVISUALIZER_TUBELIGHT];
					desc.rs = rasterizers[RSTYPE_FRONT];
					break;
				}

				desc.numRTs = 1;
				desc.RTFormats[0] = RTFormat_hdr;
				desc.DSFormat = DSFormat_full;

				RECREATE(PSO_lightvisualizer[type]);
				device->CreateGraphicsPSO(&desc, PSO_lightvisualizer[type]);
			}


			// volumetric lights:
			if (type <= Light::SPOT)
			{
				desc.dss = depthStencils[DSSTYPE_XRAY];
				desc.bs = blendStates[BSTYPE_ADDITIVE];
				desc.rs = rasterizers[RSTYPE_BACK];

				switch (type)
				{
				case Light::DIRECTIONAL:
					desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
					desc.ps = pixelShaders[PSTYPE_VOLUMETRICLIGHT_DIRECTIONAL];
					break;
				case Light::POINT:
					desc.vs = vertexShaders[VSTYPE_POINTLIGHT];
					desc.ps = pixelShaders[PSTYPE_VOLUMETRICLIGHT_POINT];
					break;
				case Light::SPOT:
					desc.vs = vertexShaders[VSTYPE_SPOTLIGHT];
					desc.ps = pixelShaders[PSTYPE_VOLUMETRICLIGHT_SPOT];
					break;
				}

				desc.numRTs = 1;
				desc.RTFormats[0] = RTFormat_hdr;
				desc.DSFormat = FORMAT_UNKNOWN;

				RECREATE(PSO_volumetriclight[type]);
				device->CreateGraphicsPSO(&desc, PSO_volumetriclight[type]);
			}


		}
		{
			GraphicsPSODesc desc;
			desc.vs = vertexShaders[VSTYPE_DIRLIGHT];
			desc.ps = pixelShaders[PSTYPE_ENVIRONMENTALLIGHT];
			desc.rs = rasterizers[RSTYPE_BACK];
			desc.bs = blendStates[BSTYPE_ENVIRONMENTALLIGHT];
			desc.dss = depthStencils[DSSTYPE_DIRLIGHT];

			desc.numRTs = 2;
			desc.RTFormats[0] = RTFormat_deferred_lightbuffer;
			desc.RTFormats[1] = RTFormat_deferred_lightbuffer;
			desc.DSFormat = DSFormat_full;

			RECREATE(PSO_enviromentallight);
			device->CreateGraphicsPSO(&desc, PSO_enviromentallight);
		}
		for (int type = 0; type < SKYRENDERING_COUNT; ++type)
		{
			GraphicsPSODesc desc;
			desc.rs = rasterizers[RSTYPE_SKY];
			desc.dss = depthStencils[DSSTYPE_DEPTHREAD];

			switch (type)
			{
			case SKYRENDERING_STATIC:
				desc.bs = blendStates[BSTYPE_OPAQUE];
				desc.vs = vertexShaders[VSTYPE_SKY];
				desc.ps = pixelShaders[PSTYPE_SKY_STATIC];
				desc.numRTs = 2;
				desc.RTFormats[0] = RTFormat_hdr;
				desc.RTFormats[1] = RTFormat_gbuffer_1;
				desc.DSFormat = DSFormat_full;
				break;
			case SKYRENDERING_DYNAMIC:
				desc.bs = blendStates[BSTYPE_OPAQUE];
				desc.vs = vertexShaders[VSTYPE_SKY];
				desc.ps = pixelShaders[PSTYPE_SKY_DYNAMIC];
				desc.numRTs = 2;
				desc.RTFormats[0] = RTFormat_hdr;
				desc.RTFormats[1] = RTFormat_gbuffer_1;
				desc.DSFormat = DSFormat_full;
				break;
			case SKYRENDERING_SUN:
				desc.bs = blendStates[BSTYPE_ADDITIVE];
				desc.vs = vertexShaders[VSTYPE_SKY];
				desc.ps = pixelShaders[PSTYPE_SUN];
				desc.numRTs = 1;
				desc.RTFormats[0] = RTFormat_hdr;
				desc.DSFormat = DSFormat_full;
				break;
			case SKYRENDERING_ENVMAPCAPTURE_STATIC:
				desc.bs = blendStates[BSTYPE_OPAQUE];
				desc.vs = vertexShaders[VSTYPE_ENVMAP_SKY];
				desc.ps = pixelShaders[PSTYPE_ENVMAP_SKY_STATIC];
				desc.gs = geometryShaders[GSTYPE_ENVMAP_SKY];
				desc.numRTs = 1;
				desc.RTFormats[0] = RTFormat_hdr;
				desc.DSFormat = DSFormat_small;
				break;
			case SKYRENDERING_ENVMAPCAPTURE_DYNAMIC:
				desc.bs = blendStates[BSTYPE_OPAQUE];
				desc.vs = vertexShaders[VSTYPE_ENVMAP_SKY];
				desc.ps = pixelShaders[PSTYPE_ENVMAP_SKY_DYNAMIC];
				desc.gs = geometryShaders[GSTYPE_ENVMAP_SKY];
				desc.numRTs = 1;
				desc.RTFormats[0] = RTFormat_hdr;
				desc.DSFormat = DSFormat_small;
				break;
			}

			RECREATE(PSO_sky[type]);
			device->CreateGraphicsPSO(&desc, PSO_sky[type]);
		}
		for (int debug = 0; debug < DEBUGRENDERING_COUNT; ++debug)
		{
			GraphicsPSODesc desc;

			switch (debug)
			{
			case DEBUGRENDERING_TRANSLATOR_WIREPART:
				desc.vs = vertexShaders[VSTYPE_LINE];
				desc.ps = pixelShaders[PSTYPE_LINE];
				desc.il = vertexLayouts[VLTYPE_LINE];
				desc.dss = depthStencils[DSSTYPE_XRAY];
				desc.rs = rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = blendStates[BSTYPE_TRANSPARENT];
				desc.pt = LINELIST;
				break;
			case DEBUGRENDERING_TRANSLATOR_SOLIDPART:
				desc.vs = vertexShaders[VSTYPE_LINE];
				desc.ps = pixelShaders[PSTYPE_LINE];
				desc.il = vertexLayouts[VLTYPE_LINE];
				desc.dss = depthStencils[DSSTYPE_XRAY];
				desc.rs = rasterizers[RSTYPE_DOUBLESIDED];
				desc.bs = blendStates[BSTYPE_ADDITIVE];
				desc.pt = TRIANGLELIST;
				break;
			case DEBUGRENDERING_ENVPROBE:
				desc.vs = vertexShaders[VSTYPE_SPHERE];
				desc.ps = pixelShaders[PSTYPE_CUBEMAP];
				desc.dss = depthStencils[DSSTYPE_DEFAULT];
				desc.rs = rasterizers[RSTYPE_FRONT];
				desc.bs = blendStates[BSTYPE_OPAQUE];
				desc.pt = TRIANGLELIST;
				break;
			case DEBUGRENDERING_GRID:
				desc.vs = vertexShaders[VSTYPE_LINE];
				desc.ps = pixelShaders[PSTYPE_LINE];
				desc.il = vertexLayouts[VLTYPE_LINE];
				desc.dss = depthStencils[DSSTYPE_DEPTHREAD];
				desc.rs = rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = blendStates[BSTYPE_TRANSPARENT];
				desc.pt = LINELIST;
				break;
			case DEBUGRENDERING_CUBE:
				desc.vs = vertexShaders[VSTYPE_LINE];
				desc.ps = pixelShaders[PSTYPE_LINE];
				desc.il = vertexLayouts[VLTYPE_LINE];
				desc.dss = depthStencils[DSSTYPE_DEPTHREAD];
				desc.rs = rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = blendStates[BSTYPE_TRANSPARENT];
				desc.pt = LINELIST;
				break;
			case DEBUGRENDERING_LINES:
				desc.vs = vertexShaders[VSTYPE_LINE];
				desc.ps = pixelShaders[PSTYPE_LINE];
				desc.il = vertexLayouts[VLTYPE_LINE];
				desc.dss = depthStencils[DSSTYPE_XRAY];
				desc.rs = rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = blendStates[BSTYPE_TRANSPARENT];
				desc.pt = LINELIST;
				break;
			case DEBUGRENDERING_BONELINES:
				desc.vs = vertexShaders[VSTYPE_LINE];
				desc.ps = pixelShaders[PSTYPE_LINE];
				desc.il = vertexLayouts[VLTYPE_LINE];
				desc.dss = depthStencils[DSSTYPE_XRAY];
				desc.rs = rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = blendStates[BSTYPE_TRANSPARENT];
				desc.pt = LINELIST;
				break;
			case DEBUGRENDERING_EMITTER:
				desc.vs = vertexShaders[VSTYPE_OBJECT_DEBUG];
				desc.ps = pixelShaders[PSTYPE_OBJECT_DEBUG];
				desc.il = vertexLayouts[VLTYPE_OBJECT_DEBUG];
				desc.dss = depthStencils[DSSTYPE_DEPTHREAD];
				desc.rs = rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = blendStates[BSTYPE_OPAQUE];
				desc.pt = TRIANGLELIST;
				break;
			case DEBUGRENDERING_VOXEL:
				desc.vs = vertexShaders[VSTYPE_VOXEL];
				desc.ps = pixelShaders[PSTYPE_VOXEL];
				desc.gs = geometryShaders[GSTYPE_VOXEL];
				desc.dss = depthStencils[DSSTYPE_DEFAULT];
				desc.rs = rasterizers[RSTYPE_BACK];
				desc.bs = blendStates[BSTYPE_OPAQUE];
				desc.pt = POINTLIST;
				break;
			case DEBUGRENDERING_FORCEFIELD_POINT:
				desc.vs = vertexShaders[VSTYPE_FORCEFIELDVISUALIZER_POINT];
				desc.ps = pixelShaders[PSTYPE_FORCEFIELDVISUALIZER];
				desc.dss = depthStencils[DSSTYPE_XRAY];
				desc.rs = rasterizers[RSTYPE_BACK];
				desc.bs = blendStates[BSTYPE_TRANSPARENT];
				desc.pt = TRIANGLELIST;
				break;
			case DEBUGRENDERING_FORCEFIELD_PLANE:
				desc.vs = vertexShaders[VSTYPE_FORCEFIELDVISUALIZER_PLANE];
				desc.ps = pixelShaders[PSTYPE_FORCEFIELDVISUALIZER];
				desc.dss = depthStencils[DSSTYPE_XRAY];
				desc.rs = rasterizers[RSTYPE_FRONT];
				desc.bs = blendStates[BSTYPE_TRANSPARENT];
				desc.pt = TRIANGLESTRIP;
				break;
			}

			desc.numRTs = 1;
			desc.RTFormats[0] = RTFormat_hdr;
			desc.DSFormat = DSFormat_full;

			RECREATE(PSO_debug[debug]);
			device->CreateGraphicsPSO(&desc, PSO_debug[debug]);
		}
	}));


	thread_pool.push_back(thread([&] {
		for (int i = 0; i < TILEDLIGHTING_TYPE_COUNT; ++i)
		{
			for (int j = 0; j < TILEDLIGHTING_CULLING_COUNT; ++j)
			{
				for (int k = 0; k < TILEDLIGHTING_DEBUG_COUNT; ++k)
				{
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

					ComputePSODesc desc;
					desc.cs = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + name, wiResourceManager::COMPUTESHADER));
					RECREATE(CPSO_tiledlighting[i][j][k]);
					device->CreateComputePSO(&desc, CPSO_tiledlighting[i][j][k]);
				}
			}
		}

		for (int i = 0; i < CSTYPE_LAST; ++i)
		{
			ComputePSODesc desc;
			desc.cs = computeShaders[i];
			RECREATE(CPSO[i]);
			device->CreateComputePSO(&desc, CPSO[i]);
		}
	}));


	for (auto& x : thread_pool)
	{
		x.join();
	}
	thread_pool.clear();
}

void wiRenderer::ReloadShaders(const std::string& path)
{

	if (path.length() > 0)
	{
		SHADERPATH = path;
	}

	GetDevice()->LOCK();

	GetDevice()->WaitForGPU();

	wiResourceManager::GetShaderManager()->CleanUp();
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

	GetDevice()->UNLOCK();
}


void wiRenderer::SetUpStates()
{
	for (int i = 0; i < SSLOT_COUNT; ++i)
	{
		samplers[i] = new Sampler;
	}

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
	samplerDesc.MaxLOD = FLOAT32_MAX;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_LINEAR_MIRROR]);

	samplerDesc.Filter = FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_LINEAR_CLAMP]);

	samplerDesc.Filter = FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_LINEAR_WRAP]);
	
	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_POINT_MIRROR]);
	
	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_POINT_WRAP]);
	
	
	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_POINT_CLAMP]);
	
	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MaxAnisotropy = 16;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_ANISO_CLAMP]);
	
	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 16;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_ANISO_WRAP]);
	
	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MaxAnisotropy = 16;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_ANISO_MIRROR]);

	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 16;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_OBJECTSHADER]);

	ZeroMemory( &samplerDesc, sizeof(SamplerDesc) );
	samplerDesc.Filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = COMPARISON_GREATER_EQUAL;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_CMP_DEPTH]);

	for (int i = 0; i < SSTYPE_LAST; ++i)
	{
		customsamplers[i] = new Sampler;
	}

	samplerDesc.Filter = FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = FLOAT32_MAX;
	GetDevice()->CreateSamplerState(&samplerDesc, customsamplers[SSTYPE_MAXIMUM_CLAMP]);


	for (int i = 0; i < RSTYPE_LAST; ++i)
	{
		rasterizers[i] = new RasterizerState;
	}
	
	RasterizerStateDesc rs;
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=true;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	rs.ConservativeRasterizationEnable = false;
	GetDevice()->CreateRasterizerState(&rs,rasterizers[RSTYPE_FRONT]);

	
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias = -2.0f;
	rs.DepthClipEnable=true;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	rs.ConservativeRasterizationEnable = false;
	GetDevice()->CreateRasterizerState(&rs,rasterizers[RSTYPE_SHADOW]);

	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias = -2.0f;
	rs.DepthClipEnable=true;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	rs.ConservativeRasterizationEnable = false;
	GetDevice()->CreateRasterizerState(&rs,rasterizers[RSTYPE_SHADOW_DOUBLESIDED]);

	rs.FillMode=FILL_WIREFRAME;
	rs.CullMode=CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=true;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	GetDevice()->CreateRasterizerState(&rs, rasterizers[RSTYPE_WIRE]);
	rs.AntialiasedLineEnable = true;
	GetDevice()->CreateRasterizerState(&rs, rasterizers[RSTYPE_WIRE_SMOOTH]);
	
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=true;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	rs.ConservativeRasterizationEnable = false;
	GetDevice()->CreateRasterizerState(&rs,rasterizers[RSTYPE_DOUBLESIDED]);
	
	rs.FillMode=FILL_WIREFRAME;
	rs.CullMode=CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=true;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable = false;
	rs.ConservativeRasterizationEnable = false;
	GetDevice()->CreateRasterizerState(&rs, rasterizers[RSTYPE_WIRE_DOUBLESIDED]);
	rs.AntialiasedLineEnable = true;
	GetDevice()->CreateRasterizerState(&rs, rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH]);
	
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_FRONT;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=true;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	rs.ConservativeRasterizationEnable = false;
	GetDevice()->CreateRasterizerState(&rs,rasterizers[RSTYPE_BACK]);

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
	GetDevice()->CreateRasterizerState(&rs, rasterizers[RSTYPE_OCCLUDEE]);

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
	GetDevice()->CreateRasterizerState(&rs, rasterizers[RSTYPE_SKY]);

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
	GetDevice()->CreateRasterizerState(&rs, rasterizers[RSTYPE_VOXELIZE]);

	for (int i = 0; i < DSSTYPE_LAST; ++i)
	{
		depthStencils[i] = new DepthStencilState;
	}

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
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_DEFAULT]);

	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_GREATER;
	dsd.StencilEnable = false;
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_SHADOW]);


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
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_DIRLIGHT]);


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
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_LIGHT]);


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
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_DECAL]);


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
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_STENCILREAD_MATCH]);


	dsd.DepthEnable = true;
	dsd.StencilEnable = false;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_GREATER_EQUAL;
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_DEPTHREAD]);

	dsd.DepthEnable = false;
	dsd.StencilEnable=false;
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_XRAY]);


	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_EQUAL;
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_DEPTHREADEQUAL]);


	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_GREATER;
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_ENVMAP]);


	for (int i = 0; i < BSTYPE_LAST; ++i)
	{
		blendStates[i] = new BlendState;
	}
	
	BlendStateDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=false;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_MAX;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.AlphaToCoverageEnable=false;
	bd.IndependentBlendEnable = false;
	GetDevice()->CreateBlendState(&bd,blendStates[BSTYPE_OPAQUE]);

	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.AlphaToCoverageEnable = false;
	bd.IndependentBlendEnable = false;
	GetDevice()->CreateBlendState(&bd,blendStates[BSTYPE_TRANSPARENT]);


	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable=false,
	bd.AlphaToCoverageEnable=false;
	GetDevice()->CreateBlendState(&bd,blendStates[BSTYPE_ADDITIVE]);


	bd.RenderTarget[0].BlendEnable = false;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_DISABLE;
	bd.IndependentBlendEnable = false,
	bd.AlphaToCoverageEnable = false;
	GetDevice()->CreateBlendState(&bd, blendStates[BSTYPE_COLORWRITEDISABLE]);


	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN | COLOR_WRITE_ENABLE_BLUE; // alpha is not written by deferred lights!
	bd.IndependentBlendEnable=false,
	bd.AlphaToCoverageEnable=false;
	GetDevice()->CreateBlendState(&bd,blendStates[BSTYPE_DEFERREDLIGHT]);

	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false,
	bd.AlphaToCoverageEnable = false;
	GetDevice()->CreateBlendState(&bd, blendStates[BSTYPE_ENVIRONMENTALLIGHT]);

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
	GetDevice()->CreateBlendState(&bd, blendStates[BSTYPE_INVERSE]);


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
	GetDevice()->CreateBlendState(&bd, blendStates[BSTYPE_DECAL]);


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
	GetDevice()->CreateBlendState(&bd, blendStates[BSTYPE_MULTIPLY]);


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
	GetDevice()->CreateBlendState(&bd, blendStates[BSTYPE_TRANSPARENTSHADOWMAP]);
}

void wiRenderer::BindPersistentState(GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = GetDevice();

	for (int i = 0; i < SSLOT_COUNT; ++i)
	{
		device->BindSampler(PS, samplers[i], i, threadID);
		device->BindSampler(VS, samplers[i], i, threadID);
		device->BindSampler(GS, samplers[i], i, threadID);
		device->BindSampler(DS, samplers[i], i, threadID);
		device->BindSampler(HS, samplers[i], i, threadID);
		device->BindSampler(CS, samplers[i], i, threadID);
	}


	device->BindConstantBuffer(PS, constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	device->BindConstantBuffer(VS, constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	device->BindConstantBuffer(GS, constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	device->BindConstantBuffer(HS, constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	device->BindConstantBuffer(DS, constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	device->BindConstantBuffer(CS, constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);

	device->BindConstantBuffer(PS, constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	device->BindConstantBuffer(VS, constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	device->BindConstantBuffer(GS, constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	device->BindConstantBuffer(HS, constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	device->BindConstantBuffer(DS, constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	device->BindConstantBuffer(CS, constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);

	device->BindConstantBuffer(PS, constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	device->BindConstantBuffer(VS, constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	device->BindConstantBuffer(GS, constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	device->BindConstantBuffer(HS, constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	device->BindConstantBuffer(DS, constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	device->BindConstantBuffer(CS, constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);

	device->BindConstantBuffer(VS, constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	device->BindConstantBuffer(PS, constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	device->BindConstantBuffer(GS, constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	device->BindConstantBuffer(DS, constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	device->BindConstantBuffer(HS, constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	device->BindConstantBuffer(CS, constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);

	device->BindConstantBuffer(VS, constantBuffers[CBTYPE_API], CB_GETBINDSLOT(APICB), threadID);
	device->BindConstantBuffer(PS, constantBuffers[CBTYPE_API], CB_GETBINDSLOT(APICB), threadID);
}

Transform* wiRenderer::getTransformByName(const std::string& get)
{
	//auto transf = transforms.find(get);
	//if (transf != transforms.end())
	//{
	//	return transf->second;
	//}
	return GetScene().GetWorldNode()->find(get);
}
Transform* wiRenderer::getTransformByID(uint64_t id)
{
	for (Model* model : GetScene().models)
	{
		Transform* found = model->find(id);
		if (found != nullptr)
		{
			return found;
		}
	}
	return nullptr;
}
Armature* wiRenderer::getArmatureByName(const std::string& get)
{
	for (Model* model : GetScene().models)
	{
		for (Armature* armature : model->armatures)
			if (!armature->name.compare(get))
				return armature;
	}
	return nullptr;
}
int wiRenderer::getActionByName(Armature* armature, const std::string& get)
{
	if(armature==nullptr)
		return (-1);

	stringstream ss("");
	ss<<armature->name<<get;
	for (unsigned int j = 0; j<armature->actions.size(); j++)
		if(!armature->actions[j].name.compare(ss.str()))
			return j;
	return (-1);
}
int wiRenderer::getBoneByName(Armature* armature, const std::string& get)
{
	for (unsigned int j = 0; j<armature->boneCollection.size(); j++)
		if(!armature->boneCollection[j]->name.compare(get))
			return j;
	return (-1);
}
Material* wiRenderer::getMaterialByName(const std::string& get)
{
	for (Model* model : GetScene().models)
	{
		MaterialCollection::iterator iter = model->materials.find(get);
		if (iter != model->materials.end())
			return iter->second;
	}
	return NULL;
}
Object* wiRenderer::getObjectByName(const std::string& name)
{
	for (Model* model : GetScene().models)
	{
		for (auto& x : model->objects)
		{
			if (!x->name.compare(name))
			{
				return x;
			}
		}
	}

	return nullptr;
}
Camera* wiRenderer::getCameraByName(const std::string& name)
{
	for (Model* model : GetScene().models)
	{
		for (auto& x : model->cameras)
		{
			if (!x->name.compare(name))
			{
				return x;
			}
		}
	}

	return nullptr;
}
Light* wiRenderer::getLightByName(const std::string& name)
{
	for (Model* model : GetScene().models)
	{
		for (auto& x : model->lights)
		{
			if (!x->name.compare(name))
			{
				return x;
			}
		}
	}
	return nullptr;
}

Mesh::Vertex_FULL wiRenderer::TransformVertex(const Mesh* mesh, int vertexI, const XMMATRIX& mat)
{
	XMMATRIX sump;
	XMVECTOR pos = mesh->vertices_POS[vertexI].LoadPOS();
	XMVECTOR nor = mesh->vertices_POS[vertexI].LoadNOR();

	if (mesh->hasArmature() && !mesh->armature->boneCollection.empty())
	{
		XMFLOAT4 ind = mesh->vertices_BON[vertexI].GetInd_FULL();
		XMFLOAT4 wei = mesh->vertices_BON[vertexI].GetWei_FULL();


		float inWei[4] = {
			wei.x,
			wei.y,
			wei.z,
			wei.w 
		};
		float inBon[4] = {
			ind.x,
			ind.y,
			ind.z,
			ind.w 
		};
		if (inWei[0] || inWei[1] || inWei[2] || inWei[3])
		{
			sump = XMMATRIX(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
			for (unsigned int i = 0; i < 4; i++)
			{
				sump += XMLoadFloat4x4(&mesh->armature->boneCollection[int(inBon[i])]->boneRelativity) * inWei[i];
			}
		}
		else
		{
			sump = XMMatrixIdentity();
		}
		sump = XMMatrixMultiply(sump, mat);
	}
	else
	{
		sump = mat;
	}

	XMFLOAT3 transformedP, transformedN;
	XMStoreFloat3(&transformedP, XMVector3Transform(pos, sump));

	XMStoreFloat3(&transformedN, XMVector3Normalize(XMVector3TransformNormal(nor, sump)));

	Mesh::Vertex_FULL retV(transformedP);
	retV.nor = XMFLOAT4(transformedN.x, transformedN.y, transformedN.z, retV.nor.w);
	retV.tex = mesh->vertices_FULL[vertexI].tex;

	return retV;
}
void wiRenderer::FixedUpdate()
{
	cam->UpdateTransform();

	objectsWithTrails.clear();
	emitterSystems.clear();

	GetScene().Update();

}
void wiRenderer::UpdatePerFrameData(float dt)
{
	// update the space partitioning trees:
	wiProfiler::GetInstance().BeginRange("SPTree Update", wiProfiler::DOMAIN_CPU);
	if (GetGameSpeed() > 0)
	{
		if (spTree != nullptr && spTree->root != nullptr)
		{
			wiSPTree* newTree = spTree->updateTree();
			if (newTree != nullptr)
			{
				SAFE_DELETE(spTree);
				spTree = newTree;
			}
		}
		if (spTree_lights != nullptr && spTree_lights->root != nullptr)
		{
			wiSPTree* newTree = spTree_lights->updateTree();
			if (newTree != nullptr)
			{
				SAFE_DELETE(spTree_lights);
				spTree_lights = newTree;
			}
		}
	}
	wiProfiler::GetInstance().EndRange(); // SPTree Update

	// Update Voxelization parameters:
	if (spTree != nullptr)
	{
		// We don't update it if the scene is empty, this even makes it easier to debug
		const float f = 0.05f / voxelSceneData.voxelsize;
		XMFLOAT3 center = XMFLOAT3(floorf(cam->translation.x * f) / f, floorf(cam->translation.y * f) / f, floorf(cam->translation.z * f) / f);
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

	// Perform culling and obtain closest reflector:
	requestReflectionRendering = false;
	wiProfiler::GetInstance().BeginRange("SPTree Culling", wiProfiler::DOMAIN_CPU);
	{
		for (auto& x : frameCullings)
		{
			Camera* camera = x.first;
			FrameCulling& culling = x.second;
			culling.Clear();

			if (!freezeCullingCamera)
			{
				culling.frustum = camera->frustum;
			}

			if (spTree != nullptr)
			{
				CulledList culledObjects;
				spTree->getVisible(culling.frustum, culledObjects, wiSPTree::SortType::SP_TREE_SORT_FRONT_TO_BACK);
				for (Cullable* x : culledObjects)
				{
					Object* object = (Object*)x;
					culling.culledRenderer[object->mesh].push_front(object);
					for (wiHairParticle* hair : object->hParticleSystems)
					{
						culling.culledHairParticleSystems.push_back(hair);
					}
					if (object->GetRenderTypes() & RENDERTYPE_OPAQUE)
					{
						culling.culledRenderer_opaque[object->mesh].push_front(object);
					}
					if (!requestReflectionRendering && camera == getCamera() && object->IsReflector())
					{
						// If it is the main camera's culling, then obtain the reflectors:
						XMVECTOR _refPlane = XMPlaneFromPointNormal(XMLoadFloat3(&object->/*bounds.getCenter()*/translation), XMVectorSet(0, 1, 0, 0));
						XMFLOAT4 plane;
						XMStoreFloat4(&plane, _refPlane);
						waterPlane = wiWaterPlane(plane.x, plane.y, plane.z, plane.w);
						requestReflectionRendering = true;
					}
				}
				wiSPTree::Sort(camera->translation, culledObjects, wiSPTree::SortType::SP_TREE_SORT_BACK_TO_FRONT);
				for (Cullable* x : culledObjects)
				{
					Object* object = (Object*)x;
					if (object->GetRenderTypes() & RENDERTYPE_TRANSPARENT || object->GetRenderTypes() & RENDERTYPE_WATER)
					{
						culling.culledRenderer_transparent[object->mesh].push_front(object);
					}
				}
			}
			if (camera==getCamera() && spTree_lights != nullptr) // only the main camera can render lights and write light array properties (yet)!
			{
				for (Model* model : GetScene().models)
				{
					for (Decal* decal : model->decals)
					{
						if ((decal->texture || decal->normal) && culling.frustum.CheckBox(decal->bounds))
						{
							x.second.culledDecals.push_back(decal);
						}
					}

					for (EnvironmentProbe* probe : model->environmentProbes)
					{
						if (probe->textureIndex >= 0 && culling.frustum.CheckBox(probe->bounds))
						{
							x.second.culledEnvProbes.push_back(probe);
						}
					}
				}

				spTree_lights->getVisible(culling.frustum, culling.culledLights, wiSPTree::SortType::SP_TREE_SORT_NONE);

				if (GetVoxelRadianceEnabled())
				{
					// Inject lights which are inside the voxel grid too
					AABB box;
					box.createFromHalfWidth(voxelSceneData.center, voxelSceneData.extents);
					spTree_lights->getVisible(box, culling.culledLights, wiSPTree::SortType::SP_TREE_SORT_NONE);
				}

				// We sort lights so that closer lights will have more priority for shadows!
				spTree_lights->Sort(camera->translation, culling.culledLights, wiSPTree::SortType::SP_TREE_SORT_FRONT_TO_BACK);

				int i = 0;
				int shadowCounter_2D = 0;
				int shadowCounter_Cube = 0;
				for (auto& c : culling.culledLights)
				{
					Light* l = (Light*)c;
					l->entityArray_index = i;

					l->UpdateLight();

					// Link shadowmaps to lights till there are free slots

					l->shadowMap_index = -1;

					if (l->shadow)
					{
						switch (l->GetType())
						{
						case Light::DIRECTIONAL:
							if (!l->shadowCam_dirLight.empty() && (shadowCounter_2D + 2) < SHADOWCOUNT_2D)
							{
								l->shadowMap_index = shadowCounter_2D;
								shadowCounter_2D += 3;
							}
							break;
						case Light::SPOT:
							if (!l->shadowCam_spotLight.empty() && shadowCounter_2D < SHADOWCOUNT_2D)
							{
								l->shadowMap_index = shadowCounter_2D;
								shadowCounter_2D++;
							}
							break;
						case Light::POINT:
						case Light::SPHERE:
						case Light::DISC:
						case Light::RECTANGLE:
						case Light::TUBE:
							if (!l->shadowCam_pointLight.empty() && shadowCounter_Cube < SHADOWCOUNT_CUBE)
							{
								l->shadowMap_index = shadowCounter_Cube;
								shadowCounter_Cube++;
							}
							break;
						default:
							break;
						}
					}

					i++;
				}
			}
		}
	}
	wiProfiler::GetInstance().EndRange(); // SPTree Culling

	// Ocean will override any current reflectors
	if (ocean != nullptr)
	{
		requestReflectionRendering = true; 
		XMVECTOR _refPlane = XMPlaneFromPointNormal(XMVectorSet(0, ocean->waterHeight, 0, 0), XMVectorSet(0, 1, 0, 0));
		XMFLOAT4 plane;
		XMStoreFloat4(&plane, _refPlane);
		waterPlane = wiWaterPlane(plane.x, plane.y, plane.z, plane.w);
	}

	for (auto& x : emitterSystems)
	{
		x->Update(dt*GetGameSpeed());
	}

	if (GetTemporalAAEnabled())
	{
		const XMFLOAT4& halton = wiMath::GetHaltonSequence(GetDevice()->GetFrameCount() % 256);
		static float jitter = 1.0f;
		temporalAAJitterPrev = temporalAAJitter;
		temporalAAJitter.x = jitter * (halton.x * 2 - 1) / (float)GetInternalResolution().x;
		temporalAAJitter.y = jitter * (halton.y * 2 - 1) / (float)GetInternalResolution().y;
		cam->Projection.m[2][0] = temporalAAJitter.x;
		cam->Projection.m[2][1] = temporalAAJitter.y;
		cam->BakeMatrices();
	}
	else
	{
		temporalAAJitter = XMFLOAT2(0, 0);
		temporalAAJitterPrev = XMFLOAT2(0, 0);
	}

	refCam->Reflect(cam, waterPlane.getXMFLOAT4());

	UpdateBoneLines();
	UpdateCubes();

	renderTime_Prev = renderTime;
	renderTime += dt * GameSpeed;
	deltaTime = dt;
}
void wiRenderer::UpdateRenderData(GRAPHICSTHREAD threadID)
{
	const FrameCulling& mainCameraCulling = frameCullings[getCamera()];

	// Fill Light Array with lights + envprobes + decals in the frustum:
	{
		const CulledList& culledLights = mainCameraCulling.culledLights;

		static ShaderEntityType* entityArray = (ShaderEntityType*)_mm_malloc(sizeof(ShaderEntityType)*MAX_SHADER_ENTITY_COUNT, 16);
		static XMMATRIX* matrixArray = (XMMATRIX*)_mm_malloc(sizeof(XMMATRIX)*MATRIXARRAY_COUNT, 16);

		const XMMATRIX viewMatrix = cam->GetView();

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

		entityArrayOffset_Lights = entityCounter;
		for (Cullable* c : culledLights)
		{
			if (entityCounter == MAX_SHADER_ENTITY_COUNT)
			{
				assert(0); // too many entities!
				entityCounter--;
				break;
			}

			Light* l = (Light*)c;
			if (!l->IsActive())
			{
				continue;
			}

			const int shadowIndex = l->shadowMap_index;

			entityArray[entityCounter].type = l->GetType();
			entityArray[entityCounter].positionWS = l->translation;
			XMStoreFloat3(&entityArray[entityCounter].positionVS, XMVector3TransformCoord(XMLoadFloat3(&entityArray[entityCounter].positionWS), viewMatrix));
			entityArray[entityCounter].range = l->enerDis.y;
			entityArray[entityCounter].color = wiMath::CompressColor(l->color);
			entityArray[entityCounter].energy = l->enerDis.x;
			entityArray[entityCounter].shadowBias = l->shadowBias;
			entityArray[entityCounter].additionalData_index = shadowIndex;
			switch (l->GetType())
			{
			case Light::DIRECTIONAL:
			{
				entityArray[entityCounter].directionWS = l->GetDirection();
				entityArray[entityCounter].shadowKernel = 1.0f / SHADOWRES_2D;

				if (l->shadow && shadowIndex >= 0 && !l->shadowCam_dirLight.empty())
				{
					matrixArray[shadowIndex + 0] = l->shadowCam_dirLight[0].getVP();
					matrixArray[shadowIndex + 1] = l->shadowCam_dirLight[1].getVP();
					matrixArray[shadowIndex + 2] = l->shadowCam_dirLight[2].getVP();
					matrixCounter = max(matrixCounter, (UINT)shadowIndex + 3);
				}
			}
			break;
			case Light::SPOT:
			{
				entityArray[entityCounter].coneAngleCos = cosf(l->enerDis.z * 0.5f);
				entityArray[entityCounter].directionWS = l->GetDirection();
				XMStoreFloat3(&entityArray[entityCounter].directionVS, XMVector3TransformNormal(XMLoadFloat3(&entityArray[entityCounter].directionWS), viewMatrix));
				entityArray[entityCounter].shadowKernel = 1.0f / SHADOWRES_2D;

				if (l->shadow && shadowIndex >= 0 && !l->shadowCam_spotLight.empty())
				{
					matrixArray[shadowIndex + 0] = l->shadowCam_spotLight[0].getVP();
					matrixCounter = max(matrixCounter, (UINT)shadowIndex + 1);
				}
			}
			break;
			case Light::POINT:
			{
				entityArray[entityCounter].shadowKernel = 1.0f / SHADOWRES_CUBE;
			}
			break;
			case Light::SPHERE:
			case Light::DISC:
			case Light::RECTANGLE:
			case Light::TUBE:
			{
				XMMATRIX lightMat = XMLoadFloat4x4(&l->world);
				// Note: area lights are facing back by default
				XMStoreFloat3(&entityArray[entityCounter].directionWS, XMVector3TransformNormal(XMVectorSet(-1, 0, 0, 0), lightMat)); // right dir
				XMStoreFloat3(&entityArray[entityCounter].directionVS, XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), lightMat)); // up dir
				XMStoreFloat3(&entityArray[entityCounter].positionVS, XMVector3TransformNormal(XMVectorSet(0, 0, -1, 0), lightMat)); // front dir
				entityArray[entityCounter].texMulAdd = XMFLOAT4(l->radius, l->width, l->height, 0);
			}
			break;
			}

			entityCounter++;
		}
		entityArrayCount_Lights = entityCounter - entityArrayOffset_Lights;

		entityArrayOffset_EnvProbes = entityCounter;
		for (EnvironmentProbe* probe : mainCameraCulling.culledEnvProbes)
		{
			if (probe->textureIndex < 0)
			{
				continue;
			}
			if (entityCounter == MAX_SHADER_ENTITY_COUNT)
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

			entityArray[entityCounter].type = ENTITY_TYPE_ENVMAP;
			entityArray[entityCounter].positionWS = probe->translation;
			XMStoreFloat3(&entityArray[entityCounter].positionVS, XMVector3TransformCoord(XMLoadFloat3(&probe->translation), viewMatrix));
			entityArray[entityCounter].range = max(probe->scale.x, max(probe->scale.y, probe->scale.z)) * 2;
			entityArray[entityCounter].shadowBias = (float)probe->textureIndex;

			entityArray[entityCounter].additionalData_index = matrixCounter;
			matrixArray[matrixCounter] = XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&probe->world)));
			matrixCounter++;

			entityCounter++;
		}
		entityArrayCount_EnvProbes = entityCounter - entityArrayOffset_EnvProbes;

		entityArrayOffset_Decals = entityCounter;
		for (Decal* decal : mainCameraCulling.culledDecals)
		{
			if (entityCounter == MAX_SHADER_ENTITY_COUNT)
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
			entityArray[entityCounter].type = ENTITY_TYPE_DECAL;
			entityArray[entityCounter].positionWS = decal->translation;
			XMStoreFloat3(&entityArray[entityCounter].positionVS, XMVector3TransformCoord(XMLoadFloat3(&decal->translation), viewMatrix));
			entityArray[entityCounter].range = max(decal->scale.x, max(decal->scale.y, decal->scale.z)) * 2;
			entityArray[entityCounter].texMulAdd = decal->atlasMulAdd;
			entityArray[entityCounter].color = wiMath::CompressColor(XMFLOAT4(decal->color.x, decal->color.y, decal->color.z, decal->GetOpacity()));
			entityArray[entityCounter].energy = decal->emissive;

			entityArray[entityCounter].additionalData_index = matrixCounter;
			matrixArray[matrixCounter] = XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&decal->world)));
			matrixCounter++;

			entityCounter++;
		}
		entityArrayCount_Decals = entityCounter - entityArrayOffset_Decals;

		entityArrayOffset_ForceFields = entityCounter;
		for (auto& model : GetScene().models)
		{
			for (ForceField* force : model->forces)
			{
				if (entityCounter == MAX_SHADER_ENTITY_COUNT)
				{
					assert(0); // too many entities!
					entityCounter--;
					break;
				}

				entityArray[entityCounter].type = force->type;
				entityArray[entityCounter].positionWS = force->translation;
				entityArray[entityCounter].energy = force->gravity;
				entityArray[entityCounter].range = 1.0f / max(0.0001f, force->range); // avoid division in shader
				entityArray[entityCounter].coneAngleCos = force->range; // this will be the real range in the less common shaders...
				// The default planar force field is facing upwards, and thus the pull direction is downwards:
				XMStoreFloat3(&entityArray[entityCounter].directionWS, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, -1, 0, 0), XMLoadFloat4x4(&force->world))));

				entityCounter++;
			}
		}
		entityArrayCount_ForceFields = entityCounter - entityArrayOffset_ForceFields;

		GetDevice()->UpdateBuffer(resourceBuffers[RBTYPE_ENTITYARRAY], entityArray, threadID, sizeof(ShaderEntityType)*entityCounter);
		GetDevice()->UpdateBuffer(resourceBuffers[RBTYPE_MATRIXARRAY], matrixArray, threadID, sizeof(XMMATRIX)*matrixCounter);

		GPUResource* resources[] = {
			resourceBuffers[RBTYPE_ENTITYARRAY],
			resourceBuffers[RBTYPE_MATRIXARRAY],
		};
		GetDevice()->BindResources(VS, resources, SBSLOT_ENTITYARRAY, ARRAYSIZE(resources), threadID);
		GetDevice()->BindResources(PS, resources, SBSLOT_ENTITYARRAY, ARRAYSIZE(resources), threadID);
		GetDevice()->BindResources(CS, resources, SBSLOT_ENTITYARRAY, ARRAYSIZE(resources), threadID);
	}


	UpdateWorldCB(threadID); // only commits when parameters are changed
	UpdateFrameCB(threadID);
	BindPersistentState(threadID);

	ManageDecalAtlas(threadID);

	wiProfiler::GetInstance().BeginRange("Skinning", wiProfiler::DOMAIN_GPU, threadID);
	GetDevice()->EventBegin("Skinning", threadID);
	{
		bool streamOutSetUp = false;
		CSTYPES lastCS = CSTYPE_SKINNING_LDS;

		for (Model* model : GetScene().models)
		{
			// Update material constant buffers:
			MaterialCB materialGPUData;
			for (auto& it : model->materials)
			{
				Material* material = it.second;
				materialGPUData.Create(*material);
				// These will probably not change every time so only issue a GPU memory update if it is necessary:
				if (memcmp(&material->gpuData, &materialGPUData, sizeof(MaterialCB)) != 0)
				{
					material->gpuData = materialGPUData;
					GetDevice()->UpdateBuffer(&material->constantBuffer, &materialGPUData, threadID);
				}
			}

			// Skinning:
			for (MeshCollection::iterator iter = model->meshes.begin(); iter != model->meshes.end(); ++iter)
			{
				Mesh* mesh = iter->second;

				if (mesh->hasArmature() && !mesh->hasDynamicVB() && mesh->renderable && !mesh->vertices_POS.empty()
					&& mesh->streamoutBuffer_POS != nullptr && mesh->vertexBuffer_POS != nullptr)
				{
					Armature* armature = mesh->armature;

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
						GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
						GetDevice()->BindComputePSO(CPSO[CSTYPE_SKINNING_LDS], threadID);
					}

					CSTYPES targetCS = CSTYPE_SKINNING_LDS;

					if (!GetLDSSkinningEnabled() || armature->boneCollection.size() > SKINNING_COMPUTE_THREADCOUNT)
					{
						// If we have more bones that can fit into LDS, we switch to a skinning shader which loads from device memory:
						targetCS = CSTYPE_SKINNING;
					}

					if (targetCS != lastCS)
					{
						lastCS = targetCS;
						GetDevice()->BindComputePSO(CPSO[targetCS], threadID);
					}

					// Upload bones for skinning to shader
					for (unsigned int k = 0; k < armature->boneCollection.size(); k++)
					{
						armature->boneData[k].Create(armature->boneCollection[k]->boneRelativity);
					}
					GetDevice()->UpdateBuffer(&armature->boneBuffer, armature->boneData.data(), threadID, (int)(sizeof(Armature::ShaderBoneType) * armature->boneCollection.size()));
					GetDevice()->BindResource(CS, &armature->boneBuffer, SKINNINGSLOT_IN_BONEBUFFER, threadID);

					// Do the skinning
					GPUResource* vbs[] = {
						mesh->vertexBuffer_POS,
						mesh->vertexBuffer_BON,
					};
					GPUResource* sos[] = {
						mesh->streamoutBuffer_POS,
						mesh->streamoutBuffer_PRE,
					};

					GetDevice()->BindResources(CS, vbs, SKINNINGSLOT_IN_VERTEX_POS, ARRAYSIZE(vbs), threadID);
					GetDevice()->BindUnorderedAccessResourcesCS(sos, 0, ARRAYSIZE(sos), threadID);

					GetDevice()->Dispatch((UINT)ceilf((float)mesh->vertices_POS.size() / SKINNING_COMPUTE_THREADCOUNT), 1, 1, threadID);
					GetDevice()->UAVBarrier(sos, ARRAYSIZE(sos), threadID); // todo: defer
					//GetDevice()->TransitionBarrier(sos, ARRAYSIZE(sos), RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, threadID);
				}
				else if (mesh->hasDynamicVB())
				{
					// Upload CPU skinned vertex buffer (Soft body VB)
					size_t size_pos = sizeof(Mesh::Vertex_POS)*mesh->vertices_Transformed_POS.size();
					size_t size_pre = sizeof(Mesh::Vertex_POS)*mesh->vertices_Transformed_PRE.size();
					UINT offset;
					void* vertexData = GetDevice()->AllocateFromRingBuffer(dynamicVertexBufferPool, size_pos + size_pre, offset, threadID);
					mesh->bufferOffset_POS = offset;
					mesh->bufferOffset_PRE = offset + (UINT)size_pos;
					memcpy(vertexData, mesh->vertices_Transformed_POS.data(), size_pos);
					memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(vertexData) + size_pos), mesh->vertices_Transformed_PRE.data(), size_pre);
					GetDevice()->InvalidateBufferAccess(dynamicVertexBufferPool, threadID);
				}
			}
		}

		if (streamOutSetUp)
		{
			GetDevice()->UnBindUnorderedAccessResources(0, 2, threadID);
			GetDevice()->UnBindResources(SKINNINGSLOT_IN_VERTEX_POS, 2, threadID);
		}

	}
	GetDevice()->EventEnd(threadID);
	wiProfiler::GetInstance().EndRange(threadID); // skinning

	// Particle system simulation/sorting/culling:
	for (auto& x : emitterSystems)
	{
		x->UpdateRenderData(threadID);
	}
	for (wiHairParticle* hair : mainCameraCulling.culledHairParticleSystems)
	{
		hair->ComputeCulling(getCamera(), threadID);
	}

	// Compute water simulation:
	if (ocean != nullptr)
	{
		ocean->UpdateDisplacementMap(renderTime, threadID);
	}

	// Generate cloud layer:
	if(enviroMap == nullptr && GetScene().worldInfo.cloudiness > 0) // generate only when sky is dynamic
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

			GetDevice()->CreateTexture2D(&desc, nullptr, (Texture2D**)&textures[TEXTYPE_2D_CLOUDS]);
		}

		float cloudPhase = renderTime * GetScene().worldInfo.cloudSpeed;
		GenerateClouds((Texture2D*)textures[TEXTYPE_2D_CLOUDS], 5, cloudPhase, GRAPHICSTHREAD_IMMEDIATE);
	}

	// Render out of date environment probes:
	RefreshEnvProbes(threadID);
}
void wiRenderer::OcclusionCulling_Render(GRAPHICSTHREAD threadID)
{
	if (!GetOcclusionCullingEnabled() || spTree == nullptr || GetFreezeCullingCameraEnabled())
	{
		return;
	}

	const FrameCulling& culling = frameCullings[getCamera()];
	const CulledCollection& culledRenderer = culling.culledRenderer;

	wiProfiler::GetInstance().BeginRange("Occlusion Culling Render", wiProfiler::DOMAIN_GPU, threadID);

	int queryID = 0;

	if (!culledRenderer.empty())
	{
		GetDevice()->EventBegin("Occlusion Culling Render", threadID);

		//GetDevice()->BindRasterizerState(rasterizers[RSTYPE_OCCLUDEE], threadID);
		//GetDevice()->BindBlendState(blendStates[BSTYPE_COLORWRITEDISABLE], threadID);
		//GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_DEFAULT, threadID);
		//GetDevice()->BindVertexLayout(nullptr, threadID);
		//GetDevice()->BindVS(vertexShaders[VSTYPE_CUBE], threadID);
		//GetDevice()->BindPS(nullptr, threadID);

		GetDevice()->BindGraphicsPSO(PSO_occlusionquery, threadID);

		int queryID = 0;

		for (CulledCollection::const_iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter)
		{
			Mesh* mesh = iter->first;
			if (!mesh->renderable)
			{
				continue;
			}
			const CulledObjectList& visibleInstances = iter->second;

			MiscCB cb;
			for (Object* instance : visibleInstances)
			{
				if(queryID >= ARRAYSIZE(occlusionQueries))
				{
					instance->occlusionQueryID = -1; // assign an invalid id from the pool
					continue;
				}
				
				// If a query could be retrieved from the pool for the instance, the instance can be occluded, so render it
				GPUQuery& query = occlusionQueries[queryID];
				if (!query.IsValid())
				{
					continue;
				}

				if (instance->bounds.intersects(getCamera()->translation) || mesh->softBody) // todo: correct softbody
				{
					// camera is inside the instance, mark it as visible in this frame:
					instance->occlusionHistory |= 1;
				}
				else
				{
					// only query for occlusion if the camera is outside the instance
					instance->occlusionQueryID = queryID; // just assign the id from the pool
					queryID++;

					// previous frame view*projection because these are drawn against the previous depth buffer:
					cb.mTransform = XMMatrixTranspose(instance->GetOBB()*prevFrameCam->GetViewProjection()); 
					GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &cb, threadID);

					// render bounding box to later read the occlusion status
					GetDevice()->QueryBegin(&query, threadID);
					GetDevice()->Draw(14, 0, threadID);
					GetDevice()->QueryEnd(&query, threadID);
				}
			}
		}

		GetDevice()->EventEnd(threadID);
	}

	wiProfiler::GetInstance().EndRange(threadID); // Occlusion Culling Render
}
void wiRenderer::OcclusionCulling_Read()
{
	if (!GetOcclusionCullingEnabled() || spTree == nullptr || GetFreezeCullingCameraEnabled())
	{
		return;
	}

	wiProfiler::GetInstance().BeginRange("Occlusion Culling Read", wiProfiler::DOMAIN_CPU);

	const FrameCulling& culling = frameCullings[getCamera()];
	const CulledCollection& culledRenderer = culling.culledRenderer;

	if (!culledRenderer.empty())
	{
		GetDevice()->EventBegin("Occlusion Culling Read", GRAPHICSTHREAD_IMMEDIATE);

		for (CulledCollection::const_iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter)
		{
			Mesh* mesh = iter->first;
			if (!mesh->renderable || mesh->softBody) // todo: correct softbody
			{
				continue;
			}

			const CulledObjectList& visibleInstances = iter->second;

			for (Object* instance : visibleInstances)
			{
				instance->occlusionHistory <<= 1; // advance history by 1 frame
				if (instance->occlusionQueryID < 0)
				{
					instance->occlusionHistory |= 1; // mark this frame as visible
					continue;
				}
				GPUQuery& query = occlusionQueries[instance->occlusionQueryID];
				if (!query.IsValid())
				{
					instance->occlusionHistory |= 1; // mark this frame as visible
					continue;
				}

				while (!GetDevice()->QueryRead(&query, GRAPHICSTHREAD_IMMEDIATE)) {}

				if (query.result_passed == TRUE)
				{
					instance->occlusionHistory |= 1; // mark this frame as visible
				}
				else
				{
					// leave this frame as occluded
				}
			}
		}

		GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);
	}

	wiProfiler::GetInstance().EndRange(); // Occlusion Culling Read
}
void wiRenderer::UpdateImages()
{
	for (wiSprite* x : images)
		x->Update(GameSpeed);
	for (wiSprite* x : waterRipples)
		x->Update(GameSpeed);

	ManageImages();
	ManageWaterRipples();
}
void wiRenderer::ManageImages()
{
	while (!images.empty() &&
		(images.front()->effects.opacity <= 0 + FLT_EPSILON || images.front()->effects.fade == 1))
	{
		images.pop_front();
	}
}
void wiRenderer::PutDecal(Decal* decal)
{
	GetScene().GetWorldNode()->decals.insert(decal);
}
void wiRenderer::PutWaterRipple(const std::string& image, const XMFLOAT3& pos)
{
	wiSprite* img=new wiSprite("","",image);
	img->anim.fad=0.01f;
	img->anim.scaleX=0.2f;
	img->anim.scaleY=0.2f;
	img->effects.pos=pos;
	img->effects.rotation=(wiRandom::getRandom(0,1000)*0.001f)*2*3.1415f;
	img->effects.siz=XMFLOAT2(1,1);
	img->effects.typeFlag=WORLD;
	img->effects.quality=QUALITY_ANISOTROPIC;
	img->effects.pivot = XMFLOAT2(0.5f, 0.5f);
	img->effects.lookAt=waterPlane.getXMFLOAT4();
	img->effects.lookAt.w=1;
	waterRipples.push_back(img);
}
void wiRenderer::ManageWaterRipples(){
	while(	
		!waterRipples.empty() && 
			(waterRipples.front()->effects.opacity <= 0 + FLT_EPSILON || waterRipples.front()->effects.fade==1)
		)
		waterRipples.pop_front();
}
void wiRenderer::DrawWaterRipples(GRAPHICSTHREAD threadID)
{
	GetDevice()->EventBegin("Water Ripples", threadID);
	for(wiSprite* i:waterRipples){
		i->DrawNormal(threadID);
	}
	GetDevice()->EventEnd(threadID);
}

void wiRenderer::DrawDebugSpheres(Camera* camera, GRAPHICSTHREAD threadID)
{
	//if(debugSpheres){
	//	BindPrimitiveTopology(TRIANGLESTRIP,threadID);
	//	BindVertexLayout(vertexLayouts[VLTYPE_EFFECT] : vertexLayouts[VLTYPE_LINE],threadID);
	//
	//	BindRasterizerState(rasterizers[RSTYPE_FRONT],threadID);
	//	BindDepthStencilState(depthStencils[DSSTYPE_XRAY],STENCILREF_EMPTY,threadID);
	//	BindBlendState(blendStates[BSTYPE_TRANSPARENT],threadID);


	//	BindPS(linePS,threadID);
	//	BindVS(lineVS,threadID);

	//	BindVertexBuffer(HitSphere::vertexBuffer,0,sizeof(XMFLOAT3A),threadID);

	//	for (unsigned int i = 0; i<spheres.size(); i++){
	//		//MAPPED_SUBRESOURCE mappedResource;
	//		LineBuffer sb;
	//		sb.mWorldViewProjection=XMMatrixTranspose(
	//			XMMatrixRotationQuaternion(XMLoadFloat4(&camera->rotation))*
	//			XMMatrixScaling( spheres[i]->radius,spheres[i]->radius,spheres[i]->radius ) *
	//			XMMatrixTranslationFromVector( XMLoadFloat3(&spheres[i]->translation) )
	//			*camera->GetViewProjection()
	//			);

	//		XMFLOAT4A propColor;
	//		if(spheres[i]->TYPE==HitSphere::HitSphereType::HITTYPE)      propColor = XMFLOAT4A(0.1098f,0.4196f,1,1);
	//		else if(spheres[i]->TYPE==HitSphere::HitSphereType::INVTYPE) propColor=XMFLOAT4A(0,0,0,1);
	//		else if(spheres[i]->TYPE==HitSphere::HitSphereType::ATKTYPE) propColor=XMFLOAT4A(0.96f,0,0,1);
	//		sb.color=propColor;

	//		UpdateBuffer(lineBuffer,&sb,threadID);


	//		//threadID->Draw((HitSphere::RESOLUTION+1)*2,0);
	//		Draw((HitSphere::RESOLUTION+1)*2,threadID);

	//	}
	//}
	
}
void wiRenderer::DrawDebugBoneLines(Camera* camera, GRAPHICSTHREAD threadID)
{
	if(debugBoneLines)
	{
		GetDevice()->EventBegin("DebugBoneLines", threadID);
		
		GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_BONELINES], threadID);

		MiscCB sb;
		for (unsigned int i = 0; i<boneLines.size(); i++){
			sb.mTransform = XMMatrixTranspose(XMLoadFloat4x4(&boneLines[i]->desc.transform)*camera->GetViewProjection());
			sb.mColor = boneLines[i]->desc.color;

			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

			GPUBuffer* vbs[] = {
				&boneLines[i]->vertexBuffer,
			};
			const UINT strides[] = {
				sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
			};
			GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
			GetDevice()->Draw(2, 0, threadID);
		}

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawDebugLines(Camera* camera, GRAPHICSTHREAD threadID)
{
	if (linesTemp.empty())
		return;

	GetDevice()->EventBegin("DebugLines", threadID);

	GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_LINES], threadID);

	MiscCB sb;
	for (unsigned int i = 0; i<linesTemp.size(); i++){
		sb.mTransform = XMMatrixTranspose(XMLoadFloat4x4(&linesTemp[i]->desc.transform)*camera->GetViewProjection());
		sb.mColor = linesTemp[i]->desc.color;

		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

		GPUBuffer* vbs[] = {
			&linesTemp[i]->vertexBuffer,
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		GetDevice()->Draw(2, 0, threadID);
	}

	for (Lines* x : linesTemp)
		delete x;
	linesTemp.clear();

	GetDevice()->EventEnd(threadID);
}
void wiRenderer::DrawDebugBoxes(Camera* camera, GRAPHICSTHREAD threadID)
{
	if(debugPartitionTree || !renderableBoxes.empty())
	{
		GetDevice()->EventBegin("DebugBoxes", threadID);
		
		GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_CUBE], threadID);

		GPUBuffer* vbs[] = {
			&Cube::vertexBuffer,
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		GetDevice()->BindIndexBuffer(&Cube::indexBuffer, INDEXFORMAT_16BIT, 0, threadID);

		MiscCB sb;
		for (auto& x : cubes)
		{
			sb.mTransform =XMMatrixTranspose(XMLoadFloat4x4(&x.desc.transform)*camera->GetViewProjection());
			sb.mColor=x.desc.color;

			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

			GetDevice()->DrawIndexed(24, 0, 0, threadID);
		}
		cubes.clear();

		for (auto& x : renderableBoxes)
		{
			sb.mTransform = XMMatrixTranspose(XMLoadFloat4x4(&x.first)*camera->GetViewProjection());
			sb.mColor = x.second;

			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

			GetDevice()->DrawIndexed(24, 0, 0, threadID);
		}
		renderableBoxes.clear();

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawTranslators(Camera* camera, GRAPHICSTHREAD threadID)
{
	if(!renderableTranslators.empty())
	{
		GetDevice()->EventBegin("Translators", threadID);
		
		XMMATRIX VP = camera->GetViewProjection();

		MiscCB sb;
		for (auto& x : renderableTranslators)
		{
			if (!x->enabled)
				continue;

			XMMATRIX mat = XMMatrixScaling(x->dist, x->dist, x->dist)*XMMatrixTranslation(x->translation.x, x->translation.y, x->translation.z)*VP;
			XMMATRIX matX = XMMatrixTranspose(mat);
			XMMATRIX matY = XMMatrixTranspose(XMMatrixRotationZ(XM_PIDIV2)*XMMatrixRotationY(XM_PIDIV2)*mat);
			XMMATRIX matZ = XMMatrixTranspose(XMMatrixRotationY(-XM_PIDIV2)*XMMatrixRotationZ(-XM_PIDIV2)*mat);

			// Planes:
			{
				GPUBuffer* vbs[] = {
					wiTranslator::vertexBuffer_Plane,
				};
				const UINT strides[] = {
					sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
				};
				GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
				GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_TRANSLATOR_SOLIDPART], threadID);
			}

			// xy
			sb.mTransform = matX;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_XY ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.2f, 0.2f, 0, 0.2f);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Plane, 0, threadID);

			// xz
			sb.mTransform = matZ;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_XZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.2f, 0.2f, 0, 0.2f);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Plane, 0, threadID);

			// yz
			sb.mTransform = matY;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_YZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.2f, 0.2f, 0, 0.2f);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Plane, 0, threadID);

			// Lines:
			{
				GPUBuffer* vbs[] = {
					wiTranslator::vertexBuffer_Axis,
				};
				const UINT strides[] = {
					sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
				};
				GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
				GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_TRANSLATOR_WIREPART], threadID);
			}

			// x
			sb.mTransform = matX;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_X ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(1, 0, 0, 1);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Axis, 0, threadID);

			// y
			sb.mTransform = matY;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_Y ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0, 1, 0, 1);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Axis, 0, threadID);

			// z
			sb.mTransform = matZ;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_Z ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0, 0, 1, 1);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Axis, 0, threadID);

			// Origin:
			{
				GPUBuffer* vbs[] = {
					wiTranslator::vertexBuffer_Origin,
				};
				const UINT strides[] = {
					sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
				};
				GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
				sb.mTransform = XMMatrixTranspose(mat);
				sb.mColor = x->state == wiTranslator::TRANSLATOR_XYZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.5f, 0.5f, 0.5f, 1);
				GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
				GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_TRANSLATOR_SOLIDPART], threadID);
				GetDevice()->Draw(wiTranslator::vertexCount_Origin, 0, threadID);
			}
		}

		GetDevice()->EventEnd(threadID);

		renderableTranslators.clear();
	}
}
void wiRenderer::DrawDebugEnvProbes(Camera* camera, GRAPHICSTHREAD threadID)
{
	if (debugEnvProbes) 
	{
		GetDevice()->EventBegin("Debug EnvProbes", threadID);


		// Envmap spheres:

		GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_ENVPROBE], threadID);

		MiscCB sb;
		for (Model* model : GetScene().models)
		{
			for (auto& x : model->environmentProbes)
			{
				if (x->textureIndex < 0)
				{
					continue;
				}

				sb.mTransform = XMMatrixTranspose(XMMatrixTranslation(x->translation.x, x->translation.y, x->translation.z));
				GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

				GetDevice()->BindResource(PS, textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], TEXSLOT_ONDEMAND0, threadID, x->textureIndex);

				GetDevice()->Draw(2880, 0, threadID); // uv-sphere
			}
		}


		// Local proxy boxes:

		GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_CUBE], threadID);

		GPUBuffer* vbs[] = {
			&Cube::vertexBuffer,
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		GetDevice()->BindIndexBuffer(&Cube::indexBuffer, INDEXFORMAT_16BIT, 0, threadID);

		for (Model* model : GetScene().models)
		{
			for (auto& x : model->environmentProbes)
			{
				sb.mTransform = XMMatrixTranspose(XMLoadFloat4x4(&x->world)*camera->GetViewProjection());
				sb.mColor = XMFLOAT4(0, 1, 1, 1);

				GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

				GetDevice()->DrawIndexed(24, 0, 0, threadID);
			}
		}

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawDebugGridHelper(Camera* camera, GRAPHICSTHREAD threadID)
{
	if(gridHelper)
	{
		GetDevice()->EventBegin("GridHelper", threadID);

		GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_GRID], threadID);

		static float col = 0.7f;
		static int gridVertexCount = 0;
		static GPUBuffer* grid = nullptr;
		if (grid == nullptr)
		{
			const float h = 0.01f; // avoid z-fight with zero plane
			const int a = 20;
			XMFLOAT4 verts[((a+1) * 2 + (a+1) * 2) * 2];

			int count = 0;
			for (int i = 0; i <= a; ++i)
			{
				verts[count++] = XMFLOAT4(i - a*0.5f, h, -a*0.5f, 1);
				verts[count++] = (i == a / 2 ? XMFLOAT4(0, 0, 1, 1) : XMFLOAT4(col, col, col, 1));

				verts[count++] = XMFLOAT4(i - a*0.5f, h, +a*0.5f, 1);
				verts[count++] = (i == a / 2 ? XMFLOAT4(0, 0, 1, 1) : XMFLOAT4(col, col, col, 1));
			}
			for (int j = 0; j <= a; ++j)
			{
				verts[count++] = XMFLOAT4(-a*0.5f, h, j - a*0.5f, 1);
				verts[count++] = (j == a / 2 ? XMFLOAT4(1, 0, 0, 1) : XMFLOAT4(col, col, col, 1));

				verts[count++] = XMFLOAT4(+a*0.5f, h, j - a*0.5f, 1);
				verts[count++] = (j == a / 2 ? XMFLOAT4(1, 0, 0, 1) : XMFLOAT4(col, col, col, 1));
			}

			gridVertexCount = ARRAYSIZE(verts) / 2;

			GPUBufferDesc bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_IMMUTABLE;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = verts;
			grid = new GPUBuffer;
			wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, grid);
		}

		MiscCB sb;
		sb.mTransform = XMMatrixTranspose(camera->GetViewProjection());
		sb.mColor = XMFLOAT4(1, 1, 1, 1);

		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

		GPUBuffer* vbs[] = {
			grid,
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		GetDevice()->Draw(gridVertexCount, 0, threadID);

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawDebugVoxels(Camera* camera, GRAPHICSTHREAD threadID)
{
	if (voxelHelper && textures[TEXTYPE_3D_VOXELRADIANCE] != nullptr) 
	{
		GetDevice()->EventBegin("Debug Voxels", threadID);

		GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_VOXEL], threadID);


		MiscCB sb;
		sb.mTransform = XMMatrixTranspose(XMMatrixTranslationFromVector(XMLoadFloat3(&voxelSceneData.center)) * camera->GetViewProjection());
		sb.mColor = XMFLOAT4(1, 1, 1, 1);

		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

		GetDevice()->Draw(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res, 0, threadID);

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawDebugEmitters(Camera* camera, GRAPHICSTHREAD threadID)
{
	if (debugEmitters || !renderableBoxes.empty()) 
	{
		GetDevice()->EventBegin("DebugEmitters", threadID);
		
		GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_EMITTER], threadID);

		MiscCB sb;
		for (auto& x : emitterSystems)
		{
			if (x->object != nullptr && x->object->mesh != nullptr)
			{
				sb.mTransform = XMMatrixTranspose(XMLoadFloat4x4(&x->object->world)*camera->GetViewProjection());
				sb.mColor = XMFLOAT4(0, 1, 0, 1);
				GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

				GPUBuffer* vbs[] = {
					x->object->mesh->vertexBuffer_POS,
				};
				const UINT strides[] = {
					sizeof(Mesh::Vertex_POS),
				};
				GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
				GetDevice()->BindIndexBuffer(x->object->mesh->indexBuffer, x->object->mesh->GetIndexFormat(), 0, threadID);

				GetDevice()->DrawIndexed((int)x->object->mesh->indices.size(), 0, 0, threadID);
			}
		}

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawDebugForceFields(Camera* camera, GRAPHICSTHREAD threadID)
{
	if (debugForceFields) 
	{
		GetDevice()->EventBegin("DebugForceFields", threadID);

		MiscCB sb;
		uint32_t i = 0;
		for (auto& model : GetScene().models)
		{
			for (ForceField* force : model->forces)
			{
				sb.mTransform = XMMatrixTranspose(camera->GetViewProjection());
				sb.mColor = XMFLOAT4(camera->translation.x, camera->translation.y, camera->translation.z, (float)i);
				GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

				switch (force->type)
				{
				case ENTITY_TYPE_FORCEFIELD_POINT:
					GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_FORCEFIELD_POINT], threadID);
					GetDevice()->Draw(2880, 0, threadID); // uv-sphere
					break;
				case ENTITY_TYPE_FORCEFIELD_PLANE:
					GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_FORCEFIELD_PLANE], threadID);
					GetDevice()->Draw(14, 0, threadID); // box
					break;
				}

				++i;
			}
		}

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawDebugCameras(Camera* camera, GRAPHICSTHREAD threadID)
{
	if (debugCameras)
	{
		GetDevice()->EventBegin("DebugCameras", threadID);

		GetDevice()->BindGraphicsPSO(PSO_debug[DEBUGRENDERING_CUBE], threadID);

		GPUBuffer* vbs[] = {
			&Cube::vertexBuffer,
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		GetDevice()->BindIndexBuffer(&Cube::indexBuffer, INDEXFORMAT_16BIT, 0, threadID);

		MiscCB sb;
		sb.mColor = XMFLOAT4(1, 1, 1, 1);

		for (auto& model : GetScene().models)
		{
			for (auto& x : model->cameras)
			{
				sb.mTransform = XMMatrixTranspose(XMLoadFloat4x4(&x->world)*camera->GetViewProjection());

				GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

				GetDevice()->DrawIndexed(24, 0, 0, threadID);
			}
		}

		GetDevice()->EventEnd(threadID);
	}
}

void wiRenderer::DrawSoftParticles(Camera* camera, bool distortion, GRAPHICSTHREAD threadID)
{
	// todo: remove allocation of vector
	vector<wiEmittedParticle*> sortedEmitters(emitterSystems.begin(), emitterSystems.end());
	std::sort(sortedEmitters.begin(), sortedEmitters.end(), [&](const wiEmittedParticle* a, const wiEmittedParticle* b) {
		return wiMath::DistanceSquared(camera->translation, a->GetPosition()) > wiMath::DistanceSquared(camera->translation, b->GetPosition());
	});

	for (wiEmittedParticle* e : sortedEmitters)
	{
		if (distortion && e->shaderType == wiEmittedParticle::SOFT_DISTORTION)
		{
			e->Draw(threadID);
		}
		else if (!distortion && (e->shaderType == wiEmittedParticle::SOFT || e->shaderType == wiEmittedParticle::SIMPLEST || IsWireRender()))
		{
			e->Draw(threadID);
		}
	}
}
void wiRenderer::DrawTrails(GRAPHICSTHREAD threadID, Texture2D* refracRes)
{
	//if (objectsWithTrails.empty())
	//{
	//	return;
	//}

	//GetDevice()->EventBegin("RibbonTrails", threadID);

	//GetDevice()->BindPrimitiveTopology(TRIANGLESTRIP,threadID);
	//GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_TRAIL],threadID);

	//GetDevice()->BindRasterizerState(wireRender?rasterizers[RSTYPE_WIRE_DOUBLESIDED]:rasterizers[RSTYPE_DOUBLESIDED],threadID);
	//GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT],STENCILREF_EMPTY,threadID);
	//GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE],threadID);

	//GetDevice()->BindPS(pixelShaders[PSTYPE_TRAIL],threadID);
	//GetDevice()->BindVS(vertexShaders[VSTYPE_TRAIL],threadID);
	//
	//GetDevice()->BindResource(PS, refracRes,TEXSLOT_ONDEMAND0,threadID);

	//for (Object* o : objectsWithTrails)
	//{
	//	if (o->trail.size() >= 4)
	//	{

	//		GetDevice()->BindResource(PS, o->trailDistortTex, TEXSLOT_ONDEMAND1, threadID);
	//		GetDevice()->BindResource(PS, o->trailTex, TEXSLOT_ONDEMAND2, threadID);

	//		std::vector<RibbonVertex> trails;

	//		int bounds = (int)o->trail.size();
	//		trails.reserve(bounds * 10);
	//		int req = bounds - 3;
	//		for (int k = 0; k < req; k += 2)
	//		{
	//			static const float trailres = 10.f;
	//			for (float r = 0.0f; r <= 1.0f; r += 1.0f / trailres)
	//			{
	//				XMVECTOR point0 = XMVectorCatmullRom(
	//					XMLoadFloat3(&o->trail[k ? (k - 2) : 0].pos)
	//					, XMLoadFloat3(&o->trail[k].pos)
	//					, XMLoadFloat3(&o->trail[k + 2].pos)
	//					, XMLoadFloat3(&o->trail[k + 6 < bounds ? (k + 6) : (bounds - 2)].pos)
	//					, r
	//				),
	//					point1 = XMVectorCatmullRom(
	//						XMLoadFloat3(&o->trail[k ? (k - 1) : 1].pos)
	//						, XMLoadFloat3(&o->trail[k + 1].pos)
	//						, XMLoadFloat3(&o->trail[k + 3].pos)
	//						, XMLoadFloat3(&o->trail[k + 5 < bounds ? (k + 5) : (bounds - 1)].pos)
	//						, r
	//					);
	//				XMFLOAT3 xpoint0, xpoint1;
	//				XMStoreFloat3(&xpoint0, point0);
	//				XMStoreFloat3(&xpoint1, point1);
	//				trails.push_back(RibbonVertex(xpoint0
	//					, wiMath::Lerp(XMFLOAT2((float)k / (float)bounds, 0), XMFLOAT2((float)(k + 1) / (float)bounds, 0), r)
	//					, wiMath::Lerp(o->trail[k].col, o->trail[k + 2].col, r)
	//					, 1
	//				));
	//				trails.push_back(RibbonVertex(xpoint1
	//					, wiMath::Lerp(XMFLOAT2((float)k / (float)bounds, 1), XMFLOAT2((float)(k + 1) / (float)bounds, 1), r)
	//					, wiMath::Lerp(o->trail[k + 1].col, o->trail[k + 3].col, r)
	//					, 1
	//				));
	//			}
	//		}
	//		if (!trails.empty())
	//		{
	//			UINT trailOffset;
	//			void* buffer = GetDevice()->AllocateFromRingBuffer(dynamicVertexBufferPool, sizeof(RibbonVertex)*trails.size(), trailOffset, threadID);
	//			memcpy(buffer, trails.data(), sizeof(RibbonVertex)*trails.size());
	//			GetDevice()->InvalidateBufferAccess(dynamicVertexBufferPool, threadID);

	//			const GPUBuffer* vbs[] = {
	//				dynamicVertexBufferPool
	//			};
	//			const UINT strides[] = {
	//				sizeof(RibbonVertex)
	//			};
	//			const UINT offsets[] = {
	//				trailOffset
	//			};
	//			GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
	//			GetDevice()->Draw((int)trails.size(), 0, threadID);

	//			trails.clear();
	//		}
	//	}
	//}

	//GetDevice()->EventEnd(threadID);
}
void wiRenderer::DrawImagesAdd(GRAPHICSTHREAD threadID, Texture2D* refracRes){
	imagesRTAdd.Activate(threadID,0,0,0,1);
	//wiImage::BatchBegin(threadID);
	for(wiSprite* x : images){
		if(x->effects.blendFlag==BLENDMODE_ADDITIVE){
			/*Texture2D* nor = x->effects.normalMap;
			x->effects.setNormalMap(nullptr);
			bool changedBlend=false;
			if(x->effects.blendFlag==BLENDMODE_OPAQUE && nor){
				x->effects.blendFlag=BLENDMODE_ADDITIVE;
				changedBlend=true;
			}*/
			x->Draw(refracRes,  threadID);
			/*if(changedBlend)
				x->effects.blendFlag=BLENDMODE_OPAQUE;
			x->effects.setNormalMap(nor);*/
		}
	}
}
void wiRenderer::DrawImages(GRAPHICSTHREAD threadID, Texture2D* refracRes){
	imagesRT.Activate(threadID,0,0,0,0);
	//wiImage::BatchBegin(threadID);
	for(wiSprite* x : images){
		if(x->effects.blendFlag==BLENDMODE_ALPHA || x->effects.blendFlag==BLENDMODE_OPAQUE){
			/*Texture2D* nor = x->effects.normalMap;
			x->effects.setNormalMap(nullptr);
			bool changedBlend=false;
			if(x->effects.blendFlag==BLENDMODE_OPAQUE && nor){
				x->effects.blendFlag=BLENDMODE_ADDITIVE;
				changedBlend=true;
			}*/
			x->Draw(refracRes,  threadID);
			/*if(changedBlend)
				x->effects.blendFlag=BLENDMODE_OPAQUE;
			x->effects.setNormalMap(nor);*/
		}
	}
}
void wiRenderer::DrawImagesNormals(GRAPHICSTHREAD threadID, Texture2D* refracRes){
	normalMapRT.Activate(threadID,0,0,0,0);
	//wiImage::BatchBegin(threadID);
	for(wiSprite* x : images){
		x->DrawNormal(threadID);
	}
}
void wiRenderer::DrawLights(Camera* camera, GRAPHICSTHREAD threadID)
{
	const FrameCulling& culling = frameCullings[camera];
	const CulledList& culledLights = culling.culledLights;

	GetDevice()->EventBegin("Light Render", threadID);
	wiProfiler::GetInstance().BeginRange("Light Render", wiProfiler::DOMAIN_GPU, threadID);

	// Environmental light (envmap + voxelGI) is always drawn
	{
		GetDevice()->BindGraphicsPSO(PSO_enviromentallight, threadID);
		GetDevice()->Draw(3, 0, threadID); // full screen triangle
	}

	for (int type = 0; type < Light::LIGHTTYPE_COUNT; ++type)
	{
		GetDevice()->BindGraphicsPSO(PSO_deferredlight[type], threadID);

		for (Cullable* c : culledLights)
		{
			Light* l = (Light*)c;
			if (l->GetType() != type || !l->IsActive())
				continue;

			switch (type)
			{
			case Light::DIRECTIONAL:
			case Light::SPHERE:
			case Light::DISC:
			case Light::RECTANGLE:
			case Light::TUBE:
				{
					MiscCB miscCb;
					miscCb.mColor.x = (float)l->entityArray_index;
					GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &miscCb, threadID);

					GetDevice()->Draw(3, 0, threadID); // full screen triangle
				}
				break;
			case Light::POINT:
				{
					MiscCB miscCb;
					miscCb.mColor.x = (float)l->entityArray_index;
					float sca = l->enerDis.y + 1;
					miscCb.mTransform = XMMatrixTranspose(XMMatrixScaling(sca, sca, sca)*XMMatrixTranslation(l->translation.x, l->translation.y, l->translation.z) * camera->GetViewProjection());
					GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &miscCb, threadID);

					GetDevice()->Draw(240, 0, threadID); // icosphere
				}
				break;
			case Light::SPOT:
				{
					MiscCB miscCb;
					miscCb.mColor.x = (float)l->entityArray_index;
					const float coneS = (const float)(l->enerDis.z / XM_PIDIV4);
					miscCb.mTransform = XMMatrixTranspose(
						XMMatrixScaling(coneS*l->enerDis.y, l->enerDis.y, coneS*l->enerDis.y)*
						XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))*
						XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation)) *
						camera->GetViewProjection()
					);
					GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &miscCb, threadID);

					GetDevice()->Draw(192, 0, threadID); // cone
				}
				break;
			}
		}


	}

	wiProfiler::GetInstance().EndRange(threadID);
	GetDevice()->EventEnd(threadID);
}
void wiRenderer::DrawLightVisualizers(Camera* camera, GRAPHICSTHREAD threadID)
{
	const FrameCulling& culling = frameCullings[camera];
	const CulledList& culledLights = culling.culledLights;

	if(!culledLights.empty())
	{
		GetDevice()->EventBegin("Light Visualizer Render", threadID);

		GetDevice()->BindConstantBuffer(PS, constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), threadID);
		GetDevice()->BindConstantBuffer(VS, constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), threadID);


		for (int type = Light::POINT; type < Light::LIGHTTYPE_COUNT; ++type)
		{
			GetDevice()->BindGraphicsPSO(PSO_lightvisualizer[type], threadID);

			for (Cullable* c : culledLights) 
			{
				Light* l = (Light*)c;
				if (l->GetType() == type && l->noHalo == false) {

					VolumeLightCB lcb;
					lcb.col = l->color;
					lcb.enerdis = l->enerDis;

					if (type == Light::POINT) 
					{
						lcb.enerdis.w = l->enerDis.y*l->enerDis.x*0.01f; // scale
						lcb.world = XMMatrixTranspose(
							XMMatrixScaling(lcb.enerdis.w, lcb.enerdis.w, lcb.enerdis.w)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&camera->rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation))
						);

						GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, threadID);

						GetDevice()->Draw(108, 0, threadID); // circle
					}
					else if(type == Light::SPOT)
					{
						float coneS = (float)(l->enerDis.z / 0.7853981852531433);
						lcb.enerdis.w = l->enerDis.y*l->enerDis.x*0.03f; // scale
						lcb.world = XMMatrixTranspose(
							XMMatrixScaling(coneS*lcb.enerdis.w, lcb.enerdis.w, coneS*lcb.enerdis.w)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation))
						);

						GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, threadID);

						GetDevice()->Draw(192, 0, threadID); // cone
					}
					else if (type == Light::SPHERE)
					{
						lcb.world = XMMatrixTranspose(
							XMMatrixScaling(l->radius, l->radius, l->radius)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation))*
							camera->GetViewProjection()
						);

						GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, threadID);

						GetDevice()->Draw(2880, 0, threadID); // uv-sphere
					}
					else if (type == Light::DISC)
					{
						lcb.world = XMMatrixTranspose(
							XMMatrixScaling(l->radius, l->radius, l->radius)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation))*
							camera->GetViewProjection()
						);

						GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, threadID);

						GetDevice()->Draw(108, 0, threadID); // circle
					}
					else if (type == Light::RECTANGLE)
					{
						lcb.world = XMMatrixTranspose(
							XMMatrixScaling(l->width * 0.5f, l->height * 0.5f, 0.5f)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation))*
							camera->GetViewProjection()
						);

						GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, threadID);

						GetDevice()->Draw(6, 0, threadID); // quad
					}
					else if (type == Light::TUBE)
					{
						lcb.world = XMMatrixTranspose(
							XMMatrixScaling(max(l->width * 0.5f, l->radius), l->radius, l->radius)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation))*
							camera->GetViewProjection()
						);

						GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_VOLUMELIGHT], &lcb, threadID);

						GetDevice()->Draw(384, 0, threadID); // cylinder
					}
				}
			}

		}

		GetDevice()->EventEnd(threadID);
	}


}
void wiRenderer::DrawVolumeLights(Camera* camera, GRAPHICSTHREAD threadID)
{
	const FrameCulling& culling = frameCullings[camera];
	const CulledList& culledLights = culling.culledLights;

	if (!culledLights.empty())
	{
		GetDevice()->EventBegin("Volumetric Light Render", threadID);


		for (int type = 0; type < Light::LIGHTTYPE_COUNT; ++type)
		{
			GraphicsPSO* pso = PSO_volumetriclight[type];

			if (pso == nullptr)
			{
				continue;
			}

			GetDevice()->BindGraphicsPSO(pso, threadID);

			for (Cullable* c : culledLights)
			{
				Light* l = (Light*)c;
				if (l->GetType() == type && l->volumetrics)
				{

					switch (type)
					{
					case Light::DIRECTIONAL:
					case Light::SPHERE:
					case Light::DISC:
					case Light::RECTANGLE:
					case Light::TUBE:
					{
						MiscCB miscCb;
						miscCb.mColor.x = (float)l->entityArray_index;
						GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &miscCb, threadID);

						GetDevice()->Draw(3, 0, threadID); // full screen triangle
					}
					break;
					case Light::POINT:
					{
						MiscCB miscCb;
						miscCb.mColor.x = (float)l->entityArray_index;
						float sca = l->enerDis.y + 1;
						miscCb.mTransform = XMMatrixTranspose(XMMatrixScaling(sca, sca, sca)*XMMatrixTranslation(l->translation.x, l->translation.y, l->translation.z) * camera->GetViewProjection());
						GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &miscCb, threadID);

						GetDevice()->Draw(240, 0, threadID); // icosphere
					}
					break;
					case Light::SPOT:
					{
						MiscCB miscCb;
						miscCb.mColor.x = (float)l->entityArray_index;
						const float coneS = (const float)(l->enerDis.z / XM_PIDIV4);
						miscCb.mTransform = XMMatrixTranspose(
							XMMatrixScaling(coneS*l->enerDis.y, l->enerDis.y, coneS*l->enerDis.y)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))*
							XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation)) *
							camera->GetViewProjection()
						);
						GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &miscCb, threadID);

						GetDevice()->Draw(192, 0, threadID); // cone
					}
					break;
					}

				}
			}

		}

		GetDevice()->EventEnd(threadID);
	}


}
void wiRenderer::DrawLensFlares(GRAPHICSTHREAD threadID)
{
	const FrameCulling& culling = frameCullings[getCamera()];
	const CulledList& culledLights = culling.culledLights;

	for(Cullable* c: culledLights)
	{
		Light* l = (Light*)c;

		if(!l->lensFlareRimTextures.empty())
		{

			XMVECTOR POS;

			if(l->GetType() ==Light::POINT || l->GetType() ==Light::SPOT){
				POS = XMLoadFloat3(&l->translation);
			}

			else{
				POS = XMVector3Normalize(
					-XMVector3Transform(XMVectorSet(0, -1, 0, 1), XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation)))
				) * 100000;
			}
			
			XMVECTOR flarePos = XMVector3Project(POS,0.f,0.f,(float)GetInternalResolution().x,(float)GetInternalResolution().y,0.0f,1.0f,getCamera()->GetRealProjection(),getCamera()->GetView(),XMMatrixIdentity());

			if( XMVectorGetX(XMVector3Dot( XMVectorSubtract(POS,getCamera()->GetEye()),getCamera()->GetAt() ))>0 )
				wiLensFlare::Draw(threadID,flarePos,l->lensFlareRimTextures);

		}

	}
}

void wiRenderer::SetShadowProps2D(int resolution, int count, int softShadowQuality)
{
	SHADOWRES_2D = resolution;
	SHADOWCOUNT_2D = count;
	SOFTSHADOWQUALITY_2D = softShadowQuality;

	SAFE_DELETE(Light::shadowMapArray_2D);
	Light::shadowMapArray_2D = new Texture2D;
	Light::shadowMapArray_2D->RequestIndependentRenderTargetArraySlices(true);

	SAFE_DELETE(Light::shadowMapArray_Transparent);
	Light::shadowMapArray_Transparent = new Texture2D;
	Light::shadowMapArray_Transparent->RequestIndependentRenderTargetArraySlices(true);

	TextureDesc desc;
	ZeroMemory(&desc, sizeof(desc));
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
	GetDevice()->CreateTexture2D(&desc, nullptr, &Light::shadowMapArray_2D);

	desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
	desc.Format = RTFormat_ldr;
	GetDevice()->CreateTexture2D(&desc, nullptr, &Light::shadowMapArray_Transparent);
}
void wiRenderer::SetShadowPropsCube(int resolution, int count)
{
	SHADOWRES_CUBE = resolution;
	SHADOWCOUNT_CUBE = count;

	SAFE_DELETE(Light::shadowMapArray_Cube);
	Light::shadowMapArray_Cube = new Texture2D;
	Light::shadowMapArray_Cube->RequestIndependentRenderTargetArraySlices(true);
	Light::shadowMapArray_Cube->RequestIndependentRenderTargetCubemapFaces(false);

	TextureDesc desc;
	ZeroMemory(&desc, sizeof(desc));
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
	GetDevice()->CreateTexture2D(&desc, nullptr, &Light::shadowMapArray_Cube);
}
void wiRenderer::DrawForShadowMap(GRAPHICSTHREAD threadID, uint32_t layerMask)
{
	if (wireRender)
		return;

	// We need to render shadows even if the gamespeed is 0 for these reasons:
	// 1.) Shadow cascades is updated every time according to camera
	// 2.) We can move any other light, or object, too

	//if (GetGameSpeed() > 0) 
	{
		GetDevice()->EventBegin("ShadowMap Render", threadID);
		wiProfiler::GetInstance().BeginRange("Shadow Rendering", wiProfiler::DOMAIN_GPU, threadID);

		const bool all_layers = layerMask == 0xFFFFFFFF; // this can avoid the recursive call per object : GetLayerMask()

		const FrameCulling& culling = frameCullings[getCamera()];
		const CulledList& culledLights = culling.culledLights;

		ViewPort vp;

		// RGB: Shadow tint (multiplicative), A: Refraction caustics(additive)
		const float transparentShadowClearColor[] = { 1,1,1,0 };

		if (!culledLights.empty())
		{
			GetDevice()->UnBindResources(TEXSLOT_SHADOWARRAY_2D, 2, threadID);

			int shadowCounter_2D = 0;
			int shadowCounter_Cube = 0;
			for (int type = 0; type < Light::LIGHTTYPE_COUNT; ++type)
			{
				switch (type)
				{
				case Light::DIRECTIONAL:
				case Light::SPOT:
				{
					vp.TopLeftX = 0;
					vp.TopLeftY = 0;
					vp.Width = (float)SHADOWRES_2D;
					vp.Height = (float)SHADOWRES_2D;
					vp.MinDepth = 0.0f;
					vp.MaxDepth = 1.0f;
					GetDevice()->BindViewports(1, &vp, threadID);
					break;
				}
				break;
				case Light::POINT:
				case Light::SPHERE:
				case Light::DISC:
				case Light::RECTANGLE:
				case Light::TUBE:
				{
					vp.TopLeftX = 0;
					vp.TopLeftY = 0;
					vp.Width = (float)SHADOWRES_CUBE;
					vp.Height = (float)SHADOWRES_CUBE;
					vp.MinDepth = 0.0f;
					vp.MaxDepth = 1.0f;
					GetDevice()->BindViewports(1, &vp, threadID);

					GetDevice()->BindConstantBuffer(GS, constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubeMapRenderCB), threadID);
					break;
				}
				break;
				default:
					break;
				}

				for (Cullable* c : culledLights)
				{
					Light* l = (Light*)c;
					if (l->GetType() != type || !l->shadow || !l->IsActive())
					{
						continue;
					}

					switch (type)
					{
					case Light::DIRECTIONAL:
					{
						if ((shadowCounter_2D + 2) >= SHADOWCOUNT_2D || l->shadowMap_index < 0 || l->shadowCam_dirLight.empty())
							break;
						shadowCounter_2D += 3; // shadow indices are already complete so a shadow slot is consumed here even if no rendering actually happens!

						for (int cascade = 0; cascade < 3; ++cascade)
						{
							const float siz = l->shadowCam_dirLight[cascade].size * 0.5f;
							const float f = l->shadowCam_dirLight[cascade].farplane * 0.5f;
							AABB boundingbox;
							boundingbox.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(siz, siz, f));
							if (spTree != nullptr)
							{
								CulledList culledObjects;
								CulledCollection culledRenderer;
								spTree->getVisible(boundingbox.get(XMMatrixInverse(0, XMLoadFloat4x4(&l->shadowCam_dirLight[cascade].View))), culledObjects);
								bool transparentShadowsRequested = false;
								for (Cullable* x : culledObjects)
								{
									Object* object = (Object*)x;
									if (cascade < object->cascadeMask)
									{
										continue;
									}
									if (all_layers || (layerMask & object->GetLayerMask()))
									{
										if (object->IsCastingShadow())
										{
											culledRenderer[object->mesh].push_front(object);

											if (object->GetRenderTypes() & RENDERTYPE_TRANSPARENT || object->GetRenderTypes() & RENDERTYPE_WATER)
											{
												transparentShadowsRequested = true;
											}
										}
									}
								}
								if (!culledRenderer.empty())
								{
									CameraCB cb;
									cb.mVP = l->shadowCam_dirLight[cascade].getVP();
									GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_CAMERA], &cb, threadID);

									GetDevice()->ClearDepthStencil(Light::shadowMapArray_2D, CLEAR_DEPTH, 0.0f, 0, threadID, l->shadowMap_index + cascade);

									// unfortunately we will always have to clear the associated transparent shadowmap to avoid discrepancy with shadowmap indexing changes across frames
									GetDevice()->ClearRenderTarget(Light::shadowMapArray_Transparent, transparentShadowClearColor, threadID, l->shadowMap_index + cascade);

									// render opaque shadowmap:
									GetDevice()->BindRenderTargets(0, nullptr, Light::shadowMapArray_2D, threadID, l->shadowMap_index + cascade);
									RenderMeshes(l->shadowCam_dirLight[cascade].Eye, culledRenderer, SHADERTYPE_SHADOW, RENDERTYPE_OPAQUE, threadID);

									if (GetTransparentShadowsEnabled() && transparentShadowsRequested)
									{
										// render transparent shadowmap:
										Texture2D* rts[] = {
											Light::shadowMapArray_Transparent
										};
										GetDevice()->BindRenderTargets(ARRAYSIZE(rts), rts, Light::shadowMapArray_2D, threadID, l->shadowMap_index + cascade);
										RenderMeshes(l->shadowCam_dirLight[cascade].Eye, culledRenderer, SHADERTYPE_SHADOW, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, threadID);
									}
								}
							}
						}
					}
					break;
					case Light::SPOT:
					{
						if (shadowCounter_2D >= SHADOWCOUNT_2D || l->shadowMap_index < 0 || l->shadowCam_spotLight.empty())
							break;
						shadowCounter_2D++; // shadow indices are already complete so a shadow slot is consumed here even if no rendering actually happens!

						Frustum frustum;
						frustum.ConstructFrustum(l->shadowCam_spotLight[0].farplane, l->shadowCam_spotLight[0].realProjection, l->shadowCam_spotLight[0].View);
						if (spTree != nullptr)
						{
							CulledList culledObjects;
							CulledCollection culledRenderer;
							spTree->getVisible(frustum, culledObjects);
							bool transparentShadowsRequested = false;
							for (Cullable* x : culledObjects)
							{
								Object* object = (Object*)x;
								if (all_layers || (layerMask & object->GetLayerMask()))
								{
									if (object->IsCastingShadow())
									{
										culledRenderer[object->mesh].push_front(object);

										if (object->GetRenderTypes() & RENDERTYPE_TRANSPARENT || object->GetRenderTypes() & RENDERTYPE_WATER)
										{
											transparentShadowsRequested = true;
										}
									}
								}
							}
							if (!culledRenderer.empty())
							{
								CameraCB cb;
								cb.mVP = l->shadowCam_spotLight[0].getVP();
								GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_CAMERA], &cb, threadID);

								GetDevice()->ClearDepthStencil(Light::shadowMapArray_2D, CLEAR_DEPTH, 0.0f, 0, threadID, l->shadowMap_index);

								// unfortunately we will always have to clear the associated transparent shadowmap to avoid discrepancy with shadowmap indexing changes across frames
								GetDevice()->ClearRenderTarget(Light::shadowMapArray_Transparent, transparentShadowClearColor, threadID, l->shadowMap_index);

								// render opaque shadowmap:
								GetDevice()->BindRenderTargets(0, nullptr, Light::shadowMapArray_2D, threadID, l->shadowMap_index);
								RenderMeshes(l->translation, culledRenderer, SHADERTYPE_SHADOW, RENDERTYPE_OPAQUE, threadID);

								if (GetTransparentShadowsEnabled() && transparentShadowsRequested)
								{
									// render transparent shadowmap:
									Texture2D* rts[] = {
										Light::shadowMapArray_Transparent
									};
									GetDevice()->BindRenderTargets(ARRAYSIZE(rts), rts, Light::shadowMapArray_2D, threadID, l->shadowMap_index);
									RenderMeshes(l->translation, culledRenderer, SHADERTYPE_SHADOW, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, threadID);
								}
							}
						}
					}
					break;
					case Light::POINT:
					case Light::SPHERE:
					case Light::DISC:
					case Light::RECTANGLE:
					case Light::TUBE:
					{
						if (shadowCounter_Cube >= SHADOWCOUNT_CUBE || l->shadowMap_index < 0 || l->shadowCam_pointLight.empty())
							break;
						shadowCounter_Cube++; // shadow indices are already complete so a shadow slot is consumed here even if no rendering actually happens!

						if (spTree != nullptr)
						{
							CulledList culledObjects;
							CulledCollection culledRenderer;
							spTree->getVisible(l->bounds, culledObjects);
							for (Cullable* x : culledObjects)
							{
								Object* object = (Object*)x;

								if (all_layers || (layerMask & object->GetLayerMask()))
								{
									if (object->IsCastingShadow())
									{
										culledRenderer[object->mesh].push_front(object);
									}
								}
							}
							if (!culledRenderer.empty())
							{
								GetDevice()->BindRenderTargets(0, nullptr, Light::shadowMapArray_Cube, threadID, l->shadowMap_index);
								GetDevice()->ClearDepthStencil(Light::shadowMapArray_Cube, CLEAR_DEPTH, 0.0f, 0, threadID, l->shadowMap_index);

								MiscCB miscCb;
								miscCb.mColor = XMFLOAT4(l->translation.x, l->translation.y, l->translation.z, 1.0f / l->GetRange()); // reciprocal range, to avoid division in shader
								GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &miscCb, threadID);

								CubeMapRenderCB cb;
								for (unsigned int shcam = 0; shcam < l->shadowCam_pointLight.size(); ++shcam)
									cb.mViewProjection[shcam] = l->shadowCam_pointLight[shcam].getVP();

								GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_CUBEMAPRENDER], &cb, threadID);

								RenderMeshes(l->translation, culledRenderer, SHADERTYPE_SHADOWCUBE, RENDERTYPE_OPAQUE, threadID);
							}
						}
					}
					break;
					} // terminate switch
				}

			}

			GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);
		}


		wiProfiler::GetInstance().EndRange(); // Shadow Rendering
		GetDevice()->EventEnd(threadID);
	}

	GetDevice()->BindResource(PS, Light::shadowMapArray_2D, TEXSLOT_SHADOWARRAY_2D, threadID);
	GetDevice()->BindResource(PS, Light::shadowMapArray_Cube, TEXSLOT_SHADOWARRAY_CUBE, threadID);
	if (GetTransparentShadowsEnabled())
	{
		GetDevice()->BindResource(PS, Light::shadowMapArray_Transparent, TEXSLOT_SHADOWARRAY_TRANSPARENT, threadID);
	}
}

void wiRenderer::RenderMeshes(const XMFLOAT3& eye, const CulledCollection& culledRenderer, SHADERTYPE shaderType, UINT renderTypeFlags, GRAPHICSTHREAD threadID,
	bool tessellation, bool occlusionCulling, uint32_t layerMask)
{
	// Intensive section, refactor and optimize!

	if (!culledRenderer.empty())
	{
		GraphicsDevice* device = GetDevice();

		device->EventBegin("RenderMeshes", threadID);

		tessellation = tessellation && device->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION);

		const XMFLOAT4X4 __identityMat = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
		XMFLOAT4X4 tempMat;

		struct InstBuf
		{
			Instance instance;
			InstancePrev instancePrev;
		};

		const bool advancedVBRequest =
			!IsWireRender() &&
			(shaderType == SHADERTYPE_FORWARD ||
			shaderType == SHADERTYPE_DEFERRED ||
			shaderType == SHADERTYPE_TILEDFORWARD);

		const bool easyTextureBind = 
			shaderType == SHADERTYPE_TEXTURE || 
			shaderType == SHADERTYPE_SHADOW || 
			shaderType == SHADERTYPE_SHADOWCUBE || 
			shaderType == SHADERTYPE_DEPTHONLY || 
			shaderType == SHADERTYPE_VOXELIZE;

		const bool all_layers = layerMask == 0xFFFFFFFF; // this can avoid the recursive call per object : GetLayerMask()

		GraphicsPSO* impostorRequest = GetImpostorPSO(shaderType);

		// Render impostors:
		if (impostorRequest != nullptr)
		{
			bool impostorGraphicsStateComplete = false;

			for (CulledCollection::const_iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter)
			{
				Mesh* mesh = iter->first;
				if (!mesh->renderable || !mesh->hasImpostor() || !(mesh->GetRenderTypes() & renderTypeFlags))
				{
					continue;
				}

				const CulledObjectList& visibleInstances = iter->second;

				UINT instancesOffset;
				size_t alloc_size = visibleInstances.size();
				alloc_size *= advancedVBRequest ? sizeof(InstBuf) : sizeof(Instance);
				void* instances = device->AllocateFromRingBuffer(dynamicVertexBufferPool, alloc_size, instancesOffset, threadID);

				int k = 0;
				for (const Object* instance : visibleInstances)
				{
					if (occlusionCulling && instance->IsOccluded())
						continue;

					if (all_layers || (layerMask & instance->GetLayerMask()))
					{
						const float impostorThreshold = instance->bounds.getRadius();
						float dist = wiMath::Distance(eye, instance->bounds.getCenter());
						float dither = instance->transparency;
						dither = wiMath::SmoothStep(1.0f, dither, wiMath::Clamp((dist - mesh->impostorDistance) / impostorThreshold, 0, 1));
						if (dither > 1.0f - FLT_EPSILON)
							continue;

						XMMATRIX boxMat = mesh->aabb.getAsBoxMatrix();

						XMStoreFloat4x4(&tempMat, boxMat*XMLoadFloat4x4(&instance->world));

						if (advancedVBRequest)
						{
							((volatile InstBuf*)instances)[k].instance.Create(tempMat, dither, instance->color);

							XMStoreFloat4x4(&tempMat, boxMat*XMLoadFloat4x4(&instance->worldPrev));
							((volatile InstBuf*)instances)[k].instancePrev.Create(tempMat);
						}
						else
						{
							((volatile Instance*)instances)[k].Create(tempMat, dither, instance->color);
						}

						++k;
					}
				}

				device->InvalidateBufferAccess(dynamicVertexBufferPool, threadID);

				if (k < 1)
					continue;

				if (!advancedVBRequest || IsWireRender())
				{
					GPUBuffer* vbs[] = {
						&Mesh::impostorVB_POS,
						&Mesh::impostorVB_TEX,
						dynamicVertexBufferPool
					};
					UINT strides[] = {
						sizeof(Mesh::Vertex_POS),
						sizeof(Mesh::Vertex_TEX),
						sizeof(Instance)
					};
					UINT offsets[] = {
						0,
						0,
						instancesOffset
					};
					device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
				}
				else
				{
					GPUBuffer* vbs[] = {
						&Mesh::impostorVB_POS,
						&Mesh::impostorVB_TEX,
						&Mesh::impostorVB_POS,
						dynamicVertexBufferPool
					};
					UINT strides[] = {
						sizeof(Mesh::Vertex_POS),
						sizeof(Mesh::Vertex_TEX),
						sizeof(Mesh::Vertex_POS),
						sizeof(InstBuf)
					};
					UINT offsets[] = {
						0,
						0,
						0,
						instancesOffset
					};
					device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
				}

				GPUResource* res[] = {
					mesh->impostorTarget.GetTexture(0),
					mesh->impostorTarget.GetTexture(1),
					mesh->impostorTarget.GetTexture(2)
				};
				device->BindResources(PS, res, TEXSLOT_ONDEMAND0, (easyTextureBind ? 1 : ARRAYSIZE(res)), threadID);

				if (!impostorGraphicsStateComplete)
				{
					device->BindGraphicsPSO(impostorRequest, threadID);
					device->BindConstantBuffer(PS, Material::constantBuffer_Impostor, CB_GETBINDSLOT(MaterialCB), threadID);
					SetAlphaRef(0.75f, threadID);
					impostorGraphicsStateComplete = true;
				}

				device->DrawInstanced(6 * 6, k, 0, 0, threadID); // 6 * 6: see Mesh::CreateImpostorVB function

			}
		}


		PRIMITIVETOPOLOGY prevTOPOLOGY = TRIANGLELIST;

		// Render meshes:
		for (CulledCollection::const_iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) 
		{
			Mesh* mesh = iter->first;
			if (!mesh->renderable || !(mesh->GetRenderTypes() & renderTypeFlags))
			{
				continue;
			}

			const CulledObjectList& visibleInstances = iter->second;

			const float tessF = mesh->getTessellationFactor();
			const bool tessellatorRequested = tessF > 0 && tessellation;

			if (tessellatorRequested)
			{
				TessellationCB tessCB;
				tessCB.tessellationFactors = XMFLOAT4(tessF, tessF, tessF, tessF);
				device->UpdateBuffer(constantBuffers[CBTYPE_TESSELLATION], &tessCB, threadID);
				device->BindConstantBuffer(HS, constantBuffers[CBTYPE_TESSELLATION], CBSLOT_RENDERER_TESSELLATION, threadID);
			}

			bool forceAlphaTestForDithering = false;

			UINT instancesOffset;
			size_t alloc_size = visibleInstances.size();
			alloc_size *= advancedVBRequest ? sizeof(InstBuf) : sizeof(Instance);
			void* instances = device->AllocateFromRingBuffer(dynamicVertexBufferPool, alloc_size, instancesOffset, threadID);

			int k = 0;
			for (const Object* instance : visibleInstances) 
			{
				if (occlusionCulling && instance->IsOccluded())
					continue;

				if (all_layers || (layerMask & instance->GetLayerMask()))
				{
					float dither = instance->transparency;
					if (impostorRequest != nullptr)
					{
						// fade out to impostor...
						const float impostorThreshold = instance->bounds.getRadius();
						float dist = wiMath::Distance(eye, instance->bounds.getCenter());
						if (mesh->hasImpostor())
							dither = wiMath::SmoothStep(dither, 1.0f, wiMath::Clamp((dist - impostorThreshold - mesh->impostorDistance) / impostorThreshold, 0, 1));
					}
					if (dither > 1.0f - FLT_EPSILON)
						continue;

					forceAlphaTestForDithering = forceAlphaTestForDithering || (dither > 0);

					if (mesh->softBody)
						tempMat = __identityMat;
					else
						tempMat = instance->world;

					if (advancedVBRequest || tessellatorRequested)
					{
						((volatile InstBuf*)instances)[k].instance.Create(tempMat, dither, instance->color);

						if (mesh->softBody)
							tempMat = __identityMat;
						else
							tempMat = instance->worldPrev;
						((volatile InstBuf*)instances)[k].instancePrev.Create(tempMat);
					}
					else
					{
						((volatile Instance*)instances)[k].Create(tempMat, dither, instance->color);
					}

					++k;
				}
			}

			device->InvalidateBufferAccess(dynamicVertexBufferPool, threadID);

			if (k < 1)
				continue;

			device->BindIndexBuffer(mesh->indexBuffer, mesh->GetIndexFormat(), 0, threadID);

			enum class BOUNDVERTEXBUFFERTYPE
			{
				NOTHING,
				POSITION,
				POSITION_TEXCOORD,
				EVERYTHING,
			};
			BOUNDVERTEXBUFFERTYPE boundVBType_Prev = BOUNDVERTEXBUFFERTYPE::NOTHING;

			for (MeshSubset& subset : mesh->subsets)
			{
				if (subset.subsetIndices.empty() || subset.material->isSky)
				{
					continue;
				}
				Material* material = subset.material;

				GraphicsPSO* pso = material->customShader == nullptr ? GetObjectPSO(shaderType, mesh->doubleSided, tessellatorRequested, material, forceAlphaTestForDithering) : material->customShader->passes[shaderType].pso;
				if (pso == nullptr)
				{
					continue;
				}

				bool subsetRenderable = false;

				if (renderTypeFlags & RENDERTYPE_OPAQUE)
				{
					subsetRenderable = subsetRenderable || (!material->IsTransparent() && !material->IsWater());
				}
				if (renderTypeFlags & RENDERTYPE_TRANSPARENT)
				{
					subsetRenderable = subsetRenderable || material->IsTransparent();
				}
				if (renderTypeFlags & RENDERTYPE_WATER)
				{
					subsetRenderable = subsetRenderable || material->IsWater();
				}
				if (shaderType == SHADERTYPE_SHADOW || shaderType == SHADERTYPE_SHADOWCUBE)
				{
					subsetRenderable = subsetRenderable && material->IsCastingShadow();
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
					if ((shaderType == SHADERTYPE_DEPTHONLY || shaderType == SHADERTYPE_SHADOW || shaderType == SHADERTYPE_SHADOWCUBE) && !material->IsAlphaTestEnabled() && !forceAlphaTestForDithering)
					{
						if (shaderType == SHADERTYPE_SHADOW && material->IsTransparent())
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

				if (material->IsWater())
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
						GPUBuffer* vbs[] = {
							mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_POS != nullptr ? mesh->streamoutBuffer_POS : mesh->vertexBuffer_POS),
							dynamicVertexBufferPool
						};
						UINT strides[] = {
							sizeof(Mesh::Vertex_POS),
							sizeof(Instance)
						};
						UINT offsets[] = {
							mesh->hasDynamicVB() ? mesh->bufferOffset_POS : 0,
							instancesOffset
						};
						device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
					}
					break;
					case BOUNDVERTEXBUFFERTYPE::POSITION_TEXCOORD:
					{
						GPUBuffer* vbs[] = {
							mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_POS != nullptr ? mesh->streamoutBuffer_POS : mesh->vertexBuffer_POS),
							mesh->vertexBuffer_TEX,
							dynamicVertexBufferPool
						};
						UINT strides[] = {
							sizeof(Mesh::Vertex_POS),
							sizeof(Mesh::Vertex_TEX),
							sizeof(Instance)
						};
						UINT offsets[] = {
							mesh->hasDynamicVB() ? mesh->bufferOffset_POS : 0,
							0,
							instancesOffset
						};
						device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
					}
					break;
					case BOUNDVERTEXBUFFERTYPE::EVERYTHING:
					{
						GPUBuffer* vbs[] = {
							mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_POS != nullptr ? mesh->streamoutBuffer_POS : mesh->vertexBuffer_POS),
							mesh->vertexBuffer_TEX,
							mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_PRE != nullptr ? mesh->streamoutBuffer_PRE : mesh->vertexBuffer_POS),
							dynamicVertexBufferPool
						};
						UINT strides[] = {
							sizeof(Mesh::Vertex_POS),
							sizeof(Mesh::Vertex_TEX),
							sizeof(Mesh::Vertex_POS),
							sizeof(InstBuf)
						};
						UINT offsets[] = {
							mesh->hasDynamicVB() ? mesh->bufferOffset_POS : 0,
							0,
							mesh->hasDynamicVB() ? mesh->bufferOffset_PRE : 0,
							instancesOffset
						};
						device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
					}
					break;
					default:
						assert(0);
						break;
					}
				}
				boundVBType_Prev = boundVBType;

				device->BindConstantBuffer(PS, &material->constantBuffer, CB_GETBINDSLOT(MaterialCB), threadID);

				device->BindStencilRef(material->GetStencilRef(), threadID);

				device->BindGraphicsPSO(pso, threadID);

				GPUResource* res[] = {
					material->GetBaseColorMap(),
					material->GetNormalMap(),
					material->GetSurfaceMap(),
					material->GetDisplacementMap(),
				};
				device->BindResources(PS, res, TEXSLOT_ONDEMAND0, (easyTextureBind ? 2 : ARRAYSIZE(res)), threadID);

				SetAlphaRef(material->alphaRef, threadID);

				device->DrawIndexedInstanced((int)subset.subsetIndices.size(), k, subset.indexBufferOffset, 0, 0, threadID);
			}

		}

		ResetAlphaRef(threadID);

		device->EventEnd(threadID);
	}
}

void wiRenderer::DrawWorld(Camera* camera, bool tessellation, GRAPHICSTHREAD threadID, SHADERTYPE shaderType, bool grass, bool occlusionCulling, uint32_t layerMask)
{

	const FrameCulling& culling = frameCullings[camera];
	const CulledCollection& culledRenderer = culling.culledRenderer_opaque;

	GetDevice()->EventBegin("DrawWorld", threadID);

	if (shaderType == SHADERTYPE_TILEDFORWARD)
	{
		GetDevice()->BindResource(PS, resourceBuffers[RBTYPE_ENTITYINDEXLIST_OPAQUE], SBSLOT_ENTITYINDEXLIST, threadID);
	}

	if (grass)
	{
		if (GetAlphaCompositionEnabled())
		{
			// cut off most transparent areas
			SetAlphaRef(0.25f, threadID);
		}
		for (wiHairParticle* hair : culling.culledHairParticleSystems)
		{
			hair->Draw(camera, shaderType, false, threadID);
		}
	}

	if (!culledRenderer.empty() || (grass && culling.culledHairParticleSystems.empty()))
	{
		RenderMeshes(camera->translation, culledRenderer, shaderType, RENDERTYPE_OPAQUE, threadID, tessellation, GetOcclusionCullingEnabled() && occlusionCulling, layerMask);
	}

	GetDevice()->EventEnd(threadID);

}

void wiRenderer::DrawWorldTransparent(Camera* camera, SHADERTYPE shaderType, GRAPHICSTHREAD threadID, bool grass, bool occlusionCulling, uint32_t layerMask)
{

	const FrameCulling& culling = frameCullings[camera];
	const CulledCollection& culledRenderer = culling.culledRenderer_transparent;

	GetDevice()->EventBegin("DrawWorldTransparent", threadID);

	if (shaderType == SHADERTYPE_TILEDFORWARD)
	{
		GetDevice()->BindResource(PS, resourceBuffers[RBTYPE_ENTITYINDEXLIST_TRANSPARENT], SBSLOT_ENTITYINDEXLIST, threadID);
	}

	if (ocean != nullptr)
	{
		ocean->Render(camera, renderTime, threadID);
	}

	if (grass && GetAlphaCompositionEnabled())
	{
		// transparent passes can only render hair when alpha composition is enabled
		for (wiHairParticle* hair : culling.culledHairParticleSystems)
		{
			hair->Draw(camera, shaderType, true, threadID);
		}
	}

	if (!culledRenderer.empty())
	{
		RenderMeshes(camera->translation, culledRenderer, shaderType, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, threadID, false, GetOcclusionCullingEnabled() && occlusionCulling, layerMask);
	}

	GetDevice()->EventEnd(threadID);
}


void wiRenderer::DrawSky(GRAPHICSTHREAD threadID)
{
	GetDevice()->EventBegin("DrawSky", threadID);
	
	if (enviroMap != nullptr)
	{
		GetDevice()->BindGraphicsPSO(PSO_sky[SKYRENDERING_STATIC], threadID);
		GetDevice()->BindResource(PS, enviroMap, TEXSLOT_ONDEMAND0, threadID);
	}
	else
	{
		GetDevice()->BindGraphicsPSO(PSO_sky[SKYRENDERING_DYNAMIC], threadID);
		if (GetScene().worldInfo.cloudiness > 0)
		{
			GetDevice()->BindResource(PS, textures[TEXTYPE_2D_CLOUDS], TEXSLOT_ONDEMAND0, threadID);
		}
		else
		{
			GetDevice()->BindResource(PS, wiTextureHelper::getInstance()->getBlack(), TEXSLOT_ONDEMAND0, threadID);
		}
	}

	GetDevice()->Draw(240, 0, threadID);

	GetDevice()->EventEnd(threadID);
}
void wiRenderer::DrawSun(GRAPHICSTHREAD threadID)
{
	GetDevice()->EventBegin("DrawSun", threadID);

	GetDevice()->BindGraphicsPSO(PSO_sky[SKYRENDERING_SUN], threadID);

	if (enviroMap != nullptr)
	{
		GetDevice()->BindResource(PS, wiTextureHelper::getInstance()->getBlack(), TEXSLOT_ONDEMAND0, threadID);
	}
	else
	{
		GetDevice()->BindResource(PS, textures[TEXTYPE_2D_CLOUDS], TEXSLOT_ONDEMAND0, threadID);
	}

	GetDevice()->Draw(240, 0, threadID);

	GetDevice()->EventEnd(threadID);
}

void wiRenderer::DrawDecals(Camera* camera, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = GetDevice();

	bool boundCB = false;
	for (Model* model : GetScene().models)
	{
		if (model->decals.empty())
			continue;

		device->EventBegin("Decals", threadID);

		if (!boundCB)
		{
			boundCB = true;
			device->BindConstantBuffer(PS, constantBuffers[CBTYPE_DECAL], CB_GETBINDSLOT(DecalCB),threadID);
		}

		device->BindStencilRef(STENCILREF_DEFAULT, threadID);

		device->BindGraphicsPSO(PSO_decal, threadID);

		for (Decal* decal : model->decals) 
		{

			if ((decal->texture || decal->normal) && camera->frustum.CheckBox(decal->bounds)) {

				device->BindResource(PS, decal->texture, TEXSLOT_ONDEMAND0, threadID);
				device->BindResource(PS, decal->normal, TEXSLOT_ONDEMAND1, threadID);

				XMMATRIX decalWorld = XMLoadFloat4x4(&decal->world);

				MiscCB dcbvs;
				dcbvs.mTransform =XMMatrixTranspose(decalWorld*camera->GetViewProjection());
				device->UpdateBuffer(constantBuffers[CBTYPE_MISC], &dcbvs, threadID);

				DecalCB dcbps;
				dcbps.mDecalVP = XMMatrixTranspose(XMMatrixInverse(nullptr, decalWorld));
				dcbps.hasTexNor = 0;
				if (decal->texture != nullptr)
					dcbps.hasTexNor |= 0x0000001;
				if (decal->normal != nullptr)
					dcbps.hasTexNor |= 0x0000010;
				XMStoreFloat3(&dcbps.eye, camera->GetEye());
				dcbps.opacity = decal->GetOpacity();
				dcbps.front = decal->front;
				device->UpdateBuffer(constantBuffers[CBTYPE_DECAL], &dcbps, threadID);

				device->Draw(14, 0, threadID);

			}

		}

		device->EventEnd(threadID);
	}
}

void wiRenderer::RefreshEnvProbes(GRAPHICSTHREAD threadID)
{
	GetDevice()->EventBegin("EnvironmentProbe Refresh", threadID);

	static const UINT envmapRes = 128;
	static const UINT envmapCount = 16;
	static const UINT envmapMIPs = 8;
	static const FORMAT envmapFormat = FORMAT_R16G16B16A16_FLOAT;

	if (textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY] == nullptr)
	{
		TextureDesc desc;
		desc.ArraySize = envmapCount * 6;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
		desc.CPUAccessFlags = 0;
		desc.Format = envmapFormat;
		desc.Height = envmapRes;
		desc.Width = envmapRes;
		desc.MipLevels = envmapMIPs;
		desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE | RESOURCE_MISC_GENERATE_MIPS;
		desc.Usage = USAGE_DEFAULT;

		textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY] = new Texture2D;
		textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]->RequestIndependentRenderTargetArraySlices(true);
		textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]->RequestIndependentShaderResourceArraySlices(true);
		HRESULT hr = GetDevice()->CreateTexture2D(&desc, nullptr, (Texture2D**)&textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY]);
		assert(SUCCEEDED(hr));
	}

	static Texture2D* envrenderingDepthBuffer = nullptr;
	if (envrenderingDepthBuffer == nullptr)
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

		HRESULT hr = GetDevice()->CreateTexture2D(&desc, nullptr, &envrenderingDepthBuffer);
		assert(SUCCEEDED(hr));
	}

	ViewPort VP;
	VP.Height = envmapRes;
	VP.Width = envmapRes;
	VP.TopLeftX = 0;
	VP.TopLeftY = 0;
	VP.MinDepth = 0.0f;
	VP.MaxDepth = 1.0f;
	GetDevice()->BindViewports(1, &VP, threadID);

	const float zNearP = getCamera()->zNearP;
	const float zFarP = getCamera()->zFarP;


	// reconstruct envmap array status:
	bool envmapTaken[envmapCount] = {};
	for (Model* model : GetScene().models)
	{
		for (EnvironmentProbe* probe : model->environmentProbes)
		{
			if (probe->textureIndex >= 0)
			{
				envmapTaken[probe->textureIndex] = true;
			}
		}
	}

	for (Model* model : GetScene().models)
	{
		for (EnvironmentProbe* probe : model->environmentProbes)
		{
			if (probe->textureIndex < 0)
			{
				// need to take a free envmap texture slot:
				bool found = false;
				for (int i = 0; i < ARRAYSIZE(envmapTaken); ++i)
				{
					if (envmapTaken[i] == false)
					{
						envmapTaken[i] = true;
						probe->textureIndex = i;
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

			if (probe->isUpToDate)
			{
				continue;
			}
			if (!probe->realTime)
			{
				probe->isUpToDate = true;
			}

			GetDevice()->BindRenderTargets(1, (Texture2D**)&textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], envrenderingDepthBuffer, threadID, probe->textureIndex);
			const float clearColor[4] = { 0,0,0,1 };
			GetDevice()->ClearRenderTarget(textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], clearColor, threadID, probe->textureIndex);
			GetDevice()->ClearDepthStencil(envrenderingDepthBuffer, CLEAR_DEPTH, 0.0f, 0, threadID);


			std::vector<SHCAM> cameras;
			{
				cameras.clear();

				cameras.push_back(SHCAM(XMFLOAT4(0.5f, -0.5f, -0.5f, -0.5f), zNearP, zFarP, XM_PI / 2.0f)); //+x
				cameras.push_back(SHCAM(XMFLOAT4(0.5f, 0.5f, 0.5f, -0.5f), zNearP, zFarP, XM_PI / 2.0f)); //-x

				cameras.push_back(SHCAM(XMFLOAT4(1, 0, 0, -0), zNearP, zFarP, XM_PI / 2.0f)); //+y
				cameras.push_back(SHCAM(XMFLOAT4(0, 0, 0, -1), zNearP, zFarP, XM_PI / 2.0f)); //-y

				cameras.push_back(SHCAM(XMFLOAT4(0.707f, 0, 0, -0.707f), zNearP, zFarP, XM_PI / 2.0f)); //+z
				cameras.push_back(SHCAM(XMFLOAT4(0, 0.707f, 0.707f, 0), zNearP, zFarP, XM_PI / 2.0f)); //-z
			}

			XMFLOAT3 center = probe->translation;
			XMVECTOR vCenter = XMLoadFloat3(&center);

			CubeMapRenderCB cb;
			for (unsigned int i = 0; i < cameras.size(); ++i)
			{
				cameras[i].Update(vCenter);
				cb.mViewProjection[i] = cameras[i].getVP();
			}

			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_CUBEMAPRENDER], &cb, threadID);
			GetDevice()->BindConstantBuffer(GS, constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubeMapRenderCB), threadID);


			CameraCB camcb;
			camcb.mCamPos = center; // only this will be used by envprobe rendering shaders the rest is read from cubemaprenderCB
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_CAMERA], &camcb, threadID);


			CulledList culledObjects;
			CulledCollection culledRenderer;

			SPHERE culler = SPHERE(center, zFarP);
			if (spTree != nullptr)
			{
				spTree->getVisible(culler, culledObjects);

				for (Cullable* object : culledObjects)
				{
					culledRenderer[((Object*)object)->mesh].push_front((Object*)object);
				}

				RenderMeshes(center, culledRenderer, SHADERTYPE_ENVMAPCAPTURE, RENDERTYPE_OPAQUE, threadID);
			}

			// sky
			{

				if (enviroMap != nullptr)
				{
					GetDevice()->BindGraphicsPSO(PSO_sky[SKYRENDERING_ENVMAPCAPTURE_STATIC], threadID);
					GetDevice()->BindResource(PS, enviroMap, TEXSLOT_ONDEMAND0, threadID);
				}
				else
				{
					GetDevice()->BindGraphicsPSO(PSO_sky[SKYRENDERING_ENVMAPCAPTURE_DYNAMIC], threadID);
					GetDevice()->BindResource(PS, textures[TEXTYPE_2D_CLOUDS], TEXSLOT_ONDEMAND0, threadID);
				}

				GetDevice()->Draw(240, 0, threadID);
			}

			GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);
			GetDevice()->GenerateMips(textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], threadID, probe->textureIndex);
		}
	}

	GetDevice()->BindResource(PS, textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], TEXSLOT_ENVMAPARRAY, threadID);
	GetDevice()->BindResource(CS, textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY], TEXSLOT_ENVMAPARRAY, threadID);

	GetDevice()->EventEnd(threadID); // EnvironmentProbe Refresh
}

void wiRenderer::VoxelRadiance(GRAPHICSTHREAD threadID)
{
	if (!GetVoxelRadianceEnabled())
	{
		return;
	}

	GetDevice()->EventBegin("Voxel Radiance", threadID);
	wiProfiler::GetInstance().BeginRange("Voxel Radiance", wiProfiler::DOMAIN_GPU, threadID);


	if (textures[TEXTYPE_3D_VOXELRADIANCE] == nullptr)
	{
		TextureDesc desc;
		ZeroMemory(&desc, sizeof(desc));
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
		HRESULT hr = GetDevice()->CreateTexture3D(&desc, nullptr, (Texture3D**)&textures[TEXTYPE_3D_VOXELRADIANCE]);
		assert(SUCCEEDED(hr));
	}
	if (voxelSceneData.secondaryBounceEnabled && textures[TEXTYPE_3D_VOXELRADIANCE_HELPER] == nullptr)
	{
		TextureDesc desc = ((Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE])->GetDesc();
		textures[TEXTYPE_3D_VOXELRADIANCE_HELPER] = new Texture3D;
		textures[TEXTYPE_3D_VOXELRADIANCE_HELPER]->RequestIndependentShaderResourcesForMIPs(true);
		textures[TEXTYPE_3D_VOXELRADIANCE_HELPER]->RequestIndependentUnorderedAccessResourcesForMIPs(true);
		HRESULT hr = GetDevice()->CreateTexture3D(&desc, nullptr, (Texture3D**)&textures[TEXTYPE_3D_VOXELRADIANCE_HELPER]);
		assert(SUCCEEDED(hr));
	}
	if (resourceBuffers[RBTYPE_VOXELSCENE] == nullptr)
	{
		GPUBufferDesc desc;
		desc.StructureByteStride = sizeof(UINT) * 2;
		desc.ByteWidth = desc.StructureByteStride * voxelSceneData.res * voxelSceneData.res * voxelSceneData.res;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.Usage = USAGE_DEFAULT;

		resourceBuffers[RBTYPE_VOXELSCENE] = new GPUBuffer;
		HRESULT hr = GetDevice()->CreateBuffer(&desc, nullptr, resourceBuffers[RBTYPE_VOXELSCENE]);
		assert(SUCCEEDED(hr));
	}

	Texture3D* result = (Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE];

	CulledList culledObjects;
	CulledCollection culledRenderer;

	AABB bbox;
	XMFLOAT3 extents = voxelSceneData.extents;
	XMFLOAT3 center = voxelSceneData.center;
	bbox.createFromHalfWidth(center, extents);
	if (spTree != nullptr && extents.x > 0 && extents.y > 0 && extents.z > 0)
	{
		spTree->getVisible(bbox, culledObjects);

		for (Cullable* object : culledObjects)
		{
			culledRenderer[((Object*)object)->mesh].push_front((Object*)object);
		}

		ViewPort VP;
		VP.TopLeftX = 0;
		VP.TopLeftY = 0;
		VP.Width = (float)voxelSceneData.res;
		VP.Height = (float)voxelSceneData.res;
		VP.MinDepth = 0.0f;
		VP.MaxDepth = 1.0f;
		GetDevice()->BindViewports(1, &VP, threadID);

		GPUResource* UAVs[] = { resourceBuffers[RBTYPE_VOXELSCENE] };
		GetDevice()->BindRenderTargetsUAVs(0, nullptr, nullptr, UAVs, 0, 1, threadID);

		RenderMeshes(center, culledRenderer, SHADERTYPE_VOXELIZE, RENDERTYPE_OPAQUE, threadID);

		// Copy the packed voxel scene data to a 3D texture, then delete the voxel scene emission data. The cone tracing will operate on the 3D texture
		GetDevice()->EventBegin("Voxel Scene Copy - Clear", threadID);
		GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);
		GetDevice()->BindUnorderedAccessResourceCS(resourceBuffers[RBTYPE_VOXELSCENE], 0, threadID);
		GetDevice()->BindUnorderedAccessResourceCS(textures[TEXTYPE_3D_VOXELRADIANCE], 1, threadID);

		if (GetDevice()->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_UNORDEREDACCESSTEXTURE_LOAD_FORMAT_EXT))
		{
			GetDevice()->BindComputePSO(CPSO[CSTYPE_VOXELSCENECOPYCLEAR_TEMPORALSMOOTHING], threadID);
		}
		else
		{
			GetDevice()->BindComputePSO(CPSO[CSTYPE_VOXELSCENECOPYCLEAR], threadID);
		}
		GetDevice()->Dispatch((UINT)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 256), 1, 1, threadID);
		GetDevice()->EventEnd(threadID);

		if (voxelSceneData.secondaryBounceEnabled)
		{
			GetDevice()->EventBegin("Voxel Radiance Secondary Bounce", threadID);
			GetDevice()->UnBindUnorderedAccessResources(1, 1, threadID);
			// Pre-integrate the voxel texture by creating blurred mip levels:
			GenerateMipChain((Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE], MIPGENFILTER_LINEAR, threadID);
			GetDevice()->BindUnorderedAccessResourceCS(textures[TEXTYPE_3D_VOXELRADIANCE_HELPER], 0, threadID);
			GetDevice()->BindResource(CS, textures[TEXTYPE_3D_VOXELRADIANCE], 0, threadID);
			GetDevice()->BindResource(CS, resourceBuffers[RBTYPE_VOXELSCENE], 1, threadID);
			GetDevice()->BindComputePSO(CPSO[CSTYPE_VOXELRADIANCESECONDARYBOUNCE], threadID);
			GetDevice()->Dispatch((UINT)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 64), 1, 1, threadID);
			GetDevice()->EventEnd(threadID);

			GetDevice()->EventBegin("Voxel Scene Clear Normals", threadID);
			GetDevice()->UnBindResources(1, 1, threadID);
			GetDevice()->BindUnorderedAccessResourceCS(resourceBuffers[RBTYPE_VOXELSCENE], 0, threadID);
			GetDevice()->BindComputePSO(CPSO[CSTYPE_VOXELCLEARONLYNORMAL], threadID);
			GetDevice()->Dispatch((UINT)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 256), 1, 1, threadID);
			GetDevice()->EventEnd(threadID);

			result = (Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE_HELPER];
		}

		GetDevice()->UnBindUnorderedAccessResources(0, 2, threadID);


		// Pre-integrate the voxel texture by creating blurred mip levels:
		//if (GetDevice()->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_UNORDEREDACCESSTEXTURE_LOAD_FORMAT_EXT))
		//{
		//	GenerateMipChain(result, MIPGENFILTER_GAUSSIAN, threadID);
		//}
		//else
		{
			GenerateMipChain(result, MIPGENFILTER_LINEAR, threadID);
		}
	}

	if (voxelHelper)
	{
		GetDevice()->BindResource(VS, result, TEXSLOT_VOXELRADIANCE, threadID);
	}
	GetDevice()->BindResource(PS, result, TEXSLOT_VOXELRADIANCE, threadID);
	GetDevice()->BindResource(CS, result, TEXSLOT_VOXELRADIANCE, threadID);

	wiProfiler::GetInstance().EndRange(threadID);
	GetDevice()->EventEnd(threadID);
}


void wiRenderer::ComputeTiledLightCulling(bool deferred, GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Tiled Entity Processing", wiProfiler::DOMAIN_GPU, threadID);
	GraphicsDevice* device = wiRenderer::GetDevice();

	int _width = GetInternalResolution().x;
	int _height = GetInternalResolution().y;

	const XMUINT3 tileCount = GetEntityCullingTileCount();

	static int _savedWidth = 0;
	static int _savedHeight = 0;
	bool _resolutionChanged = GetDevice()->ResolutionChanged();
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

		UINT _stride = sizeof(XMFLOAT4) * 4;

		GPUBufferDesc bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.ByteWidth = _stride * tileCount.x * tileCount.y * tileCount.z; // storing 4 planes for every tile
		bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.Usage = USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
		bd.StructureByteStride = _stride;
		device->CreateBuffer(&bd, nullptr, frustumBuffer);
	}
	if (_resolutionChanged)
	{
		SAFE_DELETE(resourceBuffers[RBTYPE_ENTITYINDEXLIST_OPAQUE]);
		SAFE_DELETE(resourceBuffers[RBTYPE_ENTITYINDEXLIST_TRANSPARENT]);
		resourceBuffers[RBTYPE_ENTITYINDEXLIST_OPAQUE] = new GPUBuffer;
		resourceBuffers[RBTYPE_ENTITYINDEXLIST_TRANSPARENT] = new GPUBuffer;

		GPUBufferDesc bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.ByteWidth = sizeof(UINT) * tileCount.x * tileCount.y * tileCount.z * MAX_SHADER_ENTITY_COUNT_PER_TILE;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		bd.CPUAccessFlags = 0;
		bd.StructureByteStride = sizeof(UINT);
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		device->CreateBuffer(&bd, nullptr, resourceBuffers[RBTYPE_ENTITYINDEXLIST_OPAQUE]);
		device->CreateBuffer(&bd, nullptr, resourceBuffers[RBTYPE_ENTITYINDEXLIST_TRANSPARENT]);
	}
	if (deferred && (textures[TEXTYPE_2D_TILEDDEFERRED_DIFFUSEUAV] == nullptr || textures[TEXTYPE_2D_TILEDDEFERRED_SPECULARUAV] == nullptr))
	{
		TextureDesc desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.ArraySize = 1;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.Format = RTFormat_deferred_lightbuffer;
		desc.Width = (UINT)_width;
		desc.Height = (UINT)_height;
		desc.MipLevels = 1;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = USAGE_DEFAULT;
		device->CreateTexture2D(&desc, nullptr, (Texture2D**)&textures[TEXTYPE_2D_TILEDDEFERRED_DIFFUSEUAV]);
		device->CreateTexture2D(&desc, nullptr, (Texture2D**)&textures[TEXTYPE_2D_TILEDDEFERRED_SPECULARUAV]);
	}

	// calculate the per-tile frustums once:
	static bool frustumsComplete = false;
	static XMFLOAT4X4 _savedProjection;
	if (memcmp(&_savedProjection, &getCamera()->Projection, sizeof(XMFLOAT4X4)) != 0)
	{
		_savedProjection = getCamera()->Projection;
		frustumsComplete = false;
	}
	if(!frustumsComplete || _resolutionChanged)
	{
		frustumsComplete = true;

		GPUResource* uavs[] = { frustumBuffer };

		device->BindUnorderedAccessResourcesCS(uavs, UAVSLOT_TILEFRUSTUMS, ARRAYSIZE(uavs), threadID);
		device->BindComputePSO(CPSO[CSTYPE_TILEFRUSTUMS], threadID);

		DispatchParamsCB dispatchParams;
		dispatchParams.numThreads[0] = tileCount.x;
		dispatchParams.numThreads[1] = tileCount.y;
		dispatchParams.numThreads[2] = 1;
		dispatchParams.numThreadGroups[0] = (UINT)ceilf(dispatchParams.numThreads[0] / (float)TILED_CULLING_BLOCKSIZE);
		dispatchParams.numThreadGroups[1] = (UINT)ceilf(dispatchParams.numThreads[1] / (float)TILED_CULLING_BLOCKSIZE);
		dispatchParams.numThreadGroups[2] = 1;
		device->UpdateBuffer(constantBuffers[CBTYPE_DISPATCHPARAMS], &dispatchParams, threadID);
		device->BindConstantBuffer(CS, constantBuffers[CBTYPE_DISPATCHPARAMS], CB_GETBINDSLOT(DispatchParamsCB), threadID);

		device->Dispatch(dispatchParams.numThreadGroups[0], dispatchParams.numThreadGroups[1], dispatchParams.numThreadGroups[2], threadID);
		device->UnBindUnorderedAccessResources(UAVSLOT_TILEFRUSTUMS, 1, threadID);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
	}

	if (textures[TEXTYPE_2D_DEBUGUAV] == nullptr || _resolutionChanged)
	{
		SAFE_DELETE(textures[TEXTYPE_2D_DEBUGUAV]);

		TextureDesc desc;
		ZeroMemory(&desc, sizeof(desc));
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

		device->CreateTexture2D(&desc, nullptr, (Texture2D**)&textures[TEXTYPE_2D_DEBUGUAV]);
	}

	// Perform the culling
	{
		device->EventBegin("Entity Culling", threadID);

		device->UnBindResources(SBSLOT_ENTITYINDEXLIST, 1, threadID);

		device->BindResource(CS, frustumBuffer, SBSLOT_TILEFRUSTUMS, threadID);

		device->BindComputePSO(CPSO_tiledlighting[deferred][GetAdvancedLightCulling()][GetDebugLightCulling()], threadID);

		if (GetDebugLightCulling())
		{
			device->BindUnorderedAccessResourceCS(textures[TEXTYPE_2D_DEBUGUAV], UAVSLOT_DEBUGTEXTURE, threadID);
		}


		const FrameCulling& frameCulling = frameCullings[getCamera()];


		DispatchParamsCB dispatchParams;
		dispatchParams.numThreadGroups[0] = tileCount.x;
		dispatchParams.numThreadGroups[1] = tileCount.y;
		dispatchParams.numThreadGroups[2] = 1;
		dispatchParams.numThreads[0] = dispatchParams.numThreadGroups[0] * TILED_CULLING_BLOCKSIZE;
		dispatchParams.numThreads[1] = dispatchParams.numThreadGroups[1] * TILED_CULLING_BLOCKSIZE;
		dispatchParams.numThreads[2] = 1;
		dispatchParams.value0 = (UINT)(frameCulling.culledLights.size() + frameCulling.culledEnvProbes.size() + frameCulling.culledDecals.size());
		device->UpdateBuffer(constantBuffers[CBTYPE_DISPATCHPARAMS], &dispatchParams, threadID);
		device->BindConstantBuffer(CS, constantBuffers[CBTYPE_DISPATCHPARAMS], CB_GETBINDSLOT(DispatchParamsCB), threadID);

		if (deferred)
		{
			GPUResource* uavs[] = {
				textures[TEXTYPE_2D_TILEDDEFERRED_DIFFUSEUAV],
				resourceBuffers[RBTYPE_ENTITYINDEXLIST_TRANSPARENT],
				textures[TEXTYPE_2D_TILEDDEFERRED_SPECULARUAV],
			};
			device->BindUnorderedAccessResourcesCS(uavs, UAVSLOT_TILEDDEFERRED_DIFFUSE, ARRAYSIZE(uavs), threadID);

			GetDevice()->BindResource(CS, Light::shadowMapArray_2D, TEXSLOT_SHADOWARRAY_2D, threadID);
			GetDevice()->BindResource(CS, Light::shadowMapArray_Cube, TEXSLOT_SHADOWARRAY_CUBE, threadID);
			GetDevice()->BindResource(CS, Light::shadowMapArray_Transparent, TEXSLOT_SHADOWARRAY_TRANSPARENT, threadID);

			device->Dispatch(dispatchParams.numThreadGroups[0], dispatchParams.numThreadGroups[1], dispatchParams.numThreadGroups[2], threadID);
			device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		}
		else
		{
			GPUResource* uavs[] = {
				resourceBuffers[RBTYPE_ENTITYINDEXLIST_OPAQUE],
				resourceBuffers[RBTYPE_ENTITYINDEXLIST_TRANSPARENT],
			};
			device->BindUnorderedAccessResourcesCS(uavs, UAVSLOT_ENTITYINDEXLIST_OPAQUE, ARRAYSIZE(uavs), threadID);

			device->Dispatch(dispatchParams.numThreadGroups[0], dispatchParams.numThreadGroups[1], dispatchParams.numThreadGroups[2], threadID);
			device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		}

		device->UnBindUnorderedAccessResources(0, 8, threadID); // this unbinds pretty much every uav

		device->EventEnd(threadID);
	}

	wiProfiler::GetInstance().EndRange(threadID);
}
void wiRenderer::ResolveMSAADepthBuffer(Texture2D* dst, Texture2D* src, GRAPHICSTHREAD threadID)
{
	GetDevice()->EventBegin("Resolve MSAA DepthBuffer", threadID);

	GetDevice()->BindResource(CS, src, TEXSLOT_ONDEMAND0, threadID);
	GetDevice()->BindUnorderedAccessResourceCS(dst, 0, threadID);

	TextureDesc desc = src->GetDesc();

	GetDevice()->BindComputePSO(CPSO[CSTYPE_RESOLVEMSAADEPTHSTENCIL], threadID);
	GetDevice()->Dispatch((UINT)ceilf(desc.Width / 16.f), (UINT)ceilf(desc.Height / 16.f), 1, threadID);


	GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
	GetDevice()->UnBindUnorderedAccessResources(0, 1, threadID);

	GetDevice()->EventEnd(threadID);
}
void wiRenderer::GenerateMipChain(Texture1D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID)
{
	assert(0 && "Not implemented!");
}
void wiRenderer::GenerateMipChain(Texture2D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID)
{
	TextureDesc desc = texture->GetDesc();

	if (desc.MipLevels < 2)
	{
		assert(0);
		return;
	}

	GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);

	switch (filter)
	{
	case wiRenderer::MIPGENFILTER_POINT:
		GetDevice()->EventBegin("GenerateMipChain 2D - PointFilter", threadID);
		GetDevice()->BindComputePSO(CPSO[CSTYPE_GENERATEMIPCHAIN2D_SIMPLEFILTER], threadID);
		GetDevice()->BindSampler(CS, samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_LINEAR:
		GetDevice()->EventBegin("GenerateMipChain 2D - LinearFilter", threadID);
		GetDevice()->BindComputePSO(CPSO[CSTYPE_GENERATEMIPCHAIN2D_SIMPLEFILTER], threadID);
		GetDevice()->BindSampler(CS, samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_LINEAR_MAXIMUM:
		GetDevice()->EventBegin("GenerateMipChain 2D - LinearMaxFilter", threadID);
		GetDevice()->BindComputePSO(CPSO[CSTYPE_GENERATEMIPCHAIN2D_SIMPLEFILTER], threadID);
		GetDevice()->BindSampler(CS, customsamplers[SSTYPE_MAXIMUM_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_GAUSSIAN:
		GetDevice()->EventBegin("GenerateMipChain 2D - GaussianFilter", threadID);
		GetDevice()->BindComputePSO(CPSO[CSTYPE_GENERATEMIPCHAIN2D_GAUSSIAN], threadID);
		break;
	}

	for (UINT i = 0; i < desc.MipLevels - 1; ++i)
	{
		GetDevice()->BindUnorderedAccessResourceCS(texture, 0, threadID, i + 1);
		GetDevice()->BindResource(CS, texture, TEXSLOT_UNIQUE0, threadID, i);
		desc.Width = max(1, (UINT)ceilf(desc.Width * 0.5f));
		desc.Height = max(1, (UINT)ceilf(desc.Height * 0.5f));
		GetDevice()->Dispatch(
			max(1, (UINT)ceilf((float)desc.Width / GENERATEMIPCHAIN_2D_BLOCK_SIZE)), 
			max(1, (UINT)ceilf((float)desc.Height / GENERATEMIPCHAIN_2D_BLOCK_SIZE)), 
			1, 
			threadID);
	}

	GetDevice()->UnBindResources(TEXSLOT_UNIQUE0, 1, threadID);
	GetDevice()->UnBindUnorderedAccessResources(0, 1, threadID);

	GetDevice()->EventEnd(threadID);
}
void wiRenderer::GenerateMipChain(Texture3D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID)
{
	TextureDesc desc = texture->GetDesc();

	if (desc.MipLevels < 2)
	{
		assert(0);
		return;
	}

	GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);

	switch (filter)
	{
	case wiRenderer::MIPGENFILTER_POINT:
		GetDevice()->EventBegin("GenerateMipChain 3D - PointFilter", threadID);
		GetDevice()->BindComputePSO(CPSO[CSTYPE_GENERATEMIPCHAIN3D_SIMPLEFILTER], threadID);
		GetDevice()->BindSampler(CS, samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_LINEAR:
		GetDevice()->EventBegin("GenerateMipChain 3D - LinearFilter", threadID);
		GetDevice()->BindComputePSO(CPSO[CSTYPE_GENERATEMIPCHAIN3D_SIMPLEFILTER], threadID);
		GetDevice()->BindSampler(CS, samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_LINEAR_MAXIMUM:
		GetDevice()->EventBegin("GenerateMipChain 3D - LinearMaxFilter", threadID);
		GetDevice()->BindComputePSO(CPSO[CSTYPE_GENERATEMIPCHAIN3D_SIMPLEFILTER], threadID);
		GetDevice()->BindSampler(CS, customsamplers[SSTYPE_MAXIMUM_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_GAUSSIAN:
		GetDevice()->EventBegin("GenerateMipChain 3D - GaussianFilter", threadID);
		GetDevice()->BindComputePSO(CPSO[CSTYPE_GENERATEMIPCHAIN3D_GAUSSIAN], threadID);
		break;
	}

	for (UINT i = 0; i < desc.MipLevels - 1; ++i)
	{
		GetDevice()->BindUnorderedAccessResourceCS(texture, 0, threadID, i + 1);
		GetDevice()->BindResource(CS, texture, TEXSLOT_UNIQUE0, threadID, i);
		desc.Width = max(1, (UINT)ceilf(desc.Width * 0.5f));
		desc.Height = max(1, (UINT)ceilf(desc.Height * 0.5f));
		desc.Depth = max(1, (UINT)ceilf(desc.Depth * 0.5f));
		GetDevice()->Dispatch(
			max(1, (UINT)ceilf((float)desc.Width / GENERATEMIPCHAIN_3D_BLOCK_SIZE)), 
			max(1, (UINT)ceilf((float)desc.Height / GENERATEMIPCHAIN_3D_BLOCK_SIZE)), 
			max(1, (UINT)ceilf((float)desc.Depth / GENERATEMIPCHAIN_3D_BLOCK_SIZE)), 
			threadID);
	}

	GetDevice()->UnBindResources(TEXSLOT_UNIQUE0, 1, threadID);
	GetDevice()->UnBindUnorderedAccessResources(0, 1, threadID);

	GetDevice()->EventEnd(threadID);
}

void wiRenderer::GenerateClouds(Texture2D* dst, UINT refinementCount, float randomness, GRAPHICSTHREAD threadID)
{
	GetDevice()->EventBegin("Cloud Generator", threadID);

	TextureDesc src_desc = wiTextureHelper::getInstance()->getRandom64x64()->GetDesc();

	TextureDesc dst_desc = dst->GetDesc();
	assert(dst_desc.BindFlags & BIND_UNORDERED_ACCESS);

	GetDevice()->BindResource(CS, wiTextureHelper::getInstance()->getRandom64x64(), TEXSLOT_ONDEMAND0, threadID);
	GetDevice()->BindUnorderedAccessResourceCS(dst, 0, threadID);

	CloudGeneratorCB cb;
	cb.xNoiseTexDim = XMFLOAT2((float)src_desc.Width, (float)src_desc.Height);
	cb.xRandomness = randomness;
	if (refinementCount == 0)
	{
		cb.xRefinementCount = max(1, (UINT)log2(dst_desc.Width));
	}
	else
	{
		cb.xRefinementCount = refinementCount;
	}
	GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_CLOUDGENERATOR], &cb, threadID);
	GetDevice()->BindConstantBuffer(CS, constantBuffers[CBTYPE_CLOUDGENERATOR], CB_GETBINDSLOT(CloudGeneratorCB), threadID);

	GetDevice()->BindComputePSO(CPSO[CSTYPE_CLOUDGENERATOR], threadID);
	GetDevice()->Dispatch((UINT)ceilf(dst_desc.Width / (float)CLOUDGENERATOR_BLOCKSIZE), (UINT)ceilf(dst_desc.Height / (float)CLOUDGENERATOR_BLOCKSIZE), 1, threadID);

	GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
	GetDevice()->UnBindUnorderedAccessResources(0, 1, threadID);


	GetDevice()->EventEnd(threadID);
}

void wiRenderer::ManageDecalAtlas(GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = GetDevice();

	static Texture2D* atlasTexture = nullptr;

	for (Model* model : GetScene().models)
	{
		if (model->decals.empty())
			continue;

		for (Decal* decal : model->decals)
		{
			if (decal->texture == nullptr)
			{
				continue;
			}

			using namespace wiRectPacker;
			static std::map<Texture2D*, rect_xywhf> storedTextures;

			if (storedTextures.find(decal->texture) == storedTextures.end())
			{
				// we need to pack this decal texture into the atlas
				rect_xywhf newRect = rect_xywhf(0, 0, decal->texture->GetDesc().Width, decal->texture->GetDesc().Height);
				storedTextures[decal->texture] = newRect;

				rect_xywhf** out_rects = new rect_xywhf*[storedTextures.size()];
				int i = 0;
				for (auto& it : storedTextures)
				{
					out_rects[i] = &it.second;
					i++;
				}

				std::vector<bin> bins;
				if (pack(out_rects, (int)storedTextures.size(), 16384, bins))
				{
					assert(bins.size() == 1 && "Decal atlas packing into single texture failed!");

					SAFE_DELETE(atlasTexture);

					TextureDesc desc;
					ZeroMemory(&desc, sizeof(desc));
					desc.Width = (UINT)bins[0].size.w;
					desc.Height = (UINT)bins[0].size.h;
					desc.MipLevels = 6;
					desc.ArraySize = 1;
					desc.Format = FORMAT_B8G8R8A8_UNORM; // png decals are loaded into this format! todo: DXT!
					desc.SampleDesc.Count = 1;
					desc.SampleDesc.Quality = 0;
					desc.Usage = USAGE_DEFAULT;
					desc.BindFlags = BIND_SHADER_RESOURCE;
					desc.CPUAccessFlags = 0;
					desc.MiscFlags = 0;

					device->CreateTexture2D(&desc, nullptr, &atlasTexture);

					for (UINT mip = 0; mip < atlasTexture->GetDesc().MipLevels; ++mip)
					{
						for (auto& it : storedTextures)
						{
							if (mip < it.first->GetDesc().MipLevels)
							{
								device->CopyTexture2D_Region(atlasTexture, mip, it.second.x >> mip, it.second.y >> mip, it.first, mip, threadID);
							}
						}
					}
				}
				else
				{
					wiBackLog::post("Decal atlas packing failed!");
				}
			}
			
			rect_xywhf rect = storedTextures[decal->texture];
			TextureDesc desc = atlasTexture->GetDesc();
			decal->atlasMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height, (float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);
		}

	}

	if (atlasTexture != nullptr)
	{
		device->BindResource(PS, atlasTexture, TEXSLOT_DECALATLAS, threadID);
	}
}

void wiRenderer::UpdateWorldCB(GRAPHICSTHREAD threadID)
{
	static WorldCB prevcb[GRAPHICSTHREAD_COUNT];

	WorldCB value;
	ZeroMemory(&value, sizeof(value));

	value.mScreenWidthHeight = XMFLOAT2((float)GetDevice()->GetScreenWidth(), (float)GetDevice()->GetScreenHeight());
	value.mScreenWidthHeight_Inverse = XMFLOAT2(1.0f / value.mScreenWidthHeight.x, 1.0f / value.mScreenWidthHeight.y);
	value.mInternalResolution = XMFLOAT2((float)GetInternalResolution().x, (float)GetInternalResolution().y);
	value.mInternalResolution_Inverse = XMFLOAT2(1.0f / value.mInternalResolution.x, 1.0f / value.mInternalResolution.y);
	value.mGamma = GetGamma();
	auto& world = GetScene().worldInfo;
	value.mAmbient = world.ambient;
	value.mCloudiness = world.cloudiness;
	value.mCloudScale = world.cloudScale;
	value.mFog = world.fogSEH;
	value.mHorizon = world.horizon;
	value.mZenith = world.zenith;
	value.mSpecularAA = SPECULARAA;
	value.mVoxelRadianceDataSize = voxelSceneData.voxelsize;
	value.mVoxelRadianceDataSize_Inverse = 1.0f / (float)value.mVoxelRadianceDataSize;
	value.mVoxelRadianceDataRes = GetVoxelRadianceEnabled() ? (UINT)voxelSceneData.res : 0;
	value.mVoxelRadianceDataRes_Inverse = 1.0f / (float)value.mVoxelRadianceDataRes;
	value.mVoxelRadianceDataMIPs = voxelSceneData.mips;
	value.mVoxelRadianceDataNumCones = max(min(voxelSceneData.numCones, 16), 1);
	value.mVoxelRadianceDataNumCones_Inverse = 1.0f / (float)value.mVoxelRadianceDataNumCones;
	value.mVoxelRadianceDataRayStepSize = voxelSceneData.rayStepSize;
	value.mVoxelRadianceReflectionsEnabled = voxelSceneData.reflectionsEnabled;
	value.mVoxelRadianceDataCenter = voxelSceneData.center;
	value.mAdvancedRefractions = GetAdvancedRefractionsEnabled() ? 1 : 0;
	value.mEntityCullingTileCount = GetEntityCullingTileCount();
	value.mTransparentShadowsEnabled = TRANSPARENTSHADOWSENABLED;
	value.mGlobalEnvProbeIndex = -1;
	value.mEnvProbeMipCount = 0;
	value.mEnvProbeMipCount_Inverse = 1.0f;
	for (Model* model : GetScene().models)
	{
		if (!model->environmentProbes.empty())
		{
			value.mGlobalEnvProbeIndex = 0; // for now, the global envprobe will be the first probe in the array. Easy change later on if required...
			break;
		}
	}
	if (textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY] != nullptr)
	{
		value.mEnvProbeMipCount = static_cast<Texture2D*>(textures[TEXTYPE_CUBEARRAY_ENVMAPARRAY])->GetDesc().MipLevels;
		value.mEnvProbeMipCount_Inverse = 1.0f / (float)value.mEnvProbeMipCount;
	}

	if (memcmp(&prevcb[threadID], &value, sizeof(WorldCB)) != 0) // prevent overcommit
	{
		prevcb[threadID] = value;
		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_WORLD], &prevcb[threadID], threadID);
	}
}
void wiRenderer::UpdateFrameCB(GRAPHICSTHREAD threadID)
{
	FrameCB cb;

	cb.mTime = renderTime;
	cb.mTimePrev = renderTime_Prev;
	cb.mDeltaTime = deltaTime;
	cb.mLightArrayOffset = entityArrayOffset_Lights;
	cb.mLightArrayCount = entityArrayCount_Lights;
	cb.mDecalArrayOffset = entityArrayOffset_Decals;
	cb.mDecalArrayCount = entityArrayCount_Decals;
	cb.mForceFieldArrayOffset = entityArrayOffset_ForceFields;
	cb.mForceFieldArrayCount = entityArrayCount_ForceFields;
	cb.mEnvProbeArrayOffset = entityArrayOffset_EnvProbes;
	cb.mEnvProbeArrayCount = entityArrayCount_EnvProbes;
	cb.mVoxelRadianceRetargetted = voxelSceneData.centerChangedThisFrame ? 1 : 0;
	auto& wind = GetScene().wind;
	cb.mWindRandomness = wind.randomness;
	cb.mWindWaveSize = wind.waveSize;
	cb.mWindDirection = wind.direction;
	cb.mFrameCount = (UINT)GetDevice()->GetFrameCount();
	cb.mSunEntityArrayIndex = GetSunArrayIndex();
	cb.mTemporalAASampleRotation = 0;
	if (GetTemporalAAEnabled())
	{
		UINT id = cb.mFrameCount % 4;
		UINT x = 0;
		UINT y = 0;
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
		cb.mTemporalAASampleRotation = (x & 0x000000FF) | ((y & 0x000000FF) << 8);
	}
	cb.mTemporalAAJitter = temporalAAJitter;
	cb.mTemporalAAJitterPrev = temporalAAJitterPrev;

	auto camera = getCamera();
	auto prevCam = prevFrameCam;
	auto reflCam = getRefCamera();

	cb.mVP = XMMatrixTranspose(camera->GetViewProjection());
	cb.mView = XMMatrixTranspose(camera->GetView());
	cb.mProj = XMMatrixTranspose(camera->GetProjection());
	cb.mCamPos = camera->translation;
	cb.mCamDistanceFromOrigin = XMVectorGetX(XMVector3Length(XMLoadFloat3(&cb.mCamPos)));
	cb.mPrevV = XMMatrixTranspose(prevCam->GetView());
	cb.mPrevP = XMMatrixTranspose(prevCam->GetProjection());
	cb.mPrevVP = XMMatrixTranspose(prevCam->GetViewProjection());
	cb.mPrevInvVP = XMMatrixTranspose(prevCam->GetInvViewProjection());
	cb.mReflVP = XMMatrixTranspose(reflCam->GetViewProjection());
	cb.mInvV = XMMatrixTranspose(camera->GetInvView());
	cb.mInvP = XMMatrixTranspose(camera->GetInvProjection());
	cb.mInvVP = XMMatrixTranspose(camera->GetInvViewProjection());
	cb.mAt = camera->At;
	cb.mUp = camera->Up;
	cb.mZNearP = camera->zNearP;
	cb.mZFarP = camera->zFarP;
	cb.mZNearP_Recip = 1.0f / max(0.0001f, cb.mZNearP);
	cb.mZFarP_Recip = 1.0f / max(0.0001f, cb.mZFarP);
	cb.mZRange = abs(cb.mZFarP - cb.mZNearP);
	cb.mZRange_Recip = 1.0f / max(0.0001f, cb.mZRange);
	cb.mFrustumPlanesWS[0] = camera->frustum.getLeftPlane();
	cb.mFrustumPlanesWS[1] = camera->frustum.getRightPlane();
	cb.mFrustumPlanesWS[2] = camera->frustum.getTopPlane();
	cb.mFrustumPlanesWS[3] = camera->frustum.getBottomPlane();
	cb.mFrustumPlanesWS[4] = camera->frustum.getNearPlane();
	cb.mFrustumPlanesWS[5] = camera->frustum.getFarPlane();

	GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_FRAME], &cb, threadID);
}
void wiRenderer::UpdateCameraCB(Camera* camera, GRAPHICSTHREAD threadID)
{
	CameraCB cb;

	cb.mVP = XMMatrixTranspose(camera->GetViewProjection());
	cb.mView = XMMatrixTranspose(camera->GetView());
	cb.mProj = XMMatrixTranspose(camera->GetProjection());
	cb.mCamPos = camera->translation;

	GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_CAMERA], &cb, threadID);
}
wiRenderer::APICB apiCB[GRAPHICSTHREAD_COUNT];
void wiRenderer::SetClipPlane(const XMFLOAT4& clipPlane, GRAPHICSTHREAD threadID)
{
	apiCB[threadID].clipPlane = clipPlane;
	GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_API], &apiCB[threadID], threadID);
}
void wiRenderer::SetAlphaRef(float alphaRef, GRAPHICSTHREAD threadID)
{
	if (alphaRef != apiCB[threadID].alphaRef)
	{
		apiCB[threadID].alphaRef = alphaRef;
		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_API], &apiCB[threadID], threadID);
	}
}
void wiRenderer::UpdateGBuffer(Texture2D* slot0, Texture2D* slot1, Texture2D* slot2, Texture2D* slot3, Texture2D* slot4, GRAPHICSTHREAD threadID)
{
	GetDevice()->BindResource(PS, slot0, TEXSLOT_GBUFFER0, threadID);
	GetDevice()->BindResource(PS, slot1, TEXSLOT_GBUFFER1, threadID);
	GetDevice()->BindResource(PS, slot2, TEXSLOT_GBUFFER2, threadID);
	GetDevice()->BindResource(PS, slot3, TEXSLOT_GBUFFER3, threadID);
	GetDevice()->BindResource(PS, slot4, TEXSLOT_GBUFFER4, threadID);

	GetDevice()->BindResource(CS, slot0, TEXSLOT_GBUFFER0, threadID);
	GetDevice()->BindResource(CS, slot1, TEXSLOT_GBUFFER1, threadID);
	GetDevice()->BindResource(CS, slot2, TEXSLOT_GBUFFER2, threadID);
	GetDevice()->BindResource(CS, slot3, TEXSLOT_GBUFFER3, threadID);
	GetDevice()->BindResource(CS, slot4, TEXSLOT_GBUFFER4, threadID);
}
void wiRenderer::UpdateDepthBuffer(Texture2D* depth, Texture2D* linearDepth, GRAPHICSTHREAD threadID)
{
	GetDevice()->BindResource(PS, depth, TEXSLOT_DEPTH, threadID);
	GetDevice()->BindResource(VS, depth, TEXSLOT_DEPTH, threadID);
	GetDevice()->BindResource(GS, depth, TEXSLOT_DEPTH, threadID);
	GetDevice()->BindResource(CS, depth, TEXSLOT_DEPTH, threadID);

	GetDevice()->BindResource(PS, linearDepth, TEXSLOT_LINEARDEPTH, threadID);
	GetDevice()->BindResource(VS, linearDepth, TEXSLOT_LINEARDEPTH, threadID);
	GetDevice()->BindResource(GS, linearDepth, TEXSLOT_LINEARDEPTH, threadID);
	GetDevice()->BindResource(CS, linearDepth, TEXSLOT_LINEARDEPTH, threadID);
}


Texture2D* wiRenderer::GetLuminance(Texture2D* sourceImage, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	static Texture2D* luminance_map = nullptr;
	static std::vector<Texture2D*> luminance_avg(0);
	if (luminance_map == nullptr)
	{
		SAFE_DELETE(luminance_map);
		for (auto& x : luminance_avg)
		{
			SAFE_DELETE(x);
		}
		luminance_avg.clear();

		// lower power of two
		//UINT minRes = wiMath::GetNextPowerOfTwo(min(device->GetScreenWidth(), device->GetScreenHeight())) / 2;

		TextureDesc desc;
		ZeroMemory(&desc, sizeof(desc));
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

		device->CreateTexture2D(&desc, nullptr, &luminance_map);

		while (desc.Width > 1)
		{
			desc.Width = max(desc.Width / 16, 1);
			desc.Height = desc.Width;

			Texture2D* tex = nullptr;
			device->CreateTexture2D(&desc, nullptr, &tex);

			luminance_avg.push_back(tex);
		}
	}
	if (luminance_map != nullptr)
	{
		// Pass 1 : Create luminance map from scene tex
		TextureDesc luminance_map_desc = luminance_map->GetDesc();
		device->BindComputePSO(CPSO[CSTYPE_LUMINANCE_PASS1], threadID);
		device->BindResource(CS, sourceImage, TEXSLOT_ONDEMAND0, threadID);
		device->BindUnorderedAccessResourceCS(luminance_map, 0, threadID);
		device->Dispatch(luminance_map_desc.Width/16, luminance_map_desc.Height/16, 1, threadID);

		// Pass 2 : Reduce for average luminance until we got an 1x1 texture
		TextureDesc luminance_avg_desc;
		for (size_t i = 0; i < luminance_avg.size(); ++i)
		{
			luminance_avg_desc = luminance_avg[i]->GetDesc();
			device->BindComputePSO(CPSO[CSTYPE_LUMINANCE_PASS2], threadID);
			device->BindUnorderedAccessResourceCS(luminance_avg[i], 0, threadID);
			if (i > 0)
			{
				device->BindResource(CS, luminance_avg[i-1], TEXSLOT_ONDEMAND0, threadID);
			}
			else
			{
				device->BindResource(CS, luminance_map, TEXSLOT_ONDEMAND0, threadID);
			}
			device->Dispatch(luminance_avg_desc.Width, luminance_avg_desc.Height, 1, threadID);
		}


		device->UnBindUnorderedAccessResources(0, 1, threadID);

		return luminance_avg.back();
	}

	return nullptr;
}

wiWaterPlane wiRenderer::GetWaterPlane()
{
	return waterPlane;
}

wiRenderer::Picked wiRenderer::Pick(RAY& ray, int pickType, uint32_t layerMask)
{
	std::vector<Picked> pickPoints;

	// pick meshes...
	CulledCollection culledRenderer;
	CulledList culledObjects;
	wiSPTree* searchTree = spTree;
	if (searchTree != nullptr)
	{
		searchTree->getVisible(ray, culledObjects);

		RayIntersectMeshes(ray, culledObjects, pickPoints, pickType, true, true, layerMask);
	}

	// pick other...
	for (auto& model : GetScene().models)
	{
		if (pickType & PICK_LIGHT)
		{
			for (auto& light : model->lights)
			{
				XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&ray.origin), XMLoadFloat3(&ray.origin) + XMLoadFloat3(&ray.direction), XMLoadFloat3(&light->translation));
				float dis = XMVectorGetX(disV);
				if (dis < wiMath::Distance(light->translation, cam->translation) * 0.05f)
				{
					Picked pick = Picked();
					pick.transform = light;
					pick.light = light;
					pick.distance = wiMath::Distance(light->translation, ray.origin) * 0.95f;
					pickPoints.push_back(pick);
				}
				//if (light->bounds.intersects(ray))
				//{
				//	Picked pick = Picked();
				//	pick.transform = light;
				//	pick.light = light;
				//	if (light->type == Light::DIRECTIONAL)
				//	{
				//		pick.distance = FLT_MAX;
				//	}
				//	else
				//	{
				//		pick.distance = wiMath::Distance(light->translation, ray.origin);
				//	}
				//	pickPoints.push_back(pick);
				//}
			}
		}
		if (pickType & PICK_DECAL)
		{
			for (auto& decal : model->decals)
			{
				//XMVECTOR localOrigin = XMLoadFloat3(&ray.origin), localDirection = XMLoadFloat3(&ray.direction);
				//XMMATRIX localTransform = XMLoadFloat4x4(&decal->world);
				//localTransform = XMMatrixInverse(nullptr, localTransform);
				//localOrigin = XMVector3Transform(localOrigin, localTransform);
				//localDirection = XMVector3TransformNormal(localDirection, localTransform);
				//RAY localRay = RAY(localOrigin, localDirection);
				//if (AABB(XMFLOAT3(-1, -1, -1), XMFLOAT3(1, 1, 1)).intersects(localRay))
				//{
				//	Picked pick = Picked();
				//	pick.transform = decal;
				//	pick.decal = decal;
				//	pick.distance = wiMath::Distance(decal->translation, ray.origin);
				//	pickPoints.push_back(pick);
				//}

				// decals are now picked like lights instead (user experience reasons)
				XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&ray.origin), XMLoadFloat3(&ray.origin) + XMLoadFloat3(&ray.direction), XMLoadFloat3(&decal->translation));
				float dis = XMVectorGetX(disV);
				if (dis < wiMath::Distance(decal->translation, cam->translation) * 0.05f)
				{
					Picked pick = Picked();
					pick.transform = decal;
					pick.decal = decal;
					pick.distance = wiMath::Distance(decal->translation, ray.origin) * 0.95f;
					pickPoints.push_back(pick);
				}
			}
		}
		if (pickType & PICK_FORCEFIELD)
		{
			for (auto& force : model->forces)
			{
				XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&ray.origin), XMLoadFloat3(&ray.origin) + XMLoadFloat3(&ray.direction), XMLoadFloat3(&force->translation));
				float dis = XMVectorGetX(disV);
				if (dis < wiMath::Distance(force->translation, cam->translation) * 0.05f)
				{
					Picked pick = Picked();
					pick.transform = force;
					pick.forceField = force;
					pick.distance = wiMath::Distance(force->translation, ray.origin) * 0.95f;
					pickPoints.push_back(pick);
				}
			}
		}
		if (pickType & PICK_EMITTER)
		{
			for (auto& object : model->objects)
			{
				if (object->eParticleSystems.empty())
				{
					continue;
				}

				XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&ray.origin), XMLoadFloat3(&ray.origin) + XMLoadFloat3(&ray.direction), XMLoadFloat3(&object->translation));
				float dis = XMVectorGetX(disV);
				if (dis < wiMath::Distance(object->translation, cam->translation) * 0.05f)
				{
					Picked pick = Picked();
					pick.transform = object;
					pick.object = object;
					pick.distance = wiMath::Distance(object->translation, ray.origin) * 0.95f;
					pickPoints.push_back(pick);
				}
			}
		}

		if (pickType & PICK_ENVPROBE)
		{
			for (auto& x : model->environmentProbes)
			{
				if (SPHERE(x->translation, 1).intersects(ray))
				{
					Picked pick = Picked();
					pick.transform = x;
					pick.envProbe = x;
					pick.distance = wiMath::Distance(x->translation, ray.origin);
					pickPoints.push_back(pick);
				}
			}
		}
		if (pickType & PICK_CAMERA)
		{
			for (auto& camera : model->cameras)
			{
				XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&ray.origin), XMLoadFloat3(&ray.origin) + XMLoadFloat3(&ray.direction), XMLoadFloat3(&camera->translation));
				float dis = XMVectorGetX(disV);
				if (dis < wiMath::Distance(camera->translation, cam->translation) * 0.05f)
				{
					Picked pick = Picked();
					pick.transform = camera;
					pick.camera = camera;
					pick.distance = wiMath::Distance(camera->translation, ray.origin) * 0.95f;
					pickPoints.push_back(pick);
				}
			}
		}
	}

	if (!pickPoints.empty()) {
		Picked min = pickPoints.front();
		for (unsigned int i = 1; i < pickPoints.size(); ++i) {
			if (pickPoints[i].distance < min.distance) {
				min = pickPoints[i];
			}
		}
		return min;
	}

	return Picked();
}
wiRenderer::Picked wiRenderer::Pick(long cursorX, long cursorY, int pickType, uint32_t layerMask)
{
	RAY ray = getPickRay(cursorX, cursorY);

	return Pick(ray, pickType, layerMask);
}

RAY wiRenderer::getPickRay(long cursorX, long cursorY){
	Camera* cam = getCamera();
	XMMATRIX V = cam->GetView();
	XMMATRIX P = cam->GetRealProjection();
	XMMATRIX W = XMMatrixIdentity();
	XMVECTOR& lineStart = XMVector3Unproject(XMVectorSet((float)cursorX, (float)cursorY, 0, 1), 0, 0, cam->width, cam->height, 0.0f, 1.0f, P, V, W);
	XMVECTOR& lineEnd = XMVector3Unproject(XMVectorSet((float)cursorX, (float)cursorY, 1, 1), 0, 0, cam->width, cam->height, 0.0f, 1.0f, P, V, W);
	XMVECTOR& rayDirection = XMVector3Normalize(XMVectorSubtract(lineEnd, lineStart));
	return RAY(lineStart, rayDirection);
}

void wiRenderer::RayIntersectMeshes(const RAY& ray, const CulledList& culledObjects, std::vector<Picked>& points,
	int pickType, bool dynamicObjects, bool onlyVisible, uint32_t layerMask)
{
	if (culledObjects.empty())
	{
		return;
	}

	const XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
	const XMVECTOR rayDirection = XMVector3Normalize(XMLoadFloat3(&ray.direction));

	// pre allocate helper vector array:
	static size_t _arraySize = 10000;
	static XMVECTOR* _vertices = (XMVECTOR*)_mm_malloc(sizeof(XMVECTOR)*_arraySize, 16);

	for (Cullable* culled : culledObjects)
	{
		Object* object = (Object*)culled;

		const uint32_t objectLayerMask = object->GetLayerMask();
		if (objectLayerMask & layerMask)
		{

			if (!(pickType & object->GetRenderTypes()))
			{
				continue;
			}
			if (!dynamicObjects && object->isDynamic())
			{
				continue;
			}
			if (onlyVisible && object->IsOccluded() && GetOcclusionCullingEnabled())
			{
				continue;
			}

			Mesh* mesh = object->mesh;
			if (mesh->vertices_POS.size() >= _arraySize)
			{
				// grow preallocated vector helper array
				_mm_free(_vertices);
				_arraySize = (mesh->vertices_POS.size() + 1) * 2;
				_vertices = (XMVECTOR*)_mm_malloc(sizeof(XMVECTOR)*_arraySize, 16);
			}

			const XMMATRIX objectMat = object->getMatrix();
			const XMMATRIX objectMat_Inverse = XMMatrixInverse(nullptr, objectMat);

			const XMVECTOR rayOrigin_local = XMVector3Transform(rayOrigin, objectMat_Inverse);
			const XMVECTOR rayDirection_local = XMVector3Normalize(XMVector3TransformNormal(rayDirection, objectMat_Inverse));

			Mesh::Vertex_FULL _tmpvert;

			if (object->isArmatureDeformed() && !object->mesh->armature->boneCollection.empty())
			{
				for (size_t i = 0; i < mesh->vertices_POS.size(); ++i)
				{
					_tmpvert = TransformVertex(mesh, (int)i);
					_vertices[i] = XMLoadFloat4(&_tmpvert.pos);
				}
			}
			else if (mesh->hasDynamicVB())
			{
				for (size_t i = 0; i < mesh->vertices_Transformed_POS.size(); ++i)
				{
					_vertices[i] = mesh->vertices_Transformed_POS[i].LoadPOS();
				}
			}
			else
			{
				for (size_t i = 0; i < mesh->vertices_POS.size(); ++i)
				{
					_vertices[i] = mesh->vertices_POS[i].LoadPOS();
				}
			}

			for (size_t i = 0; i < mesh->indices.size(); i += 3)
			{
				int i0 = mesh->indices[i], i1 = mesh->indices[i + 1], i2 = mesh->indices[i + 2];
				float distance;
				if (TriangleTests::Intersects(rayOrigin_local, rayDirection_local, _vertices[i0], _vertices[i1], _vertices[i2], distance))
				{
					XMVECTOR& pos = XMVector3Transform(XMVectorAdd(rayOrigin_local, rayDirection_local*distance), objectMat);
					XMVECTOR& nor = XMVector3Normalize(XMVector3TransformNormal(XMVector3Normalize(XMVector3Cross(XMVectorSubtract(_vertices[i2], _vertices[i1]), XMVectorSubtract(_vertices[i1], _vertices[i0]))), objectMat));
					Picked picked = Picked();
					picked.transform = object;
					picked.object = object;
					XMStoreFloat3(&picked.position, pos);
					XMStoreFloat3(&picked.normal, nor);
					picked.distance = wiMath::Distance(pos, rayOrigin);
					picked.subsetIndex = (int)mesh->vertices_FULL[i0].tex.z;
					points.push_back(picked);
				}
			}

		}

	}
}

void wiRenderer::CalculateVertexAO(Object* object)
{
	////TODO

	//static const float minAmbient = 0.05f;
	//static const float falloff = 0.1f;

	//Mesh* mesh = object->mesh;

	//XMMATRIX& objectMat = object->getMatrix();

	//CulledCollection culledRenderer;
	//CulledList culledObjects;
	//wiSPTree* searchTree = spTree;

	//int ind = 0;
	//for (SkinnedVertex& vert : mesh->vertices)
	//{
	//	float ambientShadow = 0.0f;

	//	XMFLOAT3 vPos, vNor;

	//	//XMVECTOR p = XMVector4Transform(XMVectorSet(vert.pos.x, vert.pos.y, vert.pos.z, 1), XMLoadFloat4x4(&object->world));
	//	//XMVECTOR n = XMVector3Transform(XMVectorSet(vert.nor.x, vert.nor.y, vert.nor.z, 0), XMLoadFloat4x4(&object->world));

	//	//XMStoreFloat3(&vPos, p);
	//	//XMStoreFloat3(&vNor, n);

	//	Vertex v = TransformVertex(mesh, vert, objectMat);
	//	vPos.x = v.pos.x;
	//	vPos.y = v.pos.y;
	//	vPos.z = v.pos.z;
	//	vNor.x = v.nor.x;
	//	vNor.y = v.nor.y;
	//	vNor.z = v.nor.z;

	//		RAY ray = RAY(vPos, vNor);
	//		XMVECTOR& rayOrigin = XMLoadFloat3(&ray.origin);
	//		XMVECTOR& rayDirection = XMLoadFloat3(&ray.direction);

	//		searchTree->getVisible(ray, culledObjects);

	//		std::vector<Picked> points;

	//		RayIntersectMeshes(ray, culledObjects, points, PICK_OPAQUE, false);


	//		if (!points.empty()){
	//			Picked min = points.front();
	//			float mini = wiMath::DistanceSquared(min.position, ray.origin);
	//			for (unsigned int i = 1; i<points.size(); ++i){
	//				if (float nm = wiMath::DistanceSquared(points[i].position, ray.origin)<mini){
	//					min = points[i];
	//					mini = nm;
	//				}
	//			}

	//			float ambientLightIntensity = wiMath::Clamp(abs(wiMath::Distance(ray.origin, min.position)) / falloff, 0, 1);
	//			ambientLightIntensity += minAmbient;

	//			vert.nor.w = ambientLightIntensity;
	//			mesh->vertices[ind].nor.w = ambientLightIntensity;
	//		}

	//		++ind;
	//}

	//mesh->calculatedAO = true;
}

Model* wiRenderer::LoadModel(const std::string& fileName, const XMMATRIX& transform)
{
	static int unique_identifier = 0;

	Model* model = new Model;
	model->LoadFromDisk(fileName);

	model->transform(transform);

	AddModel(model);

	LoadWorldInfo(fileName);

	unique_identifier++;

	return model;
}
void wiRenderer::LoadWorldInfo(const std::string& fileName)
{
	LoadWiWorldInfo(fileName, GetScene().worldInfo, GetScene().wind);
}
void wiRenderer::LoadDefaultLighting()
{
	GetScene().worldInfo.ambient = XMFLOAT3(0.3f, 0.3f, 0.3f);

	Light* defaultLight = new Light();
	defaultLight->name = "_WickedEngine_DefaultLight_";
	defaultLight->SetType(Light::DIRECTIONAL);
	defaultLight->color = XMFLOAT4(1, 1, 1, 1);
	defaultLight->enerDis = XMFLOAT4(3, 0, 0, 0);
	XMStoreFloat4(&defaultLight->rotation_rest, XMQuaternionRotationRollPitchYaw(0, -XM_PIDIV4, XM_PIDIV4));
	defaultLight->UpdateTransform();
	defaultLight->UpdateLight();

	Model* model = new Model;
	model->name = "_WickedEngine_DefaultLight_Holder_";
	model->lights.insert(defaultLight);
	GetScene().models.push_back(model);

	if (spTree_lights) {
		spTree_lights->AddObjects(spTree_lights->root, std::vector<Cullable*>(model->lights.begin(), model->lights.end()));
	}
	else
	{
		spTree_lights = new Octree(std::vector<Cullable*>(model->lights.begin(), model->lights.end()));
	}
}
Scene& wiRenderer::GetScene()
{
	if (scene == nullptr)
	{
		scene = new Scene;
	}
	return *scene;
}

void wiRenderer::SynchronizeWithPhysicsEngine(float dt)
{
	if (physicsEngine && GetGameSpeed())
	{
		physicsEngine->addWind(GetScene().wind.direction);

		// Update physics world data
		for (Model* model : GetScene().models)
		{
			for (Object* object : model->objects) 
			{
				Mesh* mesh = object->mesh;
				int pI = object->physicsObjectID;

				if (pI < 0 && (object->rigidBody || mesh->softBody))
				{
					// Register the objects with physics attributes that doesn't exist in the simulation
					physicsEngine->registerObject(object);
				}

				if (pI >= 0) 
				{
					if (mesh->softBody) 
					{
						int gvg = mesh->goalVG;
						if (gvg >= 0)
						{
							XMMATRIX worldMat = mesh->hasArmature() ? XMMatrixIdentity() : XMLoadFloat4x4(&object->world);
							int j = 0;
							for (std::map<int, float>::iterator it = mesh->vertexGroups[gvg].vertices.begin(); it != mesh->vertexGroups[gvg].vertices.end(); ++it)
							{
								int vi = (*it).first;
								Mesh::Vertex_FULL tvert = TransformVertex(mesh, vi, worldMat);
								mesh->goalPositions[j] = XMFLOAT3(tvert.pos.x, tvert.pos.y, tvert.pos.z);
								mesh->goalNormals[j] = XMFLOAT3(tvert.nor.x, tvert.nor.y, tvert.nor.z);
								++j;
							}
						}
						physicsEngine->connectSoftBodyToVertices(
							object->mesh, pI
							);
					}
					if (object->kinematic && object->rigidBody)
					{
						physicsEngine->transformBody(object->rotation, object->translation, pI);
					}
				}
			}
		}

		// Run physics simulation
		physicsEngine->Update(dt);

		// Retrieve physics simulation data
		for (Model* model : GetScene().models)
		{
			for (Object* object : model->objects) {
				int pI = object->physicsObjectID;
				if (pI >= 0 && !object->kinematic && (object->rigidBody || object->mesh->softBody)) {
					PHYSICS::PhysicsTransform* transform(physicsEngine->getObject(pI));
					object->translation_rest = transform->position;
					object->rotation_rest = transform->rotation;

					if (object->mesh->softBody) {
						object->scale_rest = XMFLOAT3(1, 1, 1);
						physicsEngine->connectVerticesToSoftBody(
							object->mesh, pI
							);
					}
				}
			}
		}


		physicsEngine->NextRunWorld();
	}
}

void wiRenderer::PutEnvProbe(const XMFLOAT3& position)
{
	EnvironmentProbe* probe = new EnvironmentProbe;
	probe->transform(position);

	GetScene().GetWorldNode()->environmentProbes.push_back(probe);
}

void wiRenderer::CreateImpostor(Mesh* mesh)
{
	Mesh::CreateImpostorVB();

	static const GRAPHICSTHREAD threadID;
	static const int res = 256;

	const AABB& bbox = mesh->aabb;
	const XMFLOAT3 extents = bbox.getHalfWidth();
	if (!mesh->impostorTarget.IsInitialized())
	{
		mesh->impostorTarget.Initialize(res * 6, res, true, RTFormat_impostor_albedo, 0);
		mesh->impostorTarget.Add(RTFormat_impostor_normal);			// normal, roughness
		mesh->impostorTarget.Add(RTFormat_impostor_surface);		// surface properties
	}

	Camera savedCam = *cam;

	GetDevice()->LOCK();

	BindPersistentState(threadID);

	const XMFLOAT4X4 __identity = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	struct InstBuf
	{
		Instance instance;
		InstancePrev instancePrev;
	};
	UINT instancesOffset; 
	volatile InstBuf* buff = (volatile InstBuf*)GetDevice()->AllocateFromRingBuffer(dynamicVertexBufferPool, sizeof(InstBuf), instancesOffset, threadID);
	buff->instance.Create(__identity);
	buff->instancePrev.Create(__identity);
	GetDevice()->InvalidateBufferAccess(dynamicVertexBufferPool, threadID);

	GPUBuffer* vbs[] = {
		mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_POS != nullptr ? mesh->streamoutBuffer_POS : mesh->vertexBuffer_POS),
		mesh->vertexBuffer_TEX,
		mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_PRE != nullptr ? mesh->streamoutBuffer_PRE : mesh->vertexBuffer_POS),
		dynamicVertexBufferPool
	};
	UINT strides[] = {
		sizeof(Mesh::Vertex_POS),
		sizeof(Mesh::Vertex_TEX),
		sizeof(Mesh::Vertex_POS),
		sizeof(InstBuf)
	};
	UINT offsets[] = {
		mesh->hasDynamicVB() ? mesh->bufferOffset_POS : 0,
		0,
		mesh->hasDynamicVB() ? mesh->bufferOffset_PRE : 0,
		instancesOffset
	};
	GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);

	GetDevice()->BindIndexBuffer(mesh->indexBuffer, mesh->GetIndexFormat(), 0, threadID);

	GetDevice()->BindGraphicsPSO(PSO_captureimpostor, threadID);

	ViewPort savedViewPort = mesh->impostorTarget.viewPort;
	mesh->impostorTarget.Activate(threadID, 0, 0, 0, 0);
	for (size_t i = 0; i < 6; ++i)
	{
		mesh->impostorTarget.viewPort.Height = (float)res;
		mesh->impostorTarget.viewPort.Width = (float)res;
		mesh->impostorTarget.viewPort.TopLeftX = (float)(i*res);
		mesh->impostorTarget.viewPort.TopLeftY = 0.f;
		mesh->impostorTarget.Set(threadID);

		cam->Clear();
		cam->Translate(bbox.getCenter());
		switch (i)
		{
		case 0:
		{
			// front capture
			XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(-extents.x, extents.x, -extents.y, extents.y, -extents.z, extents.z);
			XMStoreFloat4x4(&cam->Projection, ortho);
		}
		break;
		case 1:
		{
			// right capture
			XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(-extents.z, extents.z, -extents.y, extents.y, -extents.x, extents.x);
			XMStoreFloat4x4(&cam->Projection, ortho);
			cam->RotateRollPitchYaw(XMFLOAT3(0, -XM_PIDIV2, 0));
		}
		break;
		case 2:
		{
			// back capture
			XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(-extents.x, extents.x, -extents.y, extents.y, -extents.z, extents.z);
			XMStoreFloat4x4(&cam->Projection, ortho);
			cam->RotateRollPitchYaw(XMFLOAT3(0, -XM_PI, 0));
		}
		break;
		case 3:
		{
			// left capture
			XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(-extents.z, extents.z, -extents.y, extents.y, -extents.x, extents.x);
			XMStoreFloat4x4(&cam->Projection, ortho);
			cam->RotateRollPitchYaw(XMFLOAT3(0, XM_PIDIV2, 0));
		}
		break;
		case 4:
		{
			// bottom capture
			XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(-extents.x, extents.x, -extents.z, extents.z, -extents.y, extents.y);
			XMStoreFloat4x4(&cam->Projection, ortho);
			cam->RotateRollPitchYaw(XMFLOAT3(-XM_PIDIV2, 0, 0));
		}
		break;
		case 5:
		{
			// top capture
			XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(-extents.x, extents.x, -extents.z, extents.z, -extents.y, extents.y);
			XMStoreFloat4x4(&cam->Projection, ortho);
			cam->RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
		}
		break;
		default:
			break;
		}
		cam->UpdateProps();
		UpdateCameraCB(cam, threadID);

		for (MeshSubset& subset : mesh->subsets)
		{
			if (subset.subsetIndices.empty())
			{
				continue;
			}
			if (!subset.material->IsTransparent() && !subset.material->isSky && !subset.material->water)
			{
				GetDevice()->BindConstantBuffer(PS, &subset.material->constantBuffer, CB_GETBINDSLOT(MaterialCB), threadID);

				GetDevice()->BindResource(PS, subset.material->GetBaseColorMap(), TEXSLOT_ONDEMAND0, threadID);
				GetDevice()->BindResource(PS, subset.material->GetNormalMap(), TEXSLOT_ONDEMAND1, threadID);
				GetDevice()->BindResource(PS, subset.material->GetSurfaceMap(), TEXSLOT_ONDEMAND2, threadID);

				GetDevice()->DrawIndexedInstanced((int)subset.subsetIndices.size(), 1, subset.indexBufferOffset, 0, 0, threadID);
			}
		}

	}
	GetDevice()->GenerateMips(mesh->impostorTarget.GetTexture(), threadID);

	//GetDevice()->SaveTexturePNG("C:\\Users\\turanszkij\\Documents\\asd_col.png", mesh->impostorTarget.GetTexture(0), threadID);
	//GetDevice()->SaveTexturePNG("C:\\Users\\turanszkij\\Documents\\asd_nor.png", mesh->impostorTarget.GetTexture(1), threadID);
	//GetDevice()->SaveTexturePNG("C:\\Users\\turanszkij\\Documents\\asd_rou.png", mesh->impostorTarget.GetTexture(2), threadID);
	//GetDevice()->SaveTexturePNG("C:\\Users\\turanszkij\\Documents\\asd_ref.png", mesh->impostorTarget.GetTexture(3), threadID);
	//GetDevice()->SaveTexturePNG("C:\\Users\\turanszkij\\Documents\\asd_met.png", mesh->impostorTarget.GetTexture(4), threadID);

	mesh->impostorTarget.viewPort = savedViewPort;
	*cam = savedCam;
	UpdateCameraCB(cam, threadID);

	GetDevice()->UNLOCK();
}

void wiRenderer::AddRenderableTranslator(wiTranslator* translator)
{
	renderableTranslators.push_back(translator);
}

void wiRenderer::AddRenderableBox(const XMFLOAT4X4& boxMatrix, const XMFLOAT4& color)
{
	renderableBoxes.push_back(pair<XMFLOAT4X4,XMFLOAT4>(boxMatrix,color));
}



void wiRenderer::AddModel(Model* model)
{
	GetScene().AddModel(model);

	FixedUpdate();

	// add object batch 
	{
		vector<Cullable*> collection(model->objects.begin(), model->objects.end());
		if (spTree != nullptr)
		{
			spTree->AddObjects(spTree->root, collection);
		}
		else
		{
			spTree = new Octree(collection);
		}
	}

	// add light batch
	{
		vector<Cullable*> collection(model->lights.begin(), model->lights.end());
		if (spTree_lights != nullptr)
		{
			spTree_lights->AddObjects(spTree_lights->root, collection);
		}
		else
		{
			spTree_lights = new Octree(collection);
		}
	}

	SetUpCubes();
	SetUpBoneLines();
}

void wiRenderer::Add(Object* value)
{
	if (value->parentModel == nullptr)
	{
		GetScene().GetWorldNode()->Add(value);
	}
	else
	{
		value->parentModel->Add(value);
	}

	if (value->parent == nullptr)
	{
		value->attachTo(GetScene().GetWorldNode());
	}

	vector<Cullable*> collection(0);
	collection.push_back(value);
	if (spTree != nullptr) 
	{
		spTree->AddObjects(spTree->root, collection);
	}
	else
	{
		spTree = new Octree(collection);
	}
}
void wiRenderer::Add(Light* value)
{
	if (value->parentModel == nullptr)
	{
		GetScene().GetWorldNode()->Add(value);
	}
	else
	{
		value->parentModel->Add(value);
	}

	if (value->parent == nullptr)
	{
		value->attachTo(GetScene().GetWorldNode());
	}

	vector<Cullable*> collection(0);
	collection.push_back(value);
	if (spTree_lights != nullptr) 
	{
		spTree_lights->AddObjects(spTree_lights->root, collection);
	}
	else
	{
		spTree_lights = new Octree(collection);
	}
}
void wiRenderer::Add(ForceField* value)
{
	if (value->parentModel == nullptr)
	{
		GetScene().GetWorldNode()->Add(value);
	}
	else
	{
		value->parentModel->Add(value);
	}

	if (value->parent == nullptr)
	{
		value->attachTo(GetScene().GetWorldNode());
	}
}
void wiRenderer::Add(Camera* value)
{
	if (value->parentModel == nullptr)
	{
		GetScene().GetWorldNode()->Add(value);
	}
	else
	{
		value->parentModel->Add(value);
	}

	if (value->parent == nullptr)
	{
		value->attachTo(GetScene().GetWorldNode());
	}
}

void wiRenderer::Remove(Object* value)
{
	if (value != nullptr)
	{
		for (auto& x : GetScene().models)
		{
			x->objects.erase(value);
			value->parentModel = nullptr;
		}
		spTree->Remove(value);
		value->detach();
	}
}
void wiRenderer::Remove(Light* value)
{
	if (value != nullptr)
	{
		for (auto& x : GetScene().models)
		{
			x->lights.erase(value);
			value->parentModel = nullptr;
		}
		spTree_lights->Remove(value);
		value->detach();
	}
}
void wiRenderer::Remove(Decal* value)
{
	if (value != nullptr)
	{
		for (auto& x : GetScene().models)
		{
			x->decals.erase(value);
			value->parentModel = nullptr;
		}
		value->detach();
	}
}
void wiRenderer::Remove(EnvironmentProbe* value)
{
	if (value != nullptr)
	{
		for (auto& x : GetScene().models)
		{
			x->environmentProbes.remove(value);
			value->parentModel = nullptr;
		}
		value->detach();
	}
}
void wiRenderer::Remove(ForceField* value)
{
	if (value != nullptr)
	{
		for (auto& x : GetScene().models)
		{
			x->forces.erase(value);
			value->parentModel = nullptr;
		}
		value->detach();
	}
}
void wiRenderer::Remove(Camera* value)
{
	if (value != nullptr)
	{
		for (auto& x : GetScene().models)
		{
			x->cameras.remove(value);
			value->parentModel = nullptr;
		}
		value->detach();
	}
}


void wiRenderer::SetOcclusionCullingEnabled(bool value)
{
	static bool initialized = false;

	if (!initialized && value == true)
	{
		initialized = true;

		GPUQueryDesc desc;
		desc.Type = GPU_QUERY_TYPE_OCCLUSION_PREDICATE;
		desc.MiscFlags = 0;
		desc.async_latency = 1;

		for (int i = 0; i < ARRAYSIZE(occlusionQueries); ++i)
		{
			wiRenderer::GetDevice()->CreateQuery(&desc, &occlusionQueries[i]);
			occlusionQueries[i].result_passed = TRUE;
		}
	}

	occlusionCulling = value;
}

bool wiRenderer::GetAdvancedRefractionsEnabled()
{
	return advancedRefractions && GetDevice()->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_UNORDEREDACCESSTEXTURE_LOAD_FORMAT_EXT);
}

void wiRenderer::SetOceanEnabled(bool enabled, const wiOceanParameter& params)
{
	SAFE_DELETE(ocean);

	if (enabled)
	{
		ocean = new wiOcean(params);
	}
}
