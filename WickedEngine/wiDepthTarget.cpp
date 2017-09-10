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
	SAFE_DELETE(texture);
	SAFE_DELETE(texture_resolvedMSAA);
}

void wiDepthTarget::Initialize(int width, int height, UINT MSAAC)
{
	SAFE_DELETE(texture);
	SAFE_DELETE(texture_resolvedMSAA);
	resolvedMSAAUptodate = false;

	Texture2DDesc depthDesc;
	ZeroMemory(&depthDesc, sizeof(depthDesc));

	// Set up the description of the depth buffer.
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	//depthDesc.Format = FORMAT_R24G8_TYPELESS;
	depthDesc.Format = FORMAT_R32G8X24_TYPELESS; // better utilization of hardware
	depthDesc.SampleDesc.Count = MSAAC;
	// depthDesc.SampleDesc.Quality = 0; // auto-filll in device
	depthDesc.Usage = USAGE_DEFAULT;
	depthDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;

	wiRenderer::GetDevice()->CreateTexture2D(&depthDesc, nullptr, &texture);

	if (MSAAC > 1)
	{
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Format = FORMAT_R32_FLOAT;
		depthDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		wiRenderer::GetDevice()->CreateTexture2D(&depthDesc, nullptr, &texture_resolvedMSAA);
	}
}
void wiDepthTarget::InitializeCube(int size, bool independentFaces)
{
	SAFE_DELETE(texture);
	SAFE_DELETE(texture_resolvedMSAA);
	resolvedMSAAUptodate = false;

	Texture2DDesc depthGPUBufferDesc;
	ZeroMemory(&depthGPUBufferDesc, sizeof(depthGPUBufferDesc));

	// Set up the description of the depth buffer.
	depthGPUBufferDesc.Width = size;
	depthGPUBufferDesc.Height = size;
	depthGPUBufferDesc.MipLevels = 1;
	depthGPUBufferDesc.ArraySize = 6;
	depthGPUBufferDesc.Format = FORMAT_R16_TYPELESS;
	depthGPUBufferDesc.SampleDesc.Count = 1;
	depthGPUBufferDesc.SampleDesc.Quality = 0;
	depthGPUBufferDesc.Usage = USAGE_DEFAULT;
	depthGPUBufferDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	depthGPUBufferDesc.CPUAccessFlags = 0;
	depthGPUBufferDesc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;

	texture = new Texture2D;
	texture->RequestIndepententRenderTargetCubemapFaces(independentFaces);
	wiRenderer::GetDevice()->CreateTexture2D(&depthGPUBufferDesc, nullptr, &texture);
}

void wiDepthTarget::Clear(GRAPHICSTHREAD threadID)
{
	float depthClear = GetDesc().MiscFlags & RESOURCE_MISC_TEXTURECUBE ? 1.0f : 0.0f;
	wiRenderer::GetDevice()->ClearDepthStencil(GetTexture(), CLEAR_DEPTH | CLEAR_STENCIL, depthClear, 0, threadID);
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
			wiRenderer::ResolveMSAADepthBuffer(texture_resolvedMSAA, texture, threadID);
			resolvedMSAAUptodate = true;
		}
		return texture_resolvedMSAA;
	}
	return texture;
}
