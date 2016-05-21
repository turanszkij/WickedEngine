#include "wiRenderTarget.h"
#include "wiRenderer.h"
#include "wiDepthTarget.h"

using namespace wiGraphicsTypes;


wiRenderTarget::wiRenderTarget()
{
	numViews = 0;
	depth = nullptr;
	isCube = false;
}
wiRenderTarget::wiRenderTarget(UINT width, UINT height, int numViews, bool hasDepth, UINT MSAAC, UINT MSAAQ
	, FORMAT format, UINT mipMapLevelCount)
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
	, FORMAT format, UINT mipMapLevelCount)
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

	for (int i = 0; i < numViews; ++i)
	{
		wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &renderTargets[i]);
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
void wiRenderTarget::InitializeCube(UINT size, int numViews, bool hasDepth, FORMAT format, UINT mipMapLevelCount)
{
	clear();

	isCube = true;

	this->numViews = numViews;
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

	for (int i = 0; i < numViews; ++i)
	{
		wiRenderer::GetDevice()->CreateTextureCube(&textureDesc, nullptr, &renderTargets_Cube[i]);
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
	InitializeCube(size,numViews,hasDepth,FORMAT_R8G8B8A8_UNORM);
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
		wiRenderer::GetDevice()->ClearRenderTarget(GetTexture(i), ClearColor, threadID);
	if(depth) depth->Clear(threadID);
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, float r, float g, float b, float a)
{
	Set(threadID,getDepth);
	float ClearColor[4] = { r, g, b, a };
	for(int i=0;i<numViews;++i)
		wiRenderer::GetDevice()->ClearRenderTarget(GetTexture(i), ClearColor, threadID);
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth)
{
	Activate(threadID,getDepth,0,0,0,0);
}
void wiRenderTarget::Deactivate(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);
}
void wiRenderTarget::Set(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->BindViewports(1, &viewPort, threadID);
	wiRenderer::GetDevice()->BindRenderTargets(numViews, (isCube ? (Texture2D**)renderTargets_Cube.data() : renderTargets.data()), (depth ? depth->GetTexture() : nullptr), threadID);
}
void wiRenderTarget::Set(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth)
{
	depth = getDepth;
	wiRenderer::GetDevice()->BindViewports(1, &viewPort, threadID);
	wiRenderer::GetDevice()->BindRenderTargets(numViews, (isCube ? (Texture2D**)renderTargets_Cube.data() : renderTargets.data()), (depth ? depth->GetTexture() : nullptr), threadID);
}

UINT wiRenderTarget::GetMipCount()
{
	Texture2DDesc desc = GetDesc();

	if (desc.MipLevels>0)
		return desc.MipLevels;

	UINT maxDim = max(desc.Width, desc.Height);
	return static_cast<UINT>( log2(static_cast<double>(maxDim)) );
}

