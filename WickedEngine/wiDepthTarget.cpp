#include "wiDepthTarget.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;

wiDepthTarget::wiDepthTarget()
{
	texture = nullptr;
}


wiDepthTarget::~wiDepthTarget()
{
	SAFE_DELETE(texture);
}

void wiDepthTarget::Initialize(int width, int height, UINT MSAAC)
{
	SAFE_DELETE(texture);

	Texture2DDesc depthGPUBufferDesc;
	ZeroMemory(&depthGPUBufferDesc, sizeof(depthGPUBufferDesc));

	// Set up the description of the depth buffer.
	depthGPUBufferDesc.Width = width;
	depthGPUBufferDesc.Height = height;
	depthGPUBufferDesc.MipLevels = 1;
	depthGPUBufferDesc.ArraySize = 1;
	depthGPUBufferDesc.Format = FORMAT_R24G8_TYPELESS;
	depthGPUBufferDesc.SampleDesc.Count = MSAAC;
	// depthGPUBufferDesc.SampleDesc.Quality = 0; // auto-filll in device
	depthGPUBufferDesc.Usage = USAGE_DEFAULT;
	depthGPUBufferDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	depthGPUBufferDesc.CPUAccessFlags = 0;
	depthGPUBufferDesc.MiscFlags = 0;

	wiRenderer::GetDevice()->CreateTexture2D(&depthGPUBufferDesc, nullptr, &texture);
}
void wiDepthTarget::InitializeCube(int size, bool independentFaces)
{
	SAFE_DELETE(texture);

	Texture2DDesc depthGPUBufferDesc;
	ZeroMemory(&depthGPUBufferDesc, sizeof(depthGPUBufferDesc));

	// Set up the description of the depth buffer.
	depthGPUBufferDesc.Width = size;
	depthGPUBufferDesc.Height = size;
	depthGPUBufferDesc.MipLevels = 1;
	depthGPUBufferDesc.ArraySize = 6;
	depthGPUBufferDesc.Format = FORMAT_R32_TYPELESS;
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
	wiRenderer::GetDevice()->ClearDepthStencil(GetTexture(), CLEAR_DEPTH | CLEAR_STENCIL, 1.0f, 0, threadID);
}
void wiDepthTarget::CopyFrom(const wiDepthTarget& from, GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->CopyTexture2D(GetTexture(), from.GetTexture(), threadID);
}

