#include "wiGraphicsAPI_DX11.h"
#include "wiHelper.h"
#include "ResourceMapping.h"

#include "Utility/WicTextureLoader.h"
#include "Utility/DDSTextureLoader.h"
#include "Utility/ScreenGrab.h"

namespace wiGraphicsTypes
{

#ifndef WINSTORE_SUPPORT
GraphicsDevice_DX11::GraphicsDevice_DX11(HWND window, int screenW, int screenH, bool windowed) : GraphicsDevice()
#else
GraphicsDevice_DX11::GraphicsDevice_DX11(Windows::UI::Core::CoreWindow^ window)
#endif
{
	HRESULT hr = S_OK;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
#ifndef WINSTORE_SUPPORT
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
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
	SCREENWIDTH = screenW;
	SCREENHEIGHT = screenH;

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = SCREENWIDTH;
	sd.BufferDesc.Height = SCREENHEIGHT;
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
		hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &device
			, &featureLevel, &deviceContexts[GRAPHICSTHREAD_IMMEDIATE]);
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
	sd.Scaling = DXGI_SCALING_STRETCH;
	sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

	IDXGIDevice2 * pDXGIDevice;
	hr = device->QueryInterface(__uuidof(IDXGIDevice2), (void **)&pDXGIDevice);

	IDXGIAdapter * pDXGIAdapter;
	hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter);

	IDXGIFactory2 * pIDXGIFactory;
	pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), (void **)&pIDXGIFactory);


	hr = pIDXGIFactory->CreateSwapChainForCoreWindow(device, reinterpret_cast<IUnknown*>(window), &sd
		, nullptr, &swapChain);

	if (FAILED(hr)) {
		wiHelper::messageBox("Swap chain creation failed!", "Error!");
		exit(1);
	}

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
	// ensures that the application will only render after each VSync, minimizing power consumption.
	hr = pDXGIDevice->SetMaximumFrameLatency(1);
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

	hr = deviceContexts[GRAPHICSTHREAD_IMMEDIATE]->QueryInterface(__uuidof(userDefinedAnnotation), 
		reinterpret_cast<void**>(&userDefinedAnnotation));

	// Create a render target view
	backBuffer = NULL;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	if (FAILED(hr)) {
		wiHelper::messageBox("BackBuffer creation Failed!", "Error!", nullptr);
		exit(0);
	}

	hr = device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);
	//pBackBuffer->Release();
	if (FAILED(hr)) {
		wiHelper::messageBox("Main Rendertarget creation Failed!", "Error!", nullptr);
		exit(0);
	}


	// Setup the main viewport
	viewPort.Width = (FLOAT)SCREENWIDTH;
	viewPort.Height = (FLOAT)SCREENHEIGHT;
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

Texture2D GraphicsDevice_DX11::GetBackBuffer()
{
	Texture2D result;
	result.texture2D_DX11 = backBuffer;
	backBuffer->AddRef();
	return result;
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
	if (value & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
		_flag |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
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
inline DXGI_FORMAT _ConvertFormat(FORMAT value)
{
	switch (value)
	{
	case FORMAT_UNKNOWN:
		return DXGI_FORMAT_UNKNOWN;
		break;
	case FORMAT_R32G32B32A32_TYPELESS:
		return DXGI_FORMAT_R32G32B32A32_TYPELESS;
		break;
	case FORMAT_R32G32B32A32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	case FORMAT_R32G32B32A32_UINT:
		return DXGI_FORMAT_R32G32B32A32_UINT;
		break;
	case FORMAT_R32G32B32A32_SINT:
		return DXGI_FORMAT_R32G32B32A32_SINT;
		break;
	case FORMAT_R32G32B32_TYPELESS:
		return DXGI_FORMAT_R32G32B32_TYPELESS;
		break;
	case FORMAT_R32G32B32_FLOAT:
		return DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	case FORMAT_R32G32B32_UINT:
		return DXGI_FORMAT_R32G32B32_UINT;
		break;
	case FORMAT_R32G32B32_SINT:
		return DXGI_FORMAT_R32G32B32_SINT;
		break;
	case FORMAT_R16G16B16A16_TYPELESS:
		return DXGI_FORMAT_R16G16B16A16_TYPELESS;
		break;
	case FORMAT_R16G16B16A16_FLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	case FORMAT_R16G16B16A16_UNORM:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
		break;
	case FORMAT_R16G16B16A16_UINT:
		return DXGI_FORMAT_R16G16B16A16_UINT;
		break;
	case FORMAT_R16G16B16A16_SNORM:
		return DXGI_FORMAT_R16G16B16A16_SNORM;
		break;
	case FORMAT_R16G16B16A16_SINT:
		return DXGI_FORMAT_R16G16B16A16_SINT;
		break;
	case FORMAT_R32G32_TYPELESS:
		return DXGI_FORMAT_R32G32_TYPELESS;
		break;
	case FORMAT_R32G32_FLOAT:
		return DXGI_FORMAT_R32G32_FLOAT;
		break;
	case FORMAT_R32G32_UINT:
		return DXGI_FORMAT_R32G32_UINT;
		break;
	case FORMAT_R32G32_SINT:
		return DXGI_FORMAT_R32G32_SINT;
		break;
	case FORMAT_R32G8X24_TYPELESS:
		return DXGI_FORMAT_R32G8X24_TYPELESS;
		break;
	case FORMAT_D32_FLOAT_S8X24_UINT:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		break;
	case FORMAT_R32_FLOAT_X8X24_TYPELESS:
		return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		break;
	case FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
		break;
	case FORMAT_R10G10B10A2_TYPELESS:
		return DXGI_FORMAT_R10G10B10A2_TYPELESS;
		break;
	case FORMAT_R10G10B10A2_UNORM:
		return DXGI_FORMAT_R10G10B10A2_UNORM;
		break;
	case FORMAT_R10G10B10A2_UINT:
		return DXGI_FORMAT_R10G10B10A2_UINT;
		break;
	case FORMAT_R11G11B10_FLOAT:
		return DXGI_FORMAT_R11G11B10_FLOAT;
		break;
	case FORMAT_R8G8B8A8_TYPELESS:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;
		break;
	case FORMAT_R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		break;
	case FORMAT_R8G8B8A8_UINT:
		return DXGI_FORMAT_R8G8B8A8_UINT;
		break;
	case FORMAT_R8G8B8A8_SNORM:
		return DXGI_FORMAT_R8G8B8A8_SNORM;
		break;
	case FORMAT_R8G8B8A8_SINT:
		return DXGI_FORMAT_R8G8B8A8_SINT;
		break;
	case FORMAT_R16G16_TYPELESS:
		return DXGI_FORMAT_R16G16_TYPELESS;
		break;
	case FORMAT_R16G16_FLOAT:
		return DXGI_FORMAT_R16G16_FLOAT;
		break;
	case FORMAT_R16G16_UNORM:
		return DXGI_FORMAT_R16G16_UNORM;
		break;
	case FORMAT_R16G16_UINT:
		return DXGI_FORMAT_R16G16_UINT;
		break;
	case FORMAT_R16G16_SNORM:
		return DXGI_FORMAT_R16G16_SNORM;
		break;
	case FORMAT_R16G16_SINT:
		return DXGI_FORMAT_R16G16_SINT;
		break;
	case FORMAT_R32_TYPELESS:
		return DXGI_FORMAT_R32_TYPELESS;
		break;
	case FORMAT_D32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;
		break;
	case FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
		break;
	case FORMAT_R32_UINT:
		return DXGI_FORMAT_R32_UINT;
		break;
	case FORMAT_R32_SINT:
		return DXGI_FORMAT_R32_SINT;
		break;
	case FORMAT_R24G8_TYPELESS:
		return DXGI_FORMAT_R24G8_TYPELESS;
		break;
	case FORMAT_D24_UNORM_S8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	case FORMAT_R24_UNORM_X8_TYPELESS:
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;
	case FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		break;
	case FORMAT_R8G8_TYPELESS:
		return DXGI_FORMAT_R8G8_TYPELESS;
		break;
	case FORMAT_R8G8_UNORM:
		return DXGI_FORMAT_R8G8_UNORM;
		break;
	case FORMAT_R8G8_UINT:
		return DXGI_FORMAT_R8G8_UINT;
		break;
	case FORMAT_R8G8_SNORM:
		return DXGI_FORMAT_R8G8_SNORM;
		break;
	case FORMAT_R8G8_SINT:
		return DXGI_FORMAT_R8G8_SINT;
		break;
	case FORMAT_R16_TYPELESS:
		return DXGI_FORMAT_R16_TYPELESS;
		break;
	case FORMAT_R16_FLOAT:
		return DXGI_FORMAT_R16_FLOAT;
		break;
	case FORMAT_D16_UNORM:
		return DXGI_FORMAT_D16_UNORM;
		break;
	case FORMAT_R16_UNORM:
		return DXGI_FORMAT_R16_UNORM;
		break;
	case FORMAT_R16_UINT:
		return DXGI_FORMAT_R16_UINT;
		break;
	case FORMAT_R16_SNORM:
		return DXGI_FORMAT_R16_SNORM;
		break;
	case FORMAT_R16_SINT:
		return DXGI_FORMAT_R16_SINT;
		break;
	case FORMAT_R8_TYPELESS:
		return DXGI_FORMAT_R8_TYPELESS;
		break;
	case FORMAT_R8_UNORM:
		return DXGI_FORMAT_R8_UNORM;
		break;
	case FORMAT_R8_UINT:
		return DXGI_FORMAT_R8_UINT;
		break;
	case FORMAT_R8_SNORM:
		return DXGI_FORMAT_R8_SNORM;
		break;
	case FORMAT_R8_SINT:
		return DXGI_FORMAT_R8_SINT;
		break;
	case FORMAT_A8_UNORM:
		return DXGI_FORMAT_A8_UNORM;
		break;
	case FORMAT_R1_UNORM:
		return DXGI_FORMAT_R1_UNORM;
		break;
	case FORMAT_R9G9B9E5_SHAREDEXP:
		return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
		break;
	case FORMAT_R8G8_B8G8_UNORM:
		return DXGI_FORMAT_R8G8_B8G8_UNORM;
		break;
	case FORMAT_G8R8_G8B8_UNORM:
		return DXGI_FORMAT_G8R8_G8B8_UNORM;
		break;
	case FORMAT_BC1_TYPELESS:
		return DXGI_FORMAT_BC1_TYPELESS;
		break;
	case FORMAT_BC1_UNORM:
		return DXGI_FORMAT_BC1_UNORM;
		break;
	case FORMAT_BC1_UNORM_SRGB:
		return DXGI_FORMAT_BC1_UNORM_SRGB;
		break;
	case FORMAT_BC2_TYPELESS:
		return DXGI_FORMAT_BC2_TYPELESS;
		break;
	case FORMAT_BC2_UNORM:
		return DXGI_FORMAT_BC2_UNORM;
		break;
	case FORMAT_BC2_UNORM_SRGB:
		return DXGI_FORMAT_BC2_UNORM_SRGB;
		break;
	case FORMAT_BC3_TYPELESS:
		return DXGI_FORMAT_BC3_TYPELESS;
		break;
	case FORMAT_BC3_UNORM:
		return DXGI_FORMAT_BC3_UNORM;
		break;
	case FORMAT_BC3_UNORM_SRGB:
		return DXGI_FORMAT_BC3_UNORM_SRGB;
		break;
	case FORMAT_BC4_TYPELESS:
		return DXGI_FORMAT_BC4_TYPELESS;
		break;
	case FORMAT_BC4_UNORM:
		return DXGI_FORMAT_BC4_UNORM;
		break;
	case FORMAT_BC4_SNORM:
		return DXGI_FORMAT_BC4_SNORM;
		break;
	case FORMAT_BC5_TYPELESS:
		return DXGI_FORMAT_BC5_TYPELESS;
		break;
	case FORMAT_BC5_UNORM:
		return DXGI_FORMAT_BC5_UNORM;
		break;
	case FORMAT_BC5_SNORM:
		return DXGI_FORMAT_BC5_SNORM;
		break;
	case FORMAT_B5G6R5_UNORM:
		return DXGI_FORMAT_B5G6R5_UNORM;
		break;
	case FORMAT_B5G5R5A1_UNORM:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
		break;
	case FORMAT_B8G8R8A8_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
		break;
	case FORMAT_B8G8R8X8_UNORM:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
		break;
	case FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
		break;
	case FORMAT_B8G8R8A8_TYPELESS:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;
		break;
	case FORMAT_B8G8R8A8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		break;
	case FORMAT_B8G8R8X8_TYPELESS:
		return DXGI_FORMAT_B8G8R8X8_TYPELESS;
		break;
	case FORMAT_B8G8R8X8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		break;
	case FORMAT_BC6H_TYPELESS:
		return DXGI_FORMAT_BC6H_TYPELESS;
		break;
	case FORMAT_BC6H_UF16:
		return DXGI_FORMAT_BC6H_UF16;
		break;
	case FORMAT_BC6H_SF16:
		return DXGI_FORMAT_BC6H_SF16;
		break;
	case FORMAT_BC7_TYPELESS:
		return DXGI_FORMAT_BC7_TYPELESS;
		break;
	case FORMAT_BC7_UNORM:
		return DXGI_FORMAT_BC7_UNORM;
		break;
	case FORMAT_BC7_UNORM_SRGB:
		return DXGI_FORMAT_BC7_UNORM_SRGB;
		break;
	case FORMAT_AYUV:
		return DXGI_FORMAT_AYUV;
		break;
	case FORMAT_Y410:
		return DXGI_FORMAT_Y410;
		break;
	case FORMAT_Y416:
		return DXGI_FORMAT_Y416;
		break;
	case FORMAT_NV12:
		return DXGI_FORMAT_NV12;
		break;
	case FORMAT_P010:
		return DXGI_FORMAT_P010;
		break;
	case FORMAT_P016:
		return DXGI_FORMAT_P016;
		break;
	case FORMAT_420_OPAQUE:
		return DXGI_FORMAT_420_OPAQUE;
		break;
	case FORMAT_YUY2:
		return DXGI_FORMAT_YUY2;
		break;
	case FORMAT_Y210:
		return DXGI_FORMAT_Y210;
		break;
	case FORMAT_Y216:
		return DXGI_FORMAT_Y216;
		break;
	case FORMAT_NV11:
		return DXGI_FORMAT_NV11;
		break;
	case FORMAT_AI44:
		return DXGI_FORMAT_AI44;
		break;
	case FORMAT_IA44:
		return DXGI_FORMAT_IA44;
		break;
	case FORMAT_P8:
		return DXGI_FORMAT_P8;
		break;
	case FORMAT_A8P8:
		return DXGI_FORMAT_A8P8;
		break;
	case FORMAT_B4G4R4A4_UNORM:
		return DXGI_FORMAT_B4G4R4A4_UNORM;
		break;
	case FORMAT_FORCE_UINT:
		return DXGI_FORMAT_FORCE_UINT;
		break;
	default:
		break;
	}
	return DXGI_FORMAT_UNKNOWN;
}

inline D3D11_TEXTURE2D_DESC _ConvertTexture2DDesc(const Texture2DDesc* pDesc)
{
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = pDesc->Width;
	desc.Height = pDesc->Height;
	desc.MipLevels = pDesc->MipLevels;
	desc.ArraySize = pDesc->ArraySize;
	desc.Format = _ConvertFormat(pDesc->Format);
	desc.SampleDesc.Count = pDesc->SampleDesc.Count;
	desc.SampleDesc.Quality = pDesc->SampleDesc.Quality;
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


HRESULT GraphicsDevice_DX11::CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer)
{
	D3D11_BUFFER_DESC desc; 
	desc.ByteWidth = pDesc->ByteWidth;
	desc.Usage = _ConvertUsage(pDesc->Usage);
	desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);
	desc.StructureByteStride = pDesc->StructureByteStride;

	D3D11_SUBRESOURCE_DATA* data = _ConvertSubresourceData(pInitialData);

	ppBuffer->desc = *pDesc;
	HRESULT hr = device->CreateBuffer(&desc, data, &ppBuffer->resource_DX11);
	assert(SUCCEEDED(hr) && "GPUBuffer Creation failed!");

	if (SUCCEEDED(hr))
	{
		// Create resource views if needed
		if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
		{

			D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
			ZeroMemory(&srv_desc, sizeof(srv_desc));
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			srv_desc.BufferEx.FirstElement = 0;

			if (desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer

				srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
				srv_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
				srv_desc.BufferEx.NumElements = desc.ByteWidth / 4;
			}
			else if (desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer

				srv_desc.Format = DXGI_FORMAT_UNKNOWN;
				srv_desc.BufferEx.NumElements = desc.ByteWidth / desc.StructureByteStride;
			}

			hr = device->CreateShaderResourceView(ppBuffer->resource_DX11, &srv_desc, &ppBuffer->shaderResourceView_DX11);

			assert(SUCCEEDED(hr) && "ShaderResourceView of the GPUBuffer could not be created!");
		}
		if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
			ZeroMemory(&uav_desc, sizeof(uav_desc));
			uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;

			if (desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer

				uav_desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
				uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
				uav_desc.Buffer.NumElements = desc.ByteWidth / 4;
			}
			else if (desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer

				uav_desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
				uav_desc.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
			}

			hr = device->CreateUnorderedAccessView(ppBuffer->resource_DX11, &uav_desc, &ppBuffer->unorderedAccessView_DX11);

			assert(SUCCEEDED(hr) && "UnorderedAccessView of the GPUBuffer could not be created!");
		}
	}

	return hr;
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
	
	hr = device->CreateTexture2D(&desc, data, &((*ppTexture2D)->texture2D_DX11));
	if (FAILED(hr))
		return hr;

	CreateRenderTargetView(*ppTexture2D);
	CreateShaderResourceView(*ppTexture2D);
	CreateDepthStencilView(*ppTexture2D);


	if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
		ZeroMemory(&uav_desc, sizeof(uav_desc));
		uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uav_desc.Buffer.FirstElement = 0;
		uav_desc.Format = desc.Format;
		uav_desc.Buffer.NumElements = desc.Width * desc.Height;

		hr = device->CreateUnorderedAccessView((*ppTexture2D)->texture2D_DX11, &uav_desc, &(*ppTexture2D)->unorderedAccessView_DX11);

		assert(SUCCEEDED(hr) && "UnorderedAccessView of the Texture2D could not be created!");
	}

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

	hr = device->CreateTexture2D(&desc, data, &((*ppTextureCube)->texture2D_DX11));
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
			shaderResourceViewDesc.Format = _ConvertFormat(pTexture->desc.Format);
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
			shaderResourceViewDesc.Format = _ConvertFormat(pTexture->desc.Format);
			if (pTexture->desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
			{
				shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			}
			shaderResourceViewDesc.ViewDimension = (pTexture->desc.SampleDesc.Quality == 0 ? D3D11_SRV_DIMENSION_TEXTURE2D : D3D11_SRV_DIMENSION_TEXTURE2DMS);
			shaderResourceViewDesc.Texture2D.MostDetailedMip = 0; //from most detailed...
			shaderResourceViewDesc.Texture2D.MipLevels = -1; //...to least detailed
		}

		hr = device->CreateShaderResourceView(pTexture->texture2D_DX11, &shaderResourceViewDesc, &pTexture->shaderResourceView_DX11);
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
			renderTargetViewDesc.Format = _ConvertFormat(pTexture->desc.Format);
			renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
			renderTargetViewDesc.Texture2DArray.ArraySize = 6;
			renderTargetViewDesc.Texture2DArray.MipSlice = 0;
		}
		else
		{
			renderTargetViewDesc.Format = _ConvertFormat(pTexture->desc.Format);
			renderTargetViewDesc.ViewDimension = (pTexture->desc.SampleDesc.Quality == 0 ? D3D11_RTV_DIMENSION_TEXTURE2D : D3D11_RTV_DIMENSION_TEXTURE2DMS);
			renderTargetViewDesc.Texture2D.MipSlice = 0;
		}

		hr = device->CreateRenderTargetView(pTexture->texture2D_DX11, &renderTargetViewDesc, &pTexture->renderTargetView_DX11);
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

		hr = device->CreateDepthStencilView(pTexture->texture2D_DX11, &depthStencilViewDesc, &pTexture->depthStencilView_DX11);
	}
	return hr;
}
HRESULT GraphicsDevice_DX11::CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
	const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout)
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

	HRESULT hr = device->CreateInputLayout(desc, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, &pInputLayout->resource_DX11);

	SAFE_DELETE_ARRAY(desc);

	return hr;
}
HRESULT GraphicsDevice_DX11::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, VertexShader *pVertexShader)
{
	return device->CreateVertexShader(pShaderBytecode, BytecodeLength, (pClassLinkage == nullptr ? nullptr : pClassLinkage->resource_DX11), &pVertexShader->resource_DX11);
}
HRESULT GraphicsDevice_DX11::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, PixelShader *pPixelShader)
{
	return device->CreatePixelShader(pShaderBytecode, BytecodeLength, (pClassLinkage == nullptr ? nullptr : pClassLinkage->resource_DX11), &pPixelShader->resource_DX11);
}
HRESULT GraphicsDevice_DX11::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, GeometryShader *pGeometryShader)
{
	return device->CreateGeometryShader(pShaderBytecode, BytecodeLength, (pClassLinkage == nullptr ? nullptr : pClassLinkage->resource_DX11), &pGeometryShader->resource_DX11);
}
HRESULT GraphicsDevice_DX11::CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode, SIZE_T BytecodeLength, const StreamOutDeclaration *pSODeclaration,
	UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ClassLinkage* pClassLinkage, GeometryShader *pGeometryShader)
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
		NumStrides, RasterizedStream, (pClassLinkage == nullptr ? nullptr : pClassLinkage->resource_DX11), &pGeometryShader->resource_DX11);

	SAFE_DELETE_ARRAY(decl);

	return hr;
}
HRESULT GraphicsDevice_DX11::CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, HullShader *pHullShader)
{
	return device->CreateHullShader(pShaderBytecode, BytecodeLength, (pClassLinkage == nullptr ? nullptr : pClassLinkage->resource_DX11), &pHullShader->resource_DX11);
}
HRESULT GraphicsDevice_DX11::CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, DomainShader *pDomainShader)
{
	return device->CreateDomainShader(pShaderBytecode, BytecodeLength, (pClassLinkage == nullptr ? nullptr : pClassLinkage->resource_DX11), &pDomainShader->resource_DX11);
}
HRESULT GraphicsDevice_DX11::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, ComputeShader *pComputeShader)
{
	return device->CreateComputeShader(pShaderBytecode, BytecodeLength, (pClassLinkage == nullptr ? nullptr : pClassLinkage->resource_DX11), &pComputeShader->resource_DX11);
}
HRESULT GraphicsDevice_DX11::CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState)
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

	pBlendState->desc = *pBlendStateDesc;
	return device->CreateBlendState(&desc, &pBlendState->resource_DX11);
}
HRESULT GraphicsDevice_DX11::CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState)
{
	D3D11_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = pDepthStencilStateDesc->DepthEnable;
	desc.DepthWriteMask = _ConvertDepthWriteMask(pDepthStencilStateDesc->DepthWriteMask);
	desc.DepthFunc = _ConvertComparisonFunc(pDepthStencilStateDesc->DepthFunc);
	desc.StencilEnable = pDepthStencilStateDesc->StencilEnable;
	desc.StencilReadMask = pDepthStencilStateDesc->StencilReadMask;
	desc.StencilWriteMask = pDepthStencilStateDesc->StencilWriteMask;
	desc.FrontFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc->FrontFace.StencilDepthFailOp);
	desc.FrontFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc->FrontFace.StencilFailOp);
	desc.FrontFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc->FrontFace.StencilFunc);
	desc.FrontFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc->FrontFace.StencilPassOp);
	desc.BackFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc->BackFace.StencilDepthFailOp);
	desc.BackFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc->BackFace.StencilFailOp);
	desc.BackFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc->BackFace.StencilFunc);
	desc.BackFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc->BackFace.StencilPassOp);

	pDepthStencilState->desc = *pDepthStencilStateDesc;
	return device->CreateDepthStencilState(&desc, &pDepthStencilState->resource_DX11);
}
HRESULT GraphicsDevice_DX11::CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState)
{
	D3D11_RASTERIZER_DESC desc;
	desc.FillMode = _ConvertFillMode(pRasterizerStateDesc->FillMode);
	desc.CullMode = _ConvertCullMode(pRasterizerStateDesc->CullMode);
	desc.FrontCounterClockwise = pRasterizerStateDesc->FrontCounterClockwise;
	desc.DepthBias = pRasterizerStateDesc->DepthBias;
	desc.DepthBiasClamp = pRasterizerStateDesc->DepthBiasClamp;
	desc.SlopeScaledDepthBias = pRasterizerStateDesc->SlopeScaledDepthBias;
	desc.DepthClipEnable = pRasterizerStateDesc->DepthClipEnable;
	desc.ScissorEnable = pRasterizerStateDesc->ScissorEnable;
	desc.MultisampleEnable = pRasterizerStateDesc->MultisampleEnable;
	desc.AntialiasedLineEnable = pRasterizerStateDesc->AntialiasedLineEnable;

	pRasterizerState->desc = *pRasterizerStateDesc;
	return device->CreateRasterizerState(&desc, &pRasterizerState->resource_DX11);
}
HRESULT GraphicsDevice_DX11::CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState)
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

	pSamplerState->desc = *pSamplerDesc;
	return device->CreateSamplerState(&desc, &pSamplerState->resource_DX11);
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

	UnBindResources(0, TEXSLOT_COUNT, GRAPHICSTHREAD_IMMEDIATE);

	FRAMECOUNT++;

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

			UnBindResources(0, TEXSLOT_COUNT, (GRAPHICSTHREAD)i);
		}
	}
}
void GraphicsDevice_DX11::FinishCommandList(GRAPHICSTHREAD thread)
{
	if (thread == GRAPHICSTHREAD_IMMEDIATE)
		return;
	deviceContexts[thread]->FinishCommandList(true, &commandLists[thread]);
}

void GraphicsDevice_DX11::EventBegin(const wchar_t* name)
{
	userDefinedAnnotation->BeginEvent(name);
}
void GraphicsDevice_DX11::EventEnd()
{
	userDefinedAnnotation->EndEvent();
}
void GraphicsDevice_DX11::SetMarker(const wchar_t* name)
{
	userDefinedAnnotation->SetMarker(name);
}

HRESULT GraphicsDevice_DX11::CreateTextureFromFile(const wstring& fileName, Texture2D **ppTexture, bool mipMaps, GRAPHICSTHREAD threadID)
{
	HRESULT hr = E_FAIL;
	(*ppTexture) = new Texture2D();

	if (!fileName.substr(fileName.length() - 4).compare(wstring(L".dds")))
	{
		// Load dds
		hr = CreateDDSTextureFromFile(device, fileName.c_str(), (ID3D11Resource**)&(*ppTexture)->texture2D_DX11, &(*ppTexture)->shaderResourceView_DX11);
	}
	else
	{
		// Load WIC
		if (mipMaps && threadID == GRAPHICSTHREAD_IMMEDIATE)
			LOCK();
		hr = CreateWICTextureFromFile(mipMaps, device, deviceContexts[threadID], fileName.c_str(), (ID3D11Resource**)&(*ppTexture)->texture2D_DX11, &(*ppTexture)->shaderResourceView_DX11);
		if (mipMaps && threadID == GRAPHICSTHREAD_IMMEDIATE)
			UNLOCK();
	}

	if (FAILED(hr))
		SAFE_DELETE(*ppTexture);

	return hr;
}
HRESULT GraphicsDevice_DX11::SaveTexturePNG(const wstring& fileName, Texture2D *pTexture, GRAPHICSTHREAD threadID)
{
	return SaveWICTextureToFile(deviceContexts[threadID], pTexture->texture2D_DX11, GUID_ContainerFormatPng, fileName.c_str());
}
HRESULT GraphicsDevice_DX11::SaveTextureDDS(const wstring& fileName, Texture *pTexture, GRAPHICSTHREAD threadID)
{
	Texture2D* tex2D = dynamic_cast<Texture2D*>(pTexture);
	if (tex2D != nullptr)
	{
		return SaveDDSTextureToFile(deviceContexts[threadID], tex2D->texture2D_DX11, fileName.c_str());
	}
	return E_FAIL;
}

}
