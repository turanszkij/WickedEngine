#include "Renderer.h"
#include "skinningDEF.h"

#pragma region STATICS
D3D_DRIVER_TYPE						Renderer::driverType;
D3D_FEATURE_LEVEL					Renderer::featureLevel;
Renderer::SwapChain					Renderer::swapChain;
Renderer::RenderTargetView			Renderer::renderTargetView;
Renderer::ViewPort					Renderer::viewPort;
Renderer::GraphicsDevice			Renderer::graphicsDevice;
Renderer::DeviceContext				Renderer::immediateContext;
bool Renderer::DX11 = false,Renderer::VSYNC=true;
Renderer::DeviceContext				Renderer::deferredContexts[];
Renderer::CommandList				Renderer::commandLists[];
mutex								Renderer::graphicsMutex;
Renderer::Sampler Renderer::ssClampLin,Renderer::ssClampPoi,Renderer::ssMirrorLin,Renderer::ssMirrorPoi,Renderer::ssWrapLin,Renderer::ssWrapPoi
		,Renderer::ssClampAni,Renderer::ssWrapAni,Renderer::ssMirrorAni,Renderer::ssComp;

map<Renderer::DeviceContext,long> Renderer::drawCalls;

int Renderer::RENDERWIDTH,Renderer::RENDERHEIGHT,Renderer::SCREENWIDTH,Renderer::SCREENHEIGHT,Renderer::SHADOWMAPRES,Renderer::SOFTSHADOW
	,Renderer::POINTLIGHTSHADOW=1,Renderer::POINTLIGHTSHADOWRES=256;
bool Renderer::HAIRPARTICLEENABLED,Renderer::EMITTERSENABLED;
bool Renderer::wireRender,Renderer::debugSpheres,Renderer::debugLines,Renderer::debugBoxes;
ID3D11BlendState		*Renderer::blendState, *Renderer::blendStateTransparent, *Renderer::blendStateAdd;
ID3D11Buffer			*Renderer::constantBuffer, *Renderer::staticCb, *Renderer::shCb, *Renderer::pixelCB, *Renderer::matCb
	, *Renderer::lightCb[3], *Renderer::tessBuf, *Renderer::lineBuffer, *Renderer::skyCb
	, *Renderer::trailCB, *Renderer::lightStaticCb, *Renderer::vLightCb,*Renderer::cubeShCb,*Renderer::fxCb
	, *Renderer::decalCbVS, *Renderer::decalCbPS, *Renderer::viewPropCB;
ID3D11VertexShader		*Renderer::vertexShader10, *Renderer::vertexShader, *Renderer::shVS, *Renderer::lineVS, *Renderer::trailVS
	,*Renderer::waterVS,*Renderer::lightVS[3],*Renderer::vSpotLightVS,*Renderer::vPointLightVS,*Renderer::cubeShVS,*Renderer::sOVS,*Renderer::decalVS;
ID3D11PixelShader		*Renderer::pixelShader, *Renderer::shPS, *Renderer::linePS, *Renderer::trailPS, *Renderer::simplestPS,*Renderer::blackoutPS
	,*Renderer::textureonlyPS,*Renderer::waterPS,*Renderer::transparentPS,*Renderer::lightPS[3],*Renderer::vLightPS,*Renderer::cubeShPS
	,*Renderer::decalPS;
ID3D11GeometryShader *Renderer::cubeShGS,*Renderer::sOGS;
ID3D11HullShader		*Renderer::hullShader;
ID3D11DomainShader		*Renderer::domainShader;
ID3D11InputLayout		*Renderer::vertexLayout, *Renderer::lineIL,*Renderer::trailIL, *Renderer::sOIL;
ID3D11SamplerState		*Renderer::texSampler, *Renderer::mapSampler, *Renderer::comparisonSampler, *Renderer::mirSampler,*Renderer::pointSampler;
ID3D11RasterizerState	*Renderer::rasterizerState, *Renderer::rssh, *Renderer::nonCullRSsh, *Renderer::wireRS, *Renderer::nonCullRS,*Renderer::nonCullWireRS
	,*Renderer::backFaceRS;
ID3D11DepthStencilState	*Renderer::depthStencilState,*Renderer::xRayStencilState,*Renderer::depthReadStencilState,*Renderer::stencilReadState
	,*Renderer::stencilReadMatch;
ID3D11PixelShader		*Renderer::skyPS;
ID3D11VertexShader		*Renderer::skyVS;
ID3D11SamplerState		*Renderer::skySampler;
Renderer::TextureView Renderer::noiseTex,Renderer::trailDistortTex,Renderer::enviroMap,Renderer::colorGrading;
float Renderer::GameSpeed=1,Renderer::overrideGameSpeed=1;
int Renderer::visibleCount;
float Renderer::shBias;
RenderTarget Renderer::normalMapRT,Renderer::imagesRT,Renderer::imagesRTAdd;
Camera *Renderer::cam;

vector<Renderer::TextureView> Renderer::textureSlotsPS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
Renderer::PixelShader Renderer::boundPS=nullptr;
#pragma endregion

#pragma region STATIC TEMP

int Renderer::vertexCount;
deque<oImage*> Renderer::images;
deque<oImage*> Renderer::waterRipples;

SPTree* Renderer::spTree;
SPTree* Renderer::spTree_trans;
SPTree* Renderer::spTree_water;
SPTree* Renderer::spTree_lights;

vector<Object*>		Renderer::everyObject;
vector<Object*>		Renderer::objects;
vector<Object*>		Renderer::objects_trans;
vector<Object*>		Renderer::objects_water;	
MeshCollection		Renderer::meshes;
MaterialCollection	Renderer::materials;
vector<Armature*>	Renderer::armatures;
vector<Lines>		Renderer::boneLines;
vector<Cube>		Renderer::cubes;
vector<Light*>		Renderer::lights;
map<string,vector<EmittedParticle*>> Renderer::emitterSystems;
map<string,Transform*> Renderer::transforms;
list<Decal*> Renderer::decals;

#pragma endregion

Renderer::Renderer()
{
}


HRESULT Renderer::InitDevice(HWND window, int screenW, int screenH, bool windowed, short& requestMultiThreading)
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
	//sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	
	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
											D3D11_SDK_VERSION, &sd, &swapChain, &Renderer::graphicsDevice, &featureLevel, &Renderer::immediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
	if(FAILED(hr)){
        MessageBox(window,L"SwapChain Creation Failed!",0,0);
#ifdef BACKLOG
			BackLog::post("SwapChain Creation Failed!");
#endif
		exit(0);
	}
	DX11 = ( ( Renderer::graphicsDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ) ? true:false );
	drawCalls.insert(pair<DeviceContext,long>(immediateContext,0));

	if(requestMultiThreading){
		D3D11_FEATURE_DATA_THREADING threadingFeature;
		Renderer::graphicsDevice->CheckFeatureSupport( D3D11_FEATURE_THREADING,&threadingFeature,sizeof(threadingFeature) );
		if(threadingFeature.DriverConcurrentCreates && threadingFeature.DriverCommandLists){
			for(int i=0;i<NUM_DCONTEXT;i++){
				Renderer::graphicsDevice->CreateDeferredContext( 0,&deferredContexts[i] );
				drawCalls.insert(pair<DeviceContext,long>(deferredContexts[i],0));
			}
#ifdef BACKLOG
			stringstream ss("");
			ss<<NUM_DCONTEXT<<" defferred contexts created!";
			BackLog::post(ss.str().c_str());
#endif
		}
		else {
			//MessageBox(window,L"Deferred Context not supported!",L"Error!",0);
#ifdef BACKLOG
			BackLog::post("Deferred context not supported!");
#endif
			requestMultiThreading=0;
			//exit(0);
		}
	}
	

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) ){
        MessageBox(window,L"BackBuffer Failed!",0,0);
		exit(0);
	}

    hr = Renderer::graphicsDevice->CreateRenderTargetView( pBackBuffer, NULL, &renderTargetView );
    //pBackBuffer->Release();
    if( FAILED( hr ) ){
        MessageBox(window,L"Main Rendertarget Failed!",0,0);
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
void Renderer::DestroyDevice()
{
	
	if( renderTargetView ) 
		renderTargetView->Release();
    if( swapChain ) 
		swapChain->Release();
	if( Renderer::immediateContext ) 
		Renderer::immediateContext->ClearState();
    if( Renderer::immediateContext ) 
		Renderer::immediateContext->Release();
    if( graphicsDevice ) 
		graphicsDevice->Release();

	for(int i=0;i<NUM_DCONTEXT;i++){
		if(commandLists[i]) {commandLists[i]->Release(); commandLists[i]=0;}
		if(deferredContexts[i]) {deferredContexts[i]->Release(); deferredContexts[i]=0;}
	}
}
void Renderer::Present(function<void()> drawToScreen1,function<void()> drawToScreen2,function<void()> drawToScreen3)
{
	Renderer::graphicsMutex.lock();

	Renderer::immediateContext->RSSetViewports( 1, &viewPort );
	Renderer::immediateContext->OMSetRenderTargets( 1, &renderTargetView, 0 );
	float ClearColor[4] = { 0, 0, 0, 1.0f }; // red,green,blue,alpha
    Renderer::immediateContext->ClearRenderTargetView( renderTargetView, ClearColor );
	//Renderer::immediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	
	if(drawToScreen1!=nullptr)
		drawToScreen1();
	if(drawToScreen2!=nullptr)
		drawToScreen2();
	if(drawToScreen3!=nullptr)
		drawToScreen3();
	


	//FrameRate::Count();
	FrameRate::Frame();

	swapChain->Present( VSYNC, 0 );
	for(auto& d : drawCalls){
		d.second=0;
	}


	Renderer::immediateContext->OMSetRenderTargets(NULL, NULL,NULL);

	Renderer::graphicsMutex.unlock();
}
void Renderer::ReleaseCommandLists()
{
	for(int i=0;i<NUM_DCONTEXT;i++)
		if(commandLists[i]){ commandLists[i]->Release(); commandLists[i]=NULL; }
}
void Renderer::ExecuteDeferredContexts()
{
	for(int i=0;i<NUM_DCONTEXT;i++)
		if(commandLists[i]) Renderer::immediateContext->ExecuteCommandList( commandLists[i], false );

	ReleaseCommandLists();
}
void Renderer::FinishCommandList(int THREAD)
{
	if(commandLists[THREAD]) {
		commandLists[THREAD]->Release();
		commandLists[THREAD]=0;
	}
	deferredContexts[THREAD]->FinishCommandList( false,&commandLists[THREAD] );
}

long Renderer::getDrawCallCount(){
	long retVal=0;
	for(auto d:drawCalls){
		retVal+=d.second;
	}
	return retVal;
}

void Renderer::CleanUp()
{
	for(int i=0;i<spheres.size();i++)
		delete spheres[i];
	spheres.clear();
}

void Renderer::SetUpStaticComponents()
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

	cam = new Camera(SCREENWIDTH, SCREENHEIGHT, 0.1f, 800, XMVectorSet(0, 0, 0, 1));
	
	noiseTex = (ID3D11ShaderResourceView*)ResourceManager::add("images/noise.png");
	trailDistortTex = (ID3D11ShaderResourceView*)ResourceManager::add("images/normalmap1.jpg");

	wireRender=false;
	debugSpheres=false;
	
	
	thread sub1(Renderer::LoadBasicShaders);
	thread sub2(Renderer::LoadShadowShaders);
	thread sub3(Renderer::LoadSkyShaders);
	thread sub4(Renderer::LoadLineShaders);
	thread sub5(Renderer::LoadTrailShaders);
	thread sub6(Renderer::LoadWaterShaders);
	Renderer::LoadTessShaders();

	Renderer::SetUpStates();
	Renderer::LoadBuffers();

	
	sub2.join();
	sub3.join();
	sub4.join();
	sub5.join();
	sub6.join();
	sub1.join();

	sub1.~thread();
	sub2.~thread();
	sub3.~thread();
	sub4.~thread();
	sub5.~thread();
	sub6.~thread();
	
	HairParticle::SetUpStatic();
	EmittedParticle::SetUpStatic();

	GameSpeed=1;
	shBias = 9.99995464e-005;

	resetVertexCount();
	resetVisibleObjectCount();

	//vector<Armature*>a(0);
	//MaterialCollection m;
	//LoadWiMeshes("lights/","lights.wi","",lightGRenderer,a,m);

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
}
void Renderer::CleanUpStatic()
{
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

	HairParticle::CleanUpStatic();
	EmittedParticle::CleanUpStatic();
	Cube::CleanUpStatic();
	HitSphere::CleanUpStatic();


	//ResourceManager::del("images/noise.png");
	//ResourceManager::del("images/normalmap1.jpg");
	ResourceManager::del("images/clipbox.png");
}
void Renderer::CleanUpStaticTemp(){
	
	Renderer::resetVertexCount();
	
	for(int i=0;i<images.size();i++)
		images[i]->CleanUp();
	images.clear();
	for(int i=0;i<waterRipples.size();i++)
		waterRipples[i]->CleanUp();
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
	
	for(int i=0;i<objects.size();i++){
		objects[i]->CleanUp();
		for(int j=0;j<objects[i]->eParticleSystems.size();++j)
			objects[i]->eParticleSystems[j]->CleanUp();
		objects[i]->eParticleSystems.clear();

		for(int j=0;j<objects[i]->hParticleSystems.size();++j)
			objects[i]->hParticleSystems[j]->CleanUp();
		objects[i]->hParticleSystems.clear();
	}
	objects.clear();
	
	for(int i=0;i<objects_trans.size();i++){
		objects_trans[i]->CleanUp();
		for(int j=0;j<objects_trans[i]->eParticleSystems.size();++j)
			objects_trans[i]->eParticleSystems[j]->CleanUp();
		objects_trans[i]->eParticleSystems.clear();

		for(int j=0;j<objects_trans[i]->hParticleSystems.size();++j)
			objects_trans[i]->hParticleSystems[j]->CleanUp();
		objects_trans[i]->hParticleSystems.clear();
	}
	objects_trans.clear();
	
	for(int i=0;i<objects_water.size();i++){
		objects_water[i]->CleanUp();
		for(int j=0;j<objects_water[i]->eParticleSystems.size();++j)
			objects_water[i]->eParticleSystems[j]->CleanUp();
		objects_water[i]->eParticleSystems.clear();

		for(int j=0;j<objects_water[i]->hParticleSystems.size();++j)
			objects_water[i]->hParticleSystems[j]->CleanUp();
		objects_water[i]->hParticleSystems.clear();
	}
	objects_water.clear();
	everyObject.clear();

	for(MaterialCollection::iterator iter=materials.begin(); iter!=materials.end();++iter)
		iter->second->CleanUp();
	materials.clear();


	for(int i=0;i<boneLines.size();i++)
		boneLines[i].CleanUp();
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
XMVECTOR Renderer::GetSunPosition()
{
	for(Light* l : lights)
		if(l->type==Light::DIRECTIONAL)
			return -XMVector3Transform( XMVectorSet(0,-1,0,1), XMMatrixRotationQuaternion( XMLoadFloat4(&l->rotation) ) );
	return XMVectorSet(0.5f,1,-0.5f,0);
}
XMFLOAT4 Renderer::GetSunColor()
{
	for(Light* l : lights)
		if(l->type==Light::DIRECTIONAL)
			return l->color;
	return XMFLOAT4(1,1,1,1);
}
float Renderer::GetGameSpeed(){return GameSpeed*overrideGameSpeed;}

void Renderer::UpdateSpheres()
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
float Renderer::getSphereRadius(const int& index){
	return spheres[index]->radius;
}
void Renderer::SetUpBoneLines()
{
	boneLines.resize(0);
	for(int i=0;i<armatures.size();i++){
		for(int j=0;j<armatures[i]->boneCollection.size();j++){
			boneLines.push_back( Lines(armatures[i]->boneCollection[j]->length,XMFLOAT4A(1,1,1,1),i,j) );
		}
	}
}
void Renderer::UpdateBoneLines()
{
	if(debugLines)
		for(int i=0;i<boneLines.size();i++){
			int armatureI=boneLines[i].parentArmature;
			int boneI=boneLines[i].parentBone;

			//XMMATRIX rest = XMLoadFloat4x4(&armatures[armatureI]->boneCollection[boneI]->recursiveRest) ;
			//XMMATRIX pose = XMLoadFloat4x4(&armatures[armatureI]->boneCollection[boneI]->world) ;
			//XMFLOAT4X4 finalM;
			//XMStoreFloat4x4( &finalM,  /*rest**/pose );

			boneLines[i].Transform(armatures[armatureI]->boneCollection[boneI]->world);
		}
}
void iterateSPTree2(SPTree::Node* n, vector<Cube>& cubes, const XMFLOAT4A& col);
void iterateSPTree(SPTree::Node* n, vector<Cube>& cubes, const XMFLOAT4A& col){
	if(!n) return;
	if(n->count){
		for(int i=0;i<n->children.size();++i)
			iterateSPTree(n->children[i],cubes,col);
	}
	if(!n->objects.empty()){
		cubes.push_back(Cube(n->box.getCenter(),n->box.getHalfWidth(),col));
		for(Cullable* object:n->objects){
			cubes.push_back(Cube(object->bounds.getCenter(),object->bounds.getHalfWidth(),XMFLOAT4A(1,0,0,1)));
			//Object* o = (Object*)object;
			//for(HairParticle& hps : o->hParticleSystems)
			//	iterateSPTree2(hps.spTree->root,cubes,XMFLOAT4A(0,1,0,1));
		}
	}
}
void iterateSPTree2(SPTree::Node* n, vector<Cube>& cubes, const XMFLOAT4A& col){
	if(!n) return;
	if(n->count){
		for(int i=0;i<n->children.size();++i)
			iterateSPTree2(n->children[i],cubes,col);
	}
	if(!n->objects.empty()){
		cubes.push_back(Cube(n->box.getCenter(),n->box.getHalfWidth(),col));
	}
}
void Renderer::SetUpCubes(){
	/*if(debugBoxes){
		cubes.resize(0);
		iterateSPTree(spTree->root,cubes);
		for(Object* object:objects)
			cubes.push_back(Cube(XMFLOAT3(0,0,0),XMFLOAT3(1,1,1),XMFLOAT4A(1,0,0,1)));
	}*/
	cubes.clear();
}
void Renderer::UpdateCubes(){
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
void Renderer::UpdateSPTree(SPTree*& tree){
	if(tree && tree->root){
		SPTree* newTree = tree->updateTree(tree->root);
		if(newTree){
			tree->CleanUp();
			tree=newTree;
		}
	}
}

void Renderer::LoadBuffers()
{
    D3D11_BUFFER_DESC bd;
	// Create the constant buffer
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(StaticCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &staticCb );

	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(PixelCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &pixelCB );


	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ForShadowMapCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &shCb );
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(CubeShadowCb);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &cubeShCb );

	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(MaterialCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &matCb );

	

	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(TessBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &tessBuf );

	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(LineBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &lineBuffer );

	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(XMMATRIX);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &trailCB );
	
	//
	//ZeroMemory( &bd, sizeof(bd) );
	//bd.Usage = D3D11_USAGE_DYNAMIC;
	//bd.ByteWidth = sizeof(MatIndexBuf);
	//bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	//bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &matIndexBuf );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(LightStaticCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &lightStaticCb );

	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(dLightBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, 0, &lightCb[0] );
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(pLightBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, 0, &lightCb[1] );
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(sLightBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, 0, &lightCb[2] );

	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(vLightBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, 0, &vLightCb );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(FxCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, 0, &fxCb );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(SkyBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, 0, &skyCb );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(DecalCBVS);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, 0, &decalCbVS );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(DecalCBPS);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, 0, &decalCbPS );
	
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ViewPropCB);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, 0, &viewPropCB );

}

void Renderer::LoadBasicShaders()
{
	
    ID3DBlob* pVSBlob = NULL;
	if(FAILED(D3DReadFileToBlob(L"shaders/effectVS10.cso", &pVSBlob))){MessageBox(0,L"Failed To load effectVS10.cso",0,0);}
	else Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vertexShader10 );
	
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//#ifdef USE_GPU_SKINNING
//			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//			{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//#endif
			
			{ "MATI", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "MATI", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		};
		UINT numElements = ARRAYSIZE( layout );

		Renderer::graphicsDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
											  pVSBlob->GetBufferSize(), &vertexLayout );


	if(pVSBlob){ pVSBlob->Release(); pVSBlob=NULL;}
	
	if(FAILED(D3DReadFileToBlob(L"shaders/sOVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load sOVS.cso",0,0);}
	else Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &sOVS );
	D3D11_INPUT_ELEMENT_DESC oslayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	numElements = ARRAYSIZE( oslayout );

	Renderer::graphicsDevice->CreateInputLayout( oslayout, numElements, pVSBlob->GetBufferPointer(),
											pVSBlob->GetBufferSize(), &sOIL );
	if(pVSBlob){ pVSBlob->Release(); pVSBlob=NULL;}

	
	if(FAILED(D3DReadFileToBlob(L"shaders/sOGS.cso", &pVSBlob))){MessageBox(0,L"Failed To load sOGS.cso",0,0);}
	else Renderer::graphicsDevice->CreateGeometryShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &sOGS );
	D3D11_SO_DECLARATION_ENTRY pDecl[] =
	{
		// semantic name, semantic index, start component, component count, output slot
		{ 0, "SV_POSITION", 0, 0, 4, 0 },   // output all components of position
		{ 0, "NORMAL", 0, 0, 3, 0 },     // output the first 3 of the normal
		{ 0, "TEXCOORD", 0, 0, 4, 0 },     // output the first 2 texture coordinates
		{ 0, "TEXCOORD", 1, 0, 4, 0 },     // output the first 2 texture coordinates
	};
	HRESULT hr = Renderer::graphicsDevice->CreateGeometryShaderWithStreamOutput( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), pDecl, 
		4, NULL, 0, sOGS?0:D3D11_SO_NO_RASTERIZED_STREAM, NULL, &sOGS );
	if(pVSBlob){ pVSBlob->Release(); pVSBlob=NULL;}

	
	if(FAILED(D3DReadFileToBlob(L"shaders/effectVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load effectVS.cso",0,0);}
	else Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vertexShader );
	if(pVSBlob){ pVSBlob->Release(); pVSBlob=NULL;}

	if(FAILED(D3DReadFileToBlob(L"shaders/dirLightVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load dirlightVS.cso",0,0);}
	else Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &lightVS[0] );
	if(pVSBlob){ pVSBlob->Release(); pVSBlob=NULL;}

	if(FAILED(D3DReadFileToBlob(L"shaders/pointLightVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load pointlightVS.cso",0,0);}
	else Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &lightVS[1] );
	if(pVSBlob){ pVSBlob->Release(); pVSBlob=NULL;}

	if(FAILED(D3DReadFileToBlob(L"shaders/spotLightVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load spotlightVS.cso",0,0);}
	else Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &lightVS[2] );
	if(pVSBlob){ pVSBlob->Release(); pVSBlob=NULL;}

	if(FAILED(D3DReadFileToBlob(L"shaders/vSpotLightVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load vSpotLightVS.cso",0,0);}
	else Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vSpotLightVS );
	if(pVSBlob){ pVSBlob->Release(); pVSBlob=NULL;}

	if(FAILED(D3DReadFileToBlob(L"shaders/vPointLightVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load vPointLightVS.cso",0,0);}
	else Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vPointLightVS );
	if(pVSBlob){ pVSBlob->Release(); pVSBlob=NULL;}

	if(FAILED(D3DReadFileToBlob(L"shaders/decalVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load decalVS.cso",0,0);}
	else Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &decalVS );
	if(pVSBlob){ pVSBlob->Release(); pVSBlob=NULL;}

	
	ID3DBlob* pPSBlob = NULL;
	if(FAILED(D3DReadFileToBlob(L"shaders/effectPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load effectPS.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &pixelShader );
	if(pPSBlob) { pPSBlob->Release(); pPSBlob=NULL;}
	
	if(FAILED(D3DReadFileToBlob(L"shaders/effectPS_transparent.cso", &pPSBlob))){MessageBox(0,L"Failed To load effectPS_transparent.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &transparentPS );
	if(pPSBlob) { pPSBlob->Release(); pPSBlob=NULL;}
	
	if(FAILED(D3DReadFileToBlob(L"shaders/effectPS_simplest.cso", &pPSBlob))){MessageBox(0,L"Failed To load effectPS_simplest.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &simplestPS );
	if(pPSBlob) { pPSBlob->Release(); pPSBlob=NULL;}
	
	if(FAILED(D3DReadFileToBlob(L"shaders/effectPS_blackout.cso", &pPSBlob))){MessageBox(0,L"Failed To load effectPS_blackout.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &blackoutPS );
	if(pPSBlob) { pPSBlob->Release(); pPSBlob=NULL;}
	
	if(FAILED(D3DReadFileToBlob(L"shaders/effectPS_textureonly.cso", &pPSBlob))){MessageBox(0,L"Failed To load effectPS_textureonly.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &textureonlyPS );
	if(pPSBlob) { pPSBlob->Release(); pPSBlob=NULL;}
	
	if(FAILED(D3DReadFileToBlob(L"shaders/dirLightPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load dirLightPS.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &lightPS[0] );
	if(pPSBlob) { pPSBlob->Release(); pPSBlob=NULL;}
	
	if(FAILED(D3DReadFileToBlob(L"shaders/pointLightPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load pointLightPS.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &lightPS[1] );
	if(pPSBlob) { pPSBlob->Release(); pPSBlob=NULL;}
	
	if(FAILED(D3DReadFileToBlob(L"shaders/spotLightPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load spotLightPS.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &lightPS[2] );
	if(pPSBlob) { pPSBlob->Release(); pPSBlob=NULL;}
	
	if(FAILED(D3DReadFileToBlob(L"shaders/volumeLightPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load volumeLightPS.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &vLightPS );
	if(pPSBlob) { pPSBlob->Release(); pPSBlob=NULL;}
	
	if(FAILED(D3DReadFileToBlob(L"shaders/decalPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load decalPS.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &decalPS );
	if(pPSBlob) { pPSBlob->Release(); pPSBlob=NULL;}
}
void Renderer::LoadLineShaders()
{
	ID3DBlob* pSVSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/linesVS.cso", &pSVSBlob))){MessageBox(0,L"Failed To load linesVS.cso",0,0);}
	else {
		Renderer::graphicsDevice->CreateVertexShader( pSVSBlob->GetBufferPointer(), pSVSBlob->GetBufferSize(), NULL, &lineVS );
	
	


		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE( layout );

		Renderer::graphicsDevice->CreateInputLayout( layout, numElements, pSVSBlob->GetBufferPointer(),
											  pSVSBlob->GetBufferSize(), &lineIL );
	}

	if(pSVSBlob){ pSVSBlob->Release(); pSVSBlob=NULL; }
	
	ID3DBlob* pSPSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/linesPS.cso", &pSPSBlob))){MessageBox(0,L"Failed To load linesPS.cso",0,0);}
	else Renderer::graphicsDevice->CreatePixelShader( pSPSBlob->GetBufferPointer(), pSPSBlob->GetBufferSize(), NULL, &linePS );

	if(pSPSBlob){ pSPSBlob->Release();pSPSBlob=NULL; }
}
void Renderer::LoadTessShaders()
{
	ID3DBlob* pHSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/effectHS.cso", &pHSBlob))){MessageBox(0,L"Failed To load effectHS.cso",0,0);}
	else Renderer::graphicsDevice->CreateHullShader( pHSBlob->GetBufferPointer(), pHSBlob->GetBufferSize(), NULL, &hullShader );
	if(pHSBlob) {pHSBlob->Release();pHSBlob=NULL;}

	ID3DBlob* pDSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/effectDS.cso", &pDSBlob))){MessageBox(0,L"Failed To load effectDS.cso",0,0);}
	else Renderer::graphicsDevice->CreateDomainShader( pDSBlob->GetBufferPointer(), pDSBlob->GetBufferSize(), NULL, &domainShader );
	if(pDSBlob) {pDSBlob->Release();pDSBlob=NULL;}
}
void Renderer::LoadSkyShaders()
{
    ID3DBlob* pVSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/skyVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load skyVS.cso",0,0);}
	Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &skyVS );

	if(pVSBlob){ pVSBlob->Release();
	pVSBlob=NULL; }


	ID3DBlob* pPSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/skyPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load skyPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &skyPS );
	
	if(pPSBlob){ pPSBlob->Release(); pPSBlob=NULL; }
}
void Renderer::LoadShadowShaders()
{
    ID3DBlob* pVSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/shadowVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load shadowVS.cso",0,0);}
	Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &shVS );
	if(pVSBlob) {pVSBlob->Release(); pVSBlob=NULL;}
	

	if(FAILED(D3DReadFileToBlob(L"shaders/cubeShadowVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load cubeShadowVS.cso",0,0);}
	Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &cubeShVS );
	if(pVSBlob) {pVSBlob->Release(); pVSBlob=NULL;}

	
	ID3DBlob* pPSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/shadowPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load shadowPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &shPS );
	if(pPSBlob){ pPSBlob->Release(); pPSBlob=NULL; }

	if(FAILED(D3DReadFileToBlob(L"shaders/cubeShadowPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load cubeShadowPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &cubeShPS );
	if(pPSBlob){ pPSBlob->Release(); pPSBlob=NULL; }



	if(FAILED(D3DReadFileToBlob(L"shaders/cubeShadowGS.cso", &pPSBlob))){MessageBox(0,L"Failed To load cubeShadowGS.cso",0,0);}
	Renderer::graphicsDevice->CreateGeometryShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &cubeShGS );
	if(pPSBlob){ pPSBlob->Release(); pPSBlob=NULL; }
}
void Renderer::LoadWaterShaders()
{
    ID3DBlob* pVSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/waterVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load waterVS.cso",0,0);}
	Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &waterVS );
	if(pVSBlob) {pVSBlob->Release(); pVSBlob=NULL;}


	
	ID3DBlob* pPSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/waterPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load waterPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &waterPS );
	if(pPSBlob){ pPSBlob->Release(); pPSBlob=NULL; }
}
void Renderer::LoadTrailShaders(){
    ID3DBlob* pVSBlob = NULL;
	if(FAILED(D3DReadFileToBlob(L"shaders/trailVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load trailVS.cso",0,0);}
	Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &trailVS );
	
	D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
	UINT numElements = ARRAYSIZE( layout );
	Renderer::graphicsDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &trailIL );
	
	if(pVSBlob) {pVSBlob->Release(); pVSBlob=NULL;}


	
	ID3DBlob* pPSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/trailPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load trailPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &trailPS );

	if(pPSBlob){ pPSBlob->Release(); pPSBlob=NULL; }
}


void Renderer::SetUpStates()
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


Armature* Renderer::getArmatureByName(const string& get)
{
	for(Armature* armature : armatures)
		if(!armature->name.compare(get))
			return armature;
	return nullptr;
}
int Renderer::getActionByName(Armature* armature, const string& get)
{
	if(armature==nullptr)
		return (-1);

	stringstream ss("");
	ss<<armature->unidentified_name<<get;
	for(int j=0;j<armature->actions.size();j++)
		if(!armature->actions[j].name.compare(ss.str()))
			return j;
	return (-1);
}
int Renderer::getBoneByName(Armature* armature, const string& get)
{
	for(int j=0;j<armature->boneCollection.size();j++)
		if(!armature->boneCollection[j]->name.compare(get))
			return j;
	return (-1);
}
Material* Renderer::getMaterialByName(const string& get)
{
	MaterialCollection::iterator iter = materials.find(get);
	if(iter!=materials.end())
		return iter->second;
	return NULL;
}
HitSphere* Renderer::getSphereByName(const string& get){
	for(HitSphere* hs : spheres)
		if(!hs->name.compare(get))
			return hs;
	return nullptr;
}

void Renderer::RecursiveBoneTransform(Armature* armature, Bone* bone, const XMMATRIX& parentCombinedMat){
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

	for(int i=0;i<bone->childrenI.size();++i){
		RecursiveBoneTransform(armature,bone->childrenI[i],boneMat);
	}

}
XMVECTOR Renderer::InterPolateKeyFrames(const float& cf, const int& maxCf, const vector<KeyFrame>& keyframeList, KeyFrameType type)
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
			for(int k=0;k<keyframeList.size();k++)
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
			float intervalBegin = maxCf - keyframes[0];
			float intervalEnd = keyframes[1] + intervalBegin;
			float intervalLen = abs(intervalEnd - intervalBegin);
			float offsetCf = cf + intervalBegin;
			if(intervalLen) interframe = offsetCf / intervalLen;
		}
		else{
			float intervalBegin = keyframes[0];
			float intervalEnd = keyframes[1];
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
Vertex Renderer::TransformVertex(const Mesh* mesh, const int& vertexI, const XMMATRIX& mat){
	XMVECTOR pos = XMLoadFloat4( &mesh->vertices[vertexI].pos );
	XMVECTOR nor = XMLoadFloat3( &mesh->vertices[vertexI].nor );
	float inWei[4]={mesh->vertices[vertexI].wei.x
		,mesh->vertices[vertexI].wei.y
		,mesh->vertices[vertexI].wei.z
		,mesh->vertices[vertexI].wei.w};
	float inBon[4]={mesh->vertices[vertexI].bon.x
		,mesh->vertices[vertexI].bon.y
		,mesh->vertices[vertexI].bon.z
		,mesh->vertices[vertexI].bon.w};
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

	XMFLOAT3 transformedP,transformedN;
	XMStoreFloat3( &transformedP,XMVector3Transform(pos,sump) );
	
	sump.r[3]=XMVectorSetX(sump.r[3],0);
	sump.r[3]=XMVectorSetY(sump.r[3],0);
	sump.r[3]=XMVectorSetZ(sump.r[3],0);
	//sump.r[3].m128_f32[0]=sump.r[3].m128_f32[1]=sump.r[3].m128_f32[2]=0;
	XMStoreFloat3( &transformedN,XMVector3Normalize(XMVector3Transform(nor,sump)));

	Vertex retV(transformedP);
	retV.nor=transformedN;
	retV.tex=mesh->vertices[vertexI].tex;
	retV.pre=XMFLOAT4(0,0,0,1);

	return retV;
}
XMFLOAT3 Renderer::VertexVelocity(const Mesh* mesh, const int& vertexI){
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
void Renderer::Update(float amount)
{
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
				
				cf = armature->currentFrame += amount;
				maxCf = armature->actions[activeAction].frameCount;
				if((int)cf>maxCf)
					cf = armature->currentFrame = 1;

				{
					for(int j=0;j<armature->rootbones.size();++j){
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

		for(map<string,vector<EmittedParticle*>>::iterator iter=emitterSystems.begin();iter!=emitterSystems.end();++iter){
			for(EmittedParticle* e:iter->second)
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
void Renderer::UpdateRenderInfo(ID3D11DeviceContext* context)
{

	UpdatePerWorldCB(context);
		
	UpdateObjects();

	//if(GetGameSpeed())
	{


		for(MeshCollection::iterator iter=meshes.begin(); iter!=meshes.end(); ++iter){
			Mesh* mesh = iter->second;
			if(mesh->hasArmature() && !mesh->softBody && mesh->renderable){
#ifdef USE_GPU_SKINNING
				BoneShaderBuffer bonebuf = BoneShaderBuffer();
				for(int k=0;k<mesh->armature->boneCollection.size();k++){
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
		//Renderer::graphicsMutex.lock();
		DrawForSO(context);
		//Renderer::graphicsMutex.unlock();
#endif


		UpdateSPTree(spTree);
		UpdateSPTree(spTree_trans);
		UpdateSPTree(spTree_water);
	}
	
	UpdateBoneLines();
	UpdateCubes();

	wind.time=(float)((Timer::TotalTime())/1000.0*GameSpeed/2.0*3.1415)*XMVectorGetX(XMVector3Length(XMLoadFloat3(&wind.direction)))*0.1f;
}
void Renderer::UpdateObjects(){
	for(int i=0;i<everyObject.size();i++){
		//XMMATRIX world;

		////if(!everyObject[i]->armatureTransform()){
		//	world = XMMatrixScalingFromVector(XMLoadFloat3(&everyObject[i]->scale));

		//	if(everyObject[i]->mesh->isBillboarded){
		//		XMMATRIX bbMat = XMMatrixIdentity();
		//		if(everyObject[i]->mesh->billboardAxis.x || everyObject[i]->mesh->billboardAxis.y || everyObject[i]->mesh->billboardAxis.z){
		//			float angle = 0;
		//			angle = atan2(everyObject[i]->translation.x - XMVectorGetX(Renderer::cam->Eye), everyObject[i]->translation.z - XMVectorGetZ(Renderer::cam->Eye)) * (180.0 / XM_PI);
		//			bbMat = XMMatrixRotationAxis(XMLoadFloat3(&everyObject[i]->mesh->billboardAxis), angle * 0.0174532925f );
		//		}
		//		else
		//			bbMat = XMMatrixInverse(0,XMMatrixLookAtLH(XMVectorSet(0,0,0,0),XMLoadFloat3(&everyObject[i]->translation)-Renderer::cam->Eye,XMVectorSet(0,1,0,0)));
		//			
		//		world *= bbMat * XMMatrixRotationQuaternion(XMLoadFloat4(&everyObject[i]->rotation));
		//	}
		//	else
		//		world *= XMMatrixRotationQuaternion(XMLoadFloat4(&everyObject[i]->rotation));

		//	world *= XMMatrixTranslationFromVector(XMLoadFloat3(&everyObject[i]->translation));
		////}
		//	if(everyObject[i]->parentbone()){
		//		int bi=everyObject[i]->boneIndex;
		//		world = 
		//			world*
		//			XMLoadFloat4x4(&everyObject[i]->mesh->armature->boneCollection[bi].poseFrame)
		//			;
		//	}
		////}
		////else world = XMMatrixIdentity();

		//everyObject[i]->worldPrev=everyObject[i]->world;
		//XMStoreFloat4x4(&everyObject[i]->world,world);

		XMMATRIX world = everyObject[i]->getTransform();
		
		if(everyObject[i]->mesh->isBillboarded){
			XMMATRIX bbMat = XMMatrixIdentity();
			if(everyObject[i]->mesh->billboardAxis.x || everyObject[i]->mesh->billboardAxis.y || everyObject[i]->mesh->billboardAxis.z){
				float angle = 0;
				angle = atan2(everyObject[i]->translation.x - XMVectorGetX(Renderer::cam->Eye), everyObject[i]->translation.z - XMVectorGetZ(Renderer::cam->Eye)) * (180.0 / XM_PI);
				bbMat = XMMatrixRotationAxis(XMLoadFloat3(&everyObject[i]->mesh->billboardAxis), angle * 0.0174532925f );
			}
			else
				bbMat = XMMatrixInverse(0,XMMatrixLookAtLH(XMVectorSet(0,0,0,0),XMVectorSubtract(XMLoadFloat3(&everyObject[i]->translation),Renderer::cam->Eye),XMVectorSet(0,1,0,0)));
					
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
			//XMMATRIX mw = XMLoadFloat4x4(&everyObject[i]->world);
			//if(everyObject[i]->armatureTransform())
			//	mw = mw * XMLoadFloat4x4(&everyObject[i]->mesh->armature->world);
			everyObject[i]->bounds=everyObject[i]->mesh->aabb.get(world);
		}
		else if(everyObject[i]->mesh->renderable)
			everyObject[i]->bounds.createFromHalfWidth(everyObject[i]->translation,everyObject[i]->scale);
	}
}
void Renderer::UpdateSoftBodyPinning(){
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
					m->goalNormals[j] = tvert.nor;
					++j;
				}
			}
		}
	}
}
void Renderer::UpdateSkinnedVB(){
	Renderer::graphicsMutex.lock();
	for(MeshCollection::iterator iter=meshes.begin(); iter!=meshes.end(); ++iter){
		Mesh* m = iter->second;
#ifdef USE_GPU_SKINNING
		if(m->softBody)
#else
		if(m->softBody || m->hasArmature())
#endif
		{
			UpdateBuffer(m->meshVertBuff,m->skinnedVertices.data(),Renderer::immediateContext,sizeof(Vertex)*m->skinnedVertices.size());
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			//void* dataPtr;
			//Renderer::immediateContext->Map(m->meshVertBuff,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr = (void*)mappedResource.pData;
			//memcpy(dataPtr,m->skinnedVertices.data(),sizeof(Vertex)*m->skinnedVertices.size());
			//Renderer::immediateContext->Unmap(m->meshVertBuff,0);
		}
	}
	Renderer::graphicsMutex.unlock();
}
void Renderer::UpdateImages(){
	for(int i=0;i<images.size();++i)
		images[i]->Update(GameSpeed);
	ManageImages();
}
void Renderer::ManageImages(){
		while(	
				!images.empty() && 
				(images.front()->effects.opacity==1 || images.front()->effects.fade==1)
			)
			images.pop_front();
}
void Renderer::PutWaterRipple(const string& image, const XMFLOAT3& pos, const XMFLOAT4& plane){
	oImage* img=new oImage("","",image);
	img->anim.fad=0.01f;
	img->anim.scaleX=0.2f;
	img->anim.scaleY=0.2f;
	img->effects.pos=pos;
	img->effects.rotation=(rand()%1000*0.001f)*2*3.1415f;
	img->effects.siz=XMFLOAT2(1,1);
	img->effects.typeFlag=WORLD;
	img->effects.quality=QUALITY_ANISOTROPIC;
	img->effects.pivotFlag=Pivot::CENTER;
	img->effects.lookAt=plane;
	img->effects.lookAt.w=1;
	waterRipples.push_back(img);
}
void Renderer::ManageWaterRipples(){
	while(	
		!waterRipples.empty() && 
			(waterRipples.front()->effects.opacity==1 || waterRipples.front()->effects.fade==1)
		)
		waterRipples.pop_front();
}
void Renderer::DrawWaterRipples(ID3D11DeviceContext* context){
	Image::BatchBegin(context);
	for(oImage* i:waterRipples){
		i->DrawNormal(context);
		i->Update(GetGameSpeed());
	}
}

void Renderer::DrawDebugSpheres(const XMMATRIX& newView, ID3D11DeviceContext* context)
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

		for(int i=0;i<spheres.size();i++){
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			LineBuffer sb;
			sb.mWorldViewProjection=XMMatrixTranspose(
				XMMatrixRotationX(Renderer::cam->updownRot)*XMMatrixRotationY(Renderer::cam->leftrightRot)*
				XMMatrixScaling( spheres[i]->radius,spheres[i]->radius,spheres[i]->radius ) *
				XMMatrixTranslationFromVector( XMLoadFloat3(&spheres[i]->translation) )
				*newView*Renderer::cam->Projection
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
void Renderer::DrawDebugLines(const XMMATRIX& newView, ID3D11DeviceContext* context)
{
	if(debugLines){
		//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
		//context->IASetInputLayout( lineIL );
		BindPrimitiveTopology(LINELIST,context);
		BindVertexLayout(lineIL,context);
	
		BindRasterizerState(rssh,context);
		BindDepthStencilState(xRayStencilState,STENCILREF_EMPTY,context);
		BindBlendState(blendState,context);


		//context->VSSetShader( lineVS, NULL, 0 );
		//context->PSSetShader( linePS, NULL, 0 );
		BindPS(linePS,context);
		BindVS(lineVS,context);

		BindConstantBufferVS(lineBuffer,0,context);

		for(int i=0;i<boneLines.size();i++){
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			LineBuffer sb;
			sb.mWorldViewProjection=XMMatrixTranspose(
				XMLoadFloat4x4(&boneLines[i].desc.transform)
				*newView*Renderer::cam->Projection
				);
			sb.color=boneLines[i].desc.color;

			UpdateBuffer(lineBuffer,&sb,context);
			//LineBuffer* dataPtr;
			//context->Map(lineBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr = (LineBuffer*)mappedResource.pData;
			//memcpy(dataPtr,&sb,sizeof(LineBuffer));
			//context->Unmap(lineBuffer,0);

			//context->VSSetConstantBuffers( 0, 1, &lineBuffer );

			//UINT stride = sizeof( XMFLOAT3A );
			//UINT offset = 0;
			//context->IASetVertexBuffers( 0, 1, &boneLines[i].vertexBuffer, &stride, &offset );

			//context->Draw(2,0);
			BindVertexBuffer(boneLines[i].vertexBuffer,0,sizeof(XMFLOAT3A),context);
			Draw(2,context);
		}
	}
}
void Renderer::DrawDebugBoxes(const XMMATRIX& newView, ID3D11DeviceContext* context)
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

		for(int i=0;i<cubes.size();i++){
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			LineBuffer sb;
			sb.mWorldViewProjection=XMMatrixTranspose(XMLoadFloat4x4(&cubes[i].desc.transform)*newView*Renderer::cam->Projection);
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

void Renderer::DrawSoftParticles(const XMVECTOR eye, const XMMATRIX& view, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, bool dark)
{
	static struct particlesystem_comparator {
		bool operator() (const EmittedParticle* a, const EmittedParticle* b) const{
			return a->lastSquaredDistMulThousand>b->lastSquaredDistMulThousand;
		}
	};

	set<EmittedParticle*,particlesystem_comparator> psystems;
	for(map<string,vector<EmittedParticle*>>::iterator iter=emitterSystems.begin();iter!=emitterSystems.end();++iter){
		for(EmittedParticle* e:iter->second){
			e->lastSquaredDistMulThousand=(long)(WickedMath::DistanceEstimated(e->bounding_box->getCenter(),XMFLOAT3(XMVectorGetX(eye),XMVectorGetY(eye),XMVectorGetZ(eye)))*1000);
			psystems.insert(e);
		}
	}

	for(EmittedParticle* e:psystems){
		e->DrawNonPremul(eye,view,context,depth,dark);
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
void Renderer::DrawSoftPremulParticles(const XMVECTOR eye, const XMMATRIX& view, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, bool dark)
{
	for(map<string,vector<EmittedParticle*>>::iterator iter=emitterSystems.begin();iter!=emitterSystems.end();++iter){
		for(EmittedParticle* e:iter->second)
			e->DrawPremul(eye,view,context,depth,dark);
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
void Renderer::DrawTrails(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes){
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

	for(int i=0;i<everyObject.size();i++){
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
						,WickedMath::Lerp(everyObject[i]->trail[k].tex,everyObject[i]->trail[k+2].tex,r)
						,WickedMath::Lerp(everyObject[i]->trail[k].col,everyObject[i]->trail[k+2].col,r)
						));
					trails.push_back(RibbonVertex(xpoint1
						,WickedMath::Lerp(everyObject[i]->trail[k+1].tex,everyObject[i]->trail[k+3].tex,r)
						,WickedMath::Lerp(everyObject[i]->trail[k+1].col,everyObject[i]->trail[k+3].col,r)
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
void Renderer::DrawImagesAdd(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes){
	imagesRTAdd.Activate(context,0,0,0,1);
	Image::BatchBegin(context);
	for(oImage* x : images){
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
void Renderer::DrawImages(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes){
	imagesRT.Activate(context,0,0,0,0);
	Image::BatchBegin(context);
	for(oImage* x : images){
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
void Renderer::DrawImagesNormals(ID3D11DeviceContext* context, ID3D11ShaderResourceView* refracRes){
	normalMapRT.Activate(context,0,0,0,0);
	Image::BatchBegin(context);
	for(oImage* x : images){
		x->DrawNormal(context);
	}
}
void Renderer::DrawLights(const XMMATRIX& newView, ID3D11DeviceContext* context
				, ID3D11ShaderResourceView* depth, ID3D11ShaderResourceView* normal, ID3D11ShaderResourceView* material
				, unsigned int stencilRef){

	
	Frustum frustum = Frustum();
	XMFLOAT4X4 proj,view;
	XMStoreFloat4x4( &proj,Renderer::cam->Projection );
	XMStoreFloat4x4( &view,newView );
	frustum.ConstructFrustum(Renderer::cam->zFarP,proj,view);
	
	CulledList culledObjects;
	if(spTree_lights)
		SPTree::getVisible(spTree_lights->root,frustum,culledObjects);

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

	for(int type=0;type<3;++type){

			
		BindVS(lightVS[type],context);
		BindPS(lightPS[type],context);
		BindConstantBufferPS(lightCb[type],1,context);
		BindConstantBufferVS(lightCb[type],1,context);


		for(Cullable* c : culledObjects){
			Light* l = (Light*)c;
			if(l->type==type){

			switch(type){
			case 0:{ //dir
					dLightBuffer lcb;
					lcb.direction=XMVector3Normalize(
						-XMVector3Transform( XMVectorSet(0,-1,0,1), XMMatrixRotationQuaternion( XMLoadFloat4(&l->rotation) ) )
						);
					lcb.col=XMFLOAT4(l->color.x*l->enerDis.x,l->color.y*l->enerDis.x,l->color.z*l->enerDis.x,1);
					lcb.mBiasResSoftshadow=XMFLOAT4(shBias,SHADOWMAPRES,SOFTSHADOW,0);
					for(int shmap=0;shmap<l->shadowMap.size();++shmap){
						lcb.mShM[shmap]=l->shadowCam[shmap].getVP();
						BindTexturePS(l->shadowMap[shmap].depth->shaderResource,4+shmap,context);
					}
					UpdateBuffer(lightCb[type],&lcb,context);

					}
					break;
			case 1:{ //point
					pLightBuffer lcb;
					lcb.pos=l->translation;
					lcb.col=l->color;
					lcb.enerdis=l->enerDis;
					lcb.enerdis.w=l->shadowMap.size();
					UpdateBuffer(lightCb[type],&lcb,context);

					if(!l->shadowMap.empty()) 
						BindTexturePS(l->shadowMap.front().depth->shaderResource,7,context);
					}
					break;
			case 2:{ //spot
					sLightBuffer lcb;
					const float coneS=l->enerDis.z/0.7853981852531433;
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
					lcb.mBiasResSoftshadow=XMFLOAT4(shBias,SHADOWMAPRES,SOFTSHADOW,0);
					lcb.mShM=l->shadowCam.size()?l->shadowCam[0].getVP():XMMatrixIdentity();
					lcb.col=l->color;
					lcb.enerdis=l->enerDis;
					lcb.enerdis.z=cos(l->enerDis.z/2.0);
					UpdateBuffer(lightCb[type],&lcb,context);

					for(int shmap=0;shmap<l->shadowMap.size();++shmap)
						BindTexturePS(l->shadowMap[shmap].depth->shaderResource,4+shmap,context);
					}
					break;
			default: 
				break;
			}

			//context->DrawIndexed(lightGRenderer[Light::getTypeStr(type)]->indices.size(),0,0);
			BindVertexBuffer(nullptr,0,0,context);
			switch (l->type)
			{
			case Light::LightType::DIRECTIONAL:
				//context->Draw(6,0);
				Draw(6,context);
				break;
			case Light::LightType::SPOT:
				//context->Draw(96,0);
				Draw(192,context);
				break;
			case Light::LightType::POINT:
				//context->Draw(240,0);
				Draw(240,context);
				break;
			default:
				break;
			}

			}
		}


	}

	}
}
void Renderer::DrawVolumeLights(const XMMATRIX& newView, ID3D11DeviceContext* context)
{
	
		Frustum frustum = Frustum();
		XMFLOAT4X4 proj,view;
		XMStoreFloat4x4( &proj,Renderer::cam->Projection );
		XMStoreFloat4x4( &view,newView );
		frustum.ConstructFrustum(Renderer::cam->zFarP,proj,view);

		
		CulledList culledObjects;
		if(spTree_lights)
			SPTree::getVisible(spTree_lights->root,frustum,culledObjects);

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
					//		XMMatrixRotationX(Renderer::cam->updownRot)*XMMatrixRotationY(Renderer::cam->leftrightRot)*
					//		XMMatrixTranslationFromVector( XMVector3Transform(Renderer::cam->Eye+XMVectorSet(0,100000,0,0),XMMatrixRotationQuaternion(XMLoadFloat4(&l->rotation))) )
					//		);
					//}
					if(type==1){ //point
						sca = l->enerDis.y*l->enerDis.x*0.01;
						world = XMMatrixTranspose(
							XMMatrixScaling(sca,sca,sca)*
							XMMatrixRotationX(Renderer::cam->updownRot)*XMMatrixRotationY(Renderer::cam->leftrightRot)*
							XMMatrixTranslationFromVector( XMLoadFloat3(&l->translation) )
							);
					}
					else{ //spot
						float coneS=l->enerDis.z/0.7853981852531433;
						sca = l->enerDis.y*l->enerDis.x*0.03;
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


void Renderer::DrawLensFlares(ID3D11DeviceContext* context, ID3D11ShaderResourceView* depth, const int& RENDERWIDTH, const int& RENDERHEIGHT){
	
	Frustum frustum = Frustum();
	XMFLOAT4X4 proj,view;
	XMStoreFloat4x4( &proj,Renderer::cam->Projection );
	XMStoreFloat4x4( &view,Renderer::cam->View );
	frustum.ConstructFrustum(Renderer::cam->zFarP,proj,view);

		
	CulledList culledObjects;
	if(spTree_lights)
		SPTree::getVisible(spTree_lights->root,frustum,culledObjects);

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
			
			XMVECTOR flarePos = XMVector3Project(POS,0,0,RENDERWIDTH,RENDERHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());

			if( XMVectorGetX(XMVector3Dot( XMVectorSubtract(POS,Renderer::cam->Eye),Renderer::cam->At ))>0 )
				LensFlare::Draw(depth,context,flarePos,l->lensFlareRimTextures);

		}

	}
}
void Renderer::ClearShadowMaps(ID3D11DeviceContext* context){
	if(GetGameSpeed())
	for(Light* l : lights){
		for(int index=0;index<l->shadowMap.size();++index){
			l->shadowMap[index].Activate(context);
		}
	}
}
void Renderer::DrawForShadowMap(ID3D11DeviceContext* context)
{
	if(GameSpeed){

	Frustum frustum = Frustum();
	XMFLOAT4X4 proj,view;
	XMStoreFloat4x4( &proj,Renderer::cam->Projection );
	XMStoreFloat4x4( &view,Renderer::cam->View );
	frustum.ConstructFrustum(Renderer::cam->zFarP,proj,view);

		
	CulledList culledLights;
	if(spTree_lights)
		SPTree::getVisible(spTree_lights->root,frustum,culledLights);

	if(culledLights.size()>0)
	{

	//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	BindPrimitiveTopology(TRIANGLELIST,context);
	BindVertexLayout(vertexLayout,context);
	//context->IASetInputLayout( vertexLayout );


	//context->OMSetDepthStencilState(depthStencilState, STENCILREF_DEFAULT);
	BindDepthStencilState(depthStencilState,STENCILREF_DEFAULT,context);
	
	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//UINT sampleMask   = 0xffffffff;
	//context->OMSetBlendState(blendState, blendFactor, sampleMask);
	BindBlendState(blendState,context);
	
	//context->PSSetShader( shPS, NULL, 0 );
	BindPS(shPS,context);
	//context->PSSetSamplers(0, 1, &texSampler);
	BindSamplerPS(texSampler,0,context);
	
	//context->VSSetShader( shVS, NULL, 0 );
	BindVS(shVS,context);
	//context->VSSetConstantBuffers( 0, 1, &shCb );
	//context->VSSetConstantBuffers( 1, 1, &matCb );
	//context->PSSetConstantBuffers(1,1,&matCb);
	BindConstantBufferVS(shCb,0,context);
	BindConstantBufferVS(matCb,1,context);
	BindConstantBufferPS(matCb,1,context);
	//context->VSSetConstantBuffers(3,1,&matIndexBuf);
	//context->PSSetConstantBuffers(3,1,&matIndexBuf);

	vector<Light*> pointLightsSaved(0);

	//DIRECTIONAL AND SPOTLIGHT 2DSHADOWMAPS
	for(Cullable* c : culledLights){
	Light* l=(Light*)c;
	if(l->type!=Light::POINT){

		for(int index=0;index<l->shadowMap.size();++index){
			l->shadowMap[index].Set(context);
			
			CulledCollection culledRenderer;
			CulledList culledObjects;

			if(l->type==Light::DIRECTIONAL){
				const float siz = l->shadowCam[index].size * 0.5f;
				const float f = l->shadowCam[index].farplane;
				AABB boundingbox;
				boundingbox.createFromHalfWidth(XMFLOAT3(0,0,0),XMFLOAT3(siz,f,siz));
				if(spTree)
					SPTree::getVisible(spTree->root,boundingbox.get(
							XMMatrixInverse(0,XMLoadFloat4x4(&l->shadowCam[index].View))
						),culledObjects);
			}
			else if(l->type==Light::SPOT){
				Frustum frustum = Frustum();
				XMFLOAT4X4 proj,view;
				XMStoreFloat4x4( &proj,XMLoadFloat4x4(&l->shadowCam[index].Projection) );
				XMStoreFloat4x4( &view,XMLoadFloat4x4(&l->shadowCam[index].View) );
				frustum.ConstructFrustum(Renderer::cam->zFarP,proj,view);
				if(spTree)
					SPTree::getVisible(spTree->root,frustum,culledObjects);
			}

			if(!culledObjects.empty()){

				for(Cullable* object : culledObjects){
					culledRenderer[((Object*)object)->mesh].insert((Object*)object);
				}
				
				for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
					Mesh* mesh = iter->first;
					CulledObjectList& visibleInstances = iter->second;

					if(visibleInstances.size() && !mesh->isBillboarded){
						
						if(!mesh->doubleSided)
							//context->RSSetState(rssh);
							BindRasterizerState(rssh,context);
						else
							//context->RSSetState(nonCullRSsh);
							BindRasterizerState(nonCullRSsh,context);
			
						//D3D11_MAPPED_SUBRESOURCE mappedResource;
						ForShadowMapCB cb;
						cb.mViewProjection = l->shadowCam[index].getVP();
						cb.mWind=wind.direction;
						cb.time=wind.time;
						cb.windRandomness=wind.randomness;
						cb.windWaveSize=wind.waveSize;
						UpdateBuffer(shCb,&cb,context);
						//ForShadowMapCB* dataPtr;
						//context->Map(shCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
						//dataPtr = (ForShadowMapCB*)mappedResource.pData;
						//memcpy(dataPtr,&cb,sizeof(ForShadowMapCB));
						//context->Unmap(shCb,0);
				
//#ifdef USE_GPU_SKINNING
//						context->VSSetConstantBuffers( 1, 1, &mesh->boneBuffer );
//#endif

						
						int k=0;
						for(CulledObjectList::iterator viter=visibleInstances.begin();viter!=visibleInstances.end();++viter){
							if((*viter)->particleEmitter!=Object::ParticleEmitter::EMITTER_INVISIBLE){
								if(mesh->softBody || (*viter)->armatureDeform)
									mesh->instances[0][k] = Instance( XMMatrixIdentity() );
								else 
									mesh->instances[0][k]=Instance(
										XMMatrixTranspose( XMLoadFloat4x4(&(*viter)->world) )
										);
								++k;
							}
						}
						if(k<1)
							continue;

						UpdateBuffer(mesh->meshInstanceBuffer,mesh->instances[0].data(),context,sizeof(Instance)*visibleInstances.size());

			
						//ID3D11Buffer* vertexBuffers[2] = {
						//	(mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff)
						//	,mesh->meshInstanceBuffer};
						//UINT strides[2] = {sizeof( Vertex ),sizeof(Instance)};
						//UINT offsets[2] = {0,0};
						//context->IASetVertexBuffers( 0, 2, vertexBuffers, strides, offsets );
						//context->IASetIndexBuffer(mesh->meshIndexBuff,DXGI_FORMAT_R32_UINT,0);
						
						BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),context);
						BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),context);
						BindIndexBuffer(mesh->meshIndexBuff,context);
			
				
						int matsiz = mesh->materialIndices.size();
						int m=0;
						for(Material* iMat : mesh->materials){

							if(!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
								BindTexturePS(iMat->texture,0,context);

						
								//MaterialCB* mcb = (MaterialCB*)_aligned_malloc(sizeof(MaterialCB),16);
								//mcb->difColor=XMFLOAT4A(iMat->diffuseColor.x,iMat->diffuseColor.y,iMat->diffuseColor.z,iMat->alpha);
								//mcb->hasRefNorTexSpe=XMFLOAT4A(iMat->hasRefMap,iMat->hasNormalMap,iMat->hasTexture,iMat->hasSpecularMap);
								//mcb->specular=iMat->specular;
								//mcb->refractionIndexMovingTexEnv=XMFLOAT4A(iMat->refraction_index,iMat->texOffset.x,iMat->texOffset.y,iMat->enviroReflection);
								//mcb->shadeless=iMat->shadeless;
								//mcb->specular_power=iMat->specular_power;
								//mcb->toon=iMat->toonshading;
								//mcb->matIndex=m;
								//mcb->emit=iMat->emit;
								
								MaterialCB* mcb = new MaterialCB(*iMat,m);

								UpdateBuffer(matCb,mcb,context);
								
								delete mcb;

								//MaterialCB* MdataPtr;
								//context->Map(matCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
								//MdataPtr = (MaterialCB*)mappedResource.pData;
								//memcpy(MdataPtr,&mcb,sizeof(MaterialCB));
								//context->Unmap(matCb,0);
					
								//MatIndexBuf ib;
								//ib.matIndex=m;
								//ib.padding=XMFLOAT3(0,0,0);

								//UpdateBuffer(matIndexBuf,&ib,context);
								//MatIndexBuf* IdataPtr;
								//context->Map(matIndexBuf,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
								//IdataPtr = (MatIndexBuf*)mappedResource.pData;
								//memcpy(IdataPtr,&ib,sizeof(MatIndexBuf));
								//context->Unmap(matIndexBuf,0);

								//context->DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),0,0,0);
								DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),context);

								m++;
							}
						}
					}
				}

			}
		}
	}
	else{
		pointLightsSaved.push_back(l);
	}
	}

		if(!pointLightsSaved.empty() && POINTLIGHTSHADOW){


			set<Light*,Cullable> orderedLights;
			for(Light* l : pointLightsSaved){
				l->lastSquaredDistMulThousand=(long)(WickedMath::DistanceEstimated(l->translation,XMFLOAT3(XMVectorGetX(Renderer::cam->Eye),XMVectorGetY(Renderer::cam->Eye),XMVectorGetZ(Renderer::cam->Eye)))*1000);
				orderedLights.insert(l);
			}

			int cube_shadowrenders_remain = POINTLIGHTSHADOW;

			BindPS(cubeShPS,context);
			BindVS(cubeShVS,context);
			BindGS(cubeShGS,context);
			BindConstantBufferGS(cubeShCb,0,context);
			BindConstantBufferPS(lightCb[1],2,context);
			for(Light* l : orderedLights){

				for(int index=0;index<l->shadowMap.size();++index){
				if(cube_shadowrenders_remain<=0)
					break;
				cube_shadowrenders_remain-=1;
				l->shadowMap[index].Set(context);

				Frustum frustum = Frustum();
				XMFLOAT4X4 proj,view;
				XMStoreFloat4x4( &proj,XMLoadFloat4x4(&l->shadowCam[index].Projection) );
				XMStoreFloat4x4( &view,XMLoadFloat4x4(&l->shadowCam[index].View) );
				frustum.ConstructFrustum(Renderer::cam->zFarP,proj,view);

				//D3D11_MAPPED_SUBRESOURCE mappedResource;
				pLightBuffer lcb;
				lcb.enerdis=l->enerDis;
				lcb.pos=l->translation;
				UpdateBuffer(lightCb[1],&lcb,context);
				//pLightBuffer* dataPtr;
				//context->Map(lightCb[1],0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
				//dataPtr = (pLightBuffer*)mappedResource.pData;
				//memcpy(dataPtr,&lcb,sizeof(pLightBuffer));
				//context->Unmap(lightCb[1],0);
				
				CubeShadowCb cb;
				for(int shcam=0;shcam<l->shadowCam.size();++shcam)
					cb.mViewProjection[shcam] = l->shadowCam[shcam].getVP();

				UpdateBuffer(cubeShCb,&cb,context);
				//CubeShadowCb* dataPtr1;
				//context->Map(cubeShCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
				//dataPtr1 = (CubeShadowCb*)mappedResource.pData;
				//memcpy(dataPtr1,&cb,sizeof(CubeShadowCb));
				//context->Unmap(cubeShCb,0);

				CulledCollection culledRenderer;
				CulledList culledObjects;

				if(spTree)
					SPTree::getVisible(spTree->root,l->bounds,culledObjects);

				for(Cullable* object : culledObjects)
					culledRenderer[((Object*)object)->mesh].insert((Object*)object);
				
				for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
					Mesh* mesh = iter->first;
					CulledObjectList& visibleInstances = iter->second;

					if(!mesh->isBillboarded && !visibleInstances.empty()){

							if(!mesh->doubleSided)
								//context->RSSetState(rssh);
								BindRasterizerState(rssh,context);
							else
								//context->RSSetState(nonCullRSsh);
								BindRasterizerState(nonCullRSsh,context);
			

						
							int k=0;
							for(CulledObjectList::iterator viter=visibleInstances.begin();viter!=visibleInstances.end();++viter){
								if((*viter)->particleEmitter!=Object::ParticleEmitter::EMITTER_INVISIBLE){
									if(mesh->softBody || (*viter)->armatureDeform)
										mesh->instances[0][k] = Instance( XMMatrixIdentity() );
									else 
										mesh->instances[0][k]=Instance(
											XMMatrixTranspose( XMLoadFloat4x4(&(*viter)->world) )
											);
									++k;
								}
							}
							if(k<1)
								continue;

							UpdateBuffer(mesh->meshInstanceBuffer,mesh->instances[0].data(),context,sizeof(Instance)*visibleInstances.size());

							BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),context);
							BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),context);
							BindIndexBuffer(mesh->meshIndexBuff,context);
			
				
							int matsiz = mesh->materialIndices.size();
							int m=0;
							for(Material* iMat : mesh->materials){
								if(!wireRender && !iMat->isSky && !iMat->water && iMat->cast_shadow) {
									BindTexturePS(iMat->texture,0,context);
						
									//MaterialCB* mcb = (MaterialCB*)_aligned_malloc(sizeof(MaterialCB),16);
									//mcb->difColor=XMFLOAT4A(iMat->diffuseColor.x,iMat->diffuseColor.y,iMat->diffuseColor.z,iMat->alpha);
									//mcb->hasRefNorTexSpe=XMFLOAT4A(iMat->hasRefMap,iMat->hasNormalMap,iMat->hasTexture,iMat->hasSpecularMap);
									//mcb->specular=iMat->specular;
									//mcb->refractionIndexMovingTexEnv=XMFLOAT4A(iMat->refraction_index,iMat->texOffset.x,iMat->texOffset.y,iMat->enviroReflection);
									//mcb->shadeless=iMat->shadeless;
									//mcb->specular_power=iMat->specular_power;
									//mcb->toon=iMat->toonshading;
									//mcb->matIndex=m;
									//mcb->emit=iMat->emit;
									MaterialCB* mcb = new MaterialCB(*iMat,m);

									UpdateBuffer(matCb,mcb,context);
									delete mcb;

									DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),context);
								}
									m++;
							}
						}
						visibleInstances.clear();
					}
				}
			}
		}
	}
		
	//context->GSSetShader(0,0,0);
		BindGS(nullptr,context);
	}
}
void Renderer::DrawForSO(ID3D11DeviceContext* context)
{
	//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
	BindPrimitiveTopology(POINTLIST,context);
	BindVertexLayout(sOIL,context);
	//context->IASetInputLayout( sOIL );
	//context->VSSetShader( sOVS, NULL, 0 );
	//context->GSSetShader( sOGS, NULL, 0 );
	BindVS(sOVS,context);
	BindGS(sOGS,context);
	//context->PSSetShader( 0, NULL, 0);
	BindPS(nullptr,context);

	for(MeshCollection::iterator iter=meshes.begin(); iter!=meshes.end(); ++iter){
		Mesh* mesh = iter->second;
		if(mesh->hasArmature() && !mesh->softBody){
			//UINT stride = sizeof( SkinnedVertex );
			//UINT offset = 0;
			//context->IASetVertexBuffers( 0, 1, &mesh->meshVertBuff, &stride, &offset );
			//context->IASetIndexBuffer(mesh->meshIndexBuff,DXGI_FORMAT_R32_UINT,0);
			BindVertexBuffer(mesh->meshVertBuff,0,sizeof(SkinnedVertex),context);
			//context->VSSetConstantBuffers( 1, 1, &mesh->boneBuffer );
			BindConstantBufferVS(mesh->boneBuffer,1,context);
			//UINT offsetSO[1] = {0};
			//context->SOSetTargets(1,&mesh->sOutBuffer,offsetSO);
			BindStreamOutTarget(mesh->sOutBuffer,context);
			//context->Draw(mesh->vertices.size(),0);
			Draw(mesh->vertices.size(),context);
			//context->DrawAuto();
			//context->DrawIndexed(mesh->indices.size(),0,0);
		}
	}

	//context->GSSetShader( 0, NULL, 0 );
	//context->VSSetShader( 0, NULL, 0 );
	BindGS(nullptr,context);
	BindVS(nullptr,context);
	//UINT offsetSO[1] = {0};
	//context->SOSetTargets(0,nullptr,offsetSO);
	BindStreamOutTarget(nullptr,context);
}

void Renderer::DrawWorld(const XMMATRIX& newView, bool DX11Eff, int tessF, ID3D11DeviceContext* context
				  , bool BlackOut, bool shaded
				  , ID3D11ShaderResourceView* refRes, bool grass, int passIdentifier)
{
	
	if(objects.empty())
		return;

	Frustum frustum = Frustum();
	XMFLOAT4X4 proj,view;
	XMStoreFloat4x4( &proj,Renderer::cam->Projection );
	XMStoreFloat4x4( &view,newView );
	frustum.ConstructFrustum(Renderer::cam->zFarP,proj,view);

	CulledCollection culledRenderer;
	CulledList culledObjects;
	if(spTree)
		SPTree::getVisible(spTree->root,frustum,culledObjects);
		//SPTree::getAll(spTree->root,culledObjects);

	if(!culledObjects.empty())
	{	

		for(Cullable* object : culledObjects){
			culledRenderer[((Object*)object)->mesh].insert((Object*)object);
			if(grass){
				for(HairParticle* hair : ((Object*)object)->hParticleSystems){
					XMFLOAT3 eye;
					XMStoreFloat3(&eye,Renderer::cam->Eye);
					hair->Draw(eye,newView,Renderer::cam->Projection,context);
				}
			}
		}

		if(DX11Eff && tessF) //context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST );
			BindPrimitiveTopology(PATCHLIST,context);
		else		//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
			BindPrimitiveTopology(TRIANGLELIST,context);
		//context->IASetInputLayout( vertexLayout );
		BindVertexLayout(vertexLayout,context);

		if(DX11Eff && tessF) //context->VSSetShader( vertexShader, NULL, 0 );
			BindVS(vertexShader,context);
		else //context->VSSetShader( vertexShader10, NULL, 0 );
			BindVS(vertexShader10,context);
		if(DX11Eff && tessF) //context->HSSetShader( hullShader, NULL, 0 );
			BindHS(hullShader,context);
		else		//context->HSSetShader( 0, NULL, 0 );
			BindHS(nullptr,context);
		if(DX11Eff && tessF) //context->DSSetShader( domainShader, NULL, 0 );
			BindDS(domainShader,context);
		else		//context->DSSetShader( 0, NULL, 0 );
			BindDS(nullptr,context);
		BindPS( wireRender?simplestPS:(BlackOut?blackoutPS:(shaded?pixelShader:textureonlyPS)),context);
	

		//context->VSSetSamplers(0, 1, &texSampler);
		//context->VSSetShaderResources(0,1,&noiseTex);
		BindSamplerVS(texSampler,0,context);
		BindTextureVS(noiseTex,0,context);
		if(DX11Eff && tessF) //context->DSSetSamplers(0, 1, &texSampler);
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

		//if(DX11Eff) context->DSSetConstantBuffers(3,1,&matIndexBuf);

	
		//float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		//UINT sampleMask   = 0xffffffff;
		//context->OMSetBlendState(blendState, blendFactor, sampleMask);
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
				//context->RSSetState(wireRender?wireRS:rasterizerState);
				BindRasterizerState(wireRender?wireRS:rasterizerState,context);
			else
				//context->RSSetState(wireRender?wireRS:nonCullRS);
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


			//Instance* instanceData=new Instance[visibleInstances.size()];
			//int k=0;
			//for(CulledObjectList::iterator viter=visibleInstances.begin();viter!=visibleInstances.end();++viter){
			//	if((*viter)->particleEmitter!=Object::ParticleEmitter::EMITTER_INVISIBLE){
			//		if(mesh->softBody || (*viter)->armatureDeform)
			//			instanceData[k] = Instance( XMMatrixIdentity() );
			//		else 
			//			instanceData[k]=Instance(
			//				XMMatrixTranspose( XMLoadFloat4x4(&(*viter)->world) )
			//				);
			//		++k;
			//	}
			//}
			//if(k<1)
			//	continue;

			//UpdateBuffer(mesh->meshInstanceBuffer,instanceData,context,sizeof(Instance)*visibleInstances.size());
			//delete[visibleInstances.size()] instanceData;



			int k=0;
			for(CulledObjectList::iterator viter=visibleInstances.begin();viter!=visibleInstances.end();++viter){
				if((*viter)->particleEmitter!=Object::ParticleEmitter::EMITTER_INVISIBLE){
					if(mesh->softBody || (*viter)->armatureDeform)
						mesh->instances[passIdentifier][k] = Instance( XMMatrixIdentity() );
					else 
						mesh->instances[passIdentifier][k]=Instance(
							XMMatrixTranspose( XMLoadFloat4x4(&(*viter)->world) )
							);
					++k;
				}
			}
			if(k<1)
				continue;

			UpdateBuffer(mesh->meshInstanceBuffer,mesh->instances[passIdentifier].data(),context,sizeof(Instance)*visibleInstances.size());
			
				
				
			//ID3D11Buffer* vertexBuffers[2] = {
			//	(mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff)
			//	,mesh->meshInstanceBuffer};
			//UINT strides[2] = {sizeof( Vertex ),sizeof(Instance)};
			//UINT offsets[2] = {0,0};
			//context->IASetVertexBuffers( 0, 2, vertexBuffers, strides, offsets );
			//context->IASetIndexBuffer(mesh->meshIndexBuff,DXGI_FORMAT_R32_UINT,0);
			BindIndexBuffer(mesh->meshIndexBuff,context);
			BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),context);
			BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),context);

			int m=0;
			for(Material* iMat : mesh->materials){

				if(!iMat->transparent && !iMat->isSky && !iMat->water){
					
					if(iMat->shadeless)
						//context->OMSetDepthStencilState(depthStencilState, STENCILREF_SHADELESS);
						BindDepthStencilState(depthStencilState,STENCILREF_SHADELESS,context);
					if(iMat->subsurface_scattering)
						//context->OMSetDepthStencilState(depthStencilState, STENCILREF_SKIN);
						BindDepthStencilState(depthStencilState,STENCILREF_SKIN,context);
					else
						//context->OMSetDepthStencilState(depthStencilState, mesh->stencilRef);
						BindDepthStencilState(depthStencilState,mesh->stencilRef,context);

					//MaterialCB* mcb = (MaterialCB*)_aligned_malloc(sizeof(MaterialCB),16);
					//mcb->difColor=XMFLOAT4A(iMat->diffuseColor.x,iMat->diffuseColor.y,iMat->diffuseColor.z,iMat->alpha);
					//mcb->hasRefNorTexSpe=XMFLOAT4A(iMat->hasRefMap && refRes,iMat->hasNormalMap,iMat->hasTexture,iMat->hasSpecularMap);
					//mcb->specular=iMat->specular;
					//mcb->refractionIndexMovingTexEnv=XMFLOAT4A(iMat->refraction_index,iMat->texOffset.x,iMat->texOffset.y,iMat->enviroReflection);
					//mcb->shadeless=iMat->shadeless;
					//mcb->specular_power=iMat->specular_power;
					//mcb->toon=iMat->toonshading;
					//mcb->matIndex=m;
					//mcb->emit=iMat->emit;

					MaterialCB* mcb = new MaterialCB(*iMat,m);

					UpdateBuffer(matCb,mcb,context);
					_aligned_free(mcb);

					if(!wireRender) BindTexturePS(iMat->texture,3,context);
					if(!wireRender) BindTexturePS(iMat->refMap,4,context);
					if(!wireRender) BindTexturePS(iMat->normalMap,5,context);
					if(!wireRender) BindTexturePS(iMat->specularMap,6,context);
					if(DX11Eff) //context->DSSetShaderResources(0,1,&iMat->displacementMap);
						BindTextureDS(iMat->displacementMap,0,context);
					
					//MatIndexBuf ib;
					//ib.matIndex=m;
					//ib.padding=XMFLOAT3(0,0,0);

					//UpdateBuffer(matIndexBuf,&ib,context);

					//context->DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),0,0,0);

					DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),context);
				}
				m++;
			}

		}

		
		//context->VSSetShader( 0, NULL, 0 );
		//context->HSSetShader( 0, NULL, 0 );
		//context->DSSetShader( 0, NULL, 0 );
		//context->PSSetShader( 0, NULL, 0 );
		BindPS(nullptr,context);
		BindVS(nullptr,context);
		BindDS(nullptr,context);
		BindHS(nullptr,context);

	}

}
void Renderer::DrawWorldWater(const XMMATRIX& newView, ID3D11ShaderResourceView* refracRes, ID3D11ShaderResourceView* refRes
		, ID3D11ShaderResourceView* depth, ID3D11ShaderResourceView* nor, ID3D11DeviceContext* context, int passIdentifier){
			
	if(objects_water.empty())
		return;

	Frustum frustum = Frustum();
	XMFLOAT4X4 proj,view;
	XMStoreFloat4x4( &proj,Renderer::cam->Projection );
	XMStoreFloat4x4( &view,newView );
	frustum.ConstructFrustum(Renderer::cam->zFarP,proj,view);

	CulledCollection culledRenderer;
	CulledList culledObjects;
	if(spTree_water)
		SPTree::getVisible(spTree_water->root,frustum,culledObjects);

	if(!culledObjects.empty())
	{
		for(Cullable* object : culledObjects)
			culledRenderer[((Object*)object)->mesh].insert((Object*)object);

		//D3D11_MAPPED_SUBRESOURCE mappedResource;

		//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		BindPrimitiveTopology(TRIANGLELIST,context);
		BindVertexLayout(vertexLayout,context);
		//context->IASetInputLayout( vertexLayout );
		//context->PSSetShader( wireRender?simplestPS:waterPS, NULL, 0 );
		BindPS(wireRender?simplestPS:waterPS,context);
		//context->VSSetShader( waterVS, NULL, 0 );
		BindVS(vertexShader10,context);

		//context->PSSetSamplers(0, 1, &texSampler);
		//context->PSSetSamplers(1, 1, &mapSampler);
		BindSamplerPS(texSampler,0,context);
		BindSamplerPS(mapSampler,1,context);
	
		if(!wireRender) BindTexturePS(enviroMap,0,context);
		if(!wireRender) BindTexturePS(refRes,1,context);
		if(!wireRender) BindTexturePS(refracRes,2,context);
		if(!wireRender) BindTexturePS(depth,7,context);
		if(!wireRender) BindTexturePS(nor,8,context);

		//context->VSSetConstantBuffers(0,1,&staticCb);

		//context->PSSetConstantBuffers( 0, 1, &pixelCB );
		//context->PSSetConstantBuffers( 1, 1, &fxCb );
		
		BindConstantBufferVS(staticCb,0,context);
		BindConstantBufferPS(pixelCB,0,context);
		BindConstantBufferPS(fxCb,1,context);
		BindConstantBufferVS(constantBuffer,3,context);
		BindConstantBufferVS(matCb,2,context);
		BindConstantBufferPS(matCb,2,context);

	
		//float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		//UINT sampleMask   = 0xffffffff;
		//context->OMSetBlendState(blendState, blendFactor, sampleMask);
		BindBlendState(blendState,context);
		//if(opaque) context->OMSetBlendState(blendState, blendFactor, sampleMask);
		//if(transparent) context->OMSetBlendState(blendStateTransparent, blendFactor, sampleMask);
		//context->OMSetDepthStencilState(depthReadStencilState, STENCILREF_EMPTY);
		//context->RSSetState(wireRender?wireRS:rasterizerState);
		BindDepthStencilState(depthReadStencilState,STENCILREF_EMPTY,context);
		BindRasterizerState(wireRender?wireRS:rasterizerState,context);
	
		//context->VSSetConstantBuffers( 2, 1, &matCb );
		//context->PSSetConstantBuffers( 2, 1, &matCb );
		//context->VSSetConstantBuffers(3,1,&matIndexBuf);
		//context->PSSetConstantBuffers(3,1,&matIndexBuf);

		
		for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
			Mesh* mesh = iter->first;
			CulledObjectList& visibleInstances = iter->second;


			int matsiz = mesh->materialIndices.size();

				
			int k=0;
			for(CulledObjectList::iterator viter=visibleInstances.begin();viter!=visibleInstances.end();++viter){
				if((*viter)->particleEmitter!=Object::ParticleEmitter::EMITTER_INVISIBLE){
					if(mesh->softBody || (*viter)->armatureDeform)
						mesh->instances[passIdentifier][k] = Instance( XMMatrixIdentity() );
					else 
						mesh->instances[passIdentifier][k]=Instance(
							XMMatrixTranspose( XMLoadFloat4x4(&(*viter)->world) )
							);
					++k;
				}
			}
			if(k<1)
				continue;

			UpdateBuffer(mesh->meshInstanceBuffer,mesh->instances[passIdentifier].data(),context,sizeof(Instance)*visibleInstances.size());
			
				
			//ID3D11Buffer* vertexBuffers[2] = {
			//	(mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff)
			//	,mesh->meshInstanceBuffer};
			//UINT strides[2] = {sizeof( Vertex ),sizeof(Instance)};
			//UINT offsets[2] = {0,0};
			//context->IASetVertexBuffers( 0, 2, vertexBuffers, strides, offsets );
			//context->IASetIndexBuffer(mesh->meshIndexBuff,DXGI_FORMAT_R32_UINT,0);
			BindIndexBuffer(mesh->meshIndexBuff,context);
			BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),context);
			BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),context);

			int m=0;
			for(Material* iMat : mesh->materials){

				if(iMat->water){

				//MaterialCB* mcb = (MaterialCB*)_aligned_malloc(sizeof(MaterialCB),16);
				//mcb->difColor=XMFLOAT4(iMat->diffuseColor.x,iMat->diffuseColor.y,iMat->diffuseColor.z,iMat->alpha);
				//mcb->hasRefNorTexSpe=XMFLOAT4(iMat->hasRefMap,iMat->hasNormalMap,iMat->hasTexture,iMat->hasSpecularMap);
				//mcb->specular=iMat->specular;
				//mcb->refractionIndexMovingTexEnv=XMFLOAT4(iMat->refraction_index,iMat->texOffset.x,iMat->texOffset.y,iMat->enviroReflection);
				//mcb->shadeless=iMat->shadeless;
				//mcb->specular_power=iMat->specular_power;
				//mcb->toon=iMat->toonshading;
				//mcb->matIndex=m;
				//mcb->emit=iMat->emit;
				MaterialCB* mcb = new MaterialCB(*iMat,m);

				UpdateBuffer(matCb,mcb,context);
				_aligned_free(mcb);

				if(!wireRender) BindTexturePS(iMat->texture,3,context);
				if(!wireRender) BindTexturePS(iMat->refMap,4,context);
				if(!wireRender) BindTexturePS(iMat->normalMap,5,context);
				if(!wireRender) BindTexturePS(iMat->specularMap,6,context);
					
				//MatIndexBuf ib;
				//ib.matIndex=m;
				//ib.padding=XMFLOAT3(0,0,0);

				//UpdateBuffer(matIndexBuf,&ib,context);

				//context->DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),0,0,0);

				DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),context);
				}
				m++;
			}
		}

	}

}
void Renderer::DrawWorldTransparent(const XMMATRIX& newView, ID3D11ShaderResourceView* refracRes, ID3D11ShaderResourceView* refRes
		, ID3D11ShaderResourceView* depth, ID3D11DeviceContext* context, int passIdentifier){

	if(objects_trans.empty())
		return;

	Frustum frustum = Frustum();
	XMFLOAT4X4 proj,view;
	XMStoreFloat4x4( &proj,Renderer::cam->Projection );
	XMStoreFloat4x4( &view,newView );
	frustum.ConstructFrustum(Renderer::cam->zFarP,proj,view);

	CulledCollection culledRenderer;
	CulledList culledObjects;
	if(spTree_trans)
		SPTree::getVisible(spTree_trans->root,frustum,culledObjects);

	if(!culledObjects.empty())
	{
		for(Cullable* object : culledObjects)
			culledRenderer[((Object*)object)->mesh].insert((Object*)object);

		//D3D11_MAPPED_SUBRESOURCE mappedResource;

		//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		BindPrimitiveTopology(TRIANGLELIST,context);
		BindVertexLayout(vertexLayout,context);
		//context->IASetInputLayout( vertexLayout );
		//context->PSSetShader( wireRender?simplestPS:transparentPS, NULL, 0 );
		BindPS(wireRender?simplestPS:transparentPS,context);
		//context->VSSetShader( vertexShader10, NULL, 0 );
		BindVS(vertexShader10,context);

		//context->PSSetSamplers(0, 1, &texSampler);
		//context->PSSetSamplers(1, 1, &mapSampler);
		BindSamplerPS(texSampler,0,context);
		BindSamplerPS(mapSampler,1,context);
	
		if(!wireRender) BindTexturePS(enviroMap,0,context);
		if(!wireRender) BindTexturePS(refRes,1,context);
		if(!wireRender) BindTexturePS(refracRes,2,context);
		if(!wireRender) BindTexturePS(depth,7,context);

	
		//context->VSSetConstantBuffers(0,1,&staticCb);
		//context->PSSetConstantBuffers( 0, 1, &pixelCB );
		//context->PSSetConstantBuffers( 1, 1, &fxCb );

		BindConstantBufferVS(staticCb,0,context);
		BindConstantBufferPS(pixelCB,0,context);
		BindConstantBufferPS(fxCb,1,context);
		BindConstantBufferVS(constantBuffer,3,context);
		BindConstantBufferVS(matCb,2,context);
		BindConstantBufferPS(matCb,2,context);

	
		//float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		//UINT sampleMask   = 0xffffffff;
		//context->OMSetBlendState(blendState, blendFactor, sampleMask);
		BindBlendState(blendState,context);
		//if(opaque) context->OMSetBlendState(blendState, blendFactor, sampleMask);
		//if(transparent) context->OMSetBlendState(blendStateTransparent, blendFactor, sampleMask);
		//context->OMSetDepthStencilState(depthStencilState, STENCILREF_EMPTY);
		//context->RSSetState(wireRender?wireRS:rasterizerState);
		BindDepthStencilState(depthStencilState,STENCILREF_TRANSPARENT,context);
		BindRasterizerState(wireRender?wireRS:rasterizerState,context);
	
		//context->VSSetConstantBuffers( 2, 1, &matCb );
		//context->PSSetConstantBuffers( 2, 1, &matCb );
		//context->VSSetConstantBuffers(3,1,&matIndexBuf);
		//context->PSSetConstantBuffers(3,1,&matIndexBuf);

		for (CulledCollection::iterator iter = culledRenderer.begin(); iter != culledRenderer.end(); ++iter) {
			Mesh* mesh = iter->first;
			CulledObjectList& visibleInstances = iter->second;



			if(!mesh->doubleSided)
				context->RSSetState(wireRender?wireRS:rasterizerState);
			else
				context->RSSetState(wireRender?wireRS:nonCullRS);

			int matsiz = mesh->materialIndices.size();

				
			
			int k=0;
			for(CulledObjectList::iterator viter=visibleInstances.begin();viter!=visibleInstances.end();++viter){
				if((*viter)->particleEmitter!=Object::ParticleEmitter::EMITTER_INVISIBLE){
					if(mesh->softBody || (*viter)->armatureDeform)
						mesh->instances[passIdentifier][k] = Instance( XMMatrixIdentity() );
					else 
						mesh->instances[passIdentifier][k]=Instance(
							XMMatrixTranspose( XMLoadFloat4x4(&(*viter)->world) )
							);
					++k;
				}
			}
			if(k<1)
				continue;

			UpdateBuffer(mesh->meshInstanceBuffer,mesh->instances[passIdentifier].data(),context,sizeof(Instance)*visibleInstances.size());
			
				
				
			//ID3D11Buffer* vertexBuffers[2] = {
			//	(mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff)
			//	,mesh->meshInstanceBuffer};
			//UINT strides[2] = {sizeof( Vertex ),sizeof(Instance)};
			//UINT offsets[2] = {0,0};
			//context->IASetVertexBuffers( 0, 2, vertexBuffers, strides, offsets );
			//context->IASetIndexBuffer(mesh->meshIndexBuff,DXGI_FORMAT_R32_UINT,0);
			BindIndexBuffer(mesh->meshIndexBuff,context);
			BindVertexBuffer((mesh->sOutBuffer?mesh->sOutBuffer:mesh->meshVertBuff),0,sizeof(Vertex),context);
			BindVertexBuffer(mesh->meshInstanceBuffer,1,sizeof(Instance),context);
				
	//#ifdef USE_GPU_SKINNING
	//		context->VSSetConstantBuffers( 1, 1, &mesh->boneBuffer );
	//#endif

			int m=0;
			for(Material* iMat : mesh->materials){

				if(iMat->transparent && iMat->alpha>0 && !iMat->water && !iMat->isSky){

				//MaterialCB* mcb = (MaterialCB*)_aligned_malloc(sizeof(MaterialCB),16);
				//mcb->difColor=XMFLOAT4(iMat->diffuseColor.x,iMat->diffuseColor.y,iMat->diffuseColor.z,iMat->alpha);
				//mcb->hasRefNorTexSpe=XMFLOAT4(iMat->hasRefMap,iMat->hasNormalMap,iMat->hasTexture,iMat->hasSpecularMap);
				//mcb->specular=iMat->specular;
				//mcb->refractionIndexMovingTexEnv=XMFLOAT4(iMat->refraction_index,iMat->texOffset.x,iMat->texOffset.y,iMat->enviroReflection);
				//mcb->shadeless=iMat->shadeless;
				//mcb->specular_power=iMat->specular_power;
				//mcb->toon=iMat->toonshading;
				//mcb->matIndex=m;
				//mcb->emit=iMat->emit;
					
				MaterialCB* mcb = new MaterialCB(*iMat,m);

				UpdateBuffer(matCb,mcb,context);
				_aligned_free(mcb);

				if(!wireRender) BindTexturePS(iMat->texture,3,context);
				if(!wireRender) BindTexturePS(iMat->refMap,4,context);
				if(!wireRender) BindTexturePS(iMat->normalMap,5,context);
				if(!wireRender) BindTexturePS(iMat->specularMap,6,context);
					
				//MatIndexBuf ib;
				//ib.matIndex=m;
				//ib.padding=XMFLOAT3(0,0,0);

				//UpdateBuffer(matIndexBuf,&ib,context);

				//context->DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),0,0,0);
				
				DrawIndexedInstanced(mesh->indices.size(),visibleInstances.size(),context);
				}
				m++;
			}
		}
	}
	
}


void Renderer::DrawSky(const XMVECTOR& newCenter, ID3D11DeviceContext* context)
{
	//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	BindPrimitiveTopology(TRIANGLELIST,context);
	//context->RSSetState(backFaceRS);
	//context->OMSetDepthStencilState(depthReadStencilState, STENCILREF_SKY);
	//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//UINT sampleMask   = 0xffffffff;
	//context->OMSetBlendState(blendState, blendFactor, sampleMask);
	BindRasterizerState(backFaceRS,context);
	BindDepthStencilState(depthReadStencilState,STENCILREF_SKY,context);
	BindBlendState(blendState,context);
	
	//context->VSSetShader( skyVS, NULL, 0 );
	//context->PSSetShader( skyPS, NULL, 0 );
	BindVS(skyVS,context);
	BindPS(skyPS,context);
	
	BindTexturePS(enviroMap,0,context);
	//context->PSSetSamplers(0, 1, &skySampler);
	BindSamplerPS(skySampler,0,context);
	
	//context->VSSetConstantBuffers( 3, 1, &skyCb );
	//context->PSSetConstantBuffers( 0, 1, &pixelCB );

	//context->Draw(240,0);

	BindConstantBufferVS(skyCb,3,context);
	BindConstantBufferPS(pixelCB,0,context);
	BindConstantBufferPS(fxCb,1,context);

	BindVertexBuffer(nullptr,0,0,context);
	Draw(240,context);
}

void Renderer::DrawDecals(const XMMATRIX& newView, DeviceContext context, TextureView depth)
{
	if(!decals.empty()){
		Frustum frustum = Frustum();
		XMFLOAT4X4 proj,view;
		XMStoreFloat4x4( &proj,Renderer::cam->Projection );
		XMStoreFloat4x4( &view,newView );
		frustum.ConstructFrustum(Renderer::cam->zFarP,proj,view);

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
						*newView
						*Renderer::cam->Projection
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
				XMStoreFloat3(&dcbps->eye,Renderer::cam->Eye);
				dcbps->opacity=WickedMath::Clamp((decal->life<=-2?1:decal->life<decal->fadeStart?decal->life/decal->fadeStart:1),0,1);
				dcbps->front=decal->front;
				UpdateBuffer(decalCbPS,dcbps,context);
				_aligned_free(dcbps);

				Draw(36,context);

			}

		}

	}
}


void Renderer::UpdatePerWorldCB(ID3D11DeviceContext* context){
	PixelCB* pcb = (PixelCB*)_aligned_malloc(sizeof(PixelCB),16);
	pcb->mSun=XMVector3Normalize( GetSunPosition() );
	pcb->mHorizon=worldInfo.horizon;
	pcb->mAmbient=worldInfo.ambient;
	pcb->mSunColor=GetSunColor();
	pcb->mFogSEH=worldInfo.fogSEH;
	UpdateBuffer(pixelCB,pcb,context);
	_aligned_free(pcb);


	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//PixelCB pcb;
	//pcb.mSun=XMVector3Normalize( GetSunPosition() );
	//pcb.mHorizon=worldInfo.horizon;
	//pcb.mAmbient=worldInfo.ambient;
	//pcb.mSunColor=GetSunColor();
	//pcb.mFogSEH=worldInfo.fogSEH;
	//PixelCB* dataPtr2;
	//context->Map(pixelCB,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	//dataPtr2 = (PixelCB*)mappedResource.pData;
	//memcpy(dataPtr2,&pcb,sizeof(PixelCB));
	//context->Unmap(pixelCB,0);
}
void Renderer::UpdatePerFrameCB(ID3D11DeviceContext* context){
	ViewPropCB cb;
	cb.mZFarP=Renderer::cam->zFarP;
	cb.mZNearP=Renderer::cam->zNearP;
	UpdateBuffer(viewPropCB,&cb,context);
	
	BindConstantBufferPS(viewPropCB,10,context);
}
void Renderer::UpdatePerRenderCB(ID3D11DeviceContext* context, int tessF){
	if(tessF){
		TessBuffer tb;
		tb.g_f4Eye = Renderer::cam->Eye;
		tb.g_f4TessFactors = XMFLOAT4A( tessF,2,4,6 );
		UpdateBuffer(tessBuf,&tb,context);
	}

	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//if(tessF){
	//	TessBuffer tb;
	//	tb.g_f4Eye = Renderer::cam->Eye;
	//	tb.g_f4TessFactors = XMFLOAT4A( tessF,2,4,6 );
	//	TessBuffer* dataPtr;
	//	context->Map(tessBuf,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	//	dataPtr = (TessBuffer*)mappedResource.pData;
	//	memcpy(dataPtr,&tb,sizeof(TessBuffer));
	//	context->Unmap(tessBuf,0);
	//}
}
void Renderer::UpdatePerViewCB(ID3D11DeviceContext* context, const XMMATRIX& newView, const XMMATRIX& newRefView, const XMMATRIX& newProjection
							 , const XMVECTOR& newEye, const XMFLOAT4& newClipPlane){

	
	StaticCB* cb = (StaticCB*)_aligned_malloc(sizeof(StaticCB),16);
	cb->mViewProjection = XMMatrixTranspose( newView * newProjection );
	cb->mRefViewProjection = XMMatrixTranspose( newRefView * newProjection);
	cb->mCamPos = newEye;
	cb->mClipPlane = newClipPlane;
	cb->mWind=wind.direction;
	cb->time=wind.time;
	cb->windRandomness=wind.randomness;
	cb->windWaveSize=wind.waveSize;
	UpdateBuffer(staticCb,cb,context);
	_aligned_free(cb);

	SkyBuffer* scb = (SkyBuffer*)_aligned_malloc(sizeof(SkyBuffer),16);
	scb->mV=XMMatrixTranspose(newView);
	scb->mP=XMMatrixTranspose(newProjection);
	//scb.mV = XMMatrixTranspose( XMMatrixInverse(0, ( newView )) );
	//scb.mP = XMMatrixTranspose( XMMatrixInverse(0, ( newProjection )) );
	UpdateBuffer(skyCb,scb,context);
	_aligned_free(scb);

	UpdateBuffer(trailCB,&XMMatrixTranspose( newView * newProjection ),context);

	LightStaticCB* lcb = (LightStaticCB*)_aligned_malloc(sizeof(LightStaticCB),16);
	lcb->mProjInv=XMMatrixInverse( 0,XMMatrixTranspose(newView*newProjection) );
	UpdateBuffer(lightStaticCb,lcb,context);
	_aligned_free(lcb);

	//D3D11_MAPPED_SUBRESOURCE mapRes;
	//StaticCB cb;
	//cb.mViewProjection = XMMatrixTranspose( newView * newProjection );
	//cb.mRefViewProjection = XMMatrixTranspose( newRefView * newProjection);
	//cb.mCamPos = newEye;
	////cb.mMotionBlur = XMFLOAT4A(vertexBlur,0,0,0);
	//cb.mClipPlane = newClipPlane;
	//cb.mWind=wind.direction;
	//cb.time=wind.time;
	//cb.windRandomness=wind.randomness;
	//cb.windWaveSize=wind.waveSize;
	//StaticCB* dataPtr;
	//context->Map(staticCb,0,D3D11_MAP_WRITE_DISCARD,0,&mapRes);
	//dataPtr = (StaticCB*)mapRes.pData;
	//memcpy(dataPtr,&cb,sizeof(StaticCB));
	//context->Unmap(staticCb,0);

	//D3D11_MAPPED_SUBRESOURCE smapRes;
	//SkyBuffer scb;
	//scb.mV = XMMatrixTranspose( XMMatrixInverse(0, ( newView )) );
	//scb.mP = XMMatrixTranspose( XMMatrixInverse(0, ( newProjection )) );
	//SkyBuffer* sdataPtr;
	//context->Map(skyCb,0,D3D11_MAP_WRITE_DISCARD,0,&smapRes);
	//sdataPtr = (SkyBuffer*)smapRes.pData;
	//memcpy(sdataPtr,&scb,sizeof(SkyBuffer));
	//context->Unmap(skyCb,0);

	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//XMMATRIX tcb = XMMatrixTranspose( newView * newProjection );
	//XMMATRIX* dataPtr1;
	//context->Map(trailCB,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	//dataPtr1 = (XMMATRIX*)mappedResource.pData;
	//memcpy(dataPtr1,&tcb,sizeof(XMMATRIX));
	//context->Unmap(trailCB,0);
	//
	//LightStaticCB lcb;
	//lcb.mProjInv=XMMatrixInverse( 0,XMMatrixTranspose(newView*newProjection) );
	//D3D11_MAPPED_SUBRESOURCE mr;
	//LightStaticCB* dataPtr2;
	//context->Map(lightStaticCb,0,D3D11_MAP_WRITE_DISCARD,0,&mr);
	//dataPtr2 = (LightStaticCB*)mr.pData;
	//memcpy(dataPtr2,&lcb,sizeof(LightStaticCB));
	//context->Unmap(lightStaticCb,0);
}
void Renderer::UpdatePerEffectCB(ID3D11DeviceContext* context, bool BlackOut, bool BlackWhite, bool InvertCol, const XMFLOAT4 colorMask){
	FxCB* fb = (FxCB*)_aligned_malloc(sizeof(FxCB),16);
	fb->mFx=XMFLOAT4(BlackOut,BlackWhite,InvertCol,0);
	fb->colorMask=colorMask;
	UpdateBuffer(fxCb,fb,context);
	_aligned_free(fb);
	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//FxCB fb;
	//fb.mFx=XMFLOAT4(BlackOut,BlackWhite,InvertCol,0);
	//fb.colorMask=colorMask;
	//FxCB* dataPtr;
	//context->Map(fxCb,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	//dataPtr = (FxCB*)mappedResource.pData;
	//memcpy(dataPtr,&fb,sizeof(FxCB));
	//context->Unmap(fxCb,0);
}

void Renderer::FinishLoading(){
	everyObject.reserve(objects.size()+objects_trans.size()+objects_water.size());
	everyObject.insert(everyObject.end(),objects.begin(),objects.end());
	everyObject.insert(everyObject.end(),objects_trans.begin(),objects_trans.end());
	everyObject.insert(everyObject.end(),objects_water.begin(),objects_water.end());

	for(MeshCollection::iterator iter=meshes.begin(); iter!=meshes.end(); ++iter)
		addVertexCount(iter->second->vertices.size());

	for(Object* o : everyObject)
		for(EmittedParticle* e:o->eParticleSystems){
			emitterSystems[e->name].push_back(e);
			if(e->light!=nullptr)
				lights.push_back(e->light);
		}

	SetUpLights();

	Update();
	UpdateRenderInfo(deferredContexts[0]);
	UpdateLights();
	GenerateSPTree(spTree,vector<Cullable*>(objects.begin(),objects.end()),GENERATE_OCTREE);
	GenerateSPTree(spTree_trans,vector<Cullable*>(objects_trans.begin(),objects_trans.end()),GENERATE_OCTREE);
	GenerateSPTree(spTree_water,vector<Cullable*>(objects_water.begin(),objects_water.end()),GENERATE_OCTREE);
	GenerateSPTree(spTree_lights,vector<Cullable*>(lights.begin(),lights.end()),GENERATE_OCTREE);
	SetUpCubes();
	SetUpBoneLines();
}


void Renderer::SetUpLights()
{
	for(Light* l:lights){
		if(l->type==Light::DIRECTIONAL){

			float lerp = 0.5f;
			float lerp1 = 0.12f;
			float lerp2 = 0.016f;
			XMVECTOR a0,a,b0,b;
			a0=XMVector3Unproject(XMVectorSet(0,RENDERHEIGHT,0,1),0,0,RENDERWIDTH,RENDERHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());
			a=XMVector3Unproject(XMVectorSet(0,RENDERHEIGHT,1,1),0,0,RENDERWIDTH,RENDERHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());
			b0=XMVector3Unproject(XMVectorSet(RENDERWIDTH,RENDERHEIGHT,0,1),0,0,RENDERWIDTH,RENDERHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());
			b=XMVector3Unproject(XMVectorSet(RENDERWIDTH,RENDERHEIGHT,1,1),0,0,RENDERWIDTH,RENDERHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());
			float size=XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0,b,lerp),XMVectorLerp(a0,a,lerp))));
			float size1=XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0,b,lerp1),XMVectorLerp(a0,a,lerp1))));
			float size2=XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0,b,lerp2),XMVectorLerp(a0,a,lerp2))));
			XMVECTOR rot = XMQuaternionIdentity();

			l->shadowCam.push_back(SHCAM(size,rot,0,Renderer::cam->zFarP));
			l->shadowCam.push_back(SHCAM(size1,rot,0,Renderer::cam->zFarP));
			l->shadowCam.push_back(SHCAM(size2,rot,0,Renderer::cam->zFarP));

		}
		else if(l->type==Light::SPOT && l->shadowMap.size()){
			l->shadowCam.push_back( SHCAM(l->rotation,Renderer::cam->zNearP,l->enerDis.y,l->enerDis.z) );
		}
		else if(l->type==Light::POINT && l->shadowMap.size()){
			l->shadowCam.push_back( SHCAM(XMFLOAT4(0.5,-0.5,-0.5,-0.5),Renderer::cam->zNearP,l->enerDis.y,XM_PI/2.0) ); //+x
			l->shadowCam.push_back( SHCAM(XMFLOAT4(0.5,0.5,0.5,-0.5),Renderer::cam->zNearP,l->enerDis.y,XM_PI/2.0) ); //-x

			l->shadowCam.push_back( SHCAM(XMFLOAT4(1,0,0,-0),Renderer::cam->zNearP,l->enerDis.y,XM_PI/2.0) ); //+y
			l->shadowCam.push_back( SHCAM(XMFLOAT4(0,0,0,-1),Renderer::cam->zNearP,l->enerDis.y,XM_PI/2.0) ); //-y

			l->shadowCam.push_back( SHCAM(XMFLOAT4(0.707,0,0,-0.707),Renderer::cam->zNearP,l->enerDis.y,XM_PI/2.0) ); //+z
			l->shadowCam.push_back( SHCAM(XMFLOAT4(0,0.707,0.707,0),Renderer::cam->zNearP,l->enerDis.y,XM_PI/2.0) ); //-z
		}
	}

	
}
void Renderer::UpdateLights()
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
			c=XMVector3Unproject(XMVectorSet(RENDERWIDTH/2,RENDERHEIGHT/2,1,1),0,0,RENDERWIDTH,RENDERHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());
			d=XMVector3Unproject(XMVectorSet(RENDERWIDTH/2,RENDERHEIGHT/2,0,1),0,0,RENDERWIDTH,RENDERHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());
			
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
			if(l->shadowCam.size()>0){
				l->shadowCam[0].Update(rrr*XMMatrixTranslationFromVector(e));
				if(l->shadowCam.size()>1){
					l->shadowCam[1].Update(rrr*XMMatrixTranslationFromVector(e1));
					if(l->shadowCam.size()>2)
						l->shadowCam[2].Update(rrr*XMMatrixTranslationFromVector(e2));
				}
			}
			
			l->bounds.createFromHalfWidth(XMFLOAT3(XMVectorGetX(Renderer::cam->Eye),XMVectorGetY(Renderer::cam->Eye),XMVectorGetZ(Renderer::cam->Eye)),XMFLOAT3(10000,10000,10000));
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
			for(int i=0;i<l->shadowCam.size();++i){
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

Renderer::Picked Renderer::Pick(long cursorX, long cursorY, PICKTYPE pickType)
{
	XMVECTOR& lineStart = XMVector3Unproject(XMVectorSet(cursorX,cursorY,0,1),0,0
		,SCREENWIDTH,SCREENHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());
	XMVECTOR& lineEnd = XMVector3Unproject(XMVectorSet(cursorX,cursorY,1,1),0,0
		,SCREENWIDTH,SCREENHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());
	XMVECTOR& rayOrigin = lineStart, rayDirection = XMVector3Normalize(XMVectorSubtract(lineEnd,lineStart));
	RAY ray = RAY(lineStart,rayDirection);

	CulledCollection culledRenderer;
	CulledList culledObjects;
	SPTree* searchTree = nullptr;
	switch (pickType)
	{
	case Renderer::PICK_OPAQUE:
		searchTree=spTree;
		break;
	case Renderer::PICK_TRANSPARENT:
		searchTree=spTree_trans;
		break;
	case Renderer::PICK_WATER:
		searchTree=spTree_water;
		break;
	default:
		break;
	}
	if(searchTree)
	{
		SPTree::getVisible(searchTree->root,ray,culledObjects);

		vector<Picked> pickPoints;

		if(!culledObjects.empty())
		{
			for(Cullable* culled : culledObjects){
				Object* object = (Object*)culled;
				Mesh* mesh = object->mesh;
				XMMATRIX& objectMat = XMLoadFloat4x4(&object->world);

				for(int i=0;i<mesh->indices.size();i+=3){
					int i0=mesh->indices[i],i1=mesh->indices[i+1],i2=mesh->indices[i+2];
					Vertex& v0=mesh->skinnedVertices[i0],v1=mesh->skinnedVertices[i1],v2=mesh->skinnedVertices[i2];
					XMVECTOR& V0=
						XMVector4Transform(XMLoadFloat4(&v0.pos),objectMat)
						,V1=XMVector4Transform(XMLoadFloat4(&v1.pos),objectMat)
						,V2=XMVector4Transform(XMLoadFloat4(&v2.pos),objectMat);
					float distance = 0;
					if(TriangleTests::Intersects(rayOrigin,rayDirection,V0,V1,V2,distance)){
						XMVECTOR& pos = XMVectorAdd(rayOrigin,rayDirection*distance);
						XMVECTOR& nor = XMVector3Normalize( XMVector3Cross( XMVectorSubtract(V1,V0),XMVectorSubtract(V2,V1) ) );
						Picked picked = Picked();
						picked.object = object;
						XMStoreFloat3(&picked.position,pos);
						XMStoreFloat3(&picked.normal,nor);
						pickPoints.push_back(picked);
					}
				}

			}
		}

		if(!pickPoints.empty()){
			XMFLOAT3 eye;
			XMStoreFloat3(&eye,rayOrigin);
			Picked min = pickPoints.front();
			float mini = WickedMath::DistanceEstimated(min.position,eye);
			for(int i=1;i<pickPoints.size();++i){
				if(float nm = WickedMath::DistanceEstimated(pickPoints[i].position,eye)<mini){
					min=pickPoints[i];
					mini=nm;
				}
			}
			return min;
		}

	}

	return Picked();
}

RAY Renderer::getPickRay(long cursorX, long cursorY){
	XMVECTOR& lineStart = XMVector3Unproject(XMVectorSet(cursorX,cursorY,0,1),0,0
		,SCREENWIDTH,SCREENHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());
	XMVECTOR& lineEnd = XMVector3Unproject(XMVectorSet(cursorX,cursorY,1,1),0,0
		,SCREENWIDTH,SCREENHEIGHT,0.1f,1.0f,Renderer::cam->Projection,Renderer::cam->View,XMMatrixIdentity());
	XMVECTOR& rayOrigin = lineStart, rayDirection = XMVector3Normalize(XMVectorSubtract(lineEnd,lineStart));
	return RAY(lineStart,rayDirection);
}

void Renderer::LoadModel(const string& dir, const string& name, const XMMATRIX& transform, const string& ident, PHYSICS* physicsEngine){
	static int unique_identifier = 0;

	vector<Object*> newObjects(0),newObjects_trans(0),newObjects_water(0);
	vector<Light*> newLights(0);
	stringstream idss("");
	idss<<"_"/*<<unique_identifier<<"_"*/<<ident;

	LoadFromDisk(dir,name,idss.str(),armatures,materials,newObjects,newObjects_trans,newObjects_water
		,meshes,newLights,vector<HitSphere*>(),WorldInfo(),Wind(),vector<ActionCamera>()
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


Renderer::MaterialCB::MaterialCB(const Material& mat,UINT materialIndex){
	difColor=XMFLOAT4(mat.diffuseColor.x,mat.diffuseColor.y,mat.diffuseColor.z,mat.alpha);
	hasRef=mat.refMap!=nullptr;
	hasNor=mat.normalMap!=nullptr;
	hasTex=mat.texture!=nullptr;
	hasSpe=mat.specularMap!=nullptr;
	specular=mat.specular;
	refractionIndex=mat.refraction_index;
	movingTex=mat.texOffset;
	metallic=mat.enviroReflection;
	shadeless=mat.shadeless;
	specular_power=mat.specular_power;
	toon=mat.toonshading;
	matIndex=materialIndex;
	emit=mat.emit;
}