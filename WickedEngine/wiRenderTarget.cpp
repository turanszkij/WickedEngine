#include "wiRenderTarget.h"
#include "wiRenderer.h"
#include "wiDepthTarget.h"

using namespace wiGraphicsTypes;

void wiRenderTarget::Initialize(UINT width, UINT height, bool hasDepth
	, FORMAT format, UINT mipMapLevelCount, UINT MSAAC, bool depthOnly)
{
	slots.clear();
	slots.reserve(8); // graphics types cannot be moved!

	if (!depthOnly)
	{
		TextureDesc textureDesc;
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

		slots.emplace_back();

		if (mipMapLevelCount != 1)
		{
			slots.back().target_primary.RequestIndependentShaderResourcesForMIPs(true);
			slots.back().target_primary.RequestIndependentUnorderedAccessResourcesForMIPs(true);
			textureDesc.BindFlags |= BIND_UNORDERED_ACCESS;
		}
		wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &slots.back().target_primary);
		if (MSAAC > 1)
		{
			textureDesc.SampleDesc.Count = 1;
			wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &slots.back().target_resolved);
		}
	}

	viewPort.Width = (FLOAT)width;
	viewPort.Height = (FLOAT)height;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;

	if (hasDepth)
	{
		depth.reset(new wiDepthTarget);
		depth->Initialize(width, height, MSAAC);
	}
}
void wiRenderTarget::InitializeCube(UINT size, bool hasDepth, FORMAT format, UINT mipMapLevelCount, bool depthOnly)
{
	slots.clear();
	slots.reserve(8); // graphics types cannot be moved!

	if (!depthOnly)
	{
		TextureDesc textureDesc;
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

		slots.emplace_back();

		if (mipMapLevelCount != 1)
		{
			slots.back().target_primary.RequestIndependentShaderResourcesForMIPs(true);
			slots.back().target_primary.RequestIndependentUnorderedAccessResourcesForMIPs(true);
			textureDesc.BindFlags |= BIND_UNORDERED_ACCESS;
			//textureDesc.MiscFlags |= RESOURCE_MISC_GENERATE_MIPS;
		}
		wiRenderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &slots.back().target_primary);
	}

	viewPort.Width = (FLOAT)size;
	viewPort.Height = (FLOAT)size;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;

	if (hasDepth) 
	{
		depth.reset(new wiDepthTarget);
		depth->InitializeCube(size);
	}
}
void wiRenderTarget::Add(FORMAT format)
{
	TextureDesc desc = GetTexture(0).GetDesc();
	desc.Format = format;

	if (!slots.empty())
	{
		slots.emplace_back();

		wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &slots.back().target_primary);
		if (desc.SampleDesc.Count > 1)
		{
			desc.SampleDesc.Count = 1;
			wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &slots.back().target_resolved);
		}
	}
	else
	{
		assert(0 && "Rendertarget Add failed because it is not properly initialized!");
	}

}

void wiRenderTarget::SetAndClear(GRAPHICSTHREAD threadID, bool disableColor, int viewID)
{
	SetAndClear(threadID,0,0,0,0,disableColor, viewID);
}
void wiRenderTarget::SetAndClear(GRAPHICSTHREAD threadID, float r, float g, float b, float a, bool disableColor, int viewID)
{
	Set(threadID, disableColor, viewID);
	if (!disableColor)
	{
		float ClearColor[4] = { r, g, b, a };
		if (viewID >= 0)
		{
			wiRenderer::GetDevice()->ClearRenderTarget(&GetTexture(viewID), ClearColor, threadID);
		}
		else
		{
			for (int i = 0; i < (int)slots.size(); ++i)
			{
				wiRenderer::GetDevice()->ClearRenderTarget(&GetTexture(i), ClearColor, threadID);
			}
		}
	}
	if (depth)
	{
		depth->Clear(threadID);
	}
}
void wiRenderTarget::SetAndClear(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, float r, float g, float b, float a, bool disableColor, int viewID)
{
	Set(threadID,getDepth, disableColor, viewID);
	if (!disableColor)
	{
		float ClearColor[4] = { r, g, b, a };
		if (viewID >= 0)
		{
			wiRenderer::GetDevice()->ClearRenderTarget(&GetTexture(viewID), ClearColor, threadID);
		}
		else
		{
			for (int i = 0; i < (int)slots.size(); ++i)
			{
				wiRenderer::GetDevice()->ClearRenderTarget(&GetTexture(i), ClearColor, threadID);
			}
		}
	}
}
void wiRenderTarget::SetAndClear(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, bool disableColor, int viewID)
{
	SetAndClear(threadID,getDepth,0,0,0,0, disableColor, viewID);
}
void wiRenderTarget::Deactivate(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->BindRenderTargets(0, nullptr, nullptr, threadID);
}
void wiRenderTarget::Set(GRAPHICSTHREAD threadID, bool disableColor, int viewID)
{
	wiRenderer::GetDevice()->BindViewports(1, &viewPort, threadID);

	const Texture2D* rts[8];
	if (viewID >= 0)
	{
		rts[0] = &slots[viewID].target_primary;
		wiRenderer::GetDevice()->BindRenderTargets(disableColor ? 0 : 1, disableColor ? nullptr : rts, (depth ? &depth->GetTexture() : nullptr), threadID);
		slots[viewID].dirty = true;
	}
	else
	{
		for (int i = 0; i < (int)slots.size(); ++i)
		{
			slots[i].dirty = true;
			rts[i] = &slots[i].target_primary;
		}
		wiRenderer::GetDevice()->BindRenderTargets(disableColor ? 0 : (UINT)slots.size(), disableColor ? nullptr : rts, (depth ? &depth->GetTexture() : nullptr), threadID);
	}
}
void wiRenderTarget::Set(GRAPHICSTHREAD threadID, wiDepthTarget* getDepth, bool disableColor, int viewID)
{
	wiRenderer::GetDevice()->BindViewports(1, &viewPort, threadID);

	const Texture2D* rts[8];
	if (viewID >= 0)
	{
		rts[0] = &slots[viewID].target_primary;
		wiRenderer::GetDevice()->BindRenderTargets(disableColor ? 0 : 1, disableColor ? nullptr : rts, (getDepth ? &getDepth->GetTexture() : nullptr), threadID);
		slots[viewID].dirty = true;
	}
	else
	{
		for (int i = 0; i < (int)slots.size(); ++i)
		{
			slots[i].dirty = true;
			rts[i] = &slots[i].target_primary;
		}
		wiRenderer::GetDevice()->BindRenderTargets(disableColor ? 0 : (UINT)slots.size(), disableColor ? nullptr : rts, (getDepth ? &getDepth->GetTexture() : nullptr), threadID);
	}
}


const Texture2D& wiRenderTarget::GetTextureResolvedMSAA(GRAPHICSTHREAD threadID, int viewID)
{
	if (GetDesc(viewID).SampleDesc.Count > 1)
	{
		if (slots[viewID].dirty)
		{
			wiRenderer::GetDevice()->MSAAResolve(&slots[viewID].target_resolved, &slots[viewID].target_primary, threadID);
			slots[viewID].dirty = false;
		}
		return slots[viewID].target_resolved;
	}

	return slots[viewID].target_primary;
}
UINT wiRenderTarget::GetMipCount()
{
	TextureDesc desc = GetDesc();

	if (desc.MipLevels>0)
		return desc.MipLevels;

	UINT maxDim = max(desc.Width, desc.Height);
	return static_cast<UINT>( log2(static_cast<double>(maxDim)) );
}

