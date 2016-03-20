#include "wiDepthTarget.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;

wiDepthTarget::wiDepthTarget()
{
	texture = nullptr;
	textureCube = nullptr;
	isCube = false;
}


wiDepthTarget::~wiDepthTarget()
{
	SAFE_DELETE(texture);
	SAFE_DELETE(textureCube);
}

void wiDepthTarget::Initialize(int width, int height, UINT MSAAC, UINT MSAAQ)
{
	SAFE_DELETE(texture);
	SAFE_DELETE(textureCube);

	isCube = false;

	Texture2DDesc depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = width;
	depthBufferDesc.Height = height;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = FORMAT_R24G8_TYPELESS;
	depthBufferDesc.SampleDesc.Count = MSAAC;
	depthBufferDesc.SampleDesc.Quality = MSAAQ;
	depthBufferDesc.Usage = USAGE_DEFAULT;
	depthBufferDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	wiRenderer::GetDevice()->CreateTexture2D(&depthBufferDesc, nullptr, &texture);
}
void wiDepthTarget::InitializeCube(int size)
{
	SAFE_DELETE(texture);
	SAFE_DELETE(textureCube);

	isCube = true;

	Texture2DDesc depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = size;
	depthBufferDesc.Height = size;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 6;
	depthBufferDesc.Format = FORMAT_R32_TYPELESS;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = USAGE_DEFAULT;
	depthBufferDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;

	wiRenderer::GetDevice()->CreateTextureCube(&depthBufferDesc, nullptr, &textureCube);
}

void wiDepthTarget::Clear(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->ClearDepthStencil(GetTexture(), CLEAR_DEPTH | CLEAR_STENCIL, 1.0f, 0);
}
void wiDepthTarget::CopyFrom(const wiDepthTarget& from, GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->CopyTexture2D(GetTexture(), from.GetTexture(), threadID);
}

