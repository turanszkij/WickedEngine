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

#pragma region STATICS
D3D_DRIVER_TYPE						wiRenderer::driverType;
D3D_FEATURE_LEVEL					wiRenderer::featureLevel;
SwapChain					wiRenderer::swapChain;
RenderTargetView			wiRenderer::renderTargetView;
ViewPort					wiRenderer::viewPort;
GraphicsDevice			wiRenderer::graphicsDevice;
DeviceContext				wiRenderer::immediateContext;
bool wiRenderer::DX11 = false,wiRenderer::VSYNC=true,wiRenderer::DEFERREDCONTEXT_SUPPORT=false;
DeviceContext				wiRenderer::deferredContexts[];
CommandList				wiRenderer::commandLists[];
mutex								wiRenderer::graphicsMutex;
Sampler wiRenderer::samplers[SSLOT_COUNT];

map<DeviceContext,long> wiRenderer::drawCalls;
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

TextureView wiRenderer::enviroMap,wiRenderer::colorGrading;
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
    HRESULT hr = S_OK;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
		D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        //D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );
	
#ifndef WINSTORE_SUPPORT
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 2;
	sd.BufferDesc.Width = SCREENWIDTH = screenW;
	sd.BufferDesc.Height = SCREENHEIGHT = screenH;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = window;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = windowed;
#endif
	
	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        driverType = driverTypes[driverTypeIndex];
#ifndef WINSTORE_SUPPORT
        hr = D3D11CreateDeviceAndSwapChain( NULL, driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
											D3D11_SDK_VERSION, &sd, &swapChain, &graphicsDevice, &featureLevel, &immediateContext );
#else
		hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &graphicsDevice
			, &featureLevel, &immediateContext);
#endif

		if( SUCCEEDED( hr ) )
            break;
    }
	if(FAILED(hr)){
        wiHelper::messageBox("SwapChain Creation Failed!","Error!",nullptr);
#ifdef BACKLOG
			wiBackLog::post("SwapChain Creation Failed!");
#endif
		exit(1);
	}
	DX11 = ( ( wiRenderer::graphicsDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ) ? true:false );
	drawCalls.insert(pair<DeviceContext,long>(immediateContext,0));


#ifdef WINSTORE_SUPPORT
	DXGI_SWAP_CHAIN_DESC1 sd = { 0 };
	sd.Width = SCREENWIDTH = (int)window->Bounds.Width;
	sd.Height = SCREENHEIGHT = (int)window->Bounds.Height;
	sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
	sd.Stereo = false;
	sd.SampleDesc.Count = 1; // Don't use multi-sampling.
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2; // Use double-buffering to minimize latency.
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
	sd.Flags = 0;
	sd.Scaling = DXGI_SCALING_NONE;
	sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

	IDXGIDevice2 * pDXGIDevice;
	hr = graphicsDevice->QueryInterface(__uuidof(IDXGIDevice2), (void **)&pDXGIDevice);

	IDXGIAdapter * pDXGIAdapter;
	hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter);

	IDXGIFactory2 * pIDXGIFactory;
	pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), (void **)&pIDXGIFactory);


	hr = pIDXGIFactory->CreateSwapChainForCoreWindow(graphicsDevice, reinterpret_cast<APIInterface>(window), &sd
		, nullptr, &swapChain);

	if (FAILED(hr)){
		wiHelper::messageBox("Swap chain creation failed!", "Error!");
		exit(1);
	}
#endif

	DEFERREDCONTEXT_SUPPORT = false;
	D3D11_FEATURE_DATA_THREADING threadingFeature;
	wiRenderer::graphicsDevice->CheckFeatureSupport( D3D11_FEATURE_THREADING,&threadingFeature,sizeof(threadingFeature) );
	if (threadingFeature.DriverConcurrentCreates && threadingFeature.DriverCommandLists){
		DEFERREDCONTEXT_SUPPORT = true;
		for (int i = 0; i<GRAPHICSTHREAD_COUNT; i++){
			wiRenderer::graphicsDevice->CreateDeferredContext( 0,&deferredContexts[i] );
			drawCalls.insert(pair<DeviceContext,long>(deferredContexts[i],0));
		}
#ifdef BACKLOG
		stringstream ss("");
		ss<<NUM_DCONTEXT<<" defferred contexts created!";
		wiBackLog::post(ss.str().c_str());
#endif
	}
	else {
		//MessageBox(window,L"Deferred Context not supported!",L"Error!",0);
#ifdef BACKLOG
		wiBackLog::post("Deferred context not supported!");
#endif
		DEFERREDCONTEXT_SUPPORT=false;
		//exit(0);
	}
	

    // Create a render target view
    Texture2D pBackBuffer = NULL;
    hr = swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) ){
		wiHelper::messageBox("BackBuffer creation Failed!", "Error!", nullptr);
		exit(0);
	}

    hr = wiRenderer::graphicsDevice->CreateRenderTargetView( pBackBuffer, NULL, &renderTargetView );
    //pBackBuffer->Release();
    if( FAILED( hr ) ){
		wiHelper::messageBox("Main Rendertarget creation Failed!", "Error!", nullptr);
		exit(0);
	}


    // Setup the main viewport
	viewPort.Width = (FLOAT)SCREENWIDTH;
	viewPort.Height = (FLOAT)SCREENHEIGHT;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;

	
    return S_OK;
}
void wiRenderer::DestroyDevice()
{
	
	if( renderTargetView ) 
		renderTargetView->Release();
    if( swapChain ) 
		swapChain->Release();
	if( wiRenderer::getImmediateContext() ) 
		wiRenderer::getImmediateContext()->ClearState();
    if( wiRenderer::getImmediateContext() ) 
		wiRenderer::getImmediateContext()->Release();
    if( graphicsDevice ) 
		graphicsDevice->Release();

	for(int i=0;i<GRAPHICSTHREAD_COUNT;i++){
		if(commandLists[i]) {commandLists[i]->Release(); commandLists[i]=0;}
		if(deferredContexts[i]) {deferredContexts[i]->Release(); deferredContexts[i]=0;}
	}
}
void wiRenderer::Present(function<void()> drawToScreen1,function<void()> drawToScreen2,function<void()> drawToScreen3)
{
	Lock();

	immediateContext->RSSetViewports( 1, &viewPort );
	immediateContext->OMSetRenderTargets( 1, &renderTargetView, 0 );
	float ClearColor[4] = { 0, 0, 0, 1.0f }; // red,green,blue,alpha
	immediateContext->ClearRenderTargetView( renderTargetView, ClearColor );
	//wiRenderer::getImmediateContext()->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	
	if(drawToScreen1!=nullptr)
		drawToScreen1();
	if(drawToScreen2!=nullptr)
		drawToScreen2();
	if(drawToScreen3!=nullptr)
		drawToScreen3();
	


	wiFrameRate::Frame();

	swapChain->Present( VSYNC, 0 );

	for(auto& d : drawCalls){
		d.second=0;
	}


	immediateContext->OMSetRenderTargets(0,nullptr,nullptr);

	UnbindTextures(0, 16, immediateContext);

	Unlock();

	*prevFrameCam = *cam;
}
void wiRenderer::ExecuteDeferredContexts()
{
	for (int i = 0; i < GRAPHICSTHREAD_COUNT; i++)
	{
		if (commandLists[i])
		{
			immediateContext->ExecuteCommandList(commandLists[i], true);
			commandLists[i]->Release();
			commandLists[i] = nullptr;

			UnbindTextures(0, 16, deferredContexts[i]);
		}
	}
}
void wiRenderer::FinishCommandList(GRAPHICSTHREAD thread)
{
	deferredContexts[thread]->FinishCommandList(true, &commandLists[thread]);
}

long wiRenderer::getDrawCallCount(){
	long retVal=0;
	for(auto d:drawCalls){
		retVal+=d.second;
	}
	return retVal;
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
		SafeInit(constantBuffers[i]);
	}
	for (int i = 0; i < VSTYPE_LAST; ++i)
	{
		SafeInit(vertexShaders[i]);
	}
	for (int i = 0; i < PSTYPE_LAST; ++i)
	{
		SafeInit(pixelShaders[i]);
	}
	for (int i = 0; i < GSTYPE_LAST; ++i)
	{
		SafeInit(geometryShaders[i]);
	}
	for (int i = 0; i < HSTYPE_LAST; ++i)
	{
		SafeInit(hullShaders[i]);
	}
	for (int i = 0; i < DSTYPE_LAST; ++i)
	{
		SafeInit(domainShaders[i]);
	}
	for (int i = 0; i < RSTYPE_LAST; ++i)
	{
		SafeInit(rasterizers[i]);
	}
	for (int i = 0; i < DSSTYPE_LAST; ++i)
	{
		SafeInit(depthStencils[i]);
	}
	for (int i = 0; i < VLTYPE_LAST; ++i)
	{
		SafeInit(vertexLayouts[i]);
	}
	for (int i = 0; i < SSLOT_COUNT_PERSISTENT; ++i)
	{
		SafeInit(samplers[i]);
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

	BindPersistentState(getImmediateContext());

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
		SafeRelease(constantBuffers[i]);
	}
	for (int i = 0; i < VSTYPE_LAST; ++i)
	{
		SafeRelease(vertexShaders[i]);
	}
	for (int i = 0; i < PSTYPE_LAST; ++i)
	{
		SafeRelease(pixelShaders[i]);
	}
	for (int i = 0; i < GSTYPE_LAST; ++i)
	{
		SafeRelease(geometryShaders[i]);
	}
	for (int i = 0; i < HSTYPE_LAST; ++i)
	{
		SafeRelease(hullShaders[i]);
	}
	for (int i = 0; i < DSTYPE_LAST; ++i)
	{
		SafeRelease(domainShaders[i]);
	}
	for (int i = 0; i < RSTYPE_LAST; ++i)
	{
		SafeRelease(rasterizers[i]);
	}
	for (int i = 0; i < DSSTYPE_LAST; ++i)
	{
		SafeRelease(depthStencils[i]);
	}
	for (int i = 0; i < VLTYPE_LAST; ++i)
	{
		SafeRelease(vertexLayouts[i]);
	}
	for (int i = 0; i < BSTYPE_LAST; ++i)
	{
		SafeRelease(blendStates[i]);
	}
	for (int i = 0; i < SSLOT_COUNT_PERSISTENT; ++i)
	{
		SafeRelease(samplers[i]);
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

    D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

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
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

			{ "MATI", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR_DITHER", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectVS10.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr){
			vertexShaders[VSTYPE_EFFECT10] = vsinfo->vertexShader;
			vertexLayouts[VLTYPE_EFFECT] = vsinfo->vertexLayout;
		}
	}

	{

		VertexLayoutDesc oslayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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


	vertexShaders[VSTYPE_EFFECT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_EFFECT_REFLECTION] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectVS_reflection.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_DIRLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_POINTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "pointLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_SPOTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "spotLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOLUMESPOTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vSpotLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_VOLUMEPOINTLIGHT] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vPointLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_DECAL] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "decalVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vertexShaders[VSTYPE_ENVMAP] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;

	pixelShaders[PSTYPE_EFFECT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_EFFECT_TRANSPARENT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS_transparent.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SIMPLEST] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS_simplest.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_EFFECT_FORWARDSIMPLE] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS_forwardSimple.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_BLACKOUT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS_blackout.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_TEXTUREONLY] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS_textureonly.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DIRLIGHT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DIRLIGHT_SOFT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightSoftPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_POINTLIGHT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "pointLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_SPOTLIGHT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "spotLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_VOLUMELIGHT] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "volumeLightPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_DECAL] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "decalPS.cso", wiResourceManager::PIXELSHADER));
	pixelShaders[PSTYPE_ENVMAP] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapPS.cso", wiResourceManager::PIXELSHADER));
	
	geometryShaders[GSTYPE_ENVMAP] = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "envMapGS.cso", wiResourceManager::GEOMETRYSHADER));

}
void wiRenderer::LoadLineShaders()
{
	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	hullShaders[HSTYPE_EFFECT] = static_cast<HullShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectHS.cso", wiResourceManager::HULLSHADER));
	domainShaders[DSTYPE_EFFECT] = static_cast<DomainShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectDS.cso", wiResourceManager::DOMAINSHADER));

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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

	Lock();
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
	Unlock();
}


void wiRenderer::SetUpStates()
{
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_LINEAR_MIRROR]);

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_LINEAR_CLAMP]);

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_LINEAR_WRAP]);
	
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_POINT_MIRROR]);
	
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_POINT_WRAP]);
	
	
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_POINT_CLAMP]);
	
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MaxAnisotropy = 16;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_ANISO_CLAMP]);
	
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_ANISO_WRAP]);
	
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_ANISO_MIRROR]);

	ZeroMemory( &samplerDesc, sizeof(D3D11_SAMPLER_DESC) );
	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	graphicsDevice->CreateSamplerState(&samplerDesc, &samplers[SSLOT_CMP_DEPTH]);
	
	D3D11_RASTERIZER_DESC rs;
	rs.FillMode=D3D11_FILL_SOLID;
	rs.CullMode=D3D11_CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=true;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_FRONT]);

	
	rs.FillMode=D3D11_FILL_SOLID;
	rs.CullMode=D3D11_CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=4;
	rs.DepthClipEnable=true;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_SHADOW]);

	rs.FillMode=D3D11_FILL_SOLID;
	rs.CullMode=D3D11_CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=5.0f;
	rs.DepthClipEnable=true;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_SHADOW_DOUBLESIDED]);

	rs.FillMode=D3D11_FILL_WIREFRAME;
	rs.CullMode=D3D11_CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=true;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_WIRE]);
	
	rs.FillMode=D3D11_FILL_SOLID;
	rs.CullMode=D3D11_CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_DOUBLESIDED]);
	
	rs.FillMode=D3D11_FILL_WIREFRAME;
	rs.CullMode=D3D11_CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_WIRE_DOUBLESIDED]);
	
	rs.FillMode=D3D11_FILL_SOLID;
	rs.CullMode=D3D11_CULL_FRONT;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	graphicsDevice->CreateRasterizerState(&rs,&rasterizers[RSTYPE_BACK]);

	D3D11_DEPTH_STENCIL_DESC dsd;
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	// Create the depth stencil state.
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DEFAULT]);

	
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthEnable = false;
	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	// Create the depth stencil state.
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_STENCILREAD]);

	
	dsd.DepthEnable = false;
	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_STENCILREAD_MATCH]);


	dsd.DepthEnable = true;
	dsd.StencilEnable = false;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_DEPTHREAD]);

	dsd.DepthEnable = false;
	dsd.StencilEnable=false;
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencils[DSSTYPE_XRAY]);

	
	D3D11_BLEND_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=false;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MAX;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bd.AlphaToCoverageEnable=false;
	graphicsDevice->CreateBlendState(&bd,&blendStates[BSTYPE_OPAQUE]);

	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bd.AlphaToCoverageEnable=false;
	graphicsDevice->CreateBlendState(&bd,&blendStates[BSTYPE_TRANSPARENT]);


	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	bd.IndependentBlendEnable=true,
	bd.AlphaToCoverageEnable=false;
	graphicsDevice->CreateBlendState(&bd,&blendStates[BSTYPE_ADDITIVE]);
}

void wiRenderer::BindPersistentState(DeviceContext context)
{
	Lock();

	for (int i = 0; i < SSLOT_COUNT; ++i)
	{
		if (samplers[i] == nullptr)
			continue;
		BindSamplerPS(samplers[i], i, context);
		BindSamplerVS(samplers[i], i, context);
		BindSamplerGS(samplers[i], i, context);
		BindSamplerDS(samplers[i], i, context);
		BindSamplerHS(samplers[i], i, context);
	}


	BindConstantBufferPS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), context);
	BindConstantBufferVS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), context);
	BindConstantBufferGS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), context);
	BindConstantBufferHS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), context);
	BindConstantBufferDS(constantBuffers[CBTYPE_WORLD], CB_GETBINDSLOT(WorldCB), context);

	BindConstantBufferPS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), context);
	BindConstantBufferVS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), context);
	BindConstantBufferGS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), context);
	BindConstantBufferHS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), context);
	BindConstantBufferDS(constantBuffers[CBTYPE_FRAME], CB_GETBINDSLOT(FrameCB), context);

	BindConstantBufferPS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), context);
	BindConstantBufferVS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), context);
	BindConstantBufferGS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), context);
	BindConstantBufferHS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), context);
	BindConstantBufferDS(constantBuffers[CBTYPE_CAMERA], CB_GETBINDSLOT(CameraCB), context);

	BindConstantBufferPS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), context);
	BindConstantBufferVS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), context);
	BindConstantBufferGS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), context);
	BindConstantBufferHS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), context);
	BindConstantBufferDS(constantBuffers[CBTYPE_MATERIAL], CB_GETBINDSLOT(MaterialCB), context);

	BindConstantBufferPS(constantBuffers[CBTYPE_DIRLIGHT], CB_GETBINDSLOT(DirectionalLightCB), context);
	BindConstantBufferVS(constantBuffers[CBTYPE_DIRLIGHT], CB_GETBINDSLOT(DirectionalLightCB), context);

	BindConstantBufferVS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), context);
	BindConstantBufferPS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), context);
	BindConstantBufferGS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), context);
	BindConstantBufferDS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), context);
	BindConstantBufferHS(constantBuffers[CBTYPE_MISC], CB_GETBINDSLOT(MiscCB), context);

	BindConstantBufferVS(constantBuffers[CBTYPE_SHADOW], CB_GETBINDSLOT(ShadowCB), context);

	BindConstantBufferVS(constantBuffers[CBTYPE_CLIPPLANE], CB_GETBINDSLOT(APICB), context);

	Unlock();
}
void wiRenderer::RebindPersistentState(DeviceContext context)
{
	BindPersistentState(context);

	wiImage::BindPersistentState(context);
	wiFont::BindPersistentState(context);
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
void wiRenderer::UpdateRenderData(DeviceContext context)
{
	if (context == nullptr)
		return;

	//UpdateWorldCB(context);
	UpdateFrameCB(context);
	UpdateCameraCB(context);

	
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
						BindPrimitiveTopology(POINTLIST, context);
						BindVertexLayout(vertexLayouts[VLTYPE_STREAMOUT], context);
						BindVS(vertexShaders[VSTYPE_STREAMOUT], context);
						BindGS(geometryShaders[GSTYPE_STREAMOUT], context);
						BindPS(nullptr, context);
						BindConstantBufferVS(constantBuffers[CBTYPE_BONEBUFFER], CB_GETBINDSLOT(BoneCB), context);
						BindConstantBufferGS(constantBuffers[CBTYPE_BONEBUFFER], CB_GETBINDSLOT(BoneCB), context);
					}

					// Upload bones for skinning to shader
					static thread_local BoneCB* bonebuf = new BoneCB;
					for (unsigned int k = 0; k < mesh->armature->boneCollection.size(); k++) {
						bonebuf->pose[k] = XMMatrixTranspose(XMLoadFloat4x4(&mesh->armature->boneCollection[k]->boneRelativity));
						bonebuf->prev[k] = XMMatrixTranspose(XMLoadFloat4x4(&mesh->armature->boneCollection[k]->boneRelativityPrev));
					}
					UpdateBuffer(constantBuffers[CBTYPE_BONEBUFFER], bonebuf, context);


					// Do the skinning
					BindVertexBuffer(mesh->meshVertBuff, 0, sizeof(SkinnedVertex), context);
					BindStreamOutTarget(mesh->sOutBuffer, context);
					Draw(mesh->vertices.size(), context);
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
					UpdateBuffer(mesh->meshVertBuff, mesh->skinnedVertices.data(), context, sizeof(Vertex)*mesh->skinnedVertices.size());
				}
			}
		}

#ifdef USE_GPU_SKINNING
		if (streamOutSetUp)
		{
			// Unload skinning shader
			BindGS(nullptr, context);
			BindVS(nullptr, context);
			BindStreamOutTarget(nullptr, context);
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
void wiRenderer::DrawWaterRipples(DeviceContext context){
	//wiImage::BatchBegin(context);
	for(wiSprite* i:waterRipples){
		i->DrawNormal(context);
	}
}

void wiRenderer::DrawDebugSpheres(Camera* camera, DeviceContext context)
{
	//if(debugSpheres){
	//	BindPrimitiveTopology(TRIANGLESTRIP,context);
	//	BindVertexLayout(vertexLayouts[VLTYPE_EFFECT] : vertexLayouts[VLTYPE_LINE],context);
	//
	//	BindRasterizerState(rasterizers[RSTYPE_FRONT],context);
	//	BindDepthStencilState(depthStencils[DSSTYPE_XRAY],STENCILREF_EMPTY,context);
	//	BindBlendState(blendStates[BSTYPE_TRANSPARENT],context);


	//	BindPS(linePS,context);
	//	BindVS(lineVS,context);

	//	BindVertexBuffer(HitSphere::vertexBuffer,0,sizeof(XMFLOAT3A),context);

	//	for (unsigned int i = 0; i<spheres.size(); i++){
	//		//D3D11_MAPPED_SUBRESOURCE mappedResource;
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

	//		UpdateBuffer(lineBuffer,&sb,context);


	//		//context->Draw((HitSphere::RESOLUTION+1)*2,0);
	//		Draw((HitSphere::RESOLUTION+1)*2,context);

	//	}
	//}
	
}
void wiRenderer::DrawDebugBoneLines(Camera* camera, DeviceContext context)
{
	if(debugBoneLines){
		BindPrimitiveTopology(LINELIST,context);
		BindVertexLayout(vertexLayouts[VLTYPE_LINE],context);
	
		BindRasterizerState(rasterizers[RSTYPE_SHADOW],context);
		BindDepthStencilState(depthStencils[DSSTYPE_XRAY],STENCILREF_EMPTY,context);
		BindBlendState(blendStates[BSTYPE_OPAQUE],context);


		BindPS(pixelShaders[PSTYPE_LINE],context);
		BindVS(vertexShaders[VSTYPE_LINE],context);

		static thread_local MiscCB* sb = new MiscCB;
		for (unsigned int i = 0; i<boneLines.size(); i++){
			(*sb).mTransform = XMMatrixTranspose(XMLoadFloat4x4(&boneLines[i]->desc.transform)*camera->GetViewProjection());
			(*sb).mColor = boneLines[i]->desc.color;

			UpdateBuffer(constantBuffers[CBTYPE_MISC], sb, context);

			BindVertexBuffer(boneLines[i]->vertexBuffer, 0, sizeof(XMFLOAT3A), context);
			Draw(2, context);
		}
	}
}
void wiRenderer::DrawDebugLines(Camera* camera, DeviceContext context)
{
	if (linesTemp.empty())
		return;

	BindPrimitiveTopology(LINELIST, context);
	BindVertexLayout(vertexLayouts[VLTYPE_LINE], context);

	BindRasterizerState(rasterizers[RSTYPE_SHADOW], context);
	BindDepthStencilState(depthStencils[DSSTYPE_XRAY], STENCILREF_EMPTY, context);
	BindBlendState(blendStates[BSTYPE_OPAQUE], context);


	BindPS(pixelShaders[PSTYPE_LINE], context);
	BindVS(vertexShaders[VSTYPE_LINE], context);

	static thread_local MiscCB* sb = new MiscCB;
	for (unsigned int i = 0; i<linesTemp.size(); i++){
		(*sb).mTransform = XMMatrixTranspose(XMLoadFloat4x4(&linesTemp[i]->desc.transform)*camera->GetViewProjection());
		(*sb).mColor = linesTemp[i]->desc.color;

		UpdateBuffer(constantBuffers[CBTYPE_MISC], sb, context);

		BindVertexBuffer(linesTemp[i]->vertexBuffer, 0, sizeof(XMFLOAT3A), context);
		Draw(2, context);
	}

	for (Lines* x : linesTemp)
		delete x;
	linesTemp.clear();
}
void wiRenderer::DrawDebugBoxes(Camera* camera, DeviceContext context)
{
	if(debugBoxes){
		BindPrimitiveTopology(LINELIST,context);
		BindVertexLayout(vertexLayouts[VLTYPE_LINE],context);

		BindRasterizerState(rasterizers[RSTYPE_WIRE_DOUBLESIDED],context);
		BindDepthStencilState(depthStencils[DSSTYPE_XRAY],STENCILREF_EMPTY,context);
		BindBlendState(blendStates[BSTYPE_OPAQUE],context);


		BindPS(pixelShaders[PSTYPE_LINE],context);
		BindVS(vertexShaders[VSTYPE_LINE],context);
		
		BindVertexBuffer(Cube::vertexBuffer,0,sizeof(XMFLOAT3A),context);
		BindIndexBuffer(Cube::indexBuffer,context);

		static thread_local MiscCB* sb = new MiscCB;
		for (unsigned int i = 0; i<cubes.size(); i++){
			(*sb).mTransform =XMMatrixTranspose(XMLoadFloat4x4(&cubes[i].desc.transform)*camera->GetViewProjection());
			(*sb).mColor=cubes[i].desc.color;

			UpdateBuffer(constantBuffers[CBTYPE_MISC],sb,context);

			DrawIndexed(24,context);
		}

	}
}

void wiRenderer::DrawSoftParticles(Camera* camera, ID3D11DeviceContext *context, TextureView depth, bool dark)
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
		e->DrawNonPremul(camera,context,depth,dark);
	}
}
void wiRenderer::DrawSoftPremulParticles(Camera* camera, ID3D11DeviceContext *context, TextureView depth, bool dark)
{
	for (wiEmittedParticle* e : emitterSystems)
	{
		e->DrawPremul(camera, context, depth, dark);
	}
}
void wiRenderer::DrawTrails(DeviceContext context, TextureView refracRes){
	BindPrimitiveTopology(TRIANGLESTRIP,context);
	BindVertexLayout(vertexLayouts[VLTYPE_TRAIL],context);

	BindRasterizerState(wireRender?rasterizers[RSTYPE_WIRE_DOUBLESIDED]:rasterizers[RSTYPE_DOUBLESIDED],context);
	BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT],STENCILREF_EMPTY,context);
	BindBlendState(blendStates[BSTYPE_OPAQUE],context);

	BindPS(pixelShaders[PSTYPE_TRAIL],context);
	BindVS(vertexShaders[VSTYPE_TRAIL],context);
	
	BindTexturePS(refracRes,0,context);

	for (Object* o : objectsWithTrails)
	{
		if(o->trailBuff && o->trail.size()>=4){

			BindTexturePS(o->trailDistortTex, 1, context);
			BindTexturePS(o->trailTex, 2, context);

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
				UpdateBuffer(o->trailBuff,trails.data(),context,sizeof(RibbonVertex)*trails.size());

				BindVertexBuffer(o->trailBuff,0,sizeof(RibbonVertex),context);
				Draw(trails.size(),context);

				trails.clear();
			}
		}
	}
}
void wiRenderer::DrawImagesAdd(DeviceContext context, TextureView refracRes){
	imagesRTAdd.Activate(context,0,0,0,1);
	//wiImage::BatchBegin(context);
	for(wiSprite* x : images){
		if(x->effects.blendFlag==BLENDMODE_ADDITIVE){
			/*TextureView nor = x->effects.normalMap;
			x->effects.setNormalMap(nullptr);
			bool changedBlend=false;
			if(x->effects.blendFlag==BLENDMODE_OPAQUE && nor){
				x->effects.blendFlag=BLENDMODE_ADDITIVE;
				changedBlend=true;
			}*/
			x->Draw(refracRes,  context);
			/*if(changedBlend)
				x->effects.blendFlag=BLENDMODE_OPAQUE;
			x->effects.setNormalMap(nor);*/
		}
	}
}
void wiRenderer::DrawImages(DeviceContext context, TextureView refracRes){
	imagesRT.Activate(context,0,0,0,0);
	//wiImage::BatchBegin(context);
	for(wiSprite* x : images){
		if(x->effects.blendFlag==BLENDMODE_ALPHA || x->effects.blendFlag==BLENDMODE_OPAQUE){
			/*TextureView nor = x->effects.normalMap;
			x->effects.setNormalMap(nullptr);
			bool changedBlend=false;
			if(x->effects.blendFlag==BLENDMODE_OPAQUE && nor){
				x->effects.blendFlag=BLENDMODE_ADDITIVE;
				changedBlend=true;
			}*/
			x->Draw(refracRes,  context);
			/*if(changedBlend)
				x->effects.blendFlag=BLENDMODE_OPAQUE;
			x->effects.setNormalMap(nor);*/
		}
	}
}
void wiRenderer::DrawImagesNormals(DeviceContext context, TextureView refracRes){
	normalMapRT.Activate(context,0,0,0,0);
	//wiImage::BatchBegin(context);
	for(wiSprite* x : images){
		x->DrawNormal(context);
	}
}
void wiRenderer::DrawLights(Camera* camera, DeviceContext context
				, TextureView depth, TextureView normal, TextureView material
				, unsigned int stencilRef){

	
	static thread_local Frustum frustum = Frustum();
	frustum.ConstructFrustum(min(camera->zFarP, GetScene().worldInfo.fogSEH.y),camera->Projection,camera->View);
	
	CulledList culledObjects;
	if(spTree_lights)
		wiSPTree::getVisible(spTree_lights->root,frustum,culledObjects);

	if(!culledObjects.empty())
	{


	BindPrimitiveTopology(TRIANGLELIST,context);
	
	BindTexturePS(depth,0,context);
	BindTexturePS(normal,1,context);
	BindTexturePS(material,2,context);

	
	BindBlendState(blendStates[BSTYPE_ADDITIVE],context);
	BindDepthStencilState(depthStencils[DSSTYPE_STENCILREAD],stencilRef,context);
	BindRasterizerState(rasterizers[RSTYPE_BACK],context);

	BindVertexLayout(nullptr, context);
	BindVertexBuffer(nullptr, 0, 0, context);
	BindIndexBuffer(nullptr, context);

	BindConstantBufferPS(constantBuffers[CBTYPE_POINTLIGHT], CB_GETBINDSLOT(PointLightCB), context);
	BindConstantBufferVS(constantBuffers[CBTYPE_POINTLIGHT], CB_GETBINDSLOT(PointLightCB), context);

	BindConstantBufferPS(constantBuffers[CBTYPE_SPOTLIGHT], CB_GETBINDSLOT(SpotLightCB), context);
	BindConstantBufferVS(constantBuffers[CBTYPE_SPOTLIGHT], CB_GETBINDSLOT(SpotLightCB), context);

	for(int type=0;type<3;++type){

			
		BindVS(vertexShaders[VSTYPE_DIRLIGHT + type],context);

		switch (type)
		{
		case 0:
			if (SOFTSHADOW)
			{
				BindPS(pixelShaders[PSTYPE_DIRLIGHT_SOFT], context);
			}
			else
			{
				BindPS(pixelShaders[PSTYPE_DIRLIGHT], context);
			}
			break;
		case 1:
			BindPS(pixelShaders[PSTYPE_POINTLIGHT], context);
			break;
		case 2:
			BindPS(pixelShaders[PSTYPE_SPOTLIGHT], context);
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
					BindTexturePS(l->shadowMaps_dirLight[shmap].depth->shaderResource,4+shmap,context);
				}
				UpdateBuffer(constantBuffers[CBTYPE_DIRLIGHT],lcb,context);

				Draw(3, context);
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
					BindTexturePS(Light::shadowMaps_pointLight[l->shadowMap_index].depth->shaderResource, 7, context);
				}
				UpdateBuffer(constantBuffers[CBTYPE_POINTLIGHT], lcb, context);

				Draw(240, context);
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
					BindTexturePS(Light::shadowMaps_spotLight[l->shadowMap_index].depth->shaderResource, 4, context);
				}
				UpdateBuffer(constantBuffers[CBTYPE_SPOTLIGHT], lcb, context);

				Draw(192, context);
			}
		}


	}

	}
}
void wiRenderer::DrawVolumeLights(Camera* camera, DeviceContext context)
{
	
	static thread_local Frustum frustum = Frustum();
	frustum.ConstructFrustum(min(camera->zFarP, GetScene().worldInfo.fogSEH.y), camera->Projection, camera->View);

		
		CulledList culledObjects;
		if(spTree_lights)
			wiSPTree::getVisible(spTree_lights->root,frustum,culledObjects);

		if(!culledObjects.empty())
		{

		BindPrimitiveTopology(TRIANGLELIST,context);
		BindVertexLayout(nullptr);
		BindVertexBuffer(nullptr, 0, 0, context);
		BindIndexBuffer(nullptr, context);
		BindBlendState(blendStates[BSTYPE_ADDITIVE],context);
		BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD],STENCILREF_DEFAULT,context);
		BindRasterizerState(rasterizers[RSTYPE_DOUBLESIDED],context);

		
		BindPS(pixelShaders[PSTYPE_VOLUMELIGHT],context);

		BindConstantBufferPS(constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), context);
		BindConstantBufferVS(constantBuffers[CBTYPE_VOLUMELIGHT], CB_GETBINDSLOT(VolumeLightCB), context);


		for(int type=1;type<3;++type){

			
			if(type<=1){
				BindVS(vertexShaders[VSTYPE_VOLUMEPOINTLIGHT],context);
			}
			else{
				BindVS(vertexShaders[VSTYPE_VOLUMESPOTLIGHT],context);
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

					UpdateBuffer(constantBuffers[CBTYPE_VOLUMELIGHT],lcb,context);

					if(type<=1)
						Draw(108,context);
					else
						Draw(192,context);
				}
			}

		}

	}
}


void wiRenderer::DrawLensFlares(DeviceContext context, TextureView depth){
	
		
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
				wiLensFlare::Draw(depth,context,flarePos,l->lensFlareRimTextures);

		}

	}
}
void wiRenderer::ClearShadowMaps(DeviceContext context){
	if (GetGameSpeed())
	{
		for (unsigned int index = 0; index < Light::shadowMaps_pointLight.size(); ++index) {
			Light::shadowMaps_pointLight[index].Activate(context);
		}
		for (unsigned int index = 0; index < Light::shadowMaps_spotLight.size(); ++index) {
			Light::shadowMaps_spotLight[index].Activate(context);
		}

		//for (Light* l : lights)
		//{
		//	for (unsigned int i = 0; i < l->shadowMaps_dirLight.size(); ++i)
		//	{
		//		l->shadowMaps_dirLight[i].Activate(context);
		//	}
		//}
	}
}
void wiRenderer::DrawForShadowMap(DeviceContext context)
{
	if (GameSpeed) {

		CulledList culledLights;
		if (spTree_lights)
			wiSPTree::getVisible(spTree_lights->root, cam->frustum, culledLights);

		if (culledLights.size() > 0)
		{

			BindPrimitiveTopology(TRIANGLELIST, context);
			BindVertexLayout(vertexLayouts[VLTYPE_EFFECT], context);


			BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_DEFAULT, context);

			BindBlendState(blendStates[BSTYPE_OPAQUE], context);

			BindPS(pixelShaders[PSTYPE_SHADOW], context);

			BindVS(vertexShaders[VSTYPE_SHADOW], context);


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

						auto texDesc = l->shadowMaps_dirLight[index].GetDesc();
						if (texDesc.Height != SHADOWMAPRES)
						{
							// Create the shadow map
							l->shadowMaps_dirLight[index].Initialize(SHADOWMAPRES, SHADOWMAPRES, 0, true);
						}

						l->shadowMaps_dirLight[index].Activate(context);

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
											BindRasterizerState(rasterizers[RSTYPE_SHADOW], context);
										else
											BindRasterizerState(rasterizers[RSTYPE_SHADOW_DOUBLESIDED], context);

										//D3D11_MAPPED_SUBRESOURCE mappedResource;
										static thread_local ShadowCB* cb = new ShadowCB;
										(*cb).mVP = l->shadowCam[index].getVP();
										UpdateBuffer(constantBuffers[CBTYPE_SHADOW], cb, context);


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

										Mesh::UpdateRenderableInstances(visibleInstances.size(), context);


										BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), context);
										BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), context);
										BindIndexBuffer(mesh->meshIndexBuff, context);


										int matsiz = mesh->materialIndices.size();
										int m = 0;
										for (Material* iMat : mesh->materials) {

											if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
												BindTexturePS(iMat->texture, 0, context);



												static thread_local MaterialCB* mcb = new MaterialCB;
												(*mcb).Create(*iMat, m);

												UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, context);


												DrawIndexedInstanced(mesh->indices.size(), visibleInstances.size(), context);

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

					Light::shadowMaps_spotLight[i].Set(context);
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
										BindRasterizerState(rasterizers[RSTYPE_SHADOW], context);
									else
										BindRasterizerState(rasterizers[RSTYPE_SHADOW_DOUBLESIDED], context);

									//D3D11_MAPPED_SUBRESOURCE mappedResource;
									static thread_local ShadowCB* cb = new ShadowCB;
									(*cb).mVP = l->shadowCam[index].getVP();
									UpdateBuffer(constantBuffers[CBTYPE_SHADOW], cb, context);


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

									Mesh::UpdateRenderableInstances(visibleInstances.size(), context);


									BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), context);
									BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), context);
									BindIndexBuffer(mesh->meshIndexBuff, context);


									int matsiz = mesh->materialIndices.size();
									int m = 0;
									for (Material* iMat : mesh->materials) {

										if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
											BindTexturePS(iMat->texture, 0, context);



											static thread_local MaterialCB* mcb = new MaterialCB;
											(*mcb).Create(*iMat, m);

											UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, context);


											DrawIndexedInstanced(mesh->indices.size(), visibleInstances.size(), context);

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
				BindPS(pixelShaders[PSTYPE_SHADOWCUBEMAPRENDER], context);
				BindVS(vertexShaders[VSTYPE_SHADOWCUBEMAPRENDER], context);
				BindGS(geometryShaders[GSTYPE_SHADOWCUBEMAPRENDER], context);

				BindConstantBufferPS(constantBuffers[CBTYPE_POINTLIGHT], CB_GETBINDSLOT(PointLightCB), context);
				BindConstantBufferGS(constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubeMapRenderCB), context);

				int i = 0;
				for (Light* l : orderedPointLights)
				{
					if (i >= POINTLIGHTSHADOW)
						break; 
					
					l->shadowMap_index = i;

					Light::shadowMaps_pointLight[i].Set(context);

					//D3D11_MAPPED_SUBRESOURCE mappedResource;
					static thread_local PointLightCB* lcb = new PointLightCB;
					(*lcb).enerdis = l->enerDis;
					(*lcb).pos = l->translation;
					UpdateBuffer(constantBuffers[CBTYPE_POINTLIGHT], lcb, context);

					static thread_local CubeMapRenderCB* cb = new CubeMapRenderCB;
					for (unsigned int shcam = 0; shcam < l->shadowCam.size(); ++shcam)
						(*cb).mViewProjection[shcam] = l->shadowCam[shcam].getVP();

					UpdateBuffer(constantBuffers[CBTYPE_CUBEMAPRENDER], cb, context);

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
								BindRasterizerState(rasterizers[RSTYPE_SHADOW], context);
							else
								BindRasterizerState(rasterizers[RSTYPE_SHADOW_DOUBLESIDED], context);



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

							Mesh::UpdateRenderableInstances(visibleInstances.size(), context);


							BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), context);
							BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), context);
							BindIndexBuffer(mesh->meshIndexBuff, context);


							int matsiz = mesh->materialIndices.size();
							int m = 0;
							for (Material* iMat : mesh->materials) {
								if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
									BindTexturePS(iMat->texture, 0, context);

									static thread_local MaterialCB* mcb = new MaterialCB;
									(*mcb).Create(*iMat, m);

									UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, context);

									DrawIndexedInstanced(mesh->indices.size(), visibleInstances.size(), context);
								}
								m++;
							}
						}
						visibleInstances.clear();
					}

					i++;
				}

				BindGS(nullptr, context);
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

void wiRenderer::DrawWorld(Camera* camera, bool DX11Eff, int tessF, DeviceContext context
				  , bool BlackOut, bool isReflection, SHADERTYPE shaded
				  , TextureView refRes, bool grass, GRAPHICSTHREAD thread)
{
	CulledCollection culledRenderer;
	CulledList culledObjects;
	if(spTree)
		wiSPTree::getVisible(spTree->root, camera->frustum,culledObjects,wiSPTree::SortType::SP_TREE_SORT_FRONT_TO_BACK);
	else return;

	if(!culledObjects.empty())
	{	
		//// sort opaques front to back
		//vector<Cullable*> sortedObjects(culledObjects.begin(), culledObjects.end());
		//for (unsigned int i = 0; i < sortedObjects.size() - 1; ++i)
		//{
		//	for (unsigned int j = 1; j < sortedObjects.size(); ++j)
		//	{
		//		if (wiMath::Distance(cam->translation, ((Object*)sortedObjects[i])->translation) >
		//			wiMath::Distance(cam->translation, ((Object*)sortedObjects[j])->translation))
		//		{
		//			Cullable* swap = sortedObjects[i];
		//			sortedObjects[i] = sortedObjects[j];
		//			sortedObjects[j] = swap;
		//		}
		//	}
		//}

		for(Cullable* object : culledObjects){
			culledRenderer[((Object*)object)->mesh].insert((Object*)object);
			if(grass){
				for(wiHairParticle* hair : ((Object*)object)->hParticleSystems){
					hair->Draw(camera,context);
				}
			}
		}

		if(DX11Eff && tessF) 
			BindPrimitiveTopology(PATCHLIST,context);
		else		
			BindPrimitiveTopology(TRIANGLELIST,context);
		BindVertexLayout(vertexLayouts[VLTYPE_EFFECT],context);

		if(DX11Eff && tessF)
			BindVS(vertexShaders[VSTYPE_EFFECT],context);
		else
		{
			if (isReflection)
			{
				BindVS(vertexShaders[VSTYPE_EFFECT_REFLECTION], context);
			}
			else
			{
				BindVS(vertexShaders[VSTYPE_EFFECT10], context);
			}
		}
		if(DX11Eff && tessF)
			BindHS(hullShaders[HSTYPE_EFFECT],context);
		else
			BindHS(nullptr,context);
		if(DX11Eff && tessF) 
			BindDS(domainShaders[DSTYPE_EFFECT],context);
		else		
			BindDS(nullptr,context);

		if (wireRender)
			BindPS(pixelShaders[PSTYPE_SIMPLEST], context);
		else
			if (BlackOut)
				BindPS(pixelShaders[PSTYPE_BLACKOUT], context);
			else if (shaded == SHADERTYPE_NONE)
				BindPS(pixelShaders[PSTYPE_TEXTUREONLY], context);
			else if (shaded == SHADERTYPE_DEFERRED)
				BindPS(pixelShaders[PSTYPE_EFFECT], context);
			else if (shaded == SHADERTYPE_FORWARD_SIMPLE)
				BindPS(pixelShaders[PSTYPE_EFFECT_FORWARDSIMPLE], context);
			else
				return;


		BindBlendState(blendStates[BSTYPE_OPAQUE],context);

		if(!wireRender) {
			if (enviroMap != nullptr)
			{
				BindTexturePS(enviroMap, 0, context);
			}
			else
			{
				UnbindTextures(0, 1, context);
			}
			if(refRes != nullptr) 
				BindTexturePS(refRes,1,context);
		}


		for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
			Mesh* mesh = iter->first;
			CulledObjectList& visibleInstances = iter->second;


			if(!mesh->doubleSided)
				BindRasterizerState(wireRender?rasterizers[RSTYPE_WIRE]:rasterizers[RSTYPE_FRONT],context);
			else
				BindRasterizerState(wireRender?rasterizers[RSTYPE_WIRE]:rasterizers[RSTYPE_DOUBLESIDED],context);

			int matsiz = mesh->materialIndices.size();
			
			//if(DX11 && tessF){
			//	static thread_local ConstantBuffer* cb = new ConstantBuffer;
			//	if(matsiz == 1) (*cb).mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,0,0,0);
			//	else if(matsiz == 2) (*cb).mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,mesh->materials[1]->hasDisplacementMap,0,0);
			//	else if(matsiz == 3) (*cb).mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,mesh->materials[1]->hasDisplacementMap,mesh->materials[2]->hasDisplacementMap,0);
			//	else if(matsiz == 4) (*cb).mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,mesh->materials[1]->hasDisplacementMap,mesh->materials[2]->hasDisplacementMap,mesh->materials[3]->hasDisplacementMap);

			//	UpdateBuffer(constantBuffer,cb,context);
			//}

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

			Mesh::UpdateRenderableInstances(visibleInstances.size(), context);
				
				
			BindIndexBuffer(mesh->meshIndexBuff,context);
			BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),context);
			BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),context);

			int m=0;
			for(Material* iMat : mesh->materials){

				if(!iMat->IsTransparent() && !iMat->isSky && !iMat->water){
					
					if(iMat->shadeless)
						BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT],STENCILREF_SHADELESS,context);
					if(iMat->subsurface_scattering)
						BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT],STENCILREF_SKIN,context);
					else
						BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT],mesh->stencilRef,context);


					static thread_local MaterialCB* mcb = new MaterialCB;
					(*mcb).Create(*iMat, m);

					UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, context);
					

					if(!wireRender) BindTexturePS(iMat->texture,3,context);
					if(!wireRender) BindTexturePS(iMat->refMap,4,context);
					if(!wireRender) BindTexturePS(iMat->normalMap,5,context);
					if(!wireRender) BindTexturePS(iMat->specularMap,6,context);
					if(DX11Eff)
						BindTextureDS(iMat->displacementMap,0,context);
					
					DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),context);
				}
				m++;
			}

		}

		
		BindPS(nullptr,context);
		BindVS(nullptr,context);
		BindDS(nullptr,context);
		BindHS(nullptr,context);

	}

}

void wiRenderer::DrawWorldTransparent(Camera* camera, TextureView refracRes, TextureView refRes
	, TextureView waterRippleNormals, TextureView depth, DeviceContext context)
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

		BindPrimitiveTopology(TRIANGLELIST,context);
		BindVertexLayout(vertexLayouts[VLTYPE_EFFECT],context);
		BindVS(vertexShaders[VSTYPE_EFFECT10],context);

	
		if (!wireRender)
		{

			if (enviroMap != nullptr)
			{
				BindTexturePS(enviroMap, 0, context);
			}
			else
			{
				UnbindTextures(0, 1, context);
			}
			BindTexturePS(refRes,1,context);
			BindTexturePS(refracRes,2,context);
			BindTexturePS(depth,7,context);
			BindTexturePS(waterRippleNormals, 8, context);
		}


		BindBlendState(blendStates[BSTYPE_OPAQUE],context);
	
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
				BindRasterizerState(wireRender ? rasterizers[RSTYPE_WIRE] : rasterizers[RSTYPE_FRONT], context);
			else
				BindRasterizerState(wireRender ? rasterizers[RSTYPE_WIRE] : rasterizers[RSTYPE_DOUBLESIDED], context);


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

			Mesh::UpdateRenderableInstances(visibleInstances.size(), context);

				
			BindIndexBuffer(mesh->meshIndexBuff,context);
			BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),context);
			BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),context);
				

			int m=0;
			for(Material* iMat : mesh->materials){
				if (iMat->isSky)
					continue;

				if(iMat->IsTransparent() || iMat->IsWater())
				{
					static thread_local MaterialCB* mcb = new MaterialCB;
					(*mcb).Create(*iMat, m);

					UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, context);

					if(!wireRender) BindTexturePS(iMat->texture,3,context);
					if(!wireRender) BindTexturePS(iMat->refMap,4,context);
					if(!wireRender) BindTexturePS(iMat->normalMap,5,context);
					if(!wireRender) BindTexturePS(iMat->specularMap,6,context);

					if (iMat->IsTransparent() && lastRenderType != RENDERTYPE_TRANSPARENT)
					{
						lastRenderType = RENDERTYPE_TRANSPARENT;

						BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_TRANSPARENT, context);
						BindPS(wireRender ? pixelShaders[PSTYPE_SIMPLEST] : pixelShaders[PSTYPE_EFFECT_TRANSPARENT], context);
					}
					if (iMat->IsWater() && lastRenderType != RENDERTYPE_WATER)
					{
						lastRenderType = RENDERTYPE_WATER;

						BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_EMPTY, context);
						BindPS(wireRender ? pixelShaders[PSTYPE_SIMPLEST] : pixelShaders[PSTYPE_WATER], context);
						BindRasterizerState(wireRender ? rasterizers[RSTYPE_WIRE] : rasterizers[RSTYPE_DOUBLESIDED], context);
					}
					
					DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),context);
				}
				m++;
			}
		}
	}
	
}


void wiRenderer::DrawSky(DeviceContext context, bool isReflection)
{
	if (enviroMap == nullptr)
		return;

	BindPrimitiveTopology(TRIANGLELIST,context);
	BindRasterizerState(rasterizers[RSTYPE_BACK],context);
	BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD],STENCILREF_SKY,context);
	BindBlendState(blendStates[BSTYPE_OPAQUE],context);
	
	if (!isReflection)
	{
		BindVS(vertexShaders[VSTYPE_SKY], context);
		BindPS(pixelShaders[PSTYPE_SKY], context);
	}
	else
	{
		BindVS(vertexShaders[VSTYPE_SKY_REFLECTION], context);
		BindPS(pixelShaders[PSTYPE_SKY_REFLECTION], context);
	}
	
	BindTexturePS(enviroMap,0,context);

	BindVertexBuffer(nullptr,0,0,context);
	BindVertexLayout(nullptr, context);
	Draw(240,context);
}
void wiRenderer::DrawSun(DeviceContext context)
{
	BindPrimitiveTopology(TRIANGLELIST, context);
	BindRasterizerState(rasterizers[RSTYPE_BACK], context);
	BindDepthStencilState(depthStencils[DSSTYPE_DEPTHREAD], STENCILREF_SKY, context);
	BindBlendState(blendStates[BSTYPE_ADDITIVE], context);

	BindVS(vertexShaders[VSTYPE_SKY], context);
	BindPS(pixelShaders[PSTYPE_SUN], context);

	BindVertexBuffer(nullptr, 0, 0, context);
	BindVertexLayout(nullptr, context);
	Draw(240, context);
}

void wiRenderer::DrawDecals(Camera* camera, DeviceContext context, TextureView depth)
{
	bool boundCB = false;
	for (Model* model : GetScene().models)
	{
		if (model->decals.empty())
			continue;

		if (!boundCB)
		{
			boundCB = true;
			BindConstantBufferPS(constantBuffers[CBTYPE_DECAL], CB_GETBINDSLOT(DecalCB),context);
		}

		BindTexturePS(depth, 1, context);
		BindVS(vertexShaders[VSTYPE_DECAL], context);
		BindPS(pixelShaders[PSTYPE_DECAL], context);
		BindRasterizerState(rasterizers[RSTYPE_BACK], context);
		BindBlendState(blendStates[BSTYPE_TRANSPARENT], context);
		BindDepthStencilState(depthStencils[DSSTYPE_STENCILREAD_MATCH], STENCILREF::STENCILREF_DEFAULT, context);
		BindVertexLayout(nullptr, context);
		BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST, context);

		for (Decal* decal : model->decals) {

			if ((decal->texture || decal->normal) && camera->frustum.CheckBox(decal->bounds.corners)) {

				BindTexturePS(decal->texture, 2, context);
				BindTexturePS(decal->normal, 3, context);

				static thread_local MiscCB* dcbvs = new MiscCB;
				(*dcbvs).mTransform =XMMatrixTranspose(XMLoadFloat4x4(&decal->world)*camera->GetViewProjection());
				UpdateBuffer(constantBuffers[CBTYPE_MISC], dcbvs, context);

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
				UpdateBuffer(constantBuffers[CBTYPE_DECAL], dcbps, context);

				Draw(36, context);

			}

		}
	}
}

void wiRenderer::UpdateWorldCB(DeviceContext context)
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

	UpdateBuffer(constantBuffers[CBTYPE_WORLD], cb, context);
}
void wiRenderer::UpdateFrameCB(DeviceContext context)
{
	static thread_local FrameCB* cb = new FrameCB;

	auto& wind = GetScene().wind;
	(*cb).mWindTime = wind.time;
	(*cb).mWindRandomness = wind.randomness;
	(*cb).mWindWaveSize = wind.waveSize;
	(*cb).mWindDirection = wind.direction;

	UpdateBuffer(constantBuffers[CBTYPE_FRAME], cb, context);
}
void wiRenderer::UpdateCameraCB(DeviceContext context)
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

	UpdateBuffer(constantBuffers[CBTYPE_CAMERA], cb, context);
}
void wiRenderer::SetClipPlane(XMFLOAT4 clipPlane, DeviceContext context)
{
	UpdateBuffer(constantBuffers[CBTYPE_CLIPPLANE], &clipPlane, context);
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
	
	Lock();

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

	Unlock();


	LoadWorldInfo(dir, name);


	return model;
}
void wiRenderer::LoadWorldInfo(const string& dir, const string& name)
{
	LoadWiWorldInfo(dir, name+".wiw", GetScene().worldInfo, GetScene().wind);

	Lock();
	UpdateWorldCB(getImmediateContext());
	Unlock();
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

	Lock();

	DeviceContext context = getImmediateContext();

	probe->cubeMap.Activate(context, 0, 0, 0, 1);

	BindVertexLayout(vertexLayouts[VLTYPE_EFFECT], context);
	BindPrimitiveTopology(TRIANGLELIST, context);
	BindBlendState(blendStates[BSTYPE_OPAQUE], context);

	BindPS(pixelShaders[PSTYPE_ENVMAP], context);
	BindVS(vertexShaders[VSTYPE_ENVMAP], context);
	BindGS(geometryShaders[GSTYPE_ENVMAP], context);

	BindConstantBufferGS(constantBuffers[CBTYPE_CUBEMAPRENDER], CB_GETBINDSLOT(CubeMapRenderCB), context);


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

	UpdateBuffer(constantBuffers[CBTYPE_CUBEMAPRENDER], cb, context);


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
				BindRasterizerState(rasterizers[RSTYPE_FRONT], context);
			else
				BindRasterizerState(rasterizers[RSTYPE_DOUBLESIDED], context);



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

			Mesh::UpdateRenderableInstances(visibleInstances.size(), context);


			BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), context);
			BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), context);
			BindIndexBuffer(mesh->meshIndexBuff, context);


			int matsiz = mesh->materialIndices.size();
			int m = 0;
			for (Material* iMat : mesh->materials) {
				if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {

					if (iMat->shadeless)
						BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_SHADELESS, context);
					if (iMat->subsurface_scattering)
						BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], STENCILREF_SKIN, context);
					else
						BindDepthStencilState(depthStencils[DSSTYPE_DEFAULT], mesh->stencilRef, context);

					BindTexturePS(iMat->texture, 3, context);

					static thread_local MaterialCB* mcb = new MaterialCB;
					(*mcb).Create(*iMat, m);

					UpdateBuffer(constantBuffers[CBTYPE_MATERIAL], mcb, context);

					DrawIndexedInstanced(mesh->indices.size(), visibleInstances.size(), context);
				}
				m++;
			}
		}
		visibleInstances.clear();
	}


	BindGS(nullptr, context);

	probe->cubeMap.Deactivate(context);

	GenerateMips(probe->cubeMap.shaderResource[0], context);

	enviroMap = probe->cubeMap.shaderResource.front();

	scene->environmentProbes.push_back(probe);

	Unlock();
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