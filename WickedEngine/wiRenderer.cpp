#include "wiRenderer.h"
#include "ShaderInterop.h"
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
#include "ResourceMapping.h"
#include "wiGraphicsDevice_DX11.h"
#include "wiTranslator.h"
#include "lightCullingCSInterop.h"
#include "wiRectPacker.h"
#include "wiBackLog.h"
#include "wiProfiler.h"

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

int wiRenderer::SHADOWRES_2D = 1024, wiRenderer::SHADOWRES_CUBE = 256, wiRenderer::SHADOWCOUNT_2D = 5 + 3 + 3, wiRenderer::SHADOWCOUNT_CUBE = 5, wiRenderer::SOFTSHADOWQUALITY_2D = 2;
bool wiRenderer::HAIRPARTICLEENABLED=true,wiRenderer::EMITTERSENABLED=true;
bool wiRenderer::wireRender = false, wiRenderer::debugSpheres = false, wiRenderer::debugBoneLines = false, wiRenderer::debugPartitionTree = false
, wiRenderer::debugEnvProbes = false, wiRenderer::gridHelper = false, wiRenderer::voxelHelper = false, wiRenderer::requestReflectionRendering = false;
float wiRenderer::renderTime = 0;

Texture2D* wiRenderer::enviroMap,*wiRenderer::colorGrading;
float wiRenderer::GameSpeed=1,wiRenderer::overrideGameSpeed=1;
bool wiRenderer::debugLightCulling = false;
bool wiRenderer::occlusionCulling = true;
wiRenderer::VoxelizedSceneData wiRenderer::voxelSceneData = VoxelizedSceneData();
int wiRenderer::visibleCount;
wiRenderTarget wiRenderer::normalMapRT, wiRenderer::imagesRT, wiRenderer::imagesRTAdd;
Camera *wiRenderer::cam = nullptr, *wiRenderer::refCam = nullptr, *wiRenderer::prevFrameCam = nullptr;
PHYSICS* wiRenderer::physicsEngine = nullptr;

string wiRenderer::SHADERPATH = "shaders/";
#pragma endregion

#pragma region STATIC TEMP

int wiRenderer::vertexCount;
deque<wiSprite*> wiRenderer::images;
deque<wiSprite*> wiRenderer::waterRipples;

wiSPTree* wiRenderer::spTree = nullptr;
wiSPTree* wiRenderer::spTree_lights = nullptr;

Scene* wiRenderer::scene = nullptr;

vector<Object*>		wiRenderer::objectsWithTrails;
vector<wiEmittedParticle*> wiRenderer::emitterSystems;

vector<Lines*>	wiRenderer::boneLines;
vector<Lines*>	wiRenderer::linesTemp;
vector<Cube>	wiRenderer::cubes;

vector<wiTranslator*> wiRenderer::renderableTranslators;
vector<pair<XMFLOAT4X4, XMFLOAT4>> wiRenderer::renderableBoxes;

unordered_map<Camera*, wiRenderer::FrameCulling> wiRenderer::frameCullings;

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

	LoadShaders();

	cam = new Camera();
	cam->SetUp((float)GetDevice()->GetScreenWidth(), (float)GetDevice()->GetScreenHeight(), 0.1f, 800);
	refCam = new Camera();
	refCam->SetUp((float)GetDevice()->GetScreenWidth(), (float)GetDevice()->GetScreenHeight(), 0.1f, 800);
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
		GetDevice()->GetScreenWidth()
		,GetDevice()->GetScreenHeight()
		,false,FORMAT_R8G8B8A8_SNORM
		);
	imagesRTAdd.Initialize(
		GetDevice()->GetScreenWidth()
		,GetDevice()->GetScreenHeight()
		,false
		);
	imagesRT.Initialize(
		GetDevice()->GetScreenWidth()
		,GetDevice()->GetScreenHeight()
		,false
		);

	SetShadowProps2D(SHADOWRES_2D, SHADOWCOUNT_2D, SOFTSHADOWQUALITY_2D);
	SetShadowPropsCube(SHADOWRES_CUBE, SHADOWCOUNT_CUBE);

	GetDevice()->LOCK();
	BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
	UpdateWorldCB(GRAPHICSTHREAD_IMMEDIATE);
	GetDevice()->UNLOCK();
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

	if (physicsEngine) physicsEngine->CleanUp();

	SAFE_DELETE(graphicsDevice);
}
void wiRenderer::CleanUpStaticTemp()
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
				return l->lightArray_index;
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
void iterateSPTree2(wiSPTree::Node* n, vector<Cube>& cubes, const XMFLOAT4A& col);
void iterateSPTree(wiSPTree::Node* n, vector<Cube>& cubes, const XMFLOAT4A& col){
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
void iterateSPTree2(wiSPTree::Node* n, vector<Cube>& cubes, const XMFLOAT4A& col){
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


void wiRenderer::LoadBuffers()
{
	for (int i = 0; i < CBTYPE_LAST; ++i)
	{
		constantBuffers[i] = new GPUBuffer;
	}

    GPUBufferDesc bd;
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

	bd.ByteWidth = sizeof(MaterialCB);
	GetDevice()->CreateBuffer(&bd, nullptr, constantBuffers[CBTYPE_MATERIAL]);

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

	bd.Usage = USAGE_DYNAMIC;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;

	bd.ByteWidth = sizeof(ShaderBoneType) * 100;
	bd.BindFlags = BIND_SHADER_RESOURCE;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(ShaderBoneType);
	GetDevice()->CreateBuffer(&bd, nullptr, resourceBuffers[RBTYPE_BONE]);

	bd.ByteWidth = sizeof(LightArrayType) * MAX_LIGHTS;
	bd.BindFlags = BIND_SHADER_RESOURCE;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(LightArrayType);
	GetDevice()->CreateBuffer(&bd, nullptr, resourceBuffers[RBTYPE_LIGHTARRAY]);

	SAFE_DELETE(resourceBuffers[RBTYPE_VOXELSCENE]); // lazy init on request
}

void wiRenderer::LoadShaders()
{
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "MATI", 0, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI", 1, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI", 2, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR_DITHER", 0, FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS10.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr){
			vertexShaders[VSTYPE_OBJECT10] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_EFFECT] = vsinfo->vertexLayout;
		}
	}

	{

		VertexLayoutDesc oslayout[] =
		{
			{ "POSITION", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(oslayout);


		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "sOVS.cso", wiResourceManager::VERTEXSHADER, oslayout, numElements));
		if (vsinfo != nullptr){
			vertexShaders[VSTYPE_STREAMOUT] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_STREAMOUT] = vsinfo->vertexLayout;
		}
	}

	{
		StreamOutDeclaration pDecl[] =
		{
			// semantic name, semantic index, start component, component count, output slot
			{ 0, "SV_POSITION", 0, 0, 4, 0 },   // output all components of position
			{ 0, "NORMAL", 0, 0, 4, 0 },     // output the first 3 of the normal
			{ 0, "TEXCOORD", 0, 0, 4, 0 },     // output the first 2 texture coordinates
			{ 0, "TEXCOORD", 1, 0, 4, 0 },     // output the first 2 texture coordinates
		};

		geometryShaders[GSTYPE_STREAMOUT] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "sOGS.cso", wiResourceManager::GEOMETRYSHADER, nullptr, 4, pDecl));
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



	vertexShaders[VSTYPE_OBJECT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
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
	vertexShaders[VSTYPE_SKY] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_SHADOW] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_SHADOWCUBEMAPRENDER] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_WATER] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "waterVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOXELIZER] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_voxelizer.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOXEL] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;


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

	pixelShaders[PSTYPE_SIMPLEST] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_simplest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_BLACKOUT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_blackout.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_TEXTUREONLY] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_textureonly.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ALPHATESTONLY] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_alphatestonly.cso", wiResourceManager::PIXELSHADER));
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
	pixelShaders[PSTYPE_SHADOW] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_TRAIL] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "trailPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOXELIZER] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_voxelizer.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOXEL] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelPS.cso", wiResourceManager::PIXELSHADER));


	geometryShaders[GSTYPE_ENVMAP] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_ENVMAP_SKY] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMap_skyGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_SHADOWCUBEMAPRENDER] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_VOXELIZER] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectGS_voxelizer.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_VOXEL] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelGS.cso", wiResourceManager::GEOMETRYSHADER));


	computeShaders[CSTYPE_LUMINANCE_PASS1] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "luminancePass1CS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_LUMINANCE_PASS2] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "luminancePass2CS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEFRUSTUMS] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "tileFrustumsCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEDLIGHTCULLING] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightCullingCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_TILEDLIGHTCULLING_DEBUG] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "lightCullingCS_DEBUG.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_RESOLVEMSAADEPTHSTENCIL] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "resolveMSAADepthStencilCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELSCENECOPYCLEAR] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelSceneCopyClearCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELSCENECOPYCLEAR_TEMPORALSMOOTHING] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelSceneCopyClear_TemporalSmoothing.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELRADIANCESECONDARYBOUNCE] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelRadianceSecondaryBounceCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_VOXELCLEARONLYNORMAL] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "voxelClearOnlyNormalCS.cso", wiResourceManager::COMPUTESHADER));
	computeShaders[CSTYPE_GENERATEMIPCHAIN3D_SIMPLEFILTER] = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "generateMIPChain3D_SimpleFilterCS.cso", wiResourceManager::COMPUTESHADER));
	

	hullShaders[HSTYPE_OBJECT] = static_cast<HullShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectHS.cso", wiResourceManager::HULLSHADER));


	domainShaders[DSTYPE_OBJECT] = static_cast<DomainShader*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectDS.cso", wiResourceManager::DOMAINSHADER));



}

void wiRenderer::ReloadShaders(const string& path)
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
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_ANISO_WRAP]);
	
	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_ANISO_MIRROR]);

	ZeroMemory( &samplerDesc, sizeof(SamplerDesc) );
	samplerDesc.Filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = COMPARISON_LESS_EQUAL;
	GetDevice()->CreateSamplerState(&samplerDesc, samplers[SSLOT_CMP_DEPTH]);


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
	rs.SlopeScaledDepthBias = 4.0f;
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
	rs.SlopeScaledDepthBias = 5.0f;
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
	dsd.DepthFunc = COMPARISON_LESS_EQUAL;

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


	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthEnable = false;
	dsd.DepthFunc = COMPARISON_GREATER;
	dsd.StencilEnable = true;
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
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_DIRLIGHT]);


	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthEnable = true;
	dsd.DepthFunc = COMPARISON_GREATER;
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

	
	dsd.DepthEnable = false;
	dsd.StencilEnable = true;
	dsd.DepthFunc = COMPARISON_LESS_EQUAL;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;
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
	dsd.DepthFunc = COMPARISON_LESS_EQUAL;
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
	dsd.DepthFunc = COMPARISON_LESS;
	GetDevice()->CreateDepthStencilState(&dsd, depthStencils[DSSTYPE_HAIRALPHACOMPOSITION]);


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
	GetDevice()->CreateBlendState(&bd,blendStates[BSTYPE_OPAQUE]);

	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.AlphaToCoverageEnable=false;
	GetDevice()->CreateBlendState(&bd,blendStates[BSTYPE_TRANSPARENT]);


	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_MAX;
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
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_MAX;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN | COLOR_WRITE_ENABLE_BLUE; // alpha is not written by deferred lights!
	bd.IndependentBlendEnable=false,
	bd.AlphaToCoverageEnable=false;
	GetDevice()->CreateBlendState(&bd,blendStates[BSTYPE_DEFERREDLIGHT]);
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

	GetDevice()->BindConstantBufferPS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), threadID);

	GetDevice()->BindConstantBufferVS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	GetDevice()->BindConstantBufferPS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	GetDevice()->BindConstantBufferGS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	GetDevice()->BindConstantBufferDS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	GetDevice()->BindConstantBufferHS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	GetDevice()->BindConstantBufferCS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);

	GetDevice()->BindConstantBufferVS(constantBuffers[CBTYPE_API], CB_GETBINDSLOT(APICB), threadID);
	GetDevice()->BindConstantBufferPS(constantBuffers[CBTYPE_API], CB_GETBINDSLOT(APICB), threadID);

	GetDevice()->BindResourcePS(resourceBuffers[RBTYPE_LIGHTARRAY], STRUCTUREDBUFFER_GETBINDSLOT(LightArrayType), threadID);
	GetDevice()->BindResourceCS(resourceBuffers[RBTYPE_LIGHTARRAY], STRUCTUREDBUFFER_GETBINDSLOT(LightArrayType), threadID);
}
void wiRenderer::RebindPersistentState(GRAPHICSTHREAD threadID)
{
	BindPersistentState(threadID);

	wiImage::BindPersistentState(threadID);
}

Transform* wiRenderer::getTransformByName(const string& get)
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
Armature* wiRenderer::getArmatureByName(const string& get)
{
	for (Model* model : GetScene().models)
	{
		for (Armature* armature : model->armatures)
			if (!armature->name.compare(get))
				return armature;
	}
	return nullptr;
}
int wiRenderer::getActionByName(Armature* armature, const string& get)
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
int wiRenderer::getBoneByName(Armature* armature, const string& get)
{
	for (unsigned int j = 0; j<armature->boneCollection.size(); j++)
		if(!armature->boneCollection[j]->name.compare(get))
			return j;
	return (-1);
}
Material* wiRenderer::getMaterialByName(const string& get)
{
	for (Model* model : GetScene().models)
	{
		MaterialCollection::iterator iter = model->materials.find(get);
		if (iter != model->materials.end())
			return iter->second;
	}
	return NULL;
}
HitSphere* wiRenderer::getSphereByName(const string& get){
	for(HitSphere* hs : spheres)
		if(!hs->name.compare(get))
			return hs;
	return nullptr;
}
Object* wiRenderer::getObjectByName(const string& name)
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
Light* wiRenderer::getLightByName(const string& name)
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

Vertex wiRenderer::TransformVertex(const Mesh* mesh, int vertexI, const XMMATRIX& mat)
{
	return TransformVertex(mesh, mesh->vertices[vertexI], mat);
}
Vertex wiRenderer::TransformVertex(const Mesh* mesh, const SkinnedVertex& vertex, const XMMATRIX& mat)
{
	assert(mesh->hasArmature() && string(mesh->name + "has no Armature!").c_str());
	assert(!mesh->armature->boneCollection.empty() && string(mesh->armature->name + "has no bones!").c_str());

	XMVECTOR pos = XMLoadFloat4( &vertex.pos );
	XMVECTOR nor = XMLoadFloat4(&vertex.nor);
	float inWei[4] = { 
		vertex.wei.x
		,vertex.wei.y
		,vertex.wei.z
		,vertex.wei.w};
	float inBon[4] = { 
		vertex.bon.x
		,vertex.bon.y
		,vertex.bon.z
		,vertex.bon.w};
	XMMATRIX sump;
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

	XMFLOAT3 transformedP,transformedN;
	XMStoreFloat3(&transformedP, XMVector3Transform(pos, sump));
	
	XMStoreFloat3(&transformedN, XMVector3Normalize(XMVector3TransformNormal(nor, sump)));

	Vertex retV(transformedP);
	retV.nor = XMFLOAT4(transformedN.x, transformedN.y, transformedN.z, retV.nor.w);
	retV.tex = vertex.tex;
	retV.pre = XMFLOAT4(0, 0, 0, 1);

	return retV;
}
XMFLOAT3 wiRenderer::VertexVelocity(const Mesh* mesh, const int& vertexI)
{
	XMVECTOR pos = XMLoadFloat4(&mesh->vertices[vertexI].pos);
	float inWei[4] = { mesh->vertices[vertexI].wei.x
		,mesh->vertices[vertexI].wei.y
		,mesh->vertices[vertexI].wei.z
		,mesh->vertices[vertexI].wei.w };
	float inBon[4] = { mesh->vertices[vertexI].bon.x
		,mesh->vertices[vertexI].bon.y
		,mesh->vertices[vertexI].bon.z
		,mesh->vertices[vertexI].bon.w };
	XMMATRIX sump;
	XMMATRIX sumpPrev;
	if (inWei[0] || inWei[1] || inWei[2] || inWei[3]) 
	{
		sump = sumpPrev = XMMATRIX(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		for (unsigned int i = 0; i < 4; i++)
		{
			sump += XMLoadFloat4x4(&mesh->armature->boneCollection[int(inBon[i])]->boneRelativity) * inWei[i];
			sumpPrev += XMLoadFloat4x4(&mesh->armature->boneCollection[int(inBon[i])]->boneRelativityPrev) * inWei[i];
		}
	}
	else
	{
		sump = sumpPrev = XMMatrixIdentity();
	}

	XMFLOAT3 velocity;
	XMStoreFloat3(&velocity, GetGameSpeed()*XMVectorSubtract(XMVector3Transform(pos, sump), XMVector3Transform(pos, sumpPrev)));
	return velocity;
}
void wiRenderer::Update()
{
	cam->UpdateTransform();

	objectsWithTrails.clear();
	emitterSystems.clear();


	GetScene().Update();

}
void wiRenderer::UpdatePerFrameData()
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

			culling.culledRenderer.clear();
			culling.culledRenderer_opaque.clear();
			culling.culledRenderer_transparent.clear();
			culling.culledHairParticleSystems.clear();
			culling.culledLights.clear();
			culling.culledLight_count = 0;
			culling.culledEmittedParticleSystems.clear();
			culling.culledDecals.clear();

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
				frustum.ConstructFrustum(min(camera->zFarP, GetScene().worldInfo.fogSEH.y), camera->Projection, camera->View);

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

				// We wouldn't have to sort, but we need unique lights and that only works with sorted forward_list!
				spTree_lights->Sort(camera->translation, culling.culledLights, wiSPTree::SortType::SP_TREE_SORT_BACK_TO_FRONT);

				int i = (int)x.second.culledDecals.size(); // Index the light array after the decals
				int shadowCounter_2D = 0;
				int shadowCounter_Cube = 0;
				for (auto& c : culling.culledLights)
				{
					culling.culledLight_count++;
					Light* l = (Light*)c;
					l->lightArray_index = i;

					// Link shadowmaps to lights till there are free slots

					l->shadowMap_index = -1;

					if (l->shadow)
					{
						switch (l->GetType())
						{
						case Light::DIRECTIONAL:
							if (shadowCounter_2D < SHADOWCOUNT_2D)
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
			for (auto& x : emitterSystems)
			{
				if (camera->frustum.CheckBox(*x->bounding_box))
				{
					culling.culledEmittedParticleSystems.push_back(x);
				}
			}
			std::sort(culling.culledEmittedParticleSystems.begin(), culling.culledEmittedParticleSystems.end(), [&](const wiEmittedParticle* a, const wiEmittedParticle* b) {
				return wiMath::DistanceSquared(camera->translation, a->bounding_box->getCenter()) > wiMath::DistanceSquared(camera->translation,b->bounding_box->getCenter());
			});
		}
	}
	wiProfiler::GetInstance().EndRange(); // SPTree Culling

	refCam->Reflect(cam, waterPlane.getXMFLOAT4());

	UpdateBoneLines();
	UpdateCubes();

	renderTime = (float)((wiTimer::TotalTime()) / 1000.0 * GameSpeed);
}
void wiRenderer::UpdateRenderData(GRAPHICSTHREAD threadID)
{
	UpdateWorldCB(threadID); // only commits when parameters are changed
	UpdateFrameCB(threadID);
	
	// Skinning:
	wiProfiler::GetInstance().BeginRange("Skinning", wiProfiler::DOMAIN_GPU, threadID);
	GetDevice()->EventBegin("Skinning", threadID);
	{
		bool streamOutSetUp = false;


		for (Model* model : GetScene().models)
		{
			for (MeshCollection::iterator iter = model->meshes.begin(); iter != model->meshes.end(); ++iter)
			{
				Mesh* mesh = iter->second;

				if (mesh->hasArmature() && !mesh->softBody && mesh->renderable && !mesh->vertices.empty()
					&& mesh->streamoutBuffer.IsValid() && mesh->vertexBuffer.IsValid())
				{
#ifdef USE_GPU_SKINNING

					if (!streamOutSetUp)
					{
						streamOutSetUp = true;
						GetDevice()->BindPrimitiveTopology(POINTLIST, threadID);
						GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_STREAMOUT], threadID);
						GetDevice()->BindVS(vertexShaders[VSTYPE_STREAMOUT], threadID);
						GetDevice()->BindGS(geometryShaders[GSTYPE_STREAMOUT], threadID);
						GetDevice()->BindPS(nullptr, threadID);
						GetDevice()->BindResourceVS(resourceBuffers[RBTYPE_BONE], STRUCTUREDBUFFER_GETBINDSLOT(ShaderBoneType), threadID);
					}

					// Upload bones for skinning to shader
					// Are statics thread safe here? - this is most likely only run by one thread at a time so I think in this case, yes
					static unsigned int maxBoneCount = resourceBuffers[RBTYPE_BONE]->GetDesc().ByteWidth / sizeof(ShaderBoneType); 
					static ShaderBoneType *bonebuf = nullptr;
					if (bonebuf == nullptr)
					{
						bonebuf = (ShaderBoneType*)_mm_malloc(sizeof(ShaderBoneType)*maxBoneCount, 16);
					}
					if (mesh->armature->boneCollection.size() > maxBoneCount)
					{
						maxBoneCount = (int)mesh->armature->boneCollection.size() * 2;
						if (bonebuf != nullptr)
						{
							_mm_free(bonebuf);
						}
						bonebuf = (ShaderBoneType*)_mm_malloc(sizeof(ShaderBoneType)*maxBoneCount, 16);
					}
					for (unsigned int k = 0; k < mesh->armature->boneCollection.size(); k++) {
						bonebuf[k].pose = XMMatrixTranspose(XMLoadFloat4x4(&mesh->armature->boneCollection[k]->boneRelativity));
						bonebuf[k].prev = XMMatrixTranspose(XMLoadFloat4x4(&mesh->armature->boneCollection[k]->boneRelativityPrev));
					}
					GetDevice()->UpdateBuffer(resourceBuffers[RBTYPE_BONE], bonebuf, threadID, (int)(sizeof(ShaderBoneType) * mesh->armature->boneCollection.size()));

					// Do the skinning
					GetDevice()->BindVertexBuffer(&mesh->vertexBuffer, 0, sizeof(SkinnedVertex), threadID);
					GetDevice()->BindStreamOutTarget(&mesh->streamoutBuffer, threadID);
					GetDevice()->Draw((int)mesh->vertices.size(), threadID);

#else
					// Doing skinning on the CPU
					for (int vi = 0; vi < mesh->skinnedVertices.size(); ++vi)
						mesh->skinnedVertices[vi] = TransformVertex(mesh, vi);
#endif
				}

				// Upload CPU skinned vertex buffer (Soft body VB)
#ifdef USE_GPU_SKINNING
				// If GPU skinning is enabled, we only skin soft bodies on the CPU
				if (mesh->softBody)
#else
				// Upload skinned vertex buffer to GPU
				if (mesh->softBody || mesh->hasArmature())
#endif
				{
					GetDevice()->UpdateBuffer(&mesh->vertexBuffer, mesh->vertices_Complete.data(), threadID, (int)(sizeof(Vertex)*mesh->vertices_Complete.size()));
				}
			}
		}

#ifdef USE_GPU_SKINNING
		if (streamOutSetUp)
		{
			// Unload skinning shader
			GetDevice()->BindGS(nullptr, threadID);
			GetDevice()->BindVS(nullptr, threadID);
			GetDevice()->BindStreamOutTarget(nullptr, threadID);
		}
#endif


	}
	GetDevice()->EventEnd(threadID);
	wiProfiler::GetInstance().EndRange(threadID); // skinning


	RefreshEnvProbes(threadID);

	// Environment probe setup:
	{
		Texture2D* envMaps[] = { enviroMap, enviroMap };
		XMFLOAT3 envMapPositions[] = { XMFLOAT3(0,0,0),XMFLOAT3(0,0,0) };
		GetScene().environmentProbes.sort(
			[&](EnvironmentProbe* a, EnvironmentProbe* b) {
			return wiMath::DistanceSquared(a->translation, getCamera()->translation) < wiMath::DistanceSquared(b->translation, getCamera()->translation);
		}
		);
		int envProbeInd = 0;
		for (auto& x : GetScene().environmentProbes)
		{
			envMaps[envProbeInd] = x->cubeMap.GetTexture();
			envMapPositions[envProbeInd] = x->translation;
			envProbeInd++;
			if (envProbeInd >= ARRAYSIZE(envMaps))
				break;
		}

		MiscCB envProbeCB;
		envProbeCB.mTransform.r[0] = XMLoadFloat3(&envMapPositions[0]);
		envProbeCB.mTransform.r[1] = XMLoadFloat3(&envMapPositions[1]);
		envProbeCB.mTransform = XMMatrixTranspose(envProbeCB.mTransform);
		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &envProbeCB, threadID);
		GetDevice()->BindResourcePS(envMaps[0], TEXSLOT_ENV0, threadID);
		GetDevice()->BindResourcePS(envMaps[1], TEXSLOT_ENV1, threadID);
	}

	
	const FrameCulling& mainCameraCulling = frameCullings[getCamera()];

	ManageDecalAtlas(threadID);
	
	// Fill Light Array with lights + decals in the frustum
	{
		const CulledList& culledLights = mainCameraCulling.culledLights;

		static LightArrayType* lightArray = (LightArrayType*)_mm_malloc(sizeof(LightArrayType)*MAX_LIGHTS, 16);
		ZeroMemory(lightArray, sizeof(lightArray));

		XMMATRIX viewMatrix = cam->GetView();

		UINT lightCounter = 0;
		for (Decal* decal : mainCameraCulling.culledDecals)
		{
			if (lightCounter == MAX_LIGHTS)
			{
				assert(0); // too much lights!
				lightCounter--;
				break;
			}
			XMStoreFloat3(&lightArray[lightCounter].posVS, XMVector3TransformCoord(XMLoadFloat3(&decal->translation), viewMatrix));
			lightArray[lightCounter].distance = max(decal->scale.x, max(decal->scale.y, decal->scale.z)) * 2;
			lightArray[lightCounter].shadowMatrix[0] = XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&decal->world)));
			lightArray[lightCounter].texMulAdd = decal->atlasMulAdd;
			lightArray[lightCounter].col = XMFLOAT4(decal->color.x, decal->color.y, decal->color.z, decal->GetOpacity());
			lightArray[lightCounter].energy = decal->emissive;
			lightArray[lightCounter].type = 100;

			lightCounter++;
		}
		for (Cullable* c : culledLights)
		{
			if (lightCounter == MAX_LIGHTS)
			{
				assert(0); // too much lights!
				lightCounter--;
				break;
			}

			Light* l = (Light*)c;

			lightArray[lightCounter].posWS = l->translation;
			XMStoreFloat3(&lightArray[lightCounter].posVS, XMVector3TransformCoord(XMLoadFloat3(&lightArray[lightCounter].posWS), viewMatrix));
			lightArray[lightCounter].distance = l->enerDis.y;
			lightArray[lightCounter].col = l->color;
			lightArray[lightCounter].energy = l->enerDis.x;
			lightArray[lightCounter].type = l->GetType();
			lightArray[lightCounter].shadowBias = l->shadowBias;
			lightArray[lightCounter].shadowMap_index = l->shadowMap_index;
			switch (l->GetType())
			{
			case Light::DIRECTIONAL:
			{
				lightArray[lightCounter].directionWS = l->GetDirection();
				for (unsigned int shmap = 0; shmap < min(l->shadowCam_dirLight.size(), ARRAYSIZE(lightArray[lightCounter].shadowMatrix)); ++shmap) {
					lightArray[lightCounter].shadowMatrix[shmap] = l->shadowCam_dirLight[shmap].getVP();
				}
				lightArray[lightCounter].shadowKernel = 1.0f / SHADOWRES_2D;
			}
			break;
			case Light::SPOT:
			{
				lightArray[lightCounter].coneAngle = (l->enerDis.z * 0.5f);
				lightArray[lightCounter].coneAngleCos = cosf(lightArray[lightCounter].coneAngle);
				lightArray[lightCounter].directionWS = l->GetDirection();
				XMStoreFloat3(&lightArray[lightCounter].directionVS, XMVector3TransformNormal(XMLoadFloat3(&lightArray[lightCounter].directionWS), viewMatrix));
				if (l->shadow && l->shadowMap_index >= 0 && !l->shadowCam_spotLight.empty())
				{
					lightArray[lightCounter].shadowMatrix[0] = l->shadowCam_spotLight[0].getVP();
				}
				lightArray[lightCounter].shadowKernel = 1.0f / SHADOWRES_2D;
			}
			break;
			case Light::POINT:
			{
				lightArray[lightCounter].shadowKernel = 1.0f / SHADOWRES_CUBE;
			}
			break;
			case Light::SPHERE:
			case Light::DISC:
			case Light::RECTANGLE:
			case Light::TUBE:
			{
				XMMATRIX lightMat = XMLoadFloat4x4(&l->world);
				XMStoreFloat3(&lightArray[lightCounter].directionWS, XMVector3TransformNormal(XMVectorSet(-1, 0, 0, 0), lightMat)); // left dir
				XMStoreFloat3(&lightArray[lightCounter].directionVS, XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), lightMat)); // up dir
				XMStoreFloat3(&lightArray[lightCounter].posVS, XMVector3TransformNormal(XMVectorSet(0, 0, -1, 0), lightMat)); // front dir
				lightArray[lightCounter].texMulAdd = XMFLOAT4(l->radius, l->width, l->height, 0);
			}
			break;
			}

			lightCounter++;
		}
		GetDevice()->UpdateBuffer(resourceBuffers[RBTYPE_LIGHTARRAY], lightArray, threadID, (int)(sizeof(LightArrayType)*lightCounter));
	}

	// Update visible particlesystem vertices:
	for (auto& x : mainCameraCulling.culledEmittedParticleSystems)
	{
		x->UpdateRenderData(threadID);
	}
}
void wiRenderer::OcclusionCulling_Render(GRAPHICSTHREAD threadID)
{
	if (!GetOcclusionCullingEnabled())
	{
		return;
	}

	const FrameCulling& culling = frameCullings[getCamera()];
	const CulledCollection& culledRenderer = culling.culledRenderer;

	wiProfiler::GetInstance().BeginRange("Occlusion Culling Render", wiProfiler::DOMAIN_GPU, threadID);

	if (!culledRenderer.empty())
	{
		GetDevice()->EventBegin("Occlusion Culling Render");

		GetDevice()->BindRasterizerState(rasterizers[RSTYPE_OCCLUDEE], threadID);
		GetDevice()->BindBlendState(blendStates[BSTYPE_COLORWRITEDISABLE], threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_DEFAULT, threadID);
		GetDevice()->BindVertexLayout(nullptr, threadID);
		GetDevice()->BindVertexBuffer(nullptr, 0, 0, threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_CUBE], threadID);
		GetDevice()->BindPS(nullptr, threadID);
		GetDevice()->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLESTRIP);

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
				GPUQuery& query = instance->occlusionQuery;

				if (instance->bounds.intersects(getCamera()->translation))
				{
					// if the camera is inside the bounding box, then the object is most likely visible, so skip occlusion query and mark it as visible:
					query.result_passed = true;
					instance->skipOcclusionQuery = true;
				}
				else
				{
					if (instance->skipOcclusionQuery)
					{
						instance->skipOcclusionQuery = false;
						continue;
					}
					// previous frame view*projection because these are drawn against the previous depth buffer:
					cb.mTransform = XMMatrixTranspose(instance->GetOBB()*prevFrameCam->GetViewProjection()); 
					GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &cb, threadID);

					// render bounding box to later read the occlusion status
					GetDevice()->QueryBegin(&query, threadID);
					GetDevice()->Draw(14, threadID);
					GetDevice()->QueryEnd(&query, threadID);
				}
			}
		}

		GetDevice()->EventEnd();
	}

	wiProfiler::GetInstance().EndRange(threadID); // Occlusion Culling Render
}
void wiRenderer::OcclusionCulling_Read()
{
	if (!GetOcclusionCullingEnabled())
	{
		return;
	}

	wiProfiler::GetInstance().BeginRange("Occlusion Culling Read", wiProfiler::DOMAIN_CPU);

	const FrameCulling& culling = frameCullings[getCamera()];
	const CulledCollection& culledRenderer = culling.culledRenderer;

	if (!culledRenderer.empty())
	{
		GetDevice()->EventBegin("Occlusion Culling Read");

		for (CulledCollection::const_iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter)
		{
			Mesh* mesh = iter->first;
			if (!mesh->renderable)
			{
				continue;
			}

			const CulledObjectList& visibleInstances = iter->second;

			for (Object* instance : visibleInstances)
			{
				GPUQuery& query = instance->occlusionQuery;
				while (!GetDevice()->QueryRead(&query, GRAPHICSTHREAD_IMMEDIATE)) {}
			}
		}

		GetDevice()->EventEnd();
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
void wiRenderer::PutWaterRipple(const string& image, const XMFLOAT3& pos)
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
	GetDevice()->EventBegin("Water Ripples");
	for(wiSprite* i:waterRipples){
		i->DrawNormal(threadID);
	}
	GetDevice()->EventEnd();
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

			GetDevice()->BindVertexBuffer(&boneLines[i]->vertexBuffer, 0, sizeof(XMFLOAT4) + sizeof(XMFLOAT4), threadID);
			GetDevice()->Draw(2, threadID);
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

		GetDevice()->BindVertexBuffer(&linesTemp[i]->vertexBuffer, 0, sizeof(XMFLOAT4) + sizeof(XMFLOAT4), threadID);
		GetDevice()->Draw(2, threadID);
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
		
		GetDevice()->BindVertexBuffer(&Cube::vertexBuffer,0, sizeof(XMFLOAT4) + sizeof(XMFLOAT4),threadID);
		GetDevice()->BindIndexBuffer(&Cube::indexBuffer,threadID);

		MiscCB sb;
		for (auto& x : cubes)
		{
			sb.mTransform =XMMatrixTranspose(XMLoadFloat4x4(&x.desc.transform)*camera->GetViewProjection());
			sb.mColor=x.desc.color;

			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

			GetDevice()->DrawIndexed(24,threadID);
		}
		cubes.clear();

		for (auto& x : renderableBoxes)
		{
			sb.mTransform = XMMatrixTranspose(XMLoadFloat4x4(&x.first)*camera->GetViewProjection());
			sb.mColor = x.second;

			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

			GetDevice()->DrawIndexed(24, threadID);
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

			GetDevice()->BindVertexBuffer(wiTranslator::vertexBuffer_Plane, 0, sizeof(XMFLOAT4) + sizeof(XMFLOAT4), threadID);
			GetDevice()->BindRasterizerState(rasterizers[RSTYPE_DOUBLESIDED], threadID);
			GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
			GetDevice()->BindBlendState(blendStates[BSTYPE_ADDITIVE], threadID);

			// xy
			sb.mTransform = matX;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_XY ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.2f, 0.2f, 0, 0.2f);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Plane, threadID);

			// xz
			sb.mTransform = matZ;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_XZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.2f, 0.2f, 0, 0.2f);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Plane, threadID);

			// yz
			sb.mTransform = matY;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_YZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.2f, 0.2f, 0, 0.2f);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Plane, threadID);

			// Lines:

			GetDevice()->BindVertexBuffer(wiTranslator::vertexBuffer_Axis, 0, sizeof(XMFLOAT4) + sizeof(XMFLOAT4), threadID);
			GetDevice()->BindRasterizerState(rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH], threadID);
			GetDevice()->BindPrimitiveTopology(LINELIST, threadID);
			GetDevice()->BindBlendState(blendStates[BSTYPE_TRANSPARENT], threadID);

			// x
			sb.mTransform = matX;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_X ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(1, 0, 0, 1);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Axis, threadID);

			// y
			sb.mTransform = matY;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_Y ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0, 1, 0, 1);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Axis, threadID);

			// z
			sb.mTransform = matZ;
			sb.mColor = x->state == wiTranslator::TRANSLATOR_Z ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0, 0, 1, 1);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Axis, threadID);

			// Origin:
			sb.mTransform = XMMatrixTranspose(mat);
			sb.mColor = x->state == wiTranslator::TRANSLATOR_XYZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.5f, 0.5f, 0.5f, 1);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);
			GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
			GetDevice()->BindRasterizerState(rasterizers[RSTYPE_FRONT], threadID);
			GetDevice()->BindVertexBuffer(wiTranslator::vertexBuffer_Origin, 0, sizeof(XMFLOAT4) + sizeof(XMFLOAT4), threadID);
			GetDevice()->Draw(wiTranslator::vertexCount_Origin, threadID);
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

		GetDevice()->BindVertexBuffer(nullptr, 0, 0, threadID);
		GetDevice()->BindIndexBuffer(nullptr, threadID);

		MiscCB sb;
		for (auto& x : GetScene().environmentProbes)
		{
			sb.mTransform = XMMatrixTranspose(x->getMatrix());
			sb.mColor = XMFLOAT4(1, 1, 1, 1);
			GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

			GetDevice()->BindResourcePS(x->cubeMap.GetTexture(), TEXSLOT_ENV0, threadID);

			GetDevice()->Draw(2880, threadID);
		}

		GetDevice()->EventEnd(threadID);
	}
}
void wiRenderer::DrawDebugGridHelper(Camera* camera, GRAPHICSTHREAD threadID)
{
	if(gridHelper){
		GetDevice()->EventBegin("GridHelper", threadID);

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

		GetDevice()->BindVertexBuffer(grid, 0, sizeof(XMFLOAT4) + sizeof(XMFLOAT4), threadID);
		GetDevice()->Draw(gridVertexCount, threadID);


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
		GetDevice()->BindVertexBuffer(nullptr, 0, 0, threadID);
		GetDevice()->BindIndexBuffer(nullptr, threadID);



		MiscCB sb;
		sb.mTransform = XMMatrixTranspose(XMMatrixTranslationFromVector(XMLoadFloat3(&voxelSceneData.center)) * camera->GetViewProjection());
		sb.mColor = XMFLOAT4(1, 1, 1, 1);

		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &sb, threadID);

		GetDevice()->Draw(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res, threadID);

		GetDevice()->BindGS(nullptr, threadID);

		GetDevice()->EventEnd(threadID);
	}
}

void wiRenderer::DrawSoftParticles(Camera* camera, GRAPHICSTHREAD threadID, bool dark)
{
	FrameCulling& culling = frameCullings[camera];

	for (wiEmittedParticle* e : culling.culledEmittedParticleSystems)
	{
		e->DrawNonPremul(threadID, dark);
	}
}
void wiRenderer::DrawSoftPremulParticles(Camera* camera, GRAPHICSTHREAD threadID, bool dark)
{
	FrameCulling& culling = frameCullings[camera];

	for (wiEmittedParticle* e : culling.culledEmittedParticleSystems)
	{
		e->DrawPremul(threadID, dark);
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
		if(o->trailBuff.IsValid() && o->trail.size()>=4){

			GetDevice()->BindResourcePS(o->trailDistortTex, TEXSLOT_ONDEMAND1, threadID);
			GetDevice()->BindResourcePS(o->trailTex, TEXSLOT_ONDEMAND2, threadID);

			vector<RibbonVertex> trails;

			int bounds = (int)o->trail.size();
			trails.reserve(bounds * 10);
			int req = bounds-3;
			for(int k=0;k<req;k+=2)
			{
				static const float trailres = 10.f;
				for(float r=0.0f;r<=1.0f;r+=1.0f/trailres)
				{
					XMVECTOR point0 = XMVectorCatmullRom(
						XMLoadFloat3( &o->trail[k?(k-2):0].pos )
						,XMLoadFloat3( &o->trail[k].pos )
						,XMLoadFloat3( &o->trail[k+2].pos )
						,XMLoadFloat3( &o->trail[k+6<bounds?(k+6):(bounds-2)].pos )
						,r
					),
					point1 = XMVectorCatmullRom(
						XMLoadFloat3( &o->trail[k?(k-1):1].pos )
						,XMLoadFloat3( &o->trail[k+1].pos )
						,XMLoadFloat3( &o->trail[k+3].pos )
						,XMLoadFloat3( &o->trail[k+5<bounds?(k+5):(bounds-1)].pos )
						,r
					);
					XMFLOAT3 xpoint0,xpoint1;
					XMStoreFloat3(&xpoint0,point0);
					XMStoreFloat3(&xpoint1,point1);
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
			if(!trails.empty()){
				GetDevice()->UpdateBuffer(&o->trailBuff,trails.data(),threadID,(int)(sizeof(RibbonVertex)*trails.size()));

				GetDevice()->BindVertexBuffer(&o->trailBuff,0,sizeof(RibbonVertex),threadID);
				GetDevice()->Draw((int)trails.size(),threadID);

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
	GetDevice()->BindVertexBuffer(nullptr, 0, 0, threadID);
	GetDevice()->BindIndexBuffer(nullptr, threadID);

	// Environmental light (envmap + voxelGI) is always drawn
	{
		GetDevice()->BindBlendState(blendStates[BSTYPE_ADDITIVE], threadID);
		GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DIRLIGHT], STENCILREF_DEFAULT, threadID);
		GetDevice()->BindVS(vertexShaders[VSTYPE_DIRLIGHT], threadID); // just full screen triangle so we can use it
		GetDevice()->BindPS(pixelShaders[PSTYPE_ENVIRONMENTALLIGHT], threadID);
		GetDevice()->Draw(3, threadID); // full screen triangle
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
			if (l->GetType() != type)
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
					miscCb.mColor.x = (float)l->lightArray_index;
					GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &miscCb, threadID);

					GetDevice()->Draw(3, threadID); // full screen triangle
				}
				break;
			case Light::POINT:
				{
					MiscCB miscCb;
					miscCb.mColor.x = (float)l->lightArray_index;
					float sca = l->enerDis.y + 1;
					miscCb.mTransform = XMMatrixTranspose(XMMatrixScaling(sca, sca, sca)*XMMatrixTranslation(l->translation.x, l->translation.y, l->translation.z) * camera->GetViewProjection());
					GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &miscCb, threadID);

					GetDevice()->Draw(240, threadID); // icosphere
				}
				break;
			case Light::SPOT:
				{
					MiscCB miscCb;
					miscCb.mColor.x = (float)l->lightArray_index;
					const float coneS = (const float)(l->enerDis.z / XM_PIDIV4);
					miscCb.mTransform = XMMatrixTranspose(
						XMMatrixScaling(coneS*l->enerDis.y, l->enerDis.y, coneS*l->enerDis.y)*
						XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))*
						XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation)) *
						camera->GetViewProjection()
					);
					GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MISC], &miscCb, threadID);

					GetDevice()->Draw(192, threadID); // cone
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
		GetDevice()->BindVertexLayout(nullptr);
		GetDevice()->BindVertexBuffer(nullptr, 0, 0, threadID);
		GetDevice()->BindIndexBuffer(nullptr, threadID);
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

						GetDevice()->Draw(108, threadID); // circle
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

						GetDevice()->Draw(192, threadID); // cone
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

						GetDevice()->Draw(2880, threadID); // uv-sphere
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

						GetDevice()->Draw(108, threadID); // circle
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

						GetDevice()->Draw(6, threadID); // quad
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

						GetDevice()->Draw(384, threadID); // cylinder
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
			
			XMVECTOR flarePos = XMVector3Project(POS,0.f,0.f,(float)GetDevice()->GetScreenWidth(),(float)GetDevice()->GetScreenHeight(),0.1f,1.0f,getCamera()->GetProjection(),getCamera()->GetView(),XMMatrixIdentity());

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
			GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_EFFECT], threadID);


			GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_DEFAULT, threadID);

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
				}
				break;
				default:
					break;
				}

				for (Cullable* c : culledLights)
				{
					Light* l = (Light*)c;
					if (l->GetType() != type || !l->shadow)
					{
						continue;
					}

					switch (type)
					{
					case Light::DIRECTIONAL:
					{
						if (shadowCounter_2D >= SHADOWCOUNT_2D || l->shadowMap_index < 0 || l->shadowCam_dirLight.empty())
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
									GetDevice()->ClearDepthStencil(Light::shadowMapArray_2D, CLEAR_DEPTH, 1.0f, 0, threadID, l->shadowMap_index + index);

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
						frustum.ConstructFrustum(l->shadowCam_spotLight[0].farplane, l->shadowCam_spotLight[0].Projection, l->shadowCam_spotLight[0].View);
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
								GetDevice()->ClearDepthStencil(Light::shadowMapArray_2D, CLEAR_DEPTH, 1.0f, 0, threadID, l->shadowMap_index);

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
								GetDevice()->ClearDepthStencil(Light::shadowMapArray_Cube, CLEAR_DEPTH, 1.0f, 0, threadID, l->shadowMap_index);

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


PSTYPES GetPSTYPE(SHADERTYPE shaderType, const Material* const material)
{
	PSTYPES realPS = PSTYPE_SIMPLEST;

	bool transparent = material->IsTransparent();
	bool water = material->IsWater();

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
		realPS = PSTYPE_SHADOW;
		break;
	case SHADERTYPE_SHADOWCUBE:
		realPS = PSTYPE_SHADOWCUBEMAPRENDER;
		break;
	case SHADERTYPE_ENVMAPCAPTURE:
		realPS = PSTYPE_ENVMAP;
		break;
	case SHADERTYPE_ALPHATESTONLY:
		realPS = PSTYPE_ALPHATESTONLY;
		break;
	case SHADERTYPE_VOXELIZE:
		realPS = PSTYPE_VOXELIZER;
		break;
	default:
		realPS = PSTYPE_TEXTUREONLY;
		break;
	}

	return realPS;
}
void wiRenderer::RenderMeshes(const XMFLOAT3& eye, const CulledCollection& culledRenderer, SHADERTYPE shaderType, UINT renderTypeFlags, GRAPHICSTHREAD threadID,
	bool tessellation, bool disableOcclusionCulling)
{
	if (!culledRenderer.empty())
	{
		GetDevice()->EventBegin("RenderMeshes", threadID);

		tessellation = tessellation && GetDevice()->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION);

		DSSTYPES targetDepthStencilState = DSSTYPE_DEFAULT;
		UINT targetStencilRef = STENCILREF_DEFAULT; 
		
		if (shaderType == SHADERTYPE_TILEDFORWARD && renderTypeFlags & RENDERTYPE_OPAQUE)
		{
			targetDepthStencilState = DSSTYPE_DEPTHREADEQUAL;
		}

		if (shaderType == SHADERTYPE_ALPHATESTONLY)
		{
			GetDevice()->BindBlendState(blendStates[BSTYPE_COLORWRITEDISABLE], threadID);
		}
		else if(renderTypeFlags & RENDERTYPE_OPAQUE)
		{
			GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);
		}
		else
		{
			GetDevice()->BindBlendState(blendStates[BSTYPE_TRANSPARENT], threadID);
		}

		bool easyTextureBind = shaderType == SHADERTYPE_SHADOW || shaderType == SHADERTYPE_SHADOWCUBE || shaderType == SHADERTYPE_ALPHATESTONLY || shaderType == SHADERTYPE_VOXELIZE;

		for (CulledCollection::const_iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) 
		{
			Mesh* mesh = iter->first;
			if (!mesh->renderable)
			{
				continue;
			}

			const CulledObjectList& visibleInstances = iter->second;

			float tessF = mesh->getTessellationFactor();

			if (tessellation && tessF)
				GetDevice()->BindPrimitiveTopology(PATCHLIST, threadID);
			else
				GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);

			if (shaderType == SHADERTYPE_VOXELIZE)
			{
				GetDevice()->BindVS(vertexShaders[VSTYPE_VOXELIZER], threadID);
				GetDevice()->BindGS( geometryShaders[ GSTYPE_VOXELIZER ], threadID );
			}
			else
			{
				if( shaderType == SHADERTYPE_SHADOW )
				{
					GetDevice()->BindVS( vertexShaders[ VSTYPE_SHADOW ], threadID );
				}
				else if( shaderType == SHADERTYPE_SHADOWCUBE )
				{
					GetDevice()->BindVS( vertexShaders[ VSTYPE_SHADOWCUBEMAPRENDER ], threadID );
					GetDevice()->BindGS( geometryShaders[ GSTYPE_SHADOWCUBEMAPRENDER ], threadID );
				}
				else if( shaderType == SHADERTYPE_ENVMAPCAPTURE )
				{
					GetDevice()->BindVS( vertexShaders[ VSTYPE_ENVMAP ], threadID );
					GetDevice()->BindGS( geometryShaders[ GSTYPE_ENVMAP ], threadID );
				}
				else
				{
					if( tessellation && tessF )
					{
						GetDevice()->BindVS( vertexShaders[ VSTYPE_OBJECT ], threadID );
					}
					else
					{
						GetDevice()->BindVS( vertexShaders[ VSTYPE_OBJECT10 ], threadID );
					}
				}
				GetDevice()->BindVertexLayout( vertexLayouts[ VLTYPE_EFFECT ], threadID );
			}

			if (tessellation && tessF)
			{
				TessellationCB tessCB;
				tessCB.tessellationFactors = XMFLOAT4(tessF, tessF, tessF, tessF);
				GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_TESSELLATION], &tessCB, threadID);
				GetDevice()->BindConstantBufferHS(constantBuffers[CBTYPE_TESSELLATION], CBSLOT_RENDERER_TESSELLATION, threadID);
				GetDevice()->BindHS(hullShaders[HSTYPE_OBJECT], threadID);
			}
			else
			{
				GetDevice()->BindHS(nullptr, threadID);
			}

			if (tessellation && tessF) {
				GetDevice()->BindDS(domainShaders[DSTYPE_OBJECT], threadID);
			}
			else 
			{
				GetDevice()->BindDS(nullptr, threadID);
			}


			float impostorDistance = mesh->impostorDistance;

			targetStencilRef = mesh->stencilRef;

			if (mesh->hasImpostor() && shaderType != SHADERTYPE_VOXELIZE)
			{
				int k = 0;
				for (const Object* instance : visibleInstances) 
				{
					if (instance->emitterType != Object::EmitterType::EMITTER_INVISIBLE) 
					{
						const float impostorThreshold = instance->bounds.getRadius();
						float dist = wiMath::Distance(eye, instance->bounds.getCenter());
						float dither = instance->transparency;
						dither = wiMath::SmoothStep(1.0f, dither, wiMath::Clamp((dist - impostorDistance) / impostorThreshold, 0, 1));
						if (dither > 1.0f - FLT_EPSILON)
							continue;

						if (disableOcclusionCulling || !instance->IsOccluded())
						{
							mesh->AddRenderableInstance(Instance(XMMatrixTranspose(mesh->aabb.getAsBoxMatrix()*XMLoadFloat4x4(&instance->world)), dither, instance->color), k, threadID);
							++k;
						}
					}
				}
				if (k > 0)
				{
					mesh->UpdateRenderableInstances(k, threadID);

					MaterialCB mcb;
					ZeroMemory(&mcb, sizeof(mcb));
					mcb.baseColor = XMFLOAT4(1, 1, 1, 1);
					mcb.texMulAdd = XMFLOAT4(1, 1, 0, 0);
					mcb.normalMapStrength = 1.0f;
					mcb.roughness = 1.0f;
					mcb.reflectance = 1.0f;
					mcb.metalness = 1.0f;
					UpdateMaterialCB(mcb, threadID);

					if (shaderType == SHADERTYPE_SHADOW || shaderType == SHADERTYPE_SHADOWCUBE)
					{
						GetDevice()->BindRasterizerState(rasterizers[RSTYPE_SHADOW], threadID);
					}
					else
					{
						GetDevice()->BindRasterizerState(wireRender ? rasterizers[RSTYPE_WIRE] : rasterizers[RSTYPE_FRONT], threadID);
					}

					GetDevice()->BindDepthStencilState(depthStencils[targetDepthStencilState], targetStencilRef, threadID);

					GetDevice()->BindIndexBuffer(nullptr, threadID);
					GetDevice()->BindVertexBuffer(&Mesh::impostorVB, 0, sizeof(Vertex), threadID);
					GetDevice()->BindVertexBuffer(&mesh->instanceBuffer, 1, sizeof(Instance), threadID);
					GetDevice()->BindResourcePS(mesh->impostorTarget.GetTexture(0), TEXSLOT_ONDEMAND0, threadID);
					if (!easyTextureBind)
					{
						GetDevice()->BindResourcePS(mesh->impostorTarget.GetTexture(1), TEXSLOT_ONDEMAND1, threadID);
						GetDevice()->BindResourcePS(mesh->impostorTarget.GetTexture(2), TEXSLOT_ONDEMAND2, threadID);
						GetDevice()->BindResourcePS(mesh->impostorTarget.GetTexture(3), TEXSLOT_ONDEMAND3, threadID);
						GetDevice()->BindResourcePS(mesh->impostorTarget.GetTexture(4), TEXSLOT_ONDEMAND4, threadID);
					}

					if (wireRender)
					{
						GetDevice()->BindPS(pixelShaders[PSTYPE_SIMPLEST], threadID);
					}
					else
					{
						switch (shaderType)
						{
						case SHADERTYPE_DEFERRED:
							GetDevice()->BindPS(pixelShaders[PSTYPE_OBJECT_DEFERRED_NORMALMAP], threadID);
							break;
						case SHADERTYPE_FORWARD:
							GetDevice()->BindPS(pixelShaders[PSTYPE_OBJECT_FORWARD_DIRLIGHT_NORMALMAP], threadID);
							break;
						case SHADERTYPE_TILEDFORWARD:
							GetDevice()->BindPS(pixelShaders[PSTYPE_OBJECT_TILEDFORWARD], threadID);
							break;
						case SHADERTYPE_SHADOW:
							GetDevice()->BindPS(pixelShaders[PSTYPE_SHADOW], threadID);
							break;
						case SHADERTYPE_SHADOWCUBE:
							GetDevice()->BindPS(pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER], threadID);
							break;
						default:
							GetDevice()->BindPS(pixelShaders[PSTYPE_TEXTUREONLY], threadID);
							break;
						}
					}
					GetDevice()->DrawInstanced(6 * 6, k, threadID); // 6 * 6: see Mesh::CreateImpostorVB function
				}
			}

			if (shaderType == SHADERTYPE_VOXELIZE)
			{
				GetDevice()->BindRasterizerState(rasterizers[RSTYPE_VOXELIZE], threadID);
			}
			else
			{
				if (shaderType == SHADERTYPE_SHADOW || shaderType == SHADERTYPE_SHADOWCUBE)
				{
					if (!mesh->doubleSided)
					{
						GetDevice()->BindRasterizerState(rasterizers[RSTYPE_SHADOW], threadID);
					}
					else
					{
						GetDevice()->BindRasterizerState(rasterizers[RSTYPE_SHADOW_DOUBLESIDED], threadID);
					}
				}
				else
				{
					if (!mesh->doubleSided)
					{
						GetDevice()->BindRasterizerState(wireRender ? rasterizers[RSTYPE_WIRE] : rasterizers[RSTYPE_FRONT], threadID);
					}
					else
					{
						GetDevice()->BindRasterizerState(wireRender ? rasterizers[RSTYPE_WIRE] : rasterizers[RSTYPE_DOUBLESIDED], threadID);
					}
				}
			}

			int k = 0;
			for (const Object* instance : visibleInstances) 
			{
				if (instance->emitterType != Object::EmitterType::EMITTER_INVISIBLE)
				{
					const float impostorThreshold = instance->bounds.getRadius();
					float dist = wiMath::Distance(eye, instance->bounds.getCenter());
					float dither = instance->transparency;
					if (mesh->hasImpostor())
						dither = wiMath::SmoothStep(dither, 1.0f, wiMath::Clamp((dist - impostorThreshold - impostorDistance) / impostorThreshold, 0, 1));
					if (dither > 1.0f - FLT_EPSILON)
						continue;

					if (disableOcclusionCulling || !instance->IsOccluded())
					{
						if (mesh->softBody || instance->isArmatureDeformed())
							mesh->AddRenderableInstance(Instance(XMMatrixIdentity(), dither, instance->color), k, threadID);
						else
							mesh->AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&instance->world)), dither, instance->color), k, threadID);
						++k;
					}
				}
			}
			if (k < 1)
			{
				continue;
			}

			mesh->UpdateRenderableInstances(k, threadID);

			GetDevice()->BindVertexBuffer((mesh->streamoutBuffer.IsValid() ? &mesh->streamoutBuffer : &mesh->vertexBuffer), 0, sizeof(Vertex), threadID);
			GetDevice()->BindVertexBuffer(&mesh->instanceBuffer, 1, sizeof(Instance), threadID);

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
					GetDevice()->BindIndexBuffer(&subset.indexBuffer, threadID);

					if (material->shadeless)
					{
						targetStencilRef = STENCILREF_SHADELESS;
					}
					if (material->subsurfaceScattering > 0)
					{
						targetStencilRef = STENCILREF_SKIN;
					}
					GetDevice()->BindDepthStencilState(depthStencils[targetDepthStencilState], targetStencilRef, threadID);

					UpdateMaterialCB(MaterialCB(*material), threadID);

					if (!wireRender)
					{
						GetDevice()->BindResourcePS(material->GetBaseColorMap(), TEXSLOT_ONDEMAND0, threadID);
						if (!easyTextureBind)
						{
							GetDevice()->BindResourcePS(material->GetNormalMap(), TEXSLOT_ONDEMAND1, threadID);
							GetDevice()->BindResourcePS(material->GetRoughnessMap(), TEXSLOT_ONDEMAND2, threadID);
							GetDevice()->BindResourcePS(material->GetReflectanceMap(), TEXSLOT_ONDEMAND3, threadID);
							GetDevice()->BindResourcePS(material->GetMetalnessMap(), TEXSLOT_ONDEMAND4, threadID);
							GetDevice()->BindResourcePS(material->GetDisplacementMap(), TEXSLOT_ONDEMAND5, threadID);
						}

						PSTYPES realPS = GetPSTYPE(shaderType, material);
						GetDevice()->BindPS(pixelShaders[realPS], threadID);
					}
					else
					{
						GetDevice()->BindPS(pixelShaders[PSTYPE_SIMPLEST], threadID);
					}
					if (tessellation)
					{
						GetDevice()->BindResourceDS(material->GetDisplacementMap(), TEXSLOT_ONDEMAND5, threadID);
					}

					SetAlphaRef(material->alphaRef, threadID);

					GetDevice()->DrawIndexedInstanced((int)subset.subsetIndices.size(), k, threadID);
				}
			}

		}


		GetDevice()->BindPS(nullptr, threadID);
		GetDevice()->BindGS(nullptr, threadID);
		GetDevice()->BindVS(nullptr, threadID);
		GetDevice()->BindDS(nullptr, threadID);
		GetDevice()->BindHS(nullptr, threadID);
		GetDevice()->BindVertexBuffer(nullptr, 1, 0, threadID); // unbind the instance buffer

		ResetAlphaRef(threadID);

		wiRenderer::GetDevice()->EventEnd(threadID);
	}
}

void wiRenderer::DrawWorld(Camera* camera, bool tessellation, GRAPHICSTHREAD threadID, SHADERTYPE shaderType, Texture2D* refRes, bool grass)
{

	const FrameCulling& culling = frameCullings[camera];
	const CulledCollection& culledRenderer = culling.culledRenderer_opaque;

	GetDevice()->EventBegin("DrawWorld");

	if (shaderType == SHADERTYPE_TILEDFORWARD)
	{
		GetDevice()->BindResourcePS(resourceBuffers[RBTYPE_LIGHTINDEXLIST_OPAQUE], SBSLOT_LIGHTINDEXLIST, threadID);
		GetDevice()->BindResourcePS(textures[TEXTYPE_2D_LIGHTGRID_OPAQUE], TEXSLOT_LIGHTGRID, threadID);
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

		RenderMeshes(camera->translation, culledRenderer, shaderType, RENDERTYPE_OPAQUE, threadID, tessellation, !GetOcclusionCullingEnabled());
	}

	GetDevice()->EventEnd();

}

void wiRenderer::DrawWorldTransparent(Camera* camera, SHADERTYPE shaderType, Texture2D* refracRes, Texture2D* refRes
	, Texture2D* waterRippleNormals, GRAPHICSTHREAD threadID, bool grass)
{

	const FrameCulling& culling = frameCullings[camera];
	const CulledCollection& culledRenderer = culling.culledRenderer_transparent;

	GetDevice()->EventBegin("DrawWorldTransparent");

	if (shaderType == SHADERTYPE_TILEDFORWARD)
	{
		GetDevice()->BindResourcePS(resourceBuffers[RBTYPE_LIGHTINDEXLIST_TRANSPARENT], SBSLOT_LIGHTINDEXLIST, threadID);
		GetDevice()->BindResourcePS(textures[TEXTYPE_2D_LIGHTGRID_TRANSPARENT], TEXSLOT_LIGHTGRID, threadID);
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

		RenderMeshes(camera->translation, culledRenderer, shaderType, RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER, threadID, false, !GetOcclusionCullingEnabled());
	}

	GetDevice()->EventEnd();
}


void wiRenderer::DrawSky(GRAPHICSTHREAD threadID)
{
	if (enviroMap == nullptr)
		return;

	GetDevice()->EventBegin("DrawSky", threadID);

	GetDevice()->BindPrimitiveTopology(TRIANGLELIST,threadID);
	GetDevice()->BindRasterizerState(rasterizers[RSTYPE_SKY],threadID);
	GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD],STENCILREF_SKY,threadID);
	GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE],threadID);
	
	GetDevice()->BindVS(vertexShaders[VSTYPE_SKY], threadID);
	GetDevice()->BindPS(pixelShaders[PSTYPE_SKY], threadID);
	
	GetDevice()->BindResourcePS(enviroMap, TEXSLOT_ENV_GLOBAL, threadID);

	GetDevice()->BindVertexBuffer(nullptr,0,0,threadID);
	GetDevice()->BindVertexLayout(nullptr, threadID);
	GetDevice()->Draw(240,threadID);

	GetDevice()->EventEnd();
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

	GetDevice()->BindVertexBuffer(nullptr, 0, 0, threadID);
	GetDevice()->BindVertexLayout(nullptr, threadID);
	GetDevice()->Draw(240, threadID);

	GetDevice()->EventEnd();
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
		device->BindRasterizerState(rasterizers[RSTYPE_BACK], threadID);
		device->BindBlendState(blendStates[BSTYPE_TRANSPARENT], threadID);
		device->BindDepthStencilState(depthStencils[DSSTYPE_STENCILREAD_MATCH], STENCILREF::STENCILREF_DEFAULT, threadID);
		device->BindVertexLayout(nullptr, threadID);
		device->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST, threadID);

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

				device->Draw(36, threadID);

			}

		}

		device->EventEnd(threadID);
	}
}

void wiRenderer::RefreshEnvProbes(GRAPHICSTHREAD threadID)
{
	GetDevice()->EventBegin("EnvironmentProbe Refresh");

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

		vector<SHCAM> cameras;
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

			GetDevice()->BindVertexBuffer(nullptr, 0, 0, threadID);
			GetDevice()->BindVertexLayout(nullptr, threadID);
			GetDevice()->Draw(240, threadID);
		}


		GetDevice()->BindGS(nullptr, threadID);


		probe->cubeMap.Deactivate(threadID);

		GetDevice()->GenerateMips(probe->cubeMap.GetTexture(), threadID);

	}

	GetDevice()->EventEnd();
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
		cb.mColor.x = (float)culling.culledDecals.size();
		cb.mColor.y = cb.mColor.x + (float)culling.culledLight_count;
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
		GetDevice()->EventEnd();

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
			GetDevice()->EventEnd();

			GetDevice()->EventBegin("Voxel Scene Clear Normals", threadID);
			GetDevice()->UnBindResources(1, 1, threadID);
			GetDevice()->BindUnorderedAccessResourceCS(resourceBuffers[RBTYPE_VOXELSCENE], 0, threadID);
			GetDevice()->BindCS(computeShaders[CSTYPE_VOXELCLEARONLYNORMAL], threadID);
			GetDevice()->Dispatch((UINT)(voxelSceneData.res * voxelSceneData.res * voxelSceneData.res / 1024), 1, 1, threadID);
			GetDevice()->EventEnd();

			result = (Texture3D*)textures[TEXTYPE_3D_VOXELRADIANCE_HELPER];
		}

		GetDevice()->BindCS(nullptr, threadID);
		GetDevice()->UnBindUnorderedAccessResources(0, 2, threadID);


		// Pre-integrate the voxel texture by creating blurred mip levels:
		GenerateMipChain(result, MIPGENFILTER_LINEAR, threadID);
	}

	if (voxelHelper)
	{
		GetDevice()->BindResourceVS(result, TEXSLOT_VOXELRADIANCE, threadID);
	}
	GetDevice()->BindResourcePS(result, TEXSLOT_VOXELRADIANCE, threadID);

	wiProfiler::GetInstance().EndRange(threadID);
	GetDevice()->EventEnd();
}

void wiRenderer::ComputeTiledLightCulling(GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Tiled Light Culling", wiProfiler::DOMAIN_GPU, threadID);
	GraphicsDevice* device = wiRenderer::GetDevice();

	int _width = device->GetScreenWidth();
	int _height = device->GetScreenHeight();
	UINT averageLightCountPerTile = 256;

	static int _savedWidth = 0;
	static int _savedHeight = 0;
	bool _resolutionChanged = GetDevice()->ResolutionChanged();
	if (_savedWidth != _width || _savedHeight != _height)
	{
		_resolutionChanged = true;
		_savedWidth = _width;
		_savedHeight = _height;
	}


	// Calc dispatchparams
	DispatchParamsCB dispatchParams;
	{
		dispatchParams.numThreads[0] = (UINT)ceilf(_width / (float)BLOCK_SIZE);
		dispatchParams.numThreads[1] = (UINT)ceilf(_height / (float)BLOCK_SIZE);
		dispatchParams.numThreads[2] = 1;
		dispatchParams.numThreadGroups[0] = (UINT)ceilf(dispatchParams.numThreads[0] / (float)BLOCK_SIZE);
		dispatchParams.numThreadGroups[1] = (UINT)ceilf(dispatchParams.numThreads[1] / (float)BLOCK_SIZE);
		dispatchParams.numThreadGroups[2] = 1;
		dispatchParams.value0 = frameCullings[getCamera()].culledLight_count + (UINT)frameCullings[getCamera()].culledDecals.size(); // light count (forward_list does not have size())
		device->UpdateBuffer(constantBuffers[CBTYPE_DISPATCHPARAMS], &dispatchParams, threadID);
		device->BindConstantBufferCS(constantBuffers[CBTYPE_DISPATCHPARAMS], CB_GETBINDSLOT(DispatchParamsCB), threadID);
	}

	static GPUBuffer* frustumBuffer = nullptr;
	if (frustumBuffer == nullptr || _resolutionChanged)
	{
		SAFE_DELETE(frustumBuffer);

		frustumBuffer = new GPUBuffer;

		UINT _stride = sizeof(XMFLOAT4) * 4;

		GPUBufferDesc bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.ByteWidth = _stride * dispatchParams.numThreads[0] * dispatchParams.numThreads[1] * dispatchParams.numThreads[2]; // storing 4 planes for every tile
		bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.Usage = USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
		bd.StructureByteStride = _stride;
		device->CreateBuffer(&bd, nullptr, frustumBuffer);
	}
	static GPUBuffer* lightCounterHelper_Opaque = nullptr;
	static GPUBuffer* lightCounterHelper_Transparent = nullptr;
	if (lightCounterHelper_Opaque == nullptr || lightCounterHelper_Transparent == nullptr)
	{
		lightCounterHelper_Opaque = new GPUBuffer;
		lightCounterHelper_Transparent = new GPUBuffer;

		GPUBufferDesc bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.ByteWidth = sizeof(UINT) * 1;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_UNORDERED_ACCESS; // only used in the compute shader which is assembling the light index list, so no need for Shader Resource View
		bd.CPUAccessFlags = 0;
		bd.StructureByteStride = sizeof(UINT);
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		device->CreateBuffer(&bd, nullptr, lightCounterHelper_Opaque);
		device->CreateBuffer(&bd, nullptr, lightCounterHelper_Transparent);
	}
	if (textures[TEXTYPE_2D_LIGHTGRID_OPAQUE] == nullptr || textures[TEXTYPE_2D_LIGHTGRID_TRANSPARENT] == nullptr || _resolutionChanged)
	{
		SAFE_DELETE(textures[TEXTYPE_2D_LIGHTGRID_OPAQUE]);
		SAFE_DELETE(textures[TEXTYPE_2D_LIGHTGRID_TRANSPARENT]);

		Texture2DDesc desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.ArraySize = 1;
		desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_R32G32_UINT;
		desc.Width = dispatchParams.numThreads[0];
		desc.Height = dispatchParams.numThreads[1];
		desc.MipLevels = 1;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = USAGE_DEFAULT;
		device->CreateTexture2D(&desc, nullptr, (Texture2D**)&textures[TEXTYPE_2D_LIGHTGRID_OPAQUE]);
		device->CreateTexture2D(&desc, nullptr, (Texture2D**)&textures[TEXTYPE_2D_LIGHTGRID_TRANSPARENT]);
	}
	if (_resolutionChanged)
	{
		SAFE_DELETE(resourceBuffers[RBTYPE_LIGHTINDEXLIST_OPAQUE]);
		SAFE_DELETE(resourceBuffers[RBTYPE_LIGHTINDEXLIST_TRANSPARENT]);
		resourceBuffers[RBTYPE_LIGHTINDEXLIST_OPAQUE] = new GPUBuffer;
		resourceBuffers[RBTYPE_LIGHTINDEXLIST_TRANSPARENT] = new GPUBuffer;

		GPUBufferDesc bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.ByteWidth = sizeof(UINT) * dispatchParams.numThreads[0] * dispatchParams.numThreads[1]* dispatchParams.numThreads[2] * averageLightCountPerTile;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
		bd.CPUAccessFlags = 0;
		bd.StructureByteStride = sizeof(UINT);
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
		device->CreateBuffer(&bd, nullptr, resourceBuffers[RBTYPE_LIGHTINDEXLIST_OPAQUE]);
		device->CreateBuffer(&bd, nullptr, resourceBuffers[RBTYPE_LIGHTINDEXLIST_TRANSPARENT]);
	}

	// calculate the per-tile frustums once:
	static bool frustumsComplete = false;
	if(!frustumsComplete || _resolutionChanged)
	{
		frustumsComplete = true;
		device->BindUnorderedAccessResourceCS(frustumBuffer, UAVSLOT_TILEFRUSTUMS, threadID);
		device->BindCS(computeShaders[CSTYPE_TILEFRUSTUMS], threadID);
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

		device->EventBegin("Light Culling", threadID);

		device->UnBindResources(TEXSLOT_LIGHTGRID, SBSLOT_LIGHTINDEXLIST - TEXSLOT_LIGHTGRID + 1, threadID);

		device->BindResourceCS(frustumBuffer, SBSLOT_TILEFRUSTUMS, threadID);
		
		if (GetDebugLightCulling())
		{
			device->BindUnorderedAccessResourceCS(textures[TEXTYPE_2D_DEBUGUAV], UAVSLOT_DEBUGTEXTURE, threadID);
			device->BindCS(computeShaders[CSTYPE_TILEDLIGHTCULLING_DEBUG], threadID);
		}
		else
		{
			device->BindCS(computeShaders[CSTYPE_TILEDLIGHTCULLING], threadID);
		}
		device->BindUnorderedAccessResourceCS(lightCounterHelper_Opaque, UAVSLOT_LIGHTINDEXCOUNTERHELPER_OPAQUE, threadID);
		device->BindUnorderedAccessResourceCS(lightCounterHelper_Transparent, UAVSLOT_LIGHTINDEXCOUNTERHELPER_TRANSPARENT, threadID);
		device->BindUnorderedAccessResourceCS(resourceBuffers[RBTYPE_LIGHTINDEXLIST_OPAQUE], UAVSLOT_LIGHTINDEXLIST_OPAQUE, threadID);
		device->BindUnorderedAccessResourceCS(resourceBuffers[RBTYPE_LIGHTINDEXLIST_TRANSPARENT], UAVSLOT_LIGHTINDEXLIST_TRANSPARENT, threadID);
		device->BindUnorderedAccessResourceCS(textures[TEXTYPE_2D_LIGHTGRID_OPAQUE], UAVSLOT_LIGHTGRID_OPAQUE, threadID);
		device->BindUnorderedAccessResourceCS(textures[TEXTYPE_2D_LIGHTGRID_TRANSPARENT], UAVSLOT_LIGHTGRID_TRANSPARENT, threadID);
		device->Dispatch(dispatchParams.numThreads[0], dispatchParams.numThreads[1], dispatchParams.numThreads[2], threadID);
		device->UnBindUnorderedAccessResources(0, 8, threadID); // this unbinds pretty much every uav

		device->EventEnd(threadID);
	}

	wiProfiler::GetInstance().EndRange(threadID);
}
void wiRenderer::ResolveMSAADepthBuffer(Texture2D* dst, Texture2D* src, GRAPHICSTHREAD threadID)
{
	GetDevice()->EventBegin("Resolve MSAA DepthBuffer");

	GetDevice()->BindResourceCS(src, TEXSLOT_ONDEMAND0, threadID);
	GetDevice()->BindUnorderedAccessResourceCS(dst, 0, threadID);

	Texture2DDesc desc = src->GetDesc();

	GetDevice()->BindCS(computeShaders[CSTYPE_RESOLVEMSAADEPTHSTENCIL], threadID);
	GetDevice()->Dispatch((UINT)ceilf(desc.Width / 16.f), (UINT)ceilf(desc.Height / 16.f), 1, threadID);
	GetDevice()->BindCS(nullptr, threadID);


	GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
	GetDevice()->UnBindUnorderedAccessResources(0, 1, threadID);

	GetDevice()->EventEnd();
}
void wiRenderer::GenerateMipChain(Texture1D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID)
{
	assert(0 && "Not implemented!");
}
void wiRenderer::GenerateMipChain(Texture2D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID)
{
	assert(0 && "Not implemented!");
}
void wiRenderer::GenerateMipChain(Texture3D* texture, MIPGENFILTER filter, GRAPHICSTHREAD threadID)
{
	Texture3DDesc desc = texture->GetDesc();

	if (desc.MipLevels < 2)
	{
		assert(0);
		return;
	}

	GetDevice()->EventBegin("GenerateMipChain 3D", threadID);

	switch (filter)
	{
	case wiRenderer::MIPGENFILTER_POINT:
		GetDevice()->BindCS(computeShaders[CSTYPE_GENERATEMIPCHAIN3D_SIMPLEFILTER], threadID);
		GetDevice()->BindSamplerCS(samplers[SSLOT_POINT_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_LINEAR:
		GetDevice()->BindCS(computeShaders[CSTYPE_GENERATEMIPCHAIN3D_SIMPLEFILTER], threadID);
		GetDevice()->BindSamplerCS(samplers[SSLOT_LINEAR_CLAMP], SSLOT_ONDEMAND0, threadID);
		break;
	case wiRenderer::MIPGENFILTER_GAUSSIAN:
		assert(0 && "Not implemented!");
		break;
	}

	for (UINT i = 0; i < desc.MipLevels - 1; ++i)
	{
		GetDevice()->BindUnorderedAccessResourceCS(texture, 0, threadID, i + 1);
		GetDevice()->BindResourceCS(texture, 0, threadID, i);
		desc.Width = max(1, desc.Width / 2);
		desc.Height = max(1, desc.Height / 2);
		desc.Depth = max(1, desc.Depth / 2);
		GetDevice()->Dispatch(max(1, desc.Width / 8), max(1, desc.Height / 8), max(1, desc.Depth / 8), threadID);
	}

	GetDevice()->UnBindResources(0, 1, threadID);
	GetDevice()->UnBindUnorderedAccessResources(0, 1, threadID);
	GetDevice()->BindCS(nullptr, threadID);

	GetDevice()->EventEnd();
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
			using namespace wiRectPacker;
			static map<Texture2D*, rect_xywhf> storedTextures;

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

				vector<bin> bins;
				if (pack(out_rects, (int)storedTextures.size(), 16384, bins))
				{
					assert(bins.size() == 1 && "Decal atlas packing into single texture failed!");

					SAFE_DELETE(atlasTexture);

					Texture2DDesc desc;
					ZeroMemory(&desc, sizeof(desc));
					desc.Width = (UINT)bins[0].size.w;
					desc.Height = (UINT)bins[0].size.h;
					desc.MipLevels = 1;
					desc.ArraySize = 1;
					desc.Format = FORMAT_B8G8R8A8_UNORM; // png decals are loaded into this format! todo: DXT!
					desc.SampleDesc.Count = 1;
					desc.SampleDesc.Quality = 0;
					desc.Usage = USAGE_DEFAULT;
					desc.BindFlags = BIND_SHADER_RESOURCE;
					desc.CPUAccessFlags = 0;
					desc.MiscFlags = 0;

					device->CreateTexture2D(&desc, nullptr, &atlasTexture);

					for (auto& it : storedTextures)
					{
						device->CopyTexture2D_Region(atlasTexture, 0, it.second.x, it.second.y, it.first, 0, threadID);
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
	auto& world = GetScene().worldInfo;
	value.mAmbient = world.ambient;
	value.mFog = world.fogSEH;
	value.mHorizon = world.horizon;
	value.mZenith = world.zenith;
	value.mScreenWidthHeight = XMFLOAT2((float)GetDevice()->GetScreenWidth(), (float)GetDevice()->GetScreenHeight());
	value.mVoxelRadianceDataSize = voxelSceneData.voxelsize;
	value.mVoxelRadianceDataRes = GetVoxelRadianceEnabled() ? (UINT)voxelSceneData.res : 0;
	value.mVoxelRadianceDataCenter = voxelSceneData.center;
	value.mVoxelRadianceDataConeTracingQuality = voxelSceneData.coneTracingQuality;
	value.mVoxelRadianceDataFalloff = voxelSceneData.falloff;

	if (memcmp(&prevcb[threadID], &value, sizeof(WorldCB)) != 0)
	{
		prevcb[threadID] = value;
		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_WORLD], &prevcb[threadID], threadID);
	}
}
void wiRenderer::UpdateFrameCB(GRAPHICSTHREAD threadID)
{
	FrameCB cb;

	cb.mTime = renderTime;
	auto& wind = GetScene().wind;
	cb.mWindRandomness = wind.randomness;
	cb.mWindWaveSize = wind.waveSize;
	cb.mWindDirection = wind.direction;
	cb.mFrameCount = (UINT)GetDevice()->GetFrameCount();
	cb.mSunLightArrayIndex = GetSunArrayIndex();

	auto camera = getCamera();
	auto prevCam = prevFrameCam;
	auto reflCam = getRefCamera();

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
void wiRenderer::UpdateMaterialCB(const MaterialCB& value, GRAPHICSTHREAD threadID)
{
	static MaterialCB prevcb[GRAPHICSTHREAD_COUNT];
	if (memcmp(&prevcb[threadID],&value,sizeof(MaterialCB)) != 0)
	{
		prevcb[threadID] = value;
		GetDevice()->UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], &value, threadID);
	}
}
wiRenderer::APICB apiCB[GRAPHICSTHREAD_COUNT];
void wiRenderer::SetClipPlane(XMFLOAT4 clipPlane, GRAPHICSTHREAD threadID)
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
	static vector<Texture2D*> luminance_avg(0);
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

wiRenderer::Picked wiRenderer::Pick(RAY& ray, int pickType, const string& layer,
	const string& layerDisable)
{
	CulledCollection culledRenderer;
	CulledList culledObjects;
	wiSPTree* searchTree = spTree;
	if (searchTree)
	{
		searchTree->getVisible(ray, culledObjects);

		vector<Picked> pickPoints;

		RayIntersectMeshes(ray, culledObjects, pickPoints, pickType, true, layer, layerDisable, true);

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
						pick.distance = wiMath::Distance(light->translation, ray.origin);
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
					XMVECTOR localOrigin=XMLoadFloat3(&ray.origin), localDirection=XMLoadFloat3(&ray.direction);
					XMMATRIX localTransform = XMLoadFloat4x4(&decal->world);
					localTransform = XMMatrixInverse(nullptr, localTransform);
					localOrigin = XMVector3Transform(localOrigin, localTransform);
					localDirection = XMVector3TransformNormal(localDirection, localTransform);
					RAY localRay = RAY(localOrigin, localDirection);
					if (AABB(XMFLOAT3(-1, -1, -1),XMFLOAT3(1, 1, 1)).intersects(localRay))
					{
						Picked pick = Picked();
						pick.transform = decal;
						pick.decal = decal;
						pick.distance = wiMath::Distance(decal->translation, ray.origin);
						pickPoints.push_back(pick);
					}
				}
			}
		}
		if (pickType & PICK_ENVPROBE)
		{
			for (auto& x : GetScene().environmentProbes)
			{
				if (SPHERE(x->translation,1).intersects(ray))
				{
					Picked pick = Picked();
					pick.transform = x;
					pick.envProbe = x;
					pick.distance = wiMath::Distance(x->translation, ray.origin);
					pickPoints.push_back(pick);
				}
			}
		}

		if (!pickPoints.empty()){
			Picked min = pickPoints.front();
			for (unsigned int i = 1; i<pickPoints.size(); ++i){
				if (pickPoints[i].distance < min.distance) {
					min = pickPoints[i];
				}
			}
			return min;
		}

	}

	return Picked();
}
wiRenderer::Picked wiRenderer::Pick(long cursorX, long cursorY, int pickType, const string& layer,
	const string& layerDisable)
{
	RAY ray = getPickRay(cursorX, cursorY);

	return Pick(ray, pickType, layer, layerDisable);
}

RAY wiRenderer::getPickRay(long cursorX, long cursorY){
	Camera* cam = getCamera();
	XMMATRIX V = cam->GetView();
	XMMATRIX P = cam->GetProjection();
	XMMATRIX W = XMMatrixIdentity();
	XMVECTOR& lineStart = XMVector3Unproject(XMVectorSet((float)cursorX, (float)cursorY, 0, 1), 0, 0, cam->width, cam->height, 0.0f, 1.0f, P, V, W);
	XMVECTOR& lineEnd = XMVector3Unproject(XMVectorSet((float)cursorX, (float)cursorY, 1, 1), 0, 0, cam->width, cam->height, 0.0f, 1.0f, P, V, W);
	XMVECTOR& rayDirection = XMVector3Normalize(XMVectorSubtract(lineEnd, lineStart));
	return RAY(lineStart, rayDirection);
}

void wiRenderer::RayIntersectMeshes(const RAY& ray, const CulledList& culledObjects, vector<Picked>& points,
	int pickType, bool dynamicObjects, const string& layer, const string& layerDisable, bool onlyVisible)
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

	static size_t _arraySize = 10000;
	static XMVECTOR* _vertices = (XMVECTOR*)_mm_malloc(sizeof(XMVECTOR)*_arraySize, 16);
	static int* _materialIndices = new int[_arraySize];

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
		if (onlyVisible && object->occlusionQuery.result_passed != TRUE)
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
		if (mesh->vertices.size() >= _arraySize)
		{
			_mm_free(_vertices);
			SAFE_DELETE_ARRAY(_materialIndices);
			_arraySize *= 2;
			_vertices = (XMVECTOR*)_mm_malloc(sizeof(XMVECTOR)*_arraySize, 16);
			_materialIndices = new int[_arraySize];
		}

		XMMATRIX& objectMat = object->getMatrix();
		XMMATRIX& objectMat_Inverse = XMMatrixInverse(nullptr, objectMat);

		XMVECTOR& rayOrigin_local = XMVector3Transform(rayOrigin, objectMat_Inverse);
		XMVECTOR& rayDirection_local = XMVector3Normalize(XMVector3TransformNormal(rayDirection, objectMat_Inverse));

		for (size_t i = 0; i < mesh->vertices.size(); ++i)
		{
			if (object->isArmatureDeformed() && !object->mesh->armature->boneCollection.empty())
			{
				Vertex& v = TransformVertex(mesh, mesh->vertices[i], objectMat_Inverse);
				_vertices[i] = XMLoadFloat4(&v.pos);
				_materialIndices[i] = (int)v.tex.z;
			}
			else
			{
				Vertex& v = mesh->vertices_Complete[i];
				_vertices[i] = XMLoadFloat4(&v.pos);
				_materialIndices[i] = (int)v.tex.z;
			}
		}

		for (size_t i = 0; i<mesh->indices.size(); i += 3)
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
				picked.subsetIndex = _materialIndices[i0];
				points.push_back(picked);
			}
		}

	}
}

void wiRenderer::CalculateVertexAO(Object* object)
{
	//TODO

	static const float minAmbient = 0.05f;
	static const float falloff = 0.1f;

	Mesh* mesh = object->mesh;

	XMMATRIX& objectMat = object->getMatrix();

	CulledCollection culledRenderer;
	CulledList culledObjects;
	wiSPTree* searchTree = spTree;

	int ind = 0;
	for (SkinnedVertex& vert : mesh->vertices)
	{
		float ambientShadow = 0.0f;

		XMFLOAT3 vPos, vNor;

		//XMVECTOR p = XMVector4Transform(XMVectorSet(vert.pos.x, vert.pos.y, vert.pos.z, 1), XMLoadFloat4x4(&object->world));
		//XMVECTOR n = XMVector3Transform(XMVectorSet(vert.nor.x, vert.nor.y, vert.nor.z, 0), XMLoadFloat4x4(&object->world));

		//XMStoreFloat3(&vPos, p);
		//XMStoreFloat3(&vNor, n);

		Vertex v = TransformVertex(mesh, vert, objectMat);
		vPos.x = v.pos.x;
		vPos.y = v.pos.y;
		vPos.z = v.pos.z;
		vNor.x = v.nor.x;
		vNor.y = v.nor.y;
		vNor.z = v.nor.z;

			RAY ray = RAY(vPos, vNor);
			XMVECTOR& rayOrigin = XMLoadFloat3(&ray.origin);
			XMVECTOR& rayDirection = XMLoadFloat3(&ray.direction);

			searchTree->getVisible(ray, culledObjects);

			vector<Picked> points;

			RayIntersectMeshes(ray, culledObjects, points, PICK_OPAQUE, false);


			if (!points.empty()){
				Picked min = points.front();
				float mini = wiMath::DistanceSquared(min.position, ray.origin);
				for (unsigned int i = 1; i<points.size(); ++i){
					if (float nm = wiMath::DistanceSquared(points[i].position, ray.origin)<mini){
						min = points[i];
						mini = nm;
					}
				}

				float ambientLightIntensity = wiMath::Clamp(abs(wiMath::Distance(ray.origin, min.position)) / falloff, 0, 1);
				ambientLightIntensity += minAmbient;

				vert.nor.w = ambientLightIntensity;
				mesh->vertices[ind].nor.w = ambientLightIntensity;
			}

			++ind;
	}

	mesh->calculatedAO = true;
}

Model* wiRenderer::LoadModel(const string& dir, const string& name, const XMMATRIX& transform, const string& ident)
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
void wiRenderer::LoadWorldInfo(const string& dir, const string& name)
{
	LoadWiWorldInfo(/*dir*/"", name+".wiw", GetScene().worldInfo, GetScene().wind);

	GetDevice()->LOCK();
	UpdateWorldCB(GRAPHICSTHREAD_IMMEDIATE);
	GetDevice()->UNLOCK();
}
void wiRenderer::LoadDefaultLighting()
{
	Light* defaultLight = new Light();
	defaultLight->name = "_WickedEngine_DefaultLight_";
	defaultLight->SetType(Light::DIRECTIONAL);
	defaultLight->color = XMFLOAT4(1, 1, 1, 1);
	defaultLight->enerDis = XMFLOAT4(1, 0, 0, 0);
	XMStoreFloat4(&defaultLight->rotation_rest, XMQuaternionRotationRollPitchYaw(0, -XM_PIDIV4, XM_PIDIV4));
	defaultLight->UpdateTransform();
	defaultLight->UpdateLight();

	Model* model = new Model;
	model->name = "_WickedEngine_DefaultLight_Holder_";
	model->lights.push_back(defaultLight);
	GetScene().models.push_back(model);

	if (spTree_lights) {
		spTree_lights->AddObjects(spTree_lights->root, vector<Cullable*>(model->lights.begin(), model->lights.end()));
	}
	else
	{
		GenerateSPTree(spTree_lights, vector<Cullable*>(model->lights.begin(), model->lights.end()), SPTREE_GENERATE_OCTREE);
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
	if (physicsEngine && GetGameSpeed()){

		physicsEngine->addWind(GetScene().wind.direction);

		//UpdateSoftBodyPinning();
		for (Model* model : GetScene().models)
		{
			// Soft body pinning update
			for (MeshCollection::iterator iter = model->meshes.begin(); iter != model->meshes.end(); ++iter) {
				Mesh* m = iter->second;
				if (m->softBody) {
					int gvg = m->goalVG;
					if (gvg >= 0) {
						int j = 0;
						for (map<int, float>::iterator it = m->vertexGroups[gvg].vertices.begin(); it != m->vertexGroups[gvg].vertices.end(); ++it) {
							int vi = (*it).first;
							Vertex tvert = m->vertices_Complete[vi];
							if (m->hasArmature())
								tvert = TransformVertex(m, vi);
							m->goalPositions[j] = XMFLOAT3(tvert.pos.x, tvert.pos.y, tvert.pos.z);
							m->goalNormals[j] = XMFLOAT3(tvert.nor.x, tvert.nor.y, tvert.nor.z);
							++j;
						}
					}
				}
			}


			for (Object* object : model->objects) {
				int pI = object->physicsObjectI;
				if (pI >= 0) {
					if (object->mesh->softBody) {
						physicsEngine->connectSoftBodyToVertices(
							object->mesh, pI
							);
					}
					if (object->kinematic && object->rigidBody) {
						physicsEngine->transformBody(object->rotation, object->translation, pI);
					}
				}
			}
		}

		physicsEngine->Update(dt);

		for (Model* model : GetScene().models)
		{
			for (Object* object : model->objects) {
				int pI = object->physicsObjectI;
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

	GetDevice()->LOCK();

	scene->environmentProbes.push_back(probe);

	GetDevice()->UNLOCK();
}

void wiRenderer::CreateImpostor(Mesh* mesh)
{
	Mesh::CreateImpostorVB();

	static const GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE;
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


	mesh->AddRenderableInstance(Instance(XMMatrixIdentity()), 0, threadID);
	mesh->UpdateRenderableInstances(1, threadID);
	GetDevice()->BindVertexBuffer((mesh->streamoutBuffer.IsValid() ? &mesh->streamoutBuffer : &mesh->vertexBuffer), 0, sizeof(Vertex), threadID);
	GetDevice()->BindVertexBuffer(&mesh->instanceBuffer, 1, sizeof(Instance), threadID);


	GetDevice()->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);
	GetDevice()->BindRasterizerState(rasterizers[RSTYPE_DOUBLESIDED], threadID);
	GetDevice()->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], mesh->stencilRef, threadID);
	GetDevice()->BindPrimitiveTopology(TRIANGLELIST, threadID);
	GetDevice()->BindVertexLayout(vertexLayouts[VLTYPE_EFFECT], threadID);
	GetDevice()->BindVS(vertexShaders[VSTYPE_OBJECT10], threadID);
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
				GetDevice()->BindIndexBuffer(&subset.indexBuffer, threadID);

				UpdateMaterialCB(MaterialCB(*subset.material), threadID);

				GetDevice()->BindResourcePS(subset.material->GetBaseColorMap(), TEXSLOT_ONDEMAND0, threadID);
				GetDevice()->BindResourcePS(subset.material->GetNormalMap(), TEXSLOT_ONDEMAND1, threadID);
				GetDevice()->BindResourcePS(subset.material->GetRoughnessMap(), TEXSLOT_ONDEMAND2, threadID);
				GetDevice()->BindResourcePS(subset.material->GetReflectanceMap(), TEXSLOT_ONDEMAND3, threadID);
				GetDevice()->BindResourcePS(subset.material->GetMetalnessMap(), TEXSLOT_ONDEMAND4, threadID);
				GetDevice()->BindResourcePS(subset.material->GetDisplacementMap(), TEXSLOT_ONDEMAND5, threadID);


				GetDevice()->DrawIndexedInstanced((int)subset.subsetIndices.size(), 1, threadID);
			}
		}

	}
	GetDevice()->GenerateMips(mesh->impostorTarget.GetTexture(), threadID);
	//GetDevice()->SaveTexturePNG("d:\\asd_col.png", mesh->impostorTarget.GetTexture(0), threadID);
	//GetDevice()->SaveTexturePNG("d:\\asd_nor.png", mesh->impostorTarget.GetTexture(1), threadID);
	//GetDevice()->SaveTexturePNG("d:\\asd_rou.png", mesh->impostorTarget.GetTexture(2), threadID);
	//GetDevice()->SaveTexturePNG("d:\\asd_ref.png", mesh->impostorTarget.GetTexture(3), threadID);
	//GetDevice()->SaveTexturePNG("d:\\asd_met.png", mesh->impostorTarget.GetTexture(4), threadID);

	mesh->impostorTarget.viewPort = savedViewPort;
	*cam = savedCam;
	UpdateCameraCB(cam, threadID);

	GetDevice()->UNLOCK();
}

void wiRenderer::MaterialCB::Create(const Material& mat)
{
	baseColor = XMFLOAT4(mat.baseColor.x, mat.baseColor.y, mat.baseColor.z, mat.alpha);
	texMulAdd = mat.texMulAdd;
	roughness = mat.roughness;
	reflectance = mat.reflectance;
	metalness = mat.metalness;
	emissive = mat.emissive;
	refractionIndex = mat.refractionIndex;
	subsurfaceScattering = mat.subsurfaceScattering;
	normalMapStrength = (mat.normalMap == nullptr? 0 : mat.normalMapStrength);
	parallaxOcclusionMapping = mat.parallaxOcclusionMapping;
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

	for (Object* o : model->objects)
	{
		if (physicsEngine != nullptr) {
			physicsEngine->registerObject(o);
		}
	}


	Update();

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
		spTree->AddObjects(spTree->root, vector<Cullable*>(objects.begin(), objects.end()));
	}
	else
	{
		GenerateSPTree(spTree, vector<Cullable*>(objects.begin(), objects.end()), SPTREE_GENERATE_OCTREE);
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
		spTree_lights->AddObjects(spTree_lights->root, vector<Cullable*>(lights.begin(), lights.end()));
	}
	else
	{
		GenerateSPTree(spTree_lights, vector<Cullable*>(lights.begin(), lights.end()), SPTREE_GENERATE_OCTREE);
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
