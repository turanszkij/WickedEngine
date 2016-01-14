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
Sampler wiRenderer::ssClampLin,wiRenderer::ssClampPoi,wiRenderer::ssMirrorLin,wiRenderer::ssMirrorPoi,wiRenderer::ssWrapLin,wiRenderer::ssWrapPoi
		,wiRenderer::ssClampAni,wiRenderer::ssWrapAni,wiRenderer::ssMirrorAni,wiRenderer::ssComp;

map<DeviceContext,long> wiRenderer::drawCalls;

int wiRenderer::SCREENWIDTH=1280,wiRenderer::SCREENHEIGHT=720,wiRenderer::SHADOWMAPRES=1024,wiRenderer::SOFTSHADOW=2
	,wiRenderer::POINTLIGHTSHADOW=2,wiRenderer::POINTLIGHTSHADOWRES=256, wiRenderer::SPOTLIGHTSHADOW=2, wiRenderer::SPOTLIGHTSHADOWRES=512;
bool wiRenderer::HAIRPARTICLEENABLED=true,wiRenderer::EMITTERSENABLED=true;
bool wiRenderer::wireRender = false, wiRenderer::debugSpheres = false, wiRenderer::debugBoneLines = false, wiRenderer::debugBoxes = false;
BlendState		wiRenderer::blendState, wiRenderer::blendStateTransparent, wiRenderer::blendStateAdd;
BufferResource			wiRenderer::constantBuffer, wiRenderer::staticCb, wiRenderer::shCb, wiRenderer::pixelCB, wiRenderer::matCb
	, wiRenderer::lightCb[3], wiRenderer::tessBuf, wiRenderer::lineBuffer, wiRenderer::skyCb
	, wiRenderer::trailCB, wiRenderer::lightStaticCb, wiRenderer::vLightCb,wiRenderer::cubeShCb,wiRenderer::fxCb
	, wiRenderer::decalCbVS, wiRenderer::decalCbPS, wiRenderer::viewPropCB;
VertexShader		wiRenderer::vertexShader10, wiRenderer::vertexShader, wiRenderer::shVS, wiRenderer::lineVS, wiRenderer::trailVS
	,wiRenderer::waterVS,wiRenderer::lightVS[3],wiRenderer::vSpotLightVS,wiRenderer::vPointLightVS,wiRenderer::cubeShVS,wiRenderer::sOVS,wiRenderer::decalVS;
PixelShader		wiRenderer::pixelShader, wiRenderer::shPS, wiRenderer::linePS, wiRenderer::trailPS, wiRenderer::simplestPS,wiRenderer::blackoutPS
	,wiRenderer::textureonlyPS,wiRenderer::waterPS,wiRenderer::transparentPS,wiRenderer::lightPS[3],wiRenderer::vLightPS,wiRenderer::cubeShPS
	,wiRenderer::decalPS,wiRenderer::fowardSimplePS;
GeometryShader wiRenderer::cubeShGS,wiRenderer::sOGS;
HullShader		wiRenderer::hullShader;
DomainShader		wiRenderer::domainShader;
VertexLayout		wiRenderer::vertexLayout, wiRenderer::lineIL,wiRenderer::trailIL, wiRenderer::sOIL;
Sampler		wiRenderer::texSampler, wiRenderer::mapSampler, wiRenderer::comparisonSampler, wiRenderer::mirSampler,wiRenderer::pointSampler;
RasterizerState	wiRenderer::rasterizerState, wiRenderer::rssh, wiRenderer::nonCullRSsh, wiRenderer::wireRS, wiRenderer::nonCullRS,wiRenderer::nonCullWireRS
	,wiRenderer::backFaceRS;
DepthStencilState	wiRenderer::depthStencilState,wiRenderer::xRayStencilState,wiRenderer::depthReadStencilState,wiRenderer::stencilReadState
	,wiRenderer::stencilReadMatch;
PixelShader		wiRenderer::skyPS;
VertexShader		wiRenderer::skyVS;
Sampler		wiRenderer::skySampler;
TextureView wiRenderer::enviroMap,wiRenderer::colorGrading;
float wiRenderer::GameSpeed=1,wiRenderer::overrideGameSpeed=1;
int wiRenderer::visibleCount;
wiRenderTarget wiRenderer::normalMapRT, wiRenderer::imagesRT, wiRenderer::imagesRTAdd;
Camera *wiRenderer::cam = nullptr, *wiRenderer::refCam = nullptr, *wiRenderer::prevFrameCam = nullptr;
PHYSICS* wiRenderer::physicsEngine = nullptr;
Wind wiRenderer::wind;
WorldInfo wiRenderer::worldInfo;

string wiRenderer::SHADERPATH = "shaders/";
#pragma endregion

#pragma region STATIC TEMP

int wiRenderer::vertexCount;
deque<wiSprite*> wiRenderer::images;
deque<wiSprite*> wiRenderer::waterRipples;

wiSPTree* wiRenderer::spTree = nullptr;
wiSPTree* wiRenderer::spTree_lights = nullptr;
Model* wiRenderer::world = nullptr;

vector<Model*> wiRenderer::models;

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
	wiRenderer::graphicsMutex.lock();

	wiRenderer::getImmediateContext()->RSSetViewports( 1, &viewPort );
	wiRenderer::getImmediateContext()->OMSetRenderTargets( 1, &renderTargetView, 0 );
	float ClearColor[4] = { 0, 0, 0, 1.0f }; // red,green,blue,alpha
    wiRenderer::getImmediateContext()->ClearRenderTargetView( renderTargetView, ClearColor );
	//wiRenderer::getImmediateContext()->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	
	if(drawToScreen1!=nullptr)
		drawToScreen1();
	if(drawToScreen2!=nullptr)
		drawToScreen2();
	if(drawToScreen3!=nullptr)
		drawToScreen3();
	


	//wiFrameRate::Count();
	wiFrameRate::Frame();

	swapChain->Present( VSYNC, 0 );

	for(auto& d : drawCalls){
		d.second=0;
	}


	wiRenderer::getImmediateContext()->OMSetRenderTargets(NULL, NULL,NULL);

	wiRenderer::graphicsMutex.unlock();
}
void wiRenderer::ReleaseCommandLists()
{
	for(int i=0;i<GRAPHICSTHREAD_COUNT;i++)
		if(commandLists[i]){ commandLists[i]->Release(); commandLists[i]=NULL; }
}
void wiRenderer::ExecuteDeferredContexts()
{
	for (int i = 0; i<GRAPHICSTHREAD_COUNT; i++)
		if(commandLists[i]) wiRenderer::getImmediateContext()->ExecuteCommandList( commandLists[i], false );

	ReleaseCommandLists();
}
void wiRenderer::FinishCommandList(GRAPHICSTHREAD thread)
{
	SafeRelease(commandLists[thread]);
	deferredContexts[thread]->FinishCommandList( false,&commandLists[thread] );
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
	texSampler = NULL;
	mapSampler = NULL;
	comparisonSampler = NULL;
	mirSampler = NULL;

	constantBuffer = NULL;
	staticCb = NULL;
	vertexShader = NULL;
	vertexShader10 = NULL;
	pixelShader = NULL;
	vertexLayout = NULL;
	rasterizerState = NULL;
	backFaceRS=NULL;
	depthStencilState = NULL;
	xRayStencilState=NULL;
	depthReadStencilState=NULL;
	
	shVS = NULL;
	shPS = NULL;
	shCb = NULL;

	lineVS=NULL;
	linePS=NULL;
	lineIL=NULL;
	lineBuffer=NULL;

	trailCB=NULL;
	trailIL=NULL;
	trailPS=NULL;
	trailVS=NULL;
	nonCullRS=NULL;
	nonCullWireRS=NULL;

	//matIndexBuf=NULL;

	hullShader=NULL;
	domainShader=NULL;

	thread t1(wiRenderer::LoadBasicShaders);
	thread t2(wiRenderer::LoadShadowShaders);
	thread t3(wiRenderer::LoadSkyShaders);
	thread t4(wiRenderer::LoadLineShaders);
	thread t5(wiRenderer::LoadTrailShaders);
	thread t6(wiRenderer::LoadWaterShaders);
	thread t7(wiRenderer::LoadTessShaders);

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

	//vector<Armature*>a(0);
	//MaterialCollection m;
	//LoadWiMeshes("lights/","lights.wi","",lightGwiRenderer,a,m);

	wind=Wind();

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

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();
	t7.join();
}
void wiRenderer::CleanUpStatic()
{
	SafeRelease(fowardSimplePS);
	if(shVS) shVS->Release(); shVS=NULL;
	if(shPS) shPS->Release(); shPS=NULL;
	if(shCb) shCb->Release(); shCb=NULL;
	if(cubeShCb) cubeShCb->Release(); cubeShCb=NULL;
	if(lightStaticCb) lightStaticCb->Release(); lightStaticCb=NULL;
	if(vLightCb) vLightCb->Release(); vLightCb=NULL;
	SafeRelease(vSpotLightVS);
	SafeRelease(vPointLightVS);
	if(vLightPS) vLightPS->Release(); vLightPS=NULL;
	
	if(sOVS) sOVS->Release(); sOVS=NULL;
	if(sOGS) sOGS->Release(); sOGS=NULL;
	if(sOIL) sOIL->Release(); sOIL=NULL;

	if(constantBuffer) constantBuffer->Release(); constantBuffer=NULL;
	if(staticCb) staticCb->Release(); staticCb=NULL;
	if(vertexShader) vertexShader->Release(); vertexShader=NULL;
	if(vertexShader10) vertexShader10->Release(); vertexShader10=NULL;
	if(pixelShader) pixelShader->Release(); pixelShader=NULL;
	if(simplestPS) simplestPS->Release(); simplestPS=NULL;
	if(blackoutPS)blackoutPS->Release(); blackoutPS=NULL;
	if(textureonlyPS)textureonlyPS->Release(); textureonlyPS=NULL;
	if(waterPS) waterPS->Release(); waterPS=NULL;
	if(waterVS) waterVS->Release(); waterVS=NULL;
	if(transparentPS) transparentPS->Release(); transparentPS=NULL;
	if(vertexLayout) vertexLayout->Release(); vertexLayout=NULL;
	
	if(cubeShGS) cubeShGS->Release(); cubeShGS=NULL;
	if(cubeShVS) cubeShVS->Release(); cubeShVS=NULL;
	if(cubeShPS) cubeShPS->Release(); cubeShPS=NULL;
	
	SafeRelease(decalVS);
	SafeRelease(decalPS);
	SafeRelease(decalCbVS);
	SafeRelease(decalCbPS);
	SafeRelease(stencilReadMatch);

	for(int i=0;i<3;++i){
		if(lightPS[i]){
			lightPS[i]->Release();
			lightPS[i]=NULL;
		}
		if(lightVS[i]){
			lightVS[i]->Release();
			lightVS[i]=NULL;
		}
		if(lightCb[i]){
			lightCb[i]->Release(); 
			lightCb[i]=NULL;
		}
	}

	if(pixelCB) pixelCB->Release(); pixelCB=NULL;
	if(fxCb) fxCb->Release(); fxCb=NULL;

	if(texSampler) texSampler->Release(); texSampler=NULL;
	if(mapSampler) mapSampler->Release(); mapSampler=NULL;
	if(pointSampler) pointSampler->Release(); pointSampler=NULL;
	if(comparisonSampler) comparisonSampler->Release(); comparisonSampler=NULL;
	if(mirSampler) mirSampler->Release(); mirSampler=NULL;

	if(rasterizerState) rasterizerState->Release(); rasterizerState=NULL;
	if(rssh) rssh->Release(); rssh=NULL;
	if(nonCullRSsh) nonCullRSsh->Release(); nonCullRSsh=NULL;
	if(wireRS) wireRS->Release(); wireRS=NULL;
	if(nonCullWireRS) nonCullWireRS->Release(); nonCullWireRS=NULL;
	if(nonCullRS) nonCullRS->Release(); nonCullRS=NULL;
	if(backFaceRS) backFaceRS->Release(); backFaceRS=NULL;
	if(depthStencilState) depthStencilState->Release(); depthStencilState=NULL;
	if(stencilReadState) stencilReadState->Release(); stencilReadState=NULL;
	if(xRayStencilState) xRayStencilState->Release(); xRayStencilState=NULL;
	if(depthReadStencilState) depthReadStencilState->Release(); depthReadStencilState=NULL;
	if(blendState) blendState->Release(); blendState=NULL;
	if(blendStateTransparent) blendStateTransparent->Release(); blendStateTransparent=NULL;
	if(blendStateAdd) blendStateAdd->Release(); blendStateAdd=NULL;


	if(skyPS) skyPS->Release(); skyPS=NULL;
	if(skySampler) skySampler->Release(); skySampler=NULL;
	if(skyVS) skyVS->Release(); skyVS=NULL;

	if(matCb) matCb->Release(); matCb=NULL;

	if(hullShader) hullShader->Release(); hullShader=NULL; hullShader=NULL;
	if(domainShader) domainShader->Release(); domainShader=NULL; domainShader=NULL;

	if(lineVS){
		lineVS->Release();
		lineVS=NULL;
	}
	if(linePS){
		linePS->Release();
		linePS=NULL;
	}
	if(lineIL){
		lineIL->Release();
		lineIL=NULL;
	}
	if(lineBuffer){
		lineBuffer->Release();
		lineBuffer=NULL;
	}

	if(trailIL) trailIL->Release(); trailIL=NULL;
	if(trailPS) trailPS->Release(); trailPS=NULL;
	if(trailVS) trailVS->Release(); trailVS=NULL;
	if(trailCB) trailCB->Release(); trailCB=NULL;

	SafeRelease(viewPropCB);

	wiHairParticle::CleanUpStatic();
	wiEmittedParticle::CleanUpStatic();
	Cube::CleanUpStatic();
	HitSphere::CleanUpStatic();



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

	models.clear();
	
	if(spTree_lights)
		spTree_lights->CleanUp();
	spTree_lights=nullptr;

	// Do not delete!
	cam->detach();

	Transform::DeleteTree(world);
	world = nullptr;
}
XMVECTOR wiRenderer::GetSunPosition()
{
	for (Model* model : models)
	{
		for (Light* l : model->lights)
			if (l->type == Light::DIRECTIONAL)
				return -XMVector3Transform(XMVectorSet(0, -1, 0, 1), XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation)));
	}
	return XMVectorSet(0.5f,1,-0.5f,0);
}
XMFLOAT4 wiRenderer::GetSunColor()
{
	for (Model* model : models)
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
	for (Model* model : models)
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
				for (Model* model : models)
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
    D3D11_BUFFER_DESC bd;
	// Create the constant buffer
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(StaticCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &staticCb );

	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(PixelCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &pixelCB );


	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ForShadowMapCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &shCb );
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(CubeShadowCb);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &cubeShCb );

	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(MaterialCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &matCb );

	

	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(TessBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &tessBuf );

	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(LineBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &lineBuffer );

	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(XMMATRIX);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &trailCB );
	
	//
	//ZeroMemory( &bd, sizeof(bd) );
	//bd.Usage = D3D11_USAGE_DYNAMIC;
	//bd.ByteWidth = sizeof(MatIndexBuf);
	//bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	//bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &matIndexBuf );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(LightStaticCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &lightStaticCb );

	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(dLightBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &lightCb[0] );
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(pLightBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &lightCb[1] );
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(sLightBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &lightCb[2] );

	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(vLightBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &vLightCb );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(FxCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &fxCb );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(SkyBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &skyCb );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(DecalCBVS);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &decalCbVS );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(DecalCBPS);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &decalCbPS );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ViewPropCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &viewPropCB );

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
			vertexShader10 = vsinfo->vertexShader;
			vertexLayout = vsinfo->vertexLayout;
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
			sOVS = vsinfo->vertexShader;
			sOIL = vsinfo->vertexLayout;
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

		sOGS = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "sOGS.cso", wiResourceManager::GEOMETRYSHADER, nullptr, 4, pDecl));
	}


	vertexShader = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	lightVS[0] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	lightVS[1] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "pointLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	lightVS[2] = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "spotLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vSpotLightVS = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vSpotLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	vPointLightVS = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "vPointLightVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	decalVS = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "decalVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;

	pixelShader = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS.cso", wiResourceManager::PIXELSHADER));
	transparentPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS_transparent.cso", wiResourceManager::PIXELSHADER));
	simplestPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS_simplest.cso", wiResourceManager::PIXELSHADER));
	fowardSimplePS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS_forwardSimple.cso", wiResourceManager::PIXELSHADER));
	blackoutPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS_blackout.cso", wiResourceManager::PIXELSHADER));
	textureonlyPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectPS_textureonly.cso", wiResourceManager::PIXELSHADER));
	lightPS[0] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "dirLightPS.cso", wiResourceManager::PIXELSHADER));
	lightPS[1] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "pointLightPS.cso", wiResourceManager::PIXELSHADER));
	lightPS[2] = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "spotLightPS.cso", wiResourceManager::PIXELSHADER));
	vLightPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "volumeLightPS.cso", wiResourceManager::PIXELSHADER));
	decalPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "decalPS.cso", wiResourceManager::PIXELSHADER));
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
		lineVS = vsinfo->vertexShader;
		lineIL = vsinfo->vertexLayout;
	}


	linePS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "linesPS.cso", wiResourceManager::PIXELSHADER));

}
void wiRenderer::LoadTessShaders()
{
	hullShader = static_cast<HullShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectHS.cso", wiResourceManager::HULLSHADER));
	domainShader = static_cast<DomainShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "effectDS.cso", wiResourceManager::DOMAINSHADER));

}
void wiRenderer::LoadSkyShaders()
{
	skyVS = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	skyPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "skyPS.cso", wiResourceManager::PIXELSHADER));

}
void wiRenderer::LoadShadowShaders()
{

	shVS = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	cubeShVS = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;

	shPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "shadowPS.cso", wiResourceManager::PIXELSHADER));
	cubeShPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowPS.cso", wiResourceManager::PIXELSHADER));

	cubeShGS = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "cubeShadowGS.cso", wiResourceManager::GEOMETRYSHADER));

}
void wiRenderer::LoadWaterShaders()
{

	waterVS = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "waterVS.cso", wiResourceManager::VERTEXSHADER))->vertexShader;
	waterPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "waterPS.cso", wiResourceManager::PIXELSHADER));

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
		trailVS = vsinfo->vertexShader;
		trailIL = vsinfo->vertexLayout;
	}


	trailPS = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(SHADERPATH + "trailPS.cso", wiResourceManager::PIXELSHADER));

}

void wiRenderer::ReloadShaders(const string& path)
{
	//// TODO : Also delete shaders from resourcemanager!
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "effectVS10.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "sOVS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "sOGS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "effectVS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "dirLightVS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "pointLightVS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "spotLightVS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "vSpotLightVS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "vPointLightVS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "decalVS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "effectPS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "effectPS_transparent.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "effectPS_simplest.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "effectPS_forwardSimple.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "effectPS_blackout.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "effectPS_textureonly.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "dirLightPS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "pointLightPS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "spotLightPS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "volumeLightPS.cso");
	//wiResourceManager::GetGlobal()->del(SHADERPATH + "decalPS.cso");


	//LoadBasicShaders();
	////LoadLineShaders();
	////LoadTessShaders();
	////LoadSkyShaders();
	////LoadShadowShaders();
	////LoadWaterShaders();
	////LoadTrailShaders();

	if (path.length() > 0)
	{
		SHADERPATH = path;
	}

	graphicsMutex.lock();
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
	graphicsMutex.unlock();
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
	graphicsDevice->CreateSamplerState(&samplerDesc, &ssMirrorLin);
	
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
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
	graphicsDevice->CreateSamplerState(&samplerDesc, &ssMirrorPoi);

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &ssWrapLin);
	
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &ssWrapPoi);
	
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &ssClampLin);
	
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &ssClampPoi);
	
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &ssClampAni);
	
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &ssWrapAni);
	
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &ssMirrorAni);
	
	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &ssComp);


	
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &texSampler);

	ZeroMemory( &samplerDesc, sizeof(D3D11_SAMPLER_DESC) );
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &mapSampler);

	ZeroMemory( &samplerDesc, sizeof(D3D11_SAMPLER_DESC) );
	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &comparisonSampler);

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &mirSampler);

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &pointSampler);
	
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	graphicsDevice->CreateSamplerState(&samplerDesc, &skySampler);

	
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
	graphicsDevice->CreateRasterizerState(&rs,&rasterizerState);

	
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
	graphicsDevice->CreateRasterizerState(&rs,&rssh);
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
	graphicsDevice->CreateRasterizerState(&rs,&nonCullRSsh);

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
	graphicsDevice->CreateRasterizerState(&rs,&wireRS);

	
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
	graphicsDevice->CreateRasterizerState(&rs,&nonCullRS);
	
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
	graphicsDevice->CreateRasterizerState(&rs,&nonCullWireRS);
	
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
	graphicsDevice->CreateRasterizerState(&rs,&backFaceRS);

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
	graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilState);

	
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
	graphicsDevice->CreateDepthStencilState(&dsd, &stencilReadState);

	
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
	graphicsDevice->CreateDepthStencilState(&dsd, &stencilReadMatch);


	dsd.DepthEnable = true;
	dsd.StencilEnable = false;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	graphicsDevice->CreateDepthStencilState(&dsd, &depthReadStencilState);

	dsd.DepthEnable = false;
	dsd.StencilEnable=false;
	graphicsDevice->CreateDepthStencilState(&dsd, &xRayStencilState);

	
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
	graphicsDevice->CreateBlendState(&bd,&blendState);

	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bd.AlphaToCoverageEnable=false;
	graphicsDevice->CreateBlendState(&bd,&blendStateTransparent);


	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	bd.IndependentBlendEnable=true,
	bd.AlphaToCoverageEnable=false;
	graphicsDevice->CreateBlendState(&bd,&blendStateAdd);
}


Transform* wiRenderer::getTransformByName(const string& get)
{
	//auto transf = transforms.find(get);
	//if (transf != transforms.end())
	//{
	//	return transf->second;
	//}
	return world->find(get);
}
Armature* wiRenderer::getArmatureByName(const string& get)
{
	for (Model* model : models)
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
	for (Model* model : models)
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
	for (Model* model : models)
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
	for (Model* model : models)
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
	*prevFrameCam = *cam;
	cam->UpdateTransform();
	refCam->Reflect(cam);

	objectsWithTrails.clear();
	emitterSystems.clear();


	world->UpdateTransform();

	for (Model* model : models)
	{
		model->UpdateModel();
	}


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

	wind.time = (float)((wiTimer::TotalTime()) / 1000.0*GameSpeed / 2.0*3.1415)*XMVectorGetX(XMVector3Length(XMLoadFloat3(&wind.direction)))*0.1f;

}
void wiRenderer::UpdateRenderData(DeviceContext context)
{
	//UpdateObjects();

	if (context == nullptr)
		return;

	UpdatePerWorldCB(context);
		



	//if(GetGameSpeed())
	{
#ifdef USE_GPU_SKINNING
		// Vertices are going to be skinned in a shader
		BindPrimitiveTopology(POINTLIST, context);
		BindVertexLayout(sOIL, context);
		BindVS(sOVS, context);
		BindGS(sOGS, context);
		BindPS(nullptr, context);
#endif


		for (Model* model : models)
		{
			for (MeshCollection::iterator iter = model->meshes.begin(); iter != model->meshes.end(); ++iter)
			{
				Mesh* mesh = iter->second;

				if (mesh->hasArmature() && !mesh->softBody && mesh->renderable) 
				{
#ifdef USE_GPU_SKINNING
					// Upload bones for skinning to shader
					static thread_local BoneShaderBuffer* bonebuf = new BoneShaderBuffer;
					for (unsigned int k = 0; k < mesh->armature->boneCollection.size(); k++) {
						bonebuf->pose[k] = XMMatrixTranspose(XMLoadFloat4x4(&mesh->armature->boneCollection[k]->boneRelativity));
						bonebuf->prev[k] = XMMatrixTranspose(XMLoadFloat4x4(&mesh->armature->boneCollection[k]->boneRelativityPrev));
					}
					UpdateBuffer(mesh->boneBuffer, bonebuf, context);

					// Do the skinning
					BindVertexBuffer(mesh->meshVertBuff, 0, sizeof(SkinnedVertex), context);
					BindConstantBufferVS(mesh->boneBuffer, 1, context);
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
		// Unload skinning shader
		BindGS(nullptr, context);
		BindVS(nullptr, context);
		BindStreamOutTarget(nullptr, context);
#endif
		

//#ifdef USE_GPU_SKINNING
//		DrawForSO(context);
//#endif


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
	if (world == nullptr)
	{
		return;
	}

	world->decals.push_back(decal);
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
	if(debugSpheres){
		//context->IASetPrimitiveTopology( PRIMITIVETOPOLOGY_TRIANGLESTRIP );
		//context->IASetInputLayout( lineIL );
		BindPrimitiveTopology(TRIANGLESTRIP,context);
		BindVertexLayout(lineIL,context);
	
		//context->RSSetState(rasterizerState);
		//context->OMSetDepthStencilState(xRayStencilState, STENCILREF_EMPTY);

	
		//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		//UINT sampleMask   = 0xffffffff;
		//context->OMSetBlendState(blendStateTransparent, blendFactor, sampleMask);
		BindRasterizerState(rasterizerState,context);
		BindDepthStencilState(xRayStencilState,STENCILREF_EMPTY,context);
		BindBlendState(blendStateTransparent,context);


		//context->VSSetShader( lineVS, NULL, 0 );
		//context->PSSetShader( linePS, NULL, 0 );
		BindPS(linePS,context);
		BindVS(lineVS,context);
		
		//context->VSSetConstantBuffers( 0, 1, &lineBuffer );
		//
		//UINT stride = sizeof( XMFLOAT3A );
		//UINT offset = 0;
		//context->IASetVertexBuffers( 0, 1, &HitSphere::vertexBuffer, &stride, &offset );

		BindConstantBufferVS(lineBuffer,0,context);
		BindVertexBuffer(HitSphere::vertexBuffer,0,sizeof(XMFLOAT3A),context);

		for (unsigned int i = 0; i<spheres.size(); i++){
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			LineBuffer sb;
			sb.mWorldViewProjection=XMMatrixTranspose(
				XMMatrixRotationQuaternion(XMLoadFloat4(&camera->rotation))*
				XMMatrixScaling( spheres[i]->radius,spheres[i]->radius,spheres[i]->radius ) *
				XMMatrixTranslationFromVector( XMLoadFloat3(&spheres[i]->translation) )
				*camera->GetViewProjection()
				);

			XMFLOAT4A propColor;
			if(spheres[i]->TYPE==HitSphere::HitSphereType::HITTYPE)      propColor = XMFLOAT4A(0.1098f,0.4196f,1,1);
			else if(spheres[i]->TYPE==HitSphere::HitSphereType::INVTYPE) propColor=XMFLOAT4A(0,0,0,1);
			else if(spheres[i]->TYPE==HitSphere::HitSphereType::ATKTYPE) propColor=XMFLOAT4A(0.96f,0,0,1);
			sb.color=propColor;

			UpdateBuffer(lineBuffer,&sb,context);
			//LineBuffer* dataPtr;
			//context->Map(lineBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr = (LineBuffer*)mappedResource.pData;
			//memcpy(dataPtr,&sb,sizeof(LineBuffer));
			//context->Unmap(lineBuffer,0);


			//context->Draw((HitSphere::RESOLUTION+1)*2,0);
			Draw((HitSphere::RESOLUTION+1)*2,context);

			//context->Draw(RESOLUTION,0);
			//context->Draw(RESOLUTION,RESOLUTION+1);
			//context->Draw(RESOLUTION,(RESOLUTION+1)*2);
		}
	}
	
}
void wiRenderer::DrawDebugBoneLines(Camera* camera, DeviceContext context)
{
	if(debugBoneLines){
		BindPrimitiveTopology(LINELIST,context);
		BindVertexLayout(lineIL,context);
	
		BindRasterizerState(rssh,context);
		BindDepthStencilState(xRayStencilState,STENCILREF_EMPTY,context);
		BindBlendState(blendState,context);


		BindPS(linePS,context);
		BindVS(lineVS,context);

		BindConstantBufferVS(lineBuffer,0,context);

		for (unsigned int i = 0; i<boneLines.size(); i++){
			LineBuffer sb;
			sb.mWorldViewProjection = XMMatrixTranspose(
				XMLoadFloat4x4(&boneLines[i]->desc.transform)
				*camera->GetViewProjection()
				);
			sb.color = boneLines[i]->desc.color;

			UpdateBuffer(lineBuffer, &sb, context);

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
	BindVertexLayout(lineIL, context);

	BindRasterizerState(rssh, context);
	BindDepthStencilState(xRayStencilState, STENCILREF_EMPTY, context);
	BindBlendState(blendState, context);


	BindPS(linePS, context);
	BindVS(lineVS, context);

	BindConstantBufferVS(lineBuffer, 0, context);

	for (unsigned int i = 0; i<linesTemp.size(); i++){
		LineBuffer sb;
		sb.mWorldViewProjection = XMMatrixTranspose(
			XMLoadFloat4x4(&linesTemp[i]->desc.transform)
			*camera->GetViewProjection()
			);
		sb.color = linesTemp[i]->desc.color;

		UpdateBuffer(lineBuffer, &sb, context);

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
		//context->IASetPrimitiveTopology( PRIMITIVETOPOLOGY_LINELIST );
		//context->IASetInputLayout( lineIL );
		BindPrimitiveTopology(LINELIST,context);
		BindVertexLayout(lineIL,context);
	
		//context->RSSetState(nonCullWireRS);
		//context->OMSetDepthStencilState(xRayStencilState, STENCILREF_EMPTY);

	
		//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		//UINT sampleMask   = 0xffffffff;
		//context->OMSetBlendState(blendState, blendFactor, sampleMask);

		BindRasterizerState(nonCullWireRS,context);
		BindDepthStencilState(xRayStencilState,STENCILREF_EMPTY,context);
		BindBlendState(blendState,context);


		//context->VSSetShader( lineVS, NULL, 0 );
		//context->PSSetShader( linePS, NULL, 0 );
		BindPS(linePS,context);
		BindVS(lineVS,context);
		
		//vector<Lines> edges(0);
		/*for(Object* object:objects){
			edges.push_back(Lines(object->frameBB.corners[0],object->frameBB.corners[1],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[1],object->frameBB.corners[2],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[0],object->frameBB.corners[3],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[0],object->frameBB.corners[4],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[1],object->frameBB.corners[5],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[4],object->frameBB.corners[5],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[5],object->frameBB.corners[6],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[4],object->frameBB.corners[7],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[2],object->frameBB.corners[6],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[3],object->frameBB.corners[7],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[2],object->frameBB.corners[3],XMFLOAT4A(1,0,0,1)));
			edges.push_back(Lines(object->frameBB.corners[6],object->frameBB.corners[7],XMFLOAT4A(1,0,0,1)));
		}*/

		//iterateSPTree(spTree->root,edges);

		//UINT stride = sizeof( XMFLOAT3A );
		//UINT offset = 0;
		//context->IASetVertexBuffers( 0, 1, &Cube::vertexBuffer, &stride, &offset );
		//context->IASetIndexBuffer(Cube::indexBuffer,DXGI_FORMAT_R32_UINT,0);

		BindVertexBuffer(Cube::vertexBuffer,0,sizeof(XMFLOAT3A),context);
		BindIndexBuffer(Cube::indexBuffer,context);
		BindConstantBufferVS(lineBuffer,0,context);

		for (unsigned int i = 0; i<cubes.size(); i++){
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			LineBuffer sb;
			sb.mWorldViewProjection=XMMatrixTranspose(XMLoadFloat4x4(&cubes[i].desc.transform)*camera->GetViewProjection());
			sb.color=cubes[i].desc.color;

			UpdateBuffer(lineBuffer,&sb,context);
			//LineBuffer* dataPtr;
			//context->Map(lineBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr = (LineBuffer*)mappedResource.pData;
			//memcpy(dataPtr,&sb,sizeof(LineBuffer));
			//context->Unmap(lineBuffer,0);

			//context->VSSetConstantBuffers( 0, 1, &lineBuffer );


			//context->DrawIndexed(24,0,0);
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
	BindVertexLayout(trailIL,context);

	BindRasterizerState(wireRender?nonCullWireRS:nonCullRS,context);
	BindDepthStencilState(depthStencilState,STENCILREF_EMPTY,context);
	BindBlendState(blendState,context);

	BindPS(trailPS,context);
	BindVS(trailVS,context);
	
	BindTexturePS(refracRes,0,context);
	BindSamplerPS(mirSampler,0,context);

	for (Object* o : objectsWithTrails)
	{
		if(o->trailBuff && o->trail.size()>=4){

			BindTexturePS(o->trailDistortTex, 1, context);
			BindTexturePS(o->trailTex, 2, context);

			BindConstantBufferVS(trailCB,0,context);

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
	frustum.ConstructFrustum(min(camera->zFarP, worldInfo.fogSEH.y),camera->Projection,camera->View);
	
	CulledList culledObjects;
	if(spTree_lights)
		wiSPTree::getVisible(spTree_lights->root,frustum,culledObjects);

	if(!culledObjects.empty())
	{


	BindPrimitiveTopology(TRIANGLELIST,context);
	BindConstantBufferVS(staticCb,0,context);
	BindConstantBufferPS(lightStaticCb,0,context);
	
	BindTexturePS(depth,0,context);
	BindTexturePS(normal,1,context);
	BindTexturePS(material,2,context);
	BindSamplerPS(pointSampler,0,context);
	BindSamplerPS(comparisonSampler,1,context);

	
	BindBlendState(blendStateAdd,context);
	BindDepthStencilState(stencilReadState,stencilRef,context);
	BindRasterizerState(backFaceRS,context);

	BindVertexBuffer(nullptr, 0, 0, context);

	for(int type=0;type<3;++type){

			
		BindVS(lightVS[type],context);
		BindPS(lightPS[type],context);
		BindConstantBufferPS(lightCb[type],1,context);
		BindConstantBufferVS(lightCb[type],1,context);


		for(Cullable* c : culledObjects){
			Light* l = (Light*)c;
			if (l->type != type)
				continue;
			
			if(type==0) //dir
			{
				static thread_local dLightBuffer* lcb = new dLightBuffer;
				(*lcb).direction=XMVector3Normalize(
					-XMVector3Transform( XMVectorSet(0,-1,0,1), XMMatrixRotationQuaternion( XMLoadFloat4(&l->rotation) ) )
					);
				(*lcb).col=XMFLOAT4(l->color.x*l->enerDis.x,l->color.y*l->enerDis.x,l->color.z*l->enerDis.x,1);
				(*lcb).mBiasResSoftshadow=XMFLOAT4(l->shadowBias,(float)SHADOWMAPRES,(float)SOFTSHADOW,0);
				for (unsigned int shmap = 0; shmap < l->shadowMaps_dirLight.size(); ++shmap){
					(*lcb).mShM[shmap]=l->shadowCam[shmap].getVP();
					BindTexturePS(l->shadowMaps_dirLight[shmap].depth->shaderResource,4+shmap,context);
				}
				UpdateBuffer(lightCb[type],lcb,context);

				Draw(6, context);
			}
			else if(type==1) //point
			{
				static thread_local pLightBuffer* lcb = new pLightBuffer;
				(*lcb).pos=l->translation;
				(*lcb).col=l->color;
				(*lcb).enerdis=l->enerDis;
				(*lcb).enerdis.w = 0.f;

				if (l->shadow && l->shadowMap_index>=0)
				{
					(*lcb).enerdis.w = 1.f;
					BindTexturePS(Light::shadowMaps_pointLight[l->shadowMap_index].depth->shaderResource, 7, context);
				}
				UpdateBuffer(lightCb[type], lcb, context);

				Draw(240, context);
			}
			else if(type==2) //spot
			{
				static thread_local sLightBuffer* lcb = new sLightBuffer;
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
				UpdateBuffer(lightCb[type], lcb, context);

				Draw(192, context);
			}
		}


	}

	}
}
void wiRenderer::DrawVolumeLights(Camera* camera, DeviceContext context)
{
	
	static thread_local Frustum frustum = Frustum();
	frustum.ConstructFrustum(min(camera->zFarP, worldInfo.fogSEH.y), camera->Projection, camera->View);

		
		CulledList culledObjects;
		if(spTree_lights)
			wiSPTree::getVisible(spTree_lights->root,frustum,culledObjects);

		if(!culledObjects.empty())
		{

		BindPrimitiveTopology(TRIANGLELIST,context);
		BindVertexLayout(nullptr);
		BindConstantBufferVS(staticCb,0,context);
		BindBlendState(blendStateAdd,context);
		BindDepthStencilState(depthReadStencilState,STENCILREF_DEFAULT,context);
		BindRasterizerState(nonCullRS,context);

		
		BindPS(vLightPS,context);
		BindConstantBufferVS(vLightCb,1,context);


		for(int type=1;type<3;++type){

			
			if(type<=1){
				BindVS(vPointLightVS,context);
			}
			else{
				BindVS(vSpotLightVS,context);
			}

			for(Cullable* c : culledObjects){
				Light* l = (Light*)c;
				if(l->type==type && l->noHalo==false){

					static thread_local vLightBuffer* lcb = new vLightBuffer;
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

					UpdateBuffer(vLightCb,lcb,context);

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
			BindVertexLayout(vertexLayout, context);


			BindDepthStencilState(depthStencilState, STENCILREF_DEFAULT, context);

			BindBlendState(blendState, context);

			BindPS(shPS, context);
			BindSamplerPS(texSampler, 0, context);

			BindVS(shVS, context);
			BindConstantBufferVS(shCb, 0, context);
			BindConstantBufferVS(matCb, 1, context);
			BindConstantBufferPS(matCb, 1, context);


			set<Light*, Cullable> orderedSpotLights;
			set<Light*, Cullable> orderedPointLights;
			set<Light*, Cullable> dirLights;
			for (Cullable* c : culledLights) {
				Light* l = (Light*)c;
				if (l->shadow)
				{
					l->shadowMap_index = -1;
					l->lastSquaredDistMulThousand = (long)(wiMath::DistanceEstimated(l->translation, cam->translation) * 1000);
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
											BindRasterizerState(rssh, context);
										else
											BindRasterizerState(nonCullRSsh, context);

										//D3D11_MAPPED_SUBRESOURCE mappedResource;
										ForShadowMapCB cb;
										cb.mViewProjection = l->shadowCam[index].getVP();
										cb.mWind = wind.direction;
										cb.time = wind.time;
										cb.windRandomness = wind.randomness;
										cb.windWaveSize = wind.waveSize;
										UpdateBuffer(shCb, &cb, context);


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

												UpdateBuffer(matCb, mcb, context);


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
										BindRasterizerState(rssh, context);
									else
										BindRasterizerState(nonCullRSsh, context);

									//D3D11_MAPPED_SUBRESOURCE mappedResource;
									ForShadowMapCB cb;
									cb.mViewProjection = l->shadowCam[index].getVP();
									cb.mWind = wind.direction;
									cb.time = wind.time;
									cb.windRandomness = wind.randomness;
									cb.windWaveSize = wind.waveSize;
									UpdateBuffer(shCb, &cb, context);


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

											UpdateBuffer(matCb, mcb, context);


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
				BindPS(cubeShPS, context);
				BindVS(cubeShVS, context);
				BindGS(cubeShGS, context);
				BindConstantBufferGS(cubeShCb, 0, context);
				BindConstantBufferPS(lightCb[1], 2, context);

				int i = 0;
				for (Light* l : orderedPointLights)
				{
					if (i >= POINTLIGHTSHADOW)
						break; 
					
					l->shadowMap_index = i;

					Light::shadowMaps_pointLight[i].Set(context);

					//D3D11_MAPPED_SUBRESOURCE mappedResource;
					pLightBuffer lcb;
					lcb.enerdis = l->enerDis;
					lcb.pos = l->translation;
					UpdateBuffer(lightCb[1], &lcb, context);

					CubeShadowCb cb;
					for (unsigned int shcam = 0; shcam < l->shadowCam.size(); ++shcam)
						cb.mViewProjection[shcam] = l->shadowCam[shcam].getVP();

					UpdateBuffer(cubeShCb, &cb, context);

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
								BindRasterizerState(rssh, context);
							else
								BindRasterizerState(nonCullRSsh, context);



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

									UpdateBuffer(matCb, mcb, context);

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

	//		int dirlightShadows_processed = 0;
	//		int spotlightShadows_processed = 0;

	//		//DIRECTIONAL AND SPOTLIGHT 2DSHADOWMAPS
	//		for (Cullable* c : culledLights){
	//			Light* l = (Light*)c;
	//			if (l->type != Light::POINT){

	//				int mapCount = l->type == (Light::DIRECTIONAL ? 3 : 1); // directional: 3, spot: 1 shadow maps

	//				for (int index = 0; index < mapCount; ++index)
	//				{
	//					//l->shadowMap[index].Set(context);

	//					CulledCollection culledRenderer;
	//					CulledList culledObjects;

	//					if (l->type == Light::DIRECTIONAL){
	//						Light::shadowMaps_dirLight[index + dirlightShadows_processed].Set(context);
	//						const float siz = l->shadowCam[index].size * 0.5f;
	//						const float f = l->shadowCam[index].farplane;
	//						AABB boundingbox;
	//						boundingbox.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(siz, f, siz));
	//						if (spTree)
	//							wiSPTree::getVisible(spTree->root, boundingbox.get(
	//							XMMatrixInverse(0, XMLoadFloat4x4(&l->shadowCam[index].View))
	//							), culledObjects);
	//					}
	//					else if (l->type == Light::SPOT){
	//						Light::shadowMaps_spotLight[0].Set(context);
	//						Frustum frustum = Frustum();
	//						XMFLOAT4X4 proj, view;
	//						XMStoreFloat4x4(&proj, XMLoadFloat4x4(&l->shadowCam[index].Projection));
	//						XMStoreFloat4x4(&view, XMLoadFloat4x4(&l->shadowCam[index].View));
	//						frustum.ConstructFrustum(wiRenderer::getCamera()->zFarP, proj, view);
	//						if (spTree)
	//							wiSPTree::getVisible(spTree->root, frustum, culledObjects);
	//					}

	//					if (!culledObjects.empty()){

	//						for (Cullable* object : culledObjects){
	//							culledRenderer[((Object*)object)->mesh].insert((Object*)object);
	//						}

	//						for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
	//							Mesh* mesh = iter->first;
	//							CulledObjectList& visibleInstances = iter->second;

	//							if (visibleInstances.size() && !mesh->isBillboarded){

	//								if (!mesh->doubleSided)
	//									BindRasterizerState(rssh, context);
	//								else
	//									BindRasterizerState(nonCullRSsh, context);

	//								//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//								ForShadowMapCB cb;
	//								cb.mViewProjection = l->shadowCam[index].getVP();
	//								cb.mWind = wind.direction;
	//								cb.time = wind.time;
	//								cb.windRandomness = wind.randomness;
	//								cb.windWaveSize = wind.waveSize;
	//								UpdateBuffer(shCb, &cb, context);


	//								int k = 0;
	//								for (CulledObjectList::iterator viter = visibleInstances.begin(); viter != visibleInstances.end(); ++viter){
	//									if ((*viter)->particleEmitter != Object::wiParticleEmitter::EMITTER_INVISIBLE){
	//										if (mesh->softBody || (*viter)->armatureDeform)
	//											Mesh::AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k, GRAPHICSTHREAD_SHADOWS);
	//										else
	//											Mesh::AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k, GRAPHICSTHREAD_SHADOWS);
	//										++k;
	//									}
	//								}
	//								if (k<1)
	//									continue;

	//								Mesh::UpdateRenderableInstances(visibleInstances.size(), GRAPHICSTHREAD_SHADOWS, context);


	//								BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), context);
	//								BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), context);
	//								BindIndexBuffer(mesh->meshIndexBuff, context);


	//								int matsiz = mesh->materialIndices.size();
	//								int m = 0;
	//								for (Material* iMat : mesh->materials){

	//									if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
	//										BindTexturePS(iMat->texture, 0, context);



	//										MaterialCB* mcb = new MaterialCB(*iMat, m);

	//										UpdateBuffer(matCb, mcb, context);

	//										delete mcb;

	//										DrawIndexedInstanced(mesh->indices.size(), visibleInstances.size(), context);

	//										m++;
	//									}
	//								}
	//							}
	//						}

	//					}
	//				}
	//				if (l->type == Light::DIRECTIONAL)
	//					dirlightShadows_processed++;
	//				else if (l->type == Light::SPOT)
	//					spotlightShadows_processed++;

	//			}
	//		}

	//		if (!orderedPointLights.empty() && POINTLIGHTSHADOW){


	//			//set<Light*, Cullable> orderedLights;
	//			//for (Light* l : pointLightsSaved){
	//			//	l->lastSquaredDistMulThousand = (long)(wiMath::DistanceEstimated(l->translation, cam->translation) * 1000);
	//			//	if(l->shadow)
	//			//		orderedLights.insert(l);
	//			//}

	//			int cube_shadowrenders_remain = POINTLIGHTSHADOW;

	//			BindPS(cubeShPS, context);
	//			BindVS(cubeShVS, context);
	//			BindGS(cubeShGS, context);
	//			BindConstantBufferGS(cubeShCb, 0, context);
	//			BindConstantBufferPS(lightCb[1], 2, context);
	//			for (Light* l : orderedPointLights){

	//				for (unsigned int index = 0; index<Light::shadowMaps_pointLight.size(); ++index){
	//					if (cube_shadowrenders_remain <= 0)
	//						break;
	//					cube_shadowrenders_remain -= 1;
	//					Light::shadowMaps_pointLight[index].Set(context);

	//					Frustum frustum = Frustum();
	//					XMFLOAT4X4 proj, view;
	//					XMStoreFloat4x4(&proj, XMLoadFloat4x4(&l->shadowCam[index].Projection));
	//					XMStoreFloat4x4(&view, XMLoadFloat4x4(&l->shadowCam[index].View));
	//					frustum.ConstructFrustum(wiRenderer::getCamera()->zFarP, proj, view);

	//					//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//					pLightBuffer lcb;
	//					lcb.enerdis = l->enerDis;
	//					lcb.pos = l->translation;
	//					UpdateBuffer(lightCb[1], &lcb, context);

	//					CubeShadowCb cb;
	//					for (unsigned int shcam = 0; shcam<l->shadowCam.size(); ++shcam)
	//						cb.mViewProjection[shcam] = l->shadowCam[shcam].getVP();

	//					UpdateBuffer(cubeShCb, &cb, context);

	//					CulledCollection culledRenderer;
	//					CulledList culledObjects;

	//					if (spTree)
	//						wiSPTree::getVisible(spTree->root, l->bounds, culledObjects);

	//					for (Cullable* object : culledObjects)
	//						culledRenderer[((Object*)object)->mesh].insert((Object*)object);

	//					for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
	//						Mesh* mesh = iter->first;
	//						CulledObjectList& visibleInstances = iter->second;

	//						if (!mesh->isBillboarded && !visibleInstances.empty()){

	//							if (!mesh->doubleSided)
	//								BindRasterizerState(rssh, context);
	//							else
	//								BindRasterizerState(nonCullRSsh, context);



	//							int k = 0;
	//							for (CulledObjectList::iterator viter = visibleInstances.begin(); viter != visibleInstances.end(); ++viter){
	//								if ((*viter)->particleEmitter != Object::wiParticleEmitter::EMITTER_INVISIBLE){
	//									if (mesh->softBody || (*viter)->armatureDeform)
	//										Mesh::AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k, GRAPHICSTHREAD_SHADOWS);
	//									else
	//										Mesh::AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k, GRAPHICSTHREAD_SHADOWS);
	//									++k;
	//								}
	//							}
	//							if (k<1)
	//								continue;

	//							Mesh::UpdateRenderableInstances(visibleInstances.size(), GRAPHICSTHREAD_SHADOWS, context);


	//							BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), context);
	//							BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), context);
	//							BindIndexBuffer(mesh->meshIndexBuff, context);


	//							int matsiz = mesh->materialIndices.size();
	//							int m = 0;
	//							for (Material* iMat : mesh->materials){
	//								if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
	//									BindTexturePS(iMat->texture, 0, context);

	//									MaterialCB* mcb = new MaterialCB(*iMat, m);

	//									UpdateBuffer(matCb, mcb, context);
	//									delete mcb;

	//									DrawIndexedInstanced(mesh->indices.size(), visibleInstances.size(), context);
	//								}
	//								m++;
	//							}
	//						}
	//						visibleInstances.clear();
	//					}
	//				}
	//			}
	//		}
	//	}

	//	BindGS(nullptr, context);
	//}
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
				  , bool BlackOut, SHADERTYPE shaded
				  , TextureView refRes, bool grass, GRAPHICSTHREAD thread)
{
	CulledCollection culledRenderer;
	CulledList culledObjects;
	if(spTree)
		wiSPTree::getVisible(spTree->root, camera->frustum,culledObjects);

	if(!culledObjects.empty())
	{	

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
		BindVertexLayout(vertexLayout,context);

		if(DX11Eff && tessF)
			BindVS(vertexShader,context);
		else
			BindVS(vertexShader10,context);
		if(DX11Eff && tessF)
			BindHS(hullShader,context);
		else
			BindHS(nullptr,context);
		if(DX11Eff && tessF) 
			BindDS(domainShader,context);
		else		
			BindDS(nullptr,context);

		if (wireRender)
			BindPS(simplestPS, context);
		else
			if (BlackOut)
				BindPS(blackoutPS, context);
			else if (shaded == SHADERTYPE_NONE)
				BindPS(textureonlyPS, context);
			else if (shaded == SHADERTYPE_DEFERRED)
				BindPS(pixelShader, context);
			else if (shaded == SHADERTYPE_FORWARD_SIMPLE)
				BindPS(fowardSimplePS, context);
			else
				return;

		if(DX11Eff && tessF) 
			BindSamplerDS(texSampler,0,context);
		BindSamplerPS(texSampler,0,context);
		BindSamplerPS(mapSampler,1,context);


		BindConstantBufferVS(staticCb,0,context);
		if(DX11Eff && tessF){
			BindConstantBufferDS(staticCb,0,context);
			BindConstantBufferDS(constantBuffer,3,context);
			BindConstantBufferHS(tessBuf,0,context);
		}
		BindConstantBufferPS(pixelCB,0,context);
		BindConstantBufferPS(fxCb,1,context);
		BindConstantBufferVS(constantBuffer,3,context);
		BindConstantBufferVS(matCb,2,context);
		BindConstantBufferPS(matCb,2,context);

		BindBlendState(blendState,context);

		if(!wireRender) {
			BindTexturePS(enviroMap,0,context);
			if(refRes != nullptr) 
				BindTexturePS(refRes,1,context);
		}


		for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
			Mesh* mesh = iter->first;
			CulledObjectList& visibleInstances = iter->second;


			if(!mesh->doubleSided)
				BindRasterizerState(wireRender?wireRS:rasterizerState,context);
			else
				BindRasterizerState(wireRender?wireRS:nonCullRS,context);

			int matsiz = mesh->materialIndices.size();
			
			if(DX11 && tessF){
				static thread_local ConstantBuffer* cb = new ConstantBuffer;
				if(matsiz == 1) (*cb).mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,0,0,0);
				else if(matsiz == 2) (*cb).mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,mesh->materials[1]->hasDisplacementMap,0,0);
				else if(matsiz == 3) (*cb).mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,mesh->materials[1]->hasDisplacementMap,mesh->materials[2]->hasDisplacementMap,0);
				else if(matsiz == 4) (*cb).mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,mesh->materials[1]->hasDisplacementMap,mesh->materials[2]->hasDisplacementMap,mesh->materials[3]->hasDisplacementMap);

				UpdateBuffer(constantBuffer,cb,context);
			}

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
						BindDepthStencilState(depthStencilState,STENCILREF_SHADELESS,context);
					if(iMat->subsurface_scattering)
						BindDepthStencilState(depthStencilState,STENCILREF_SKIN,context);
					else
						BindDepthStencilState(depthStencilState,mesh->stencilRef,context);


					static thread_local MaterialCB* mcb = new MaterialCB;
					(*mcb).Create(*iMat, m);

					UpdateBuffer(matCb, mcb, context);
					

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
		wiSPTree::getVisible(spTree->root, camera->frustum,culledObjects);

	if(!culledObjects.empty())
	{
		for(Cullable* object : culledObjects)
			culledRenderer[((Object*)object)->mesh].insert((Object*)object);

		BindPrimitiveTopology(TRIANGLELIST,context);
		BindVertexLayout(vertexLayout,context);
		BindVS(vertexShader10,context);

		BindSamplerPS(texSampler,0,context);
		BindSamplerPS(mapSampler,1,context);
	
		if(!wireRender) BindTexturePS(enviroMap,0,context);
		if(!wireRender) BindTexturePS(refRes,1,context);
		if(!wireRender) BindTexturePS(refracRes,2,context);
		if(!wireRender) BindTexturePS(depth,7,context);
		if(!wireRender) BindTexturePS(waterRippleNormals, 8, context);


		BindConstantBufferVS(staticCb,0,context);
		BindConstantBufferPS(pixelCB,0,context);
		BindConstantBufferPS(fxCb,1,context);
		BindConstantBufferVS(constantBuffer,3,context);
		BindConstantBufferVS(matCb,2,context);
		BindConstantBufferPS(matCb,2,context);

		BindBlendState(blendState,context);
	
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
				BindRasterizerState(wireRender ? wireRS : rasterizerState, context);
			else
				BindRasterizerState(wireRender ? wireRS : nonCullRS, context);


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

					UpdateBuffer(matCb, mcb, context);

					if(!wireRender) BindTexturePS(iMat->texture,3,context);
					if(!wireRender) BindTexturePS(iMat->refMap,4,context);
					if(!wireRender) BindTexturePS(iMat->normalMap,5,context);
					if(!wireRender) BindTexturePS(iMat->specularMap,6,context);

					if (iMat->IsTransparent() && lastRenderType != RENDERTYPE_TRANSPARENT)
					{
						lastRenderType = RENDERTYPE_TRANSPARENT;

						BindDepthStencilState(depthStencilState, STENCILREF_TRANSPARENT, context);
						BindPS(wireRender ? simplestPS : transparentPS, context);
					}
					if (iMat->IsWater() && lastRenderType != RENDERTYPE_WATER)
					{
						lastRenderType = RENDERTYPE_WATER;

						BindDepthStencilState(depthReadStencilState, STENCILREF_EMPTY, context);
						BindPS(wireRender ? simplestPS : waterPS, context); 
						BindRasterizerState(wireRender ? wireRS : nonCullRS, context);
					}
					
					DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),context);
				}
				m++;
			}
		}
	}
	
}


void wiRenderer::DrawSky(DeviceContext context)
{
	if (enviroMap == nullptr)
		return;

	BindPrimitiveTopology(TRIANGLELIST,context);
	BindRasterizerState(backFaceRS,context);
	BindDepthStencilState(depthReadStencilState,STENCILREF_SKY,context);
	BindBlendState(blendState,context);
	
	BindVS(skyVS,context);
	BindPS(skyPS,context);
	
	BindTexturePS(enviroMap,0,context);
	BindSamplerPS(skySampler,0,context);

	BindConstantBufferVS(skyCb,3,context);
	BindConstantBufferPS(pixelCB,0,context);
	BindConstantBufferPS(fxCb,1,context);

	BindVertexBuffer(nullptr,0,0,context);
	Draw(240,context);
}

void wiRenderer::DrawDecals(Camera* camera, DeviceContext context, TextureView depth)
{
	for (Model* model : models)
	{
		if (model->decals.empty())
			continue;

		BindTexturePS(depth, 1, context);
		BindSamplerPS(ssClampAni, 0, context);
		BindVS(decalVS, context);
		BindPS(decalPS, context);
		BindRasterizerState(backFaceRS, context);
		BindBlendState(blendStateTransparent, context);
		BindDepthStencilState(stencilReadMatch, STENCILREF::STENCILREF_DEFAULT, context);
		BindVertexLayout(nullptr, context);
		BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST, context);
		BindConstantBufferVS(decalCbVS, 0, context);
		BindConstantBufferPS(lightStaticCb, 0, context);
		BindConstantBufferPS(decalCbPS, 1, context);

		for (Decal* decal : model->decals) {

			if ((decal->texture || decal->normal) && camera->frustum.CheckBox(decal->bounds.corners)) {

				BindTexturePS(decal->texture, 2, context);
				BindTexturePS(decal->normal, 3, context);

				static thread_local DecalCBVS* dcbvs = new DecalCBVS;
				dcbvs->mWVP =
					XMMatrixTranspose(
						XMLoadFloat4x4(&decal->world)
						*camera->GetViewProjection()
						);
				UpdateBuffer(decalCbVS, dcbvs, context);

				static thread_local DecalCBPS* dcbps = new DecalCBPS;
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
				UpdateBuffer(decalCbPS, dcbps, context);

				Draw(36, context);

			}

		}
	}
}


void wiRenderer::UpdatePerWorldCB(DeviceContext context){
	static thread_local PixelCB* pcb = new PixelCB;
	(*pcb).mSun=XMVector3Normalize( GetSunPosition() );
	(*pcb).mHorizon=worldInfo.horizon;
	(*pcb).mAmbient=worldInfo.ambient;
	(*pcb).mSunColor=GetSunColor();
	(*pcb).mFogSEH=worldInfo.fogSEH;
	UpdateBuffer(pixelCB, pcb, context);
}
void wiRenderer::UpdatePerFrameCB(DeviceContext context){
	static thread_local ViewPropCB* cb = new ViewPropCB;
	(*cb).mZFarP=cam->zFarP;
	(*cb).mZNearP=cam->zNearP;
	(*cb).matView = XMMatrixTranspose( cam->GetView() );
	(*cb).matProj = XMMatrixTranspose( cam->GetProjection() );
	UpdateBuffer(viewPropCB,cb,context);

	BindConstantBufferPS(viewPropCB,10,context);
}
void wiRenderer::UpdatePerRenderCB(DeviceContext context, int tessF){
	if(tessF){
		static thread_local TessBuffer* tb = new TessBuffer;
		(*tb).g_f4Eye = cam->GetEye();
		(*tb).g_f4TessFactors = XMFLOAT4A( (float)tessF,2.f,4.f,6.f );
		UpdateBuffer(tessBuf,tb,context);
	}

}
void wiRenderer::UpdatePerViewCB(DeviceContext context, Camera* camera, Camera* refCamera, const XMFLOAT4& newClipPlane){

	
	static thread_local StaticCB* cb = new StaticCB;
	(*cb).mViewProjection = XMMatrixTranspose(camera->GetViewProjection());
	(*cb).mRefViewProjection = XMMatrixTranspose( refCamera->GetViewProjection());
	(*cb).mPrevViewProjection = XMMatrixTranspose(prevFrameCam->GetViewProjection());
	(*cb).mCamPos = camera->GetEye();
	(*cb).mClipPlane = newClipPlane;
	(*cb).mWind=wind.direction;
	(*cb).time=wind.time;
	(*cb).windRandomness=wind.randomness;
	(*cb).windWaveSize=wind.waveSize;
	UpdateBuffer(staticCb,cb,context);

	static thread_local SkyBuffer* scb = new SkyBuffer;
	(*scb).mV=XMMatrixTranspose(camera->GetView());
	(*scb).mP = XMMatrixTranspose(camera->GetProjection());
	(*scb).mPrevView = XMMatrixTranspose(prevFrameCam->GetView());
	(*scb).mPrevProjection = XMMatrixTranspose(prevFrameCam->GetProjection());
	UpdateBuffer(skyCb,scb,context);

	UpdateBuffer(trailCB, &XMMatrixTranspose(camera->GetViewProjection()), context);

	static thread_local LightStaticCB* lcb = new LightStaticCB;
	(*lcb).mProjInv = XMMatrixInverse(0, XMMatrixTranspose(camera->GetViewProjection()));
	UpdateBuffer(lightStaticCb,lcb,context);
}
void wiRenderer::UpdatePerEffectCB(DeviceContext context, const XMFLOAT4& blackoutBlackWhiteInvCol, const XMFLOAT4 colorMask){
	static thread_local FxCB* fb = new FxCB;
	(*fb).mFx = blackoutBlackWhiteInvCol;
	(*fb).colorMask=colorMask;
	UpdateBuffer(fxCb,fb,context);
}

void wiRenderer::FinishLoading()
{

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
	LoadWorldInfo(dir, name);

	model->transform(transform);

	if (world == nullptr)
	{
		world = new Model;
		world->name = "[WickedEngine-Default]{World}";
		models.push_back(world);
	}
	model->attachTo(world);

	models.push_back(model);

	for (Object* o : model->objects)
	{
		if (physicsEngine != nullptr) {
			physicsEngine->registerObject(o);
		}
	}
	
	graphicsMutex.lock();

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

	graphicsMutex.unlock();



	Update();
	UpdateRenderData(nullptr);
	SetUpCubes();
	SetUpBoneLines();

	return model;
}
void wiRenderer::LoadWorldInfo(const string& dir, const string& name)
{
	LoadWiWorldInfo(dir, name+".wiw", worldInfo, wind);
	if(physicsEngine)
		physicsEngine->addWind(wind.direction);
}

void wiRenderer::SychronizeWithPhysicsEngine()
{
	if (physicsEngine && GetGameSpeed()){

		//UpdateSoftBodyPinning();
		for (Model* model : models)
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
						//XMVECTOR s, r, t;
						//XMMatrixDecompose(&s, &r, &t, XMLoadFloat4x4(&object->world));
						//XMFLOAT3 T;
						//XMFLOAT4 R;
						//XMStoreFloat4(&R, r);
						//XMStoreFloat3(&T, t);
						//physicsEngine->transformBody(R, T, pI);
						physicsEngine->transformBody(object->rotation, object->translation, pI);
					}
				}
			}
		}

		physicsEngine->Update();

		for (Model* model : models)
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

void wiRenderer::MaterialCB::Create(const Material& mat,UINT materialIndex){
	difColor = XMFLOAT4(mat.diffuseColor.x, mat.diffuseColor.y, mat.diffuseColor.z, mat.alpha);
	hasRef = mat.refMap != nullptr;
	hasNor = mat.normalMap != nullptr;
	hasTex = mat.texture != nullptr;
	hasSpe = mat.specularMap != nullptr;
	specular = mat.specular;
	refractionIndex = mat.refraction_index;
	movingTex = mat.texOffset;
	metallic = mat.enviroReflection;
	shadeless = mat.shadeless;
	specular_power = mat.specular_power;
	toon = mat.toonshading;
	matIndex = materialIndex;
	emit = mat.emit;
}