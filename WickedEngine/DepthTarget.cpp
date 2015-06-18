#include "DepthTarget.h"



DepthTarget::DepthTarget()
{
	texture2D = NULL;
	depthTarget = NULL;
	shaderResource = NULL;
}


DepthTarget::~DepthTarget()
{
	if(texture2D) texture2D->Release(); texture2D = NULL;
	if(depthTarget) depthTarget->Release(); depthTarget = NULL;
	if(shaderResource) shaderResource->Release(); shaderResource = NULL;
}

void DepthTarget::Initialize(int width, int height, UINT MSAAC, UINT MSAAQ)
{
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = width;
	depthBufferDesc.Height = height;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthBufferDesc.SampleDesc.Count = MSAAC;
	depthBufferDesc.SampleDesc.Quality = MSAAQ;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	HRESULT hr;
	// Create the texture for the depth buffer using the filled out description.
	hr=Renderer::graphicsDevice->CreateTexture2D(&depthBufferDesc, NULL, &texture2D);

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = (MSAAQ==0 ? D3D11_DSV_DIMENSION_TEXTURE2D : D3D11_DSV_DIMENSION_TEXTURE2DMS);
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	depthStencilViewDesc.Flags = 0;

	// Create the depth stencil view.
	hr=Renderer::graphicsDevice->CreateDepthStencilView(texture2D, &depthStencilViewDesc, &depthTarget);

	//Create Depth ShaderResource
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	shaderResourceViewDesc.ViewDimension = (MSAAQ==0 ? D3D11_SRV_DIMENSION_TEXTURE2D : D3D11_SRV_DIMENSION_TEXTURE2DMS);
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	Renderer::graphicsDevice->CreateShaderResourceView(texture2D, &shaderResourceViewDesc, &shaderResource);
}
void DepthTarget::InitializeCube(int size)
{
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = size;
	depthBufferDesc.Height = size;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 6;
	depthBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	HRESULT hr;
	// Create the texture for the depth buffer using the filled out description.
	hr=Renderer::graphicsDevice->CreateTexture2D(&depthBufferDesc, NULL, &texture2D);

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    depthStencilViewDesc.Texture2DArray.FirstArraySlice = 0;
	depthStencilViewDesc.Texture2DArray.ArraySize = 6;
    depthStencilViewDesc.Texture2DArray.MipSlice = 0;
	depthStencilViewDesc.Flags = 0;

	// Create the depth stencil view.
	hr=Renderer::graphicsDevice->CreateDepthStencilView(texture2D, &depthStencilViewDesc, &depthTarget);

	//Create Depth ShaderResource
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	Renderer::graphicsDevice->CreateShaderResourceView(texture2D, &shaderResourceViewDesc, &shaderResource);
}

void DepthTarget::Clear(ID3D11DeviceContext* context)
{
	context->ClearDepthStencilView( depthTarget, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
void DepthTarget::CopyFrom(const DepthTarget& from,ID3D11DeviceContext* context)
{
	if(shaderResource) shaderResource->Release();
	static D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	from.shaderResource->GetDesc(&desc);
	context->CopyResource(texture2D,from.texture2D);
	HRESULT r = Renderer::graphicsDevice->CreateShaderResourceView(texture2D,&desc,&shaderResource);
}

