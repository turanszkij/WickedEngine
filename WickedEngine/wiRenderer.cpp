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
#include "wiGraphicsDevice_DX11.h"
#include "wiTranslator.h"
#include "wiRectPacker.h"
#include "wiBackLog.h"
#include "wiProfiler.h"
#include <wiOcean.h>

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
bool wiRenderer::wireRender = false, wiRenderer::debugSpheres = false, wiRenderer::debugBoneLines = false, wiRenderer::debugPartitionTree = false, wiRenderer::debugEmitters = false
, wiRenderer::debugEnvProbes = false, wiRenderer::debugForceFields = false, wiRenderer::gridHelper = false, wiRenderer::voxelHelper = false, wiRenderer::requestReflectionRendering = false, wiRenderer::advancedLightCulling = true
, wiRenderer::advancedRefractions = false;
float wiRenderer::SPECULARAA = 0.0f;
float wiRenderer::renderTime = 0, wiRenderer::renderTime_Prev = 0, wiRenderer::deltaTime = 0;
XMFLOAT2 wiRenderer::temporalAAJitter = XMFLOAT2(0, 0), wiRenderer::temporalAAJitterPrev = XMFLOAT2(0, 0);
float wiRenderer::RESOLUTIONSCALE = 1.0f;
GPUQuery wiRenderer::occlusionQueries[];
UINT wiRenderer::entityArrayOffset_ForceFields = 0, wiRenderer::entityArrayCount_ForceFields = 0;

Texture2D* wiRenderer::enviroMap,*wiRenderer::colorGrading;
float wiRenderer::GameSpeed=1,wiRenderer::overrideGameSpeed=1;
bool wiRenderer::debugLightCulling = false;
bool wiRenderer::occlusionCulling = false;
bool wiRenderer::temporalAA = false, wiRenderer::temporalAADEBUG = false;
EnvironmentProbe* wiRenderer::globalEnvProbes[] = { nullptr,nullptr };
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

void wiRenderer::InitDevice(wiWindowRegistration::window_type window, bool fullscreen)
{
	SAFE_DELETE(graphicsDevice);
	graphicsDevice = new GraphicsDevice_DX11(window, fullscreen);
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
	for (HitSphere* x : spheres)
		delete x;
	spheres.clear();
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

	LoadShaders();

	cam = new Camera();
	cam->SetUp((float)GetInternalResolution().x, (float)GetInternalResolution().y, 0.1f, 800);
	refCam = new Camera();
	refCam->SetUp((float)GetInternalResolution().x, (float)GetInternalResolution().y, 0.1f, 800);
	prevFrameCam = new Camera;
	

	wireRender=false;
	debugSpheres=false;

	wiRenderer::SetUpStates();
	wiRenderer::LoadBuffers();
	
	wiHairParticle::SetUpStatic();
	wiEmittedParticle::SetUpStatic();

	GameSpeed=1;

	resetVertexCount();
	resetVisibleObjectCount();

	GetScene().wind=Wind();

	Cube::LoadStatic();
	HitSphere::SetUpStatic();

	spTree_lights=nullptr;


	waterRipples.resize(0);

	
	normalMapRT.Initialize(
		GetInternalResolution().x
		, GetInternalResolution().y
		,false,FORMAT_R8G8B8A8_SNORM
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
	HitSphere::CleanUpStatic();


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

void wiRenderer::UpdateSpheres()
{
	//for(int i=0;i<spheres.size();i++){
	//	Armature* armature=spheres[i]->parentArmature;
	//	Bone* bone=(Bone*)spheres[i]->parent;

	//	//spheres[i]->world = *bone->boneRelativity;
	//	//XMStoreFloat3( &spheres[i]->translation, XMVector3Transform( XMLoadFloat3(&spheres[i]->translation_rest),XMLoadFloat4x4(bone->boneRelativity) ) );
	//	XMStoreFloat3( &spheres[i]->translation, 
	//		XMVector3Transform( XMLoadFloat3(&spheres[i]->translation_rest),XMLoadFloat4x4(&bone->recursiveRestInv)*XMLoadFloat4x4(&bone->world) ) 
	//		);
	//}

	//for(HitSphere* s : spheres){
	//	s->getMatrix();
	//	s->center=s->translation;
	//	s->radius=s->radius_saved*s->scale.x;
	//}
}
float wiRenderer::getSphereRadius(const int& index){
	return spheres[index]->radius;
}
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

	// The other constant buffers will be updated frequently (>= per frame) so they should reside in DYNAMIC GPU memory!
	bd.Usage = USAGE_DYNAMIC;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	bd.ByteWidth = sizeof(FrameCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_FRAME]);

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

void wiRenderer::LoadShaders()
{
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION",		0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
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
			{ "POSITION",		0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",			0, Mesh::Vertex_NOR::FORMAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",		0, Mesh::Vertex_TEX::FORMAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",		1, Mesh::Vertex_POS::FORMAT, 3, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "MATI",			0, FORMAT_R32G32B32A32_FLOAT, 4, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			1, FORMAT_R32G32B32A32_FLOAT, 4, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			2, FORMAT_R32G32B32A32_FLOAT, 4, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR_DITHER",	0, FORMAT_R32G32B32A32_FLOAT, 4, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATIPREV",		0, FORMAT_R32G32B32A32_FLOAT, 5, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATIPREV",		1, FORMAT_R32G32B32A32_FLOAT, 5, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATIPREV",		2, FORMAT_R32G32B32A32_FLOAT, 5, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_common.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr){
			vertexShaders[VSTYPE_OBJECT_COMMON] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_OBJECT_ALL] = vsinfo->vertexLayout;
		}
	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION",		0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

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
			{ "POSITION",		0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",		0, Mesh::Vertex_TEX::FORMAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "MATI",			0, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			1, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI",			2, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR_DITHER",	0, FORMAT_R32G32B32A32_FLOAT, 2, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_simple.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr){
			vertexShaders[VSTYPE_OBJECT_SIMPLE] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_OBJECT_POS_TEX] = vsinfo->vertexLayout;
		}
	}
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION",		0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

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
			{ "POSITION",		0, Mesh::Vertex_POS::FORMAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",		0, Mesh::Vertex_TEX::FORMAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

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
	vertexShaders[VSTYPE_VOLUMESPOTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vSpotLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOLUMEPOINTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vPointLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOLUMESPHERELIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vSphereLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOLUMEDISCLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vDiscLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOLUMERECTANGLELIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vRectangleLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOLUMETUBELIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vTubeLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
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
	vertexShaders[VSTYPE_OCEANGRID] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "oceanGridVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;


	pixelShaders[PSTYPE_OBJECT_DEFERRED] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_deferred.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_DEFERRED_NORMALMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_deferred_normalmap.cso", wiResourceManager::PIXELSHADER));	
	pixelShaders[PSTYPE_OBJECT_DEFERRED_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_deferred_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_DEFERRED_NORMALMAP_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_deferred_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_NORMALMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_transparent.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT_NORMALMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_transparent_normalmap.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_NORMALMAP_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_transparent_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT_NORMALMAP_PLANARREFLECTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_transparent_normalmap_planarreflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_NORMALMAP_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_transparent_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT_NORMALMAP_POM] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_transparent_normalmap_pom.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_WATER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forward_dirlight_water.cso", wiResourceManager::PIXELSHADER));

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

	pixelShaders[PSTYPE_OBJECT_DEBUG] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_debug.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_SIMPLEST] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_simplest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_BLACKOUT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_blackout.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TEXTUREONLY] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_textureonly.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_ALPHATESTONLY] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_alphatestonly.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVIRONMENTALLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "environmentalLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DIRLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DIRLIGHT_SOFT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightSoftPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_POINTLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "pointLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SPOTLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "spotLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SPHERELIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "sphereLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DISCLIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "discLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_RECTANGLELIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "rectangleLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_TUBELIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "tubeLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOLUMELIGHT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "volumeLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DECAL] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "decalPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP_SKY] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMap_skyPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_CAPTUREIMPOSTOR] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "captureImpostorPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_CUBEMAP] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubemapPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_LINE] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "linesPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SKY] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SUN] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "sunPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOW_ALPHATEST] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowPS_alphatest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowPS_alphatest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_TRAIL] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "trailPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOXELIZER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_voxelizer.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOXEL] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_FORCEFIELDVISUALIZER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "forceFieldVisualizerPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OCEANGRID] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "oceanGridPS.cso", wiResourceManager::PIXELSHADER));


	geometryShaders[GSTYPE_ENVMAP] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_ENVMAP_SKY] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMap_skyGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_SHADOWCUBEMAPRENDER] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowGS_alphatest.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_VOXELIZER] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectGS_voxelizer.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_VOXEL] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_OCEANGRID] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "oceanGridGS.cso", wiResourceManager::GEOMETRYSHADER));


	computeShaders[CSTYPE_LUMINANCE_PASS1] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "luminancePass1CS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_LUMINANCE_PASS2] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "luminancePass2CS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEFRUSTUMS] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "tileFrustumsCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEDLIGHTCULLING] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightCullingCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEDLIGHTCULLING_ADVANCED] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightCullingCS_ADVANCED.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEDLIGHTCULLING_DEBUG] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightCullingCS_DEBUG.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEDLIGHTCULLING_ADVANCED_DEBUG] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightCullingCS_ADVANCED_DEBUG.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEDLIGHTCULLING_DEFERRED] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightCullingCS_DEFERRED.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEDLIGHTCULLING_DEFERRED_ADVANCED] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightCullingCS_DEFERRED_ADVANCED.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEDLIGHTCULLING_DEFERRED_DEBUG] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightCullingCS_DEFERRED_DEBUG.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEDLIGHTCULLING_DEFERRED_ADVANCED_DEBUG] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightCullingCS_DEFERRED_ADVANCED_DEBUG.cso", wiResourceManager::COMPUTESHADER));
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


	hullShaders[HSTYPE_OBJECT] = static_cast<HullShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectHS.cso", wiResourceManager::HULLSHADER));


	domainShaders[DSTYPE_OBJECT] = static_cast<DomainShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectDS.cso", wiResourceManager::DOMAINSHADER));



}

void wiRenderer::ReloadShaders(const std::string& path)
{

	if (path.length() > 0)
	{
		SHADERPATH = path;
	}

	GetDevice()->LOCK();

	wiResourceManager::GetShaderManager()->CleanUp();
	LoadShaders();
	wiHairParticle::LoadShaders();
	wiEmittedParticle::LoadShaders();
	wiFont::LoadShaders();
	wiImage::LoadShaders();
	wiLensFlare::LoadShaders();
	wiOcean::LoadShaders();
	CSFFT_512x512_Data_t::LoadShaders();

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
	rs.ScissorEnable=false;
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
	rs.ScissorEnable=false;
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
	rs.ScissorEnable=false;
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
	rs.ScissorEnable=false;
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
	rs.ScissorEnable=false;
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
	rs.ScissorEnable=false;
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
	rs.ScissorEnable=false;
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
	rs.ScissorEnable = false;
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
	rs.ScissorEnable = false;
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
	rs.ScissorEnable = false;
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
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_GREATER;
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_HAIRALPHACOMPOSITION]);


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
}

void wiRenderer::BindPersistentState(GRAPHICSTHREAD threadID)
{
	for (int i = 0; i < SSLOT_COUNT; ++i)
	{
		GetDevice()->BindSamplerPS(samplers[i], i, threadID);
		GetDevice()->BindSamplerVS(samplers[i], i, threadID);
		GetDevice()->BindSamplerGS(samplers[i], i, threadID);
		GetDevice()->BindSamplerDS(samplers[i], i, threadID);
		GetDevice()->BindSamplerHS(samplers[i], i, threadID);
		GetDevice()->BindSamplerCS(samplers[i], i, threadID);
	}


	GetDevice()->BindConstantBufferPS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	GetDevice()->BindConstantBufferVS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	GetDevice()->BindConstantBufferGS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	GetDevice()->BindConstantBufferHS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	GetDevice()->BindConstantBufferDS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	GetDevice()->BindConstantBufferCS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);

	GetDevice()->BindConstantBufferPS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	GetDevice()->BindConstantBufferVS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	GetDevice()->BindConstantBufferGS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	GetDevice()->BindConstantBufferHS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	GetDevice()->BindConstantBufferDS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	GetDevice()->BindConstantBufferCS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);

	GetDevice()->BindConstantBufferPS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	GetDevice()->BindConstantBufferVS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	GetDevice()->BindConstantBufferGS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	GetDevice()->BindConstantBufferHS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	GetDevice()->BindConstantBufferDS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	GetDevice()->BindConstantBufferCS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);

	GetDevice()->BindConstantBufferVS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	GetDevice()->BindConstantBufferPS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	GetDevice()->BindConstantBufferGS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	GetDevice()->BindConstantBufferDS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	GetDevice()->BindConstantBufferHS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	GetDevice()->BindConstantBufferCS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);

	GetDevice()->BindConstantBufferVS(constantBuffers[CBTYPE_API], CB_GETBINDSLOT(APICB), threadID);
	GetDevice()->BindConstantBufferPS(constantBuffers[CBTYPE_API], CB_GETBINDSLOT(APICB), threadID);
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
Transform* wiRenderer::getTransformByID(unsigned long long id)
{
	return GetScene().GetWorldNode()->find(id);
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
	ss<<armature->unidentified_name<<get;
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
HitSphere* wiRenderer::getSphereByName(const std::string& get){
	for(HitSphere* hs : spheres)
		if(!hs->name.compare(get))
			return hs;
	return nullptr;
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
	XMVECTOR pos = mesh->vertices_POS[vertexI].Load();
	XMVECTOR nor = XMLoadFloat4(&mesh->vertices_NOR[vertexI].GetNor_FULL());

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
	retV.tex = XMFLOAT4(
		mesh->vertices_TEX[vertexI].tex.x,
		mesh->vertices_TEX[vertexI].tex.y,
		mesh->vertices_TEX[vertexI].tex.z,
		mesh->vertices_TEX[vertexI].tex.w
	);

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

	// Environment probe sorting:
	{
		ZeroMemory(globalEnvProbes, sizeof(globalEnvProbes));
		GetScene().environmentProbes.sort(
			[&](EnvironmentProbe* a, EnvironmentProbe* b) {
			return wiMath::DistanceSquared(a->translation, getCamera()->translation) < wiMath::DistanceSquared(b->translation, getCamera()->translation);
		}
		);
		int envProbeInd = 0;
		for (auto& x : GetScene().environmentProbes)
		{
			globalEnvProbes[envProbeInd] = x;
			envProbeInd++;
			if (envProbeInd >= ARRAYSIZE(globalEnvProbes))
				break;
		}
	}

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

			if (spTree != nullptr)
			{
				CulledList culledObjects;
				spTree->getVisible(camera->frustum, culledObjects, wiSPTree::SortType::SP_TREE_SORT_FRONT_TO_BACK);
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
				Frustum frustum;
				frustum.ConstructFrustum(min(camera->zFarP, GetScene().worldInfo.fogSEH.y), camera->realProjection, camera->View);

				for (Model* model : GetScene().models)
				{
					if (model->decals.empty())
						continue;

					for (Decal* decal : model->decals)
					{
						if ((decal->texture || decal->normal) && frustum.CheckBox(decal->bounds))
						{
							x.second.culledDecals.push_back(decal);
						}
					}
				}

				spTree_lights->getVisible(frustum, culling.culledLights, wiSPTree::SortType::SP_TREE_SORT_NONE);

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
					culling.culledLight_count++;
					Light* l = (Light*)c;
					l->entityArray_index = i;

					// Link shadowmaps to lights till there are free slots

					l->shadowMap_index = -1;

					if (l->shadow)
					{
						switch (l->GetType())
						{
						case Light::DIRECTIONAL:
							if ((shadowCounter_2D + 2) < SHADOWCOUNT_2D)
							{
								l->shadowMap_index = shadowCounter_2D;
								shadowCounter_2D += 3;
							}
							break;
						case Light::SPOT:
							if (shadowCounter_2D < SHADOWCOUNT_2D)
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
							if (shadowCounter_Cube < SHADOWCOUNT_CUBE)
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
	renderTime = (float)((wiTimer::TotalTime()) / 1000.0 * GameSpeed);
	deltaTime = dt;
}
void wiRenderer::UpdateRenderData(GRAPHICSTHREAD threadID)
{
	UpdateWorldCB(threadID); // only commits when parameters are changed
	UpdateFrameCB(threadID);
	BindPersistentState(threadID);

	ManageDecalAtlas(threadID);

	const FrameCulling& mainCameraCulling = frameCullings[getCamera()];

	// Fill Light Array with lights + decals in the frustum:
	{
		const CulledList& culledLights = mainCameraCulling.culledLights;

		static ShaderEntityType* entityArray = (ShaderEntityType*)_mm_malloc(sizeof(ShaderEntityType)*MAX_SHADER_ENTITY_COUNT, 16);
		static XMMATRIX* matrixArray = (XMMATRIX*)_mm_malloc(sizeof(XMMATRIX)*MATRIXARRAY_COUNT, 16);

		const XMMATRIX viewMatrix = cam->GetView();

		UINT entityCounter = 0;
		UINT matrixCounter = 0;

		entityArrayOffset_ForceFields = 0;
		entityArrayCount_ForceFields = 0;

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

				if (shadowIndex >= 0)
				{
					matrixArray[shadowIndex + 0] = l->shadowCam_dirLight[0].getVP();
					matrixArray[shadowIndex + 1] = l->shadowCam_dirLight[1].getVP();
					matrixArray[shadowIndex + 2] = l->shadowCam_dirLight[2].getVP();
					matrixCounter = max(matrixCounter, (UINT)shadowIndex + 2);
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
					matrixCounter = max(matrixCounter, (UINT)shadowIndex + 2);
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

		const GPUResource* resources[] = {
			resourceBuffers[RBTYPE_ENTITYARRAY],
			resourceBuffers[RBTYPE_MATRIXARRAY],
		};
		GetDevice()->BindResourcesVS(resources, SBSLOT_ENTITYARRAY, ARRAYSIZE(resources), threadID);
		GetDevice()->BindResourcesPS(resources, SBSLOT_ENTITYARRAY, ARRAYSIZE(resources), threadID);
		GetDevice()->BindResourcesCS(resources, SBSLOT_ENTITYARRAY, ARRAYSIZE(resources), threadID);
	}

	wiProfiler::GetInstance().BeginRange("Skinning", wiProfiler::DOMAIN_GPU, threadID);
	GetDevice()->EventBegin("Skinning", threadID);
	{
		bool streamOutSetUp = false;

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
					&& mesh->streamoutBuffer_POS.IsValid() && mesh->vertexBuffer_POS.IsValid())
				{
					Armature* armature = mesh->armature;

					if (!streamOutSetUp)
					{
						// Set up skinning shader
						streamOutSetUp = true;
						GetDevice()->BindVertexLayout(nullptr, threadID);
						GPUBuffer* vbs[] = {
							nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
						};
						const UINT strides[] = {
							0,0,0,0,0,0,0,0
						};
						GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
						GetDevice()->BindCS(computeShaders[CSTYPE_SKINNING], threadID);
					}

					// Upload bones for skinning to shader
					for (unsigned int k = 0; k < armature->boneCollection.size(); k++)
					{
						armature->boneData[k].Create(armature->boneCollection[k]->boneRelativity);
					}
					GetDevice()->UpdateBuffer(&armature->boneBuffer, armature->boneData.data(), threadID, (int)(sizeof(Armature::ShaderBoneType) * armature->boneCollection.size()));
					GetDevice()->BindResourceCS(&armature->boneBuffer, SKINNINGSLOT_IN_BONEBUFFER, threadID);

					// Do the skinning
					const GPUResource* vbs[] = {
						&mesh->vertexBuffer_POS,
						&mesh->vertexBuffer_NOR,
						&mesh->vertexBuffer_BON,
					};
					const GPUUnorderedResource* sos[] = {
						&mesh->streamoutBuffer_POS,
						&mesh->streamoutBuffer_NOR,
						&mesh->streamoutBuffer_PRE,
					};

					GetDevice()->BindResourcesCS(vbs, SKINNINGSLOT_IN_VERTEX_POS, ARRAYSIZE(vbs), threadID);
					GetDevice()->BindUnorderedAccessResourcesCS(sos, 0, ARRAYSIZE(sos), threadID);

					GetDevice()->Dispatch((UINT)ceilf((float)mesh->vertices_POS.size() / SKINNING_COMPUTE_THREADCOUNT), 1, 1, threadID);

				}
				else if (mesh->hasDynamicVB())
				{
					// Upload CPU skinned vertex buffer (Soft body VB)
					mesh->bufferOffset_POS = GetDevice()->AppendRingBuffer(dynamicVertexBufferPool, mesh->vertices_Transformed_POS.data(), sizeof(Mesh::Vertex_POS)*mesh->vertices_Transformed_POS.size(), threadID);
					mesh->bufferOffset_NOR = GetDevice()->AppendRingBuffer(dynamicVertexBufferPool, mesh->vertices_Transformed_NOR.data(), sizeof(Mesh::Vertex_NOR)*mesh->vertices_Transformed_NOR.size(), threadID);
					mesh->bufferOffset_PRE = GetDevice()->AppendRingBuffer(dynamicVertexBufferPool, mesh->vertices_Transformed_PRE.data(), sizeof(Mesh::Vertex_POS)*mesh->vertices_Transformed_PRE.size(), threadID);
				}
			}
		}

		if (streamOutSetUp)
		{
			// Unload skinning shader
			GetDevice()->BindCS(nullptr, threadID);
			GetDevice()->UnBindUnorderedAccessResources(0, 3, threadID);
			GetDevice()->UnBindResources(SKINNINGSLOT_IN_VERTEX_POS, 4, threadID);
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

	// Render out of date environment probes:
	RefreshEnvProbes(threadID);

	// Bind environment probes:
	{
		const GPUResource* envMaps[] = {
			globalEnvProbes[0] == nullptr ? enviroMap : globalEnvProbes[0]->cubeMap.GetTexture(),
			globalEnvProbes[1] == nullptr ? enviroMap : globalEnvProbes[1]->cubeMap.GetTexture(),
		};
		GetDevice()->BindResourcesPS(envMaps, TEXSLOT_ENV0, ARRAYSIZE(envMaps), threadID);
		GetDevice()->BindResourcesCS(envMaps, TEXSLOT_ENV0, ARRAYSIZE(envMaps), threadID);
	}
}
void wiRenderer::OcclusionCulling_Render(GRAPHICSTHREAD threadID)
{
	if (!GetOcclusionCullingEnabled() || spTree == nullptr)
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

		GetDevice()->BindRasterizerState(rasterizers[RSTYPE_OCCLUDEE], threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_COLORWRITEDISABLE], threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_DEFAULT, threadID);
		GetDevice()->BindVertexLayout(nullptr, threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_CUBE], threadID);
		GetDevice()->BindPS(nullptr, threadID);
		GetDevice()->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLESTRIP, threadID);

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
	if (!GetOcclusionCullingEnabled() || spTree == nullptr)
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
	GetScene().GetWorldNode()->decals.push_back(decal);
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
	if(debugBoneLines){
		GetDevice()->EventBegin("DebugBoneLines", threadID);

		GetDevice()->BindPrimitiveTopology(LINELIST,threadID);
		GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_LINE],threadID);
	
		GetDevice()->BindRasterizerState(rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH],threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_XRAY],STENCILREF_EMPTY,threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_TRANSPARENT],threadID);


		GetDevice()->BindPS(pixelShaders[PSTYPE_LINE],threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_LINE],threadID);

		MiscCB sb;
		for (unsigned int i = 0; i<boneLines.size(); i++){
			sb.mTransform = XMMatrixTranspose(XMLoadFloat4x4(&boneLines[i]->desc.transform)*camera->GetViewProjection());
			sb.mColor = boneLines[i]->desc.color;

			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

			const GPUBuffer* vbs[] = {
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

	GetDevice()->BindPrimitiveTopology(LINELIST, threadID);
	GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_LINE], threadID);

	GetDevice()->BindRasterizerState(rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH], threadID);
	GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_XRAY], STENCILREF_EMPTY, threadID);
	GetDevice()->BindBlendState(blendStates[BSTYPE_TRANSPARENT], threadID);


	GetDevice()->BindPS(pixelShaders[PSTYPE_LINE], threadID);
	GetDevice()->BindVS(vertexShaders[VSTYPE_LINE], threadID);

	MiscCB sb;
	for (unsigned int i = 0; i<linesTemp.size(); i++){
		sb.mTransform = XMMatrixTranspose(XMLoadFloat4x4(&linesTemp[i]->desc.transform)*camera->GetViewProjection());
		sb.mColor = linesTemp[i]->desc.color;

		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

		const GPUBuffer* vbs[] = {
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
	if(debugPartitionTree || !renderableBoxes.empty()){
		GetDevice()->EventBegin("DebugBoxes", threadID);

		GetDevice()->BindPrimitiveTopology(LINELIST,threadID);
		GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_LINE],threadID);

		GetDevice()->BindRasterizerState(rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH],threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD],STENCILREF_EMPTY,threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_TRANSPARENT],threadID);


		GetDevice()->BindPS(pixelShaders[PSTYPE_LINE],threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_LINE],threadID);

		const GPUBuffer* vbs[] = {
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
	if(!renderableTranslators.empty()){
		GetDevice()->EventBegin("Translators", threadID);


		GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_LINE],threadID);

		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_XRAY],STENCILREF_EMPTY,threadID);


		GetDevice()->BindPS(pixelShaders[PSTYPE_LINE],threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_LINE],threadID);
		
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
				const GPUBuffer* vbs[] = {
					wiTranslator::vertexBuffer_Plane,
				};
				const UINT strides[] = {
					sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
				};
				GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
				GetDevice()->BindRasterizerState(rasterizers[RSTYPE_DOUBLESIDED], threadID);
				GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
				GetDevice()->BindBlendState(blendStates[BSTYPE_ADDITIVE], threadID);
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
				const GPUBuffer* vbs[] = {
					wiTranslator::vertexBuffer_Axis,
				};
				const UINT strides[] = {
					sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
				};
				GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
				GetDevice()->BindRasterizerState(rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH], threadID);
				GetDevice()->BindPrimitiveTopology(LINELIST, threadID);
				GetDevice()->BindBlendState(blendStates[BSTYPE_TRANSPARENT], threadID);
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
				const GPUBuffer* vbs[] = {
					wiTranslator::vertexBuffer_Origin,
				};
				const UINT strides[] = {
					sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
				};
				GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
				sb.mTransform = XMMatrixTranspose(mat);
				sb.mColor = x->state == wiTranslator::TRANSLATOR_XYZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.5f, 0.5f, 0.5f, 1);
				GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
				GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
				GetDevice()->BindRasterizerState(rasterizers[RSTYPE_FRONT], threadID);
				GetDevice()->Draw(wiTranslator::vertexCount_Origin, 0, threadID);
			}
		}

		GetDevice()->EventEnd(threadID);

		renderableTranslators.clear();
	}
}
void wiRenderer::DrawDebugEnvProbes(Camera* camera, GRAPHICSTHREAD threadID)
{
	if (debugEnvProbes && !GetScene().environmentProbes.empty()) {
		GetDevice()->EventBegin("Debug EnvProbes", threadID);

		GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
		GetDevice()->BindRasterizerState(rasterizers[RSTYPE_FRONT], threadID);

		GetDevice()->BindVertexLayout(nullptr, threadID);

		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_DEFAULT, threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);


		GetDevice()->BindPS(pixelShaders[PSTYPE_CUBEMAP], threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_SPHERE], threadID);

		MiscCB sb;
		for (auto& x : GetScene().environmentProbes)
		{
			sb.mTransform = XMMatrixTranspose(x->getMatrix());
			sb.mColor = XMFLOAT4(1, 1, 1, 1);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

			GetDevice()->BindResourcePS(x->cubeMap.GetTexture(), TEXSLOT_ENV0, threadID);

			GetDevice()->Draw(2880, 0, threadID); // uv-sphere
		}

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawDebugGridHelper(Camera* camera, GRAPHICSTHREAD threadID)
{
	if(gridHelper){
		GetDevice()->EventBegin("GridHelper", threadID);









		GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
		GetDevice()->BindVertexLayout(nullptr, threadID);
		GetDevice()->BindRasterizerState(rasterizers[RSTYPE_WIRE_DOUBLESIDED], threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_EMPTY, threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_TRANSPARENT], threadID);

		GetDevice()->BindVS(vertexShaders[VSTYPE_OCEANGRID], threadID);
		GetDevice()->BindGS(geometryShaders[GSTYPE_OCEANGRID], threadID);
		GetDevice()->BindPS(pixelShaders[PSTYPE_OCEANGRID], threadID);

		const uint2 dim = uint2(160, 90);

		MiscCB cb;
		cb.mColor = XMFLOAT4((float)dim.x, (float)dim.y, 1.0f / (float)dim.x, 1.0f / (float)dim.y);
		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &cb, threadID);

		GetDevice()->Draw(dim.x*dim.y*6, 0, threadID);

		GetDevice()->BindGS(nullptr, threadID);










		GetDevice()->BindPrimitiveTopology(LINELIST,threadID);
		GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_LINE],threadID);

		GetDevice()->BindRasterizerState(rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH],threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD],STENCILREF_EMPTY,threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_TRANSPARENT],threadID);


		GetDevice()->BindPS(pixelShaders[PSTYPE_LINE],threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_LINE],threadID);

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

		const GPUBuffer* vbs[] = {
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
	if (voxelHelper && textures[TEXTYPE_3D_VOXELRADIANCE] != nullptr) {
		GetDevice()->EventBegin("Debug Voxels", threadID);

		GetDevice()->BindPrimitiveTopology(POINTLIST, threadID);

		GetDevice()->BindRasterizerState(rasterizers[RSTYPE_BACK], threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_EMPTY, threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);


		GetDevice()->BindPS(pixelShaders[PSTYPE_VOXEL], threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_VOXEL], threadID);
		GetDevice()->BindGS(geometryShaders[GSTYPE_VOXEL], threadID);

		GetDevice()->BindVertexLayout(nullptr, threadID);



		MiscCB sb;
		sb.mTransform = XMMatrixTranspose(XMMatrixTranslationFromVector(XMLoadFloat3(&voxelSceneData.center)) * camera->GetViewProjection());
		sb.mColor = XMFLOAT4(1, 1, 1, 1);

		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

		GetDevice()->Draw(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res, 0, threadID);

		GetDevice()->BindGS(nullptr, threadID);

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawDebugEmitters(Camera* camera, GRAPHICSTHREAD threadID)
{
	if (debugEmitters || !renderableBoxes.empty()) {
		GetDevice()->EventBegin("DebugEmitters", threadID);

		GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
		GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_OBJECT_DEBUG], threadID);

		GetDevice()->BindRasterizerState(rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH], threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_EMPTY, threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);


		GetDevice()->BindPS(pixelShaders[PSTYPE_OBJECT_DEBUG], threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_OBJECT_DEBUG], threadID);

		MiscCB sb;
		for (auto& x : emitterSystems)
		{
			if (x->object != nullptr && x->object->mesh != nullptr)
			{
				sb.mTransform = XMMatrixTranspose(XMLoadFloat4x4(&x->object->world)*camera->GetViewProjection());
				sb.mColor = XMFLOAT4(0, 1, 0, 1);
				GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

				const GPUBuffer* vbs[] = {
					&x->object->mesh->vertexBuffer_POS,
				};
				const UINT strides[] = {
					sizeof(Mesh::Vertex_POS),
				};
				GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
				GetDevice()->BindIndexBuffer(&x->object->mesh->indexBuffer, x->object->mesh->GetIndexFormat(), 0, threadID);

				GetDevice()->DrawIndexed((int)x->object->mesh->indices.size(), 0, 0, threadID);
			}
		}

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawDebugForceFields(Camera* camera, GRAPHICSTHREAD threadID)
{
	if (debugForceFields) {
		GetDevice()->EventBegin("DebugForceFields", threadID);

		GetDevice()->BindVertexLayout(nullptr, threadID);

		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_XRAY], STENCILREF_EMPTY, threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_TRANSPARENT], threadID);


		GetDevice()->BindPS(pixelShaders[PSTYPE_FORCEFIELDVISUALIZER], threadID);

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
					GetDevice()->BindRasterizerState(rasterizers[RSTYPE_BACK], threadID);
					GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
					GetDevice()->BindVS(vertexShaders[VSTYPE_FORCEFIELDVISUALIZER_POINT], threadID);
					GetDevice()->Draw(2880, 0, threadID); // uv-sphere
					break;
				case ENTITY_TYPE_FORCEFIELD_PLANE:
					GetDevice()->BindRasterizerState(rasterizers[RSTYPE_FRONT], threadID);
					GetDevice()->BindPrimitiveTopology(TRIANGLESTRIP, threadID);
					GetDevice()->BindVS(vertexShaders[VSTYPE_FORCEFIELDVISUALIZER_PLANE], threadID);
					GetDevice()->Draw(14, 0, threadID); // box
					break;
				}

				++i;
			}
		}

		GetDevice()->EventEnd(threadID);
	}
}

void wiRenderer::DrawSoftParticles(Camera* camera, GRAPHICSTHREAD threadID)
{
	// todo: remove allocation of vector
	vector<wiEmittedParticle*> sortedEmitters(emitterSystems.begin(), emitterSystems.end());
	std::sort(sortedEmitters.begin(), sortedEmitters.end(), [&](const wiEmittedParticle* a, const wiEmittedParticle* b) {
		return wiMath::DistanceSquared(camera->translation, a->GetPosition()) > wiMath::DistanceSquared(camera->translation, b->GetPosition());
	});

	for (wiEmittedParticle* e : sortedEmitters)
	{
		e->Draw(threadID);
	}
}
void wiRenderer::DrawTrails(GRAPHICSTHREAD threadID, Texture2D* refracRes)
{
	if (objectsWithTrails.empty())
	{
		return;
	}

	GetDevice()->EventBegin("RibbonTrails", threadID);

	GetDevice()->BindPrimitiveTopology(TRIANGLESTRIP,threadID);
	GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_TRAIL],threadID);

	GetDevice()->BindRasterizerState(wireRender?rasterizers[RSTYPE_WIRE_DOUBLESIDED]:rasterizers[RSTYPE_DOUBLESIDED],threadID);
	GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT],STENCILREF_EMPTY,threadID);
	GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE],threadID);

	GetDevice()->BindPS(pixelShaders[PSTYPE_TRAIL],threadID);
	GetDevice()->BindVS(vertexShaders[VSTYPE_TRAIL],threadID);
	
	GetDevice()->BindResourcePS(refracRes,TEXSLOT_ONDEMAND0,threadID);

	for (Object* o : objectsWithTrails)
	{
		if (o->trail.size() >= 4)
		{

			GetDevice()->BindResourcePS(o->trailDistortTex, TEXSLOT_ONDEMAND1, threadID);
			GetDevice()->BindResourcePS(o->trailTex, TEXSLOT_ONDEMAND2, threadID);

			std::vector<RibbonVertex> trails;

			int bounds = (int)o->trail.size();
			trails.reserve(bounds * 10);
			int req = bounds - 3;
			for (int k = 0; k < req; k += 2)
			{
				static const float trailres = 10.f;
				for (float r = 0.0f; r <= 1.0f; r += 1.0f / trailres)
				{
					XMVECTOR point0 = XMVectorCatmullRom(
						XMLoadFloat3(&o->trail[k ? (k - 2) : 0].pos)
						, XMLoadFloat3(&o->trail[k].pos)
						, XMLoadFloat3(&o->trail[k + 2].pos)
						, XMLoadFloat3(&o->trail[k + 6 < bounds ? (k + 6) : (bounds - 2)].pos)
						, r
					),
						point1 = XMVectorCatmullRom(
							XMLoadFloat3(&o->trail[k ? (k - 1) : 1].pos)
							, XMLoadFloat3(&o->trail[k + 1].pos)
							, XMLoadFloat3(&o->trail[k + 3].pos)
							, XMLoadFloat3(&o->trail[k + 5 < bounds ? (k + 5) : (bounds - 1)].pos)
							, r
						);
					XMFLOAT3 xpoint0, xpoint1;
					XMStoreFloat3(&xpoint0, point0);
					XMStoreFloat3(&xpoint1, point1);
					trails.push_back(RibbonVertex(xpoint0
						, wiMath::Lerp(XMFLOAT2((float)k / (float)bounds, 0), XMFLOAT2((float)(k + 1) / (float)bounds, 0), r)
						, wiMath::Lerp(o->trail[k].col, o->trail[k + 2].col, r)
						, 1
					));
					trails.push_back(RibbonVertex(xpoint1
						, wiMath::Lerp(XMFLOAT2((float)k / (float)bounds, 1), XMFLOAT2((float)(k + 1) / (float)bounds, 1), r)
						, wiMath::Lerp(o->trail[k + 1].col, o->trail[k + 3].col, r)
						, 1
					));
				}
			}
			if (!trails.empty())
			{
				UINT trailOffset = GetDevice()->AppendRingBuffer(dynamicVertexBufferPool, trails.data(), sizeof(RibbonVertex)*trails.size(), threadID);

				const GPUBuffer* vbs[] = {
					dynamicVertexBufferPool
				};
				const UINT strides[] = {
					sizeof(RibbonVertex)
				};
				const UINT offsets[] = {
					trailOffset
				};
				GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
				GetDevice()->Draw((int)trails.size(), 0, threadID);

				trails.clear();
			}
		}
	}

	GetDevice()->EventEnd(threadID);
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

	GetDevice()->BindPrimitiveTopology(TRIANGLELIST,threadID);

	
	GetDevice()->BindRasterizerState(rasterizers[RSTYPE_BACK],threadID);

	GetDevice()->BindVertexLayout(nullptr, threadID);

	// Environmental light (envmap + voxelGI) is always drawn
	{
		GetDevice()->BindBlendState(blendStates[BSTYPE_ENVIRONMENTALLIGHT], threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DIRLIGHT], STENCILREF_DEFAULT, threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_DIRLIGHT], threadID); // just full screen triangle so we can use it
		GetDevice()->BindPS(pixelShaders[PSTYPE_ENVIRONMENTALLIGHT], threadID);
		GetDevice()->Draw(3, 0, threadID); // full screen triangle
	}

	GetDevice()->BindBlendState(blendStates[BSTYPE_DEFERREDLIGHT], threadID);

	for (int type = 0; type < Light::LIGHTTYPE_COUNT; ++type)
	{

		switch (type)
		{
		case Light::DIRECTIONAL:
			GetDevice()->BindVS(vertexShaders[VSTYPE_DIRLIGHT], threadID);
			if (SOFTSHADOWQUALITY_2D)
			{
				GetDevice()->BindPS(pixelShaders[PSTYPE_DIRLIGHT_SOFT], threadID);
			}
			else
			{
				GetDevice()->BindPS(pixelShaders[PSTYPE_DIRLIGHT], threadID);
			}
			GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DIRLIGHT], STENCILREF_DEFAULT, threadID);
			break;
		case Light::POINT:
			GetDevice()->BindVS(vertexShaders[VSTYPE_POINTLIGHT], threadID);
			GetDevice()->BindPS(pixelShaders[PSTYPE_POINTLIGHT], threadID);
			GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_LIGHT], STENCILREF_DEFAULT, threadID);
			break;
		case Light::SPOT:
			GetDevice()->BindVS(vertexShaders[VSTYPE_SPOTLIGHT], threadID);
			GetDevice()->BindPS(pixelShaders[PSTYPE_SPOTLIGHT], threadID);
			GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_LIGHT], STENCILREF_DEFAULT, threadID);
			break;
		case Light::SPHERE:
			GetDevice()->BindVS(vertexShaders[VSTYPE_DIRLIGHT], threadID);
			GetDevice()->BindPS(pixelShaders[PSTYPE_SPHERELIGHT], threadID);
			GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DIRLIGHT], STENCILREF_DEFAULT, threadID);
			break;
		case Light::DISC:
			GetDevice()->BindVS(vertexShaders[VSTYPE_DIRLIGHT], threadID);
			GetDevice()->BindPS(pixelShaders[PSTYPE_DISCLIGHT], threadID);
			GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DIRLIGHT], STENCILREF_DEFAULT, threadID);
			break;
		case Light::RECTANGLE:
			GetDevice()->BindVS(vertexShaders[VSTYPE_DIRLIGHT], threadID);
			GetDevice()->BindPS(pixelShaders[PSTYPE_RECTANGLELIGHT], threadID);
			GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DIRLIGHT], STENCILREF_DEFAULT, threadID);
			break;
		case Light::TUBE:
			GetDevice()->BindVS(vertexShaders[VSTYPE_DIRLIGHT], threadID);
			GetDevice()->BindPS(pixelShaders[PSTYPE_TUBELIGHT], threadID);
			GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DIRLIGHT], STENCILREF_DEFAULT, threadID);
			break;
		}


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
void wiRenderer::DrawVolumeLights(Camera* camera, GRAPHICSTHREAD threadID)
{
	const FrameCulling& culling = frameCullings[camera];
	const CulledList& culledLights = culling.culledLights;

	if(!culledLights.empty())
	{
		GetDevice()->EventBegin("Light Volume Render", threadID);

		GetDevice()->BindPrimitiveTopology(TRIANGLELIST,threadID);
		GetDevice()->BindVertexLayout(nullptr, threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD],STENCILREF_DEFAULT,threadID);

		
		GetDevice()->BindPS(pixelShaders[PSTYPE_VOLUMELIGHT],threadID);

		GetDevice()->BindConstantBufferPS(constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), threadID);
		GetDevice()->BindConstantBufferVS(constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), threadID);


		for (int type = Light::POINT; type < Light::LIGHTTYPE_COUNT; ++type)
		{
			switch (type)
			{
			case Light::POINT:
				GetDevice()->BindBlendState(blendStates[BSTYPE_ADDITIVE], threadID);
				GetDevice()->BindVS(vertexShaders[VSTYPE_VOLUMEPOINTLIGHT], threadID);
				GetDevice()->BindRasterizerState(rasterizers[RSTYPE_FRONT], threadID);
				break;
			case Light::SPOT:
				GetDevice()->BindBlendState(blendStates[BSTYPE_ADDITIVE], threadID);
				GetDevice()->BindVS(vertexShaders[VSTYPE_VOLUMESPOTLIGHT], threadID);
				GetDevice()->BindRasterizerState(rasterizers[RSTYPE_DOUBLESIDED], threadID);
				break;
			case Light::SPHERE:
				GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);
				GetDevice()->BindVS(vertexShaders[VSTYPE_VOLUMESPHERELIGHT], threadID);
				GetDevice()->BindRasterizerState(rasterizers[RSTYPE_FRONT], threadID);
				break;
			case Light::DISC:
				GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);
				GetDevice()->BindVS(vertexShaders[VSTYPE_VOLUMEDISCLIGHT], threadID);
				GetDevice()->BindRasterizerState(rasterizers[RSTYPE_FRONT], threadID);
				break;
			case Light::RECTANGLE:
				GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);
				GetDevice()->BindVS(vertexShaders[VSTYPE_VOLUMERECTANGLELIGHT], threadID);
				GetDevice()->BindRasterizerState(rasterizers[RSTYPE_BACK], threadID);
				break;
			case Light::TUBE:
				GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);
				GetDevice()->BindVS(vertexShaders[VSTYPE_VOLUMETUBELIGHT], threadID);
				GetDevice()->BindRasterizerState(rasterizers[RSTYPE_FRONT], threadID);
				break;
			}

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
	Light::shadowMapArray_2D->RequestIndepententRenderTargetArraySlices(true);

	Texture2DDesc desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = SHADOWRES_2D;
	desc.Height = SHADOWRES_2D;
	desc.MipLevels = 1;
	desc.ArraySize = SHADOWCOUNT_2D;
	desc.Format = FORMAT_R16_TYPELESS;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = USAGE_DEFAULT;
	desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	GetDevice()->CreateTexture2D(&desc, nullptr, &Light::shadowMapArray_2D);
}
void wiRenderer::SetShadowPropsCube(int resolution, int count)
{
	SHADOWRES_CUBE = resolution;
	SHADOWCOUNT_CUBE = count;

	SAFE_DELETE(Light::shadowMapArray_Cube);
	Light::shadowMapArray_Cube = new Texture2D;
	Light::shadowMapArray_Cube->RequestIndepententRenderTargetArraySlices(true);
	Light::shadowMapArray_Cube->RequestIndepententRenderTargetCubemapFaces(false);

	Texture2DDesc desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = SHADOWRES_CUBE;
	desc.Height = SHADOWRES_CUBE;
	desc.MipLevels = 1;
	desc.ArraySize = 6 * SHADOWCOUNT_CUBE;
	desc.Format = FORMAT_R16_TYPELESS;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = USAGE_DEFAULT;
	desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;
	GetDevice()->CreateTexture2D(&desc, nullptr, &Light::shadowMapArray_Cube);
}
void wiRenderer::DrawForShadowMap(GRAPHICSTHREAD threadID)
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

		const FrameCulling& culling = frameCullings[getCamera()];
		const CulledList& culledLights = culling.culledLights;

		ViewPort vp;

		if (!culledLights.empty())
		{
			GetDevice()->UnBindResources(TEXSLOT_SHADOWARRAY_2D, 2, threadID);

			GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);

			GetDevice()->BindBlendState(blendStates[BSTYPE_COLORWRITEDISABLE], threadID);

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

					GetDevice()->BindConstantBufferGS(constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubeMapRenderCB), threadID);
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

						for (int index = 0; index < 3; ++index)
						{
							const float siz = l->shadowCam_dirLight[index].size * 0.5f;
							const float f = l->shadowCam_dirLight[index].farplane;
							AABB boundingbox;
							boundingbox.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(siz, f, siz));
							if (spTree != nullptr)
							{
								CulledList culledObjects;
								CulledCollection culledRenderer;
								spTree->getVisible(boundingbox.get(XMMatrixInverse(0, XMLoadFloat4x4(&l->shadowCam_dirLight[index].View))), culledObjects);
								for (Cullable* x : culledObjects)
								{
									Object* object = (Object*)x;
									if (object->IsCastingShadow())
									{
										culledRenderer[object->mesh].push_front(object);
									}
								}
								if (!culledRenderer.empty())
								{
									GetDevice()->BindRenderTargets(0, nullptr, Light::shadowMapArray_2D, threadID, l->shadowMap_index + index);
									GetDevice()->ClearDepthStencil(Light::shadowMapArray_2D, CLEAR_DEPTH, 0.0f, 0, threadID, l->shadowMap_index + index);

									CameraCB cb;
									cb.mVP = l->shadowCam_dirLight[index].getVP();
									GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_CAMERA], &cb, threadID);

									RenderMeshes(l->shadowCam_dirLight[index].Eye, culledRenderer, SHADERTYPE_SHADOW, RENDERTYPE_OPAQUE, threadID);
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
							for (Cullable* x : culledObjects)
							{
								Object* object = (Object*)x;
								if (object->IsCastingShadow())
								{
									culledRenderer[object->mesh].push_front(object);
								}
							}
							if (!culledRenderer.empty())
							{
								GetDevice()->BindRenderTargets(0, nullptr, Light::shadowMapArray_2D, threadID, l->shadowMap_index);
								GetDevice()->ClearDepthStencil(Light::shadowMapArray_2D, CLEAR_DEPTH, 0.0f, 0, threadID, l->shadowMap_index);

								CameraCB cb;
								cb.mVP = l->shadowCam_spotLight[0].getVP();
								GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_CAMERA], &cb, threadID);

								RenderMeshes(l->translation, culledRenderer, SHADERTYPE_SHADOW, RENDERTYPE_OPAQUE, threadID);
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
								if (object->IsCastingShadow())
								{
									culledRenderer[object->mesh].push_front(object);
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

			GetDevice()->BindGS(nullptr, threadID);
			GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);
		}


		wiProfiler::GetInstance().EndRange(); // Shadow Rendering
		GetDevice()->EventEnd(threadID);
	}

	GetDevice()->BindResourcePS(Light::shadowMapArray_2D, TEXSLOT_SHADOWARRAY_2D, threadID);
	GetDevice()->BindResourcePS(Light::shadowMapArray_Cube, TEXSLOT_SHADOWARRAY_CUBE, threadID);
}

VLTYPES GetVLTYPE(SHADERTYPE shaderType, const Material* const material, bool tessellatorRequested, bool ditheringAlphaTest)
{
	VLTYPES realVL = VLTYPE_OBJECT_POS_TEX;

	bool alphatest = material->IsAlphaTestEnabled() || ditheringAlphaTest;

	switch (shaderType)
	{
	case SHADERTYPE_TEXTURE:
		if (tessellatorRequested)
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
	case SHADERTYPE_VOXELIZE:
	case SHADERTYPE_ENVMAPCAPTURE:
		realVL = VLTYPE_OBJECT_ALL;
		break;
	case SHADERTYPE_DEPTHONLY:
		if (tessellatorRequested)
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
		if (alphatest)
		{
			realVL = VLTYPE_OBJECT_POS_TEX;
		}
		else
		{
			realVL = VLTYPE_OBJECT_POS;
		}
		break;
	default:
		assert(0);
		break;
	}

	return realVL;
}
VSTYPES GetVSTYPE(SHADERTYPE shaderType, const Material* const material, bool tessellatorRequested, bool ditheringAlphaTest)
{
	VSTYPES realVS = VSTYPE_OBJECT_SIMPLE;

	bool water = material->IsWater();
	bool alphatest = material->IsAlphaTestEnabled() || ditheringAlphaTest;

	switch (shaderType)
	{
	case SHADERTYPE_TEXTURE:
		if (tessellatorRequested)
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
		if (tessellatorRequested)
		{
			realVS = VSTYPE_OBJECT_COMMON_TESSELLATION;
		}
		else
		{
			if (water)
			{
				realVS = VSTYPE_WATER;
			}
			else
			{
				realVS = VSTYPE_OBJECT_COMMON;
			}
		}
		break;
	case SHADERTYPE_DEPTHONLY:
		if (tessellatorRequested)
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
		if (alphatest)
		{
			realVS = VSTYPE_SHADOW_ALPHATEST;
		}
		else
		{
			realVS = VSTYPE_SHADOW;
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
	default:
		assert(0);
		break;
	}

	return realVS;
}
GSTYPES GetGSTYPE(SHADERTYPE shaderType, const Material* const material)
{
	GSTYPES realGS = GSTYPE_NULL;

	bool alphatest = material->IsAlphaTestEnabled(); // do not mind the dithering alpha test!

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
	default:
		assert(0);
		break;
	}

	return realGS;
}
HSTYPES GetHSTYPE(SHADERTYPE shaderType, const Material* const material, bool tessellatorRequested)
{
	return tessellatorRequested ? HSTYPE_OBJECT : HSTYPE_NULL;
}
DSTYPES GetDSTYPE(SHADERTYPE shaderType, const Material* const material, bool tessellatorRequested)
{
	return tessellatorRequested ? DSTYPE_OBJECT : DSTYPE_NULL;
}
PSTYPES GetPSTYPE(SHADERTYPE shaderType, const Material* const material, bool ditheringAlphaTest)
{
	PSTYPES realPS = PSTYPE_OBJECT_SIMPLEST;

	bool transparent = material->IsTransparent();
	bool water = material->IsWater();
	bool alphatest = material->IsAlphaTestEnabled() || ditheringAlphaTest;

	switch (shaderType)
	{
	case SHADERTYPE_DEFERRED:
		if (material->GetNormalMap() == nullptr)
		{
			if (material->parallaxOcclusionMapping > 0)
			{
				realPS = PSTYPE_OBJECT_DEFERRED_POM;
			}
			else
			{
				realPS = PSTYPE_OBJECT_DEFERRED;
			}
		}
		else
		{
			if (material->parallaxOcclusionMapping > 0)
			{
				realPS = PSTYPE_OBJECT_DEFERRED_NORMALMAP_POM;
			}
			else
			{
				realPS = PSTYPE_OBJECT_DEFERRED_NORMALMAP;
			}
		}
		break;
	case SHADERTYPE_FORWARD:
		if (water)
		{
			realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_WATER;
		}
		else if (transparent)
		{
			if (material->GetNormalMap() == nullptr)
			{
				if (material->parallaxOcclusionMapping > 0)
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT;
				}
				if (material->HasPlanarReflection())
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT_PLANARREFLECTION;
				}
			}
			else
			{
				if (material->parallaxOcclusionMapping > 0)
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT_NORMALMAP_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT_NORMALMAP;
				}
				if (material->HasPlanarReflection())
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_TRANSPARENT_NORMALMAP_PLANARREFLECTION;
				}
			}
		}
		else
		{
			if (material->GetNormalMap() == nullptr)
			{
				if (material->parallaxOcclusionMapping > 0)
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT;
				}
				if (material->HasPlanarReflection())
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_PLANARREFLECTION;
				}
			}
			else
			{
				if (material->parallaxOcclusionMapping > 0)
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_NORMALMAP_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_NORMALMAP;
				}
				if (material->HasPlanarReflection())
				{
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_NORMALMAP_PLANARREFLECTION;
				}
			}
		}
		break;
	case SHADERTYPE_TILEDFORWARD:
		if (water)
		{
			realPS = PSTYPE_OBJECT_TILEDFORWARD_WATER;
		}
		else if (transparent)
		{
			if (material->GetNormalMap() == nullptr)
			{
				if (material->parallaxOcclusionMapping > 0)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT;
				}
				if (material->HasPlanarReflection())
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_PLANARREFLECTION;
				}
			}
			else
			{
				if (material->parallaxOcclusionMapping > 0)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP;
				}
				if (material->HasPlanarReflection())
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_TRANSPARENT_NORMALMAP_PLANARREFLECTION;
				}
			}
		}
		else
		{
			if (material->GetNormalMap() == nullptr)
			{
				if (material->parallaxOcclusionMapping > 0)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD;
				}
				if (material->HasPlanarReflection())
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_PLANARREFLECTION;
				}
			}
			else
			{
				if (material->parallaxOcclusionMapping > 0)
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP_POM;
				}
				else
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP;
				}
				if (material->HasPlanarReflection())
				{
					realPS = PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP_PLANARREFLECTION;
				}
			}
		}
		break;
	case SHADERTYPE_SHADOW:
		if (alphatest)
		{
			realPS = PSTYPE_SHADOW_ALPHATEST;
		}
		else
		{
			realPS = PSTYPE_NULL;
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
	default:
		assert(0);
		break;
	}

	return realPS;
}
void wiRenderer::RenderMeshes(const XMFLOAT3& eye, const CulledCollection& culledRenderer, SHADERTYPE shaderType, UINT renderTypeFlags, GRAPHICSTHREAD threadID,
	bool tessellation, bool occlusionCulling)
{
	// Intensive section, refactor and optimize!

	if (!culledRenderer.empty())
	{
		GraphicsDevice* device = GetDevice();

		device->EventBegin("RenderMeshes", threadID);

		tessellation = tessellation && device->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION);

		DSSTYPES targetDepthStencilState = (shaderType == SHADERTYPE_TILEDFORWARD && renderTypeFlags & RENDERTYPE_OPAQUE) ? DSSTYPE_DEPTHREADEQUAL : DSSTYPE_DEFAULT;
		targetDepthStencilState = (shaderType == SHADERTYPE_SHADOWCUBE || shaderType == SHADERTYPE_SHADOW) ? DSSTYPE_SHADOW : targetDepthStencilState;
		targetDepthStencilState = shaderType == SHADERTYPE_ENVMAPCAPTURE ? DSSTYPE_ENVMAP : targetDepthStencilState;

		UINT prevStencilRef = STENCILREF_DEFAULT;
		device->BindDepthStencilState(depthStencils[targetDepthStencilState], prevStencilRef, threadID);

		const XMFLOAT4X4 __identityMat = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
		XMFLOAT4X4 tempMat;

		Instance instances[1024];
		InstancePrev instancesPrev[1024];

		if (shaderType == SHADERTYPE_DEPTHONLY || shaderType == SHADERTYPE_SHADOW || shaderType == SHADERTYPE_SHADOWCUBE)
		{
			device->BindBlendState(blendStates[BSTYPE_COLORWRITEDISABLE], threadID);
		}
		else if(renderTypeFlags & RENDERTYPE_OPAQUE)
		{
			device->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);
		}
		else
		{
			device->BindBlendState(blendStates[BSTYPE_TRANSPARENT], threadID);
		}

		const bool easyTextureBind = 
			shaderType == SHADERTYPE_TEXTURE || 
			shaderType == SHADERTYPE_SHADOW || 
			shaderType == SHADERTYPE_SHADOWCUBE || 
			shaderType == SHADERTYPE_DEPTHONLY || 
			shaderType == SHADERTYPE_VOXELIZE;

		const bool impostorRequest = 
			shaderType != SHADERTYPE_VOXELIZE && 
			shaderType != SHADERTYPE_SHADOW && 
			shaderType != SHADERTYPE_SHADOWCUBE && 
			shaderType != SHADERTYPE_ENVMAPCAPTURE;

		// Render impostors:
		if (impostorRequest)
		{
			bool impostorRenderSetup = false;

			// Assess the render params:
			VLTYPES realVL = (shaderType == SHADERTYPE_DEPTHONLY || shaderType == SHADERTYPE_TEXTURE) ? VLTYPE_OBJECT_POS_TEX : VLTYPE_OBJECT_ALL;
			VSTYPES realVS = realVL == VLTYPE_OBJECT_POS_TEX ? VSTYPE_OBJECT_SIMPLE : VSTYPE_OBJECT_COMMON;
			PSTYPES realPS = PSTYPE_OBJECT_SIMPLEST;
			if (!wireRender)
			{
				switch (shaderType)
				{
				case SHADERTYPE_DEFERRED:
					realPS = PSTYPE_OBJECT_DEFERRED_NORMALMAP;
					break;
				case SHADERTYPE_FORWARD:
					realPS = PSTYPE_OBJECT_FORWARD_DIRLIGHT_NORMALMAP;
					break;
				case SHADERTYPE_TILEDFORWARD:
					realPS = PSTYPE_OBJECT_TILEDFORWARD_NORMALMAP;
					break;
				case SHADERTYPE_DEPTHONLY:
					realPS = PSTYPE_OBJECT_ALPHATESTONLY;
					break;
				default:
					realPS = PSTYPE_OBJECT_TEXTUREONLY;
					break;
				}
			}

			for (CulledCollection::const_iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter)
			{
				Mesh* mesh = iter->first;
				if (!mesh->renderable || !mesh->hasImpostor())
				{
					continue;
				}

				if (!impostorRenderSetup)
				{
					// Bind the static impostor params once:
					impostorRenderSetup = true;
					device->BindConstantBufferPS(Material::constantBuffer_Impostor, CB_GETBINDSLOT(MaterialCB), threadID);
					device->BindPrimitiveTopology(TRIANGLELIST, threadID);
					device->BindRasterizerState(wireRender ? rasterizers[RSTYPE_WIRE] : rasterizers[RSTYPE_FRONT], threadID);
					device->BindVertexLayout(vertexLayouts[realVL], threadID);
					device->BindVS(vertexShaders[realVS], threadID);
					device->BindPS(pixelShaders[realPS], threadID);
					SetAlphaRef(0.75f, threadID);
				}

				const CulledObjectList& visibleInstances = iter->second;

				bool instancePrevUpdated = false;

				int k = 0;
				for (const Object* instance : visibleInstances)
				{
					if (occlusionCulling && instance->IsOccluded())
						continue;

					const float impostorThreshold = instance->bounds.getRadius();
					float dist = wiMath::Distance(eye, instance->bounds.getCenter());
					float dither = instance->transparency;
					dither = wiMath::SmoothStep(1.0f, dither, wiMath::Clamp((dist - mesh->impostorDistance) / impostorThreshold, 0, 1));
					if (dither > 1.0f - FLT_EPSILON)
						continue;

					assert(k < ARRAYSIZE(instances) && "Instance buffer full!");

					XMMATRIX boxMat = mesh->aabb.getAsBoxMatrix();

					XMStoreFloat4x4(&tempMat, boxMat*XMLoadFloat4x4(&instance->world));
					instances[k].Create(tempMat, dither, instance->color);

					if (shaderType == SHADERTYPE_FORWARD || shaderType == SHADERTYPE_TILEDFORWARD || shaderType == SHADERTYPE_DEFERRED)
					{
						XMStoreFloat4x4(&tempMat, boxMat*XMLoadFloat4x4(&instance->worldPrev));
						instancesPrev[k].Create(tempMat);
						instancePrevUpdated = true;
					}

					++k;
				}
				if (k < 1)
					continue;

				UINT instanceOffset = GetDevice()->AppendRingBuffer(dynamicVertexBufferPool, instances, sizeof(Instance)*k, threadID);
				UINT instancePrevOffset = 0;
				if (instancePrevUpdated)
				{
					instancePrevOffset = GetDevice()->AppendRingBuffer(dynamicVertexBufferPool, instances, sizeof(InstancePrev)*k, threadID);
				}

				if (realVL == VLTYPE_OBJECT_POS_TEX)
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
						instanceOffset
					};
					device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
				}
				else
				{
					GPUBuffer* vbs[] = {
						&Mesh::impostorVB_POS,
						&Mesh::impostorVB_NOR,
						&Mesh::impostorVB_TEX,
						&Mesh::impostorVB_POS,
						dynamicVertexBufferPool,
						dynamicVertexBufferPool
					};
					UINT strides[] = {
						sizeof(Mesh::Vertex_POS),
						sizeof(Mesh::Vertex_NOR),
						sizeof(Mesh::Vertex_TEX),
						sizeof(Mesh::Vertex_POS),
						sizeof(Instance),
						sizeof(InstancePrev),
					};
					UINT offsets[] = {
						0,
						0,
						0,
						0,
						instanceOffset,
						instancePrevOffset
					};
					device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
				}

				const GPUResource* res[] = {
					static_cast<const GPUResource*>(mesh->impostorTarget.GetTexture(0)),
					static_cast<const GPUResource*>(mesh->impostorTarget.GetTexture(1)),
					static_cast<const GPUResource*>(mesh->impostorTarget.GetTexture(2)),
					static_cast<const GPUResource*>(mesh->impostorTarget.GetTexture(3)),
					static_cast<const GPUResource*>(mesh->impostorTarget.GetTexture(4)),
				};
				device->BindResourcesPS(res, TEXSLOT_ONDEMAND0, (easyTextureBind ? 1 : ARRAYSIZE(res)), threadID);

				device->DrawInstanced(6 * 6, k, 0, 0, threadID); // 6 * 6: see Mesh::CreateImpostorVB function

			}
		}

		VLTYPES prevVL = VLTYPE_NULL;
		VSTYPES prevVS = VSTYPE_NULL;
		GSTYPES prevGS = GSTYPE_NULL;
		HSTYPES prevHS = HSTYPE_NULL;
		DSTYPES prevDS = DSTYPE_NULL;
		PSTYPES prevPS = PSTYPE_NULL;
		PRIMITIVETOPOLOGY prevTOPOLOGY = TRIANGLELIST;
		RSTYPES prevRS = RSTYPE_FRONT;
		device->BindVS(nullptr, threadID);
		device->BindGS(nullptr, threadID);
		device->BindHS(nullptr, threadID);
		device->BindDS(nullptr, threadID);
		device->BindPS(nullptr, threadID);
		device->BindPrimitiveTopology(prevTOPOLOGY, threadID);
		device->BindRasterizerState(rasterizers[prevRS], threadID);

		// Render meshes:
		for (CulledCollection::const_iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) 
		{
			Mesh* mesh = iter->first;
			if (!mesh->renderable)
			{
				continue;
			}

			const CulledObjectList& visibleInstances = iter->second;

			const float tessF = mesh->getTessellationFactor();
			const bool tessellatorRequested = tessF > 0 && tessellation;

			PRIMITIVETOPOLOGY realTOPOLOGY = TRIANGLELIST;
			RSTYPES realRS = RSTYPE_FRONT;

			if (tessellatorRequested)
			{
				TessellationCB tessCB;
				tessCB.tessellationFactors = XMFLOAT4(tessF, tessF, tessF, tessF);
				device->UpdateBuffer(constantBuffers[CBTYPE_TESSELLATION], &tessCB, threadID);
				device->BindConstantBufferHS(constantBuffers[CBTYPE_TESSELLATION], CBSLOT_RENDERER_TESSELLATION, threadID);
				realTOPOLOGY = PATCHLIST;
			}

			if (shaderType == SHADERTYPE_VOXELIZE)
			{
				realRS = RSTYPE_VOXELIZE;
			}
			else
			{
				if (shaderType == SHADERTYPE_SHADOW || shaderType == SHADERTYPE_SHADOWCUBE)
				{
					realRS = mesh->doubleSided ? RSTYPE_SHADOW_DOUBLESIDED : RSTYPE_SHADOW;
				}
				else
				{
					if (!mesh->doubleSided)
					{
						realRS = wireRender ? RSTYPE_WIRE : RSTYPE_FRONT;
					}
					else
					{
						realRS = wireRender ? RSTYPE_WIRE : RSTYPE_DOUBLESIDED;
					}
				}
			}

			if (prevTOPOLOGY != realTOPOLOGY)
			{
				prevTOPOLOGY = realTOPOLOGY;
				device->BindPrimitiveTopology(realTOPOLOGY, threadID);
			}
			if (prevRS != realRS)
			{
				prevRS = realRS;
				device->BindRasterizerState(rasterizers[realRS], threadID);
			}


			bool instancePrevUpdated = false;

			bool forceAlphaTestForDithering = false;

			int k = 0;
			for (const Object* instance : visibleInstances) 
			{
				if (occlusionCulling && instance->IsOccluded())
					continue;

				float dither = instance->transparency;
				if (impostorRequest)
				{
					// fade out to impostor...
					const float impostorThreshold = instance->bounds.getRadius();
					float dist = wiMath::Distance(eye, instance->bounds.getCenter());
					if (mesh->hasImpostor())
						dither = wiMath::SmoothStep(dither, 1.0f, wiMath::Clamp((dist - impostorThreshold - mesh->impostorDistance) / impostorThreshold, 0, 1));
				}
				if (dither > 1.0f - FLT_EPSILON)
					continue;

				assert(k < ARRAYSIZE(instances) && "Instance buffer full!");

				forceAlphaTestForDithering = forceAlphaTestForDithering || (dither > 0);

				if (mesh->softBody)
					tempMat = __identityMat;
				else
					tempMat = instance->world;
				instances[k].Create(tempMat, dither, instance->color);

				if (shaderType == SHADERTYPE_FORWARD || shaderType == SHADERTYPE_TILEDFORWARD || shaderType == SHADERTYPE_DEFERRED)
				{
					if (mesh->softBody)
						tempMat = __identityMat;
					else
						tempMat = instance->worldPrev;
					instancesPrev[k].Create(tempMat);
					instancePrevUpdated = true;
				}
						
				++k;
			}
			if (k < 1)
				continue;

			UINT instanceOffset = GetDevice()->AppendRingBuffer(dynamicVertexBufferPool, instances, sizeof(Instance)*k, threadID);
			UINT instancePrevOffset = 0;
			if (instancePrevUpdated)
			{
				instancePrevOffset = GetDevice()->AppendRingBuffer(dynamicVertexBufferPool, instances, sizeof(InstancePrev)*k, threadID);
			}


			device->BindIndexBuffer(&mesh->indexBuffer, mesh->GetIndexFormat(), 0, threadID);

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

				if (subsetRenderable)
				{

					BOUNDVERTEXBUFFERTYPE boundVBType;
					if (!tessellatorRequested && (shaderType == SHADERTYPE_DEPTHONLY || shaderType == SHADERTYPE_TEXTURE || shaderType == SHADERTYPE_SHADOW || shaderType == SHADERTYPE_SHADOWCUBE))
					{
						// simple vertex buffers are used in some passes (note: tessellator requires more attributes)
						if ((shaderType == SHADERTYPE_DEPTHONLY || shaderType == SHADERTYPE_SHADOW || shaderType == SHADERTYPE_SHADOWCUBE) && !material->IsAlphaTestEnabled() && !forceAlphaTestForDithering)
						{
							// bypass texcoord stream for non alphatested shadows and zprepass
							boundVBType = BOUNDVERTEXBUFFERTYPE::POSITION;
						}
						else
						{
							boundVBType = BOUNDVERTEXBUFFERTYPE::POSITION_TEXCOORD;
						}
					}
					else
					{
						boundVBType = BOUNDVERTEXBUFFERTYPE::EVERYTHING;
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
								mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_POS.IsValid() ? &mesh->streamoutBuffer_POS : &mesh->vertexBuffer_POS),
								dynamicVertexBufferPool
							};
							UINT strides[] = {
								sizeof(Mesh::Vertex_POS),
								sizeof(Instance)
							};
							UINT offsets[] = {
								mesh->hasDynamicVB() ? mesh->bufferOffset_POS : 0,
								instanceOffset
							};
							device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
						}
						break;
						case BOUNDVERTEXBUFFERTYPE::POSITION_TEXCOORD:
						{
							GPUBuffer* vbs[] = {
								mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_POS.IsValid() ? &mesh->streamoutBuffer_POS : &mesh->vertexBuffer_POS),
								&mesh->vertexBuffer_TEX,
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
								instanceOffset
							};
							device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);
						}
						break;
						case BOUNDVERTEXBUFFERTYPE::EVERYTHING:
						{
							GPUBuffer* vbs[] = {
								mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_POS.IsValid() ? &mesh->streamoutBuffer_POS : &mesh->vertexBuffer_POS),
								mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_NOR.IsValid() ? &mesh->streamoutBuffer_NOR : &mesh->vertexBuffer_NOR),
								&mesh->vertexBuffer_TEX,
								mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_PRE.IsValid() ? &mesh->streamoutBuffer_PRE : &mesh->vertexBuffer_POS),
								dynamicVertexBufferPool,
								dynamicVertexBufferPool
							};
							UINT strides[] = {
								sizeof(Mesh::Vertex_POS),
								sizeof(Mesh::Vertex_NOR),
								sizeof(Mesh::Vertex_TEX),
								sizeof(Mesh::Vertex_POS),
								sizeof(Instance),
								sizeof(InstancePrev),
							};
							UINT offsets[] = {
								mesh->hasDynamicVB() ? mesh->bufferOffset_POS : 0,
								mesh->hasDynamicVB() ? mesh->bufferOffset_NOR : 0,
								0,
								mesh->hasDynamicVB() ? mesh->bufferOffset_PRE : 0,
								instanceOffset,
								instancePrevOffset
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

					device->BindConstantBufferPS(&material->constantBuffer, CB_GETBINDSLOT(MaterialCB), threadID);

					UINT realStencilRef = material->GetStencilRef();
					if (prevStencilRef != realStencilRef)
					{
						prevStencilRef = realStencilRef;
						device->BindDepthStencilState(depthStencils[targetDepthStencilState], realStencilRef, threadID);
					}

					VLTYPES realVL = GetVLTYPE(shaderType, material, tessellatorRequested, forceAlphaTestForDithering);
					if (prevVL != realVL)
					{
						prevVL = realVL;
						device->BindVertexLayout(vertexLayouts[realVL], threadID);
					}

					VSTYPES realVS = GetVSTYPE(shaderType, material, tessellatorRequested, forceAlphaTestForDithering);
					if (prevVS != realVS)
					{
						prevVS = realVS;
						device->BindVS(vertexShaders[realVS], threadID);
					}

					GSTYPES realGS = GetGSTYPE(shaderType, material);
					if (prevGS != realGS)
					{
						prevGS = realGS;
						device->BindGS(geometryShaders[realGS], threadID);
					}

					HSTYPES realHS = GetHSTYPE(shaderType, material, tessellatorRequested);
					if (prevHS = realHS)
					{
						prevHS = realHS;
						device->BindHS(hullShaders[realHS], threadID);
					}

					DSTYPES realDS = GetDSTYPE(shaderType, material, tessellatorRequested);
					if (prevDS != realDS)
					{
						prevDS = realDS;
						device->BindDS(domainShaders[realDS], threadID);
					}
					if(realDS!=DSTYPE_NULL)
					{
						device->BindResourceDS(material->GetDisplacementMap(), TEXSLOT_ONDEMAND5, threadID);
					}

					if (wireRender)
					{
						prevPS = PSTYPE_OBJECT_SIMPLEST;
						device->BindPS(pixelShaders[prevPS], threadID);
					}
					else
					{
						PSTYPES realPS = GetPSTYPE(shaderType, material, forceAlphaTestForDithering);
						if (prevPS != realPS)
						{
							prevPS = realPS;
							device->BindPS(pixelShaders[realPS], threadID);
						}
						if (realPS != PSTYPE_NULL)
						{
							const GPUResource* res[] = {
								static_cast<const GPUResource*>(material->GetBaseColorMap()),
								static_cast<const GPUResource*>(material->GetNormalMap()),
								static_cast<const GPUResource*>(material->GetRoughnessMap()),
								static_cast<const GPUResource*>(material->GetReflectanceMap()),
								static_cast<const GPUResource*>(material->GetMetalnessMap()),
								static_cast<const GPUResource*>(material->GetDisplacementMap()),
							};
							device->BindResourcesPS(res, TEXSLOT_ONDEMAND0, (easyTextureBind ? 1 : ARRAYSIZE(res)), threadID);
						}
					}

					SetAlphaRef(material->alphaRef, threadID);

					device->DrawIndexedInstanced((int)subset.subsetIndices.size(), k, subset.indexBufferOffset, 0, 0, threadID);
				}
			}

		}


		device->BindVS(nullptr, threadID);
		device->BindGS(nullptr, threadID);
		device->BindHS(nullptr, threadID);
		device->BindDS(nullptr, threadID);
		device->BindPS(nullptr, threadID);

		ResetAlphaRef(threadID);

		device->EventEnd(threadID);
	}
}

void wiRenderer::DrawWorld(Camera* camera, bool tessellation, GRAPHICSTHREAD threadID, SHADERTYPE shaderType, Texture2D* refRes, bool grass, bool occlusionCulling)
{

	const FrameCulling& culling = frameCullings[camera];
	const CulledCollection& culledRenderer = culling.culledRenderer_opaque;

	GetDevice()->EventBegin("DrawWorld", threadID);

	if (shaderType == SHADERTYPE_TILEDFORWARD)
	{
		GetDevice()->BindResourcePS(resourceBuffers[RBTYPE_ENTITYINDEXLIST_OPAQUE], SBSLOT_ENTITYINDEXLIST, threadID);
	}

	if (grass)
	{
		GetDevice()->BindDepthStencilState(depthStencils[shaderType == SHADERTYPE_TILEDFORWARD ? DSSTYPE_DEPTHREADEQUAL : DSSTYPE_DEFAULT], STENCILREF_DEFAULT, threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);
		for (wiHairParticle* hair : culling.culledHairParticleSystems)
		{
			hair->Draw(camera, shaderType, threadID);
		}
	}

	if (!culledRenderer.empty() || (grass && culling.culledHairParticleSystems.empty()))
	{
		if (!wireRender)
		{
			if (refRes != nullptr)
			{
				GetDevice()->BindResourcePS(refRes, TEXSLOT_ONDEMAND6, threadID);
			}
		}

		RenderMeshes(camera->translation, culledRenderer, shaderType, RENDERTYPE_OPAQUE, threadID, tessellation, GetOcclusionCullingEnabled() && occlusionCulling);
	}

	GetDevice()->EventEnd(threadID);

}

void wiRenderer::DrawWorldTransparent(Camera* camera, SHADERTYPE shaderType, Texture2D* refracRes, Texture2D* refRes
	, Texture2D* waterRippleNormals, GRAPHICSTHREAD threadID, bool grass, bool occlusionCulling)
{

	const FrameCulling& culling = frameCullings[camera];
	const CulledCollection& culledRenderer = culling.culledRenderer_transparent;

	GetDevice()->EventBegin("DrawWorldTransparent", threadID);

	if (shaderType == SHADERTYPE_TILEDFORWARD)
	{
		GetDevice()->BindResourcePS(resourceBuffers[RBTYPE_ENTITYINDEXLIST_TRANSPARENT], SBSLOT_ENTITYINDEXLIST, threadID);
	}

	if (ocean != nullptr)
	{
		ocean->Render(camera, renderTime, threadID);
	}

	if (grass)
	{
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_HAIRALPHACOMPOSITION], STENCILREF_DEFAULT, threadID); // minimizes overdraw by depthcomp = less
		GetDevice()->BindBlendState(blendStates[BSTYPE_TRANSPARENT], threadID);
		for (wiHairParticle* hair : culling.culledHairParticleSystems)
		{
			hair->Draw(camera, shaderType, threadID);
		}
	}

	if (!culledRenderer.empty())
	{

		if (!wireRender)
		{
			GetDevice()->BindResourcePS(refRes, TEXSLOT_ONDEMAND6, threadID);
			GetDevice()->BindResourcePS(refracRes, TEXSLOT_ONDEMAND7, threadID);
			GetDevice()->BindResourcePS(waterRippleNormals, TEXSLOT_ONDEMAND8, threadID);
		}

		RenderMeshes(camera->translation, culledRenderer, shaderType, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, threadID, false, GetOcclusionCullingEnabled() && occlusionCulling);
	}

	GetDevice()->EventEnd(threadID);
}


void wiRenderer::DrawSky(GRAPHICSTHREAD threadID)
{
	if (!GetTemporalAAEnabled()) // If temporal AA is enabled, we should render a velocity map anyway, so render a black sky as that is the default!
	{
		if (enviroMap == nullptr)
			return;
	}

	GetDevice()->EventBegin("DrawSky", threadID);

	GetDevice()->BindPrimitiveTopology(TRIANGLELIST,threadID);
	GetDevice()->BindRasterizerState(rasterizers[RSTYPE_SKY],threadID);
	GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD],STENCILREF_SKY,threadID);
	GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE],threadID);
	
	GetDevice()->BindVS(vertexShaders[VSTYPE_SKY], threadID);
	GetDevice()->BindPS(pixelShaders[PSTYPE_SKY], threadID);
	
	if (enviroMap != nullptr)
	{
		GetDevice()->BindResourcePS(enviroMap, TEXSLOT_ENV_GLOBAL, threadID);
	}
	else
	{
		// If control gets here, it means we fill out only a velocity buffer on the background for temporal AA
		GetDevice()->BindResourcePS(wiTextureHelper::getInstance()->getBlackCubeMap(), TEXSLOT_ENV_GLOBAL, threadID);
	}

	GetDevice()->BindVertexLayout(nullptr, threadID);
	GetDevice()->Draw(240, 0, threadID);

	GetDevice()->EventEnd(threadID);
}
void wiRenderer::DrawSun(GRAPHICSTHREAD threadID)
{
	GetDevice()->EventBegin("DrawSun", threadID);

	GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
	GetDevice()->BindRasterizerState(rasterizers[RSTYPE_SKY], threadID);
	GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_SKY, threadID);
	GetDevice()->BindBlendState(blendStates[BSTYPE_ADDITIVE], threadID);

	GetDevice()->BindVS(vertexShaders[VSTYPE_SKY], threadID);
	GetDevice()->BindPS(pixelShaders[PSTYPE_SUN], threadID);

	GetDevice()->BindVertexLayout(nullptr, threadID);
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
			device->BindConstantBufferPS(constantBuffers[CBTYPE_DECAL], CB_GETBINDSLOT(DecalCB),threadID);
		}

		device->BindVS(vertexShaders[VSTYPE_DECAL], threadID);
		device->BindPS(pixelShaders[PSTYPE_DECAL], threadID);
		device->BindRasterizerState(rasterizers[RSTYPE_FRONT], threadID);
		device->BindBlendState(blendStates[BSTYPE_DECAL], threadID);
		device->BindDepthStencilState(depthStencils[DSSTYPE_DECAL], STENCILREF::STENCILREF_DEFAULT, threadID);
		device->BindVertexLayout(nullptr, threadID);
		device->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLESTRIP, threadID);

		for (Decal* decal : model->decals) 
		{

			if ((decal->texture || decal->normal) && camera->frustum.CheckBox(decal->bounds)) {

				device->BindResourcePS(decal->texture, TEXSLOT_ONDEMAND0, threadID);
				device->BindResourcePS(decal->normal, TEXSLOT_ONDEMAND1, threadID);

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

	for (EnvironmentProbe* probe : GetScene().environmentProbes)
	{
		if (probe->isUpToDate)
		{
			continue;
		}
		if(!probe->realTime)
		{
			probe->isUpToDate = true;
		}

		probe->cubeMap.Activate(threadID, 0, 0, 0, 1);

		std::vector<SHCAM> cameras;
		{
			cameras.clear();

			cameras.push_back(SHCAM(XMFLOAT4(0.5f, -0.5f, -0.5f, -0.5f), getCamera()->zNearP, getCamera()->zFarP, XM_PI / 2.0f)); //+x
			cameras.push_back(SHCAM(XMFLOAT4(0.5f, 0.5f, 0.5f, -0.5f), getCamera()->zNearP, getCamera()->zFarP, XM_PI / 2.0f)); //-x

			cameras.push_back(SHCAM(XMFLOAT4(1, 0, 0, -0), getCamera()->zNearP, getCamera()->zFarP, XM_PI / 2.0f)); //+y
			cameras.push_back(SHCAM(XMFLOAT4(0, 0, 0, -1), getCamera()->zNearP, getCamera()->zFarP, XM_PI / 2.0f)); //-y

			cameras.push_back(SHCAM(XMFLOAT4(0.707f, 0, 0, -0.707f), getCamera()->zNearP, getCamera()->zFarP, XM_PI / 2.0f)); //+z
			cameras.push_back(SHCAM(XMFLOAT4(0, 0.707f, 0.707f, 0), getCamera()->zNearP, getCamera()->zFarP, XM_PI / 2.0f)); //-z
		}


		CubeMapRenderCB cb;
		for (unsigned int i = 0; i < cameras.size(); ++i)
		{
			cameras[i].Update(XMLoadFloat3(&probe->translation));
			cb.mViewProjection[i] = cameras[i].getVP();
		}

		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_CUBEMAPRENDER], &cb, threadID);
		GetDevice()->BindConstantBufferGS(constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubeMapRenderCB), threadID);


		CulledList culledObjects;
		CulledCollection culledRenderer;

		SPHERE culler = SPHERE(probe->translation, getCamera()->zFarP);
		if (spTree != nullptr)
		{
			spTree->getVisible(culler, culledObjects);

			for (Cullable* object : culledObjects)
			{
				culledRenderer[((Object*)object)->mesh].push_front((Object*)object);
			}

			RenderMeshes(probe->translation, culledRenderer, SHADERTYPE_ENVMAPCAPTURE, RENDERTYPE_OPAQUE, threadID);
		}

		// sky
		{
			GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
			GetDevice()->BindRasterizerState(rasterizers[RSTYPE_SKY], threadID);
			GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_SKY, threadID);
			GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);

			GetDevice()->BindVS(vertexShaders[VSTYPE_ENVMAP_SKY], threadID);
			GetDevice()->BindPS(pixelShaders[PSTYPE_ENVMAP_SKY], threadID);
			GetDevice()->BindGS(geometryShaders[GSTYPE_ENVMAP_SKY], threadID);

			GetDevice()->BindResourcePS(enviroMap, TEXSLOT_ENV_GLOBAL, threadID);

			GetDevice()->BindVertexLayout(nullptr, threadID);
			GetDevice()->Draw(240, 0, threadID);
		}


		GetDevice()->BindGS(nullptr, threadID);


		probe->cubeMap.Deactivate(threadID);

		GetDevice()->GenerateMips(probe->cubeMap.GetTexture(), threadID);

	}

	GetDevice()->EventEnd(threadID);
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
		Texture3DDesc desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = voxelSceneData.res;
		desc.Height = voxelSceneData.res;
		desc.Depth = voxelSceneData.res;
		desc.MipLevels = 0;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		desc.Usage = USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		textures[TEXTYPE_3D_VOXELRADIANCE] = new Texture3D;
		textures[TEXTYPE_3D_VOXELRADIANCE]->RequestIndepententShaderResourcesForMIPs(true);
		textures[TEXTYPE_3D_VOXELRADIANCE]->RequestIndepententUnorderedAccessResourcesForMIPs(true);
		HRESULT hr = GetDevice()->CreateTexture3D(&desc, nullptr, (Texture3D**)&textures[TEXTYPE_3D_VOXELRADIANCE]);
		assert(SUCCEEDED(hr));
	}
	if (voxelSceneData.secondaryBounceEnabled && textures[TEXTYPE_3D_VOXELRADIANCE_HELPER] == nullptr)
	{
		Texture3DDesc desc = ((Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE])->GetDesc();
		textures[TEXTYPE_3D_VOXELRADIANCE_HELPER] = new Texture3D;
		textures[TEXTYPE_3D_VOXELRADIANCE_HELPER]->RequestIndepententShaderResourcesForMIPs(true);
		textures[TEXTYPE_3D_VOXELRADIANCE_HELPER]->RequestIndepententUnorderedAccessResourcesForMIPs(true);
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

		const FrameCulling& culling = frameCullings[getCamera()];

		// Tell the voxelizer about the lights in the light array (exclude decals)
		MiscCB cb;
		cb.mColor.x = 0;
		cb.mColor.y = (float)culling.culledLight_count;
		// This will tell the copy compute shader to not smooth the voxel texture in this frame (todo: find better way):
		// The problem with blending the voxel texture is when the grid is repositioned, the results will be incorrect
		cb.mColor.z = voxelSceneData.centerChangedThisFrame ? 1.0f : 0.0f; 
		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &cb, threadID);

		ViewPort VP;
		VP.TopLeftX = 0;
		VP.TopLeftY = 0;
		VP.Width = (float)voxelSceneData.res;
		VP.Height = (float)voxelSceneData.res;
		VP.MinDepth = 0.0f;
		VP.MaxDepth = 1.0f;
		GetDevice()->BindViewports(1, &VP, threadID);

		GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);

		GPUUnorderedResource* UAVs[] = { resourceBuffers[RBTYPE_VOXELSCENE] };
		GetDevice()->BindRenderTargetsUAVs(0, nullptr, nullptr, UAVs, 0, 1, threadID);

		RenderMeshes(center, culledRenderer, SHADERTYPE_VOXELIZE, RENDERTYPE_OPAQUE, threadID);


		// Copy the packed voxel scene data to a 3D texture, then delete the voxel scene emission data. The cone tracing will operate on the 3D texture
		GetDevice()->EventBegin("Voxel Scene Copy - Clear", threadID);
		GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);
		GetDevice()->BindUnorderedAccessResourceCS(resourceBuffers[RBTYPE_VOXELSCENE], 0, threadID);
		GetDevice()->BindUnorderedAccessResourceCS(textures[TEXTYPE_3D_VOXELRADIANCE], 1, threadID);

		if (GetDevice()->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_UNORDEREDACCESSTEXTURE_LOAD_FORMAT_EXT))
		{
			GetDevice()->BindCS(computeShaders[CSTYPE_VOXELSCENECOPYCLEAR_TEMPORALSMOOTHING], threadID);
		}
		else
		{
			GetDevice()->BindCS(computeShaders[CSTYPE_VOXELSCENECOPYCLEAR], threadID);
		}
		GetDevice()->Dispatch((UINT)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 1024), 1, 1, threadID);
		GetDevice()->EventEnd(threadID);

		if (voxelSceneData.secondaryBounceEnabled)
		{
			GetDevice()->EventBegin("Voxel Radiance Secondary Bounce", threadID);
			GetDevice()->UnBindUnorderedAccessResources(1, 1, threadID);
			// Pre-integrate the voxel texture by creating blurred mip levels:
			GenerateMipChain((Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE], MIPGENFILTER_LINEAR, threadID);
			GetDevice()->BindUnorderedAccessResourceCS(textures[TEXTYPE_3D_VOXELRADIANCE_HELPER], 0, threadID);
			GetDevice()->BindResourceCS(textures[TEXTYPE_3D_VOXELRADIANCE], 0, threadID);
			GetDevice()->BindResourceCS(resourceBuffers[RBTYPE_VOXELSCENE], 1, threadID);
			GetDevice()->BindCS(computeShaders[CSTYPE_VOXELRADIANCESECONDARYBOUNCE], threadID);
			GetDevice()->Dispatch((UINT)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 1024), 1, 1, threadID);
			GetDevice()->EventEnd(threadID);

			GetDevice()->EventBegin("Voxel Scene Clear Normals", threadID);
			GetDevice()->UnBindResources(1, 1, threadID);
			GetDevice()->BindUnorderedAccessResourceCS(resourceBuffers[RBTYPE_VOXELSCENE], 0, threadID);
			GetDevice()->BindCS(computeShaders[CSTYPE_VOXELCLEARONLYNORMAL], threadID);
			GetDevice()->Dispatch((UINT)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 1024), 1, 1, threadID);
			GetDevice()->EventEnd(threadID);

			result = (Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE_HELPER];
		}

		GetDevice()->BindCS(nullptr, threadID);
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
		GetDevice()->BindResourceVS(result, TEXSLOT_VOXELRADIANCE, threadID);
	}
	GetDevice()->BindResourcePS(result, TEXSLOT_VOXELRADIANCE, threadID);
	GetDevice()->BindResourceCS(result, TEXSLOT_VOXELRADIANCE, threadID);

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
		Texture2DDesc desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.ArraySize = 1;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
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
		device->BindUnorderedAccessResourceCS(frustumBuffer, UAVSLOT_TILEFRUSTUMS, threadID);
		device->BindCS(computeShaders[CSTYPE_TILEFRUSTUMS], threadID);

		DispatchParamsCB dispatchParams;
		dispatchParams.numThreads[0] = tileCount.x;
		dispatchParams.numThreads[1] = tileCount.y;
		dispatchParams.numThreads[2] = 1;
		dispatchParams.numThreadGroups[0] = (UINT)ceilf(dispatchParams.numThreads[0] / (float)TILED_CULLING_BLOCKSIZE);
		dispatchParams.numThreadGroups[1] = (UINT)ceilf(dispatchParams.numThreads[1] / (float)TILED_CULLING_BLOCKSIZE);
		dispatchParams.numThreadGroups[2] = 1;
		device->UpdateBuffer(constantBuffers[CBTYPE_DISPATCHPARAMS], &dispatchParams, threadID);
		device->BindConstantBufferCS(constantBuffers[CBTYPE_DISPATCHPARAMS], CB_GETBINDSLOT(DispatchParamsCB), threadID);

		device->Dispatch(dispatchParams.numThreadGroups[0], dispatchParams.numThreadGroups[1], dispatchParams.numThreadGroups[2], threadID);
		device->UnBindUnorderedAccessResources(UAVSLOT_TILEFRUSTUMS, 1, threadID);
	}

	if (textures[TEXTYPE_2D_DEBUGUAV] == nullptr || _resolutionChanged)
	{
		SAFE_DELETE(textures[TEXTYPE_2D_DEBUGUAV]);

		Texture2DDesc desc;
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

		device->BindResourceCS(frustumBuffer, SBSLOT_TILEFRUSTUMS, threadID);
		
		if (GetDebugLightCulling())
		{
			device->BindUnorderedAccessResourceCS(textures[TEXTYPE_2D_DEBUGUAV], UAVSLOT_DEBUGTEXTURE, threadID);
			if (GetAdvancedLightCulling())
			{
				if (deferred)
				{
					device->BindCS(computeShaders[CSTYPE_TILEDLIGHTCULLING_DEFERRED_ADVANCED_DEBUG], threadID);
				}
				else
				{
					device->BindCS(computeShaders[CSTYPE_TILEDLIGHTCULLING_ADVANCED_DEBUG], threadID);
				}
			}
			else
			{
				if (deferred)
				{
					device->BindCS(computeShaders[CSTYPE_TILEDLIGHTCULLING_DEFERRED_DEBUG], threadID);
				}
				else
				{
					device->BindCS(computeShaders[CSTYPE_TILEDLIGHTCULLING_DEBUG], threadID);
				}
			}
		}
		else
		{
			if (GetAdvancedLightCulling())
			{
				if (deferred)
				{
					device->BindCS(computeShaders[CSTYPE_TILEDLIGHTCULLING_DEFERRED_ADVANCED], threadID);
				}
				else
				{
					device->BindCS(computeShaders[CSTYPE_TILEDLIGHTCULLING_ADVANCED], threadID);
				}
			}
			else
			{
				if (deferred)
				{
					device->BindCS(computeShaders[CSTYPE_TILEDLIGHTCULLING_DEFERRED], threadID);
				}
				else
				{
					device->BindCS(computeShaders[CSTYPE_TILEDLIGHTCULLING], threadID);
				}
			}
		}

		DispatchParamsCB dispatchParams;
		dispatchParams.numThreadGroups[0] = tileCount.x;
		dispatchParams.numThreadGroups[1] = tileCount.y;
		dispatchParams.numThreadGroups[2] = 1;
		dispatchParams.numThreads[0] = dispatchParams.numThreadGroups[0] * TILED_CULLING_BLOCKSIZE;
		dispatchParams.numThreads[1] = dispatchParams.numThreadGroups[1] * TILED_CULLING_BLOCKSIZE;
		dispatchParams.numThreads[2] = 1;
		dispatchParams.value0 = frameCullings[getCamera()].culledLight_count + (UINT)frameCullings[getCamera()].culledDecals.size(); // entity count (forward_list does not have size())
		device->UpdateBuffer(constantBuffers[CBTYPE_DISPATCHPARAMS], &dispatchParams, threadID);
		device->BindConstantBufferCS(constantBuffers[CBTYPE_DISPATCHPARAMS], CB_GETBINDSLOT(DispatchParamsCB), threadID);

		if (deferred)
		{
			const GPUUnorderedResource* uavs[] = {
				static_cast<const GPUUnorderedResource*>(textures[TEXTYPE_2D_TILEDDEFERRED_DIFFUSEUAV]),
				static_cast<const GPUUnorderedResource*>(resourceBuffers[RBTYPE_ENTITYINDEXLIST_TRANSPARENT]),
				static_cast<const GPUUnorderedResource*>(textures[TEXTYPE_2D_TILEDDEFERRED_SPECULARUAV]),
			};
			device->BindUnorderedAccessResourcesCS(uavs, UAVSLOT_TILEDDEFERRED_DIFFUSE, ARRAYSIZE(uavs), threadID);

			GetDevice()->BindResourceCS(Light::shadowMapArray_2D, TEXSLOT_SHADOWARRAY_2D, threadID);
			GetDevice()->BindResourceCS(Light::shadowMapArray_Cube, TEXSLOT_SHADOWARRAY_CUBE, threadID);
		}
		else
		{
			const GPUUnorderedResource* uavs[] = {
				static_cast<const GPUUnorderedResource*>(resourceBuffers[RBTYPE_ENTITYINDEXLIST_OPAQUE]),
				static_cast<const GPUUnorderedResource*>(resourceBuffers[RBTYPE_ENTITYINDEXLIST_TRANSPARENT]),
			};
			device->BindUnorderedAccessResourcesCS(uavs, UAVSLOT_ENTITYINDEXLIST_OPAQUE, ARRAYSIZE(uavs), threadID);
		}

		device->Dispatch(dispatchParams.numThreadGroups[0], dispatchParams.numThreadGroups[1], dispatchParams.numThreadGroups[2], threadID);

		device->BindCS(nullptr, threadID);
		device->UnBindUnorderedAccessResources(0, 8, threadID); // this unbinds pretty much every uav

		device->EventEnd(threadID);
	}

	wiProfiler::GetInstance().EndRange(threadID);
}
void wiRenderer::ResolveMSAADepthBuffer(Texture2D* dst, Texture2D* src, GRAPHICSTHREAD threadID)
{
	GetDevice()->EventBegin("Resolve MSAA DepthBuffer", threadID);

	GetDevice()->BindResourceCS(src, TEXSLOT_ONDEMAND0, threadID);
	GetDevice()->BindUnorderedAccessResourceCS(dst, 0, threadID);

	Texture2DDesc desc = src->GetDesc();

	GetDevice()->BindCS(computeShaders[CSTYPE_RESOLVEMSAADEPTHSTENCIL], threadID);
	GetDevice()->Dispatch((UINT)ceilf(desc.Width / 16.f), (UINT)ceilf(desc.Height / 16.f), 1, threadID);
	GetDevice()->BindCS(nullptr, threadID);


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
	Texture2DDesc desc = texture->GetDesc();

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
		GetDevice()->BindCS(computeShaders[CSTYPE_GENERATEMIPCHAIN2D_SIMPLEFILTER], threadID);
		GetDevice()->BindSamplerCS(samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_LINEAR:
		GetDevice()->EventBegin("GenerateMipChain 2D - LinearFilter", threadID);
		GetDevice()->BindCS(computeShaders[CSTYPE_GENERATEMIPCHAIN2D_SIMPLEFILTER], threadID);
		GetDevice()->BindSamplerCS(samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_LINEAR_MAXIMUM:
		GetDevice()->EventBegin("GenerateMipChain 2D - LinearMaxFilter", threadID);
		GetDevice()->BindCS(computeShaders[CSTYPE_GENERATEMIPCHAIN2D_SIMPLEFILTER], threadID);
		GetDevice()->BindSamplerCS(customsamplers[SSTYPE_MAXIMUM_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_GAUSSIAN:
		GetDevice()->EventBegin("GenerateMipChain 2D - GaussianFilter", threadID);
		GetDevice()->BindCS(computeShaders[CSTYPE_GENERATEMIPCHAIN2D_GAUSSIAN], threadID);
		break;
	}

	for (UINT i = 0; i < desc.MipLevels - 1; ++i)
	{
		GetDevice()->BindUnorderedAccessResourceCS(texture, 0, threadID, i + 1);
		GetDevice()->BindResourceCS(texture, TEXSLOT_UNIQUE0, threadID, i);
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
	GetDevice()->BindCS(nullptr, threadID);

	GetDevice()->EventEnd(threadID);
}
void wiRenderer::GenerateMipChain(Texture3D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID)
{
	Texture3DDesc desc = texture->GetDesc();

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
		GetDevice()->BindCS(computeShaders[CSTYPE_GENERATEMIPCHAIN3D_SIMPLEFILTER], threadID);
		GetDevice()->BindSamplerCS(samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_LINEAR:
		GetDevice()->EventBegin("GenerateMipChain 3D - LinearFilter", threadID);
		GetDevice()->BindCS(computeShaders[CSTYPE_GENERATEMIPCHAIN3D_SIMPLEFILTER], threadID);
		GetDevice()->BindSamplerCS(samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_LINEAR_MAXIMUM:
		GetDevice()->EventBegin("GenerateMipChain 3D - LinearMaxFilter", threadID);
		GetDevice()->BindCS(computeShaders[CSTYPE_GENERATEMIPCHAIN3D_SIMPLEFILTER], threadID);
		GetDevice()->BindSamplerCS(customsamplers[SSTYPE_MAXIMUM_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_GAUSSIAN:
		GetDevice()->EventBegin("GenerateMipChain 3D - GaussianFilter", threadID);
		GetDevice()->BindCS(computeShaders[CSTYPE_GENERATEMIPCHAIN3D_GAUSSIAN], threadID);
		break;
	}

	for (UINT i = 0; i < desc.MipLevels - 1; ++i)
	{
		GetDevice()->BindUnorderedAccessResourceCS(texture, 0, threadID, i + 1);
		GetDevice()->BindResourceCS(texture, TEXSLOT_UNIQUE0, threadID, i);
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
	GetDevice()->BindCS(nullptr, threadID);

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

					Texture2DDesc desc;
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
			Texture2DDesc desc = atlasTexture->GetDesc();
			decal->atlasMulAdd = XMFLOAT4((float)rect.w / (float)desc.Width, (float)rect.h / (float)desc.Height, (float)rect.x / (float)desc.Width, (float)rect.y / (float)desc.Height);
		}

	}

	if (atlasTexture != nullptr)
	{
		device->BindResourcePS(atlasTexture, TEXSLOT_DECALATLAS, threadID);
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
	value.mFog = world.fogSEH;
	value.mHorizon = world.horizon;
	value.mZenith = world.zenith;
	value.mSpecularAA = SPECULARAA;
	value.mVoxelRadianceDataSize = voxelSceneData.voxelsize;
	value.mVoxelRadianceDataRes = GetVoxelRadianceEnabled() ? (UINT)voxelSceneData.res : 0;
	value.mVoxelRadianceDataConeTracingQuality = voxelSceneData.coneTracingQuality;
	value.mVoxelRadianceDataFalloff = voxelSceneData.falloff;
	value.mVoxelRadianceDataCenter = voxelSceneData.center;
	value.mAdvancedRefractions = GetAdvancedRefractionsEnabled() ? 1 : 0;
	value.mEntityCullingTileCount = GetEntityCullingTileCount();

	if (memcmp(&prevcb[threadID], &value, sizeof(WorldCB)) != 0) // prevent overcommit
	{
		if (threadID == GRAPHICSTHREAD_IMMEDIATE)
		{
			wiRenderer::GetDevice()->LOCK();
		}
		prevcb[threadID] = value;
		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_WORLD], &prevcb[threadID], threadID);

		if (threadID == GRAPHICSTHREAD_IMMEDIATE)
		{
			wiRenderer::GetDevice()->UNLOCK();
		}
	}
}
void wiRenderer::UpdateFrameCB(GRAPHICSTHREAD threadID)
{
	FrameCB cb;

	cb.mTime = renderTime;
	cb.mTimePrev = renderTime_Prev;
	cb.mDeltaTime = deltaTime;
	cb.mForceFieldOffset = entityArrayOffset_ForceFields;
	cb.mForceFieldCount = entityArrayCount_ForceFields;
	auto& wind = GetScene().wind;
	cb.mWindRandomness = wind.randomness;
	cb.mWindWaveSize = wind.waveSize;
	cb.mWindDirection = wind.direction;
	cb.mFrameCount = (UINT)GetDevice()->GetFrameCount();
	cb.mSunEntityArrayIndex = GetSunArrayIndex();
	cb.mTemporalAAJitter = temporalAAJitter;
	cb.mTemporalAAJitterPrev = temporalAAJitterPrev;
	cb.mGlobalEnvMap0 = globalEnvProbes[0] == nullptr ? XMFLOAT3(0, 0, 0) : globalEnvProbes[0]->translation;
	cb.mGlobalEnvMap1 = globalEnvProbes[1] == nullptr ? XMFLOAT3(0, 0, 0) : globalEnvProbes[1]->translation;

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
	GetDevice()->BindResourcePS(slot0, TEXSLOT_GBUFFER0, threadID);
	GetDevice()->BindResourcePS(slot1, TEXSLOT_GBUFFER1, threadID);
	GetDevice()->BindResourcePS(slot2, TEXSLOT_GBUFFER2, threadID);
	GetDevice()->BindResourcePS(slot3, TEXSLOT_GBUFFER3, threadID);
	GetDevice()->BindResourcePS(slot4, TEXSLOT_GBUFFER4, threadID);

	GetDevice()->BindResourceCS(slot0, TEXSLOT_GBUFFER0, threadID);
	GetDevice()->BindResourceCS(slot1, TEXSLOT_GBUFFER1, threadID);
	GetDevice()->BindResourceCS(slot2, TEXSLOT_GBUFFER2, threadID);
	GetDevice()->BindResourceCS(slot3, TEXSLOT_GBUFFER3, threadID);
	GetDevice()->BindResourceCS(slot4, TEXSLOT_GBUFFER4, threadID);
}
void wiRenderer::UpdateDepthBuffer(Texture2D* depth, Texture2D* linearDepth, GRAPHICSTHREAD threadID)
{
	GetDevice()->BindResourcePS(depth, TEXSLOT_DEPTH, threadID);
	GetDevice()->BindResourceVS(depth, TEXSLOT_DEPTH, threadID);
	GetDevice()->BindResourceGS(depth, TEXSLOT_DEPTH, threadID);
	GetDevice()->BindResourceCS(depth, TEXSLOT_DEPTH, threadID);

	GetDevice()->BindResourcePS(linearDepth, TEXSLOT_LINEARDEPTH, threadID);
	GetDevice()->BindResourceVS(linearDepth, TEXSLOT_LINEARDEPTH, threadID);
	GetDevice()->BindResourceGS(linearDepth, TEXSLOT_LINEARDEPTH, threadID);
	GetDevice()->BindResourceCS(linearDepth, TEXSLOT_LINEARDEPTH, threadID);
}

void wiRenderer::FinishLoading()
{
	// Kept for backwards compatibility
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

		Texture2DDesc desc;
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
		Texture2DDesc luminance_map_desc = luminance_map->GetDesc();
		device->BindCS(computeShaders[CSTYPE_LUMINANCE_PASS1], threadID);
		device->BindResourceCS(sourceImage, TEXSLOT_ONDEMAND0, threadID);
		device->BindUnorderedAccessResourceCS(luminance_map, 0, threadID);
		device->Dispatch(luminance_map_desc.Width/16, luminance_map_desc.Height/16, 1, threadID);

		// Pass 2 : Reduce for average luminance until we got an 1x1 texture
		Texture2DDesc luminance_avg_desc;
		for (size_t i = 0; i < luminance_avg.size(); ++i)
		{
			luminance_avg_desc = luminance_avg[i]->GetDesc();
			device->BindCS(computeShaders[CSTYPE_LUMINANCE_PASS2], threadID);
			device->BindUnorderedAccessResourceCS(luminance_avg[i], 0, threadID);
			if (i > 0)
			{
				device->BindResourceCS(luminance_avg[i-1], TEXSLOT_ONDEMAND0, threadID);
			}
			else
			{
				device->BindResourceCS(luminance_map, TEXSLOT_ONDEMAND0, threadID);
			}
			device->Dispatch(luminance_avg_desc.Width, luminance_avg_desc.Height, 1, threadID);
		}


		device->BindCS(nullptr, threadID);
		device->UnBindUnorderedAccessResources(0, 1, threadID);

		return luminance_avg.back();
	}

	return nullptr;
}

wiWaterPlane wiRenderer::GetWaterPlane()
{
	return waterPlane;
}

wiRenderer::Picked wiRenderer::Pick(RAY& ray, int pickType, const std::string& layer,
	const std::string& layerDisable)
{
	std::vector<Picked> pickPoints;

	// pick meshes...
	CulledCollection culledRenderer;
	CulledList culledObjects;
	wiSPTree* searchTree = spTree;
	if (searchTree != nullptr)
	{
		searchTree->getVisible(ray, culledObjects);

		RayIntersectMeshes(ray, culledObjects, pickPoints, pickType, true, layer, layerDisable, true);
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
	}
	if (pickType & PICK_ENVPROBE)
	{
		for (auto& x : GetScene().environmentProbes)
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
wiRenderer::Picked wiRenderer::Pick(long cursorX, long cursorY, int pickType, const std::string& layer,
	const std::string& layerDisable)
{
	RAY ray = getPickRay(cursorX, cursorY);

	return Pick(ray, pickType, layer, layerDisable);
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
	int pickType, bool dynamicObjects, const std::string& layer, const std::string& layerDisable, bool onlyVisible)
{
	if (culledObjects.empty())
	{
		return;
	}

	bool checkLayers = false;
	if (layer.length() > 0)
	{
		checkLayers = true;
	}
	bool dontcheckLayers = false;
	if (layerDisable.length() > 0)
	{
		dontcheckLayers = true;
	}

	XMVECTOR& rayOrigin = XMLoadFloat3(&ray.origin);
	XMVECTOR& rayDirection = XMVector3Normalize(XMLoadFloat3(&ray.direction));

	// pre allocate helper vector array:
	static size_t _arraySize = 10000;
	static XMVECTOR* _vertices = (XMVECTOR*)_mm_malloc(sizeof(XMVECTOR)*_arraySize, 16);

	for (Cullable* culled : culledObjects)
	{
		Object* object = (Object*)culled;

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

		// layer support
		if (checkLayers || dontcheckLayers)
		{
			string id = object->GetLayerID();

			if (checkLayers && layer.find(id) == string::npos)
			{
				continue;
			}
			if (dontcheckLayers && layerDisable.find(id) != string::npos)
			{
				continue;
			}
		}

		Mesh* mesh = object->mesh;
		if (mesh->vertices_POS.size() >= _arraySize)
		{
			// grow preallocated vector helper array
			_mm_free(_vertices);
			_arraySize = (mesh->vertices_POS.size() + 1) * 2;
			_vertices = (XMVECTOR*)_mm_malloc(sizeof(XMVECTOR)*_arraySize, 16);
		}

		XMMATRIX& objectMat = object->getMatrix();
		XMMATRIX& objectMat_Inverse = XMMatrixInverse(nullptr, objectMat);

		XMVECTOR& rayOrigin_local = XMVector3Transform(rayOrigin, objectMat_Inverse);
		XMVECTOR& rayDirection_local = XMVector3Normalize(XMVector3TransformNormal(rayDirection, objectMat_Inverse));

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
				_vertices[i] = mesh->vertices_Transformed_POS[i].Load();
			}
		}
		else
		{
			for (size_t i = 0; i < mesh->vertices_POS.size(); ++i)
			{
				_vertices[i] = mesh->vertices_POS[i].Load();
			}
		}

		for (size_t i = 0; i < mesh->indices.size(); i += 3)
		{
			int i0 = mesh->indices[i], i1 = mesh->indices[i + 1], i2 = mesh->indices[i + 2];
			XMVECTOR& V0 = _vertices[i0];
			XMVECTOR& V1 = _vertices[i1];
			XMVECTOR& V2 = _vertices[i2];
			float distance = 0;
			if (TriangleTests::Intersects(rayOrigin_local, rayDirection_local, V0, V1, V2, distance))
			{
				XMVECTOR& pos = XMVector3Transform(XMVectorAdd(rayOrigin_local, rayDirection_local*distance), objectMat);
				XMVECTOR& nor = XMVector3TransformNormal(XMVector3Normalize(XMVector3Cross(XMVectorSubtract(V2, V1), XMVectorSubtract(V1, V0))), objectMat);
				Picked picked = Picked();
				picked.transform = object;
				picked.object = object;
				XMStoreFloat3(&picked.position, pos);
				XMStoreFloat3(&picked.normal, nor);
				picked.distance = wiMath::Distance(pos, rayOrigin);
				XMVECTOR& tmpTex = XMLoadHalf4(&mesh->vertices_TEX[i0].tex);
				picked.subsetIndex = (int)XMVectorGetZ(tmpTex); // half has no normal conversion, so have to do load-store
				points.push_back(picked);
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

Model* wiRenderer::LoadModel(const std::string& dir, const std::string& name, const XMMATRIX& transform, const std::string& ident)
{
	static int unique_identifier = 0;

	stringstream idss("");
	idss<<"_"<<ident;

	Model* model = new Model;
	model->LoadFromDisk(dir,name,idss.str());

	model->transform(transform);

	AddModel(model);

	LoadWorldInfo(dir, name);

	unique_identifier++;

	return model;
}
void wiRenderer::LoadWorldInfo(const std::string& dir, const std::string& name)
{
	LoadWiWorldInfo(dir, name+".wiw", GetScene().worldInfo, GetScene().wind);
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
	model->lights.push_back(defaultLight);
	GetScene().models.push_back(model);

	if (spTree_lights) {
		spTree_lights->AddObjects(spTree_lights->root, std::vector<Cullable*>(model->lights.begin(), model->lights.end()));
	}
	else
	{
		GenerateSPTree(spTree_lights, std::vector<Cullable*>(model->lights.begin(), model->lights.end()), SPTREE_GENERATE_OCTREE);
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

void wiRenderer::PutEnvProbe(const XMFLOAT3& position, int resolution)
{
	EnvironmentProbe* probe = new EnvironmentProbe;
	probe->transform(position);
	probe->cubeMap.InitializeCube(resolution, true, FORMAT_R16G16B16A16_FLOAT, 0);

	scene->environmentProbes.push_back(probe);
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
		mesh->impostorTarget.Initialize(res * 6, res, true, FORMAT_R8G8B8A8_UNORM, 0);
		mesh->impostorTarget.Add(FORMAT_R8G8B8A8_UNORM);	// normal
		mesh->impostorTarget.Add(FORMAT_R8_UNORM);			// roughness
		mesh->impostorTarget.Add(FORMAT_R8_UNORM);			// reflectance
		mesh->impostorTarget.Add(FORMAT_R8_UNORM);			// metalness
	}

	Camera savedCam = *cam;

	GetDevice()->LOCK();

	BindPersistentState(threadID);

	const XMFLOAT4X4 __identity = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	const Instance instance(__identity);
	const InstancePrev instancePrev(__identity);
	const UINT instanceOffset = GetDevice()->AppendRingBuffer(dynamicVertexBufferPool, &instance, sizeof(instance), threadID);
	const UINT instancePrevOffset = GetDevice()->AppendRingBuffer(dynamicVertexBufferPool, &instancePrev, sizeof(instancePrev), threadID);

	GPUBuffer* vbs[] = {
		mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_POS.IsValid() ? &mesh->streamoutBuffer_POS : &mesh->vertexBuffer_POS),
		mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_NOR.IsValid() ? &mesh->streamoutBuffer_NOR : &mesh->vertexBuffer_NOR),
		&mesh->vertexBuffer_TEX,
		mesh->hasDynamicVB() ? dynamicVertexBufferPool : (mesh->streamoutBuffer_PRE.IsValid() ? &mesh->streamoutBuffer_PRE : &mesh->vertexBuffer_POS),
		dynamicVertexBufferPool,
		dynamicVertexBufferPool
	};
	UINT strides[] = {
		sizeof(Mesh::Vertex_POS),
		sizeof(Mesh::Vertex_NOR),
		sizeof(Mesh::Vertex_TEX),
		sizeof(Mesh::Vertex_POS),
		sizeof(Instance),
		sizeof(InstancePrev)
	};
	UINT offsets[] = {
		mesh->hasDynamicVB() ? mesh->bufferOffset_POS : 0,
		mesh->hasDynamicVB() ? mesh->bufferOffset_NOR : 0,
		0,
		mesh->hasDynamicVB() ? mesh->bufferOffset_PRE : 0,
		instanceOffset,
		instancePrevOffset
	};
	GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);

	GetDevice()->BindIndexBuffer(&mesh->indexBuffer, mesh->GetIndexFormat(), 0, threadID);


	GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);
	GetDevice()->BindRasterizerState(rasterizers[RSTYPE_DOUBLESIDED], threadID);
	GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], 0, threadID);
	GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
	GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_OBJECT_ALL], threadID);
	GetDevice()->BindVS(vertexShaders[VSTYPE_OBJECT_COMMON], threadID);
	GetDevice()->BindPS(pixelShaders[PSTYPE_CAPTUREIMPOSTOR], threadID);

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
				GetDevice()->BindConstantBufferPS(&subset.material->constantBuffer, CB_GETBINDSLOT(MaterialCB), threadID);

				GetDevice()->BindResourcePS(subset.material->GetBaseColorMap(), TEXSLOT_ONDEMAND0, threadID);
				GetDevice()->BindResourcePS(subset.material->GetNormalMap(), TEXSLOT_ONDEMAND1, threadID);
				GetDevice()->BindResourcePS(subset.material->GetRoughnessMap(), TEXSLOT_ONDEMAND2, threadID);
				GetDevice()->BindResourcePS(subset.material->GetReflectanceMap(), TEXSLOT_ONDEMAND3, threadID);
				GetDevice()->BindResourcePS(subset.material->GetMetalnessMap(), TEXSLOT_ONDEMAND4, threadID);
				GetDevice()->BindResourcePS(subset.material->GetDisplacementMap(), TEXSLOT_ONDEMAND5, threadID);


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
	GetDevice()->LOCK();

	GetScene().AddModel(model);

	FixedUpdate();

	Add(model->objects);
	Add(model->lights);

	//UpdateRenderData(nullptr);

	SetUpCubes();
	SetUpBoneLines();

	GetDevice()->UNLOCK();
}

void wiRenderer::Add(Object* value)
{
	list<Object*> collection(0);
	collection.push_back(value);
	Add(collection);
}
void wiRenderer::Add(Light* value)
{
	list<Light*> collection(0);
	collection.push_back(value);
	Add(collection);
}
void wiRenderer::Add(ForceField* value)
{
	list<ForceField*> collection(0);
	collection.push_back(value);
	Add(collection);
}
void wiRenderer::Add(const list<Object*>& objects)
{
	for (Object* x : objects)
	{
		if (x->parent == nullptr)
		{
			GetScene().GetWorldNode()->Add(x);
		}
	}
	if (spTree) {
		spTree->AddObjects(spTree->root, std::vector<Cullable*>(objects.begin(), objects.end()));
	}
	else
	{
		GenerateSPTree(spTree, std::vector<Cullable*>(objects.begin(), objects.end()), SPTREE_GENERATE_OCTREE);
	}
}
void wiRenderer::Add(const list<Light*>& lights)
{
	for (Light* x : lights)
	{
		if (x->parent == nullptr)
		{
			GetScene().GetWorldNode()->Add(x);
		}
	}
	if (spTree_lights) {
		spTree_lights->AddObjects(spTree_lights->root, std::vector<Cullable*>(lights.begin(), lights.end()));
	}
	else
	{
		GenerateSPTree(spTree_lights, std::vector<Cullable*>(lights.begin(), lights.end()), SPTREE_GENERATE_OCTREE);
	}
}
void wiRenderer::Add(const list<ForceField*>& forces)
{
	for (ForceField* x : forces)
	{
		if (x->parent == nullptr)
		{
			GetScene().GetWorldNode()->Add(x);
		}
	}
}

void wiRenderer::Remove(Object* value)
{
	if (value != nullptr)
	{
		for (auto& x : GetScene().models)
		{
			x->objects.remove(value);
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
			x->lights.remove(value);
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
			x->decals.remove(value);
		}
		value->detach();
	}
}
void wiRenderer::Remove(EnvironmentProbe* value)
{
	if (value != nullptr)
	{
		GetScene().environmentProbes.remove(value);
		value->detach();
	}
}
void wiRenderer::Remove(ForceField* value)
{
	if (value != nullptr)
	{
		for (auto& x : GetScene().models)
		{
			x->forces.remove(value);
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
