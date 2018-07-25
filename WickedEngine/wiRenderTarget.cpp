#include "wiRenderTarget.h"
#include "wiRenderer.h"
#include "wiDepthTarget.h"

using namespace wiGraphicsTypes;


wiRenderTarget::wiRenderTarget()
{
	numViews = 0;
	depth = nullptr;
}
wiRenderTarget::wiRenderTarget(UINT width, UINT height, bool hasDepth, FORMAT format, UINT mipMapLevelCount, UINT MSAAC, bool depthOnly)
{
	numViews = 0;
	depth = nullptr;
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
	resolvedMSAAUptodate.clear();
}

void wiRenderTarget::Initialize(UINT width, UINT height, bool hasDepth
	, FORMAT format, UINT mipMapLevelCount, UINT MSAAC, bool depthOnly)
{
	CleanUp();

	if (!depthOnly)
	{
		TextureDesc textureDesc;
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
			texture->RequestIndependentShaderResourcesForMIPs(true);
			texture->RequestIndependentUnorderedAccessResourcesForMIPs(true);
			textureDesc.BindFlags |= BIND_UNORDERED_ACCESS;
			//textureDesc.MiscFlags = RESOURCE_MISC_GENERATE_MIPS;
		}
		renderTargets.push_back(texture);
		wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &renderTargets[0]);
		if (MSAAC > 1)
		{
			textureDesc.SampleDesc.Count = 1;
			renderTargets_resolvedMSAA.push_back(nullptr);
			wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &renderTargets_resolvedMSAA[0]);
			resolvedMSAAUptodate.push_back(false);
		}
		else
		{
			resolvedMSAAUptodate.push_back(true);
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
		TextureDesc textureDesc;
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

		Texture2D* texture = new Texture2D;
		if (mipMapLevelCount != 1)
		{
			texture->RequestIndependentShaderResourcesForMIPs(true);
			texture->RequestIndependentUnorderedAccessResourcesForMIPs(true);
			textureDesc.BindFlags |= BIND_UNORDERED_ACCESS;
			//textureDesc.MiscFlags |= RESOURCE_MISC_GENERATE_MIPS;
		}

		numViews = 1;
		renderTargets.push_back(texture);
		wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &renderTargets[0]);
		resolvedMSAAUptodate.push_back(true);
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
	TextureDesc desc = GetTexture(0)->GetDesc();
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
			wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &renderTargets_resolvedMSAA.back());
			resolvedMSAAUptodate.push_back(false);
		}
		else
		{
			resolvedMSAAUptodate.push_back(true);
		}
	}
	else
	{
		assert(0 && "Rendertarget Add failed because it is not properly initilaized!");
	}

}

void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, bool disableColor, int viewID)
{
	Activate(threadID,0,0,0,0,disableColor, viewID);
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, float r, float g, float b, float a, bool disableColor, int viewID)
{
	Set(threadID, disableColor, viewID);
	float ClearColor[4] = { r, g, b, a };
	if (viewID >= 0)
	{
		wiRenderer::GetDevice()->ClearRenderTarget(GetTexture(viewID), ClearColor, threadID);
	}
	else
	{
		for (int i = 0; i<numViews; ++i)
			wiRenderer::GetDevice()->ClearRenderTarget(GetTexture(i), ClearColor, threadID);
	}
	if(depth) depth->Clear(threadID);
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, float r, float g, float b, float a, bool disableColor, int viewID)
{
	Set(threadID,getDepth, disableColor, viewID);
	float ClearColor[4] = { r, g, b, a };
	if (viewID >= 0)
	{
		wiRenderer::GetDevice()->ClearRenderTarget(GetTexture(viewID), ClearColor, threadID);
	}
	else
	{
		for (int i = 0; i<numViews; ++i)
			wiRenderer::GetDevice()->ClearRenderTarget(GetTexture(i), ClearColor, threadID);
	}
}
void wiRenderTarget::Activate(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, bool disableColor, int viewID)
{
	Activate(threadID,getDepth,0,0,0,0, disableColor, viewID);
}
void wiRenderTarget::Deactivate(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);
}
void wiRenderTarget::Set(GRAPHICSTHREAD threadID, bool disableColor, int viewID)
{
	wiRenderer::GetDevice()->BindViewports(1, &viewPort, threadID);
	if (viewID >= 0)
	{
		wiRenderer::GetDevice()->BindRenderTargets(disableColor ? 0 : 1, disableColor ? nullptr : (Texture2D**)&renderTargets[viewID], (depth ? depth->GetTexture() : nullptr), threadID);
		resolvedMSAAUptodate[viewID] = false;
	}
	else
	{
		wiRenderer::GetDevice()->BindRenderTargets(disableColor ? 0 : numViews, disableColor ? nullptr : (Texture2D**)renderTargets.data(), (depth ? depth->GetTexture() : nullptr), threadID);
		for (auto& x : resolvedMSAAUptodate)
		{
			x = false;
		}
	}
}
void wiRenderTarget::Set(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, bool disableColor, int viewID)
{
	wiRenderer::GetDevice()->BindViewports(1, &viewPort, threadID);
	if (viewID >= 0)
	{
		wiRenderer::GetDevice()->BindRenderTargets(disableColor ? 0 : 1, disableColor ? nullptr : (Texture2D**)&renderTargets[viewID], (getDepth ? getDepth->GetTexture() : nullptr), threadID);
		resolvedMSAAUptodate[viewID] = false;
	}
	else
	{
		wiRenderer::GetDevice()->BindRenderTargets(disableColor ? 0 : numViews, disableColor ? nullptr : (Texture2D**)renderTargets.data(), (getDepth ? getDepth->GetTexture() : nullptr), threadID);
		for (auto& x : resolvedMSAAUptodate)
		{
			x = false;
		}
	}
}


Texture2D* wiRenderTarget::GetTextureResolvedMSAA(GRAPHICSTHREAD threadID, int viewID)
{
	if (GetDesc(viewID).SampleDesc.Count > 1)
	{
		if (resolvedMSAAUptodate[viewID] == false)
		{
			wiRenderer::GetDevice()->MSAAResolve(renderTargets_resolvedMSAA[viewID], renderTargets[viewID], threadID);
			resolvedMSAAUptodate[viewID] = true;
		}
		return renderTargets_resolvedMSAA[viewID];
	}

	return renderTargets[viewID];
}
UINT wiRenderTarget::GetMipCount()
{
	TextureDesc desc = GetDesc();

	if (desc.MipLevels>0)
		return desc.MipLevels;

	UINT maxDim = max(desc.Width, desc.Height);
	return static_cast<UINT>( log2(static_cast<double>(maxDim)) );
}

