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
#include "wiGraphicsThreads.h"
#include "wiRandom.h"

#pragma region STATICS
D3D_DRIVER_TYPE						wiRenderer::driverType;
D3D_FEATURE_LEVEL					wiRenderer::featureLevel;
wiRenderer::SwapChain					wiRenderer::swapChain;
wiRenderer::RenderTargetView			wiRenderer::renderTargetView;
wiRenderer::ViewPort					wiRenderer::viewPort;
wiRenderer::GraphicsDevice			wiRenderer::graphicsDevice;
wiRenderer::DeviceContext				wiRenderer::immediateContext;
bool wiRenderer::DX11 = false,wiRenderer::VSYNC=true,wiRenderer::DEFERREDCONTEXT_SUPPORT=false;
wiRenderer::DeviceContext				wiRenderer::deferredContexts[];
wiRenderer::CommandList				wiRenderer::commandLists[];
mutex								wiRenderer::graphicsMutex;
wiRenderer::Sampler wiRenderer::ssClampLin,wiRenderer::ssClampPoi,wiRenderer::ssMirrorLin,wiRenderer::ssMirrorPoi,wiRenderer::ssWrapLin,wiRenderer::ssWrapPoi
		,wiRenderer::ssClampAni,wiRenderer::ssWrapAni,wiRenderer::ssMirrorAni,wiRenderer::ssComp;

map<wiRenderer::DeviceContext,long> wiRenderer::drawCalls;

int wiRenderer::RENDERWIDTH=1280,wiRenderer::RENDERHEIGHT=720,wiRenderer::SCREENWIDTH=1280,wiRenderer::SCREENHEIGHT=720,wiRenderer::SHADOWMAPRES=1024,wiRenderer::SOFTSHADOW=2
	,wiRenderer::POINTLIGHTSHADOW=2,wiRenderer::POINTLIGHTSHADOWRES=256, wiRenderer::SPOTLIGHTSHADOW=2, wiRenderer::SPOTLIGHTSHADOWRES=512;
bool wiRenderer::HAIRPARTICLEENABLED=true,wiRenderer::EMITTERSENABLED=true;
bool wiRenderer::wireRender = false, wiRenderer::debugSpheres = false, wiRenderer::debugBoneLines = false, wiRenderer::debugBoxes = false;
ID3D11BlendState		*wiRenderer::blendState, *wiRenderer::blendStateTransparent, *wiRenderer::blendStateAdd;
ID3D11Buffer			*wiRenderer::constantBuffer, *wiRenderer::staticCb, *wiRenderer::shCb, *wiRenderer::pixelCB, *wiRenderer::matCb
	, *wiRenderer::lightCb[3], *wiRenderer::tessBuf, *wiRenderer::lineBuffer, *wiRenderer::skyCb
	, *wiRenderer::trailCB, *wiRenderer::lightStaticCb, *wiRenderer::vLightCb,*wiRenderer::cubeShCb,*wiRenderer::fxCb
	, *wiRenderer::decalCbVS, *wiRenderer::decalCbPS, *wiRenderer::viewPropCB;
ID3D11VertexShader		*wiRenderer::vertexShader10, *wiRenderer::vertexShader, *wiRenderer::shVS, *wiRenderer::lineVS, *wiRenderer::trailVS
	,*wiRenderer::waterVS,*wiRenderer::lightVS[3],*wiRenderer::vSpotLightVS,*wiRenderer::vPointLightVS,*wiRenderer::cubeShVS,*wiRenderer::sOVS,*wiRenderer::decalVS;
ID3D11PixelShader		*wiRenderer::pixelShader, *wiRenderer::shPS, *wiRenderer::linePS, *wiRenderer::trailPS, *wiRenderer::simplestPS,*wiRenderer::blackoutPS
	,*wiRenderer::textureonlyPS,*wiRenderer::waterPS,*wiRenderer::transparentPS,*wiRenderer::lightPS[3],*wiRenderer::vLightPS,*wiRenderer::cubeShPS
	,*wiRenderer::decalPS,*wiRenderer::fowardSimplePS;
ID3D11GeometryShader *wiRenderer::cubeShGS,*wiRenderer::sOGS;
ID3D11HullShader		*wiRenderer::hullShader;
ID3D11DomainShader		*wiRenderer::domainShader;
ID3D11InputLayout		*wiRenderer::vertexLayout, *wiRenderer::lineIL,*wiRenderer::trailIL, *wiRenderer::sOIL;
ID3D11SamplerState		*wiRenderer::texSampler, *wiRenderer::mapSampler, *wiRenderer::comparisonSampler, *wiRenderer::mirSampler,*wiRenderer::pointSampler;
ID3D11RasterizerState	*wiRenderer::rasterizerState, *wiRenderer::rssh, *wiRenderer::nonCullRSsh, *wiRenderer::wireRS, *wiRenderer::nonCullRS,*wiRenderer::nonCullWireRS
	,*wiRenderer::backFaceRS;
ID3D11DepthStencilState	*wiRenderer::depthStencilState,*wiRenderer::xRayStencilState,*wiRenderer::depthReadStencilState,*wiRenderer::stencilReadState
	,*wiRenderer::stencilReadMatch;
ID3D11PixelShader		*wiRenderer::skyPS;
ID3D11VertexShader		*wiRenderer::skyVS;
ID3D11SamplerState		*wiRenderer::skySampler;
wiRenderer::TextureView wiRenderer::noiseTex,wiRenderer::trailDistortTex,wiRenderer::enviroMap,wiRenderer::colorGrading;
float wiRenderer::GameSpeed=1,wiRenderer::overrideGameSpeed=1;
int wiRenderer::visibleCount;
wiRenderTarget wiRenderer::normalMapRT, wiRenderer::imagesRT, wiRenderer::imagesRTAdd;
Camera *wiRenderer::cam = nullptr, *wiRenderer::refCam = nullptr, *wiRenderer::prevFrameCam = nullptr;
PHYSICS* wiRenderer::physicsEngine = nullptr;
Wind wiRenderer::wind;
WorldInfo wiRenderer::worldInfo;

vector<wiRenderer::TextureView> wiRenderer::textureSlotsPS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
wiRenderer::PixelShader wiRenderer::boundPS=nullptr;
#pragma endregion

#pragma region STATIC TEMP

int wiRenderer::vertexCount;
deque<wiSprite*> wiRenderer::images;
deque<wiSprite*> wiRenderer::waterRipples;

wiSPTree* wiRenderer::spTree;
wiSPTree* wiRenderer::spTree_trans;
wiSPTree* wiRenderer::spTree_water;
wiSPTree* wiRenderer::spTree_lights;

vector<Object*>		wiRenderer::everyObject;
vector<Object*>		wiRenderer::objects;
vector<Object*>		wiRenderer::objects_trans;
vector<Object*>		wiRenderer::objects_water;	
MeshCollection		wiRenderer::meshes;
MaterialCollection	wiRenderer::materials;
vector<Armature*>	wiRenderer::armatures;
vector<Lines*>		wiRenderer::boneLines;
vector<Lines*>		wiRenderer::linesTemp;
vector<Cube>		wiRenderer::cubes;
vector<Light*>		wiRenderer::lights;
map<string,vector<wiEmittedParticle*>> wiRenderer::emitterSystems;
map<string,Transform*> wiRenderer::transforms;
list<Decal*> wiRenderer::decals;

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


	hr = pIDXGIFactory->CreateSwapChainForCoreWindow(graphicsDevice, reinterpret_cast<IUnknown*>(window), &sd
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
    ID3D11Texture2D* pBackBuffer = NULL;
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

	//cam = new Camera(SCREENWIDTH, SCREENHEIGHT, 0.1f, 800, XMVectorSet(0, 4, -4, 1));
	cam = new Camera();
	cam->SetUp(SCREENWIDTH, SCREENHEIGHT, 0.1f, 800);
	refCam = new Camera();
	refCam->SetUp(SCREENWIDTH, SCREENHEIGHT, 0.1f, 800);
	prevFrameCam = new Camera;
	
	noiseTex = wiTextureHelper::getInstance()->getRandom64x64();
	trailDistortTex = wiTextureHelper::getInstance()->getNormalMapDefault();

	wireRender=false;
	debugSpheres=false;
	
	
	wiRenderer::LoadBasicShaders();
	wiRenderer::LoadShadowShaders();
	wiRenderer::LoadSkyShaders();
	wiRenderer::LoadLineShaders();
	wiRenderer::LoadTrailShaders();
	wiRenderer::LoadWaterShaders();
	wiRenderer::LoadTessShaders();

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
	if(noiseTex){
		noiseTex->Release();
		noiseTex=NULL;
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

	
	for(Light* l : lights)
		l->CleanUp();
	lights.clear();

	for(Armature* armature : armatures)
		armature->CleanUp();
	armatures.clear();

	for(MeshCollection::iterator iter=meshes.begin(); iter!=meshes.end(); ++iter){
		iter->second->CleanUp();
	}
	meshes.clear();
	
	for(unsigned int i=0;i<objects.size();i++){
		objects[i]->CleanUp();
		for(unsigned int j=0;j<objects[i]->eParticleSystems.size();++j)
			objects[i]->eParticleSystems[j]->CleanUp();
		objects[i]->eParticleSystems.clear();

		for(unsigned int j=0;j<objects[i]->hwiParticleSystems.size();++j)
			objects[i]->hwiParticleSystems[j]->CleanUp();
		objects[i]->hwiParticleSystems.clear();
	}
	objects.clear();
	
	for(unsigned int i=0;i<objects_trans.size();i++){
		objects_trans[i]->CleanUp();
		for (unsigned int j = 0; j<objects_trans[i]->eParticleSystems.size(); ++j)
			objects_trans[i]->eParticleSystems[j]->CleanUp();
		objects_trans[i]->eParticleSystems.clear();

		for (unsigned int j = 0; j<objects_trans[i]->hwiParticleSystems.size(); ++j)
			objects_trans[i]->hwiParticleSystems[j]->CleanUp();
		objects_trans[i]->hwiParticleSystems.clear();
	}
	objects_trans.clear();
	
	for (unsigned int i = 0; i<objects_water.size(); i++){
		objects_water[i]->CleanUp();
		for (unsigned int j = 0; j<objects_water[i]->eParticleSystems.size(); ++j)
			objects_water[i]->eParticleSystems[j]->CleanUp();
		objects_water[i]->eParticleSystems.clear();

		for (unsigned int j = 0; j<objects_water[i]->hwiParticleSystems.size(); ++j)
			objects_water[i]->hwiParticleSystems[j]->CleanUp();
		objects_water[i]->hwiParticleSystems.clear();
	}
	objects_water.clear();
	everyObject.clear();

	for(MaterialCollection::iterator iter=materials.begin(); iter!=materials.end();++iter)
		iter->second->CleanUp();
	materials.clear();


	for (unsigned int i = 0; i < boneLines.size(); i++)
		delete boneLines[i];
	boneLines.clear();

	cubes.clear();

	emitterSystems.clear();

	
	if(spTree) 
		spTree->CleanUp();
	spTree = nullptr;
	
	if(spTree_trans) 
		spTree_trans->CleanUp();
	spTree_trans = nullptr;
	
	if(spTree_water) 
		spTree_water->CleanUp();
	spTree_water = nullptr;

	
	if(spTree_lights)
		spTree_lights->CleanUp();
	spTree_lights=nullptr;

	for(Decal* decal:decals){
		decal->CleanUp();
		delete decal;
	}
	decals.clear();

}
XMVECTOR wiRenderer::GetSunPosition()
{
	for(Light* l : lights)
		if(l->type==Light::DIRECTIONAL)
			return -XMVector3Transform( XMVectorSet(0,-1,0,1), XMMatrixRotationQuaternion( XMLoadFloat4(&l->rotation) ) );
	return XMVectorSet(0.5f,1,-0.5f,0);
}
XMFLOAT4 wiRenderer::GetSunColor()
{
	for(Light* l : lights)
		if(l->type==Light::DIRECTIONAL)
			return l->color;
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

	for(HitSphere* s : spheres){
		s->getTransform();
		s->center=s->translation;
		s->radius=s->radius_saved*s->scale.x;
	}
}
float wiRenderer::getSphereRadius(const int& index){
	return spheres[index]->radius;
}
void wiRenderer::SetUpBoneLines()
{
	boneLines.resize(0);
	for (unsigned int i = 0; i<armatures.size(); i++){
		for (unsigned int j = 0; j<armatures[i]->boneCollection.size(); j++){
			boneLines.push_back( new Lines(armatures[i]->boneCollection[j]->length,XMFLOAT4A(1,1,1,1),i,j) );
		}
	}
}
void wiRenderer::UpdateBoneLines()
{
	if (debugBoneLines)
		for (unsigned int i = 0; i<boneLines.size(); i++){
			int armatureI=boneLines[i]->parentArmature;
			int boneI = boneLines[i]->parentBone;

			//XMMATRIX rest = XMLoadFloat4x4(&armatures[armatureI]->boneCollection[boneI]->recursiveRest) ;
			//XMMATRIX pose = XMLoadFloat4x4(&armatures[armatureI]->boneCollection[boneI]->world) ;
			//XMFLOAT4X4 finalM;
			//XMStoreFloat4x4( &finalM,  /*rest**/pose );

			boneLines[i]->Transform(armatures[armatureI]->boneCollection[boneI]->world);
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
			//for(wiHairParticle& hps : o->hwiParticleSystems)
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
		if(spTree_trans) iterateSPTree(spTree_trans->root,cubes,XMFLOAT4A(0,1,1,1));
		if(spTree_water) iterateSPTree(spTree_water->root,cubes,XMFLOAT4A(0,0,1,1));
		if(spTree_lights) iterateSPTree(spTree_lights->root,cubes,XMFLOAT4A(1,1,1,1));
	}
	if(debugBoxes){
		for(Decal* decal : decals){
			cubes.push_back(Cube(decal->bounds.getCenter(),decal->bounds.getHalfWidth(),XMFLOAT4A(1,0,1,1)));
		}
	}
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
		D3D11_INPUT_ELEMENT_DESC layout[] =
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
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetGlobal()->add("shaders/effectVS10.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr){
			vertexShader10 = vsinfo->vertexShader;
			vertexLayout = vsinfo->vertexLayout;
		}
		delete vsinfo;
	}

	{

		D3D11_INPUT_ELEMENT_DESC oslayout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(oslayout);


		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetGlobal()->add("shaders/sOVS.cso", wiResourceManager::VERTEXSHADER, oslayout, numElements));
		if (vsinfo != nullptr){
			sOVS = vsinfo->vertexShader;
			sOIL = vsinfo->vertexLayout;
		}
		delete vsinfo;
	}

	{
		D3D11_SO_DECLARATION_ENTRY pDecl[] =
		{
			// semantic name, semantic index, start component, component count, output slot
			{ 0, "SV_POSITION", 0, 0, 4, 0 },   // output all components of position
			{ 0, "NORMAL", 0, 0, 4, 0 },     // output the first 3 of the normal
			{ 0, "TEXCOORD", 0, 0, 4, 0 },     // output the first 2 texture coordinates
			{ 0, "TEXCOORD", 1, 0, 4, 0 },     // output the first 2 texture coordinates
		};

		sOGS = static_cast<GeometryShader>(wiResourceManager::GetGlobal()->add("shaders/sOGS.cso", wiResourceManager::GEOMETRYSHADER, nullptr, 4, pDecl));
	}


	vertexShader = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/effectVS.cso", wiResourceManager::VERTEXSHADER));
	lightVS[0] = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/dirLightVS.cso", wiResourceManager::VERTEXSHADER));
	lightVS[1] = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/pointLightVS.cso", wiResourceManager::VERTEXSHADER));
	lightVS[2] = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/spotLightVS.cso", wiResourceManager::VERTEXSHADER));
	vSpotLightVS = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/vSpotLightVS.cso", wiResourceManager::VERTEXSHADER));
	vPointLightVS = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/vPointLightVS.cso", wiResourceManager::VERTEXSHADER));
	decalVS = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/decalVS.cso", wiResourceManager::VERTEXSHADER));

	pixelShader = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/effectPS.cso", wiResourceManager::PIXELSHADER));
	transparentPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/effectPS_transparent.cso", wiResourceManager::PIXELSHADER));
	simplestPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/effectPS_simplest.cso", wiResourceManager::PIXELSHADER));
	fowardSimplePS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/effectPS_forwardSimple.cso", wiResourceManager::PIXELSHADER));
	blackoutPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/effectPS_blackout.cso", wiResourceManager::PIXELSHADER));
	textureonlyPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/effectPS_textureonly.cso", wiResourceManager::PIXELSHADER));
	lightPS[0] = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/dirLightPS.cso", wiResourceManager::PIXELSHADER));
	lightPS[1] = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/pointLightPS.cso", wiResourceManager::PIXELSHADER));
	lightPS[2] = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/spotLightPS.cso", wiResourceManager::PIXELSHADER));
	vLightPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/volumeLightPS.cso", wiResourceManager::PIXELSHADER));
	decalPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/decalPS.cso", wiResourceManager::PIXELSHADER));
}
void wiRenderer::LoadLineShaders()
{
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetGlobal()->add("shaders/linesVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr){
		lineVS = vsinfo->vertexShader;
		lineIL = vsinfo->vertexLayout;
	}
	delete vsinfo;


	linePS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/linesPS.cso", wiResourceManager::PIXELSHADER));

}
void wiRenderer::LoadTessShaders()
{
	hullShader = static_cast<HullShader>(wiResourceManager::GetGlobal()->add("shaders/effectHS.cso", wiResourceManager::HULLSHADER));
	domainShader = static_cast<DomainShader>(wiResourceManager::GetGlobal()->add("shaders/effectDS.cso", wiResourceManager::DOMAINSHADER));

}
void wiRenderer::LoadSkyShaders()
{
	skyVS = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/skyVS.cso", wiResourceManager::VERTEXSHADER));
	skyPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/skyPS.cso", wiResourceManager::PIXELSHADER));

}
void wiRenderer::LoadShadowShaders()
{

	shVS = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/shadowVS.cso", wiResourceManager::VERTEXSHADER));
	cubeShVS = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/cubeShadowVS.cso", wiResourceManager::VERTEXSHADER));

	shPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/shadowPS.cso", wiResourceManager::PIXELSHADER));
	cubeShPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/cubeShadowPS.cso", wiResourceManager::PIXELSHADER));

	cubeShGS = static_cast<GeometryShader>(wiResourceManager::GetGlobal()->add("shaders/cubeShadowGS.cso", wiResourceManager::GEOMETRYSHADER));

}
void wiRenderer::LoadWaterShaders()
{

	waterVS = static_cast<VertexShader>(wiResourceManager::GetGlobal()->add("shaders/waterVS.cso", wiResourceManager::VERTEXSHADER));
	waterPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/waterPS.cso", wiResourceManager::PIXELSHADER));

}
void wiRenderer::LoadTrailShaders(){
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetGlobal()->add("shaders/trailVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr){
		trailVS = vsinfo->vertexShader;
		trailIL = vsinfo->vertexLayout;
	}
	delete vsinfo;


	trailPS = static_cast<PixelShader>(wiResourceManager::GetGlobal()->add("shaders/trailPS.cso", wiResourceManager::PIXELSHADER));

}

void wiRenderer::ReloadShaders()
{
	// TODO

	//graphicsMutex.lock();
	//LoadBasicShaders();
	//LoadLineShaders();
	//LoadTessShaders();
	//LoadSkyShaders();
	//LoadShadowShaders();
	//LoadWaterShaders();
	//LoadTrailShaders();
	//graphicsMutex.unlock();
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
	auto transf = transforms.find(get);
	if (transf != transforms.end())
	{
		return transf->second;
	}
	return nullptr;
}
Armature* wiRenderer::getArmatureByName(const string& get)
{
	for(Armature* armature : armatures)
		if(!armature->name.compare(get))
			return armature;
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
	MaterialCollection::iterator iter = materials.find(get);
	if(iter!=materials.end())
		return iter->second;
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
	for (auto& x : objects)
	{
		if (!x->name.compare(name))
		{
			return x;
		}
	}
	for (auto& x : objects_trans)
	{
		if (!x->name.compare(name))
		{
			return x;
		}
	}
	for (auto& x : objects_water)
	{
		if (!x->name.compare(name))
		{
			return x;
		}
	}

	return nullptr;
}
Light* wiRenderer::getLightByName(const string& name)
{
	for (auto& x : lights)
	{
		if (!x->name.compare(name))
		{
			return x;
		}
	}
	return nullptr;
}

void wiRenderer::RecursiveBoneTransform(Armature* armature, Bone* bone, const XMMATRIX& parentCombinedMat){
	float cf = armature->currentFrame;
	float prevActionResolveFrame = armature->prevActionResolveFrame;
	int maxCf = 0;
	int activeAction = armature->activeAction;
	int prevAction = armature->prevAction;
	Bone* parent = (Bone*)bone->parent;
	

	XMVECTOR& finalTrans = InterPolateKeyFrames(cf,maxCf,bone->actionFrames[activeAction].keyframesPos,POSITIONKEYFRAMETYPE);
	XMVECTOR& finalRotat = InterPolateKeyFrames(cf,maxCf,bone->actionFrames[activeAction].keyframesRot,ROTATIONKEYFRAMETYPE);
	XMVECTOR& finalScala = InterPolateKeyFrames(cf,maxCf,bone->actionFrames[activeAction].keyframesSca,SCALARKEYFRAMETYPE);
							
	bone->worldPrev = bone->world;
	bone->translationPrev = bone->translation;
	bone->rotationPrev = bone->rotation;
	XMStoreFloat3(&bone->translation , finalTrans );
	XMStoreFloat4(&bone->rotation , finalRotat );
	XMStoreFloat3(&bone->scale , finalScala );

	XMMATRIX& anim = 
		XMMatrixScalingFromVector( finalScala )
		* XMMatrixRotationQuaternion( finalRotat ) 
		* XMMatrixTranslationFromVector( finalTrans );

	XMMATRIX& rest =
		XMLoadFloat4x4( &bone->world_rest );

	XMMATRIX& boneMat = 
		anim * rest * parentCombinedMat
		;

	XMMATRIX& finalMat = 
		XMLoadFloat4x4( &bone->recursiveRestInv )*
		boneMat
		;
	
	XMStoreFloat4x4( &bone->world,boneMat );

	*bone->boneRelativityPrev=*bone->boneRelativity;
	XMStoreFloat4x4( bone->boneRelativity,finalMat );

	for (unsigned int i = 0; i<bone->childrenI.size(); ++i){
		RecursiveBoneTransform(armature,bone->childrenI[i],boneMat);
	}

}
XMVECTOR wiRenderer::InterPolateKeyFrames(float cf, const int& maxCf, const vector<KeyFrame>& keyframeList, KeyFrameType type)
{
	XMVECTOR result = XMVectorSet(0,0,0,0);
	
	if(type==POSITIONKEYFRAMETYPE) result=XMVectorSet(0,0,0,1);
	if(type==ROTATIONKEYFRAMETYPE) result=XMVectorSet(0,0,0,1);
	if(type==SCALARKEYFRAMETYPE)   result=XMVectorSet(1,1,1,1);
	
	//SEARCH 2 INTERPOLATABLE FRAMES
	int nearest[2] = {0,0};
	int first=0,last=0;
	if(keyframeList.size()>1){
		first=keyframeList[0].frameI;
		last=keyframeList.back().frameI;

		if(cf<=first){ //BROKEN INTERVAL
			nearest[0] = 0;
			nearest[1] = 0;
		}
		else if(cf>=last){
			nearest[0] = keyframeList.size()-1;
			nearest[1] = keyframeList.size()-1;
		}
		else{ //IN BETWEEN TWO KEYFRAMES, DECIDE WHICH
			for(int k=keyframeList.size()-1;k>0;k--)
				if(keyframeList[k].frameI<=cf){
					nearest[0] = k;
					break;
				}
			for (unsigned int k = 0; k<keyframeList.size(); k++)
				if(keyframeList[k].frameI>=cf){
					nearest[1] = k;
					break;
			}
		}

		//INTERPOLATE BETWEEN THE TWO FRAMES
		int keyframes[2] = {
			keyframeList[nearest[0]].frameI,
			keyframeList[nearest[1]].frameI
		};
		float interframe=0;
		if(cf<=first || cf>=last){ //BROKEN INTERVAL
			float intervalBegin = (float)(maxCf - keyframes[0]);
			float intervalEnd = keyframes[1] + intervalBegin;
			float intervalLen = abs(intervalEnd - intervalBegin);
			float offsetCf = cf + intervalBegin;
			if(intervalLen) interframe = offsetCf / intervalLen;
		}
		else{
			float intervalBegin = (float)keyframes[0];
			float intervalEnd = (float)keyframes[1];
			float intervalLen = abs(intervalEnd - intervalBegin);
			float offsetCf = cf - intervalBegin;
			if(intervalLen) interframe = offsetCf / intervalLen;
		}

		if(type==ROTATIONKEYFRAMETYPE){
			XMVECTOR quat[2]={
				XMLoadFloat4(&keyframeList[nearest[0]].data),
				XMLoadFloat4(&keyframeList[nearest[1]].data)
			};
			result = XMQuaternionNormalize( XMQuaternionSlerp(quat[0],quat[1],interframe) );
		}
		else{
			XMVECTOR tran[2]={
				XMLoadFloat4(&keyframeList[nearest[0]].data),
				XMLoadFloat4(&keyframeList[nearest[1]].data)
			};
			result = XMVectorLerp(tran[0],tran[1],interframe);
		}
	}
	else{
		if(!keyframeList.empty())
			result = XMLoadFloat4(&keyframeList.back().data);
	}

	return result;
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
			sump += XMLoadFloat4x4( mesh->armature->boneCollection[int(inBon[i])]->boneRelativity ) * inWei[i];
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
			sump += XMLoadFloat4x4( mesh->armature->boneCollection[int(inBon[i])]->boneRelativity ) * inWei[i];
			sumpPrev += XMLoadFloat4x4( mesh->armature->boneCollection[int(inBon[i])]->boneRelativityPrev ) * inWei[i];
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
void wiRenderer::Update(float amount)
{
	*prevFrameCam = *cam;
	cam->Update();
	refCam->Reflect(cam);

	//if(GetGameSpeed())
	{
		for(Armature* armature : armatures){
			if(armature->actions.size()){
				//XMMATRIX world = XMMatrixScalingFromVector(XMLoadFloat3(&armature->scale))*XMMatrixRotationQuaternion(XMLoadFloat4(&armature->rotation))*XMMatrixTranslationFromVector(XMLoadFloat3(&armature->translation));
				//XMStoreFloat4x4( &armature->world,world );
				XMMATRIX world = armature->getTransform();

				float cf = armature->currentFrame;
				float prevActionResolveFrame = armature->prevActionResolveFrame;
				int maxCf = 0;
				int activeAction = armature->activeAction;
				int prevAction = armature->prevAction;
				
				cf = armature->currentFrame += (armature->playing ? amount : 0.f);
				maxCf = armature->actions[activeAction].frameCount;
				if ((int)cf > maxCf)
				{
					armature->ResetAction();
					cf = armature->currentFrame;
				}

				{
					for (unsigned int j = 0; j<armature->rootbones.size(); ++j){
						RecursiveBoneTransform(armature,armature->rootbones[j],world);
					}
				}
			}
		}

		

		for(MaterialCollection::iterator iter=materials.begin(); iter!=materials.end();++iter){
			Material* iMat=iter->second;
			iMat->framesToWaitForTexCoordOffset-=1.0f;
			if(iMat->framesToWaitForTexCoordOffset<=0){
				iMat->texOffset.x+=iMat->movingTex.x*GameSpeed;
				iMat->texOffset.y+=iMat->movingTex.y*GameSpeed;
				iMat->framesToWaitForTexCoordOffset=iMat->movingTex.z*GameSpeed;
			}
		}

		for(map<string,vector<wiEmittedParticle*>>::iterator iter=emitterSystems.begin();iter!=emitterSystems.end();++iter){
			for(wiEmittedParticle* e:iter->second)
				e->Update(GameSpeed);
		}


		list<Decal*>::iterator iter = decals.begin();
		while (iter != decals.end())
		{
			Decal* decal = *iter;
			decal->getTransform();
			decal->Update();
			if(decal->life>-2){
				decal->life-=GameSpeed;
				if(decal->life<=0){
					decal->detach();
					decals.erase(iter++);
					continue;
				}
			}
			++iter;
		}
	
	}	
}
void wiRenderer::UpdateRenderInfo(ID3D11DeviceContext* context)
{
	UpdateObjects();

	if (context == nullptr)
		return;

	UpdatePerWorldCB(context);
		

	//if(GetGameSpeed())
	{


		for(MeshCollection::iterator iter=meshes.begin(); iter!=meshes.end(); ++iter){
			Mesh* mesh = iter->second;
			if(mesh->hasArmature() && !mesh->softBody && mesh->renderable){
#ifdef USE_GPU_SKINNING
				BoneShaderBuffer bonebuf = BoneShaderBuffer();
				for (unsigned int k = 0; k<mesh->armature->boneCollection.size(); k++){
					bonebuf.pose[k]=XMMatrixTranspose(XMLoadFloat4x4(mesh->armature->boneCollection[k]->boneRelativity));
					bonebuf.prev[k]=XMMatrixTranspose(XMLoadFloat4x4(mesh->armature->boneCollection[k]->boneRelativityPrev));
				}
				
				
				UpdateBuffer(mesh->boneBuffer,&bonebuf,context);
#else
				for(int vi=0;vi<mesh->skinnedVertices.size();++vi)
					mesh->skinnedVertices[vi]=TransformVertex(mesh,vi);
#endif
			}
		}
#ifdef USE_GPU_SKINNING
		//wiRenderer::graphicsMutex.lock();
		DrawForSO(context);
		//wiRenderer::graphicsMutex.unlock();
#endif


		UpdateSPTree(spTree);
		UpdateSPTree(spTree_trans);
		UpdateSPTree(spTree_water);
	}
	
	UpdateBoneLines();
	UpdateCubes();

	wind.time = (float)((wiTimer::TotalTime()) / 1000.0*GameSpeed / 2.0*3.1415)*XMVectorGetX(XMVector3Length(XMLoadFloat3(&wind.direction)))*0.1f;
}
void wiRenderer::UpdateObjects(){
	for (unsigned int i = 0; i<everyObject.size(); i++){

		XMMATRIX world = everyObject[i]->getTransform();
		
		if(everyObject[i]->mesh->isBillboarded){
			XMMATRIX bbMat = XMMatrixIdentity();
			if(everyObject[i]->mesh->billboardAxis.x || everyObject[i]->mesh->billboardAxis.y || everyObject[i]->mesh->billboardAxis.z){
				float angle = 0;
				angle = (float)atan2(everyObject[i]->translation.x - wiRenderer::getCamera()->translation.x, everyObject[i]->translation.z - wiRenderer::getCamera()->translation.z) * (180.0f / XM_PI);
				bbMat = XMMatrixRotationAxis(XMLoadFloat3(&everyObject[i]->mesh->billboardAxis), angle * 0.0174532925f );
			}
			else
				bbMat = XMMatrixInverse(0,XMMatrixLookAtLH(XMVectorSet(0,0,0,0),XMVectorSubtract(XMLoadFloat3(&everyObject[i]->translation),wiRenderer::getCamera()->GetEye()),XMVectorSet(0,1,0,0)));
					
			XMMATRIX w = XMMatrixScalingFromVector(XMLoadFloat3(&everyObject[i]->scale)) * 
						bbMat * 
						XMMatrixRotationQuaternion(XMLoadFloat4(&everyObject[i]->rotation)) *
						XMMatrixTranslationFromVector(XMLoadFloat3(&everyObject[i]->translation)
					);
			XMStoreFloat4x4(&everyObject[i]->world,w);
		}

		if(everyObject[i]->mesh->softBody)
			everyObject[i]->bounds=everyObject[i]->mesh->aabb;
		else if(!everyObject[i]->mesh->isBillboarded && everyObject[i]->mesh->renderable){
			everyObject[i]->bounds=everyObject[i]->mesh->aabb.get(world);
		}
		else if(everyObject[i]->mesh->renderable)
			everyObject[i]->bounds.createFromHalfWidth(everyObject[i]->translation,everyObject[i]->scale);
	}
}
void wiRenderer::UpdateSoftBodyPinning(){
	for(MeshCollection::iterator iter=meshes.begin(); iter!=meshes.end(); ++iter){
		Mesh* m = iter->second;
		if(m->softBody){
			int gvg = m->goalVG;
			if(gvg>=0){
				int j=0;
				for(map<int,float>::iterator it=m->vertexGroups[gvg].vertices.begin();it!=m->vertexGroups[gvg].vertices.end();++it){
					int vi = (*it).first;
					Vertex tvert=m->skinnedVertices[vi];
					if(m->hasArmature()) 
						tvert = TransformVertex(m,vi);
					m->goalPositions[j] = XMFLOAT3(tvert.pos.x,tvert.pos.y,tvert.pos.z);
					m->goalNormals[j] = XMFLOAT3(tvert.nor.x, tvert.nor.y, tvert.nor.z);
					++j;
				}
			}
		}
	}
}
void wiRenderer::UpdateSkinnedVB(){
	wiRenderer::graphicsMutex.lock();
	for(MeshCollection::iterator iter=meshes.begin(); iter!=meshes.end(); ++iter){
		Mesh* m = iter->second;
#ifdef USE_GPU_SKINNING
		if(m->softBody)
#else
		if(m->softBody || m->hasArmature())
#endif
		{
			UpdateBuffer(m->meshVertBuff,m->skinnedVertices.data(),wiRenderer::getImmediateContext(),sizeof(Vertex)*m->skinnedVertices.size());
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			//void* dataPtr;
			//wiRenderer::getImmediateContext()->Map(m->meshVertBuff,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr = (void*)mappedResource.pData;
			//memcpy(dataPtr,m->skinnedVertices.data(),sizeof(Vertex)*m->skinnedVertices.size());
			//wiRenderer::getImmediateContext()->Unmap(m->meshVertBuff,0);
		}
	}
	wiRenderer::graphicsMutex.unlock();
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
				(images.front()->effects.opacity==1 || images.front()->effects.fade==1)
			)
			images.pop_front();
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
			(waterRipples.front()->effects.opacity==1 || waterRipples.front()->effects.fade==1)
		)
		waterRipples.pop_front();
}
void wiRenderer::DrawWaterRipples(ID3D11DeviceContext* context){
	wiImage::BatchBegin(context);
	for(wiSprite* i:waterRipples){
		i->DrawNormal(context);
	}
}

void wiRenderer::DrawDebugSpheres(Camera* camera, ID3D11DeviceContext* context)
{
	if(debugSpheres){
		//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
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
void wiRenderer::DrawDebugBoneLines(Camera* camera, ID3D11DeviceContext* context)
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
void wiRenderer::DrawDebugLines(Camera* camera, ID3D11DeviceContext* context)
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
void wiRenderer::DrawDebugBoxes(Camera* camera, ID3D11DeviceContext* context)
{
	if(debugBoxes){
		//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
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

void wiRenderer::DrawSoftParticles(Camera* camera, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, bool dark)
{
	struct particlesystem_comparator {
		bool operator() (const wiEmittedParticle* a, const wiEmittedParticle* b) const{
			return a->lastSquaredDistMulThousand>b->lastSquaredDistMulThousand;
		}
	};

	set<wiEmittedParticle*,particlesystem_comparator> psystems;
	for(map<string,vector<wiEmittedParticle*>>::iterator iter=emitterSystems.begin();iter!=emitterSystems.end();++iter){
		for(wiEmittedParticle* e:iter->second){
			e->lastSquaredDistMulThousand=(long)(wiMath::DistanceEstimated(e->bounding_box->getCenter(),camera->translation)*1000);
			psystems.insert(e);
		}
	}

	for(wiEmittedParticle* e:psystems){
		e->DrawNonPremul(camera,context,depth,dark);
	}

	//for(int i=0;i<objects.size();++i){
	//	for(int j=0;j<objects[i]->eParticleSystems.size();j++){
	//		Material* mat = objects[i]->eParticleSystems[j]->material;
	//		if(mat){
	//			if(!mat->premultipliedTexture) objects[i]->eParticleSystems[j]->Draw(eye,view,mat->texture,context,depth,dark);
	//		}
	//	}
	//}
}
void wiRenderer::DrawSoftPremulParticles(Camera* camera, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, bool dark)
{
	for(map<string,vector<wiEmittedParticle*>>::iterator iter=emitterSystems.begin();iter!=emitterSystems.end();++iter){
		for(wiEmittedParticle* e:iter->second)
			e->DrawPremul(camera,context,depth,dark);
	}

	//for(int i=0;i<objects.size();++i){
	//	for(int j=0;j<objects[i]->eParticleSystems.size();j++){
	//		Material* mat = objects[i]->eParticleSystems[j]->material;
	//		if(mat){
	//			if(mat->premultipliedTexture) objects[i]->eParticleSystems[j]->Draw(eye,view,mat->texture,context,depth,dark);
	//		}
	//	}
	//}
}
void wiRenderer::DrawTrails(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes){
	//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
	//context->IASetInputLayout( trailIL );
	BindPrimitiveTopology(TRIANGLESTRIP,context);
	BindVertexLayout(trailIL,context);
	
	//context->RSSetState(wireRender?nonCullWireRS:nonCullRS);
	//context->OMSetDepthStencilState(depthReadStencilState, STENCILREF_EMPTY);

	
	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//UINT sampleMask   = 0xffffffff;
	//context->OMSetBlendState(blendState, blendFactor, sampleMask);

	BindRasterizerState(wireRender?nonCullWireRS:nonCullRS,context);
	BindDepthStencilState(depthReadStencilState,STENCILREF_EMPTY,context);
	BindBlendState(blendState,context);

	//context->VSSetShader( trailVS, NULL, 0 );
	//context->PSSetShader( trailPS, NULL, 0 );
	BindPS(trailPS,context);
	BindVS(trailVS,context);
	
	BindTexturePS(refracRes,0,context);
	BindTexturePS(trailDistortTex,1,context);
	//context->PSSetSamplers( 0,1,&mirSampler );
	BindSamplerPS(mirSampler,0,context);

	for (unsigned int i = 0; i<everyObject.size(); i++){
		if(everyObject[i]->trailBuff && everyObject[i]->trail.size()>=4){
			
			
			//context->VSSetConstantBuffers( 0, 1, &trailCB );
			BindConstantBufferVS(trailCB,0,context);

			vector<RibbonVertex> trails;
			//for(int k=0;k<everyObject[i]->trail.size();++k)
			//	trails.push_back(everyObject[i]->trail[k]);

			int bounds = everyObject[i]->trail.size();
			int req = bounds-3;
			for(int k=0;k<req;k+=2)
			{
				static const float trailres = 10.f;
				for(float r=0.0f;r<=1.0f;r+=1.0f/trailres)
				{
					XMVECTOR point0 = XMVectorCatmullRom(
						XMLoadFloat3( &everyObject[i]->trail[k?(k-2):0].pos )
						,XMLoadFloat3( &everyObject[i]->trail[k].pos )
						,XMLoadFloat3( &everyObject[i]->trail[k+2].pos )
						,XMLoadFloat3( &everyObject[i]->trail[k+6<bounds?(k+6):(bounds-2)].pos )
						,r
					),
					point1 = XMVectorCatmullRom(
						XMLoadFloat3( &everyObject[i]->trail[k?(k-1):1].pos )
						,XMLoadFloat3( &everyObject[i]->trail[k+1].pos )
						,XMLoadFloat3( &everyObject[i]->trail[k+3].pos )
						,XMLoadFloat3( &everyObject[i]->trail[k+5<bounds?(k+5):(bounds-1)].pos )
						,r
					);
					XMFLOAT3 xpoint0,xpoint1;
					XMStoreFloat3(&xpoint0,point0);
					XMStoreFloat3(&xpoint1,point1);
					trails.push_back(RibbonVertex(xpoint0
						,wiMath::Lerp(everyObject[i]->trail[k].tex,everyObject[i]->trail[k+2].tex,r)
						,wiMath::Lerp(everyObject[i]->trail[k].col,everyObject[i]->trail[k+2].col,r)
						));
					trails.push_back(RibbonVertex(xpoint1
						,wiMath::Lerp(everyObject[i]->trail[k+1].tex,everyObject[i]->trail[k+3].tex,r)
						,wiMath::Lerp(everyObject[i]->trail[k+1].col,everyObject[i]->trail[k+3].col,r)
						));
				}
			}
			if(!trails.empty()){
				UpdateBuffer(everyObject[i]->trailBuff,trails.data(),context,sizeof(RibbonVertex)*trails.size());
				//D3D11_MAPPED_SUBRESOURCE mappedResource2;
				//RibbonVertex* dataPtrV;
				//context->Map(everyObject[i]->trailBuff,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource2);
				//dataPtrV = (RibbonVertex*)mappedResource2.pData;
				//memcpy(dataPtrV,trails.data(),sizeof(RibbonVertex)*trails.size());
				//context->Unmap(everyObject[i]->trailBuff,0);
			
				//UINT stride = sizeof( RibbonVertex );
				//UINT offset = 0;
				//context->IASetVertexBuffers( 0, 1, &everyObject[i]->trailBuff, &stride, &offset );
			


				//context->Draw(trails.size(),0);
				BindVertexBuffer(everyObject[i]->trailBuff,0,sizeof(RibbonVertex),context);
				Draw(trails.size(),context);

				trails.clear();
			}
		}
	}
}
void wiRenderer::DrawImagesAdd(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes){
	imagesRTAdd.Activate(context,0,0,0,1);
	wiImage::BatchBegin(context);
	for(wiSprite* x : images){
		if(x->effects.blendFlag==BLENDMODE_ADDITIVE){
			/*ID3D11ShaderResourceView* nor = x->effects.normalMap;
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
void wiRenderer::DrawImages(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes){
	imagesRT.Activate(context,0,0,0,0);
	wiImage::BatchBegin(context);
	for(wiSprite* x : images){
		if(x->effects.blendFlag==BLENDMODE_ALPHA || x->effects.blendFlag==BLENDMODE_OPAQUE){
			/*ID3D11ShaderResourceView* nor = x->effects.normalMap;
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
void wiRenderer::DrawImagesNormals(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes){
	normalMapRT.Activate(context,0,0,0,0);
	wiImage::BatchBegin(context);
	for(wiSprite* x : images){
		x->DrawNormal(context);
	}
}
void wiRenderer::DrawLights(Camera* camera, ID3D11DeviceContext* context
				, ID3D11ShaderResourceView* depth, ID3D11ShaderResourceView* normal, ID3D11ShaderResourceView* material
				, unsigned int stencilRef){

	
	Frustum frustum = Frustum();
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
				dLightBuffer lcb;
				lcb.direction=XMVector3Normalize(
					-XMVector3Transform( XMVectorSet(0,-1,0,1), XMMatrixRotationQuaternion( XMLoadFloat4(&l->rotation) ) )
					);
				lcb.col=XMFLOAT4(l->color.x*l->enerDis.x,l->color.y*l->enerDis.x,l->color.z*l->enerDis.x,1);
				lcb.mBiasResSoftshadow=XMFLOAT4(l->shadowBias,(float)SHADOWMAPRES,(float)SOFTSHADOW,0);
				for (unsigned int shmap = 0; shmap < l->shadowMaps_dirLight.size(); ++shmap){
					lcb.mShM[shmap]=l->shadowCam[shmap].getVP();
					BindTexturePS(l->shadowMaps_dirLight[shmap].depth->shaderResource,4+shmap,context);
				}
				UpdateBuffer(lightCb[type],&lcb,context);

				Draw(6, context);
			}
			else if(type==1) //point
			{
				pLightBuffer lcb;
				lcb.pos=l->translation;
				lcb.col=l->color;
				lcb.enerdis=l->enerDis;
				lcb.enerdis.w = 0.f;

				if (l->shadow && l->shadowMap_index>=0)
				{
					lcb.enerdis.w = 1.f;
					BindTexturePS(Light::shadowMaps_pointLight[l->shadowMap_index].depth->shaderResource, 7, context);
				}
				UpdateBuffer(lightCb[type], &lcb, context);

				Draw(240, context);
			}
			else if(type==2) //spot
			{
				sLightBuffer lcb;
				const float coneS=(const float)(l->enerDis.z/0.7853981852531433);
				XMMATRIX world,rot;
				world = XMMatrixTranspose(
						XMMatrixScaling(coneS*l->enerDis.y,l->enerDis.y,coneS*l->enerDis.y)*
						XMMatrixRotationQuaternion( XMLoadFloat4( &l->rotation ) )*
						XMMatrixTranslationFromVector( XMLoadFloat3(&l->translation) )
						);
				rot=XMMatrixRotationQuaternion( XMLoadFloat4(&l->rotation) );
				lcb.direction=XMVector3Normalize(
					-XMVector3Transform( XMVectorSet(0,-1,0,1), rot )
					);
				lcb.world=world;
				lcb.mBiasResSoftshadow=XMFLOAT4(l->shadowBias,(float)SPOTLIGHTSHADOWRES,(float)SOFTSHADOW,0);
				lcb.mShM = XMMatrixIdentity();
				lcb.col=l->color;
				lcb.enerdis=l->enerDis;
				lcb.enerdis.z=(float)cos(l->enerDis.z/2.0);

				if (l->shadow && l->shadowMap_index>=0)
				{
					lcb.mShM = l->shadowCam[0].getVP();
					BindTexturePS(Light::shadowMaps_spotLight[l->shadowMap_index].depth->shaderResource, 4, context);
				}
				UpdateBuffer(lightCb[type], &lcb, context);

				Draw(192, context);
			}
		}


	}

	}
}
void wiRenderer::DrawVolumeLights(Camera* camera, ID3D11DeviceContext* context)
{
	
	Frustum frustum = Frustum();
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

					vLightBuffer lcb;
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
					lcb.world=world;
					lcb.col=l->color;
					lcb.enerdis=l->enerDis;
					lcb.enerdis.w=sca;

					UpdateBuffer(vLightCb,&lcb,context);

					if(type<=1)
						Draw(108,context);
					else
						Draw(192,context);
				}
			}

		}

	}
}


void wiRenderer::DrawLensFlares(ID3D11DeviceContext* context, ID3D11ShaderResourceView* depth, const int& RENDERWIDTH, const int& RENDERHEIGHT){
	
	Frustum frustum = Frustum();
	frustum.ConstructFrustum(cam->zFarP,cam->Projection,cam->View);

		
	CulledList culledObjects;
	if(spTree_lights)
		wiSPTree::getVisible(spTree_lights->root,frustum,culledObjects);

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
			
			XMVECTOR flarePos = XMVector3Project(POS,0.f,0.f,(float)RENDERWIDTH,(float)RENDERHEIGHT,0.1f,1.0f,wiRenderer::getCamera()->GetProjection(),wiRenderer::getCamera()->GetView(),XMMatrixIdentity());

			if( XMVectorGetX(XMVector3Dot( XMVectorSubtract(POS,wiRenderer::getCamera()->GetEye()),wiRenderer::getCamera()->GetAt() ))>0 )
				wiLensFlare::Draw(depth,context,flarePos,l->lensFlareRimTextures);

		}

	}
}
void wiRenderer::ClearShadowMaps(ID3D11DeviceContext* context){
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
void wiRenderer::DrawForShadowMap(ID3D11DeviceContext* context)
{
	if (GameSpeed) {

		Frustum frustum = Frustum();
		frustum.ConstructFrustum(cam->zFarP, cam->Projection, cam->View);


		CulledList culledLights;
		if (spTree_lights)
			wiSPTree::getVisible(spTree_lights->root, frustum, culledLights);

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
						CulledCollection culledwiRenderer;

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
									culledwiRenderer[((Object*)object)->mesh].insert((Object*)object);
								}

								for (CulledCollection::iterator iter = culledwiRenderer.begin(); iter != culledwiRenderer.end(); ++iter) {
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
											if ((*viter)->particleEmitter != Object::wiParticleEmitter::EMITTER_INVISIBLE) {
												if (mesh->softBody || (*viter)->armatureDeform)
													mesh->AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k, GRAPHICSTHREAD_SCENE);
												else
													mesh->AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k, GRAPHICSTHREAD_SCENE);
												++k;
											}
										}
										if (k < 1)
											continue;

										mesh->UpdateRenderableInstances(visibleInstances.size(), GRAPHICSTHREAD_SCENE, context);


										BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), context);
										BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), context);
										BindIndexBuffer(mesh->meshIndexBuff, context);


										int matsiz = mesh->materialIndices.size();
										int m = 0;
										for (Material* iMat : mesh->materials) {

											if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
												BindTexturePS(iMat->texture, 0, context);



												MaterialCB mcb = MaterialCB(*iMat, m);

												UpdateBuffer(matCb, &mcb, context);


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
					CulledCollection culledwiRenderer;

					Light::shadowMaps_spotLight[i].Set(context);
					Frustum frustum = Frustum();
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
								culledwiRenderer[((Object*)object)->mesh].insert((Object*)object);
							}

							for (CulledCollection::iterator iter = culledwiRenderer.begin(); iter != culledwiRenderer.end(); ++iter) {
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
										if ((*viter)->particleEmitter != Object::wiParticleEmitter::EMITTER_INVISIBLE) {
											if (mesh->softBody || (*viter)->armatureDeform)
												mesh->AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k, GRAPHICSTHREAD_SCENE);
											else
												mesh->AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k, GRAPHICSTHREAD_SCENE);
											++k;
										}
									}
									if (k < 1)
										continue;

									mesh->UpdateRenderableInstances(visibleInstances.size(), GRAPHICSTHREAD_SCENE, context);


									BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), context);
									BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), context);
									BindIndexBuffer(mesh->meshIndexBuff, context);


									int matsiz = mesh->materialIndices.size();
									int m = 0;
									for (Material* iMat : mesh->materials) {

										if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
											BindTexturePS(iMat->texture, 0, context);



											MaterialCB mcb = MaterialCB(*iMat, m);

											UpdateBuffer(matCb, &mcb, context);


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
					CulledCollection culledwiRenderer;

					if (spTree)
						wiSPTree::getVisible(spTree->root, l->bounds, culledObjects);

					for (Cullable* object : culledObjects)
						culledwiRenderer[((Object*)object)->mesh].insert((Object*)object);

					for (CulledCollection::iterator iter = culledwiRenderer.begin(); iter != culledwiRenderer.end(); ++iter) {
						Mesh* mesh = iter->first;
						CulledObjectList& visibleInstances = iter->second;

						if (!mesh->isBillboarded && !visibleInstances.empty()) {

							if (!mesh->doubleSided)
								BindRasterizerState(rssh, context);
							else
								BindRasterizerState(nonCullRSsh, context);



							int k = 0;
							for (CulledObjectList::iterator viter = visibleInstances.begin(); viter != visibleInstances.end(); ++viter) {
								if ((*viter)->particleEmitter != Object::wiParticleEmitter::EMITTER_INVISIBLE) {
									if (mesh->softBody || (*viter)->armatureDeform)
										mesh->AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k, GRAPHICSTHREAD_SCENE);
									else
										mesh->AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k, GRAPHICSTHREAD_SCENE);
									++k;
								}
							}
							if (k < 1)
								continue;

							mesh->UpdateRenderableInstances(visibleInstances.size(), GRAPHICSTHREAD_SCENE, context);


							BindVertexBuffer((mesh->sOutBuffer ? mesh->sOutBuffer : mesh->meshVertBuff), 0, sizeof(Vertex), context);
							BindVertexBuffer(mesh->meshInstanceBuffer, 1, sizeof(Instance), context);
							BindIndexBuffer(mesh->meshIndexBuff, context);


							int matsiz = mesh->materialIndices.size();
							int m = 0;
							for (Material* iMat : mesh->materials) {
								if (!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
									BindTexturePS(iMat->texture, 0, context);

									MaterialCB mcb = MaterialCB(*iMat, m);

									UpdateBuffer(matCb, &mcb, context);

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

	//					CulledCollection culledwiRenderer;
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
	//							culledwiRenderer[((Object*)object)->mesh].insert((Object*)object);
	//						}

	//						for (CulledCollection::iterator iter = culledwiRenderer.begin(); iter != culledwiRenderer.end(); ++iter) {
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
	//											mesh->AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k, GRAPHICSTHREAD_SHADOWS);
	//										else
	//											mesh->AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k, GRAPHICSTHREAD_SHADOWS);
	//										++k;
	//									}
	//								}
	//								if (k<1)
	//									continue;

	//								mesh->UpdateRenderableInstances(visibleInstances.size(), GRAPHICSTHREAD_SHADOWS, context);


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

	//					CulledCollection culledwiRenderer;
	//					CulledList culledObjects;

	//					if (spTree)
	//						wiSPTree::getVisible(spTree->root, l->bounds, culledObjects);

	//					for (Cullable* object : culledObjects)
	//						culledwiRenderer[((Object*)object)->mesh].insert((Object*)object);

	//					for (CulledCollection::iterator iter = culledwiRenderer.begin(); iter != culledwiRenderer.end(); ++iter) {
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
	//										mesh->AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency), k, GRAPHICSTHREAD_SHADOWS);
	//									else
	//										mesh->AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency), k, GRAPHICSTHREAD_SHADOWS);
	//									++k;
	//								}
	//							}
	//							if (k<1)
	//								continue;

	//							mesh->UpdateRenderableInstances(visibleInstances.size(), GRAPHICSTHREAD_SHADOWS, context);


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
void wiRenderer::DrawForSO(ID3D11DeviceContext* context)
{
	BindPrimitiveTopology(POINTLIST,context);
	BindVertexLayout(sOIL,context);
	BindVS(sOVS,context);
	BindGS(sOGS,context);
	BindPS(nullptr,context);

	for(MeshCollection::iterator iter=meshes.begin(); iter!=meshes.end(); ++iter)
	{
		Mesh* mesh = iter->second;
		if(mesh->hasArmature() && !mesh->softBody)
		{
			BindVertexBuffer(mesh->meshVertBuff,0,sizeof(SkinnedVertex),context);
			BindConstantBufferVS(mesh->boneBuffer,1,context);
			BindStreamOutTarget(mesh->sOutBuffer,context);
			Draw(mesh->vertices.size(),context);
		}
	}

	BindGS(nullptr,context);
	BindVS(nullptr,context);
	BindStreamOutTarget(nullptr,context);
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

void wiRenderer::DrawWorld(Camera* camera, bool DX11Eff, int tessF, ID3D11DeviceContext* context
				  , bool BlackOut, SHADED_TYPE shaded
				  , ID3D11ShaderResourceView* refRes, bool grass, GRAPHICSTHREAD thread)
{
	
	if(objects.empty())
		return;

	Frustum frustum = Frustum();
	frustum.ConstructFrustum(camera->zFarP,camera->Projection,camera->View);

	CulledCollection culledwiRenderer;
	CulledList culledObjects;
	if(spTree)
		wiSPTree::getVisible(spTree->root,frustum,culledObjects);
		//wiSPTree::getAll(spTree->root,culledObjects);

	if(!culledObjects.empty())
	{	

		for(Cullable* object : culledObjects){
			culledwiRenderer[((Object*)object)->mesh].insert((Object*)object);
			if(grass){
				for(wiHairParticle* hair : ((Object*)object)->hwiParticleSystems){
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
			else if (shaded == SHADED_NONE)
				BindPS(textureonlyPS, context);
			else if (shaded == SHADED_DEFERRED)
				BindPS(pixelShader, context);
			else if (shaded == SHADED_FORWARD_SIMPLE)
				BindPS(fowardSimplePS, context);
			else
				return;

		BindSamplerVS(texSampler,0,context);
		BindTextureVS(noiseTex,0,context);
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


		for (CulledCollection::iterator iter = culledwiRenderer.begin(); iter != culledwiRenderer.end(); ++iter) {
			Mesh* mesh = iter->first;
			CulledObjectList& visibleInstances = iter->second;


			if(!mesh->doubleSided)
				BindRasterizerState(wireRender?wireRS:rasterizerState,context);
			else
				BindRasterizerState(wireRender?wireRS:nonCullRS,context);

			int matsiz = mesh->materialIndices.size();
			
			if(DX11 && tessF){
				ConstantBuffer cb;
				if(matsiz == 1) cb.mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,0,0,0);
				else if(matsiz == 2) cb.mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,mesh->materials[1]->hasDisplacementMap,0,0);
				else if(matsiz == 3) cb.mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,mesh->materials[1]->hasDisplacementMap,mesh->materials[2]->hasDisplacementMap,0);
				else if(matsiz == 4) cb.mDisplace = XMVectorSet(mesh->materials[0]->hasDisplacementMap,mesh->materials[1]->hasDisplacementMap,mesh->materials[2]->hasDisplacementMap,mesh->materials[3]->hasDisplacementMap);

				UpdateBuffer(constantBuffer,&cb,context);
			}

			int k=0;
			for(CulledObjectList::iterator viter=visibleInstances.begin();viter!=visibleInstances.end();++viter){
				if((*viter)->particleEmitter!=Object::wiParticleEmitter::EMITTER_INVISIBLE){
					if (mesh->softBody || (*viter)->armatureDeform)
						mesh->AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency, (*viter)->color), k, thread);
					else 
						mesh->AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency, (*viter)->color), k, thread);
					++k;
				}
			}
			if(k<1)
				continue;

			mesh->UpdateRenderableInstances(visibleInstances.size(), thread, context);
				
				
			BindIndexBuffer(mesh->meshIndexBuff,context);
			BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),context);
			BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),context);

			int m=0;
			for(Material* iMat : mesh->materials){

				if(!iMat->transparent && !iMat->isSky && !iMat->water){
					
					if(iMat->shadeless)
						BindDepthStencilState(depthStencilState,STENCILREF_SHADELESS,context);
					if(iMat->subsurface_scattering)
						BindDepthStencilState(depthStencilState,STENCILREF_SKIN,context);
					else
						BindDepthStencilState(depthStencilState,mesh->stencilRef,context);


					MaterialCB* mcb = new MaterialCB(*iMat,m);

					UpdateBuffer(matCb,mcb,context);
					
					delete mcb;

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
void wiRenderer::DrawWorldWater(Camera* camera, ID3D11ShaderResourceView* refracRes, ID3D11ShaderResourceView* refRes
		, ID3D11ShaderResourceView* depth, ID3D11ShaderResourceView* nor, ID3D11DeviceContext* context){
			
	if(objects_water.empty())
		return;

	Frustum frustum = Frustum();
	frustum.ConstructFrustum(camera->zFarP, camera->Projection, camera->View);

	CulledCollection culledwiRenderer;
	CulledList culledObjects;
	if(spTree_water)
		wiSPTree::getVisible(spTree_water->root,frustum,culledObjects);

	if(!culledObjects.empty())
	{
		for(Cullable* object : culledObjects)
			culledwiRenderer[((Object*)object)->mesh].insert((Object*)object);


		BindPrimitiveTopology(TRIANGLELIST,context);
		BindVertexLayout(vertexLayout,context);
		BindPS(wireRender?simplestPS:waterPS,context);
		BindVS(vertexShader10,context);

		BindSamplerPS(texSampler,0,context);
		BindSamplerPS(mapSampler,1,context);
	
		if(!wireRender) BindTexturePS(enviroMap,0,context);
		if(!wireRender) BindTexturePS(refRes,1,context);
		if(!wireRender) BindTexturePS(refracRes,2,context);
		if(!wireRender) BindTexturePS(depth,7,context);
		if(!wireRender) BindTexturePS(nor,8,context);

		
		BindConstantBufferVS(staticCb,0,context);
		BindConstantBufferPS(pixelCB,0,context);
		BindConstantBufferPS(fxCb,1,context);
		BindConstantBufferVS(constantBuffer,3,context);
		BindConstantBufferVS(matCb,2,context);
		BindConstantBufferPS(matCb,2,context);

	
		BindBlendState(blendState,context);
		BindDepthStencilState(depthReadStencilState,STENCILREF_EMPTY,context);
		BindRasterizerState(wireRender?wireRS:nonCullRS,context);
	

		
		for (CulledCollection::iterator iter = culledwiRenderer.begin(); iter != culledwiRenderer.end(); ++iter) {
			Mesh* mesh = iter->first;
			CulledObjectList& visibleInstances = iter->second;


			int matsiz = mesh->materialIndices.size();

				
			int k = 0;
			for (CulledObjectList::iterator viter = visibleInstances.begin(); viter != visibleInstances.end(); ++viter){
				if ((*viter)->particleEmitter != Object::wiParticleEmitter::EMITTER_INVISIBLE){
					if (mesh->softBody || (*viter)->armatureDeform)
						mesh->AddRenderableInstance(Instance(XMMatrixIdentity()), k, GRAPHICSTHREAD_MISC1);
					else
						mesh->AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world))), k, GRAPHICSTHREAD_MISC1);
					++k;
				}
			}
			if (k<1)
				continue;

			mesh->UpdateRenderableInstances(visibleInstances.size(), GRAPHICSTHREAD_MISC1, context);
				
			BindIndexBuffer(mesh->meshIndexBuff,context);
			BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),context);
			BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),context);

			int m=0;
			for(Material* iMat : mesh->materials){

				if(iMat->water){

				MaterialCB* mcb = new MaterialCB(*iMat,m);

				UpdateBuffer(matCb,mcb,context);
				_aligned_free(mcb);

				if(!wireRender) BindTexturePS(iMat->texture,3,context);
				if(!wireRender) BindTexturePS(iMat->refMap,4,context);
				if(!wireRender) BindTexturePS(iMat->normalMap,5,context);
				if(!wireRender) BindTexturePS(iMat->specularMap,6,context);

				DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),context);
				}
				m++;
			}
		}

	}

}
void wiRenderer::DrawWorldTransparent(Camera* camera, ID3D11ShaderResourceView* refracRes, ID3D11ShaderResourceView* refRes
		, ID3D11ShaderResourceView* depth, ID3D11DeviceContext* context){

	if(objects_trans.empty())
		return;

	Frustum frustum = Frustum();
	frustum.ConstructFrustum(camera->zFarP,camera->Projection,camera->View);

	CulledCollection culledwiRenderer;
	CulledList culledObjects;
	if(spTree_trans)
		wiSPTree::getVisible(spTree_trans->root,frustum,culledObjects);

	if(!culledObjects.empty())
	{
		for(Cullable* object : culledObjects)
			culledwiRenderer[((Object*)object)->mesh].insert((Object*)object);

		BindPrimitiveTopology(TRIANGLELIST,context);
		BindVertexLayout(vertexLayout,context);
		BindPS(wireRender?simplestPS:transparentPS,context);
		BindVS(vertexShader10,context);

		BindSamplerPS(texSampler,0,context);
		BindSamplerPS(mapSampler,1,context);
	
		if(!wireRender) BindTexturePS(enviroMap,0,context);
		if(!wireRender) BindTexturePS(refRes,1,context);
		if(!wireRender) BindTexturePS(refracRes,2,context);
		if(!wireRender) BindTexturePS(depth,7,context);


		BindConstantBufferVS(staticCb,0,context);
		BindConstantBufferPS(pixelCB,0,context);
		BindConstantBufferPS(fxCb,1,context);
		BindConstantBufferVS(constantBuffer,3,context);
		BindConstantBufferVS(matCb,2,context);
		BindConstantBufferPS(matCb,2,context);

		BindBlendState(blendState,context);
		BindDepthStencilState(depthStencilState,STENCILREF_TRANSPARENT,context);
		BindRasterizerState(wireRender?wireRS:rasterizerState,context);
	

		for (CulledCollection::iterator iter = culledwiRenderer.begin(); iter != culledwiRenderer.end(); ++iter) {
			Mesh* mesh = iter->first;
			CulledObjectList& visibleInstances = iter->second;



			if(!mesh->doubleSided)
				context->RSSetState(wireRender?wireRS:rasterizerState);
			else
				context->RSSetState(wireRender?wireRS:nonCullRS);

			int matsiz = mesh->materialIndices.size();

				
			
			int k = 0;
			for (CulledObjectList::iterator viter = visibleInstances.begin(); viter != visibleInstances.end(); ++viter){
				if ((*viter)->particleEmitter != Object::wiParticleEmitter::EMITTER_INVISIBLE){
					if (mesh->softBody || (*viter)->armatureDeform)
						mesh->AddRenderableInstance(Instance(XMMatrixIdentity(), (*viter)->transparency, (*viter)->color), k, GRAPHICSTHREAD_MISC1);
					else
						mesh->AddRenderableInstance(Instance(XMMatrixTranspose(XMLoadFloat4x4(&(*viter)->world)), (*viter)->transparency, (*viter)->color), k, GRAPHICSTHREAD_MISC1);
					++k;
				}
			}
			if (k<1)
				continue;

			mesh->UpdateRenderableInstances(visibleInstances.size(), GRAPHICSTHREAD_MISC1, context);

				
			BindIndexBuffer(mesh->meshIndexBuff,context);
			BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),context);
			BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),context);
				

			int m=0;
			for(Material* iMat : mesh->materials){

				if(iMat->transparent && iMat->alpha>0 && !iMat->water && !iMat->isSky){

					
				MaterialCB* mcb = new MaterialCB(*iMat,m);

				UpdateBuffer(matCb,mcb,context);
				_aligned_free(mcb);

				if(!wireRender) BindTexturePS(iMat->texture,3,context);
				if(!wireRender) BindTexturePS(iMat->refMap,4,context);
				if(!wireRender) BindTexturePS(iMat->normalMap,5,context);
				if(!wireRender) BindTexturePS(iMat->specularMap,6,context);
					
				DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),context);
				}
				m++;
			}
		}
	}
	
}


void wiRenderer::DrawSky(ID3D11DeviceContext* context)
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
	if(!decals.empty()){
		Frustum frustum = Frustum();
		frustum.ConstructFrustum(camera->zFarP, camera->Projection, camera->View);

		BindTexturePS(depth,1,context);
		BindSamplerPS(ssClampAni,0,context);
		BindVS(decalVS,context);
		BindPS(decalPS,context);
		BindRasterizerState(backFaceRS,context);
		BindBlendState(blendStateTransparent,context);
		BindDepthStencilState(stencilReadMatch,STENCILREF::STENCILREF_DEFAULT,context);
		BindVertexLayout(nullptr,context);
		BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST,context);
		BindConstantBufferVS(decalCbVS,0,context);
		BindConstantBufferPS(lightStaticCb,0,context);
		BindConstantBufferPS(decalCbPS,1,context);

		for(Decal* decal : decals){

			if((decal->texture || decal->normal) && frustum.CheckBox(decal->bounds.corners)){
				
				BindTexturePS(decal->texture,2,context);
				BindTexturePS(decal->normal,3,context);

				DecalCBVS dcbvs;
				dcbvs.mWVP=
					XMMatrixTranspose(
						XMLoadFloat4x4(&decal->world)
						*camera->GetViewProjection()
					);
				UpdateBuffer(decalCbVS,&dcbvs,context);

				DecalCBPS* dcbps = (DecalCBPS*)_aligned_malloc(sizeof(DecalCBPS),16);
				dcbps->mDecalVP=
					XMMatrixTranspose(
						XMLoadFloat4x4(&decal->view)*XMLoadFloat4x4(&decal->projection)
					);
				dcbps->hasTexNor=0;
				if(decal->texture!=nullptr)
					dcbps->hasTexNor|=0x0000001;
				if(decal->normal!=nullptr)
					dcbps->hasTexNor|=0x0000010;
				XMStoreFloat3(&dcbps->eye,camera->GetEye());
				dcbps->opacity=wiMath::Clamp((decal->life<=-2?1:decal->life<decal->fadeStart?decal->life/decal->fadeStart:1),0,1);
				dcbps->front=decal->front;
				UpdateBuffer(decalCbPS,dcbps,context);
				_aligned_free(dcbps);

				Draw(36,context);

			}

		}

	}
}


void wiRenderer::UpdatePerWorldCB(ID3D11DeviceContext* context){
	PixelCB pcb;
	pcb.mSun=XMVector3Normalize( GetSunPosition() );
	pcb.mHorizon=worldInfo.horizon;
	pcb.mAmbient=worldInfo.ambient;
	pcb.mSunColor=GetSunColor();
	pcb.mFogSEH=worldInfo.fogSEH;
	UpdateBuffer(pixelCB, &pcb, context);
}
void wiRenderer::UpdatePerFrameCB(ID3D11DeviceContext* context){
	ViewPropCB cb;
	cb.mZFarP=cam->zFarP;
	cb.mZNearP=cam->zNearP;
	cb.matView = XMMatrixTranspose( cam->GetView() );
	cb.matProj = XMMatrixTranspose( cam->GetProjection() );
	UpdateBuffer(viewPropCB,&cb,context);

	BindConstantBufferPS(viewPropCB,10,context);
}
void wiRenderer::UpdatePerRenderCB(ID3D11DeviceContext* context, int tessF){
	if(tessF){
		TessBuffer tb;
		tb.g_f4Eye = cam->GetEye();
		tb.g_f4TessFactors = XMFLOAT4A( (float)tessF,2.f,4.f,6.f );
		UpdateBuffer(tessBuf,&tb,context);
	}

}
void wiRenderer::UpdatePerViewCB(ID3D11DeviceContext* context, Camera* camera, Camera* refCamera, const XMFLOAT4& newClipPlane){

	
	StaticCB cb;
	cb.mViewProjection = XMMatrixTranspose(camera->GetViewProjection());
	cb.mRefViewProjection = XMMatrixTranspose( refCamera->GetViewProjection());
	cb.mPrevViewProjection = XMMatrixTranspose(prevFrameCam->GetViewProjection());
	cb.mCamPos = camera->GetEye();
	cb.mClipPlane = newClipPlane;
	cb.mWind=wind.direction;
	cb.time=wind.time;
	cb.windRandomness=wind.randomness;
	cb.windWaveSize=wind.waveSize;
	UpdateBuffer(staticCb,&cb,context);

	SkyBuffer scb;
	scb.mV=XMMatrixTranspose(camera->GetView());
	scb.mP = XMMatrixTranspose(camera->GetProjection());
	scb.mPrevView = XMMatrixTranspose(prevFrameCam->GetView());
	scb.mPrevProjection = XMMatrixTranspose(prevFrameCam->GetProjection());
	UpdateBuffer(skyCb,&scb,context);

	UpdateBuffer(trailCB, &XMMatrixTranspose(camera->GetViewProjection()), context);

	LightStaticCB lcb;
	lcb.mProjInv = XMMatrixInverse(0, XMMatrixTranspose(camera->GetViewProjection()));
	UpdateBuffer(lightStaticCb,&lcb,context);
}
void wiRenderer::UpdatePerEffectCB(ID3D11DeviceContext* context, const XMFLOAT4& blackoutBlackWhiteInvCol, const XMFLOAT4 colorMask){
	FxCB fb;
	fb.mFx = blackoutBlackWhiteInvCol;
	fb.colorMask=colorMask;
	UpdateBuffer(fxCb,&fb,context);
}

void wiRenderer::FinishLoading(){
	everyObject.reserve(objects.size() + objects_trans.size() + objects_water.size());
	everyObject.insert(everyObject.end(), objects.begin(), objects.end());
	everyObject.insert(everyObject.end(), objects_trans.begin(), objects_trans.end());
	everyObject.insert(everyObject.end(), objects_water.begin(), objects_water.end());

	physicsEngine->FirstRunWorld();
	physicsEngine->addWind(wind.direction);
	for (Object* o : everyObject){
		for (wiEmittedParticle* e : o->eParticleSystems){
			emitterSystems[e->name].push_back(e);
			if (e->light != nullptr)
				lights.push_back(e->light);
		}
		if (physicsEngine){
			physicsEngine->registerObject(o);
		}
	}

	SetUpLights();

	Update();
	UpdateRenderInfo(nullptr);
	UpdateLights();
	GenerateSPTree(spTree,vector<Cullable*>(objects.begin(),objects.end()),SPTREE_GENERATE_OCTREE);
	GenerateSPTree(spTree_trans,vector<Cullable*>(objects_trans.begin(),objects_trans.end()),SPTREE_GENERATE_OCTREE);
	GenerateSPTree(spTree_water,vector<Cullable*>(objects_water.begin(),objects_water.end()),SPTREE_GENERATE_OCTREE);
	GenerateSPTree(spTree_lights,vector<Cullable*>(lights.begin(),lights.end()),SPTREE_GENERATE_OCTREE);
	SetUpCubes();
	SetUpBoneLines();


	vector<thread> aoThreads(0);

	for (Object* o : objects)
	{
		//if (o->mesh->renderable && !o->mesh->calculatedAO && o->mesh->usedBy.size() == 1 && !o->isDynamic() && o->particleEmitter != Object::EMITTER_INVISIBLE)
		if (o->mesh->renderable && o->mesh->usedBy.size() <2)
		{
			//aoThreads.push_back(thread(CalculateVertexAO, o));
		}
	}

	for (auto& t : aoThreads)
	{
		t.join();
	}

	for (MeshCollection::iterator iter = meshes.begin(); iter != meshes.end(); ++iter)
	{
		//iter->second->CreateBuffers();
		addVertexCount(iter->second->vertices.size());
	}

}


void wiRenderer::SetUpLights()
{
	for(Light* l:lights){
		if (!l->shadowCam.empty())
			continue;

		if(l->type==Light::DIRECTIONAL){

			float lerp = 0.5f;
			float lerp1 = 0.12f;
			float lerp2 = 0.016f;
			XMVECTOR a0,a,b0,b;
			a0 = XMVector3Unproject(XMVectorSet(0, (float)RENDERHEIGHT, 0, 1), 0, 0, (float)RENDERWIDTH, (float)RENDERHEIGHT, 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
			a = XMVector3Unproject(XMVectorSet(0, (float)RENDERHEIGHT, 1, 1), 0, 0, (float)RENDERWIDTH, (float)RENDERHEIGHT, 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
			b0 = XMVector3Unproject(XMVectorSet((float)RENDERWIDTH, (float)RENDERHEIGHT, 0, 1), 0, 0, (float)RENDERWIDTH, (float)RENDERHEIGHT, 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
			b = XMVector3Unproject(XMVectorSet((float)RENDERWIDTH, (float)RENDERHEIGHT, 1, 1), 0, 0, (float)RENDERWIDTH, (float)RENDERHEIGHT, 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
			float size=XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0,b,lerp),XMVectorLerp(a0,a,lerp))));
			float size1=XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0,b,lerp1),XMVectorLerp(a0,a,lerp1))));
			float size2=XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0,b,lerp2),XMVectorLerp(a0,a,lerp2))));
			XMVECTOR rot = XMQuaternionIdentity();

			l->shadowCam.push_back(SHCAM(size,rot,0,wiRenderer::getCamera()->zFarP));
			l->shadowCam.push_back(SHCAM(size1,rot,0,wiRenderer::getCamera()->zFarP));
			l->shadowCam.push_back(SHCAM(size2,rot,0,wiRenderer::getCamera()->zFarP));

		}
		else if(l->type==Light::SPOT && l->shadow){
			l->shadowCam.push_back( SHCAM(XMFLOAT4(0,0,0,1),wiRenderer::getCamera()->zNearP,l->enerDis.y,l->enerDis.z) );
		}
		else if(l->type==Light::POINT && l->shadow){
			l->shadowCam.push_back( SHCAM(XMFLOAT4(0.5f,-0.5f,-0.5f,-0.5f),wiRenderer::getCamera()->zNearP,l->enerDis.y,XM_PI/2.0f) ); //+x
			l->shadowCam.push_back( SHCAM(XMFLOAT4(0.5f,0.5f,0.5f,-0.5f),wiRenderer::getCamera()->zNearP,l->enerDis.y,XM_PI/2.0f) ); //-x

			l->shadowCam.push_back( SHCAM(XMFLOAT4(1,0,0,-0),wiRenderer::getCamera()->zNearP,l->enerDis.y,XM_PI/2.0f) ); //+y
			l->shadowCam.push_back( SHCAM(XMFLOAT4(0,0,0,-1),wiRenderer::getCamera()->zNearP,l->enerDis.y,XM_PI/2.0f) ); //-y

			l->shadowCam.push_back( SHCAM(XMFLOAT4(0.707f,0,0,-0.707f),wiRenderer::getCamera()->zNearP,l->enerDis.y,XM_PI/2.0f) ); //+z
			l->shadowCam.push_back( SHCAM(XMFLOAT4(0,0.707f,0.707f,0),wiRenderer::getCamera()->zNearP,l->enerDis.y,XM_PI/2.0f) ); //-z
		}
	}

	
}
void wiRenderer::UpdateLights()
{
	if(GetGameSpeed()){
	for(Light* l:lights){
		//Attributes
		//if(l->parent!=nullptr){
		//	XMVECTOR sca,rot,pos;
		//	XMMatrixDecompose(&sca,&rot,&pos,XMLoadFloat4x4(&l->parent->world));
		//	XMStoreFloat3(&l->translation,pos);
		//	XMStoreFloat4(&l->rotation,XMQuaternionMultiply(XMLoadFloat4(&l->rotation_rest),rot));
		//}
		//else{
		//	l->translation=l->translation_rest;
		//	l->rotation=l->rotation_rest;
		//}
		l->getTransform();

		//Shadows
		if(l->type==Light::DIRECTIONAL){

			float lerp = 0.5f;//third slice distance from cam (percentage)
			float lerp1 = 0.12f;//second slice distance from cam (percentage)
			float lerp2 = 0.016f;//first slice distance from cam (percentage)
			XMVECTOR c,d,e,e1,e2;
			c = XMVector3Unproject(XMVectorSet((float)RENDERWIDTH / 2, (float)RENDERHEIGHT / 2, 1, 1), 0, 0, (float)RENDERWIDTH, (float)RENDERHEIGHT, 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
			d = XMVector3Unproject(XMVectorSet((float)RENDERWIDTH / 2, (float)RENDERHEIGHT / 2, 0, 1), 0, 0, (float)RENDERWIDTH, (float)RENDERHEIGHT, 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());

			if (!l->shadowCam.empty()) {

				float f = l->shadowCam[0].size/(float)SHADOWMAPRES;
				e=	XMVectorFloor( XMVectorLerp(d,c,lerp)/f	)*f;
				f = l->shadowCam[1].size/(float)SHADOWMAPRES;
				e1=	XMVectorFloor( XMVectorLerp(d,c,lerp1)/f)*f;
				f = l->shadowCam[2].size/(float)SHADOWMAPRES;
				e2=	XMVectorFloor( XMVectorLerp(d,c,lerp2)/f)*f;

				//static float rot=0.0f;
				////rot+=0.001f;
				//XMStoreFloat4(&l->rotation,XMQuaternionMultiply(XMLoadFloat4(&l->rotation_rest),XMQuaternionRotationAxis(XMVectorSet(1,0,0,0),rot)));


				XMMATRIX rrr = XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation_rest));
				l->shadowCam[0].Update(rrr*XMMatrixTranslationFromVector(e));
				if(l->shadowCam.size()>1){
					l->shadowCam[1].Update(rrr*XMMatrixTranslationFromVector(e1));
					if(l->shadowCam.size()>2)
						l->shadowCam[2].Update(rrr*XMMatrixTranslationFromVector(e2));
				}
			}
			
			l->bounds.createFromHalfWidth(wiRenderer::getCamera()->translation,XMFLOAT3(10000,10000,10000));
		}
		else if(l->type==Light::SPOT){
			if(!l->shadowCam.empty()){
				l->shadowCam[0].Update( XMLoadFloat4x4(&l->world) );
			}
			//l->shadowCam[0].Update( XMMatrixRotationQuaternion(XMLoadFloat4(&l->frameRot))*XMMatrixTranslationFromVector(XMLoadFloat3(&l->framePos)) );
			//l->bounds=l->mesh->aabb.get( 
			//	XMMatrixScaling(l->enerDis.y,l->enerDis.y,l->enerDis.y)
			//	*XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))
			//	*XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation)) 
			//	);

			l->bounds.createFromHalfWidth(l->translation,XMFLOAT3(l->enerDis.y,l->enerDis.y,l->enerDis.y));
		}
		else if(l->type==Light::POINT){
			for(unsigned int i=0;i<l->shadowCam.size();++i){
				l->shadowCam[i].Update(XMLoadFloat3(&l->translation));
			}
			//l->bounds=l->mesh->aabb.get( 
			//	XMMatrixScaling(l->enerDis.y,l->enerDis.y,l->enerDis.y)
			//	*XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))
			//	*XMMatrixTranslationFromVector(XMLoadFloat3(&l->translation)) 
			//	);

			l->bounds.createFromHalfWidth(l->translation,XMFLOAT3(l->enerDis.y,l->enerDis.y,l->enerDis.y));
		}
	}

	
	UpdateSPTree(spTree_lights);
		

	}
}

wiRenderer::Picked wiRenderer::Pick(RAY& ray, PICKTYPE pickType)
{
	CulledCollection culledwiRenderer;
	CulledList culledObjects;
	wiSPTree* searchTree = nullptr;
	switch (pickType)
	{
	case wiRenderer::PICK_OPAQUE:
		searchTree = spTree;
		break;
	case wiRenderer::PICK_TRANSPARENT:
		searchTree = spTree_trans;
		break;
	case wiRenderer::PICK_WATER:
		searchTree = spTree_water;
		break;
	default:
		break;
	}
	if (searchTree)
	{
		wiSPTree::getVisible(searchTree->root, ray, culledObjects);

		vector<Picked> pickPoints;

		RayIntersectMeshes(ray, culledObjects, pickPoints);

		if (!pickPoints.empty()){
			Picked min = pickPoints.front();
			float mini = wiMath::DistanceSquared(min.position, ray.origin);
			for (unsigned int i = 1; i<pickPoints.size(); ++i){
				if (float nm = wiMath::DistanceSquared(pickPoints[i].position, ray.origin)<mini){
					min = pickPoints[i];
					mini = nm;
				}
			}
			return min;
		}

	}

	return Picked();
}
wiRenderer::Picked wiRenderer::Pick(long cursorX, long cursorY, PICKTYPE pickType)
{
	RAY ray = getPickRay(cursorX, cursorY);

	return Pick(ray, pickType);
}

RAY wiRenderer::getPickRay(long cursorX, long cursorY){
	XMVECTOR& lineStart = XMVector3Unproject(XMVectorSet((float)cursorX,(float)cursorY,0,1),0,0
		, (float)SCREENWIDTH, (float)SCREENHEIGHT, 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
	XMVECTOR& lineEnd = XMVector3Unproject(XMVectorSet((float)cursorX, (float)cursorY, 1, 1), 0, 0
		, (float)SCREENWIDTH, (float)SCREENHEIGHT, 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
	XMVECTOR& rayDirection = XMVector3Normalize(XMVectorSubtract(lineEnd,lineStart));
	return RAY(lineStart,rayDirection);
}

void wiRenderer::RayIntersectMeshes(const RAY& ray, const CulledList& culledObjects, vector<Picked>& points, bool dynamicObjects)
{
	if (!culledObjects.empty())
	{
		XMVECTOR& rayOrigin = XMLoadFloat3(&ray.origin);
		XMVECTOR& rayDirection = XMLoadFloat3(&ray.direction);

		for (Cullable* culled : culledObjects){
			Object* object = (Object*)culled;
			if (!dynamicObjects && object->isDynamic())
			{
				continue;
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
					points.push_back(picked);
				}
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

	XMMATRIX& objectMat = object->getTransform();

	CulledCollection culledwiRenderer;
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

		Vertex v = TransformVertex(mesh, vert, object->getTransform());
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

			RayIntersectMeshes(ray, culledObjects, points, false);


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

void wiRenderer::LoadModel(const string& dir, const string& name, const XMMATRIX& transform, const string& ident, PHYSICS* physicsEngine){
	static int unique_identifier = 0;

	vector<Object*> newObjects(0),newObjects_trans(0),newObjects_water(0);
	vector<Light*> newLights(0);
	stringstream idss("");
	idss<<"_"/*<<unique_identifier<<"_"*/<<ident;

	LoadFromDisk(dir,name,idss.str(),armatures,materials,newObjects,newObjects_trans,newObjects_water
		,meshes,newLights,vector<HitSphere*>(),worldInfo,wind,vector<Camera>()
		,vector<Armature*>(),everyObject,transforms,decals);


	for(Object* o:newObjects){
		o->transform(transform);
		o->bounds=o->bounds.get(transform);
		o->getTransform();
		if(physicsEngine!=nullptr){
			physicsEngine->registerObject(o);
		}
	}
	for(Object* o:newObjects_trans){
		o->transform(transform);
		o->bounds=o->bounds.get(transform);
		o->getTransform();
		if(physicsEngine!=nullptr){
			physicsEngine->registerObject(o);
		}
	}
	for(Object* o:newObjects_water){
		o->transform(transform);
		o->bounds=o->bounds.get(transform);
		o->getTransform();
		if(physicsEngine!=nullptr){
			physicsEngine->registerObject(o);
		}
	}
	//for(Light* l:newLights){
	//	l->transform(transform);
	//	l->bounds=l->bounds.get(transform);
	//	l->getTransform();
	//}
	
	graphicsMutex.lock();

	if(spTree){
		spTree->AddObjects(spTree->root,vector<Cullable*>(newObjects.begin(),newObjects.end()));
	}
	if(spTree_trans){
		spTree_trans->AddObjects(spTree_trans->root,vector<Cullable*>(newObjects_trans.begin(),newObjects_trans.end()));
	}
	if(spTree_water){
		spTree_water->AddObjects(spTree_water->root,vector<Cullable*>(newObjects_water.begin(),newObjects_water.end()));
	}
	if(spTree_lights){
		spTree_lights->AddObjects(spTree_lights->root,vector<Cullable*>(newLights.begin(),newLights.end()));
	}
	
	objects.insert(objects.end(),newObjects.begin(),newObjects.end());
	objects_trans.insert(objects_trans.end(),newObjects_trans.begin(),newObjects_trans.end());
	objects_water.insert(objects_water.end(),newObjects_water.begin(),newObjects_water.end());

	UpdateLights();
	lights.insert(lights.end(),newLights.begin(),newLights.end());

	graphicsMutex.unlock();

	unique_identifier++;
}
void wiRenderer::LoadWorldInfo(const string& dir, const string& name)
{
	LoadWiWorldInfo(dir, name, worldInfo, wind);
}

void wiRenderer::SychronizeWithPhysicsEngine()
{
	if (physicsEngine && GetGameSpeed()){

		UpdateSoftBodyPinning();
		for (Object* object : everyObject){
			int pI = object->physicsObjectI;
			if (pI >= 0){
				if (object->mesh->softBody){
					physicsEngine->connectSoftBodyToVertices(
						object->mesh, pI
						);
				}
				if (object->kinematic){
					XMVECTOR s, r, t;
					XMMatrixDecompose(&s, &r, &t, XMLoadFloat4x4(&object->world));
					XMFLOAT3 T;
					XMFLOAT4 R;
					XMStoreFloat4(&R, r);
					XMStoreFloat3(&T, t);
					physicsEngine->transformBody(R, T, pI);
				}
			}
		}

		physicsEngine->Update();

		for (Object* object : everyObject){
			int pI = object->physicsObjectI;
			if (pI >= 0 && !object->kinematic){
				PHYSICS::Transform* transform(physicsEngine->getObject(pI));
				object->translation_rest = transform->position;
				object->rotation_rest = transform->rotation;

				if (object->mesh->softBody){
					object->scale_rest = XMFLOAT3(1, 1, 1);
					physicsEngine->connectVerticesToSoftBody(
						object->mesh, pI
						);
				}
			}
		}


		physicsEngine->NextRunWorld();
	}
}

wiRenderer::MaterialCB::MaterialCB(const Material& mat,UINT materialIndex){
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