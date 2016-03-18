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
	createDeviceFlags |= CREATE_DEVICE_DEBUG;
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
			CreateDeferredContext(0, &deviceContexts[i]);
		}
#ifdef BACKLOG
		stringstream ss("");
		ss << NUM_DCONTEXT << " defferred contexts created!";
		wiBackLog::post(ss.str().c_str());
#endif
	}
	else {
		//MessageBox(window,L"Deferred Context not supported!",L"Error!",0);
#ifdef BACKLOG
		wiBackLog::post("Deferred context not supported!");
#endif
		DEFERREDCONTEXT_SUPPORT = false;
		//exit(0);
	}


	// Create a render target view
	Texture2D pBackBuffer = NULL;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr)) {
		wiHelper::messageBox("BackBuffer creation Failed!", "Error!", nullptr);
		exit(0);
	}

	hr = CreateRenderTargetView(pBackBuffer, NULL, &renderTargetView);
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


HRESULT GraphicsDevice_DX11::CreateBuffer(const BufferDesc *pDesc, const SubresourceData* pInitialData, BufferResource *ppBuffer)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D *ppTexture2D)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateShaderResourceView(APIResource pResource, const ShaderResourceViewDesc* pDesc, TextureView *ppSRView)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateRenderTargetView(APIResource pResource, const RenderTargetViewDesc* pDesc, RenderTargetView *ppRTView)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateDepthStencilView(APIResource pResource, const DepthStencilViewDesc* pDesc, DepthStencilView *ppDepthStencilView)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
	const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *ppInputLayout)
{

}
HRESULT GraphicsDevice_DX11::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, VertexShader *ppVertexShader)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, PixelShader *ppPixelShader)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, GeometryShader *ppGeometryShader)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode, SIZE_T BytecodeLength, const StreamOutDeclaration *pSODeclaration,
	UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ClassLinkage pClassLinkage, GeometryShader *ppGeometryShader)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, HullShader *ppHullShader)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, DomainShader *ppDomainShader)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, ComputeShader *ppComputeShader)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateBlendState(const BlendDesc *pBlendStateDesc, BlendState *ppBlendState)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateDepthStencilState(const DepthStencilDesc *pDepthStencilDesc, DepthStencilState *ppDepthStencilState)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateRasterizerState(const RasterizerDesc *pRasterizerDesc, RasterizerState *ppRasterizerState)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *ppSamplerState)
{
	return E_FAIL;
}
HRESULT GraphicsDevice_DX11::CreateDeferredContext(UINT ContextFlags, DeviceContext *ppDeferredContext)
{
	return E_FAIL;
}
DeviceContext GraphicsDevice_DX11::GetImmediateContext()
{
	return deviceContexts[GRAPHICSTHREAD_IMMEDIATE];
}

void GraphicsDevice_DX11::PresentBegin()
{
	LOCK();

	BindViewports(1, &viewPort);
	GetImmediateContext()->OMSetRenderTargets(1, &renderTargetView, 0);
	float ClearColor[4] = { 0, 0, 0, 1.0f }; // red,green,blue,alpha
	GetImmediateContext()->ClearRenderTargetView(renderTargetView, ClearColor);

}
void GraphicsDevice_DX11::PresentEnd()
{
	swapChain->Present(VSYNC, 0);


	GetImmediateContext()->OMSetRenderTargets(0, nullptr, nullptr);

	UnbindTextures(0, TEXSLOT_COUNT, GRAPHICSTHREAD_IMMEDIATE);

	UNLOCK();
}

void GraphicsDevice_DX11::ExecuteDeferredContexts()
{
	for (int i = 0; i < GRAPHICSTHREAD_COUNT; i++)
	{
		if (i != (GRAPHICSTHREAD)GRAPHICSTHREAD_IMMEDIATE &&commandLists[i])
		{
			GetImmediateContext()->ExecuteCommandList(commandLists[i], true);
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



HRESULT GraphicsDevice_DX11::CreateTextureFromFile(const wstring& fileName, TextureView *ppShaderResourceView, bool mipMaps, GRAPHICSTHREAD threadID)
{
	if (!fileName.substr(fileName.length() - 4).compare(wstring(L".dds")))
	{
		// Load dds
		CreateDDSTextureFromFile(device, fileName.c_str(), nullptr, ppShaderResourceView);
	}
	else
	{
		// Load WIC
		if (mipMaps && threadID == GRAPHICSTHREAD_IMMEDIATE)
			LOCK();
		CreateWICTextureFromFile(mipMaps, device, deviceContexts[threadID], fileName.c_str(), nullptr, ppShaderResourceView);
		if (mipMaps && threadID == GRAPHICSTHREAD_IMMEDIATE)
			UNLOCK();
	}
}
