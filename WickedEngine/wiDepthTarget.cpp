#include "wiDepthTarget.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;

wiDepthTarget::wiDepthTarget()
{
	texture = nullptr;
	texture_resolvedMSAA = nullptr;
	resolvedMSAAUptodate = false;
}


wiDepthTarget::~wiDepthTarget()
{
}

void wiDepthTarget::Initialize(int width, int height, UINT MSAAC)
{
	resolvedMSAAUptodate = false;

	TextureDesc depthDesc;

	// Set up the description of the depth buffer.
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = wiRenderer::DSFormat_full_alias;
	depthDesc.SampleDesc.Count = MSAAC;
	// depthDesc.SampleDesc.Quality = 0; // auto-filll in device
	depthDesc.Usage = USAGE_DEFAULT;
	depthDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;

	texture.reset(new Texture2D);
	wiRenderer::GetDevice()->CreateTexture2D(&depthDesc, nullptr, texture.get());

	if (MSAAC > 1)
	{
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Format = wiRenderer::RTFormat_depthresolve;
		depthDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		texture_resolvedMSAA.reset(new Texture2D);
		wiRenderer::GetDevice()->CreateTexture2D(&depthDesc, nullptr, texture_resolvedMSAA.get());
	}
}
void wiDepthTarget::InitializeCube(int size, bool independentFaces)
{
	resolvedMSAAUptodate = false;

	TextureDesc depthGPUBufferDesc;

	// Set up the description of the depth buffer.
	depthGPUBufferDesc.Width = size;
	depthGPUBufferDesc.Height = size;
	depthGPUBufferDesc.MipLevels = 1;
	depthGPUBufferDesc.ArraySize = 6;
	depthGPUBufferDesc.Format = wiRenderer::DSFormat_small_alias;
	depthGPUBufferDesc.SampleDesc.Count = 1;
	depthGPUBufferDesc.SampleDesc.Quality = 0;
	depthGPUBufferDesc.Usage = USAGE_DEFAULT;
	depthGPUBufferDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	depthGPUBufferDesc.CPUAccessFlags = 0;
	depthGPUBufferDesc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;

	texture.reset(new Texture2D);
	texture->RequestIndependentRenderTargetCubemapFaces(independentFaces);
	wiRenderer::GetDevice()->CreateTexture2D(&depthGPUBufferDesc, nullptr, texture.get());
}

void wiDepthTarget::Clear(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->ClearDepthStencil(GetTexture(), CLEAR_DEPTH | CLEAR_STENCIL, 0.0f, 0, threadID);
	resolvedMSAAUptodate = false;
}
void wiDepthTarget::CopyFrom(const wiDepthTarget& from, GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->CopyTexture2D(GetTexture(), from.GetTexture(), threadID);
	resolvedMSAAUptodate = false;
}

Texture2D* wiDepthTarget::GetTextureResolvedMSAA(GRAPHICSTHREAD threadID)
{
	if (texture_resolvedMSAA != nullptr)
	{
		if (!resolvedMSAAUptodate)
		{
			wiRenderer::ResolveMSAADepthBuffer(texture_resolvedMSAA.get(), texture.get(), threadID);
			resolvedMSAAUptodate = true;
		}
		return texture_resolvedMSAA.get();
	}
	return texture.get();
}
