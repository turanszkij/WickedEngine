#include "wiRenderTarget.h"
#include "wiRenderer.h"
#include "wiDepthTarget.h"


wiRenderTarget::wiRenderTarget()
{
	numViews = 0;
	depth = nullptr;
	isCube = false;
}
wiRenderTarget::wiRenderTarget(UINT width, UINT height, int numViews, bool hasDepth, UINT MSAAC, UINT MSAAQ
	, DXGI_FORMAT format, UINT mipMapLevelCount)
{
	numViews = 0;
	depth = nullptr;
	isCube = false;
	Initialize(width, height, numViews, hasDepth, MSAAC, MSAAQ, format, mipMapLevelCount);
}


wiRenderTarget::~wiRenderTarget()
{
	clear();
}

void wiRenderTarget::clear() {
	for (unsigned int i = 0; i < renderTargets.size(); ++i)
	{
		SAFE_DELETE(renderTargets[i]);
	}
	for (unsigned int i = 0; i < renderTargets_Cube.size(); ++i)
	{
		SAFE_DELETE(renderTargets_Cube[i]);
	}
	SAFE_DELETE(depth);
}

void wiRenderTarget::Initialize(UINT width, UINT height, int numViews, bool hasDepth, UINT MSAAC, UINT MSAAQ
	, DXGI_FORMAT format, UINT mipMapLevelCount)
{
	clear();

	isCube = false;

	this->numViews = numViews;
	//texture2D.resize(numViews);
	//renderTarget.resize(numViews);
	//shaderResource.resize(numViews);
	//SAVEDshaderResource.resize(numViews);
	renderTargets.resize(numViews);

	Texture2DDesc textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = mipMapLevelCount;
	textureDesc.ArraySize = 1;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = MSAAC;
	textureDesc.SampleDesc.Quality = MSAAQ;
	textureDesc.Usage = USAGE_DEFAULT;
	textureDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	if (mipMapLevelCount != 1)
	{
		textureDesc.MiscFlags = RESOURCE_MISC_GENERATE_MIPS;
	}
	
	//RenderTargetViewDesc renderTargetViewDesc;
	//renderTargetViewDesc.Format = format;
	//renderTargetViewDesc.ViewDimension = (MSAAQ==0 ? RESOURCE_DIMENSION_TEXTURE2D : RESOURCE_DIMENSION_TEXTURE2DMS);
	////renderTargetViewDesc.Texture2D.MipSlice = 0;

	//
	//ShaderResourceViewDesc shaderResourceViewDesc;
	//shaderResourceViewDesc.Format = format;
	//shaderResourceViewDesc.ViewDimension = (MSAAQ==0 ? RESOURCE_DIMENSION_TEXTURE2D : RESOURCE_DIMENSION_TEXTURE2DMS);
	////shaderResourceViewDesc.Texture2D.MostDetailedMip = 0; //from most detailed...
	////shaderResourceViewDesc.Texture2D.MipLevels = -1; //...to least detailed
	//shaderResourceViewDesc.mipLevels = -1;

	//for(int i=0;i<numViews;++i){
	//	wiRenderer::graphicsDevice->CreateTexture2D(&textureDesc, nullptr, &texture2D[i]);
	//	wiRenderer::graphicsDevice->CreateRenderTargetView(texture2D[i], &renderTargetViewDesc, &renderTarget[i]);
	//	wiRenderer::graphicsDevice->CreateShaderResourceView(texture2D[i], &shaderResourceViewDesc, &shaderResource[i]);
	//}
	for (int i = 0; i < numViews; ++i)
	{
		wiRenderer::graphicsDevice->CreateTexture2D(&textureDesc, nullptr, &renderTargets[i]);
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
void wiRenderTarget::InitializeCube(UINT size, int numViews, bool hasDepth, DXGI_FORMAT format, UINT mipMapLevelCount)
{
	clear();

	isCube = true;

	this->numViews = numViews;
	//texture2D.resize(numViews);
	//renderTarget.resize(numViews);
	//shaderResource.resize(numViews);
	//SAVEDshaderResource.resize(numViews);
	renderTargets_Cube.resize(numViews);
	
	Texture2DDesc textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = size;
	textureDesc.Height = size;
	textureDesc.MipLevels = mipMapLevelCount;
	textureDesc.ArraySize = 6;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = USAGE_DEFAULT;
	textureDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;
	if (mipMapLevelCount != 1)
	{
		textureDesc.MiscFlags |= RESOURCE_MISC_GENERATE_MIPS;
	}

	
	//RenderTargetViewDesc renderTargetViewDesc;
	//renderTargetViewDesc.Format = format;
	//renderTargetViewDesc.ViewDimension = RESOURCE_DIMENSION_TEXTURE2DARRAY;
 ////   renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
	////renderTargetViewDesc.Texture2DArray.ArraySize = 6;
 ////   renderTargetViewDesc.Texture2DArray.MipSlice = 0;
	//renderTargetViewDesc.ArraySize = 6;
	//
	//ShaderResourceViewDesc shaderResourceViewDesc;
	//shaderResourceViewDesc.Format = format;
	//shaderResourceViewDesc.ViewDimension = RESOURCE_DIMENSION_TEXTURECUBE;
	////shaderResourceViewDesc.TextureCube.MostDetailedMip = 0; //from most detailed...
	////shaderResourceViewDesc.TextureCube.MipLevels = -1; //...to least detailed
	//shaderResourceViewDesc.mipLevels = -1;

	//for(int i=0;i<numViews;++i){
	//	wiRenderer::graphicsDevice->CreateTexture2D(&textureDesc, NULL, &texture2D[i]);
	//	wiRenderer::graphicsDevice->CreateRenderTargetView(texture2D[i], &renderTargetViewDesc, &renderTarget[i]);
	//	wiRenderer::graphicsDevice->CreateShaderResourceView(texture2D[i], &shaderResourceViewDesc, &shaderResource[0]);
	//}

	for (int i = 0; i < numViews; ++i)
	{
		wiRenderer::graphicsDevice->CreateTextureCube(&textureDesc, nullptr, &renderTargets_Cube[i]);
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

void wiRenderTarget::Activate(GRAPHICSTHREAD threadID)
{
	Activate(threadID,0,0,0,0);
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, float r, float g, float b, float a)
{
	Set(threadID);
	float ClearColor[4] = { r, g, b, a };
	for(int i=0;i<numViews;++i)
		wiRenderer::graphicsDevice->ClearRenderTarget(GetTexture(i), ClearColor);
	if(depth) depth->Clear(threadID);
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, float r, float g, float b, float a)
{
	Set(threadID,getDepth);
	float ClearColor[4] = { r, g, b, a };
	for(int i=0;i<numViews;++i)
		wiRenderer::graphicsDevice->ClearRenderTarget(GetTexture(i), ClearColor);
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth)
{
	Activate(threadID,getDepth,0,0,0,0);
}
void wiRenderTarget::Deactivate(GRAPHICSTHREAD threadID)
{
	wiRenderer::graphicsDevice->BindRenderTargets(0, nullptr, nullptr);
}
void wiRenderTarget::Set(GRAPHICSTHREAD threadID)
{
	wiRenderer::graphicsDevice->BindViewports(1, &viewPort);
	wiRenderer::graphicsDevice->BindRenderTargets(numViews, (isCube ? (Texture2D**)renderTargets_Cube.data() : renderTargets.data()), (depth ? depth->GetTexture() : nullptr));
}
void wiRenderTarget::Set(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth)
{
	depth = getDepth;
	wiRenderer::graphicsDevice->BindViewports(1, &viewPort);
	wiRenderer::graphicsDevice->BindRenderTargets(numViews, (isCube ? (Texture2D**)renderTargets_Cube.data() : renderTargets.data()), (depth ? depth->GetTexture() : nullptr));
}
//void wiRenderTarget::Retarget(Texture2D* resource)
//{
//	retargetted=true;
//	for(unsigned int i=0;i<shaderResource.size();++i){
//		SAVEDshaderResource[i]=shaderResource[i];
//		shaderResource[i]=resource;
//	}
//}
//void wiRenderTarget::Restore(){
//	if(retargetted){
//		for(unsigned int i=0;i<shaderResource.size();++i){
//			shaderResource[i]=SAVEDshaderResource[i];
//		}
//	}
//	retargetted=false;
//}

UINT wiRenderTarget::GetMipCount()
{
	//if (shaderResource.empty())
	//	return 0U;
	Texture2DDesc desc = GetDesc();

	if (desc.MipLevels>0)
		return desc.MipLevels;

	UINT maxDim = max(desc.Width, desc.Height);
	return static_cast<UINT>( log2(static_cast<double>(maxDim)) );
}

