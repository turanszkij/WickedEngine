#include "wiDepthTarget.h"
#include "wiRenderer.h"


wiDepthTarget::wiDepthTarget()
{
	texture2D = NULL;
	depthTarget = NULL;
	shaderResource = NULL;
}


wiDepthTarget::~wiDepthTarget()
{
	if(texture2D) texture2D->Release(); texture2D = NULL;
	if(depthTarget) depthTarget->Release(); depthTarget = NULL;
	if(shaderResource) shaderResource->Release(); shaderResource = NULL;
}

void wiDepthTarget::Initialize(int width, int height, UINT MSAAC, UINT MSAAQ)
{
	Texture2DDesc depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = width;
	depthBufferDesc.Height = height;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthBufferDesc.SampleDesc.Count = MSAAC;
	depthBufferDesc.SampleDesc.Quality = MSAAQ;
	depthBufferDesc.Usage = USAGE_DEFAULT;
	depthBufferDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	HRESULT hr;
	// Create the texture for the depth buffer using the filled out description.
	hr=wiRenderer::graphicsDevice->CreateTexture2D(&depthBufferDesc, nullptr, &texture2D);

	DepthStencilViewDesc depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = (MSAAQ==0 ? RESOURCE_DIMENSION_TEXTURE2D : RESOURCE_DIMENSION_TEXTURE2DMS);
	//depthStencilViewDesc.Texture2D.MipSlice = 0;
	depthStencilViewDesc.Flags = 0;

	// Create the depth stencil view.
	hr=wiRenderer::graphicsDevice->CreateDepthStencilView(texture2D, &depthStencilViewDesc, &depthTarget);

	//Create Depth ShaderResource
	//ShaderResourceViewDesc shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	shaderResourceViewDesc.ViewDimension = (MSAAQ==0 ? RESOURCE_DIMENSION_TEXTURE2D : RESOURCE_DIMENSION_TEXTURE2DMS);
	//shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	//shaderResourceViewDesc.Texture2D.MipLevels = 1;
	shaderResourceViewDesc.mipLevels = 1;

	wiRenderer::graphicsDevice->CreateShaderResourceView(texture2D, &shaderResourceViewDesc, &shaderResource);
}
void wiDepthTarget::InitializeCube(int size)
{
	Texture2DDesc depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = size;
	depthBufferDesc.Height = size;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 6;
	depthBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = USAGE_DEFAULT;
	depthBufferDesc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;

	HRESULT hr;
	// Create the texture for the depth buffer using the filled out description.
	hr=wiRenderer::graphicsDevice->CreateTexture2D(&depthBufferDesc, NULL, &texture2D);

	DepthStencilViewDesc depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = RESOURCE_DIMENSION_TEXTURE2DARRAY;
 //   depthStencilViewDesc.Texture2DArray.FirstArraySlice = 0;
	//depthStencilViewDesc.Texture2DArray.ArraySize = 6;
 //   depthStencilViewDesc.Texture2DArray.MipSlice = 0;
	depthStencilViewDesc.ArraySize = 6;
	depthStencilViewDesc.Flags = 0;

	// Create the depth stencil view.
	hr=wiRenderer::graphicsDevice->CreateDepthStencilView(texture2D, &depthStencilViewDesc, &depthTarget);

	//Create Depth ShaderResource
	//ShaderResourceViewDesc shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shaderResourceViewDesc.ViewDimension = RESOURCE_DIMENSION_TEXTURECUBE;
	//shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	//shaderResourceViewDesc.Texture2D.MipLevels = 1;
	shaderResourceViewDesc.mipLevels = 1;

	wiRenderer::graphicsDevice->CreateShaderResourceView(texture2D, &shaderResourceViewDesc, &shaderResource);
}

void wiDepthTarget::Clear(GRAPHICSTHREAD threadID)
{
	wiRenderer::graphicsDevice->ClearDepthStencil( depthTarget, CLEAR_DEPTH | CLEAR_STENCIL, 1.0f, 0);
}
void wiDepthTarget::CopyFrom(const wiDepthTarget& from, GRAPHICSTHREAD threadID)
{
	if(shaderResource) shaderResource->Release();
	//static ShaderResourceViewDesc desc;
	//from.shaderResource->GetDesc(&desc);
	wiRenderer::graphicsDevice->CopyResource(texture2D,from.texture2D);
	HRESULT r = wiRenderer::graphicsDevice->CreateShaderResourceView(texture2D,&from.shaderResourceViewDesc,&shaderResource);
}

