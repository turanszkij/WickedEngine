#include "wiGraphicsAPI_DX11.h"
#include "wiHelper.h"
#include "TextureMapping.h"

#include "Utility/WicTextureLoader.h"
#include "Utility/DDSTextureLoader.h"

#ifndef WINSTORE_SUPPORT
GraphicsDevice_DX11::GraphicsDevice_DX11(HWND window, int screenW, int screenH, bool windowed) : GraphicsDevice()
#else
GraphicsDevice_DX11::GraphicsDevice_DX11(Windows::UI::Core::CoreWindow^ window)
#endif
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
	ID3D11Texture2D* pBackBuffer = NULL;
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

	SCREENWIDTH = screenW;
	SCREENHEIGHT = screenH;
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

void GraphicsDevice_DX11::SetScreenWidth(int value)
{
	SCREENWIDTH = value;
	// TODO: resize backbuffer
}
void GraphicsDevice_DX11::SetScreenHeight(int value)
{
	SCREENHEIGHT = value;
	// TODO: resize backbuffer
}

inline UINT _ParseBindFlags(UINT value)
{
	UINT _flag = 0;

	if (value & BIND_VERTEX_BUFFER)
		_flag |= D3D11_BIND_VERTEX_BUFFER;
	if (value & BIND_INDEX_BUFFER)
		_flag |= D3D11_BIND_INDEX_BUFFER;
	if (value & BIND_CONSTANT_BUFFER)
		_flag |= D3D11_BIND_CONSTANT_BUFFER;
	if (value & BIND_SHADER_RESOURCE)
		_flag |= D3D11_BIND_SHADER_RESOURCE;
	if (value & BIND_STREAM_OUTPUT)
		_flag |= D3D11_BIND_STREAM_OUTPUT;
	if (value & BIND_RENDER_TARGET)
		_flag |= D3D11_BIND_RENDER_TARGET;
	if (value & BIND_DEPTH_STENCIL)
		_flag |= D3D11_BIND_DEPTH_STENCIL;
	if (value & BIND_UNORDERED_ACCESS)
		_flag |= D3D11_BIND_UNORDERED_ACCESS;

	return _flag;
}
inline UINT _ParseCPUAccessFlags(UINT value)
{
	UINT _flag = 0;

	if (value & CPU_ACCESS_WRITE)
		_flag |= D3D11_CPU_ACCESS_WRITE;
	if (value & CPU_ACCESS_READ)
		_flag |= D3D11_CPU_ACCESS_READ;

	return _flag;
}
inline UINT _ParseResourceMiscFlags(UINT value)
{
	UINT _flag = 0;

	if (value & RESOURCE_MISC_GENERATE_MIPS)
		_flag |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	if (value & RESOURCE_MISC_SHARED)
		_flag |= D3D11_RESOURCE_MISC_SHARED;
	if (value & RESOURCE_MISC_TEXTURECUBE)
		_flag |= D3D11_RESOURCE_MISC_TEXTURECUBE;
	if (value & RESOURCE_MISC_BUFFER_STRUCTURED)
		_flag |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	if (value & RESOURCE_MISC_TILED)
		_flag |= D3D11_RESOURCE_MISC_TILED;

	return _flag;
}

inline D3D11_FILTER _ConvertFilter(FILTER value)
{
	switch (value)
	{
	case FILTER_MIN_MAG_MIP_POINT:
		return D3D11_FILTER_MIN_MAG_MIP_POINT;
		break;
	case FILTER_MIN_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MIN_POINT_MAG_MIP_LINEAR:
		return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		break;
	case FILTER_MIN_LINEAR_MAG_MIP_POINT:
		return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		break;
	case FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MIN_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MIN_MAG_MIP_LINEAR:
		return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		break;
	case FILTER_ANISOTROPIC:
		return D3D11_FILTER_ANISOTROPIC;
		break;
	case FILTER_COMPARISON_MIN_MAG_MIP_POINT:
		return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		break;
	case FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
		return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
		break;
	case FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
		return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
		break;
	case FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
		return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		break;
	case FILTER_COMPARISON_ANISOTROPIC:
		return D3D11_FILTER_COMPARISON_ANISOTROPIC;
		break;
	case FILTER_MINIMUM_MIN_MAG_MIP_POINT:
		return D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
		break;
	case FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
		return D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
		break;
	case FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
		return D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
		break;
	case FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:
		return D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
		break;
	case FILTER_MINIMUM_ANISOTROPIC:
		return D3D11_FILTER_MINIMUM_ANISOTROPIC;
		break;
	case FILTER_MAXIMUM_MIN_MAG_MIP_POINT:
		return D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
		break;
	case FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
		return D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
		break;
	case FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
		return D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
		break;
	case FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:
		return D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
		break;
	case FILTER_MAXIMUM_ANISOTROPIC:
		return D3D11_FILTER_MAXIMUM_ANISOTROPIC;
		break;
	default:
		break;
	}
	return D3D11_FILTER_MIN_MAG_MIP_POINT;
}
inline D3D11_TEXTURE_ADDRESS_MODE _ConvertTextureAddressMode(TEXTURE_ADDRESS_MODE value)
{
	switch (value)
	{
	case TEXTURE_ADDRESS_WRAP:
		return D3D11_TEXTURE_ADDRESS_WRAP;
		break;
	case TEXTURE_ADDRESS_MIRROR:
		return D3D11_TEXTURE_ADDRESS_MIRROR;
		break;
	case TEXTURE_ADDRESS_CLAMP:
		return D3D11_TEXTURE_ADDRESS_CLAMP;
		break;
	case TEXTURE_ADDRESS_BORDER:
		return D3D11_TEXTURE_ADDRESS_BORDER;
		break;
	case TEXTURE_ADDRESS_MIRROR_ONCE:
		return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
		break;
	default:
		break;
	}
	return D3D11_TEXTURE_ADDRESS_WRAP;
}
inline D3D11_COMPARISON_FUNC _ConvertComparisonFunc(COMPARISON_FUNC value)
{
	switch (value)
	{
	case COMPARISON_NEVER:
		return D3D11_COMPARISON_NEVER;
		break;
	case COMPARISON_LESS:
		return D3D11_COMPARISON_LESS;
		break;
	case COMPARISON_EQUAL:
		return D3D11_COMPARISON_EQUAL;
		break;
	case COMPARISON_LESS_EQUAL:
		return D3D11_COMPARISON_LESS_EQUAL;
		break;
	case COMPARISON_GREATER:
		return D3D11_COMPARISON_GREATER;
		break;
	case COMPARISON_NOT_EQUAL:
		return D3D11_COMPARISON_NOT_EQUAL;
		break;
	case COMPARISON_GREATER_EQUAL:
		return D3D11_COMPARISON_GREATER_EQUAL;
		break;
	case COMPARISON_ALWAYS:
		return D3D11_COMPARISON_ALWAYS;
		break;
	default:
		break;
	}
	return D3D11_COMPARISON_NEVER;
}
inline D3D11_FILL_MODE _ConvertFillMode(FILL_MODE value)
{
	switch (value)
	{
	case FILL_WIREFRAME:
		return D3D11_FILL_WIREFRAME;
		break;
	case FILL_SOLID:
		return D3D11_FILL_SOLID;
		break;
	default:
		break;
	}
	return D3D11_FILL_WIREFRAME;
}
inline D3D11_CULL_MODE _ConvertCullMode(CULL_MODE value)
{
	switch (value)
	{
	case CULL_NONE:
		return D3D11_CULL_NONE;
		break;
	case CULL_FRONT:
		return D3D11_CULL_FRONT;
		break;
	case CULL_BACK:
		return D3D11_CULL_BACK;
		break;
	default:
		break;
	}
	return D3D11_CULL_NONE;
}
inline D3D11_DEPTH_WRITE_MASK _ConvertDepthWriteMask(DEPTH_WRITE_MASK value)
{
	switch (value)
	{
	case DEPTH_WRITE_MASK_ZERO:
		return D3D11_DEPTH_WRITE_MASK_ZERO;
		break;
	case DEPTH_WRITE_MASK_ALL:
		return D3D11_DEPTH_WRITE_MASK_ALL;
		break;
	default:
		break;
	}
	return D3D11_DEPTH_WRITE_MASK_ZERO;
}
inline D3D11_STENCIL_OP _ConvertStencilOp(STENCIL_OP value)
{
	switch (value)
	{
	case STENCIL_OP_KEEP:
		return D3D11_STENCIL_OP_KEEP;
		break;
	case STENCIL_OP_ZERO:
		return D3D11_STENCIL_OP_ZERO;
		break;
	case STENCIL_OP_REPLACE:
		return D3D11_STENCIL_OP_REPLACE;
		break;
	case STENCIL_OP_INCR_SAT:
		return D3D11_STENCIL_OP_INCR_SAT;
		break;
	case STENCIL_OP_DECR_SAT:
		return D3D11_STENCIL_OP_DECR_SAT;
		break;
	case STENCIL_OP_INVERT:
		return D3D11_STENCIL_OP_INVERT;
		break;
	case STENCIL_OP_INCR:
		return D3D11_STENCIL_OP_INCR;
		break;
	case STENCIL_OP_DECR:
		return D3D11_STENCIL_OP_DECR;
		break;
	default:
		break;
	}
	return D3D11_STENCIL_OP_KEEP;
}
inline D3D11_BLEND _ConvertBlend(BLEND value)
{
	switch (value)
	{
	case BLEND_ZERO:
		return D3D11_BLEND_ZERO;
		break;
	case BLEND_ONE:
		return D3D11_BLEND_ONE;
		break;
	case BLEND_SRC_COLOR:
		return D3D11_BLEND_SRC_COLOR;
		break;
	case BLEND_INV_SRC_COLOR:
		return D3D11_BLEND_INV_SRC_COLOR;
		break;
	case BLEND_SRC_ALPHA:
		return D3D11_BLEND_SRC_ALPHA;
		break;
	case BLEND_INV_SRC_ALPHA:
		return D3D11_BLEND_INV_SRC_ALPHA;
		break;
	case BLEND_DEST_ALPHA:
		return D3D11_BLEND_DEST_ALPHA;
		break;
	case BLEND_INV_DEST_ALPHA:
		return D3D11_BLEND_INV_DEST_ALPHA;
		break;
	case BLEND_DEST_COLOR:
		return D3D11_BLEND_DEST_COLOR;
		break;
	case BLEND_INV_DEST_COLOR:
		return D3D11_BLEND_INV_DEST_COLOR;
		break;
	case BLEND_SRC_ALPHA_SAT:
		return D3D11_BLEND_SRC_ALPHA_SAT;
		break;
	case BLEND_BLEND_FACTOR:
		return D3D11_BLEND_BLEND_FACTOR;
		break;
	case BLEND_INV_BLEND_FACTOR:
		return D3D11_BLEND_INV_BLEND_FACTOR;
		break;
	case BLEND_SRC1_COLOR:
		return D3D11_BLEND_SRC1_COLOR;
		break;
	case BLEND_INV_SRC1_COLOR:
		return D3D11_BLEND_INV_SRC1_COLOR;
		break;
	case BLEND_SRC1_ALPHA:
		return D3D11_BLEND_SRC1_ALPHA;
		break;
	case BLEND_INV_SRC1_ALPHA:
		return D3D11_BLEND_INV_SRC1_ALPHA;
		break;
	default:
		break;
	}
	return D3D11_BLEND_ZERO;
}
inline D3D11_BLEND_OP _ConvertBlendOp(BLEND_OP value)
{
	switch (value)
	{
	case BLEND_OP_ADD:
		return D3D11_BLEND_OP_ADD;
		break;
	case BLEND_OP_SUBTRACT:
		return D3D11_BLEND_OP_SUBTRACT;
		break;
	case BLEND_OP_REV_SUBTRACT:
		return D3D11_BLEND_OP_REV_SUBTRACT;
		break;
	case BLEND_OP_MIN:
		return D3D11_BLEND_OP_MIN;
		break;
	case BLEND_OP_MAX:
		return D3D11_BLEND_OP_MAX;
		break;
	default:
		break;
	}
	return D3D11_BLEND_OP_ADD;
}
inline D3D11_USAGE _ConvertUsage(USAGE value)
{
	switch (value)
	{
	case USAGE_DEFAULT:
		return D3D11_USAGE_DEFAULT;
		break;
	case USAGE_IMMUTABLE:
		return D3D11_USAGE_IMMUTABLE;
		break;
	case USAGE_DYNAMIC:
		return D3D11_USAGE_DYNAMIC;
		break;
	case USAGE_STAGING:
		return D3D11_USAGE_STAGING;
		break;
	default:
		break;
	}
	return D3D11_USAGE_DEFAULT;
}
inline D3D11_INPUT_CLASSIFICATION _ConvertInputClassification(INPUT_CLASSIFICATION value)
{
	switch (value)
	{
	case INPUT_PER_VERTEX_DATA:
		return D3D11_INPUT_PER_VERTEX_DATA;
		break;
	case INPUT_PER_INSTANCE_DATA:
		return D3D11_INPUT_PER_INSTANCE_DATA;
		break;
	default:
		break;
	}
	return D3D11_INPUT_PER_VERTEX_DATA;
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
	desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);

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
	desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);
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

	CreateRenderTargetView(*ppTexture2D);
	CreateShaderResourceView(*ppTexture2D);
	CreateDepthStencilView(*ppTexture2D);

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

	if (!(desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE))
	{
		desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
	}

	hr = device->CreateTexture2D(&desc, data, &((*ppTextureCube)->texture2D));
	if (FAILED(hr))
		return hr;

	CreateRenderTargetView(*ppTextureCube);
	CreateShaderResourceView(*ppTextureCube);
	CreateDepthStencilView(*ppTextureCube);

	return hr;
}
HRESULT GraphicsDevice_DX11::CreateShaderResourceView(Texture2D* pTexture)
{
	HRESULT hr = E_FAIL;
	if (pTexture->desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		if (pTexture->desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
		{
			shaderResourceViewDesc.Format = pTexture->desc.Format;
			if (pTexture->desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
			{
				shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
			}
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			shaderResourceViewDesc.TextureCube.MostDetailedMip = 0; //from most detailed...
			shaderResourceViewDesc.TextureCube.MipLevels = -1; //...to least detailed
		}
		else
		{
			shaderResourceViewDesc.Format = pTexture->desc.Format;
			if (pTexture->desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
			{
				shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			}
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
		if (pTexture->desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
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
		if (pTexture->desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
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
		if (desc[i].AlignedByteOffset == APPEND_ALIGNED_ELEMENT)
			desc[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
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

	if (RasterizedStream == SO_NO_RASTERIZED_STREAM)
		RasterizedStream = D3D11_SO_NO_RASTERIZED_STREAM;

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

	oddFrame = !oddFrame;

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
