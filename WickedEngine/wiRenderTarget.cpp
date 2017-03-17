#include "wiRenderTarget.h"
#include "wiRenderer.h"
#include "wiDepthTarget.h"

using namespace wiGraphicsTypes;


wiRenderTarget::wiRenderTarget()
{
	numViews = 0;
	depth = nullptr;
	resolvedMSAAUptodate = false;
}
wiRenderTarget::wiRenderTarget(UINT width, UINT height, bool hasDepth, FORMAT format, UINT mipMapLevelCount, UINT MSAAC, bool depthOnly)
{
	numViews = 0;
	depth = nullptr;
	resolvedMSAAUptodate = false;
	Initialize(width, height, hasDepth, format, mipMapLevelCount, MSAAC);
}


wiRenderTarget::~wiRenderTarget()
{
	CleanUp();
}

void wiRenderTarget::CleanUp() {
	for (size_t i = 0; i < renderTargets.size(); ++i)
	{
		SAFE_DELETE(renderTargets[i]);
	}
	for (size_t i = 0; i < renderTargets_resolvedMSAA.size(); ++i)
	{
		SAFE_DELETE(renderTargets_resolvedMSAA[i]);
	}
	renderTargets.clear();
	renderTargets_resolvedMSAA.clear();
	SAFE_DELETE(depth);
	resolvedMSAAUptodate = false;
}

void wiRenderTarget::Initialize(UINT width, UINT height, bool hasDepth
	, FORMAT format, UINT mipMapLevelCount, UINT MSAAC, bool depthOnly)
{
	CleanUp();

	if (!depthOnly)
	{
		Texture2DDesc textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = mipMapLevelCount;
		textureDesc.ArraySize = 1;
		textureDesc.Format = format;
		textureDesc.SampleDesc.Count = MSAAC;
		//textureDesc.SampleDesc.Quality = 0; // auto-fill in device to maximum
		textureDesc.Usage = USAGE_DEFAULT;
		textureDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		numViews = 1;
		Texture2D* texture = new Texture2D;
		if (mipMapLevelCount != 1)
		{
			texture->RequestIndepententShaderResourcesForMIPs(true);
			texture->RequestIndepententUnorderedAccessResourcesForMIPs(true);
			textureDesc.BindFlags |= BIND_UNORDERED_ACCESS;
			textureDesc.MiscFlags = RESOURCE_MISC_GENERATE_MIPS;
		}
		renderTargets.push_back(texture);
		wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &renderTargets[0]);
		if (MSAAC > 1)
		{
			textureDesc.SampleDesc.Count = 1;
			renderTargets_resolvedMSAA.push_back(nullptr);
			wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &renderTargets_resolvedMSAA[0]);
		}
	}
	
	viewPort.Width = (FLOAT)width;
	viewPort.Height = (FLOAT)height;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;

	if(hasDepth) {
		depth = new wiDepthTarget();
		depth->Initialize(width,height,MSAAC);
	}
}
void wiRenderTarget::InitializeCube(UINT size, bool hasDepth, FORMAT format, UINT mipMapLevelCount, bool depthOnly)
{
	CleanUp();

	if (!depthOnly)
	{
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

		numViews = 1;
		renderTargets.push_back(nullptr);
		wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &renderTargets[0]);
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
void wiRenderTarget::Add(FORMAT format)
{
	Texture2DDesc desc = GetTexture(0)->GetDesc();
	desc.Format = format;

	if (!renderTargets.empty())
	{
		numViews++;
		renderTargets.push_back(nullptr);
		wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &renderTargets.back());
		if (desc.SampleDesc.Count > 1)
		{
			desc.SampleDesc.Count = 1;
			renderTargets_resolvedMSAA.push_back(nullptr);
			wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &renderTargets_resolvedMSAA[0]);
		}
	}
	else
	{
		assert(0 && "Rendertarget Add failed because it is not properly initilaized!");
	}

}

void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, bool disableColor)
{
	Activate(threadID,0,0,0,0,disableColor);
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, float r, float g, float b, float a, bool disableColor)
{
	Set(threadID, disableColor);
	float ClearColor[4] = { r, g, b, a };
	for(int i=0;i<numViews;++i)
		wiRenderer::GetDevice()->ClearRenderTarget(GetTexture(i), ClearColor, threadID);
	if(depth) depth->Clear(threadID);
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, float r, float g, float b, float a, bool disableColor)
{
	Set(threadID,getDepth, disableColor);
	float ClearColor[4] = { r, g, b, a };
	for(int i=0;i<numViews;++i)
		wiRenderer::GetDevice()->ClearRenderTarget(GetTexture(i), ClearColor, threadID);
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, bool disableColor)
{
	Activate(threadID,getDepth,0,0,0,0, disableColor);
}
void wiRenderTarget::Deactivate(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);
}
void wiRenderTarget::Set(GRAPHICSTHREAD threadID, bool disableColor)
{
	wiRenderer::GetDevice()->BindViewports(1, &viewPort, threadID);
	wiRenderer::GetDevice()->BindRenderTargets(disableColor ? 0 : numViews, disableColor ? nullptr : (Texture**)renderTargets.data(), (depth ? depth->GetTexture() : nullptr), threadID);
	resolvedMSAAUptodate = false;
}
void wiRenderTarget::Set(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, bool disableColor)
{
	wiRenderer::GetDevice()->BindViewports(1, &viewPort, threadID);
	wiRenderer::GetDevice()->BindRenderTargets(disableColor ? 0 : numViews, disableColor ? nullptr : (Texture**)renderTargets.data(), (getDepth ? getDepth->GetTexture() : nullptr), threadID);
	resolvedMSAAUptodate = false;
}


Texture2D* wiRenderTarget::GetTextureResolvedMSAA(GRAPHICSTHREAD threadID, int viewID)
{ 
	if (GetDesc(viewID).SampleDesc.Count > 1)
	{
		if (!resolvedMSAAUptodate)
		{
			wiRenderer::GetDevice()->MSAAResolve(renderTargets_resolvedMSAA[viewID], renderTargets[viewID], threadID);
			resolvedMSAAUptodate = true;
		}
		return renderTargets_resolvedMSAA[viewID];
	}

	return renderTargets[viewID];
}
UINT wiRenderTarget::GetMipCount()
{
	Texture2DDesc desc = GetDesc();

	if (desc.MipLevels>0)
		return desc.MipLevels;

	UINT maxDim = max(desc.Width, desc.Height);
	return static_cast<UINT>( log2(static_cast<double>(maxDim)) );
}

