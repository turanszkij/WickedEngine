#include "wiRenderTarget.h"
#include "wiRenderer.h"
#include "wiDepthTarget.h"


void wiRenderTarget::clear(){
	textureDesc = { 0 };
	numViews = 0;
	viewPort = D3D11_VIEWPORT();
	depth = NULL;
	retargetted = false;
}

wiRenderTarget::wiRenderTarget()
{
	clear();
}
wiRenderTarget::wiRenderTarget(UINT width, UINT height, int numViews, bool hasDepth, UINT MSAAC, UINT MSAAQ
	, DXGI_FORMAT format, UINT mipMapLevelCount)
{
	clear();
	Initialize(width, height, numViews, hasDepth, MSAAC, MSAAQ, format, mipMapLevelCount);
}


wiRenderTarget::~wiRenderTarget()
{
	for(int i=0;i<numViews;++i){
		if(texture2D[i]) texture2D[i]->Release(); texture2D[i] = NULL;
		if(renderTarget[i]) renderTarget[i]->Release(); renderTarget[i] = NULL;
		if(SAVEDshaderResource[i]) SAVEDshaderResource[i]->Release(); SAVEDshaderResource[i]=NULL;
		if(shaderResource[i] && !retargetted) shaderResource[i]->Release(); shaderResource[i] = NULL;
	}
	texture2D.clear();
	renderTarget.clear();
	shaderResource.clear();
	if(depth) depth->~wiDepthTarget(); depth=NULL;
}

void wiRenderTarget::Initialize(UINT width, UINT height, int numViews, bool hasDepth, UINT MSAAC, UINT MSAAQ
	, DXGI_FORMAT format, UINT mipMapLevelCount)
{
	this->numViews = numViews;
	texture2D.resize(numViews);
	renderTarget.resize(numViews);
	shaderResource.resize(numViews);
	SAVEDshaderResource.resize(numViews);
	
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = mipMapLevelCount;
	textureDesc.ArraySize = 1;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = MSAAC;
	textureDesc.SampleDesc.Quality = MSAAQ;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	if (mipMapLevelCount != 1)
	{
		textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}
	
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = format;
	renderTargetViewDesc.ViewDimension = (MSAAQ==0 ? D3D11_RTV_DIMENSION_TEXTURE2D : D3D11_RTV_DIMENSION_TEXTURE2DMS);
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = format;
	shaderResourceViewDesc.ViewDimension = (MSAAQ==0 ? D3D11_SRV_DIMENSION_TEXTURE2D : D3D11_SRV_DIMENSION_TEXTURE2DMS);
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0; //from most detailed...
	shaderResourceViewDesc.Texture2D.MipLevels = -1; //...to least detailed

	for(int i=0;i<numViews;++i){
		wiRenderer::graphicsDevice->CreateTexture2D(&textureDesc, nullptr, &texture2D[i]);
		wiRenderer::graphicsDevice->CreateRenderTargetView(texture2D[i], &renderTargetViewDesc, &renderTarget[i]);
		wiRenderer::graphicsDevice->CreateShaderResourceView(texture2D[i], &shaderResourceViewDesc, &shaderResource[i]);
	}
	
	viewPort.Width = (FLOAT)width;
	viewPort.Height = (FLOAT)height;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;

	if(hasDepth) {
		depth = new wiDepthTarget();
		depth->Initialize(width,height,MSAAC,MSAAQ);
	}
}
void wiRenderTarget::InitializeCube(UINT size, int numViews, bool hasDepth, DXGI_FORMAT format)
{
	this->numViews = numViews;
	texture2D.resize(numViews);
	renderTarget.resize(numViews);
	shaderResource.resize(numViews);
	SAVEDshaderResource.resize(numViews);
	
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = size;
	textureDesc.Height = size;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 6;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
	renderTargetViewDesc.Texture2DArray.ArraySize = 6;
    renderTargetViewDesc.Texture2DArray.MipSlice = 0;

	
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	shaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
	shaderResourceViewDesc.TextureCube.MipLevels = 1;

	for(int i=0;i<numViews;++i){
		wiRenderer::graphicsDevice->CreateTexture2D(&textureDesc, NULL, &texture2D[i]);
		wiRenderer::graphicsDevice->CreateRenderTargetView(texture2D[i], &renderTargetViewDesc, &renderTarget[i]);
		wiRenderer::graphicsDevice->CreateShaderResourceView(texture2D[i], &shaderResourceViewDesc, &shaderResource[0]);
	}
	
	viewPort.Width = (FLOAT)size;
	viewPort.Height = (FLOAT)size;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;

	if(hasDepth) {
		depth = new wiDepthTarget();
		depth->InitializeCube(size);
	}
}
void wiRenderTarget::InitializeCube(UINT size, int numViews, bool hasDepth)
{
	InitializeCube(size,numViews,hasDepth,DXGI_FORMAT_R8G8B8A8_UNORM);
}

void wiRenderTarget::Activate(ID3D11DeviceContext* context)
{
	Activate(context,0,0,0,0);
}
void wiRenderTarget::Activate(ID3D11DeviceContext* context, float r, float g, float b, float a)
{
	if(context!=nullptr){
		Set(context);
		float ClearColor[4] = { r, g, b, a }; // red,green,blue,alpha
		for(int i=0;i<numViews;++i)
			context->ClearRenderTargetView(renderTarget[i], ClearColor);
		if(depth) depth->Clear(context);
	}
}
void wiRenderTarget::Activate(ID3D11DeviceContext* context, wiDepthTarget* getDepth, float r, float g, float b, float a)
{
	if(context!=nullptr){
		Set(context,getDepth);
		float ClearColor[4] = { r, g, b, a }; // red,green,blue,alpha
		for(int i=0;i<numViews;++i)
			context->ClearRenderTargetView(renderTarget[i], ClearColor);
	}
}
void wiRenderTarget::Activate(ID3D11DeviceContext* context, wiDepthTarget* getDepth)
{
	Activate(context,getDepth,0,0,0,0);
}
void wiRenderTarget::Set(ID3D11DeviceContext* context)
{
	//ID3D11ShaderResourceView* t[]={nullptr};
	//context->PSSetShaderResources(6,1,t);
	if(context!=nullptr){
		context->RSSetViewports(1, &viewPort);
		context->OMSetRenderTargets(numViews, renderTarget.data(),(depth?depth->depthTarget:nullptr));
	}
}
void wiRenderTarget::Set(ID3D11DeviceContext* context, wiDepthTarget* getDepth)
{
	//ID3D11ShaderResourceView* t[]={nullptr};
	//context->PSSetShaderResources(6,1,t);
	if(context!=nullptr){
		depth = getDepth;
		context->RSSetViewports(1, &viewPort);
		context->OMSetRenderTargets(numViews, renderTarget.data(),(depth?depth->depthTarget:nullptr));
	}
}
void wiRenderTarget::Retarget(ID3D11ShaderResourceView* resource)
{
	retargetted=true;
	for(unsigned int i=0;i<shaderResource.size();++i){
		SAVEDshaderResource[i]=shaderResource[i];
		shaderResource[i]=resource;
	}
}
void wiRenderTarget::Restore(){
	if(retargetted){
		for(unsigned int i=0;i<shaderResource.size();++i){
			shaderResource[i]=SAVEDshaderResource[i];
		}
	}
	retargetted=false;
}

UINT wiRenderTarget::GetMipCount()
{
	if (shaderResource.empty())
		return 0U;

	if (textureDesc.MipLevels>0)
		return textureDesc.MipLevels;

	UINT maxDim = max(textureDesc.Width, textureDesc.Height);
	return static_cast<UINT>( log2(static_cast<double>(maxDim)) );
}

