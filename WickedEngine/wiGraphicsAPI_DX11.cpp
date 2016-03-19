#include "CommonInclude.h"
#include "wiGraphicsAPI_DX11.h"
#include "wiHelper.h"
#include "TextureMapping.h"

#include "Utility/WicTextureLoader.h"
#include "Utility/DDSTextureLoader.h"

GraphicsDevice_DX11::GraphicsDevice_DX11(HWND window, int screenW, int screenH, bool windowed)
{
	HRESULT hr = S_OK;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		//D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

#ifndef WINSTORE_SUPPORT
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = screenW;
	sd.BufferDesc.Height = screenH;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = window;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = windowed;
#endif

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		driverType = driverTypes[driverTypeIndex];
#ifndef WINSTORE_SUPPORT
		hr = D3D11CreateDeviceAndSwapChain(NULL, driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &swapChain, &device, &featureLevel, &deviceContexts[GRAPHICSTHREAD_IMMEDIATE]);
#else
		hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, SDK_VERSION, &graphicsDevice
			, &featureLevel, &immediateContext);
#endif

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr)) {
		wiHelper::messageBox("SwapChain Creation Failed!", "Error!", nullptr);
#ifdef BACKLOG
		wiBackLog::post("SwapChain Creation Failed!");
#endif
		exit(1);
	}
	DX11 = ((device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? true : false);


#ifdef WINSTORE_SUPPORT
	DXGI_SWAP_CHAIN_DESC1 sd = { 0 };
	sd.Width = SCREENWIDTH = (int)window->Bounds.Width;
	sd.Height = SCREENHEIGHT = (int)window->Bounds.Height;
	sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
	sd.Stereo = false;
	sd.SampleDesc.Count = 1; // Don't use multi-sampling.
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2; // Use double-buffering to minimize latency.
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
	sd.Flags = 0;
	sd.Scaling = DXGI_SCALING_NONE;
	sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

	IDXGIDevice2 * pDXGIDevice;
	hr = graphicsDevice->QueryInterface(__uuidof(IDXGIDevice2), (void **)&pDXGIDevice);

	IDXGIAdapter * pDXGIAdapter;
	hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter);

	IDXGIFactory2 * pIDXGIFactory;
	pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), (void **)&pIDXGIFactory);


	hr = pIDXGIFactory->CreateSwapChainForCoreWindow(graphicsDevice, reinterpret_cast<APIInterface>(window), &sd
		, nullptr, &swapChain);

	if (FAILED(hr)) {
		wiHelper::messageBox("Swap chain creation failed!", "Error!");
		exit(1);
	}
#endif

	DEFERREDCONTEXT_SUPPORT = false;
	D3D11_FEATURE_DATA_THREADING threadingFeature;
	device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingFeature, sizeof(threadingFeature));
	if (threadingFeature.DriverConcurrentCreates && threadingFeature.DriverCommandLists) {
		DEFERREDCONTEXT_SUPPORT = true;
		for (int i = 0; i<GRAPHICSTHREAD_COUNT; i++) {
			if (i == (int)GRAPHICSTHREAD_IMMEDIATE)
				continue;
			device->CreateDeferredContext(0, &deviceContexts[i]);
		}
	}
	else {
		DEFERREDCONTEXT_SUPPORT = false;
	}


	// Create a render target view
	APITexture2D pBackBuffer = NULL;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr)) {
		wiHelper::messageBox("BackBuffer creation Failed!", "Error!", nullptr);
		exit(0);
	}

	hr = device->CreateRenderTargetView(pBackBuffer, NULL, &renderTargetView);
	//pBackBuffer->Release();
	if (FAILED(hr)) {
		wiHelper::messageBox("Main Rendertarget creation Failed!", "Error!", nullptr);
		exit(0);
	}


	// Setup the main viewport
	viewPort.Width = (FLOAT)screenW;
	viewPort.Height = (FLOAT)screenH;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
}
GraphicsDevice_DX11::~GraphicsDevice_DX11()
{
	SAFE_RELEASE(renderTargetView);
	SAFE_RELEASE(swapChain);

	for (int i = 0; i<GRAPHICSTHREAD_COUNT; i++) {
		SAFE_RELEASE(commandLists[i]);
		SAFE_RELEASE(deviceContexts[i]);
	}

	SAFE_RELEASE(device);
}


// TODO!
inline D3D11_FILTER _ConvertFilter(FILTER value)
{
	return (D3D11_FILTER)value;
}
inline D3D11_TEXTURE_ADDRESS_MODE _ConvertTextureAddressMode(TEXTURE_ADDRESS_MODE value)
{
	return (D3D11_TEXTURE_ADDRESS_MODE)value;
}
inline D3D11_COMPARISON_FUNC _ConvertComparisonFunc(COMPARISON_FUNC value)
{
	return (D3D11_COMPARISON_FUNC)value;
}
inline D3D11_FILL_MODE _ConvertFillMode(FILL_MODE value)
{
	return (D3D11_FILL_MODE)value;
}
inline D3D11_CULL_MODE _ConvertCullMode(CULL_MODE value)
{
	return (D3D11_CULL_MODE)value;
}
inline D3D11_DEPTH_WRITE_MASK _ConvertDepthWriteMask(DEPTH_WRITE_MASK value)
{
	return (D3D11_DEPTH_WRITE_MASK)value;
}
inline D3D11_STENCIL_OP _ConvertStencilOp(STENCIL_OP value)
{
	return (D3D11_STENCIL_OP)value;
}
inline D3D11_BLEND _ConvertBlend(BLEND value)
{
	return (D3D11_BLEND)value;
}
inline D3D11_BLEND_OP _ConvertBlendOp(BLEND_OP value)
{
	return (D3D11_BLEND_OP)value;
}
inline D3D11_USAGE _ConvertUsage(USAGE value)
{
	return (D3D11_USAGE)value;
}
inline D3D11_INPUT_CLASSIFICATION _ConvertInputClassification(INPUT_CLASSIFICATION value)
{
	return (D3D11_INPUT_CLASSIFICATION)value;
}
inline DXGI_FORMAT _ConvertFormat(DXGI_FORMAT value)
{
	return value;
}
inline DXGI_SAMPLE_DESC _ConvertSampleDesc(DXGI_SAMPLE_DESC value)
{
	return value;
}

inline D3D11_TEXTURE2D_DESC _ConvertTexture2DDesc(const Texture2DDesc* pDesc)
{
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = pDesc->Width;
	desc.Height = pDesc->Height;
	desc.MipLevels = pDesc->MipLevels;
	desc.ArraySize = pDesc->ArraySize;
	desc.Format = _ConvertFormat(pDesc->Format);
	desc.SampleDesc = _ConvertSampleDesc(pDesc->SampleDesc);
	desc.Usage = _ConvertUsage(pDesc->Usage);
	desc.BindFlags = pDesc->BindFlags;
	desc.CPUAccessFlags = pDesc->CPUAccessFlags;
	desc.MiscFlags = pDesc->MiscFlags;

	return desc;
}
inline D3D11_SUBRESOURCE_DATA* _ConvertSubresourceData(const SubresourceData* pInitialData)
{
	if (pInitialData == nullptr)
		return nullptr;

	D3D11_SUBRESOURCE_DATA* data = new D3D11_SUBRESOURCE_DATA;
	data->pSysMem = pInitialData->pSysMem;
	data->SysMemPitch = pInitialData->SysMemPitch;
	data->SysMemSlicePitch = pInitialData->SysMemSlicePitch;

	return data;
}


HRESULT GraphicsDevice_DX11::CreateBuffer(const BufferDesc *pDesc, const SubresourceData* pInitialData, BufferResource *ppBuffer)
{
	D3D11_BUFFER_DESC desc; 
	desc.ByteWidth = pDesc->ByteWidth;
	desc.Usage = _ConvertUsage(pDesc->Usage);
	desc.BindFlags = pDesc->BindFlags;
	desc.CPUAccessFlags = pDesc->CPUAccessFlags;
	desc.MiscFlags = pDesc->MiscFlags;
	desc.StructureByteStride = pDesc->StructureByteStride;

	D3D11_SUBRESOURCE_DATA* data = _ConvertSubresourceData(pInitialData);

	return device->CreateBuffer(&desc, data, ppBuffer);
}
HRESULT GraphicsDevice_DX11::CreateTexture1D()
{
	// TODO
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D)
{
	D3D11_TEXTURE2D_DESC desc = _ConvertTexture2DDesc(pDesc);

	D3D11_SUBRESOURCE_DATA* data = _ConvertSubresourceData(pInitialData);

	(*ppTexture2D) = new Texture2D;
	(*ppTexture2D)->desc = *pDesc;

	HRESULT hr = S_OK;
	
	hr = device->CreateTexture2D(&desc, data, &((*ppTexture2D)->texture2D));
	if (FAILED(hr))
		return hr;

	hr = CreateRenderTargetView(*ppTexture2D);
	if (FAILED(hr))
		return hr;
	hr = CreateShaderResourceView(*ppTexture2D);
	if (FAILED(hr))
		return hr;
	hr = CreateDepthStencilView(*ppTexture2D);
	if (FAILED(hr))
		return hr;

	return hr;
}
HRESULT GraphicsDevice_DX11::CreateTexture3D()
{
	// TODO
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateTextureCube(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, TextureCube **ppTextureCube)
{
	D3D11_TEXTURE2D_DESC desc = _ConvertTexture2DDesc(pDesc);

	D3D11_SUBRESOURCE_DATA* data = _ConvertSubresourceData(pInitialData);

	(*ppTextureCube) = new TextureCube;
	(*ppTextureCube)->desc = *pDesc;

	HRESULT hr = S_OK;

	if (!(desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE))
	{
		desc.MiscFlags |= RESOURCE_MISC_TEXTURECUBE;
	}

	hr = device->CreateTexture2D(&desc, data, &((*ppTextureCube)->texture2D));
	if (FAILED(hr))
		return hr;

	hr = CreateRenderTargetView(*ppTextureCube);
	if (FAILED(hr))
		return hr;
	hr = CreateShaderResourceView(*ppTextureCube);
	if (FAILED(hr))
		return hr;
	hr = CreateDepthStencilView(*ppTextureCube);
	if (FAILED(hr))
		return hr;

	return hr;
}
HRESULT GraphicsDevice_DX11::CreateShaderResourceView(Texture2D* pTexture)
{
	HRESULT hr = E_FAIL;
	if (pTexture->desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		if (pTexture->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
		{
			shaderResourceViewDesc.Format = pTexture->desc.Format;
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			shaderResourceViewDesc.TextureCube.MostDetailedMip = 0; //from most detailed...
			shaderResourceViewDesc.TextureCube.MipLevels = -1; //...to least detailed
		}
		else
		{
			shaderResourceViewDesc.Format = pTexture->desc.Format;
			shaderResourceViewDesc.ViewDimension = (pTexture->desc.SampleDesc.Quality == 0 ? D3D11_SRV_DIMENSION_TEXTURE2D : D3D11_SRV_DIMENSION_TEXTURE2DMS);
			shaderResourceViewDesc.Texture2D.MostDetailedMip = 0; //from most detailed...
			shaderResourceViewDesc.Texture2D.MipLevels = -1; //...to least detailed
		}

		hr = device->CreateShaderResourceView(pTexture->texture2D, &shaderResourceViewDesc, &pTexture->shaderResourceView);
	}
	return hr;
}
HRESULT GraphicsDevice_DX11::CreateRenderTargetView(Texture2D* pTexture)
{
	HRESULT hr = E_FAIL;
	if (pTexture->desc.BindFlags & D3D11_BIND_RENDER_TARGET)
	{
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		if (pTexture->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
		{
			renderTargetViewDesc.Format = pTexture->desc.Format;
			renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
			renderTargetViewDesc.Texture2DArray.ArraySize = 6;
			renderTargetViewDesc.Texture2DArray.MipSlice = 0;
		}
		else
		{
			renderTargetViewDesc.Format = pTexture->desc.Format;
			renderTargetViewDesc.ViewDimension = (pTexture->desc.SampleDesc.Quality == 0 ? D3D11_RTV_DIMENSION_TEXTURE2D : D3D11_RTV_DIMENSION_TEXTURE2DMS);
			renderTargetViewDesc.Texture2D.MipSlice = 0;
		}

		hr = device->CreateRenderTargetView(pTexture->texture2D, &renderTargetViewDesc, &pTexture->renderTargetView);
	}
	return hr;
}
HRESULT GraphicsDevice_DX11::CreateDepthStencilView(Texture2D* pTexture)
{
	HRESULT hr = E_FAIL;
	if (pTexture->desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		if (pTexture->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
		{
			ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
			depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			depthStencilViewDesc.Texture2DArray.FirstArraySlice = 0;
			depthStencilViewDesc.Texture2DArray.ArraySize = 6;
			depthStencilViewDesc.Texture2DArray.MipSlice = 0;
			depthStencilViewDesc.Flags = 0;
		}
		else
		{
			ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
			depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilViewDesc.ViewDimension = (pTexture->desc.SampleDesc.Quality == 0 ? D3D11_DSV_DIMENSION_TEXTURE2D : D3D11_DSV_DIMENSION_TEXTURE2DMS);
			depthStencilViewDesc.Texture2D.MipSlice = 0;
			depthStencilViewDesc.Flags = 0;
		}

		hr = device->CreateDepthStencilView(pTexture->texture2D, &depthStencilViewDesc, &pTexture->depthStencilView);
	}
	return hr;
}
HRESULT GraphicsDevice_DX11::CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
	const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *ppInputLayout)
{
	D3D11_INPUT_ELEMENT_DESC* desc = new D3D11_INPUT_ELEMENT_DESC[NumElements];
	for (UINT i = 0; i < NumElements; ++i)
	{
		desc[i].SemanticName = pInputElementDescs[i].SemanticName;
		desc[i].SemanticIndex = pInputElementDescs[i].SemanticIndex;
		desc[i].Format = _ConvertFormat(pInputElementDescs[i].Format);
		desc[i].InputSlot = pInputElementDescs[i].InputSlot;
		desc[i].AlignedByteOffset = pInputElementDescs[i].AlignedByteOffset;
		desc[i].InputSlotClass = _ConvertInputClassification(pInputElementDescs[i].InputSlotClass);
		desc[i].InstanceDataStepRate = pInputElementDescs[i].InstanceDataStepRate;
	}

	HRESULT hr = device->CreateInputLayout(desc, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout);

	SAFE_DELETE_ARRAY(desc);

	return hr;
}
HRESULT GraphicsDevice_DX11::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, VertexShader *ppVertexShader)
{
	return device->CreateVertexShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
}
HRESULT GraphicsDevice_DX11::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, PixelShader *ppPixelShader)
{
	return device->CreatePixelShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
}
HRESULT GraphicsDevice_DX11::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, GeometryShader *ppGeometryShader)
{
	return device->CreateGeometryShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppGeometryShader);
}
HRESULT GraphicsDevice_DX11::CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode, SIZE_T BytecodeLength, const StreamOutDeclaration *pSODeclaration,
	UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ClassLinkage pClassLinkage, GeometryShader *ppGeometryShader)
{
	D3D11_SO_DECLARATION_ENTRY* decl = new D3D11_SO_DECLARATION_ENTRY[NumEntries];
	for (UINT i = 0; i < NumEntries; ++i)
	{
		decl[i].Stream = pSODeclaration[i].Stream;
		decl[i].SemanticName = pSODeclaration[i].SemanticName;
		decl[i].SemanticIndex = pSODeclaration[i].SemanticIndex;
		decl[i].StartComponent = pSODeclaration[i].StartComponent;
		decl[i].ComponentCount = pSODeclaration[i].ComponentCount;
		decl[i].OutputSlot = pSODeclaration[i].OutputSlot;
	}

	HRESULT hr = device->CreateGeometryShaderWithStreamOutput(pShaderBytecode, BytecodeLength, decl, NumEntries, pBufferStrides,
		NumStrides, RasterizedStream, pClassLinkage, ppGeometryShader);

	SAFE_DELETE_ARRAY(decl);

	return hr;
}
HRESULT GraphicsDevice_DX11::CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, HullShader *ppHullShader)
{
	return device->CreateHullShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppHullShader);
}
HRESULT GraphicsDevice_DX11::CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, DomainShader *ppDomainShader)
{
	return device->CreateDomainShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppDomainShader);
}
HRESULT GraphicsDevice_DX11::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, ComputeShader *ppComputeShader)
{
	return device->CreateComputeShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppComputeShader);
}
HRESULT GraphicsDevice_DX11::CreateBlendState(const BlendDesc *pBlendStateDesc, BlendState *ppBlendState)
{
	D3D11_BLEND_DESC desc;
	desc.AlphaToCoverageEnable = pBlendStateDesc->AlphaToCoverageEnable;
	desc.IndependentBlendEnable = pBlendStateDesc->IndependentBlendEnable;
	for (int i = 0; i < 8; ++i)
	{
		desc.RenderTarget[i].BlendEnable = pBlendStateDesc->RenderTarget[i].BlendEnable;
		desc.RenderTarget[i].SrcBlend = _ConvertBlend(pBlendStateDesc->RenderTarget[i].SrcBlend);
		desc.RenderTarget[i].DestBlend = _ConvertBlend(pBlendStateDesc->RenderTarget[i].DestBlend);
		desc.RenderTarget[i].BlendOp = _ConvertBlendOp(pBlendStateDesc->RenderTarget[i].BlendOp);
		desc.RenderTarget[i].SrcBlendAlpha = _ConvertBlend(pBlendStateDesc->RenderTarget[i].SrcBlendAlpha);
		desc.RenderTarget[i].DestBlendAlpha = _ConvertBlend(pBlendStateDesc->RenderTarget[i].DestBlendAlpha);
		desc.RenderTarget[i].BlendOpAlpha = _ConvertBlendOp(pBlendStateDesc->RenderTarget[i].BlendOpAlpha);
		desc.RenderTarget[i].RenderTargetWriteMask = pBlendStateDesc->RenderTarget[i].RenderTargetWriteMask;
	}

	return device->CreateBlendState(&desc, ppBlendState);
}
HRESULT GraphicsDevice_DX11::CreateDepthStencilState(const DepthStencilDesc *pDepthStencilDesc, DepthStencilState *ppDepthStencilState)
{
	D3D11_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = pDepthStencilDesc->DepthEnable;
	desc.DepthWriteMask = _ConvertDepthWriteMask(pDepthStencilDesc->DepthWriteMask);
	desc.DepthFunc = _ConvertComparisonFunc(pDepthStencilDesc->DepthFunc);
	desc.StencilEnable = pDepthStencilDesc->StencilEnable;
	desc.StencilReadMask = pDepthStencilDesc->StencilReadMask;
	desc.StencilWriteMask = pDepthStencilDesc->StencilWriteMask;
	desc.FrontFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilDesc->FrontFace.StencilDepthFailOp);
	desc.FrontFace.StencilFailOp = _ConvertStencilOp(pDepthStencilDesc->FrontFace.StencilFailOp);
	desc.FrontFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilDesc->FrontFace.StencilFunc);
	desc.FrontFace.StencilPassOp = _ConvertStencilOp(pDepthStencilDesc->FrontFace.StencilPassOp);
	desc.BackFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilDesc->BackFace.StencilDepthFailOp);
	desc.BackFace.StencilFailOp = _ConvertStencilOp(pDepthStencilDesc->BackFace.StencilFailOp);
	desc.BackFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilDesc->BackFace.StencilFunc);
	desc.BackFace.StencilPassOp = _ConvertStencilOp(pDepthStencilDesc->BackFace.StencilPassOp);

	return device->CreateDepthStencilState(&desc, ppDepthStencilState);
}
HRESULT GraphicsDevice_DX11::CreateRasterizerState(const RasterizerDesc *pRasterizerDesc, RasterizerState *ppRasterizerState)
{
	D3D11_RASTERIZER_DESC desc;
	desc.FillMode = _ConvertFillMode(pRasterizerDesc->FillMode);
	desc.CullMode = _ConvertCullMode(pRasterizerDesc->CullMode);
	desc.FrontCounterClockwise = pRasterizerDesc->FrontCounterClockwise;
	desc.DepthBias = pRasterizerDesc->DepthBias;
	desc.DepthBiasClamp = pRasterizerDesc->DepthBiasClamp;
	desc.SlopeScaledDepthBias = pRasterizerDesc->SlopeScaledDepthBias;
	desc.DepthClipEnable = pRasterizerDesc->DepthClipEnable;
	desc.ScissorEnable = pRasterizerDesc->ScissorEnable;
	desc.MultisampleEnable = pRasterizerDesc->MultisampleEnable;
	desc.AntialiasedLineEnable = pRasterizerDesc->AntialiasedLineEnable;

	return device->CreateRasterizerState(&desc, ppRasterizerState);
}
HRESULT GraphicsDevice_DX11::CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *ppSamplerState)
{
	D3D11_SAMPLER_DESC desc;
	desc.Filter = _ConvertFilter(pSamplerDesc->Filter);
	desc.AddressU = _ConvertTextureAddressMode(pSamplerDesc->AddressU);
	desc.AddressV = _ConvertTextureAddressMode(pSamplerDesc->AddressV);
	desc.AddressW = _ConvertTextureAddressMode(pSamplerDesc->AddressW);
	desc.MipLODBias = pSamplerDesc->MipLODBias;
	desc.MaxAnisotropy = pSamplerDesc->MaxAnisotropy;
	desc.ComparisonFunc = _ConvertComparisonFunc(pSamplerDesc->ComparisonFunc);
	desc.BorderColor[0] = pSamplerDesc->BorderColor[0];
	desc.BorderColor[1] = pSamplerDesc->BorderColor[1];
	desc.BorderColor[2] = pSamplerDesc->BorderColor[2];
	desc.BorderColor[3] = pSamplerDesc->BorderColor[3];
	desc.MinLOD = pSamplerDesc->MinLOD;
	desc.MaxLOD = pSamplerDesc->MaxLOD;

	return device->CreateSamplerState(&desc, ppSamplerState);
}


void GraphicsDevice_DX11::PresentBegin()
{
	LOCK();

	BindViewports(1, &viewPort);
	deviceContexts[GRAPHICSTHREAD_IMMEDIATE]->OMSetRenderTargets(1, &renderTargetView, 0);
	float ClearColor[4] = { 0, 0, 0, 1.0f }; // red,green,blue,alpha
	deviceContexts[GRAPHICSTHREAD_IMMEDIATE]->ClearRenderTargetView(renderTargetView, ClearColor);

}
void GraphicsDevice_DX11::PresentEnd()
{
	swapChain->Present(VSYNC, 0);


	deviceContexts[GRAPHICSTHREAD_IMMEDIATE]->OMSetRenderTargets(0, nullptr, nullptr);

	UnbindTextures(0, TEXSLOT_COUNT, GRAPHICSTHREAD_IMMEDIATE);

	UNLOCK();
}

void GraphicsDevice_DX11::ExecuteDeferredContexts()
{
	for (int i = 0; i < GRAPHICSTHREAD_COUNT; i++)
	{
		if (i != (GRAPHICSTHREAD)GRAPHICSTHREAD_IMMEDIATE &&commandLists[i])
		{
			deviceContexts[GRAPHICSTHREAD_IMMEDIATE]->ExecuteCommandList(commandLists[i], true);
			commandLists[i]->Release();
			commandLists[i] = nullptr;

			UnbindTextures(0, TEXSLOT_COUNT, (GRAPHICSTHREAD)i);
		}
	}
}
void GraphicsDevice_DX11::FinishCommandList(GRAPHICSTHREAD thread)
{
	if (thread == GRAPHICSTHREAD_IMMEDIATE)
		return;
	deviceContexts[thread]->FinishCommandList(true, &commandLists[thread]);
}


HRESULT GraphicsDevice_DX11::CreateTextureFromFile(const wstring& fileName, Texture2D **ppTexture, bool mipMaps, GRAPHICSTHREAD threadID)
{
	HRESULT hr = E_FAIL;
	(*ppTexture) = new Texture2D();

	if (!fileName.substr(fileName.length() - 4).compare(wstring(L".dds")))
	{
		// Load dds
		hr = CreateDDSTextureFromFile(device, fileName.c_str(), nullptr, &(*ppTexture)->shaderResourceView);
	}
	else
	{
		// Load WIC
		if (mipMaps && threadID == GRAPHICSTHREAD_IMMEDIATE)
			LOCK();
		hr = CreateWICTextureFromFile(mipMaps, device, deviceContexts[threadID], fileName.c_str(), nullptr, &(*ppTexture)->shaderResourceView);
		if (mipMaps && threadID == GRAPHICSTHREAD_IMMEDIATE)
			UNLOCK();
	}

	if (FAILED(hr))
		SAFE_DELETE(*ppTexture);

	return hr;
}
