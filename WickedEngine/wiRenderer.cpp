#include "wiRenderer.h"
#include "skinningDEF.h"
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
#include "TextureMapping.h"
#include "wiGraphicsAPI_DX11.h"

#pragma region STATICS
GraphicsDevice* wiRenderer::graphicsDevice = nullptr;

Sampler wiRenderer::samplers[SSLOT_COUNT];

BufferResource		wiRenderer::constantBuffers[CBTYPE_LAST];
VertexShader		wiRenderer::vertexShaders[VSTYPE_LAST];
PixelShader			wiRenderer::pixelShaders[PSTYPE_LAST];
GeometryShader		wiRenderer::geometryShaders[GSTYPE_LAST];
HullShader			wiRenderer::hullShaders[HSTYPE_LAST];
DomainShader		wiRenderer::domainShaders[DSTYPE_LAST];
RasterizerState		wiRenderer::rasterizers[RSTYPE_LAST];
DepthStencilState	wiRenderer::depthStencils[DSSTYPE_LAST];
VertexLayout		wiRenderer::vertexLayouts[VLTYPE_LAST];
BlendState			wiRenderer::blendStates[BSTYPE_LAST];

int wiRenderer::SCREENWIDTH=1280,wiRenderer::SCREENHEIGHT=720,wiRenderer::SHADOWMAPRES=1024,wiRenderer::SOFTSHADOW=2
	,wiRenderer::POINTLIGHTSHADOW=2,wiRenderer::POINTLIGHTSHADOWRES=256, wiRenderer::SPOTLIGHTSHADOW=2, wiRenderer::SPOTLIGHTSHADOWRES=512;
bool wiRenderer::HAIRPARTICLEENABLED=true,wiRenderer::EMITTERSENABLED=true;
bool wiRenderer::wireRender = false, wiRenderer::debugSpheres = false, wiRenderer::debugBoneLines = false, wiRenderer::debugBoxes = false;

Texture2D* wiRenderer::enviroMap,*wiRenderer::colorGrading;
float wiRenderer::GameSpeed=1,wiRenderer::overrideGameSpeed=1;
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

#pragma endregion

wiRenderer::wiRenderer()
{
}

#ifndef WINSTORE_SUPPORT
HRESULT wiRenderer::InitDevice(HWND window, int screenW, int screenH, bool windowed)
#else
HRESULT wiRenderer::InitDevice(Windows::UI::Core::CoreWindow^ window)
#endif
{
	SCREENWIDTH = screenW;
	SCREENHEIGHT = screenH;

	graphicsDevice = new GraphicsDevice_DX11(window, screenW, screenH, windowed);

	return (graphicsDevice != nullptr ? S_OK : E_FAIL);
}
void wiRenderer::DestroyDevice()
{
	SAFE_DELETE(graphicsDevice);
}
void wiRenderer::Present(function<void()> drawToScreen1,function<void()> drawToScreen2,function<void()> drawToScreen3)
{

	graphicsDevice->PresentBegin();
	
	if(drawToScreen1!=nullptr)
		drawToScreen1();
	if(drawToScreen2!=nullptr)
		drawToScreen2();
	if(drawToScreen3!=nullptr)
		drawToScreen3();
	


	wiFrameRate::Frame();

	graphicsDevice->PresentEnd();

	*prevFrameCam = *cam;
}


void wiRenderer::CleanUp()
{
	for (HitSphere* x : spheres)
		delete x;
	spheres.clear();
}

void wiRenderer::SetUpStaticComponents()
{
	for (int i = 0; i < CBTYPE_LAST; ++i)
	{
		SAFE_INIT(constantBuffers[i]);
	}
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
	for (int i = 0; i < RSTYPE_LAST; ++i)
	{
		SAFE_INIT(rasterizers[i]);
	}
	for (int i = 0; i < DSSTYPE_LAST; ++i)
	{
		SAFE_INIT(depthStencils[i]);
	}
	for (int i = 0; i < VLTYPE_LAST; ++i)
	{
		SAFE_INIT(vertexLayouts[i]);
	}
	for (int i = 0; i < SSLOT_COUNT_PERSISTENT; ++i)
	{
		SAFE_INIT(samplers[i]);
	}

	//thread t1(LoadBasicShaders);
	//thread t2(LoadShadowShaders);
	//thread t3(LoadSkyShaders);
	//thread t4(LoadLineShaders);
	//thread t5(LoadTrailShaders);
	//thread t6(LoadWaterShaders);
	//thread t7(LoadTessShaders);

	LoadBasicShaders();
	LoadShadowShaders();
	LoadSkyShaders();
	LoadLineShaders();
	LoadTrailShaders();
	LoadWaterShaders();
	LoadTessShaders();

	//cam = new Camera(SCREENWIDTH, SCREENHEIGHT, 0.1f, 800, XMVectorSet(0, 4, -4, 1));
	cam = new Camera();
	cam->SetUp(SCREENWIDTH, SCREENHEIGHT, 0.1f, 800);
	refCam = new Camera();
	refCam->SetUp(SCREENWIDTH, SCREENHEIGHT, 0.1f, 800);
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
		SCREENWIDTH
		,SCREENHEIGHT
		,1,false,1,0,DXGI_FORMAT_R8G8B8A8_SNORM
		);
	imagesRTAdd.Initialize(
		SCREENWIDTH
		,SCREENHEIGHT
		,1,false
		);
	imagesRT.Initialize(
		SCREENWIDTH
		,SCREENHEIGHT
		,1,false
		);

	SetDirectionalLightShadowProps(1024, 2);
	SetPointLightShadowProps(2, 512);
	SetSpotLightShadowProps(2, 512);

	BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);

	//t1.join();
	//t2.join();
	//t3.join();
	//t4.join();
	//t5.join();
	//t6.join();
	//t7.join();
}
void wiRenderer::CleanUpStatic()
{


	wiHairParticle::CleanUpStatic();
	wiEmittedParticle::CleanUpStatic();
	Cube::CleanUpStatic();
	HitSphere::CleanUpStatic();


	for (int i = 0; i < CBTYPE_LAST; ++i)
	{
		SAFE_RELEASE(constantBuffers[i]);
	}
	for (int i = 0; i < VSTYPE_LAST; ++i)
	{
		SAFE_RELEASE(vertexShaders[i]);
	}
	for (int i = 0; i < PSTYPE_LAST; ++i)
	{
		SAFE_RELEASE(pixelShaders[i]);
	}
	for (int i = 0; i < GSTYPE_LAST; ++i)
	{
		SAFE_RELEASE(geometryShaders[i]);
	}
	for (int i = 0; i < HSTYPE_LAST; ++i)
	{
		SAFE_RELEASE(hullShaders[i]);
	}
	for (int i = 0; i < DSTYPE_LAST; ++i)
	{
		SAFE_RELEASE(domainShaders[i]);
	}
	for (int i = 0; i < RSTYPE_LAST; ++i)
	{
		SAFE_RELEASE(rasterizers[i]);
	}
	for (int i = 0; i < DSSTYPE_LAST; ++i)
	{
		SAFE_RELEASE(depthStencils[i]);
	}
	for (int i = 0; i < VLTYPE_LAST; ++i)
	{
		SAFE_RELEASE(vertexLayouts[i]);
	}
	for (int i = 0; i < BSTYPE_LAST; ++i)
	{
		SAFE_RELEASE(blendStates[i]);
	}
	for (int i = 0; i < SSLOT_COUNT_PERSISTENT; ++i)
	{
		SAFE_RELEASE(samplers[i]);
	}

	if (physicsEngine) physicsEngine->CleanUp();
}
void wiRenderer::CleanUpStaticTemp(){
	
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

	if(spTree) 
		spTree->CleanUp();
	spTree = nullptr;

	
	if(spTree_lights)
		spTree_lights->CleanUp();
	spTree_lights=nullptr;

	cam->detach();

	GetScene().ClearWorld();
}
XMVECTOR wiRenderer::GetSunPosition()
{
	for (Model* model : GetScene().models)
	{
		for (Light* l : model->lights)
			if (l->type == Light::DIRECTIONAL)
				return -XMVector3Transform(XMVectorSet(0, -1, 0, 1), XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation)));
	}
	return XMVectorSet(0, 1, 0, 1);
}
XMFLOAT4 wiRenderer::GetSunColor()
{
	for (Model* model : GetScene().models)
	{
		for (Light* l : model->lights)
			if (l->type == Light::DIRECTIONAL)
				return l->color;
	}
	return XMFLOAT4(1,1,1,1);
}
float wiRenderer::GetGameSpeed(){return GameSpeed*overrideGameSpeed;}

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
		for (unsigned int i = 0; i < model->armatures.size(); i++) {
			for (unsigned int j = 0; j < model->armatures[i]->boneCollection.size(); j++) {
				boneLines.push_back(new Lines(model->armatures[i]->boneCollection[j]->length, XMFLOAT4A(1, 1, 1, 1), i, j));
			}
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

				//XMMATRIX rest = XMLoadFloat4x4(&armatures[armatureI]->boneCollection[boneI]->recursiveRest) ;
				//XMMATRIX pose = XMLoadFloat4x4(&armatures[armatureI]->boneCollection[boneI]->world) ;
				//XMFLOAT4X4 finalM;
				//XMStoreFloat4x4( &finalM,  /*rest**/pose );

				int arm = 0;
				for (Model* model : GetScene().models)
				{
					for (Armature* armature : model->armatures)
					{
						if (arm == armatureI)
						{
							boneLines[i]->Transform(armature->boneCollection[boneI]->world);
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
	if(debugBoxes && spTree && spTree->root){
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
void wiRenderer::UpdateSPTree(wiSPTree*& tree){
	if(tree && tree->root){
		wiSPTree* newTree = tree->updateTree(tree->root);
		if(newTree){
			tree->CleanUp();
			tree=newTree;
		}
	}
}

void wiRenderer::LoadBuffers()
{
	for (int i = 0; i < CBTYPE_LAST; ++i)
	{
		constantBuffers[i] = nullptr;
	}

    BufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DYNAMIC;
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;

	//Persistent buffers...
	bd.ByteWidth = sizeof(WorldCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_WORLD]);

	bd.ByteWidth = sizeof(FrameCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_FRAME]);

	bd.ByteWidth = sizeof(CameraCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_CAMERA]);

	bd.ByteWidth = sizeof(MaterialCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_MATERIAL]);

	bd.ByteWidth = sizeof(DirectionalLightCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_DIRLIGHT]);

	bd.ByteWidth = sizeof(MiscCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_MISC]);

	bd.ByteWidth = sizeof(ShadowCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_SHADOW]);

	bd.ByteWidth = sizeof(APICB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_CLIPPLANE]);


	// On demand buffers...
	bd.ByteWidth = sizeof(PointLightCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_POINTLIGHT]);

	bd.ByteWidth = sizeof(SpotLightCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_SPOTLIGHT]);

	bd.ByteWidth = sizeof(VolumeLightCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_VOLUMELIGHT]);

	bd.ByteWidth = sizeof(DecalCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_DECAL]);

	bd.ByteWidth = sizeof(CubeMapRenderCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_CUBEMAPRENDER]);

	bd.ByteWidth = sizeof(BoneCB);
	graphicsDevice->CreateBuffer(&bd, NULL, &constantBuffers[CBTYPE_BONEBUFFER]);

}

void wiRenderer::LoadBasicShaders()
{
	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },

			{ "MATI", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR_DITHER", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, APPEND_ALIGNED_ELEMENT, INPUT_PER_INSTANCE_DATA, 1 },
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
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
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

		geometryShaders[GSTYPE_STREAMOUT] = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "sOGS.cso", wiResourceManager::GEOMETRYSHADER, nullptr, 4, pDecl));
	}


	vertexShaders[VSTYPE_OBJECT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_OBJECT_REFLECTION] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectVS_reflection.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_DIRLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_POINTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "pointLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_SPOTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "spotLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOLUMESPOTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vSpotLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOLUMEPOINTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vPointLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_DECAL] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "decalVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_ENVMAP] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_ENVMAP_SKY] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMap_skyVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;

	pixelShaders[PSTYPE_OBJECT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_TRANSPARENT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_transparent.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SIMPLEST] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_simplest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_OBJECT_FORWARDSIMPLE] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_forwardSimple.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_BLACKOUT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_blackout.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_TEXTUREONLY] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectPS_textureonly.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DIRLIGHT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DIRLIGHT_SOFT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightSoftPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_POINTLIGHT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "pointLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SPOTLIGHT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "spotLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOLUMELIGHT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "volumeLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DECAL] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "decalPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP_SKY] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMap_skyPS.cso", wiResourceManager::PIXELSHADER));

	geometryShaders[GSTYPE_ENVMAP] = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapGS.cso", wiResourceManager::GEOMETRYSHADER));
	geometryShaders[GSTYPE_ENVMAP_SKY] = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMap_skyGS.cso", wiResourceManager::GEOMETRYSHADER));

}
void wiRenderer::LoadLineShaders()
{
	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "linesVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr){
		vertexShaders[VSTYPE_LINE] = vsinfo->vertexShader;
		vertexLayouts[VLTYPE_LINE] = vsinfo->vertexLayout;
	}


	pixelShaders[PSTYPE_LINE] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "linesPS.cso", wiResourceManager::PIXELSHADER));

}
void wiRenderer::LoadTessShaders()
{
	hullShaders[HSTYPE_OBJECT] = static_cast<HullShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectHS.cso", wiResourceManager::HULLSHADER));
	domainShaders[DSTYPE_OBJECT] = static_cast<DomainShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "objectDS.cso", wiResourceManager::DOMAINSHADER));

}
void wiRenderer::LoadSkyShaders()
{
	vertexShaders[VSTYPE_SKY] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	pixelShaders[PSTYPE_SKY] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyPS.cso", wiResourceManager::PIXELSHADER));
	vertexShaders[VSTYPE_SKY_REFLECTION] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyVS_reflection.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	pixelShaders[PSTYPE_SKY_REFLECTION] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyPS_reflection.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SUN] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "sunPS.cso", wiResourceManager::PIXELSHADER));
}
void wiRenderer::LoadShadowShaders()
{

	vertexShaders[VSTYPE_SHADOW] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_SHADOWCUBEMAPRENDER] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;

	pixelShaders[PSTYPE_SHADOW] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowPS.cso", wiResourceManager::PIXELSHADER));

	geometryShaders[GSTYPE_SHADOWCUBEMAPRENDER] = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowGS.cso", wiResourceManager::GEOMETRYSHADER));

}
void wiRenderer::LoadWaterShaders()
{

	vertexShaders[VSTYPE_WATER] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "waterVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	pixelShaders[PSTYPE_WATER]= static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "waterPS.cso", wiResourceManager::PIXELSHADER));

}
void wiRenderer::LoadTrailShaders(){
	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "trailVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr){
		vertexShaders[VSTYPE_TRAIL] = vsinfo->vertexShader;
		vertexLayouts[VLTYPE_TRAIL] = vsinfo->vertexLayout;
	}


	pixelShaders[PSTYPE_TRAIL] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "trailPS.cso", wiResourceManager::PIXELSHADER));

}

void wiRenderer::ReloadShaders(const string& path)
{

	if (path.length() > 0)
	{
		SHADERPATH = path;
	}

	graphicsDevice->LOCK();

	wiResourceManager::GetShaderManager()->CleanUp();
	LoadBasicShaders();
	LoadLineShaders();
	LoadTessShaders();
	LoadSkyShaders();
	LoadShadowShaders();
	LoadWaterShaders();
	LoadTrailShaders();
	wiHairParticle::LoadShaders();
	wiEmittedParticle::LoadShaders();
	wiFont::LoadShaders();
	wiImage::LoadShaders();
	wiLensFlare::LoadShaders();

	graphicsDevice->UNLOCK();
}


void wiRenderer::SetUpStates()
{
	SamplerDesc samplerDesc;
	samplerDesc.Filter = FILTER_MIN_MAG_LINEAR_MIP_POINT;
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
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_LINEAR_MIRROR]);

	samplerDesc.Filter = FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_LINEAR_CLAMP]);

	samplerDesc.Filter = FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_LINEAR_WRAP]);
	
	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_POINT_MIRROR]);
	
	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_POINT_WRAP]);
	
	
	samplerDesc.Filter = FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_POINT_CLAMP]);
	
	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MaxAnisotropy = 16;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_ANISO_CLAMP]);
	
	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_WRAP;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_ANISO_WRAP]);
	
	samplerDesc.Filter = FILTER_ANISOTROPIC;
	samplerDesc.AddressU = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = TEXTURE_ADDRESS_MIRROR;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_ANISO_MIRROR]);

	ZeroMemory( &samplerDesc, sizeof(SamplerDesc) );
	samplerDesc.Filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = COMPARISON_LESS_EQUAL;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_CMP_DEPTH]);
	
	RasterizerDesc rs;
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
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_FRONT]);

	
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=4;
	rs.DepthClipEnable=true;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_SHADOW]);

	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=5.0f;
	rs.DepthClipEnable=true;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_SHADOW_DOUBLESIDED]);

	rs.FillMode=FILL_WIREFRAME;
	rs.CullMode=CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=true;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_WIRE]);
	
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_DOUBLESIDED]);
	
	rs.FillMode=FILL_WIREFRAME;
	rs.CullMode=CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_WIRE_DOUBLESIDED]);
	
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_FRONT;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_BACK]);

	DepthStencilDesc dsd;
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
	// Create the depth stencil state.
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DEFAULT]);

	
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthEnable = false;
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
	// Create the depth stencil state.
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_STENCILREAD]);

	
	dsd.DepthEnable = false;
	dsd.StencilEnable = true;
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
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_STENCILREAD_MATCH]);


	dsd.DepthEnable = true;
	dsd.StencilEnable = false;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_LESS_EQUAL;
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DEPTHREAD]);

	dsd.DepthEnable = false;
	dsd.StencilEnable=false;
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_XRAY]);

	
	BlendDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=false;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_MAX;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bd.AlphaToCoverageEnable=false;
	graphicsDevice->CreateBlendState(&bd,&blendStates[BSTYPE_OPAQUE]);

	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bd.AlphaToCoverageEnable=false;
	graphicsDevice->CreateBlendState(&bd,&blendStates[BSTYPE_TRANSPARENT]);


	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_MAX;
	bd.IndependentBlendEnable=true,
	bd.AlphaToCoverageEnable=false;
	graphicsDevice->CreateBlendState(&bd,&blendStates[BSTYPE_ADDITIVE]);
}

void wiRenderer::BindPersistentState(GRAPHICSTHREAD threadID)
{
	graphicsDevice->LOCK();

	for (int i = 0; i < SSLOT_COUNT; ++i)
	{
		if (samplers[i] == nullptr)
			continue;
		graphicsDevice->BindSamplerPS(samplers[i], i, threadID);
		graphicsDevice->BindSamplerVS(samplers[i], i, threadID);
		graphicsDevice->BindSamplerGS(samplers[i], i, threadID);
		graphicsDevice->BindSamplerDS(samplers[i], i, threadID);
		graphicsDevice->BindSamplerHS(samplers[i], i, threadID);
	}


	graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	graphicsDevice->BindConstantBufferGS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	graphicsDevice->BindConstantBufferHS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);
	graphicsDevice->BindConstantBufferDS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), threadID);

	graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	graphicsDevice->BindConstantBufferGS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	graphicsDevice->BindConstantBufferHS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);
	graphicsDevice->BindConstantBufferDS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), threadID);

	graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	graphicsDevice->BindConstantBufferGS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	graphicsDevice->BindConstantBufferHS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);
	graphicsDevice->BindConstantBufferDS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), threadID);

	graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), threadID);
	graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), threadID);
	graphicsDevice->BindConstantBufferGS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), threadID);
	graphicsDevice->BindConstantBufferHS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), threadID);
	graphicsDevice->BindConstantBufferDS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), threadID);

	graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_DIRLIGHT], CB_GETBINDSLOT(DirectionalLightCB), threadID);
	graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_DIRLIGHT], CB_GETBINDSLOT(DirectionalLightCB), threadID);

	graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	graphicsDevice->BindConstantBufferGS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	graphicsDevice->BindConstantBufferDS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);
	graphicsDevice->BindConstantBufferHS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), threadID);

	graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_SHADOW], CB_GETBINDSLOT(ShadowCB), threadID);

	graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_CLIPPLANE], CB_GETBINDSLOT(APICB), threadID);

	graphicsDevice->UNLOCK();
}
void wiRenderer::RebindPersistentState(GRAPHICSTHREAD threadID)
{
	BindPersistentState(threadID);

	wiImage::BindPersistentState(threadID);
	wiFont::BindPersistentState(threadID);
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
Vertex wiRenderer::TransformVertex(const Mesh* mesh, const SkinnedVertex& vertex, const XMMATRIX& mat){
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
	if(inWei[0] || inWei[1] || inWei[2] || inWei[3]){
		sump = XMMATRIX(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
		float sumw = 0;
		for(unsigned int i=0;i<4;i++){
			sumw += inWei[i];
			sump += XMLoadFloat4x4( &mesh->armature->boneCollection[int(inBon[i])]->boneRelativity ) * inWei[i];
		}
		if(sumw) sump/=sumw;

		//sump = XMMatrixTranspose(sump);
	}
	else
		sump = XMMatrixIdentity();

	//sump*=mat;
	sump = XMMatrixMultiply(sump, mat);

	XMFLOAT3 transformedP,transformedN;
	XMStoreFloat3( &transformedP,XMVector3Transform(pos,sump) );
	
	sump.r[3]=XMVectorSetX(sump.r[3],0);
	sump.r[3]=XMVectorSetY(sump.r[3],0);
	sump.r[3]=XMVectorSetZ(sump.r[3],0);
	//sump.r[3].m128_f32[0]=sump.r[3].m128_f32[1]=sump.r[3].m128_f32[2]=0;
	XMStoreFloat3( &transformedN,XMVector3Normalize(XMVector3Transform(nor,sump)));

	Vertex retV(transformedP);
	retV.nor = XMFLOAT4(transformedN.x, transformedN.y, transformedN.z,retV.nor.w);
	retV.tex = vertex.tex;
	retV.pre=XMFLOAT4(0,0,0,1);

	return retV;
}
XMFLOAT3 wiRenderer::VertexVelocity(const Mesh* mesh, const int& vertexI){
	XMVECTOR pos = XMLoadFloat4( &mesh->vertices[vertexI].pos );
	float inWei[4]={mesh->vertices[vertexI].wei.x
		,mesh->vertices[vertexI].wei.y
		,mesh->vertices[vertexI].wei.z
		,mesh->vertices[vertexI].wei.w};
	float inBon[4]={mesh->vertices[vertexI].bon.x
		,mesh->vertices[vertexI].bon.y
		,mesh->vertices[vertexI].bon.z
		,mesh->vertices[vertexI].bon.w};
	XMMATRIX sump;
	XMMATRIX sumpPrev;
	if(inWei[0] || inWei[1] || inWei[2] || inWei[3]){
		sump = sumpPrev = XMMATRIX(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
		float sumw = 0;
		for(unsigned int i=0;i<4;i++){
			sumw += inWei[i];
			sump += XMLoadFloat4x4( &mesh->armature->boneCollection[int(inBon[i])]->boneRelativity ) * inWei[i];
			sumpPrev += XMLoadFloat4x4( &mesh->armature->boneCollection[int(inBon[i])]->boneRelativityPrev ) * inWei[i];
		}
		if(sumw){
			sump/=sumw;
			sumpPrev/=sumw;
		}
	}
	else
		sump = sumpPrev = XMMatrixIdentity();
	XMFLOAT3 velocity;
	XMStoreFloat3( &velocity,GetGameSpeed()*XMVectorSubtract(XMVector3Transform(pos,sump),XMVector3Transform(pos,sumpPrev)) );
	return velocity;
}
void wiRenderer::Update()
{
	cam->UpdateTransform();

	objectsWithTrails.clear();
	emitterSystems.clear();


	GetScene().Update();

	refCam->Reflect(cam);
}
void wiRenderer::UpdatePerFrameData()
{
	if (GetGameSpeed() > 0)
	{

		UpdateSPTree(spTree);
		UpdateSPTree(spTree_lights);
	}

	UpdateBoneLines();
	UpdateCubes();

	GetScene().wind.time = (float)((wiTimer::TotalTime()) / 1000.0*GameSpeed / 2.0*3.1415)*XMVectorGetX(XMVector3Length(XMLoadFloat3(&GetScene().wind.direction)))*0.1f;

}
void wiRenderer::UpdateRenderData(GRAPHICSTHREAD threadID)
{
	//UpdateWorldCB(threadID);
	UpdateFrameCB(threadID);
	UpdateCameraCB(threadID);

	
	{
		bool streamOutSetUp = false;


		for (Model* model : GetScene().models)
		{
			for (MeshCollection::iterator iter = model->meshes.begin(); iter != model->meshes.end(); ++iter)
			{
				Mesh* mesh = iter->second;

				if (mesh->hasArmature() && !mesh->softBody && mesh->renderable && !mesh->vertices.empty()
					&& mesh->sOutBuffer != nullptr && mesh->meshVertBuff != nullptr)
				{
#ifdef USE_GPU_SKINNING
					if (!streamOutSetUp)
					{
						streamOutSetUp = true;
						wiRenderer::graphicsDevice->BindPrimitiveTopology(POINTLIST, threadID);
						wiRenderer::graphicsDevice->BindVertexLayout(vertexLayouts[VLTYPE_STREAMOUT], threadID);
						wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_STREAMOUT], threadID);
						wiRenderer::graphicsDevice->BindGS(geometryShaders[GSTYPE_STREAMOUT], threadID);
						wiRenderer::graphicsDevice->BindPS(nullptr, threadID);
						wiRenderer::graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_BONEBUFFER], CB_GETBINDSLOT(BoneCB), threadID);
						wiRenderer::graphicsDevice->BindConstantBufferGS(constantBuffers[CBTYPE_BONEBUFFER], CB_GETBINDSLOT(BoneCB), threadID);
					}

					// Upload bones for skinning to shader
					static thread_local BoneCB* bonebuf = new BoneCB;
					for (unsigned int k = 0; k < mesh->armature->boneCollection.size(); k++) {
						bonebuf->pose[k] = XMMatrixTranspose(XMLoadFloat4x4(&mesh->armature->boneCollection[k]->boneRelativity));
						bonebuf->prev[k] = XMMatrixTranspose(XMLoadFloat4x4(&mesh->armature->boneCollection[k]->boneRelativityPrev));
					}
					wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_BONEBUFFER], bonebuf, threadID);


					// Do the skinning
					wiRenderer::graphicsDevice->BindVertexBuffer(mesh->meshVertBuff, 0, sizeof(SkinnedVertex), threadID);
					wiRenderer::graphicsDevice->BindStreamOutTarget(mesh->sOutBuffer, threadID);
					wiRenderer::graphicsDevice->Draw(mesh->vertices.size(), threadID);
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
					wiRenderer::graphicsDevice->UpdateBuffer(mesh->meshVertBuff, mesh->skinnedVertices.data(), threadID, sizeof(Vertex)*mesh->skinnedVertices.size());
				}
			}
		}

#ifdef USE_GPU_SKINNING
		if (streamOutSetUp)
		{
			// Unload skinning shader
			wiRenderer::graphicsDevice->BindGS(nullptr, threadID);
			wiRenderer::graphicsDevice->BindVS(nullptr, threadID);
			wiRenderer::graphicsDevice->BindStreamOutTarget(nullptr, threadID);
		}
#endif


	}
	

}
void wiRenderer::UpdateImages(){
	for (wiSprite* x : images)
		x->Update(GameSpeed);
	for (wiSprite* x : waterRipples)
		x->Update(GameSpeed);

	ManageImages();
	ManageWaterRipples();
}
void wiRenderer::ManageImages(){
		while(	
				!images.empty() && 
				(images.front()->effects.opacity <= 0 + FLT_EPSILON || images.front()->effects.fade==1)
			)
			images.pop_front();
}
void wiRenderer::PutDecal(Decal* decal)
{
	GetScene().GetWorldNode()->decals.push_back(decal);
}
void wiRenderer::PutWaterRipple(const string& image, const XMFLOAT3& pos, const wiWaterPlane& waterPlane){
	wiSprite* img=new wiSprite("","",image);
	img->anim.fad=0.01f;
	img->anim.scaleX=0.2f;
	img->anim.scaleY=0.2f;
	img->effects.pos=pos;
	img->effects.rotation=(wiRandom::getRandom(0,1000)*0.001f)*2*3.1415f;
	img->effects.siz=XMFLOAT2(1,1);
	img->effects.typeFlag=WORLD;
	img->effects.quality=QUALITY_ANISOTROPIC;
	img->effects.pivotFlag=Pivot::CENTER;
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
void wiRenderer::DrawWaterRipples(GRAPHICSTHREAD threadID){
	//wiImage::BatchBegin(threadID);
	for(wiSprite* i:waterRipples){
		i->DrawNormal(threadID);
	}
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
		wiRenderer::graphicsDevice->BindPrimitiveTopology(LINELIST,threadID);
		wiRenderer::graphicsDevice->BindVertexLayout(vertexLayouts[VLTYPE_LINE],threadID);
	
		wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_SHADOW],threadID);
		wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_XRAY],STENCILREF_EMPTY,threadID);
		wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_OPAQUE],threadID);


		wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_LINE],threadID);
		wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_LINE],threadID);

		static thread_local MiscCB* sb = new MiscCB;
		for (unsigned int i = 0; i<boneLines.size(); i++){
			(*sb).mTransform = XMMatrixTranspose(XMLoadFloat4x4(&boneLines[i]->desc.transform)*camera->GetViewProjection());
			(*sb).mColor = boneLines[i]->desc.color;

			wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MISC], sb, threadID);

			wiRenderer::graphicsDevice->BindVertexBuffer(boneLines[i]->vertexBuffer, 0, sizeof(XMFLOAT3A), threadID);
			wiRenderer::graphicsDevice->Draw(2, threadID);
		}
	}
}
void wiRenderer::DrawDebugLines(Camera* camera, GRAPHICSTHREAD threadID)
{
	if (linesTemp.empty())
		return;

	wiRenderer::graphicsDevice->BindPrimitiveTopology(LINELIST, threadID);
	wiRenderer::graphicsDevice->BindVertexLayout(vertexLayouts[VLTYPE_LINE], threadID);

	wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_SHADOW], threadID);
	wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_XRAY], STENCILREF_EMPTY, threadID);
	wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);


	wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_LINE], threadID);
	wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_LINE], threadID);

	static thread_local MiscCB* sb = new MiscCB;
	for (unsigned int i = 0; i<linesTemp.size(); i++){
		(*sb).mTransform = XMMatrixTranspose(XMLoadFloat4x4(&linesTemp[i]->desc.transform)*camera->GetViewProjection());
		(*sb).mColor = linesTemp[i]->desc.color;

		wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MISC], sb, threadID);

		wiRenderer::graphicsDevice->BindVertexBuffer(linesTemp[i]->vertexBuffer, 0, sizeof(XMFLOAT3A), threadID);
		wiRenderer::graphicsDevice->Draw(2, threadID);
	}

	for (Lines* x : linesTemp)
		delete x;
	linesTemp.clear();
}
void wiRenderer::DrawDebugBoxes(Camera* camera, GRAPHICSTHREAD threadID)
{
	if(debugBoxes){
		wiRenderer::graphicsDevice->BindPrimitiveTopology(LINELIST,threadID);
		wiRenderer::graphicsDevice->BindVertexLayout(vertexLayouts[VLTYPE_LINE],threadID);

		wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_WIRE_DOUBLESIDED],threadID);
		wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_XRAY],STENCILREF_EMPTY,threadID);
		wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_OPAQUE],threadID);


		wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_LINE],threadID);
		wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_LINE],threadID);
		
		wiRenderer::graphicsDevice->BindVertexBuffer(Cube::vertexBuffer,0,sizeof(XMFLOAT3A),threadID);
		wiRenderer::graphicsDevice->BindIndexBuffer(Cube::indexBuffer,threadID);

		static thread_local MiscCB* sb = new MiscCB;
		for (unsigned int i = 0; i<cubes.size(); i++){
			(*sb).mTransform =XMMatrixTranspose(XMLoadFloat4x4(&cubes[i].desc.transform)*camera->GetViewProjection());
			(*sb).mColor=cubes[i].desc.color;

			wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MISC],sb,threadID);

			wiRenderer::graphicsDevice->DrawIndexed(24,threadID);
		}

	}
}

void wiRenderer::DrawSoftParticles(Camera* camera, GRAPHICSTHREAD threadID, bool dark)
{
	struct particlesystem_comparator {
		bool operator() (const wiEmittedParticle* a, const wiEmittedParticle* b) const{
			return a->lastSquaredDistMulThousand>b->lastSquaredDistMulThousand;
		}
	};

	set<wiEmittedParticle*,particlesystem_comparator> psystems;
	for(wiEmittedParticle* e : emitterSystems){
		e->lastSquaredDistMulThousand=(long)(wiMath::DistanceEstimated(e->bounding_box->getCenter(),camera->translation)*1000);
		psystems.insert(e);
	}

	for(wiEmittedParticle* e:psystems){
		e->DrawNonPremul(camera,threadID,dark);
	}
}
void wiRenderer::DrawSoftPremulParticles(Camera* camera, GRAPHICSTHREAD threadID, bool dark)
{
	for (wiEmittedParticle* e : emitterSystems)
	{
		e->DrawPremul(camera, threadID, dark);
	}
}
void wiRenderer::DrawTrails(GRAPHICSTHREAD threadID, Texture2D* refracRes){
	wiRenderer::graphicsDevice->BindPrimitiveTopology(TRIANGLESTRIP,threadID);
	wiRenderer::graphicsDevice->BindVertexLayout(vertexLayouts[VLTYPE_TRAIL],threadID);

	wiRenderer::graphicsDevice->BindRasterizerState(wireRender?rasterizers[RSTYPE_WIRE_DOUBLESIDED]:rasterizers[RSTYPE_DOUBLESIDED],threadID);
	wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT],STENCILREF_EMPTY,threadID);
	wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_OPAQUE],threadID);

	wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_TRAIL],threadID);
	wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_TRAIL],threadID);
	
	wiRenderer::graphicsDevice->BindTexturePS(refracRes,TEXSLOT_ONDEMAND0,threadID);

	for (Object* o : objectsWithTrails)
	{
		if(o->trailBuff && o->trail.size()>=4){

			wiRenderer::graphicsDevice->BindTexturePS(o->trailDistortTex, TEXSLOT_ONDEMAND1, threadID);
			wiRenderer::graphicsDevice->BindTexturePS(o->trailTex, TEXSLOT_ONDEMAND2, threadID);

			vector<RibbonVertex> trails;

			int bounds = o->trail.size();
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
				wiRenderer::graphicsDevice->UpdateBuffer(o->trailBuff,trails.data(),threadID,sizeof(RibbonVertex)*trails.size());

				wiRenderer::graphicsDevice->BindVertexBuffer(o->trailBuff,0,sizeof(RibbonVertex),threadID);
				wiRenderer::graphicsDevice->Draw(trails.size(),threadID);

				trails.clear();
			}
		}
	}
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
void wiRenderer::DrawLights(Camera* camera, GRAPHICSTHREAD threadID, unsigned int stencilRef){

	
	static thread_local Frustum frustum = Frustum();
	frustum.ConstructFrustum(min(camera->zFarP, GetScene().worldInfo.fogSEH.y),camera->Projection,camera->View);
	
	CulledList culledObjects;
	if(spTree_lights)
		wiSPTree::getVisible(spTree_lights->root,frustum,culledObjects);

	if(!culledObjects.empty())
	{


		wiRenderer::graphicsDevice->BindPrimitiveTopology(TRIANGLELIST,threadID);
	
	//BindTexturePS(depth,0,threadID);
	//BindTexturePS(normal,1,threadID);
	//BindTexturePS(material,2,threadID);

	
	wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_ADDITIVE],threadID);
	wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_STENCILREAD],stencilRef,threadID);
	wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_BACK],threadID);

	wiRenderer::graphicsDevice->BindVertexLayout(nullptr, threadID);
	wiRenderer::graphicsDevice->BindVertexBuffer(nullptr, 0, 0, threadID);
	wiRenderer::graphicsDevice->BindIndexBuffer(nullptr, threadID);

	wiRenderer::graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_POINTLIGHT], CB_GETBINDSLOT(PointLightCB), threadID);
	wiRenderer::graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_POINTLIGHT], CB_GETBINDSLOT(PointLightCB), threadID);

	wiRenderer::graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_SPOTLIGHT], CB_GETBINDSLOT(SpotLightCB), threadID);
	wiRenderer::graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_SPOTLIGHT], CB_GETBINDSLOT(SpotLightCB), threadID);

	for(int type=0;type<3;++type){

			
		wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_DIRLIGHT + type],threadID);

		switch (type)
		{
		case 0:
			if (SOFTSHADOW)
			{
				wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_DIRLIGHT_SOFT], threadID);
			}
			else
			{
				wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_DIRLIGHT], threadID);
			}
			break;
		case 1:
			wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_POINTLIGHT], threadID);
			break;
		case 2:
			wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_SPOTLIGHT], threadID);
			break;
		default:
			break;
		}


		for(Cullable* c : culledObjects){
			Light* l = (Light*)c;
			if (l->type != type)
				continue;
			
			if(type==0) //dir
			{
				static thread_local DirectionalLightCB* lcb = new DirectionalLightCB;
				(*lcb).direction=XMVector3Normalize(
					-XMVector3Transform( XMVectorSet(0,-1,0,1), XMMatrixRotationQuaternion( XMLoadFloat4(&l->rotation) ) )
					);
				(*lcb).col=XMFLOAT4(l->color.x*l->enerDis.x,l->color.y*l->enerDis.x,l->color.z*l->enerDis.x,1);
				(*lcb).mBiasResSoftshadow=XMFLOAT4(l->shadowBias,(float)SHADOWMAPRES,(float)SOFTSHADOW,0);
				for (unsigned int shmap = 0; shmap < l->shadowMaps_dirLight.size(); ++shmap){
					(*lcb).mShM[shmap]=l->shadowCam[shmap].getVP();
					if(l->shadowMaps_dirLight[shmap].depth)
						wiRenderer::graphicsDevice->BindTexturePS(l->shadowMaps_dirLight[shmap].depth->GetTexture(),TEXSLOT_SHADOW0+shmap,threadID);
				}
				wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_DIRLIGHT],lcb,threadID);

				wiRenderer::graphicsDevice->Draw(3, threadID);
			}
			else if(type==1) //point
			{
				static thread_local PointLightCB* lcb = new PointLightCB;
				(*lcb).pos=l->translation;
				(*lcb).col=l->color;
				(*lcb).enerdis=l->enerDis;
				(*lcb).enerdis.w = 0.f;

				if (l->shadow && l->shadowMap_index>=0)
				{
					(*lcb).enerdis.w = 1.f;
					if(Light::shadowMaps_pointLight[l->shadowMap_index].depth)
						wiRenderer::graphicsDevice->BindTexturePS(Light::shadowMaps_pointLight[l->shadowMap_index].depth->GetTexture(), TEXSLOT_SHADOW_CUBE, threadID);
				}
				wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_POINTLIGHT], lcb, threadID);

				wiRenderer::graphicsDevice->Draw(240, threadID);
			}
			else if(type==2) //spot
			{
				static thread_local SpotLightCB* lcb = new SpotLightCB;
				const float coneS=(const float)(l->enerDis.z/0.7853981852531433);
				XMMATRIX world,rot;
				world = XMMatrixTranspose(
						XMMatrixScaling(coneS*l->enerDis.y,l->enerDis.y,coneS*l->enerDis.y)*
						XMMatrixRotationQuaternion( XMLoadFloat4( &l->rotation ) )*
						XMMatrixTranslationFromVector( XMLoadFloat3(&l->translation) )
						);
				rot=XMMatrixRotationQuaternion( XMLoadFloat4(&l->rotation) );
				(*lcb).direction=XMVector3Normalize(
					-XMVector3Transform( XMVectorSet(0,-1,0,1), rot )
					);
				(*lcb).world=world;
				(*lcb).mBiasResSoftshadow=XMFLOAT4(l->shadowBias,(float)SPOTLIGHTSHADOWRES,(float)SOFTSHADOW,0);
				(*lcb).mShM = XMMatrixIdentity();
				(*lcb).col=l->color;
				(*lcb).enerdis=l->enerDis;
				(*lcb).enerdis.z=(float)cos(l->enerDis.z/2.0);

				if (l->shadow && l->shadowMap_index>=0)
				{
					(*lcb).mShM = l->shadowCam[0].getVP();
					if(Light::shadowMaps_spotLight[l->shadowMap_index].depth)
						wiRenderer::graphicsDevice->BindTexturePS(Light::shadowMaps_spotLight[l->shadowMap_index].depth->GetTexture(), TEXSLOT_SHADOW0, threadID);
				}
				wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_SPOTLIGHT], lcb, threadID);

				wiRenderer::graphicsDevice->Draw(192, threadID);
			}
		}


	}

	}
}
void wiRenderer::DrawVolumeLights(Camera* camera, GRAPHICSTHREAD threadID)
{
	
	static thread_local Frustum frustum = Frustum();
	frustum.ConstructFrustum(min(camera->zFarP, GetScene().worldInfo.fogSEH.y), camera->Projection, camera->View);

		
		CulledList culledObjects;
		if(spTree_lights)
			wiSPTree::getVisible(spTree_lights->root,frustum,culledObjects);

		if(!culledObjects.empty())
		{

		wiRenderer::graphicsDevice->BindPrimitiveTopology(TRIANGLELIST,threadID);
		wiRenderer::graphicsDevice->BindVertexLayout(nullptr);
		wiRenderer::graphicsDevice->BindVertexBuffer(nullptr, 0, 0, threadID);
		wiRenderer::graphicsDevice->BindIndexBuffer(nullptr, threadID);
		wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_ADDITIVE],threadID);
		wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD],STENCILREF_DEFAULT,threadID);
		wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_DOUBLESIDED],threadID);

		
		wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_VOLUMELIGHT],threadID);

		wiRenderer::graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), threadID);
		wiRenderer::graphicsDevice->BindConstantBufferVS(constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), threadID);


		for(int type=1;type<3;++type){

			
			if(type<=1){
				wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_VOLUMEPOINTLIGHT],threadID);
			}
			else{
				wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_VOLUMESPOTLIGHT],threadID);
			}

			for(Cullable* c : culledObjects){
				Light* l = (Light*)c;
				if(l->type==type && l->noHalo==false){

					static thread_local VolumeLightCB* lcb = new VolumeLightCB;
					XMMATRIX world;
					float sca=1;
					//if(type<1){ //sun
					//	sca = 10000;
					//	world = XMMatrixTranspose(
					//		XMMatrixScaling(sca,sca,sca)*
					//		XMMatrixRotationX(wiRenderer::getCamera()->updownRot)*XMMatrixRotationY(wiRenderer::getCamera()->leftrightRot)*
					//		XMMatrixTranslationFromVector( XMVector3Transform(wiRenderer::getCamera()->Eye+XMVectorSet(0,100000,0,0),XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))) )
					//		);
					//}
					if(type==1){ //point
						sca = l->enerDis.y*l->enerDis.x*0.01f;
						world = XMMatrixTranspose(
							XMMatrixScaling(sca,sca,sca)*
							XMMatrixRotationQuaternion(XMLoadFloat4(&camera->rotation))*
							XMMatrixTranslationFromVector( XMLoadFloat3(&l->translation) )
							);
					}
					else{ //spot
						float coneS=(float)(l->enerDis.z/0.7853981852531433);
						sca = l->enerDis.y*l->enerDis.x*0.03f;
						world = XMMatrixTranspose(
							XMMatrixScaling(coneS*sca,sca,coneS*sca)*
							XMMatrixRotationQuaternion( XMLoadFloat4( &l->rotation ) )*
							XMMatrixTranslationFromVector( XMLoadFloat3(&l->translation) )
							);
					}
					(*lcb).world=world;
					(*lcb).col=l->color;
					(*lcb).enerdis=l->enerDis;
					(*lcb).enerdis.w=sca;

					wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_VOLUMELIGHT],lcb,threadID);

					if(type<=1)
						wiRenderer::graphicsDevice->Draw(108,threadID);
					else
						wiRenderer::graphicsDevice->Draw(192,threadID);
				}
			}

		}

	}
}


void wiRenderer::DrawLensFlares(GRAPHICSTHREAD threadID){
	
		
	CulledList culledObjects;
	if(spTree_lights)
		wiSPTree::getVisible(spTree_lights->root,cam->frustum,culledObjects);

	for(Cullable* c:culledObjects)
	{
		Light* l = (Light*)c;

		if(!l->lensFlareRimTextures.empty())
		{

			XMVECTOR POS;

			if(l->type==Light::POINT || l->type==Light::SPOT){
				POS=XMLoadFloat3(&l->translation);
			}

			else{
				POS=XMVector3Normalize(
							-XMVector3Transform( XMVectorSet(0,-1,0,1), XMMatrixRotationQuaternion( XMLoadFloat4(&l->rotation) ) )
							)*100000;
			}
			
			XMVECTOR flarePos = XMVector3Project(POS,0.f,0.f,(float)GetScreenWidth(),(float)GetScreenHeight(),0.1f,1.0f,getCamera()->GetProjection(),getCamera()->GetView(),XMMatrixIdentity());

			if( XMVectorGetX(XMVector3Dot( XMVectorSubtract(POS,getCamera()->GetEye()),getCamera()->GetAt() ))>0 )
				wiLensFlare::Draw(threadID,flarePos,l->lensFlareRimTextures);

		}

	}
}
void wiRenderer::ClearShadowMaps(GRAPHICSTHREAD threadID){
	if (GetGameSpeed())
	{
		wiRenderer::graphicsDevice->UnbindTextures(TEXSLOT_SHADOW0, 1 + TEXSLOT_SHADOW_CUBE - TEXSLOT_SHADOW0, threadID);

		for (unsigned int index = 0; index < Light::shadowMaps_pointLight.size(); ++index) {
			Light::shadowMaps_pointLight[index].Activate(threadID);
		}
		for (unsigned int index = 0; index < Light::shadowMaps_spotLight.size(); ++index) {
			Light::shadowMaps_spotLight[index].Activate(threadID);
		}
	}
}
void wiRenderer::DrawForShadowMap(GRAPHICSTHREAD threadID)
{
	if (GameSpeed) {

		CulledList culledLights;
		if (spTree_lights)
			wiSPTree::getVisible(spTree_lights->root, cam->frustum, culledLights);

		if (culledLights.size() > 0)
		{

			wiRenderer::graphicsDevice->BindPrimitiveTopology(TRIANGLELIST, threadID);
			wiRenderer::graphicsDevice->BindVertexLayout(vertexLayouts[VLTYPE_EFFECT], threadID);


			wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_DEFAULT, threadID);

			wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);

			wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_SHADOW], threadID);

			wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_SHADOW], threadID);


			set<Light*, Cullable> orderedSpotLights;
			set<Light*, Cullable> orderedPointLights;
			set<Light*, Cullable> dirLights;
			for (Cullable* c : culledLights) {
				Light* l = (Light*)c;
				if (l->shadow)
				{
					l->shadowMap_index = -1;
					l->lastSquaredDistMulThousand = (long)(wiMath::DistanceSquared(l->translation, cam->translation) * 1000);
					switch (l->type)
					{
					case Light::SPOT:
						orderedSpotLights.insert(l);
						break;
					case Light::POINT:
						orderedPointLights.insert(l);
						break;
					default:
						dirLights.insert(l);
						break;
					}
				}
			}

			//DIRLIGHTS
			if (!dirLights.empty())
			{
				for (Light* l : dirLights)
				{
					for (int index = 0; index < 3; ++index)
					{
						CulledList culledObjects;
						CulledCollection culledRenderer;

						if (!l->shadowMaps_dirLight[index].IsInitialized() || l->shadowMaps_dirLight[index].depth->GetDesc().Height != SHADOWMAPRES)
						{
							// Create the shadow map
							l->shadowMaps_dirLight[index].Initialize(SHADOWMAPRES, SHADOWMAPRES, 0, true);
						}

						l->shadowMaps_dirLight[index].Activate(threadID);

						const float siz = l->shadowCam[index].size * 0.5f;
						const float f = l->shadowCam[index].farplane;
						AABB boundingbox;
						boundingbox.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(siz, f, siz));
						if (spTree)
							wiSPTree::getVisible(spTree->root, boundingbox.get(
								XMMatrixInverse(0, XMLoadFloat4x4(&l->shadowCam[index].View))
								), culledObjects);

#pragma region BLOAT
						{
							if (!culledObjects.empty()) {

								for (Cullable* object : culledObjects) {
									culledRenderer[((Object*)object)->mesh].insert((Object*)object);
								}

								for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
									Mesh* mesh = iter->first;
									CulledObjectList& visibleInstances = iter->second;

									if (visibleInstances.size() && !mesh->isBillboarded) {

										if (!mesh->doubleSided)
											wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_SHADOW], threadID);
										else
											wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_SHADOW_DOUBLESIDED], threadID);

										//MAPPED_SUBRESOURCE mappedResource;
										static thread_local ShadowCB* cb = new ShadowCB;
										(*cb).mVP = l->shadowCam[index].getVP();
										wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_SHADOW], cb, threadID);


										int k = 0;
										for (CulledObjectList::iterator viter = visibleInstances.begin(); viter != visibleInstances.end(); ++viter) {
											if ((*viter)->emitterType != Object::EmitterType::EMITTER_INVISIBLE) {
												if (mesh->softBody || (*viter)->isArmatureDeformed())
													Mesh::AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k);
												else
													Mesh::AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k);
												++k;
											}
										}
										if (k < 1)
											continue;

										Mesh::UpdateRenderableInstances(visibleInstances.size(), threadID);


										wiRenderer::graphicsDevice->BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), threadID);
										wiRenderer::graphicsDevice->BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), threadID);
										wiRenderer::graphicsDevice->BindIndexBuffer(mesh->meshIndexBuff, threadID);


										int matsiz = mesh->materialIndices.size();
										int m = 0;
										for (Material* iMat : mesh->materials) {

											if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
												wiRenderer::graphicsDevice->BindTexturePS(iMat->texture, TEXSLOT_ONDEMAND0, threadID);



												static thread_local MaterialCB* mcb = new MaterialCB;
												(*mcb).Create(*iMat, m);

												wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, threadID);


												wiRenderer::graphicsDevice->DrawIndexedInstanced(mesh->indices.size(), visibleInstances.size(), threadID);

												m++;
											}
										}
									}
								}

							}
						}
#pragma endregion
					}
				}
			}


			//SPOTLIGHTS
			if (!orderedSpotLights.empty())
			{
				int i = 0;
				for (Light* l : orderedSpotLights)
				{
					if (i >= SPOTLIGHTSHADOW)
						break;

					l->shadowMap_index = i;

					CulledList culledObjects;
					CulledCollection culledRenderer;

					Light::shadowMaps_spotLight[i].Set(threadID);
					static thread_local Frustum frustum = Frustum();
					XMFLOAT4X4 proj, view;
					XMStoreFloat4x4(&proj, XMLoadFloat4x4(&l->shadowCam[0].Projection));
					XMStoreFloat4x4(&view, XMLoadFloat4x4(&l->shadowCam[0].View));
					frustum.ConstructFrustum(wiRenderer::getCamera()->zFarP, proj, view);
					if (spTree)
						wiSPTree::getVisible(spTree->root, frustum, culledObjects);

					int index = 0;
#pragma region BLOAT
					{
						if (!culledObjects.empty()) {

							for (Cullable* object : culledObjects) {
								culledRenderer[((Object*)object)->mesh].insert((Object*)object);
							}

							for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
								Mesh* mesh = iter->first;
								CulledObjectList& visibleInstances = iter->second;

								if (visibleInstances.size() && !mesh->isBillboarded) {

									if (!mesh->doubleSided)
										wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_SHADOW], threadID);
									else
										wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_SHADOW_DOUBLESIDED], threadID);

									//MAPPED_SUBRESOURCE mappedResource;
									static thread_local ShadowCB* cb = new ShadowCB;
									(*cb).mVP = l->shadowCam[index].getVP();
									wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_SHADOW], cb, threadID);


									int k = 0;
									for (CulledObjectList::iterator viter = visibleInstances.begin(); viter != visibleInstances.end(); ++viter) {
										if ((*viter)->emitterType != Object::EmitterType::EMITTER_INVISIBLE) {
											if (mesh->softBody || (*viter)->isArmatureDeformed())
												Mesh::AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k);
											else
												Mesh::AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k);
											++k;
										}
									}
									if (k < 1)
										continue;

									Mesh::UpdateRenderableInstances(visibleInstances.size(), threadID);


									wiRenderer::graphicsDevice->BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), threadID);
									wiRenderer::graphicsDevice->BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), threadID);
									wiRenderer::graphicsDevice->BindIndexBuffer(mesh->meshIndexBuff, threadID);


									int matsiz = mesh->materialIndices.size();
									int m = 0;
									for (Material* iMat : mesh->materials) {

										if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
											wiRenderer::graphicsDevice->BindTexturePS(iMat->texture, TEXSLOT_ONDEMAND0, threadID);



											static thread_local MaterialCB* mcb = new MaterialCB;
											(*mcb).Create(*iMat, m);

											wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, threadID);


											wiRenderer::graphicsDevice->DrawIndexedInstanced(mesh->indices.size(), visibleInstances.size(), threadID);

											m++;
										}
									}
								}
							}

						}
					}
#pragma endregion


					i++;
				}
			}

			//POINTLIGHTS
			if(!orderedPointLights.empty())
			{
				wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER], threadID);
				wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_SHADOWCUBEMAPRENDER], threadID);
				wiRenderer::graphicsDevice->BindGS(geometryShaders[GSTYPE_SHADOWCUBEMAPRENDER], threadID);

				wiRenderer::graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_POINTLIGHT], CB_GETBINDSLOT(PointLightCB), threadID);
				wiRenderer::graphicsDevice->BindConstantBufferGS(constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubeMapRenderCB), threadID);

				int i = 0;
				for (Light* l : orderedPointLights)
				{
					if (i >= POINTLIGHTSHADOW)
						break; 
					
					l->shadowMap_index = i;

					Light::shadowMaps_pointLight[i].Set(threadID);

					//MAPPED_SUBRESOURCE mappedResource;
					static thread_local PointLightCB* lcb = new PointLightCB;
					(*lcb).enerdis = l->enerDis;
					(*lcb).pos = l->translation;
					wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_POINTLIGHT], lcb, threadID);

					static thread_local CubeMapRenderCB* cb = new CubeMapRenderCB;
					for (unsigned int shcam = 0; shcam < l->shadowCam.size(); ++shcam)
						(*cb).mViewProjection[shcam] = l->shadowCam[shcam].getVP();

					wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_CUBEMAPRENDER], cb, threadID);

					CulledList culledObjects;
					CulledCollection culledRenderer;

					if (spTree)
						wiSPTree::getVisible(spTree->root, l->bounds, culledObjects);

					for (Cullable* object : culledObjects)
						culledRenderer[((Object*)object)->mesh].insert((Object*)object);

					for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
						Mesh* mesh = iter->first;
						CulledObjectList& visibleInstances = iter->second;

						if (!mesh->isBillboarded && !visibleInstances.empty()) {

							if (!mesh->doubleSided)
								wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_SHADOW], threadID);
							else
								wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_SHADOW_DOUBLESIDED], threadID);



							int k = 0;
							for (CulledObjectList::iterator viter = visibleInstances.begin(); viter != visibleInstances.end(); ++viter) {
								if ((*viter)->emitterType != Object::EmitterType::EMITTER_INVISIBLE) {
									if (mesh->softBody || (*viter)->isArmatureDeformed())
										Mesh::AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k);
									else
										Mesh::AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k);
									++k;
								}
							}
							if (k < 1)
								continue;

							Mesh::UpdateRenderableInstances(visibleInstances.size(), threadID);


							wiRenderer::graphicsDevice->BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), threadID);
							wiRenderer::graphicsDevice->BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), threadID);
							wiRenderer::graphicsDevice->BindIndexBuffer(mesh->meshIndexBuff, threadID);


							int matsiz = mesh->materialIndices.size();
							int m = 0;
							for (Material* iMat : mesh->materials) {
								if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
									wiRenderer::graphicsDevice->BindTexturePS(iMat->texture, TEXSLOT_ONDEMAND0, threadID);

									static thread_local MaterialCB* mcb = new MaterialCB;
									(*mcb).Create(*iMat, m);

									wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, threadID);

									wiRenderer::graphicsDevice->DrawIndexedInstanced(mesh->indices.size(), visibleInstances.size(), threadID);
								}
								m++;
							}
						}
						visibleInstances.clear();
					}

					i++;
				}

				wiRenderer::graphicsDevice->BindGS(nullptr, threadID);
			}
		}
	}

}

void wiRenderer::SetDirectionalLightShadowProps(int resolution, int softShadowQuality)
{
	SHADOWMAPRES = resolution;
	SOFTSHADOW = softShadowQuality;
}
void wiRenderer::SetPointLightShadowProps(int shadowMapCount, int resolution)
{
	POINTLIGHTSHADOW = shadowMapCount;
	POINTLIGHTSHADOWRES = resolution;
	Light::shadowMaps_pointLight.clear();
	Light::shadowMaps_pointLight.resize(shadowMapCount);
	for (int i = 0; i < shadowMapCount; ++i)
	{
		Light::shadowMaps_pointLight[i].InitializeCube(POINTLIGHTSHADOWRES, 0, true);
	}
}
void wiRenderer::SetSpotLightShadowProps(int shadowMapCount, int resolution)
{
	SPOTLIGHTSHADOW = shadowMapCount;
	SPOTLIGHTSHADOWRES = resolution;
	Light::shadowMaps_spotLight.clear();
	Light::shadowMaps_spotLight.resize(shadowMapCount);
	for (int i = 0; i < shadowMapCount; ++i)
	{
		Light::shadowMaps_spotLight[i].Initialize(SPOTLIGHTSHADOWRES, SPOTLIGHTSHADOWRES, 1, true);
	}
}


void wiRenderer::DrawWorld(Camera* camera, bool DX11Eff, int tessF, GRAPHICSTHREAD threadID
				  , bool BlackOut, bool isReflection, SHADERTYPE shaded
				  , Texture2D* refRes, bool grass, GRAPHICSTHREAD thread)
{
	CulledCollection culledRenderer;
	CulledList culledObjects;
	if(spTree)
		wiSPTree::getVisible(spTree->root, camera->frustum,culledObjects,wiSPTree::SortType::SP_TREE_SORT_FRONT_TO_BACK);
	else return;

	if(!culledObjects.empty())
	{

		Texture2D* envMaps[] = { enviroMap, enviroMap };
		XMFLOAT3 envMapPositions[] = { XMFLOAT3(0,0,0),XMFLOAT3(0,0,0) };
		if (!GetScene().environmentProbes.empty())
		{
			// Get the closest probes to the camera and bind to shader
			vector<EnvironmentProbe*> sortedEnvProbes;
			vector<float> sortedDistances;
			sortedEnvProbes.reserve(GetScene().environmentProbes.size());
			sortedDistances.reserve(GetScene().environmentProbes.size());
			for (unsigned int i = 0; i < GetScene().environmentProbes.size(); ++i)
			{
				sortedEnvProbes.push_back(GetScene().environmentProbes[i]);
				sortedDistances.push_back(wiMath::DistanceSquared(camera->translation, GetScene().environmentProbes[i]->translation));
			}
			for (unsigned int i = 0; i < sortedEnvProbes.size() - 1; ++i)
			{
				for (unsigned int j = i + 1; j < sortedEnvProbes.size(); ++j)
				{
					if (sortedDistances[i] > sortedDistances[j])
					{
						float swapDist = sortedDistances[i];
						sortedDistances[i] = sortedDistances[j];
						sortedDistances[j] = swapDist;
						EnvironmentProbe* swapProbe = sortedEnvProbes[i];
						sortedEnvProbes[i] = sortedEnvProbes[j];
						sortedEnvProbes[j] = swapProbe;
					}
				}
			}
			for (unsigned int i = 0; i < min(sortedEnvProbes.size(), 2); ++i)
			{
				envMaps[i] = sortedEnvProbes[i]->cubeMap.GetTexture();
				envMapPositions[i] = sortedEnvProbes[i]->translation;
			}
		}
		static thread_local MiscCB* envProbeCB = new MiscCB;
		envProbeCB->mTransform.r[0] = XMLoadFloat3(&envMapPositions[0]);
		envProbeCB->mTransform.r[1] = XMLoadFloat3(&envMapPositions[1]);
		envProbeCB->mTransform = XMMatrixTranspose(envProbeCB->mTransform);
		wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MISC], envProbeCB, threadID);
		wiRenderer::graphicsDevice->BindTexturePS(envMaps[0], TEXSLOT_ENV0, threadID);
		wiRenderer::graphicsDevice->BindTexturePS(envMaps[1], TEXSLOT_ENV1, threadID);
		


		for(Cullable* object : culledObjects){
			culledRenderer[((Object*)object)->mesh].insert((Object*)object);
			if(grass){
				for(wiHairParticle* hair : ((Object*)object)->hParticleSystems){
					hair->Draw(camera,threadID);
				}
			}
		}

		if(DX11Eff && tessF) 
			wiRenderer::graphicsDevice->BindPrimitiveTopology(PATCHLIST,threadID);
		else		
			wiRenderer::graphicsDevice->BindPrimitiveTopology(TRIANGLELIST,threadID);
		wiRenderer::graphicsDevice->BindVertexLayout(vertexLayouts[VLTYPE_EFFECT],threadID);

		if(DX11Eff && tessF)
			wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_OBJECT],threadID);
		else
		{
			if (isReflection)
			{
				wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_OBJECT_REFLECTION], threadID);
			}
			else
			{
				wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_OBJECT10], threadID);
			}
		}
		if(DX11Eff && tessF)
			wiRenderer::graphicsDevice->BindHS(hullShaders[HSTYPE_OBJECT],threadID);
		else
			wiRenderer::graphicsDevice->BindHS(nullptr,threadID);
		if(DX11Eff && tessF) 
			wiRenderer::graphicsDevice->BindDS(domainShaders[DSTYPE_OBJECT],threadID);
		else		
			wiRenderer::graphicsDevice->BindDS(nullptr,threadID);

		if (wireRender)
			wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_SIMPLEST], threadID);
		else
			if (BlackOut)
				wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_BLACKOUT], threadID);
			else if (shaded == SHADERTYPE_NONE)
				wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_TEXTUREONLY], threadID);
			else if (shaded == SHADERTYPE_DEFERRED)
				wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_OBJECT], threadID);
			else if (shaded == SHADERTYPE_FORWARD_SIMPLE)
				wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_OBJECT_FORWARDSIMPLE], threadID);
			else
				return;


			wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_OPAQUE],threadID);

		if(!wireRender) {
			if (enviroMap != nullptr)
			{
				wiRenderer::graphicsDevice->BindTexturePS(enviroMap, TEXSLOT_ENV_GLOBAL, threadID);
			}
			else
			{
				wiRenderer::graphicsDevice->UnbindTextures(TEXSLOT_ENV_GLOBAL, 1, threadID);
			}
			if(refRes != nullptr) 
				wiRenderer::graphicsDevice->BindTexturePS(refRes,TEXSLOT_ONDEMAND5,threadID);
		}


		for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
			Mesh* mesh = iter->first;
			CulledObjectList& visibleInstances = iter->second;


			if(!mesh->doubleSided)
				wiRenderer::graphicsDevice->BindRasterizerState(wireRender?rasterizers[RSTYPE_WIRE]:rasterizers[RSTYPE_FRONT],threadID);
			else
				wiRenderer::graphicsDevice->BindRasterizerState(wireRender?rasterizers[RSTYPE_WIRE]:rasterizers[RSTYPE_DOUBLESIDED],threadID);

			int matsiz = mesh->materialIndices.size();

			int k=0;
			for(CulledObjectList::iterator viter=visibleInstances.begin();viter!=visibleInstances.end();++viter){
				if((*viter)->emitterType !=Object::EmitterType::EMITTER_INVISIBLE){
					if (mesh->softBody || (*viter)->isArmatureDeformed())
						Mesh::AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency, (*viter)->color), k);
					else 
						Mesh::AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency, (*viter)->color), k);
					++k;
				}
			}
			if(k<1)
				continue;

			Mesh::UpdateRenderableInstances(visibleInstances.size(), threadID);
				
				
			wiRenderer::graphicsDevice->BindIndexBuffer(mesh->meshIndexBuff,threadID);
			wiRenderer::graphicsDevice->BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),threadID);
			wiRenderer::graphicsDevice->BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),threadID);

			int m=0;
			for(Material* iMat : mesh->materials){

				if(!iMat->IsTransparent() && !iMat->isSky && !iMat->water){
					
					if(iMat->shadeless)
						wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT],STENCILREF_SHADELESS,threadID);
					if(iMat->subsurface_scattering)
						wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT],STENCILREF_SKIN,threadID);
					else
						wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT],mesh->stencilRef,threadID);


					static thread_local MaterialCB* mcb = new MaterialCB;
					(*mcb).Create(*iMat, m);

					wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, threadID);
					

					if(!wireRender) wiRenderer::graphicsDevice->BindTexturePS(iMat->texture, TEXSLOT_ONDEMAND0,threadID);
					if(!wireRender) wiRenderer::graphicsDevice->BindTexturePS(iMat->refMap, TEXSLOT_ONDEMAND1,threadID);
					if(!wireRender) wiRenderer::graphicsDevice->BindTexturePS(iMat->normalMap, TEXSLOT_ONDEMAND2,threadID);
					if(!wireRender) wiRenderer::graphicsDevice->BindTexturePS(iMat->specularMap, TEXSLOT_ONDEMAND3,threadID);
					if(DX11Eff)
						wiRenderer::graphicsDevice->BindTextureDS(iMat->displacementMap, TEXSLOT_ONDEMAND4,threadID);
					
					wiRenderer::graphicsDevice->DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),threadID);
				}
				m++;
			}

		}

		
		wiRenderer::graphicsDevice->BindPS(nullptr,threadID);
		wiRenderer::graphicsDevice->BindVS(nullptr,threadID);
		wiRenderer::graphicsDevice->BindDS(nullptr,threadID);
		wiRenderer::graphicsDevice->BindHS(nullptr,threadID);

	}

}

void wiRenderer::DrawWorldTransparent(Camera* camera, Texture2D* refracRes, Texture2D* refRes
	, Texture2D* waterRippleNormals, GRAPHICSTHREAD threadID)
{
	CulledCollection culledRenderer;
	CulledList culledObjects;
	if (spTree)
		wiSPTree::getVisible(spTree->root, camera->frustum,culledObjects, wiSPTree::SortType::SP_TREE_SORT_PAINTER);

	if(!culledObjects.empty())
	{
		//// sort transparents back to front
		//vector<Cullable*> sortedObjects(culledObjects.begin(), culledObjects.end());
		//for (unsigned int i = 0; i < sortedObjects.size() - 1; ++i)
		//{
		//	for (unsigned int j = 1; j < sortedObjects.size(); ++j)
		//	{
		//		if (wiMath::Distance(cam->translation, ((Object*)sortedObjects[i])->translation) <
		//			wiMath::Distance(cam->translation, ((Object*)sortedObjects[j])->translation))
		//		{
		//			Cullable* swap = sortedObjects[i];
		//			sortedObjects[i] = sortedObjects[j];
		//			sortedObjects[j] = swap;
		//		}
		//	}
		//}


		for(Cullable* object : culledObjects)
			culledRenderer[((Object*)object)->mesh].insert((Object*)object);

		wiRenderer::graphicsDevice->BindPrimitiveTopology(TRIANGLELIST,threadID);
		wiRenderer::graphicsDevice->BindVertexLayout(vertexLayouts[VLTYPE_EFFECT],threadID);
		wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_OBJECT10],threadID);

	
		if (!wireRender)
		{

			if (enviroMap != nullptr)
			{
				wiRenderer::graphicsDevice->BindTexturePS(enviroMap, TEXSLOT_ENV_GLOBAL, threadID);
			}
			else
			{
				wiRenderer::graphicsDevice->UnbindTextures(TEXSLOT_ENV_GLOBAL, 1, threadID);
			}
			wiRenderer::graphicsDevice->BindTexturePS(refRes, TEXSLOT_ONDEMAND5,threadID);
			wiRenderer::graphicsDevice->BindTexturePS(refracRes, TEXSLOT_ONDEMAND6,threadID);
			//BindTexturePS(depth,7,threadID);
			wiRenderer::graphicsDevice->BindTexturePS(waterRippleNormals, TEXSLOT_ONDEMAND7, threadID);
		}


		wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_OPAQUE],threadID);
	
		RENDERTYPE lastRenderType = RENDERTYPE_VOID;

		for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
			Mesh* mesh = iter->first;
			CulledObjectList& visibleInstances = iter->second;

			bool isValid = false;
			for (Material* iMat : mesh->materials)
			{
				if (iMat->IsTransparent() || iMat->IsWater())
				{
					isValid = true;
					break;
				}
			}
			if (!isValid)
				continue;

			if (!mesh->doubleSided)
				wiRenderer::graphicsDevice->BindRasterizerState(wireRender ? rasterizers[RSTYPE_WIRE] : rasterizers[RSTYPE_FRONT], threadID);
			else
				wiRenderer::graphicsDevice->BindRasterizerState(wireRender ? rasterizers[RSTYPE_WIRE] : rasterizers[RSTYPE_DOUBLESIDED], threadID);


			int matsiz = mesh->materialIndices.size();

				
			
			int k = 0;
			for (CulledObjectList::iterator viter = visibleInstances.begin(); viter != visibleInstances.end(); ++viter){
				if ((*viter)->emitterType != Object::EmitterType::EMITTER_INVISIBLE){
					if (mesh->softBody || (*viter)->isArmatureDeformed())
						Mesh::AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency, (*viter)->color), k);
					else
						Mesh::AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency, (*viter)->color), k);
					++k;
				}
			}
			if (k<1)
				continue;

			Mesh::UpdateRenderableInstances(visibleInstances.size(), threadID);

				
			wiRenderer::graphicsDevice->BindIndexBuffer(mesh->meshIndexBuff,threadID);
			wiRenderer::graphicsDevice->BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),threadID);
			wiRenderer::graphicsDevice->BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),threadID);
				

			int m=0;
			for(Material* iMat : mesh->materials){
				if (iMat->isSky)
					continue;

				if(iMat->IsTransparent() || iMat->IsWater())
				{
					static thread_local MaterialCB* mcb = new MaterialCB;
					(*mcb).Create(*iMat, m);

					wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, threadID);

					if(!wireRender) wiRenderer::graphicsDevice->BindTexturePS(iMat->texture, TEXSLOT_ONDEMAND0,threadID);
					if(!wireRender) wiRenderer::graphicsDevice->BindTexturePS(iMat->refMap, TEXSLOT_ONDEMAND1,threadID);
					if(!wireRender) wiRenderer::graphicsDevice->BindTexturePS(iMat->normalMap, TEXSLOT_ONDEMAND2,threadID);
					if(!wireRender) wiRenderer::graphicsDevice->BindTexturePS(iMat->specularMap, TEXSLOT_ONDEMAND3,threadID);

					if (iMat->IsTransparent() && lastRenderType != RENDERTYPE_TRANSPARENT)
					{
						lastRenderType = RENDERTYPE_TRANSPARENT;

						wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_TRANSPARENT, threadID);
						wiRenderer::graphicsDevice->BindPS(wireRender ? pixelShaders[PSTYPE_SIMPLEST] : pixelShaders[PSTYPE_OBJECT_TRANSPARENT], threadID);
					}
					if (iMat->IsWater() && lastRenderType != RENDERTYPE_WATER)
					{
						lastRenderType = RENDERTYPE_WATER;

						wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_EMPTY, threadID);
						wiRenderer::graphicsDevice->BindPS(wireRender ? pixelShaders[PSTYPE_SIMPLEST] : pixelShaders[PSTYPE_WATER], threadID);
						wiRenderer::graphicsDevice->BindRasterizerState(wireRender ? rasterizers[RSTYPE_WIRE] : rasterizers[RSTYPE_DOUBLESIDED], threadID);
					}
					
					wiRenderer::graphicsDevice->DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),threadID);
				}
				m++;
			}
		}
	}
	
}


void wiRenderer::DrawSky(GRAPHICSTHREAD threadID, bool isReflection)
{
	if (enviroMap == nullptr)
		return;

	wiRenderer::graphicsDevice->BindPrimitiveTopology(TRIANGLELIST,threadID);
	wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_BACK],threadID);
	wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD],STENCILREF_SKY,threadID);
	wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_OPAQUE],threadID);
	
	if (!isReflection)
	{
		wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_SKY], threadID);
		wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_SKY], threadID);
	}
	else
	{
		wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_SKY_REFLECTION], threadID);
		wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_SKY_REFLECTION], threadID);
	}
	
	wiRenderer::graphicsDevice->BindTexturePS(enviroMap, TEXSLOT_ENV_GLOBAL, threadID);

	wiRenderer::graphicsDevice->BindVertexBuffer(nullptr,0,0,threadID);
	wiRenderer::graphicsDevice->BindVertexLayout(nullptr, threadID);
	wiRenderer::graphicsDevice->Draw(240,threadID);
}
void wiRenderer::DrawSun(GRAPHICSTHREAD threadID)
{
	wiRenderer::graphicsDevice->BindPrimitiveTopology(TRIANGLELIST, threadID);
	wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_BACK], threadID);
	wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_SKY, threadID);
	wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_ADDITIVE], threadID);

	wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_SKY], threadID);
	wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_SUN], threadID);

	wiRenderer::graphicsDevice->BindVertexBuffer(nullptr, 0, 0, threadID);
	wiRenderer::graphicsDevice->BindVertexLayout(nullptr, threadID);
	wiRenderer::graphicsDevice->Draw(240, threadID);
}

void wiRenderer::DrawDecals(Camera* camera, GRAPHICSTHREAD threadID)
{
	bool boundCB = false;
	for (Model* model : GetScene().models)
	{
		if (model->decals.empty())
			continue;

		if (!boundCB)
		{
			boundCB = true;
			wiRenderer::graphicsDevice->BindConstantBufferPS(constantBuffers[CBTYPE_DECAL], CB_GETBINDSLOT(DecalCB),threadID);
		}

		//BindTexturePS(depth, 1, threadID);
		wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_DECAL], threadID);
		wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_DECAL], threadID);
		wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_BACK], threadID);
		wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_TRANSPARENT], threadID);
		wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_STENCILREAD_MATCH], STENCILREF::STENCILREF_DEFAULT, threadID);
		wiRenderer::graphicsDevice->BindVertexLayout(nullptr, threadID);
		wiRenderer::graphicsDevice->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST, threadID);

		for (Decal* decal : model->decals) {

			if ((decal->texture || decal->normal) && camera->frustum.CheckBox(decal->bounds.corners)) {

				wiRenderer::graphicsDevice->BindTexturePS(decal->texture, TEXSLOT_ONDEMAND0, threadID);
				wiRenderer::graphicsDevice->BindTexturePS(decal->normal, TEXSLOT_ONDEMAND1, threadID);

				static thread_local MiscCB* dcbvs = new MiscCB;
				(*dcbvs).mTransform =XMMatrixTranspose(XMLoadFloat4x4(&decal->world)*camera->GetViewProjection());
				wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MISC], dcbvs, threadID);

				static thread_local DecalCB* dcbps = new DecalCB;
				dcbps->mDecalVP =
					XMMatrixTranspose(
						XMLoadFloat4x4(&decal->view)*XMLoadFloat4x4(&decal->projection)
						);
				dcbps->hasTexNor = 0;
				if (decal->texture != nullptr)
					dcbps->hasTexNor |= 0x0000001;
				if (decal->normal != nullptr)
					dcbps->hasTexNor |= 0x0000010;
				XMStoreFloat3(&dcbps->eye, camera->GetEye());
				dcbps->opacity = wiMath::Clamp((decal->life <= -2 ? 1 : decal->life < decal->fadeStart ? decal->life / decal->fadeStart : 1), 0, 1);
				dcbps->front = decal->front;
				wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_DECAL], dcbps, threadID);

				wiRenderer::graphicsDevice->Draw(36, threadID);

			}

		}
	}
}

void wiRenderer::UpdateWorldCB(GRAPHICSTHREAD threadID)
{
	static thread_local WorldCB* cb = new WorldCB;

	auto& world = GetScene().worldInfo;
	(*cb).mAmbient = world.ambient;
	(*cb).mFog = world.fogSEH;
	(*cb).mHorizon = world.horizon;
	(*cb).mZenith = world.zenith;
	(*cb).mScreenWidthHeight = XMFLOAT2((float)GetScreenWidth(), (float)GetScreenHeight());
	XMStoreFloat4(&(*cb).mSun, GetSunPosition());
	(*cb).mSunColor = GetSunColor();

	wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_WORLD], cb, threadID);
}
void wiRenderer::UpdateFrameCB(GRAPHICSTHREAD threadID)
{
	static thread_local FrameCB* cb = new FrameCB;

	auto& wind = GetScene().wind;
	(*cb).mWindTime = wind.time;
	(*cb).mWindRandomness = wind.randomness;
	(*cb).mWindWaveSize = wind.waveSize;
	(*cb).mWindDirection = wind.direction;

	wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_FRAME], cb, threadID);
}
void wiRenderer::UpdateCameraCB(GRAPHICSTHREAD threadID)
{
	static thread_local CameraCB* cb = new CameraCB;

	auto camera = getCamera();
	auto prevCam = prevFrameCam;
	auto reflCam = getRefCamera();

	(*cb).mView = XMMatrixTranspose(camera->GetView());
	(*cb).mProj = XMMatrixTranspose(camera->GetProjection());
	(*cb).mVP = XMMatrixTranspose(camera->GetViewProjection());
	(*cb).mPrevV = XMMatrixTranspose(prevCam->GetView());
	(*cb).mPrevP = XMMatrixTranspose(prevCam->GetProjection());
	(*cb).mPrevVP = XMMatrixTranspose(prevCam->GetViewProjection());
	(*cb).mReflVP = XMMatrixTranspose(reflCam->GetViewProjection());
	(*cb).mInvP = XMMatrixInverse(nullptr, (*cb).mVP);
	(*cb).mCamPos = camera->translation;
	(*cb).mAt = camera->At;
	(*cb).mUp = camera->Up;
	(*cb).mZNearP = camera->zNearP;
	(*cb).mZFarP = camera->zFarP;

	wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_CAMERA], cb, threadID);
}
void wiRenderer::SetClipPlane(XMFLOAT4 clipPlane, GRAPHICSTHREAD threadID)
{
	wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_CLIPPLANE], &clipPlane, threadID);
}
void wiRenderer::UpdateGBuffer(Texture2D* slot0, Texture2D* slot1, Texture2D* slot2, Texture2D* slot3, Texture2D* slot4, GRAPHICSTHREAD threadID)
{
	wiRenderer::graphicsDevice->BindTexturePS(slot0, TEXSLOT_GBUFFER0, threadID);
	wiRenderer::graphicsDevice->BindTexturePS(slot1, TEXSLOT_GBUFFER1, threadID);
	wiRenderer::graphicsDevice->BindTexturePS(slot2, TEXSLOT_GBUFFER2, threadID);
	wiRenderer::graphicsDevice->BindTexturePS(slot3, TEXSLOT_GBUFFER3, threadID);
	wiRenderer::graphicsDevice->BindTexturePS(slot4, TEXSLOT_GBUFFER4, threadID);
}
void wiRenderer::UpdateDepthBuffer(Texture2D* depth, Texture2D* linearDepth, GRAPHICSTHREAD threadID)
{
	wiRenderer::graphicsDevice->BindTexturePS(depth, TEXSLOT_DEPTH, threadID);
	wiRenderer::graphicsDevice->BindTextureVS(depth, TEXSLOT_DEPTH, threadID);
	wiRenderer::graphicsDevice->BindTextureGS(depth, TEXSLOT_DEPTH, threadID);

	wiRenderer::graphicsDevice->BindTexturePS(linearDepth, TEXSLOT_LINEARDEPTH, threadID);
	wiRenderer::graphicsDevice->BindTextureVS(linearDepth, TEXSLOT_LINEARDEPTH, threadID);
	wiRenderer::graphicsDevice->BindTextureGS(linearDepth, TEXSLOT_LINEARDEPTH, threadID);
}

void wiRenderer::FinishLoading()
{
	// Kept for backwards compatibility
}

wiRenderer::Picked wiRenderer::Pick(RAY& ray, int pickType, const string& layer,
	const string& layerDisable)
{
	CulledCollection culledRenderer;
	CulledList culledObjects;
	wiSPTree* searchTree = spTree;
	if (searchTree)
	{
		wiSPTree::getVisible(searchTree->root, ray, culledObjects);

		vector<Picked> pickPoints;

		RayIntersectMeshes(ray, culledObjects, pickPoints, pickType, true, layer, layerDisable);

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
	XMVECTOR& lineStart = XMVector3Unproject(XMVectorSet((float)cursorX,(float)cursorY,0,1),0,0
		, (float)SCREENWIDTH, (float)SCREENHEIGHT, 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
	XMVECTOR& lineEnd = XMVector3Unproject(XMVectorSet((float)cursorX, (float)cursorY, 1, 1), 0, 0
		, (float)SCREENWIDTH, (float)SCREENHEIGHT, 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
	XMVECTOR& rayDirection = XMVector3Normalize(XMVectorSubtract(lineEnd,lineStart));
	return RAY(lineStart,rayDirection);
}

void wiRenderer::RayIntersectMeshes(const RAY& ray, const CulledList& culledObjects, vector<Picked>& points,
	int pickType, bool dynamicObjects, const string& layer, const string& layerDisable)
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
	XMVECTOR& rayDirection = XMLoadFloat3(&ray.direction);

	for (Cullable* culled : culledObjects){
		Object* object = (Object*)culled;

		if (!(pickType & object->GetRenderTypes()))
		{
			continue;
		}
		if (!dynamicObjects && object->isDynamic())
		{
			continue;
		}

		// layer support
		if (checkLayers || dontcheckLayers)
		{
			string id = object->GetID();

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
		XMMATRIX& objectMat = XMLoadFloat4x4(&object->world);

		for (unsigned int i = 0; i<mesh->indices.size(); i += 3){
			int i0 = mesh->indices[i], i1 = mesh->indices[i + 1], i2 = mesh->indices[i + 2];
			Vertex& v0 = mesh->skinnedVertices[i0], v1 = mesh->skinnedVertices[i1], v2 = mesh->skinnedVertices[i2];
			XMVECTOR& V0 =
				XMVector4Transform(XMLoadFloat4(&v0.pos), objectMat)
				, V1 = XMVector4Transform(XMLoadFloat4(&v1.pos), objectMat)
				, V2 = XMVector4Transform(XMLoadFloat4(&v2.pos), objectMat);
			float distance = 0;
			if (TriangleTests::Intersects(rayOrigin, rayDirection, V0, V1, V2, distance)){
				XMVECTOR& pos = XMVectorAdd(rayOrigin, rayDirection*distance);
				XMVECTOR& nor = XMVector3Normalize(XMVector3Cross(XMVectorSubtract(V2, V1), XMVectorSubtract(V1, V0)));
				Picked picked = Picked();
				picked.object = object;
				XMStoreFloat3(&picked.position, pos);
				XMStoreFloat3(&picked.normal, nor);
				picked.distance = distance;
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

			wiSPTree::getVisible(searchTree->root, ray, culledObjects);

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
				mesh->skinnedVertices[ind].nor.w = ambientLightIntensity;
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

	GetScene().AddModel(model);

	for (Object* o : model->objects)
	{
		if (physicsEngine != nullptr) {
			physicsEngine->registerObject(o);
		}
	}

	wiRenderer::graphicsDevice->LOCK();

	Update();

	if(spTree){
		spTree->AddObjects(spTree->root,vector<Cullable*>(model->objects.begin(), model->objects.end()));
	}
	else
	{
		GenerateSPTree(spTree, vector<Cullable*>(model->objects.begin(), model->objects.end()), SPTREE_GENERATE_OCTREE);
	}
	if(spTree_lights){
		spTree_lights->AddObjects(spTree_lights->root,vector<Cullable*>(model->lights.begin(),model->lights.end()));
	}
	else
	{
		GenerateSPTree(spTree_lights, vector<Cullable*>(model->lights.begin(), model->lights.end()), SPTREE_GENERATE_OCTREE);
	}

	unique_identifier++;



	//UpdateRenderData(nullptr);

	SetUpCubes();
	SetUpBoneLines();

	wiRenderer::graphicsDevice->UNLOCK();


	LoadWorldInfo(dir, name);


	return model;
}
void wiRenderer::LoadWorldInfo(const string& dir, const string& name)
{
	LoadWiWorldInfo(dir, name+".wiw", GetScene().worldInfo, GetScene().wind);

	wiRenderer::graphicsDevice->LOCK();
	UpdateWorldCB(GRAPHICSTHREAD_IMMEDIATE);
	wiRenderer::graphicsDevice->UNLOCK();
}
Scene& wiRenderer::GetScene()
{
	if (scene == nullptr)
	{
		scene = new Scene;
	}
	return *scene;
}

void wiRenderer::SychronizeWithPhysicsEngine()
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
							Vertex tvert = m->skinnedVertices[vi];
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
					if (object->kinematic) {
						physicsEngine->transformBody(object->rotation, object->translation, pI);
					}
				}
			}
		}

		physicsEngine->Update();

		for (Model* model : GetScene().models)
		{
			for (Object* object : model->objects) {
				int pI = object->physicsObjectI;
				if (pI >= 0 && !object->kinematic) {
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
	probe->cubeMap.InitializeCube(resolution, 1, true, DXGI_FORMAT_R16G16B16A16_FLOAT, 0);

	wiRenderer::graphicsDevice->LOCK();

	GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE;

	probe->cubeMap.Activate(threadID, 0, 0, 0, 1);

	wiRenderer::graphicsDevice->BindVertexLayout(vertexLayouts[VLTYPE_EFFECT], threadID);
	wiRenderer::graphicsDevice->BindPrimitiveTopology(TRIANGLELIST, threadID);
	wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);

	wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_ENVMAP], threadID);
	wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_ENVMAP], threadID);
	wiRenderer::graphicsDevice->BindGS(geometryShaders[GSTYPE_ENVMAP], threadID);

	wiRenderer::graphicsDevice->BindConstantBufferGS(constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubeMapRenderCB), threadID);


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


	static thread_local CubeMapRenderCB* cb = new CubeMapRenderCB;
	for (unsigned int i = 0; i < cameras.size(); ++i)
	{
		cameras[i].Update(XMLoadFloat3(&position));
		(*cb).mViewProjection[i] = cameras[i].getVP();
	}

	wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_CUBEMAPRENDER], cb, threadID);


	CulledList culledObjects;
	CulledCollection culledRenderer;

	SPHERE culler = SPHERE(position, getCamera()->zFarP);
	if (spTree)
		wiSPTree::getVisible(spTree->root, culler, culledObjects);

	for (Cullable* object : culledObjects)
		culledRenderer[((Object*)object)->mesh].insert((Object*)object);

	for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) 
	{
		Mesh* mesh = iter->first;
		CulledObjectList& visibleInstances = iter->second;

		if (!mesh->isBillboarded && !visibleInstances.empty()) {

			if (!mesh->doubleSided)
				wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_FRONT], threadID);
			else
				wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_DOUBLESIDED], threadID);



			int k = 0;
			for (CulledObjectList::iterator viter = visibleInstances.begin(); viter != visibleInstances.end(); ++viter) {
				if ((*viter)->emitterType != Object::EmitterType::EMITTER_INVISIBLE) {
					if (mesh->softBody || (*viter)->isArmatureDeformed())
						Mesh::AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k);
					else
						Mesh::AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k);
					++k;
				}
			}
			if (k < 1)
				continue;

			Mesh::UpdateRenderableInstances(visibleInstances.size(), threadID);


			wiRenderer::graphicsDevice->BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), threadID);
			wiRenderer::graphicsDevice->BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), threadID);
			wiRenderer::graphicsDevice->BindIndexBuffer(mesh->meshIndexBuff, threadID);


			int matsiz = mesh->materialIndices.size();
			int m = 0;
			for (Material* iMat : mesh->materials) {
				if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {

					if (iMat->shadeless)
						wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_SHADELESS, threadID);
					if (iMat->subsurface_scattering)
						wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_SKIN, threadID);
					else
						wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], mesh->stencilRef, threadID);

					wiRenderer::graphicsDevice->BindTexturePS(iMat->texture, TEXSLOT_ONDEMAND0, threadID);

					static thread_local MaterialCB* mcb = new MaterialCB;
					(*mcb).Create(*iMat, m);

					wiRenderer::graphicsDevice->UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, threadID);

					wiRenderer::graphicsDevice->DrawIndexedInstanced(mesh->indices.size(), visibleInstances.size(), threadID);
				}
				m++;
			}
		}
		visibleInstances.clear();
	}



	// sky
	{
		wiRenderer::graphicsDevice->BindPrimitiveTopology(TRIANGLELIST, threadID);
		wiRenderer::graphicsDevice->BindRasterizerState(rasterizers[RSTYPE_BACK], threadID);
		wiRenderer::graphicsDevice->BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_SKY, threadID);
		wiRenderer::graphicsDevice->BindBlendState(blendStates[BSTYPE_OPAQUE], threadID);

		wiRenderer::graphicsDevice->BindVS(vertexShaders[VSTYPE_ENVMAP_SKY], threadID);
		wiRenderer::graphicsDevice->BindPS(pixelShaders[PSTYPE_ENVMAP_SKY], threadID);
		wiRenderer::graphicsDevice->BindGS(geometryShaders[GSTYPE_ENVMAP_SKY], threadID);

		wiRenderer::graphicsDevice->BindTexturePS(enviroMap, TEXSLOT_ENV_GLOBAL, threadID);

		wiRenderer::graphicsDevice->BindVertexBuffer(nullptr, 0, 0, threadID);
		wiRenderer::graphicsDevice->BindVertexLayout(nullptr, threadID);
		wiRenderer::graphicsDevice->Draw(240, threadID);
	}


	wiRenderer::graphicsDevice->BindGS(nullptr, threadID);


	probe->cubeMap.Deactivate(threadID);

	wiRenderer::graphicsDevice->GenerateMips(probe->cubeMap.GetTexture(), threadID);

	enviroMap = probe->cubeMap.GetTexture();

	scene->environmentProbes.push_back(probe);

	wiRenderer::graphicsDevice->UNLOCK();
}

void wiRenderer::MaterialCB::Create(const Material& mat,UINT materialIndex){
	difColor = XMFLOAT4(mat.diffuseColor.x, mat.diffuseColor.y, mat.diffuseColor.z, mat.alpha);
	hasRef = mat.refMap != nullptr;
	hasNor = mat.normalMap != nullptr;
	hasTex = mat.texture != nullptr;
	hasSpe = mat.specularMap != nullptr;
	specular = mat.specular;
	refractionIndex = mat.refraction_index;
	texMulAdd = mat.texMulAdd;
	metallic = mat.enviroReflection;
	shadeless = mat.shadeless;
	specular_power = mat.specular_power;
	toon = mat.toonshading;
	matIndex = materialIndex;
	emissive = mat.emissive;
	roughness = mat.roughness;
}