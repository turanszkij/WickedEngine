#include "wiGraphicsDevice_DX12.h"

#ifdef WICKEDENGINE_BUILD_DX12

#include "wiGraphicsDevice_SharedInternals.h"
#include "wiHelper.h"
#include "wiMath.h"
#include "ResourceMapping.h"
#include "wiBackLog.h"
#include "wiStartupArguments.h"

#include "Utility/dx12/d3dx12.h"
#include "Utility/D3D12MemAlloc.h"

#include "Utility/dxcapi.h"
#include "Utility/dx12/d3d12shader.h"

#include <pix.h>

#ifdef _DEBUG
#include <d3d12sdklayers.h>
#endif // _DEBUG

#include <sstream>
#include <algorithm>
#include <wincodec.h>

// Choose how many constant buffers will be placed in root in auto root signature:
#define CONSTANT_BUFFER_AUTO_PLACEMENT_IN_ROOT 4
static_assert(GPU_RESOURCE_HEAP_CBV_COUNT < 32, "cbv root mask must fit into uint32_t!");

using namespace Microsoft::WRL;

namespace wiGraphics
{

namespace DX12_Internal
{

#ifdef PLATFORM_UWP
	// UWP will use static link + /DELAYLOAD linker feature for the dlls (optionally)
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#else
	using PFN_CREATE_DXGI_FACTORY_2 = decltype(&CreateDXGIFactory2);
	static PFN_CREATE_DXGI_FACTORY_2 CreateDXGIFactory2 = nullptr;
	static PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
	static PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature = nullptr;
#endif // PLATFORM_UWP

	ComPtr<IDxcUtils> dxcUtils;

	// Engine -> Native converters

	inline uint32_t _ParseColorWriteMask(uint32_t value)
	{
		uint32_t _flag = 0;

		if (value == D3D12_COLOR_WRITE_ENABLE_ALL)
		{
			return D3D12_COLOR_WRITE_ENABLE_ALL;
		}
		else
		{
			if (value & COLOR_WRITE_ENABLE_RED)
				_flag |= D3D12_COLOR_WRITE_ENABLE_RED;
			if (value & COLOR_WRITE_ENABLE_GREEN)
				_flag |= D3D12_COLOR_WRITE_ENABLE_GREEN;
			if (value & COLOR_WRITE_ENABLE_BLUE)
				_flag |= D3D12_COLOR_WRITE_ENABLE_BLUE;
			if (value & COLOR_WRITE_ENABLE_ALPHA)
				_flag |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
		}

		return _flag;
	}

	constexpr D3D12_FILTER _ConvertFilter(FILTER value)
	{
		switch (value)
		{
		case FILTER_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
			break;
		case FILTER_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case FILTER_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		case FILTER_ANISOTROPIC:
			return D3D12_FILTER_ANISOTROPIC;
			break;
		case FILTER_COMPARISON_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
			break;
		case FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
			break;
		case FILTER_COMPARISON_ANISOTROPIC:
			return D3D12_FILTER_COMPARISON_ANISOTROPIC;
			break;
		case FILTER_MINIMUM_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
			break;
		case FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
			break;
		case FILTER_MINIMUM_ANISOTROPIC:
			return D3D12_FILTER_MINIMUM_ANISOTROPIC;
			break;
		case FILTER_MAXIMUM_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
			break;
		case FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
			break;
		case FILTER_MAXIMUM_ANISOTROPIC:
			return D3D12_FILTER_MAXIMUM_ANISOTROPIC;
			break;
		default:
			break;
		}
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	}
	constexpr D3D12_TEXTURE_ADDRESS_MODE _ConvertTextureAddressMode(TEXTURE_ADDRESS_MODE value)
	{
		switch (value)
		{
		case TEXTURE_ADDRESS_WRAP:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			break;
		case TEXTURE_ADDRESS_MIRROR:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			break;
		case TEXTURE_ADDRESS_CLAMP:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			break;
		case TEXTURE_ADDRESS_BORDER:
			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			break;
		default:
			break;
		}
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
	constexpr D3D12_COMPARISON_FUNC _ConvertComparisonFunc(COMPARISON_FUNC value)
	{
		switch (value)
		{
		case COMPARISON_NEVER:
			return D3D12_COMPARISON_FUNC_NEVER;
			break;
		case COMPARISON_LESS:
			return D3D12_COMPARISON_FUNC_LESS;
			break;
		case COMPARISON_EQUAL:
			return D3D12_COMPARISON_FUNC_EQUAL;
			break;
		case COMPARISON_LESS_EQUAL:
			return D3D12_COMPARISON_FUNC_LESS_EQUAL;
			break;
		case COMPARISON_GREATER:
			return D3D12_COMPARISON_FUNC_GREATER;
			break;
		case COMPARISON_NOT_EQUAL:
			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
			break;
		case COMPARISON_GREATER_EQUAL:
			return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			break;
		case COMPARISON_ALWAYS:
			return D3D12_COMPARISON_FUNC_ALWAYS;
			break;
		default:
			break;
		}
		return D3D12_COMPARISON_FUNC_NEVER;
	}
	constexpr D3D12_FILL_MODE _ConvertFillMode(FILL_MODE value)
	{
		switch (value)
		{
		case FILL_WIREFRAME:
			return D3D12_FILL_MODE_WIREFRAME;
			break;
		case FILL_SOLID:
			return D3D12_FILL_MODE_SOLID;
			break;
		default:
			break;
		}
		return D3D12_FILL_MODE_WIREFRAME;
	}
	constexpr D3D12_CULL_MODE _ConvertCullMode(CULL_MODE value)
	{
		switch (value)
		{
		case CULL_NONE:
			return D3D12_CULL_MODE_NONE;
			break;
		case CULL_FRONT:
			return D3D12_CULL_MODE_FRONT;
			break;
		case CULL_BACK:
			return D3D12_CULL_MODE_BACK;
			break;
		default:
			break;
		}
		return D3D12_CULL_MODE_NONE;
	}
	constexpr D3D12_DEPTH_WRITE_MASK _ConvertDepthWriteMask(DEPTH_WRITE_MASK value)
	{
		switch (value)
		{
		case DEPTH_WRITE_MASK_ZERO:
			return D3D12_DEPTH_WRITE_MASK_ZERO;
			break;
		case DEPTH_WRITE_MASK_ALL:
			return D3D12_DEPTH_WRITE_MASK_ALL;
			break;
		default:
			break;
		}
		return D3D12_DEPTH_WRITE_MASK_ZERO;
	}
	constexpr D3D12_STENCIL_OP _ConvertStencilOp(STENCIL_OP value)
	{
		switch (value)
		{
		case STENCIL_OP_KEEP:
			return D3D12_STENCIL_OP_KEEP;
			break;
		case STENCIL_OP_ZERO:
			return D3D12_STENCIL_OP_ZERO;
			break;
		case STENCIL_OP_REPLACE:
			return D3D12_STENCIL_OP_REPLACE;
			break;
		case STENCIL_OP_INCR_SAT:
			return D3D12_STENCIL_OP_INCR_SAT;
			break;
		case STENCIL_OP_DECR_SAT:
			return D3D12_STENCIL_OP_DECR_SAT;
			break;
		case STENCIL_OP_INVERT:
			return D3D12_STENCIL_OP_INVERT;
			break;
		case STENCIL_OP_INCR:
			return D3D12_STENCIL_OP_INCR;
			break;
		case STENCIL_OP_DECR:
			return D3D12_STENCIL_OP_DECR;
			break;
		default:
			break;
		}
		return D3D12_STENCIL_OP_KEEP;
	}
	constexpr D3D12_BLEND _ConvertBlend(BLEND value)
	{
		switch (value)
		{
		case BLEND_ZERO:
			return D3D12_BLEND_ZERO;
			break;
		case BLEND_ONE:
			return D3D12_BLEND_ONE;
			break;
		case BLEND_SRC_COLOR:
			return D3D12_BLEND_SRC_COLOR;
			break;
		case BLEND_INV_SRC_COLOR:
			return D3D12_BLEND_INV_SRC_COLOR;
			break;
		case BLEND_SRC_ALPHA:
			return D3D12_BLEND_SRC_ALPHA;
			break;
		case BLEND_INV_SRC_ALPHA:
			return D3D12_BLEND_INV_SRC_ALPHA;
			break;
		case BLEND_DEST_ALPHA:
			return D3D12_BLEND_DEST_ALPHA;
			break;
		case BLEND_INV_DEST_ALPHA:
			return D3D12_BLEND_INV_DEST_ALPHA;
			break;
		case BLEND_DEST_COLOR:
			return D3D12_BLEND_DEST_COLOR;
			break;
		case BLEND_INV_DEST_COLOR:
			return D3D12_BLEND_INV_DEST_COLOR;
			break;
		case BLEND_SRC_ALPHA_SAT:
			return D3D12_BLEND_SRC_ALPHA_SAT;
			break;
		case BLEND_BLEND_FACTOR:
			return D3D12_BLEND_BLEND_FACTOR;
			break;
		case BLEND_INV_BLEND_FACTOR:
			return D3D12_BLEND_INV_BLEND_FACTOR;
			break;
		case BLEND_SRC1_COLOR:
			return D3D12_BLEND_SRC1_COLOR;
			break;
		case BLEND_INV_SRC1_COLOR:
			return D3D12_BLEND_INV_SRC1_COLOR;
			break;
		case BLEND_SRC1_ALPHA:
			return D3D12_BLEND_SRC1_ALPHA;
			break;
		case BLEND_INV_SRC1_ALPHA:
			return D3D12_BLEND_INV_SRC1_ALPHA;
			break;
		default:
			break;
		}
		return D3D12_BLEND_ZERO;
	}
	constexpr D3D12_BLEND_OP _ConvertBlendOp(BLEND_OP value)
	{
		switch (value)
		{
		case BLEND_OP_ADD:
			return D3D12_BLEND_OP_ADD;
			break;
		case BLEND_OP_SUBTRACT:
			return D3D12_BLEND_OP_SUBTRACT;
			break;
		case BLEND_OP_REV_SUBTRACT:
			return D3D12_BLEND_OP_REV_SUBTRACT;
			break;
		case BLEND_OP_MIN:
			return D3D12_BLEND_OP_MIN;
			break;
		case BLEND_OP_MAX:
			return D3D12_BLEND_OP_MAX;
			break;
		default:
			break;
		}
		return D3D12_BLEND_OP_ADD;
	}
	constexpr D3D12_INPUT_CLASSIFICATION _ConvertInputClassification(INPUT_CLASSIFICATION value)
	{
		switch (value)
		{
		case INPUT_PER_VERTEX_DATA:
			return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			break;
		case INPUT_PER_INSTANCE_DATA:
			return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
			break;
		default:
			break;
		}
		return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	}
	constexpr DXGI_FORMAT _ConvertFormat(FORMAT value)
	{
		switch (value)
		{
		case FORMAT_UNKNOWN:
			return DXGI_FORMAT_UNKNOWN;
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
		case FORMAT_R32G32B32_FLOAT:
			return DXGI_FORMAT_R32G32B32_FLOAT;
			break;
		case FORMAT_R32G32B32_UINT:
			return DXGI_FORMAT_R32G32B32_UINT;
			break;
		case FORMAT_R32G32B32_SINT:
			return DXGI_FORMAT_R32G32B32_SINT;
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
		case FORMAT_R10G10B10A2_UNORM:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
			break;
		case FORMAT_R10G10B10A2_UINT:
			return DXGI_FORMAT_R10G10B10A2_UINT;
			break;
		case FORMAT_R11G11B10_FLOAT:
			return DXGI_FORMAT_R11G11B10_FLOAT;
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
		case FORMAT_BC1_UNORM:
			return DXGI_FORMAT_BC1_UNORM;
			break;
		case FORMAT_BC1_UNORM_SRGB:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
			break;
		case FORMAT_BC2_UNORM:
			return DXGI_FORMAT_BC2_UNORM;
			break;
		case FORMAT_BC2_UNORM_SRGB:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
			break;
		case FORMAT_BC3_UNORM:
			return DXGI_FORMAT_BC3_UNORM;
			break;
		case FORMAT_BC3_UNORM_SRGB:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
			break;
		case FORMAT_BC4_UNORM:
			return DXGI_FORMAT_BC4_UNORM;
			break;
		case FORMAT_BC4_SNORM:
			return DXGI_FORMAT_BC4_SNORM;
			break;
		case FORMAT_BC5_UNORM:
			return DXGI_FORMAT_BC5_UNORM;
			break;
		case FORMAT_BC5_SNORM:
			return DXGI_FORMAT_BC5_SNORM;
			break;
		case FORMAT_B8G8R8A8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
			break;
		case FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
			break;
		case FORMAT_BC6H_UF16:
			return DXGI_FORMAT_BC6H_UF16;
			break;
		case FORMAT_BC6H_SF16:
			return DXGI_FORMAT_BC6H_SF16;
			break;
		case FORMAT_BC7_UNORM:
			return DXGI_FORMAT_BC7_UNORM;
			break;
		case FORMAT_BC7_UNORM_SRGB:
			return DXGI_FORMAT_BC7_UNORM_SRGB;
			break;
		}
		return DXGI_FORMAT_UNKNOWN;
	}
	inline D3D12_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
	{
		D3D12_SUBRESOURCE_DATA data;
		data.pData = pInitialData.pSysMem;
		data.RowPitch = pInitialData.SysMemPitch;
		data.SlicePitch = pInitialData.SysMemSlicePitch;

		return data;
	}
	constexpr D3D12_RESOURCE_STATES _ConvertImageLayout(IMAGE_LAYOUT value)
	{
		switch (value)
		{
		case wiGraphics::IMAGE_LAYOUT_UNDEFINED:
			return D3D12_RESOURCE_STATE_COMMON;
		case wiGraphics::IMAGE_LAYOUT_RENDERTARGET:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case wiGraphics::IMAGE_LAYOUT_DEPTHSTENCIL:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case wiGraphics::IMAGE_LAYOUT_DEPTHSTENCIL_READONLY:
			return D3D12_RESOURCE_STATE_DEPTH_READ;
		case wiGraphics::IMAGE_LAYOUT_SHADER_RESOURCE:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case wiGraphics::IMAGE_LAYOUT_UNORDERED_ACCESS:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case wiGraphics::IMAGE_LAYOUT_COPY_SRC:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case wiGraphics::IMAGE_LAYOUT_COPY_DST:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case wiGraphics::IMAGE_LAYOUT_SHADING_RATE_SOURCE:
			return D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
		}

		return D3D12_RESOURCE_STATE_COMMON;
	}
	constexpr D3D12_RESOURCE_STATES _ConvertBufferState(BUFFER_STATE value)
	{
		switch (value)
		{
		case wiGraphics::BUFFER_STATE_UNDEFINED:
			return D3D12_RESOURCE_STATE_COMMON;
		case wiGraphics::BUFFER_STATE_VERTEX_BUFFER:
			return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case wiGraphics::BUFFER_STATE_INDEX_BUFFER:
			return D3D12_RESOURCE_STATE_INDEX_BUFFER;
		case wiGraphics::BUFFER_STATE_CONSTANT_BUFFER:
			return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case wiGraphics::BUFFER_STATE_INDIRECT_ARGUMENT:
			return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		case wiGraphics::BUFFER_STATE_SHADER_RESOURCE:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case wiGraphics::BUFFER_STATE_UNORDERED_ACCESS:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case wiGraphics::BUFFER_STATE_COPY_SRC:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case wiGraphics::BUFFER_STATE_COPY_DST:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case wiGraphics::BUFFER_STATE_RAYTRACING_ACCELERATION_STRUCTURE:
			return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		}

		return D3D12_RESOURCE_STATE_COMMON;
	}
	constexpr D3D12_SHADER_VISIBILITY _ConvertShaderVisibility(SHADERSTAGE value)
	{
		switch (value)
		{
		case MS:
			return D3D12_SHADER_VISIBILITY_MESH;
		case AS:
			return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
		case VS:
			return D3D12_SHADER_VISIBILITY_VERTEX;
		case HS:
			return D3D12_SHADER_VISIBILITY_HULL;
		case DS:
			return D3D12_SHADER_VISIBILITY_DOMAIN;
		case GS:
			return D3D12_SHADER_VISIBILITY_GEOMETRY;
		case PS:
			return D3D12_SHADER_VISIBILITY_PIXEL;
		default:
			return D3D12_SHADER_VISIBILITY_ALL;
		}
		return D3D12_SHADER_VISIBILITY_ALL;
	}
	constexpr D3D12_SHADING_RATE _ConvertShadingRate(SHADING_RATE value)
	{
		switch (value)
		{
		case wiGraphics::SHADING_RATE_1X1:
			return D3D12_SHADING_RATE_1X1;
		case wiGraphics::SHADING_RATE_1X2:
			return D3D12_SHADING_RATE_1X2;
		case wiGraphics::SHADING_RATE_2X1:
			return D3D12_SHADING_RATE_2X1;
		case wiGraphics::SHADING_RATE_2X2:
			return D3D12_SHADING_RATE_2X2;
		case wiGraphics::SHADING_RATE_2X4:
			return D3D12_SHADING_RATE_2X4;
		case wiGraphics::SHADING_RATE_4X2:
			return D3D12_SHADING_RATE_4X2;
		case wiGraphics::SHADING_RATE_4X4:
			return D3D12_SHADING_RATE_4X4;
		default:
			return D3D12_SHADING_RATE_1X1;
		}
		return D3D12_SHADING_RATE_1X1;
	}
	constexpr D3D12_STATIC_SAMPLER_DESC _ConvertStaticSampler(const StaticSampler& x)
	{
		D3D12_STATIC_SAMPLER_DESC desc = {};
		desc.ShaderRegister = x.slot;
		desc.Filter = _ConvertFilter(x.sampler.desc.Filter);
		desc.AddressU = _ConvertTextureAddressMode(x.sampler.desc.AddressU);
		desc.AddressV = _ConvertTextureAddressMode(x.sampler.desc.AddressV);
		desc.AddressW = _ConvertTextureAddressMode(x.sampler.desc.AddressW);
		desc.MipLODBias = x.sampler.desc.MipLODBias;
		desc.MaxAnisotropy = x.sampler.desc.MaxAnisotropy;
		desc.ComparisonFunc = _ConvertComparisonFunc(x.sampler.desc.ComparisonFunc);
		desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		desc.MinLOD = x.sampler.desc.MinLOD;
		desc.MaxLOD = x.sampler.desc.MaxLOD;
		desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		return desc;
	}

	// Native -> Engine converters

	constexpr FORMAT _ConvertFormat_Inv(DXGI_FORMAT value)
	{
		switch (value)
		{
		case DXGI_FORMAT_UNKNOWN:
			return FORMAT_UNKNOWN;
			break;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return FORMAT_R32G32B32A32_FLOAT;
			break;
		case DXGI_FORMAT_R32G32B32A32_UINT:
			return FORMAT_R32G32B32A32_UINT;
			break;
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return FORMAT_R32G32B32A32_SINT;
			break;
		case DXGI_FORMAT_R32G32B32_FLOAT:
			return FORMAT_R32G32B32_FLOAT;
			break;
		case DXGI_FORMAT_R32G32B32_UINT:
			return FORMAT_R32G32B32_UINT;
			break;
		case DXGI_FORMAT_R32G32B32_SINT:
			return FORMAT_R32G32B32_SINT;
			break;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return FORMAT_R16G16B16A16_FLOAT;
			break;
		case DXGI_FORMAT_R16G16B16A16_UNORM:
			return FORMAT_R16G16B16A16_UNORM;
			break;
		case DXGI_FORMAT_R16G16B16A16_UINT:
			return FORMAT_R16G16B16A16_UINT;
			break;
		case DXGI_FORMAT_R16G16B16A16_SNORM:
			return FORMAT_R16G16B16A16_SNORM;
			break;
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return FORMAT_R16G16B16A16_SINT;
			break;
		case DXGI_FORMAT_R32G32_FLOAT:
			return FORMAT_R32G32_FLOAT;
			break;
		case DXGI_FORMAT_R32G32_UINT:
			return FORMAT_R32G32_UINT;
			break;
		case DXGI_FORMAT_R32G32_SINT:
			return FORMAT_R32G32_SINT;
			break;
		case DXGI_FORMAT_R32G8X24_TYPELESS:
			return FORMAT_R32G8X24_TYPELESS;
			break;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			return FORMAT_D32_FLOAT_S8X24_UINT;
			break;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return FORMAT_R10G10B10A2_UNORM;
			break;
		case DXGI_FORMAT_R10G10B10A2_UINT:
			return FORMAT_R10G10B10A2_UINT;
			break;
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return FORMAT_R11G11B10_FLOAT;
			break;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return FORMAT_R8G8B8A8_UNORM;
			break;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return FORMAT_R8G8B8A8_UNORM_SRGB;
			break;
		case DXGI_FORMAT_R8G8B8A8_UINT:
			return FORMAT_R8G8B8A8_UINT;
			break;
		case DXGI_FORMAT_R8G8B8A8_SNORM:
			return FORMAT_R8G8B8A8_SNORM;
			break;
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return FORMAT_R8G8B8A8_SINT;
			break;
		case DXGI_FORMAT_R16G16_FLOAT:
			return FORMAT_R16G16_FLOAT;
			break;
		case DXGI_FORMAT_R16G16_UNORM:
			return FORMAT_R16G16_UNORM;
			break;
		case DXGI_FORMAT_R16G16_UINT:
			return FORMAT_R16G16_UINT;
			break;
		case DXGI_FORMAT_R16G16_SNORM:
			return FORMAT_R16G16_SNORM;
			break;
		case DXGI_FORMAT_R16G16_SINT:
			return FORMAT_R16G16_SINT;
			break;
		case DXGI_FORMAT_R32_TYPELESS:
			return FORMAT_R32_TYPELESS;
			break;
		case DXGI_FORMAT_D32_FLOAT:
			return FORMAT_D32_FLOAT;
			break;
		case DXGI_FORMAT_R32_FLOAT:
			return FORMAT_R32_FLOAT;
			break;
		case DXGI_FORMAT_R32_UINT:
			return FORMAT_R32_UINT;
			break;
		case DXGI_FORMAT_R32_SINT:
			return FORMAT_R32_SINT;
			break;
		case DXGI_FORMAT_R8G8_UNORM:
			return FORMAT_R8G8_UNORM;
			break;
		case DXGI_FORMAT_R8G8_UINT:
			return FORMAT_R8G8_UINT;
			break;
		case DXGI_FORMAT_R8G8_SNORM:
			return FORMAT_R8G8_SNORM;
			break;
		case DXGI_FORMAT_R8G8_SINT:
			return FORMAT_R8G8_SINT;
			break;
		case DXGI_FORMAT_R16_TYPELESS:
			return FORMAT_R16_TYPELESS;
			break;
		case DXGI_FORMAT_R16_FLOAT:
			return FORMAT_R16_FLOAT;
			break;
		case DXGI_FORMAT_D16_UNORM:
			return FORMAT_D16_UNORM;
			break;
		case DXGI_FORMAT_R16_UNORM:
			return FORMAT_R16_UNORM;
			break;
		case DXGI_FORMAT_R16_UINT:
			return FORMAT_R16_UINT;
			break;
		case DXGI_FORMAT_R16_SNORM:
			return FORMAT_R16_SNORM;
			break;
		case DXGI_FORMAT_R16_SINT:
			return FORMAT_R16_SINT;
			break;
		case DXGI_FORMAT_R8_UNORM:
			return FORMAT_R8_UNORM;
			break;
		case DXGI_FORMAT_R8_UINT:
			return FORMAT_R8_UINT;
			break;
		case DXGI_FORMAT_R8_SNORM:
			return FORMAT_R8_SNORM;
			break;
		case DXGI_FORMAT_R8_SINT:
			return FORMAT_R8_SINT;
			break;
		case DXGI_FORMAT_BC1_UNORM:
			return FORMAT_BC1_UNORM;
			break;
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return FORMAT_BC1_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC2_UNORM:
			return FORMAT_BC2_UNORM;
			break;
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			return FORMAT_BC2_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC3_UNORM:
			return FORMAT_BC3_UNORM;
			break;
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return FORMAT_BC3_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC4_UNORM:
			return FORMAT_BC4_UNORM;
			break;
		case DXGI_FORMAT_BC4_SNORM:
			return FORMAT_BC4_SNORM;
			break;
		case DXGI_FORMAT_BC5_UNORM:
			return FORMAT_BC5_UNORM;
			break;
		case DXGI_FORMAT_BC5_SNORM:
			return FORMAT_BC5_SNORM;
			break;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return FORMAT_B8G8R8A8_UNORM;
			break;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return FORMAT_B8G8R8A8_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC6H_UF16:
			return FORMAT_BC6H_UF16;
			break;
		case DXGI_FORMAT_BC6H_SF16:
			return FORMAT_BC6H_SF16;
			break;
		case DXGI_FORMAT_BC7_UNORM:
			return FORMAT_BC7_UNORM;
			break;
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return FORMAT_BC7_UNORM_SRGB;
			break;
		}
		return FORMAT_UNKNOWN;
	}

	constexpr TextureDesc _ConvertTextureDesc_Inv(const D3D12_RESOURCE_DESC& desc)
	{
		TextureDesc retVal;
		
		switch (desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			retVal.type = TextureDesc::TEXTURE_1D;
			retVal.ArraySize = desc.DepthOrArraySize;
			break;
		default:
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			retVal.type = TextureDesc::TEXTURE_2D;
			retVal.ArraySize = desc.DepthOrArraySize;
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			retVal.type = TextureDesc::TEXTURE_3D;
			retVal.Depth = desc.DepthOrArraySize;
			break;
		}
		retVal.Format = _ConvertFormat_Inv(desc.Format);
		retVal.Width = (uint32_t)desc.Width;
		retVal.Height = desc.Height;
		retVal.MipLevels = desc.MipLevels;

		return retVal;
	}


	// Local Helpers:

	inline size_t Align(size_t uLocation, size_t uAlign)
	{
		if ((0 == uAlign) || (uAlign & (uAlign - 1)))
		{
			assert(0);
		}

		return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
	}


	struct SingleDescriptor
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
		D3D12_DESCRIPTOR_HEAP_TYPE type = {};
		union
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
			D3D12_SHADER_RESOURCE_VIEW_DESC srv;
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav;
			D3D12_SAMPLER_DESC sam;
			D3D12_RENDER_TARGET_VIEW_DESC rtv;
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv;
		};
		bool IsValid() const { return handle.ptr != 0; }
		void init(GraphicsDevice_DX12* device, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbv)
		{
			this->cbv = cbv;
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			handle = allocationhandler->descriptors_res.allocate();
			allocationhandler->device->CreateConstantBufferView(&cbv, handle);
		}
		void init(GraphicsDevice_DX12* device, const D3D12_SHADER_RESOURCE_VIEW_DESC& srv, ID3D12Resource* res)
		{
			this->srv = srv;
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			handle = allocationhandler->descriptors_res.allocate();
			allocationhandler->device->CreateShaderResourceView(res, &srv, handle);
		}
		void init(GraphicsDevice_DX12* device, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uav, ID3D12Resource* res)
		{
			this->uav = uav;
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			handle = allocationhandler->descriptors_res.allocate();
			allocationhandler->device->CreateUnorderedAccessView(res, nullptr, &uav, handle);
		}
		void init(GraphicsDevice_DX12* device, const D3D12_SAMPLER_DESC& sam)
		{
			this->sam = sam;
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			handle = allocationhandler->descriptors_sam.allocate();
			allocationhandler->device->CreateSampler(&sam, handle);
		}
		void init(GraphicsDevice_DX12* device, const D3D12_RENDER_TARGET_VIEW_DESC& rtv, ID3D12Resource* res)
		{
			this->rtv = rtv;
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			handle = allocationhandler->descriptors_rtv.allocate();
			allocationhandler->device->CreateRenderTargetView(res, &rtv, handle);
		}
		void init(GraphicsDevice_DX12* device, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsv, ID3D12Resource* res)
		{
			this->dsv = dsv;
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			handle = allocationhandler->descriptors_dsv.allocate();
			allocationhandler->device->CreateDepthStencilView(res, &dsv, handle);
		}
		void destroy()
		{
			if (IsValid())
			{
				switch (type)
				{
				case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
					allocationhandler->descriptors_res.free(handle);
					break;
				case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
					allocationhandler->descriptors_sam.free(handle);
					break;
				case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
					allocationhandler->descriptors_rtv.free(handle);
					break;
				case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
					allocationhandler->descriptors_dsv.free(handle);
					break;
				case D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES:
				default:
					assert(0);
					break;
				}
			}
		}
	};

	struct Resource_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		D3D12MA::Allocation* allocation = nullptr;
		ComPtr<ID3D12Resource> resource;
		SingleDescriptor cbv;
		SingleDescriptor srv;
		SingleDescriptor uav;
		std::vector<SingleDescriptor> subresources_srv;
		std::vector<SingleDescriptor> subresources_uav;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;

		D3D12_GPU_VIRTUAL_ADDRESS gpu_address = 0;

		GraphicsDevice::GPUAllocation dynamic[COMMANDLIST_COUNT];

		uint64_t cbv_mask_frame[COMMANDLIST_COUNT] = {};
		uint32_t cbv_mask_gfx[COMMANDLIST_COUNT] = {};
		uint32_t cbv_mask_compute[COMMANDLIST_COUNT] = {};

		virtual ~Resource_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (allocation) allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
			if (resource) allocationhandler->destroyer_resources.push_back(std::make_pair(resource, framecount));
			allocationhandler->destroylocker.unlock();

			cbv.destroy();
			srv.destroy();
			uav.destroy();
			for (auto& x : subresources_srv)
			{
				x.destroy();
			}
			for (auto& x : subresources_uav)
			{
				x.destroy();
			}
		}
	};
	struct Texture_DX12 : public Resource_DX12
	{
		SingleDescriptor rtv = {};
		SingleDescriptor dsv = {};
		std::vector<SingleDescriptor> subresources_rtv;
		std::vector<SingleDescriptor> subresources_dsv;

		~Texture_DX12() override
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroylocker.unlock();

			rtv.destroy();
			dsv.destroy();
			for (auto& x : subresources_rtv)
			{
				x.destroy();
			}
			for (auto& x : subresources_dsv)
			{
				x.destroy();
			}
		}
	};
	struct Sampler_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		SingleDescriptor descriptor;

		~Sampler_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroylocker.unlock();

			descriptor.destroy();
		}
	};
	struct Query_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		GPU_QUERY_TYPE query_type = GPU_QUERY_TYPE_INVALID;
		GraphicsDevice_DX12::AllocationHandler::QueryAllocator::Query query;

		~Query_DX12()
		{
			if (query.index != ~0) 
			{
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				switch (query_type)
				{
				case wiGraphics::GPU_QUERY_TYPE_OCCLUSION:
				case wiGraphics::GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
					allocationhandler->destroyer_queries_occlusion.push_back(std::make_pair(query, framecount));
					break;
				case wiGraphics::GPU_QUERY_TYPE_TIMESTAMP:
					allocationhandler->destroyer_queries_timestamp.push_back(std::make_pair(query, framecount));
					break;
				}
				allocationhandler->destroylocker.unlock();
			}
		}
	};
	struct PipelineState_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		ComPtr<ID3D12PipelineState> resource;
		ComPtr<ID3D12RootSignature> rootSignature;

		std::vector<D3D12_ROOT_DESCRIPTOR1> root_cbvs;
		std::vector<D3D12_DESCRIPTOR_RANGE1> resources;
		std::vector<D3D12_DESCRIPTOR_RANGE1> samplers;

		std::vector<RESOURCEBINDING> resource_bindings;

		uint32_t bindpoint_res = 0;
		uint32_t bindpoint_sam = 0;

		size_t root_binding_hash = 0;
		size_t resource_binding_hash = 0;
		size_t sampler_binding_hash = 0;

		std::vector<D3D12_STATIC_SAMPLER_DESC> staticsamplers;

		std::vector<uint8_t> shadercode;

		~PipelineState_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_pipelines.push_back(std::make_pair(resource, framecount));
			if (rootSignature) allocationhandler->destroyer_rootSignatures.push_back(std::make_pair(rootSignature, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct BVH_DX12 : public Resource_DX12
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS desc = {};
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
		GPUBuffer scratch;
	};
	struct RTPipelineState_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		ComPtr<ID3D12StateObject> resource;

		std::vector<std::wstring> export_strings;
		std::vector<D3D12_EXPORT_DESC> exports;
		std::vector<D3D12_DXIL_LIBRARY_DESC> library_descs;
		std::vector<std::wstring> group_strings;
		std::vector<D3D12_HIT_GROUP_DESC> hitgroup_descs;

		~RTPipelineState_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_stateobjects.push_back(std::make_pair(resource, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct RenderPass_DX12
	{
		D3D12_RESOURCE_BARRIER barrierdescs_begin[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		uint32_t num_barriers_begin = 0;
		D3D12_RESOURCE_BARRIER barrierdescs_end[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		uint32_t num_barriers_end = 0;

		D3D12_RENDER_PASS_FLAGS flags = D3D12_RENDER_PASS_FLAG_NONE;
		uint32_t rt_count = 0;
		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTVs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DSV = {};
		const Texture* shading_rate_image = nullptr;

		// Due to a API bug, this resolve_subresources array must be kept alive between BeginRenderpass() and EndRenderpass()!
		D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS resolve_subresources[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
	};
	struct DescriptorTable_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;

		struct Heap
		{
			ComPtr<ID3D12DescriptorHeap> heap;
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			D3D12_CPU_DESCRIPTOR_HANDLE address = {};
			std::vector<D3D12_DESCRIPTOR_RANGE1> ranges;
			std::vector<size_t> write_remap;
		};
		Heap sampler_heap;
		Heap resource_heap;
		std::vector<D3D12_STATIC_SAMPLER_DESC> staticsamplers;

		~DescriptorTable_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (sampler_heap.heap) allocationhandler->destroyer_descriptorHeaps.push_back(std::make_pair(sampler_heap.heap, framecount));
			if (resource_heap.heap) allocationhandler->destroyer_descriptorHeaps.push_back(std::make_pair(resource_heap.heap, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct RootSignature_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		ComPtr<ID3D12RootSignature> resource;
		std::vector<D3D12_ROOT_PARAMETER1> params;

		std::vector<uint32_t> table_bind_point_remap;
		uint32_t root_constant_bind_remap = 0;

		struct RootRemap
		{
			uint32_t space = 0;
			uint32_t rangeIndex = 0;
		};
		std::vector<RootRemap> root_remap;

		~RootSignature_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_rootSignatures.push_back(std::make_pair(resource, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};

	Resource_DX12* to_internal(const GPUResource* param)
	{
		return static_cast<Resource_DX12*>(param->internal_state.get());
	}
	Resource_DX12* to_internal(const GPUBuffer* param)
	{
		return static_cast<Resource_DX12*>(param->internal_state.get());
	}
	Texture_DX12* to_internal(const Texture* param)
	{
		return static_cast<Texture_DX12*>(param->internal_state.get());
	}
	Sampler_DX12* to_internal(const Sampler* param)
	{
		return static_cast<Sampler_DX12*>(param->internal_state.get());
	}
	Query_DX12* to_internal(const GPUQuery* param)
	{
		return static_cast<Query_DX12*>(param->internal_state.get());
	}
	PipelineState_DX12* to_internal(const Shader* param)
	{
		return static_cast<PipelineState_DX12*>(param->internal_state.get());
	}
	PipelineState_DX12* to_internal(const PipelineState* param)
	{
		return static_cast<PipelineState_DX12*>(param->internal_state.get());
	}
	BVH_DX12* to_internal(const RaytracingAccelerationStructure* param)
	{
		return static_cast<BVH_DX12*>(param->internal_state.get());
	}
	RTPipelineState_DX12* to_internal(const RaytracingPipelineState* param)
	{
		return static_cast<RTPipelineState_DX12*>(param->internal_state.get());
	}
	RenderPass_DX12* to_internal(const RenderPass* param)
	{
		return static_cast<RenderPass_DX12*>(param->internal_state.get());
	}
	DescriptorTable_DX12* to_internal(const DescriptorTable* param)
	{
		return static_cast<DescriptorTable_DX12*>(param->internal_state.get());
	}
	RootSignature_DX12* to_internal(const RootSignature* param)
	{
		return static_cast<RootSignature_DX12*>(param->internal_state.get());
	}
}
using namespace DX12_Internal;

	// Allocators:

	void GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::init(GraphicsDevice_DX12* device, size_t size)
	{
		this->device = device;
		auto internal_state = std::make_shared<Resource_DX12>();
		internal_state->allocationhandler = device->allocationhandler;
		buffer.internal_state = internal_state;

		HRESULT hr;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		CD3DX12_RESOURCE_DESC resdesc = CD3DX12_RESOURCE_DESC::Buffer(size);

		hr = device->allocationhandler->allocator->CreateResource(
			&allocationDesc,
			&resdesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			&internal_state->allocation,
			IID_PPV_ARGS(&internal_state->resource)
		);
		assert(SUCCEEDED(hr));

		void* pData;
		CD3DX12_RANGE readRange(0, 0);
		internal_state->resource->Map(0, &readRange, &pData);
		dataCur = dataBegin = reinterpret_cast<uint8_t*>(pData);
		dataEnd = dataBegin + size;

		// Because the "buffer" is created by hand in this, fill the desc to indicate how it can be used:
		buffer.type = GPUResource::GPU_RESOURCE_TYPE::BUFFER;
		buffer.desc.ByteWidth = (uint32_t)((size_t)dataEnd - (size_t)dataBegin);
		buffer.desc.Usage = USAGE_DYNAMIC;
		buffer.desc.BindFlags = BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
		buffer.desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

		internal_state->gpu_address = internal_state->resource->GetGPUVirtualAddress();

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		srv_desc.Buffer.NumElements = buffer.desc.ByteWidth / sizeof(uint32_t);
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		internal_state->srv.init(device, srv_desc, internal_state->resource.Get());
	}
	uint8_t* GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::allocate(size_t dataSize, size_t alignment)
	{
		dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));

		if (dataCur + dataSize > dataEnd)
		{
			init(device, ((size_t)dataEnd + dataSize - (size_t)dataBegin) * 2);
		}

		uint8_t* retVal = dataCur;

		dataCur += dataSize;

		return retVal;
	}
	void GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::clear()
	{
		dataCur = dataBegin;
	}
	uint64_t GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::calculateOffset(uint8_t* address)
	{
		assert(address >= dataBegin && address < dataEnd);
		return static_cast<uint64_t>(address - dataBegin);
	}

	void GraphicsDevice_DX12::DescriptorBinder::init(GraphicsDevice_DX12* device)
	{
		this->device = device;

		// Reset state to empty:
		reset();
	}
	void GraphicsDevice_DX12::DescriptorBinder::reset()
	{
		dirty_res = true;
		dirty_sam = true;
		ringOffset_res = 0;
		ringOffset_sam = 0;

		memset(CBV, 0, sizeof(CBV));
		memset(SRV, 0, sizeof(SRV));
		memset(SRV_index, -1, sizeof(SRV_index));
		memset(UAV, 0, sizeof(UAV));
		memset(UAV_index, -1, sizeof(UAV_index));
		memset(SAM, 0, sizeof(SAM));
	}
	void GraphicsDevice_DX12::DescriptorBinder::request_heaps(uint32_t resources, uint32_t samplers, CommandList cmd)
	{
		// Remarks:
		//	This is allocating from the global shader visible descriptor heaps in a simple incrementing
		//	lockless ring buffer fashion.
		//	In this lockless method, a descriptor array that is to be allocated might not fit without
		//	completely wrapping the beginning of the allocation.
		//	But completely wrapping after the fact we discovered that the array couldn't fit,
		//	it wouldn't be thread safe any more without introducing locks
		//	For that reason, we are reserving an excess amount of descriptors at the end which can't be normally
		//	allocated, but any out of bounds descriptors can still be safely written into it
		//
		//	This method wastes a number of descriptors essentially at the end of the heap, but it is simple
		//	and safe to implement
		//
		//	The excess amount is essentially equal to the maximum number of descriptors that can be allocated at once.

		if (resources > 0)
		{
			// The reservation is the maximum amount of descriptors that can be allocated once
			//	It can be increased if needed
			const uint32_t wrap_reservation = 100000;
			const uint32_t wrap_effective_size = device->descriptorheap_res.heapDesc.NumDescriptors - wrap_reservation;
			assert(wrap_reservation > resources); // for correct lockless wrap behaviour

			const uint64_t offset = device->descriptorheap_res.allocationOffset.fetch_add(resources);
			const uint64_t wrapped_offset = offset % wrap_effective_size;
			ringOffset_res = (uint32_t)wrapped_offset;
			const uint64_t wrapped_offset_end = wrapped_offset + resources;

			uint64_t gpu_offset = device->descriptorheap_res.cached_completedValue;
			uint64_t wrapped_gpu_offset = gpu_offset % wrap_effective_size;
			while (wrapped_offset < wrapped_gpu_offset && wrapped_offset_end > wrapped_gpu_offset)
			{
				assert(device->descriptorheap_res.fenceValue > wrapped_offset_end); // simply not enough space, even with GPU drain
				gpu_offset = device->descriptorheap_res.fence->GetCompletedValue();
				wrapped_gpu_offset = gpu_offset % wrap_effective_size;
			}
		}

		if (samplers > 0)
		{
			// The reservation is the maximum amount of descriptors that can be allocated once
			//	It can be increased if needed
			const uint32_t wrap_reservation = 16;
			const uint32_t wrap_effective_size = device->descriptorheap_sam.heapDesc.NumDescriptors - wrap_reservation;
			assert(wrap_reservation > samplers); // for correct lockless wrap behaviour

			const uint64_t offset = device->descriptorheap_sam.allocationOffset.fetch_add(samplers);
			const uint64_t wrapped_offset = offset % wrap_effective_size;
			ringOffset_sam = (uint32_t)wrapped_offset;
			const uint64_t wrapped_offset_end = wrapped_offset + samplers;

			uint64_t gpu_offset = device->descriptorheap_sam.cached_completedValue;
			uint64_t wrapped_gpu_offset = gpu_offset % wrap_effective_size;
			while (wrapped_offset < wrapped_gpu_offset && wrapped_offset_end > wrapped_gpu_offset)
			{
				assert(device->descriptorheap_sam.fenceValue > wrapped_offset_end); // simply not enough space, even with GPU drain
				gpu_offset = device->descriptorheap_sam.fence->GetCompletedValue();
				wrapped_gpu_offset = gpu_offset % wrap_effective_size;
			}
		}
	}
	void GraphicsDevice_DX12::DescriptorBinder::validate(bool graphics, CommandList cmd)
	{
		auto pso_internal = graphics ? to_internal(device->active_pso[cmd]) : to_internal(device->active_cs[cmd]);

		// Bind root descriptors:
		if ((dirty_root_cbvs_gfx != 0 && graphics) || (dirty_root_cbvs_compute != 0 && !graphics))
		{
			uint32_t root_param = 0;
			for (auto& x : pso_internal->root_cbvs)
			{
				bool dirty;
				if (graphics)
				{
					dirty = dirty_root_cbvs_gfx & (1 << x.ShaderRegister);
				}
				else
				{
					dirty = dirty_root_cbvs_compute & (1 << x.ShaderRegister);
				}
				if (!dirty)
				{
					root_param++;
					continue;
				}

				const GPUBuffer* buffer = CBV[x.ShaderRegister];

				D3D12_GPU_VIRTUAL_ADDRESS address;

				if (buffer == nullptr || !buffer->IsValid())
				{
					address = 0;
				}
				else
				{
					auto internal_state = to_internal(buffer);

					if (buffer->desc.Usage == USAGE_DYNAMIC)
					{
						GraphicsDevice::GPUAllocation allocation = internal_state->dynamic[cmd];
						address = to_internal(allocation.buffer)->gpu_address;
						address += (D3D12_GPU_VIRTUAL_ADDRESS)allocation.offset;
					}
					else
					{
						address = internal_state->cbv.cbv.BufferLocation;
					}
				}

				if (graphics)
				{
					device->GetDirectCommandList(cmd)->SetGraphicsRootConstantBufferView(root_param, address);
				}
				else
				{
					device->GetDirectCommandList(cmd)->SetComputeRootConstantBufferView(root_param, address);
				}
				root_param++;
			}

			if (graphics)
			{
				dirty_root_cbvs_gfx = 0;
			}
			else
			{
				dirty_root_cbvs_compute = 0;
			}
		}

		uint32_t request_res = dirty_res ? (uint32_t)pso_internal->resources.size() : 0;
		uint32_t request_sam = dirty_sam ? (uint32_t)pso_internal->samplers.size() : 0;
		request_heaps(request_res, request_sam, cmd);

		// Resources:
		if (!pso_internal->resources.empty() && dirty_res)
		{
			dirty_res = false;
			auto& heap = device->descriptorheap_res;
			D3D12_GPU_DESCRIPTOR_HANDLE binding_table = heap.start_gpu;
			binding_table.ptr += (UINT64)ringOffset_res * (UINT64)device->resource_descriptor_size;

			int i = 0;
			for (auto& x : pso_internal->resources)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE dst = heap.start_cpu;
				uint32_t ringOffset = ringOffset_res++;
				dst.ptr += ringOffset * device->resource_descriptor_size;

				RESOURCEBINDING binding = pso_internal->resource_bindings[i++];

				switch (x.RangeType)
				{
				default:
				case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
				{
					const GPUResource* resource = SRV[x.BaseShaderRegister];
					const int subresource = SRV_index[x.BaseShaderRegister];
					if (resource == nullptr || !resource->IsValid())
					{
						switch (binding)
						{
						case RAWBUFFER:
						case STRUCTUREDBUFFER:
						case TYPEDBUFFER:
							device->device->CopyDescriptorsSimple(1, dst, device->nullSRV_buffer, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case TEXTURE1D:
							device->device->CopyDescriptorsSimple(1, dst, device->nullSRV_texture1d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case TEXTURE1DARRAY:
							device->device->CopyDescriptorsSimple(1, dst, device->nullSRV_texture1darray, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case TEXTURE2D:
							device->device->CopyDescriptorsSimple(1, dst, device->nullSRV_texture2d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case TEXTURE2DARRAY:
							device->device->CopyDescriptorsSimple(1, dst, device->nullSRV_texture2darray, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case TEXTURECUBE:
							device->device->CopyDescriptorsSimple(1, dst, device->nullSRV_texturecube, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case TEXTURECUBEARRAY:
							device->device->CopyDescriptorsSimple(1, dst, device->nullSRV_texturecubearray, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case TEXTURE3D:
							device->device->CopyDescriptorsSimple(1, dst, device->nullSRV_texture3d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case ACCELERATIONSTRUCTURE:
							device->device->CopyDescriptorsSimple(1, dst, device->nullSRV_accelerationstructure, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						default:
							assert(0);
							break;
						}
					}
					else
					{
						auto internal_state = to_internal(resource);

						if (resource->IsAccelerationStructure())
						{
							device->device->CopyDescriptorsSimple(1, dst, internal_state->srv.handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
						}
						else
						{
							if (subresource < 0)
							{
								device->device->CopyDescriptorsSimple(1, dst, internal_state->srv.handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							}
							else
							{
								device->device->CopyDescriptorsSimple(1, dst, internal_state->subresources_srv[subresource].handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							}
						}
					}
				}
				break;

				case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
				{
					const GPUResource* resource = UAV[x.BaseShaderRegister];
					const int subresource = UAV_index[x.BaseShaderRegister];
					if (resource == nullptr || !resource->IsValid())
					{
						switch (binding)
						{
						case RWRAWBUFFER:
						case RWSTRUCTUREDBUFFER:
						case RWTYPEDBUFFER:
							device->device->CopyDescriptorsSimple(1, dst, device->nullUAV_buffer, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case RWTEXTURE1D:
							device->device->CopyDescriptorsSimple(1, dst, device->nullUAV_texture1d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case RWTEXTURE1DARRAY:
							device->device->CopyDescriptorsSimple(1, dst, device->nullUAV_texture1darray, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case RWTEXTURE2D:
							device->device->CopyDescriptorsSimple(1, dst, device->nullUAV_texture2d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case RWTEXTURE2DARRAY:
							device->device->CopyDescriptorsSimple(1, dst, device->nullUAV_texture2darray, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						case RWTEXTURE3D:
							device->device->CopyDescriptorsSimple(1, dst, device->nullUAV_texture3d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							break;
						default:
							assert(0);
							break;
						}
					}
					else
					{
						auto internal_state = to_internal(resource);

						if (subresource < 0)
						{
							device->device->CopyDescriptorsSimple(1, dst, internal_state->uav.handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
						}
						else
						{
							device->device->CopyDescriptorsSimple(1, dst, internal_state->subresources_uav[subresource].handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
						}
					}
				}
				break;

				case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
				{
					const GPUBuffer* buffer = CBV[x.BaseShaderRegister];

					if (buffer == nullptr || !buffer->IsValid())
					{
						device->device->CopyDescriptorsSimple(1, dst, device->nullCBV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					}
					else
					{
						auto internal_state = to_internal(buffer);

						if (buffer->desc.Usage == USAGE_DYNAMIC)
						{
							GraphicsDevice::GPUAllocation allocation = internal_state->dynamic[cmd];
							D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
							cbv.BufferLocation = to_internal(allocation.buffer)->gpu_address;
							cbv.BufferLocation += (D3D12_GPU_VIRTUAL_ADDRESS)allocation.offset;
							cbv.SizeInBytes = (uint32_t)Align((size_t)buffer->desc.ByteWidth, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

							device->device->CreateConstantBufferView(&cbv, dst);
						}
						else
						{
							device->device->CopyDescriptorsSimple(1, dst, internal_state->cbv.handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
						}
					}
				}
				break;
				}
			}

			if (graphics)
			{
				device->GetDirectCommandList(cmd)->SetGraphicsRootDescriptorTable(pso_internal->bindpoint_res, binding_table);
			}
			else
			{
				device->GetDirectCommandList(cmd)->SetComputeRootDescriptorTable(pso_internal->bindpoint_res, binding_table);
			}
		}

		// Samplers:
		if (!pso_internal->samplers.empty() && dirty_sam)
		{
			dirty_sam = false;
			auto& heap = device->descriptorheap_sam;
			D3D12_GPU_DESCRIPTOR_HANDLE binding_table = heap.start_gpu;
			binding_table.ptr += (UINT64)ringOffset_sam * (UINT64)device->sampler_descriptor_size;

			for (auto& x : pso_internal->samplers)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE dst = heap.start_cpu;
				uint32_t ringOffset = ringOffset_sam++;
				dst.ptr += ringOffset * device->sampler_descriptor_size;

				const Sampler* sampler = SAM[x.BaseShaderRegister];
				if (sampler == nullptr || !sampler->IsValid())
				{
					device->device->CopyDescriptorsSimple(1, dst, device->nullSAM, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
				}
				else
				{
					auto internal_state = to_internal(sampler);
					device->device->CopyDescriptorsSimple(1, dst, internal_state->descriptor.handle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
				}
			}

			if (graphics)
			{
				device->GetDirectCommandList(cmd)->SetGraphicsRootDescriptorTable(pso_internal->bindpoint_sam, binding_table);
			}
			else
			{
				device->GetDirectCommandList(cmd)->SetComputeRootDescriptorTable(pso_internal->bindpoint_sam, binding_table);
			}
		}

	}
	GraphicsDevice_DX12::DescriptorBinder::DescriptorHandles
		GraphicsDevice_DX12::DescriptorBinder::commit(const DescriptorTable* table, CommandList cmd)
	{
		auto internal_state = to_internal(table);

		request_heaps(internal_state->resource_heap.desc.NumDescriptors, internal_state->sampler_heap.desc.NumDescriptors, cmd);

		DescriptorHandles handles;

		if (!internal_state->sampler_heap.ranges.empty())
		{
			auto& heap = device->descriptorheap_sam;
			D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = heap.start_cpu;
			D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = heap.start_gpu;
			cpu_handle.ptr += ringOffset_sam * device->sampler_descriptor_size;
			gpu_handle.ptr += ringOffset_sam * device->sampler_descriptor_size;
			device->device->CopyDescriptorsSimple(
				internal_state->sampler_heap.desc.NumDescriptors,
				cpu_handle,
				internal_state->sampler_heap.address,
				internal_state->sampler_heap.desc.Type
			);
			handles.sampler_handle = gpu_handle;
		}

		if (!internal_state->resource_heap.ranges.empty())
		{
			auto& heap = device->descriptorheap_res;
			D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = heap.start_cpu;
			D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = heap.start_gpu;
			cpu_handle.ptr += ringOffset_res * device->resource_descriptor_size;
			gpu_handle.ptr += ringOffset_res * device->resource_descriptor_size;
			device->device->CopyDescriptorsSimple(
				internal_state->resource_heap.desc.NumDescriptors,
				cpu_handle,
				internal_state->resource_heap.address,
				internal_state->resource_heap.desc.Type
			);
			handles.resource_handle = gpu_handle;
		}

		return handles;
	}


	void GraphicsDevice_DX12::pso_validate(CommandList cmd)
	{
		if (!dirty_pso[cmd])
			return;

		const PipelineState* pso = active_pso[cmd];
		size_t pipeline_hash = prev_pipeline_hash[cmd];

		auto internal_state = to_internal(pso);

		ID3D12PipelineState* pipeline = nullptr;
		auto it = pipelines_global.find(pipeline_hash);
		if (it == pipelines_global.end())
		{
			for (auto& x : pipelines_worker[cmd])
			{
				if (pipeline_hash == x.first)
				{
					pipeline = x.second.Get();
					break;
				}
			}

			if (pipeline == nullptr)
			{
				struct PSO_STREAM
				{
					CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
					CD3DX12_PIPELINE_STATE_STREAM_VS VS;
					CD3DX12_PIPELINE_STATE_STREAM_HS HS;
					CD3DX12_PIPELINE_STATE_STREAM_DS DS;
					CD3DX12_PIPELINE_STATE_STREAM_GS GS;
					CD3DX12_PIPELINE_STATE_STREAM_PS PS;
					CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RS;
					CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DSS;
					CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BD;
					CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PT;
					CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT IL;
					CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE STRIP;
					CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSFormat;
					CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS Formats;
					CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
					CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;

					CD3DX12_PIPELINE_STATE_STREAM_MS MS;
					CD3DX12_PIPELINE_STATE_STREAM_AS AS;
				} stream = {};

				if (pso->desc.vs != nullptr)
				{
					auto shader_internal = to_internal(pso->desc.vs);
					stream.VS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
				}
				if (pso->desc.hs != nullptr)
				{
					auto shader_internal = to_internal(pso->desc.hs);
					stream.HS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
				}
				if (pso->desc.ds != nullptr)
				{
					auto shader_internal = to_internal(pso->desc.ds);
					stream.DS = { shader_internal->shadercode.data(),shader_internal->shadercode.size() };
				}
				if (pso->desc.gs != nullptr)
				{
					auto shader_internal = to_internal(pso->desc.gs);
					stream.GS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
				}
				if (pso->desc.ps != nullptr)
				{
					auto shader_internal = to_internal(pso->desc.ps);
					stream.PS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
				}

				if (pso->desc.ms != nullptr)
				{
					auto shader_internal = to_internal(pso->desc.ms);
					stream.MS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
				}
				if (pso->desc.as != nullptr)
				{
					auto shader_internal = to_internal(pso->desc.as);
					stream.AS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
				}

				RasterizerState pRasterizerStateDesc = pso->desc.rs != nullptr ? *pso->desc.rs : RasterizerState();
				CD3DX12_RASTERIZER_DESC rs = {};
				rs.FillMode = _ConvertFillMode(pRasterizerStateDesc.FillMode);
				rs.CullMode = _ConvertCullMode(pRasterizerStateDesc.CullMode);
				rs.FrontCounterClockwise = pRasterizerStateDesc.FrontCounterClockwise;
				rs.DepthBias = pRasterizerStateDesc.DepthBias;
				rs.DepthBiasClamp = pRasterizerStateDesc.DepthBiasClamp;
				rs.SlopeScaledDepthBias = pRasterizerStateDesc.SlopeScaledDepthBias;
				rs.DepthClipEnable = pRasterizerStateDesc.DepthClipEnable;
				rs.MultisampleEnable = pRasterizerStateDesc.MultisampleEnable;
				rs.AntialiasedLineEnable = pRasterizerStateDesc.AntialiasedLineEnable;
				rs.ConservativeRaster = ((CheckCapability(GRAPHICSDEVICE_CAPABILITY_CONSERVATIVE_RASTERIZATION) && pRasterizerStateDesc.ConservativeRasterizationEnable) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
				rs.ForcedSampleCount = pRasterizerStateDesc.ForcedSampleCount;
				stream.RS = rs;

				DepthStencilState pDepthStencilStateDesc = pso->desc.dss != nullptr ? *pso->desc.dss : DepthStencilState();
				CD3DX12_DEPTH_STENCIL_DESC dss = {};
				dss.DepthEnable = pDepthStencilStateDesc.DepthEnable;
				dss.DepthWriteMask = _ConvertDepthWriteMask(pDepthStencilStateDesc.DepthWriteMask);
				dss.DepthFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.DepthFunc);
				dss.StencilEnable = pDepthStencilStateDesc.StencilEnable;
				dss.StencilReadMask = pDepthStencilStateDesc.StencilReadMask;
				dss.StencilWriteMask = pDepthStencilStateDesc.StencilWriteMask;
				dss.FrontFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc.FrontFace.StencilDepthFailOp);
				dss.FrontFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc.FrontFace.StencilFailOp);
				dss.FrontFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.FrontFace.StencilFunc);
				dss.FrontFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc.FrontFace.StencilPassOp);
				dss.BackFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc.BackFace.StencilDepthFailOp);
				dss.BackFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc.BackFace.StencilFailOp);
				dss.BackFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.BackFace.StencilFunc);
				dss.BackFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc.BackFace.StencilPassOp);
				stream.DSS = dss;

				BlendState pBlendStateDesc = pso->desc.bs != nullptr ? *pso->desc.bs : BlendState();
				CD3DX12_BLEND_DESC bd = {};
				bd.AlphaToCoverageEnable = pBlendStateDesc.AlphaToCoverageEnable;
				bd.IndependentBlendEnable = pBlendStateDesc.IndependentBlendEnable;
				for (int i = 0; i < 8; ++i)
				{
					bd.RenderTarget[i].BlendEnable = pBlendStateDesc.RenderTarget[i].BlendEnable;
					bd.RenderTarget[i].SrcBlend = _ConvertBlend(pBlendStateDesc.RenderTarget[i].SrcBlend);
					bd.RenderTarget[i].DestBlend = _ConvertBlend(pBlendStateDesc.RenderTarget[i].DestBlend);
					bd.RenderTarget[i].BlendOp = _ConvertBlendOp(pBlendStateDesc.RenderTarget[i].BlendOp);
					bd.RenderTarget[i].SrcBlendAlpha = _ConvertBlend(pBlendStateDesc.RenderTarget[i].SrcBlendAlpha);
					bd.RenderTarget[i].DestBlendAlpha = _ConvertBlend(pBlendStateDesc.RenderTarget[i].DestBlendAlpha);
					bd.RenderTarget[i].BlendOpAlpha = _ConvertBlendOp(pBlendStateDesc.RenderTarget[i].BlendOpAlpha);
					bd.RenderTarget[i].RenderTargetWriteMask = _ParseColorWriteMask(pBlendStateDesc.RenderTarget[i].RenderTargetWriteMask);
				}
				stream.BD = bd;

				std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
				D3D12_INPUT_LAYOUT_DESC il = {};
				if (pso->desc.il != nullptr)
				{
					il.NumElements = (uint32_t)pso->desc.il->elements.size();
					elements.resize(il.NumElements);
					for (uint32_t i = 0; i < il.NumElements; ++i)
					{
						elements[i].SemanticName = pso->desc.il->elements[i].SemanticName.c_str();
						elements[i].SemanticIndex = pso->desc.il->elements[i].SemanticIndex;
						elements[i].Format = _ConvertFormat(pso->desc.il->elements[i].Format);
						elements[i].InputSlot = pso->desc.il->elements[i].InputSlot;
						elements[i].AlignedByteOffset = pso->desc.il->elements[i].AlignedByteOffset;
						if (elements[i].AlignedByteOffset == InputLayout::APPEND_ALIGNED_ELEMENT)
							elements[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
						elements[i].InputSlotClass = _ConvertInputClassification(pso->desc.il->elements[i].InputSlotClass);
						elements[i].InstanceDataStepRate = pso->desc.il->elements[i].InstanceDataStepRate;
					}
				}
				il.pInputElementDescs = elements.data();
				stream.IL = il;

				DXGI_FORMAT DSFormat = DXGI_FORMAT_UNKNOWN;
				D3D12_RT_FORMAT_ARRAY formats = {};
				formats.NumRenderTargets = 0;
				DXGI_SAMPLE_DESC sampleDesc = {};
				sampleDesc.Count = 1;
				sampleDesc.Quality = 0;
				if (active_renderpass[cmd] == &dummyRenderpass)
				{
					formats.NumRenderTargets = 1;
					formats.RTFormats[0] = _ConvertFormat(BACKBUFFER_FORMAT);
				}
				else
				{
					for (auto& attachment : active_renderpass[cmd]->desc.attachments)
					{
						if (attachment.type == RenderPassAttachment::RESOLVE ||
							attachment.type == RenderPassAttachment::SHADING_RATE_SOURCE ||
							attachment.texture == nullptr)
							continue;

						switch (attachment.type)
						{
						case RenderPassAttachment::RENDERTARGET:
							switch (attachment.texture->desc.Format)
							{
							case FORMAT_R16_TYPELESS:
								formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R16_UNORM;
								break;
							case FORMAT_R32_TYPELESS:
								formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT;
								break;
							case FORMAT_R24G8_TYPELESS:
								formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
								break;
							case FORMAT_R32G8X24_TYPELESS:
								formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
								break;
							default:
								formats.RTFormats[formats.NumRenderTargets] = _ConvertFormat(attachment.texture->desc.Format);
								break;
							}
							formats.NumRenderTargets++;
							break;
						case RenderPassAttachment::DEPTH_STENCIL:
							switch (attachment.texture->desc.Format)
							{
							case FORMAT_R16_TYPELESS:
								DSFormat = DXGI_FORMAT_D16_UNORM;
								break;
							case FORMAT_R32_TYPELESS:
								DSFormat = DXGI_FORMAT_D32_FLOAT;
								break;
							case FORMAT_R24G8_TYPELESS:
								DSFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
								break;
							case FORMAT_R32G8X24_TYPELESS:
								DSFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
								break;
							default:
								DSFormat = _ConvertFormat(attachment.texture->desc.Format);
								break;
							}
							break;
						default:
							assert(0);
							break;
						}

						sampleDesc.Count = attachment.texture->desc.SampleCount;
						sampleDesc.Quality = 0;
					}
				}
				stream.DSFormat = DSFormat;
				stream.Formats = formats;
				stream.SampleDesc = sampleDesc;
				stream.SampleMask = pso->desc.sampleMask;

				switch (pso->desc.pt)
				{
				case POINTLIST:
					stream.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
					break;
				case LINELIST:
				case LINESTRIP:
					stream.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
					break;
				case TRIANGLELIST:
				case TRIANGLESTRIP:
					stream.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
					break;
				case PATCHLIST:
					stream.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
					break;
				default:
					stream.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
					break;
				}

				stream.STRIP = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

				if (pso->desc.rootSignature == nullptr)
				{
					stream.pRootSignature = internal_state->rootSignature.Get();
				}
				else
				{
					stream.pRootSignature = to_internal(pso->desc.rootSignature)->resource.Get();
				}

				D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
				streamDesc.pPipelineStateSubobjectStream = &stream;
				streamDesc.SizeInBytes = sizeof(stream);

				ComPtr<ID3D12PipelineState> newpso;
				HRESULT hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&newpso));
				assert(SUCCEEDED(hr));

				pipelines_worker[cmd].push_back(std::make_pair(pipeline_hash, newpso));
				pipeline = newpso.Get();
			}
		}
		else
		{
			pipeline = it->second.Get();
		}
		assert(pipeline != nullptr);

		GetDirectCommandList(cmd)->SetPipelineState(pipeline);

		if (prev_pt[cmd] != pso->desc.pt)
		{
			prev_pt[cmd] = pso->desc.pt;

			D3D12_PRIMITIVE_TOPOLOGY d3dType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			switch (pso->desc.pt)
			{
			case TRIANGLELIST:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				break;
			case TRIANGLESTRIP:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
				break;
			case POINTLIST:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
				break;
			case LINELIST:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
				break;
			case LINESTRIP:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
				break;
			case PATCHLIST:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
				break;
			default:
				break;
			};
			GetDirectCommandList(cmd)->IASetPrimitiveTopology(d3dType);
		}
	}

	void GraphicsDevice_DX12::query_flush(CommandList cmd)
	{
		// Perform query resolves (must be outside of render pass):
		assert(active_renderpass[cmd] == nullptr);

		if (!query_resolves_timestamp[cmd].empty())
		{
			for (auto& x : query_resolves_timestamp[cmd])
			{
				uint32_t block = QueryResolveBlock(x);
				uint32_t index = QueryResolveIndex(x);
				GetDirectCommandList(cmd)->ResolveQueryData(
					allocationhandler->queries_timestamp.blocks[block].pool.Get(),
					D3D12_QUERY_TYPE_TIMESTAMP,
					index,
					1,
					allocationhandler->queries_timestamp.blocks[block].readback.Get(),
					(uint64_t)index * sizeof(uint64_t)
				);
			}
			query_resolves_timestamp[cmd].clear();
		}

		if (!query_resolves_occlusion[cmd].empty())
		{
			for (auto& x : query_resolves_occlusion[cmd])
			{
				uint32_t block = QueryResolveBlock(x);
				uint32_t index = QueryResolveIndex(x);
				GetDirectCommandList(cmd)->ResolveQueryData(
					allocationhandler->queries_occlusion.blocks[block].pool.Get(),
					D3D12_QUERY_TYPE_OCCLUSION,
					index,
					1,
					allocationhandler->queries_occlusion.blocks[block].readback.Get(),
					(uint64_t)index * sizeof(uint64_t)
				);
			}
			query_resolves_occlusion[cmd].clear();
		}

		if (!query_resolves_occlusionpred[cmd].empty())
		{
			for (auto& x : query_resolves_occlusionpred[cmd])
			{
				uint32_t block = QueryResolveBlock(x);
				uint32_t index = QueryResolveIndex(x);
				GetDirectCommandList(cmd)->ResolveQueryData(
					allocationhandler->queries_occlusion.blocks[block].pool.Get(),
					D3D12_QUERY_TYPE_BINARY_OCCLUSION,
					index,
					1,
					allocationhandler->queries_occlusion.blocks[block].readback.Get(),
					(uint64_t)index * sizeof(uint64_t)
				);
			}
			query_resolves_occlusionpred[cmd].clear();
		}
	}
	void GraphicsDevice_DX12::barrier_flush(CommandList cmd)
	{
		auto& barriers = frame_barriers[cmd];
		if (!barriers.empty())
		{
			// Remove NOP barriers:
			for (size_t i = 0; i < barriers.size(); ++i)
			{
				auto& barrier = barriers[i];
				if (barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION &&
					barrier.Transition.StateBefore == barrier.Transition.StateAfter)
				{
					barrier = barriers.back();
					barriers.pop_back();
					i--;
				}
			}
			GetDirectCommandList(cmd)->ResourceBarrier(
				(UINT)barriers.size(),
				barriers.data()
			);
			barriers.clear();
		}
	}
	void GraphicsDevice_DX12::predraw(CommandList cmd)
	{
		pso_validate(cmd);

		if (active_pso[cmd]->desc.rootSignature == nullptr)
		{
			descriptors[cmd].validate(true, cmd);
		}
	}
	void GraphicsDevice_DX12::predispatch(CommandList cmd)
	{
		barrier_flush(cmd);
		if (active_cs[cmd]->rootSignature == nullptr)
		{
			descriptors[cmd].validate(false, cmd);
		}
	}
	void GraphicsDevice_DX12::preraytrace(CommandList cmd)
	{
		barrier_flush(cmd);
		if (active_rt[cmd]->desc.rootSignature == nullptr)
		{
			descriptors[cmd].validate(false, cmd);
		}
	}


	// Engine functions
	GraphicsDevice_DX12::GraphicsDevice_DX12(wiPlatform::window_type window, bool fullscreen, bool debuglayer)
	{
		capabilities |= GRAPHICSDEVICE_CAPABILITY_DESCRIPTOR_MANAGEMENT;
		capabilities |= GRAPHICSDEVICE_CAPABILITY_BINDLESS_DESCRIPTORS;
		SHADER_IDENTIFIER_SIZE = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

		DEBUGDEVICE = debuglayer;
		FULLSCREEN = fullscreen;

#ifndef PLATFORM_UWP
		RECT rect;
		GetClientRect(window, &rect);
		RESOLUTIONWIDTH = rect.right - rect.left;
		RESOLUTIONHEIGHT = rect.bottom - rect.top;
#else PLATFORM_UWP
		float dpiscale = wiPlatform::GetDPIScaling();
		RESOLUTIONWIDTH = int(window->Bounds.Width * dpiscale);
		RESOLUTIONHEIGHT = int(window->Bounds.Height * dpiscale);
#endif


#ifdef PLATFORM_UWP
		HMODULE dxcompiler = LoadPackagedLibrary(L"dxcompiler.dll", 0);
#else
		HMODULE dxgi = LoadLibraryEx(L"dxgi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		HMODULE dx12 = LoadLibraryEx(L"d3d12.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		HMODULE dxcompiler = LoadLibrary(L"dxcompiler.dll");

		CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY_2)GetProcAddress(dxgi, "CreateDXGIFactory2");
		assert(CreateDXGIFactory2 != nullptr);

		D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(dx12, "D3D12CreateDevice");
		assert(D3D12CreateDevice != nullptr);

		D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(dx12, "D3D12SerializeVersionedRootSignature");
		assert(D3D12SerializeVersionedRootSignature != nullptr);
#endif // PLATFORM_UWP

		DxcCreateInstanceProc DxcCreateInstance = (DxcCreateInstanceProc)GetProcAddress(dxcompiler, "DxcCreateInstance");
		assert(DxcCreateInstance != nullptr);

		HRESULT hr;

		hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
		assert(SUCCEEDED(hr));

#if !defined(PLATFORM_UWP)
		if (debuglayer)
		{
			// Enable the debug layer.
			auto D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(dx12, "D3D12GetDebugInterface");
			if (D3D12GetDebugInterface)
			{
				ComPtr<ID3D12Debug1> d3dDebug;
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug))))
				{
					d3dDebug->EnableDebugLayer();
					if (wiStartupArguments::HasArgument("gpuvalidation"))
					{
						d3dDebug->SetEnableGPUBasedValidation(TRUE);
					}
				}
			}
		}
#endif

		hr = CreateDXGIFactory2(debuglayer ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&factory));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "Failed to create DXGI factory! ERROR: " << std::hex << hr;
			wiHelper::messageBox(ss.str(), "Error!");
			assert(0);
			wiPlatform::Exit();
		}

		// pick the highest performance adapter that is able to create the device
		Microsoft::WRL::ComPtr<IDXGIAdapter1> candidateAdapter;
		for (uint32_t i = 0; factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&candidateAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 adapterDesc;
			candidateAdapter->GetDesc1(&adapterDesc);

			// ignore software adapter and check device creation succeeds
			if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
				SUCCEEDED(D3D12CreateDevice(candidateAdapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)))
			{
				candidateAdapter.As(&adapter);
				break;
			}
		}
		if (candidateAdapter == nullptr)
		{
			wiHelper::messageBox("No capable adapter found!", "Error!");
			assert(0);
			wiPlatform::Exit();
		}

		hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "Failed to create the graphics device! ERROR: " << std::hex << hr;
			wiHelper::messageBox(ss.str(), "Error!");
			assert(0);
			wiPlatform::Exit();
		}

		if (debuglayer)
		{
			ID3D12InfoQueue* d3dInfoQueue = nullptr;
			if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D12InfoQueue), (void**)&d3dInfoQueue)))
			{
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

				D3D12_MESSAGE_ID hide[] =
				{
					D3D12_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
					// Add more message IDs here as needed
				};

				D3D12_INFO_QUEUE_FILTER filter = {};
				filter.DenyList.NumIDs = _countof(hide);
				filter.DenyList.pIDList = hide;
				d3dInfoQueue->AddStorageFilterEntries(&filter);
				d3dInfoQueue->Release();
			}
		}

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = device.Get();
		allocatorDesc.pAdapter = adapter.Get();

		allocationhandler = std::make_shared<AllocationHandler>();
		allocationhandler->device = device;

		hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocationhandler->allocator);
		assert(SUCCEEDED(hr));

		// Create command queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 0;
		hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&directQueue));
		assert(SUCCEEDED(hr));

		// Create fences for command queue:
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFence));
		assert(SUCCEEDED(hr));
		frameFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);


		// Create swapchain
		ComPtr<IDXGISwapChain1> _swapChain;

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = RESOLUTIONWIDTH;
		sd.Height = RESOLUTIONHEIGHT;
		sd.Format = _ConvertFormat(GetBackBufferFormat());
		sd.Stereo = false;
		sd.SampleDesc.Count = 1; // Don't use multi-sampling.
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = BACKBUFFER_COUNT;
		sd.Flags = 0;
		sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

#ifndef PLATFORM_UWP
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		sd.Scaling = DXGI_SCALING_STRETCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
		fullscreenDesc.RefreshRate.Numerator = 60;
		fullscreenDesc.RefreshRate.Denominator = 1;
		fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // needs to be unspecified for correct fullscreen scaling!
		fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
		fullscreenDesc.Windowed = !fullscreen;
		hr = factory->CreateSwapChainForHwnd(directQueue.Get(), window, &sd, &fullscreenDesc, nullptr, &_swapChain);
#else
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
		sd.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

		hr = factory->CreateSwapChainForCoreWindow(directQueue.Get(), reinterpret_cast<IUnknown*>(window.Get()), &sd, nullptr, &_swapChain);
#endif

		if (FAILED(hr))
		{
			wiHelper::messageBox("Failed to create a swapchain for the graphics device!", "Error!");
			assert(0);
			wiPlatform::Exit();
		}

		hr = _swapChain.As(&swapChain);
		assert(SUCCEEDED(hr));

		rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		dsv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		resource_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		sampler_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		// Resource descriptor heap (shader visible):
		{
			descriptorheap_res.heapDesc.NodeMask = 0;
			descriptorheap_res.heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			descriptorheap_res.heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			descriptorheap_res.heapDesc.NumDescriptors = 1000000; // tier 1 limit
			hr = device->CreateDescriptorHeap(&descriptorheap_res.heapDesc, IID_PPV_ARGS(&descriptorheap_res.heap_GPU));
			assert(SUCCEEDED(hr));

			descriptorheap_res.start_cpu = descriptorheap_res.heap_GPU->GetCPUDescriptorHandleForHeapStart();
			descriptorheap_res.start_gpu = descriptorheap_res.heap_GPU->GetGPUDescriptorHandleForHeapStart();

			hr = device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&descriptorheap_res.fence));
			assert(SUCCEEDED(hr));
			descriptorheap_res.fenceValue = descriptorheap_res.fence->GetCompletedValue();
		}

		// Sampler descriptor heap (shader visible):
		{
			descriptorheap_sam.heapDesc.NodeMask = 0;
			descriptorheap_sam.heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			descriptorheap_sam.heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			descriptorheap_sam.heapDesc.NumDescriptors = 2048; // tier 1 limit
			hr = device->CreateDescriptorHeap(&descriptorheap_sam.heapDesc, IID_PPV_ARGS(&descriptorheap_sam.heap_GPU));
			assert(SUCCEEDED(hr));

			descriptorheap_sam.start_cpu = descriptorheap_sam.heap_GPU->GetCPUDescriptorHandleForHeapStart();
			descriptorheap_sam.start_gpu = descriptorheap_sam.heap_GPU->GetGPUDescriptorHandleForHeapStart();

			hr = device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&descriptorheap_sam.fence));
			assert(SUCCEEDED(hr));
			descriptorheap_sam.fenceValue = descriptorheap_sam.fence->GetCompletedValue();
		}

		// Create frame-resident resources:
		for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
		{
			hr = swapChain->GetBuffer(fr, IID_PPV_ARGS(&backBuffers[fr]));
			assert(SUCCEEDED(hr));
		}

		copyAllocator.Create(device);

		hr = device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&directFence));
		assert(SUCCEEDED(hr));
		directFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		directFenceValue = directFence->GetCompletedValue();

		// Query features:

		capabilities |= GRAPHICSDEVICE_CAPABILITY_TESSELLATION;

		hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &features_0, sizeof(features_0));
		if (features_0.ConservativeRasterizationTier >= D3D12_CONSERVATIVE_RASTERIZATION_TIER_1)
		{
			capabilities |= GRAPHICSDEVICE_CAPABILITY_CONSERVATIVE_RASTERIZATION;
		}
		if (features_0.ROVsSupported == TRUE)
		{
			capabilities |= GRAPHICSDEVICE_CAPABILITY_RASTERIZER_ORDERED_VIEWS;
		}
		if (features_0.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation == TRUE)
		{
			capabilities |= GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS;
		}

		if (features_0.TypedUAVLoadAdditionalFormats)
		{
			// More info about UAV format load support: https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
			capabilities |= GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_COMMON;

			D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport = { DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
			hr = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
			if (SUCCEEDED(hr) && (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
			{
				capabilities |= GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT;
			}
		}

		hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features_5, sizeof(features_5));
		if (features_5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
		{
			capabilities |= GRAPHICSDEVICE_CAPABILITY_RAYTRACING;
			if (features_5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
			{
				capabilities |= GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE;
			}
		}

		hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &features_6, sizeof(features_6));
		if (features_6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1)
		{
			capabilities |= GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING;
			if (features_6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
			{
				capabilities |= GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2;
				VARIABLE_RATE_SHADING_TILE_SIZE = features_6.ShadingRateImageTileSize;
			}
		}

		hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features_7, sizeof(features_7));
		if (features_7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
		{
			capabilities |= GRAPHICSDEVICE_CAPABILITY_MESH_SHADER;
		}

		D3D12_FEATURE_DATA_ROOT_SIGNATURE features_rootsignature = {};
		features_rootsignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		hr = device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &features_rootsignature, sizeof(features_rootsignature));
		if (features_rootsignature.HighestVersion < D3D_ROOT_SIGNATURE_VERSION_1_1)
		{
			wiBackLog::post("DX12: Root signature version 1.1 not supported!");
			assert(0);
		}

		// Create common indirect command signatures:

		D3D12_COMMAND_SIGNATURE_DESC cmd_desc = {};

		D3D12_INDIRECT_ARGUMENT_DESC dispatchArgs[1];
		dispatchArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

		D3D12_INDIRECT_ARGUMENT_DESC drawInstancedArgs[1];
		drawInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

		D3D12_INDIRECT_ARGUMENT_DESC drawIndexedInstancedArgs[1];
		drawIndexedInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		cmd_desc.ByteStride = sizeof(IndirectDispatchArgs);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = dispatchArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, IID_PPV_ARGS(&dispatchIndirectCommandSignature));
		assert(SUCCEEDED(hr));

		cmd_desc.ByteStride = sizeof(IndirectDrawArgsInstanced);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = drawInstancedArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, IID_PPV_ARGS(&drawInstancedIndirectCommandSignature));
		assert(SUCCEEDED(hr));

		cmd_desc.ByteStride = sizeof(IndirectDrawArgsIndexedInstanced);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = drawIndexedInstancedArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, IID_PPV_ARGS(&drawIndexedInstancedIndirectCommandSignature));
		assert(SUCCEEDED(hr));

		if (CheckCapability(GRAPHICSDEVICE_CAPABILITY_MESH_SHADER))
		{
			D3D12_INDIRECT_ARGUMENT_DESC dispatchMeshArgs[1];
			dispatchMeshArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;

			cmd_desc.ByteStride = sizeof(IndirectDispatchArgs);
			cmd_desc.NumArgumentDescs = 1;
			cmd_desc.pArgumentDescs = dispatchMeshArgs;
			hr = device->CreateCommandSignature(&cmd_desc, nullptr, IID_PPV_ARGS(&dispatchMeshIndirectCommandSignature));
			assert(SUCCEEDED(hr));
		}

		allocationhandler->descriptors_res.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		allocationhandler->descriptors_sam.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		allocationhandler->descriptors_rtv.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		allocationhandler->descriptors_dsv.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		for (int i = 0; i < arraysize(backBuffers); ++i)
		{
			backbufferRTV[i] = allocationhandler->descriptors_rtv.allocate();
			device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, backbufferRTV[i]);
		}

		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
			nullCBV = allocationhandler->descriptors_res.allocate();
			device->CreateConstantBufferView(&cbv_desc, nullCBV);
		}
		{
			D3D12_SAMPLER_DESC sampler_desc = {};
			sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			nullSAM = allocationhandler->descriptors_sam.allocate();
			device->CreateSampler(&sampler_desc, nullSAM);
		}
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R32_UINT;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			nullSRV_buffer = allocationhandler->descriptors_res.allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, nullSRV_buffer);
		}
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			nullSRV_texture1d = allocationhandler->descriptors_res.allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, nullSRV_texture1d);
		}
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
			nullSRV_texture1darray = allocationhandler->descriptors_res.allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, nullSRV_texture1darray);
		}
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			nullSRV_texture2d = allocationhandler->descriptors_res.allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, nullSRV_texture2d);
		}
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			nullSRV_texture2darray = allocationhandler->descriptors_res.allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, nullSRV_texture2darray);
		}
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			nullSRV_texturecube = allocationhandler->descriptors_res.allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, nullSRV_texturecube);
		}
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
			nullSRV_texturecubearray = allocationhandler->descriptors_res.allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, nullSRV_texturecubearray);
		}
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			nullSRV_texture3d = allocationhandler->descriptors_res.allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, nullSRV_texture3d);
		}
		if(CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_UNKNOWN;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
			nullSRV_accelerationstructure = allocationhandler->descriptors_res.allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, nullSRV_accelerationstructure);
		}
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			nullUAV_buffer = allocationhandler->descriptors_res.allocate();
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, nullUAV_buffer);
		}
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			nullUAV_texture1d = allocationhandler->descriptors_res.allocate();
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, nullUAV_texture1d);
		}
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
			nullUAV_texture1darray = allocationhandler->descriptors_res.allocate();
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, nullUAV_texture1darray);
		}
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			nullUAV_texture2d = allocationhandler->descriptors_res.allocate();
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, nullUAV_texture2d);
		}
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			nullUAV_texture2darray = allocationhandler->descriptors_res.allocate();
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, nullUAV_texture2darray);
		}
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			nullUAV_texture3d = allocationhandler->descriptors_res.allocate();
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, nullUAV_texture3d);
		}

		// GPU Queries:
		allocationhandler->queries_occlusion.init(allocationhandler.get(), D3D12_QUERY_HEAP_TYPE_OCCLUSION);
		allocationhandler->queries_timestamp.init(allocationhandler.get(), D3D12_QUERY_HEAP_TYPE_TIMESTAMP);

		wiBackLog::post("Created GraphicsDevice_DX12");
	}
	GraphicsDevice_DX12::~GraphicsDevice_DX12()
	{
		WaitForGPU();

		CloseHandle(frameFenceEvent);
		CloseHandle(directFenceEvent);
	}

	void GraphicsDevice_DX12::SetResolution(int width, int height)
	{
		if ((width != RESOLUTIONWIDTH || height != RESOLUTIONHEIGHT) && width > 0 && height > 0)
		{
			RESOLUTIONWIDTH = width;
			RESOLUTIONHEIGHT = height;

			WaitForGPU();

			for (int i = 0; i < arraysize(backBuffers); ++i)
			{
				backBuffers[i].Reset();
			}

			HRESULT hr = swapChain->ResizeBuffers(GetBackBufferCount(), width, height, _ConvertFormat(GetBackBufferFormat()), 0);
			assert(SUCCEEDED(hr));
			
			for (int i = 0; i < arraysize(backBuffers); ++i)
			{
				uint32_t fr = (GetFrameCount() + i) % BACKBUFFER_COUNT;
				hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[fr]));
				assert(SUCCEEDED(hr));

				device->CreateRenderTargetView(backBuffers[fr].Get(), nullptr, backbufferRTV[fr]);
			}
		}
	}

	Texture GraphicsDevice_DX12::GetBackBuffer()
	{
		auto internal_state = std::make_shared<Texture_DX12>();
		internal_state->allocationhandler = allocationhandler;
		internal_state->resource = backBuffers[backbuffer_index];

		D3D12_RESOURCE_DESC desc = internal_state->resource->GetDesc();
		device->GetCopyableFootprints(&desc, 0, 1, 0, &internal_state->footprint, nullptr, nullptr, nullptr);

		Texture result;
		result.type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;
		result.internal_state = internal_state;
		result.desc = _ConvertTextureDesc_Inv(desc);
		return result;
	}

	bool GraphicsDevice_DX12::CreateBuffer(const GPUBufferDesc* pDesc, const SubresourceData* pInitialData, GPUBuffer* pBuffer)
	{
		auto internal_state = std::make_shared<Resource_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pBuffer->internal_state = internal_state;
		pBuffer->type = GPUResource::GPU_RESOURCE_TYPE::BUFFER;

		pBuffer->desc = *pDesc;

		if (pDesc->Usage == USAGE_DYNAMIC && pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			// this special case will use frame allocator
			return true;
		}

		HRESULT hr = E_FAIL;

		size_t alignedSize = pDesc->ByteWidth;
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			alignedSize = Align(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		}

		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Width = (UINT64)alignedSize;
		desc.Height = 1;
		desc.MipLevels = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.DepthOrArraySize = 1;
		desc.Alignment = 0;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		if (pDesc->Usage == USAGE_STAGING)
		{
			if (pDesc->CPUAccessFlags & CPU_ACCESS_READ)
			{
				allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
				resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
			}
			else
			{
				allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
				resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
			}
		}

		device->GetCopyableFootprints(&desc, 0, 1, 0, &internal_state->footprint, nullptr, nullptr, nullptr);

		hr = allocationhandler->allocator->CreateResource(
			&allocationDesc,
			&desc,
			resourceState,
			nullptr,
			&internal_state->allocation,
			IID_PPV_ARGS(&internal_state->resource)
		);
		assert(SUCCEEDED(hr));

		internal_state->gpu_address = internal_state->resource->GetGPUVirtualAddress();

		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			GPUBufferDesc uploaddesc;
			uploaddesc.ByteWidth = pDesc->ByteWidth;
			uploaddesc.Usage = USAGE_STAGING;

			auto cmd = copyAllocator.allocate(uploaddesc.ByteWidth);
			if (cmd.uploadbuffer.desc.ByteWidth < uploaddesc.ByteWidth)
			{
				bool upload_success = CreateBuffer(&uploaddesc, nullptr, &cmd.uploadbuffer);
				assert(upload_success);
			}

			ID3D12Resource* upload_resource = to_internal(&cmd.uploadbuffer)->resource.Get();

			void* pData;
			CD3DX12_RANGE readRange(0, 0);
			hr = upload_resource->Map(0, &readRange, &pData);
			assert(SUCCEEDED(hr));
			memcpy(pData, pInitialData->pSysMem, pDesc->ByteWidth);

			cmd.commandList->CopyBufferRegion(
				internal_state->resource.Get(),
				0,
				upload_resource,
				0,
				pDesc->ByteWidth
			);

			copyAllocator.submit(cmd);
		}


		// Create resource views if needed
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
			cbv_desc.SizeInBytes = (uint32_t)alignedSize;
			cbv_desc.BufferLocation = internal_state->gpu_address;

			internal_state->cbv.init(this, cbv_desc);
		}

		if (pDesc->BindFlags & BIND_SHADER_RESOURCE)
		{
			CreateSubresource(pBuffer, SRV, 0);
		}

		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			CreateSubresource(pBuffer, UAV, 0);
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture)
	{
		auto internal_state = std::make_shared<Texture_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pTexture->internal_state = internal_state;
		pTexture->type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;

		pTexture->desc = *pDesc;


		HRESULT hr = E_FAIL;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC desc;
		desc.Format = _ConvertFormat(pDesc->Format);
		desc.Width = pDesc->Width;
		desc.Height = pDesc->Height;
		desc.MipLevels = pDesc->MipLevels;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.DepthOrArraySize = (UINT16)pDesc->ArraySize;
		desc.SampleDesc.Count = pDesc->SampleCount;
		desc.SampleDesc.Quality = 0;
		desc.Alignment = 0;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (pDesc->BindFlags & BIND_DEPTH_STENCIL)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
			if (!(pDesc->BindFlags & BIND_SHADER_RESOURCE))
			{
				desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}
		if (pDesc->BindFlags & BIND_RENDER_TARGET)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
		}
		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			//desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
			if (pInitialData == nullptr)
			{
				allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
			}
		}

		switch (pTexture->desc.type)
		{
		case TextureDesc::TEXTURE_1D:
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			break;
		case TextureDesc::TEXTURE_2D:
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			break;
		case TextureDesc::TEXTURE_3D:
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			desc.DepthOrArraySize = (UINT16)pDesc->Depth;
			break;
		default:
			assert(0);
			break;
		}

		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Color[0] = pTexture->desc.clear.color[0];
		optimizedClearValue.Color[1] = pTexture->desc.clear.color[1];
		optimizedClearValue.Color[2] = pTexture->desc.clear.color[2];
		optimizedClearValue.Color[3] = pTexture->desc.clear.color[3];
		optimizedClearValue.DepthStencil.Depth = pTexture->desc.clear.depthstencil.depth;
		optimizedClearValue.DepthStencil.Stencil = pTexture->desc.clear.depthstencil.stencil;
		optimizedClearValue.Format = desc.Format;
		if (optimizedClearValue.Format == DXGI_FORMAT_R16_TYPELESS)
		{
			optimizedClearValue.Format = DXGI_FORMAT_D16_UNORM;
		}
		else if (optimizedClearValue.Format == DXGI_FORMAT_R32_TYPELESS)
		{
			optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		}
		else if (optimizedClearValue.Format == DXGI_FORMAT_R32G8X24_TYPELESS)
		{
			optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		}
		bool useClearValue = pDesc->BindFlags & BIND_RENDER_TARGET || pDesc->BindFlags & BIND_DEPTH_STENCIL;

		D3D12_RESOURCE_STATES resourceState = _ConvertImageLayout(pTexture->desc.layout);

		if (pInitialData != nullptr)
		{
			resourceState = D3D12_RESOURCE_STATE_COMMON;
		}

		if (pTexture->desc.Usage == USAGE_STAGING)
		{
			UINT64 RequiredSize = 0;
			device->GetCopyableFootprints(&desc, 0, 1, 0, &internal_state->footprint, nullptr, nullptr, &RequiredSize);
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Width = RequiredSize;
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;

			if (pTexture->desc.CPUAccessFlags & CPU_ACCESS_READ)
			{
				allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
				resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
			}
			else
			{
				allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
				resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
			}
		}

		hr = allocationhandler->allocator->CreateResource(
			&allocationDesc,
			&desc,
			resourceState,
			useClearValue ? &optimizedClearValue : nullptr,
			&internal_state->allocation,
			IID_PPV_ARGS(&internal_state->resource)
		);
		assert(SUCCEEDED(hr));

		if (pTexture->desc.MipLevels == 0)
		{
			pTexture->desc.MipLevels = (uint32_t)log2(std::max(pTexture->desc.Width, pTexture->desc.Height)) + 1;
		}


		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			uint32_t dataCount = pDesc->ArraySize * std::max(1u, pDesc->MipLevels);
			std::vector<D3D12_SUBRESOURCE_DATA> data(dataCount);
			for (uint32_t slice = 0; slice < dataCount; ++slice)
			{
				data[slice] = _ConvertSubresourceData(pInitialData[slice]);
			}

			UINT64 RequiredSize = 0;
			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(dataCount);
			std::vector<UINT64> rowSizesInBytes(dataCount);
			std::vector<UINT> numRows(dataCount);
			device->GetCopyableFootprints(&desc, 0, dataCount, 0, layouts.data(), numRows.data(), rowSizesInBytes.data(), &RequiredSize);

			GPUBufferDesc uploaddesc;
			uploaddesc.ByteWidth = (uint32_t)RequiredSize;
			uploaddesc.Usage = USAGE_STAGING;

			auto cmd = copyAllocator.allocate(uploaddesc.ByteWidth);
			if (cmd.uploadbuffer.desc.ByteWidth < uploaddesc.ByteWidth)
			{
				bool upload_success = CreateBuffer(&uploaddesc, nullptr, &cmd.uploadbuffer);
				assert(upload_success);
			}

			ID3D12Resource* upload_resource = to_internal(&cmd.uploadbuffer)->resource.Get();

			void* pData;
			CD3DX12_RANGE readRange(0, 0);
			hr = upload_resource->Map(0, &readRange, &pData);
			assert(SUCCEEDED(hr));

			for (uint32_t i = 0; i < dataCount; ++i)
			{
				if (rowSizesInBytes[i] > (SIZE_T)-1)
					continue;
				D3D12_MEMCPY_DEST DestData = {};
				DestData.pData = (void*)((UINT64)pData + layouts[i].Offset);
				DestData.RowPitch = (SIZE_T)layouts[i].Footprint.RowPitch;
				DestData.SlicePitch = (SIZE_T)layouts[i].Footprint.RowPitch * (SIZE_T)numRows[i];
				MemcpySubresource(&DestData, &data[i], (SIZE_T)rowSizesInBytes[i], numRows[i], layouts[i].Footprint.Depth);
			}

			for (UINT i = 0; i < dataCount; ++i)
			{
				CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state->resource.Get(), i);
				CD3DX12_TEXTURE_COPY_LOCATION Src(upload_resource, layouts[i]);
				cmd.commandList->CopyTextureRegion(
					&Dst,
					0,
					0,
					0,
					&Src,
					nullptr
				);
			}

			copyAllocator.submit(cmd);
		}

		if (pTexture->desc.BindFlags & BIND_RENDER_TARGET)
		{
			CreateSubresource(pTexture, RTV, 0, -1, 0, -1);
		}
		if (pTexture->desc.BindFlags & BIND_DEPTH_STENCIL)
		{
			CreateSubresource(pTexture, DSV, 0, -1, 0, -1);
		}
		if (pTexture->desc.BindFlags & BIND_SHADER_RESOURCE)
		{
			CreateSubresource(pTexture, SRV, 0, -1, 0, -1);
		}
		if (pTexture->desc.BindFlags & BIND_UNORDERED_ACCESS)
		{
			CreateSubresource(pTexture, UAV, 0, -1, 0, -1);
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateShader(SHADERSTAGE stage, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader)
	{
		auto internal_state = std::make_shared<PipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pShader->internal_state = internal_state;

		internal_state->shadercode.resize(BytecodeLength);
		std::memcpy(internal_state->shadercode.data(), pShaderBytecode, BytecodeLength);
		pShader->stage = stage;

		HRESULT hr = (internal_state->shadercode.empty() ? E_FAIL : S_OK);
		assert(SUCCEEDED(hr));


		if (pShader->rootSignature == nullptr)
		{
			auto insert_descriptor = [&](const D3D12_SHADER_INPUT_BIND_DESC& desc)
			{
				if (desc.Type == D3D_SIT_SAMPLER)
				{
					for (auto& sam : pShader->auto_samplers)
					{
						if (desc.BindPoint == sam.slot)
						{
							internal_state->staticsamplers.push_back(_ConvertStaticSampler(sam));
							return; // static sampler will be used instead
						}
					}
					for (auto& sam : common_samplers)
					{
						if (desc.BindPoint == sam.ShaderRegister)
						{
							internal_state->staticsamplers.push_back(sam);
							return; // static sampler will be used instead
						}
					}

					internal_state->samplers.emplace_back();
					D3D12_DESCRIPTOR_RANGE1& descriptor = internal_state->samplers.back();

					descriptor.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

					descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
					descriptor.BaseShaderRegister = desc.BindPoint;
					descriptor.NumDescriptors = desc.BindCount;
					descriptor.RegisterSpace = desc.Space;
					descriptor.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				}
				else
				{
					internal_state->resources.emplace_back();
					D3D12_DESCRIPTOR_RANGE1& descriptor = internal_state->resources.back();

					descriptor.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

					switch (desc.Type)
					{
					default:
					case D3D_SIT_CBUFFER:
						descriptor.Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
						descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
						break;
					case D3D_SIT_TBUFFER:
					case D3D_SIT_STRUCTURED:
					case D3D_SIT_BYTEADDRESS:
						descriptor.Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
						descriptor.Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
						descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						break;
					case D3D_SIT_TEXTURE:
					case D3D_SIT_RTACCELERATIONSTRUCTURE:
						descriptor.Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
						descriptor.Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
						descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						break;
					case D3D_SIT_UAV_RWSTRUCTURED:
					case D3D_SIT_UAV_RWBYTEADDRESS:
					case D3D_SIT_UAV_APPEND_STRUCTURED:
					case D3D_SIT_UAV_CONSUME_STRUCTURED:
					case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
					case D3D_SIT_UAV_RWTYPED:
					case D3D_SIT_UAV_FEEDBACKTEXTURE:
						descriptor.Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
						descriptor.Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
						descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						break;
					}
					descriptor.BaseShaderRegister = desc.BindPoint;
					descriptor.NumDescriptors = desc.BindCount;
					descriptor.RegisterSpace = desc.Space;
					descriptor.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

					internal_state->resource_bindings.emplace_back();
					RESOURCEBINDING& binding = internal_state->resource_bindings.back();

					switch (desc.Type)
					{
					default:
					case D3D_SIT_CBUFFER:
						binding = CONSTANTBUFFER;
						break;
					case D3D_SIT_TBUFFER:
						binding = TYPEDBUFFER;
						break;
					case D3D_SIT_STRUCTURED:
						binding = STRUCTUREDBUFFER;
						break;
					case D3D_SIT_BYTEADDRESS:
						binding = RAWBUFFER;
						break;
					case D3D_SIT_TEXTURE:
						switch (desc.Dimension)
						{
						case D3D_SRV_DIMENSION_BUFFER:
							binding = TYPEDBUFFER;
							break;
						case D3D_SRV_DIMENSION_TEXTURE1D:
							binding = TEXTURE1D;
							break;
						case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
							binding = TEXTURE1DARRAY;
							break;
						case D3D_SRV_DIMENSION_TEXTURE2D:
						case D3D_SRV_DIMENSION_TEXTURE2DMS:
							binding = TEXTURE2D;
							break;
						case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
						case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
							binding = TEXTURE2DARRAY;
							break;
						case D3D_SRV_DIMENSION_TEXTURE3D:
							binding = TEXTURE3D;
							break;
						case D3D_SRV_DIMENSION_TEXTURECUBE:
							binding = TEXTURECUBE;
							break;
						case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
							binding = TEXTURECUBEARRAY;
							break;
						default:
							assert(0);
							break;
						}
						break;
					case D3D_SIT_RTACCELERATIONSTRUCTURE:
						binding = ACCELERATIONSTRUCTURE;
						break;
					case D3D_SIT_UAV_RWSTRUCTURED:
					case D3D_SIT_UAV_APPEND_STRUCTURED:
					case D3D_SIT_UAV_CONSUME_STRUCTURED:
					case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
						binding = RWSTRUCTUREDBUFFER;
						break;
					case D3D_SIT_UAV_RWBYTEADDRESS:
						binding = RWRAWBUFFER;
						break;
					case D3D_SIT_UAV_RWTYPED:
						binding = RWTYPEDBUFFER;
						break;
					case D3D_SIT_UAV_FEEDBACKTEXTURE:
						switch (desc.Dimension)
						{
						case D3D_SRV_DIMENSION_BUFFER:
							binding = RWTYPEDBUFFER;
							break;
						case D3D_SRV_DIMENSION_TEXTURE1D:
							binding = RWTEXTURE1D;
							break;
						case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
							binding = RWTEXTURE1DARRAY;
							break;
						case D3D_SRV_DIMENSION_TEXTURE2D:
							binding = RWTEXTURE2D;
							break;
						case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
							binding = RWTEXTURE2DARRAY;
							break;
						case D3D_SRV_DIMENSION_TEXTURE3D:
							binding = RWTEXTURE3D;
							break;
						default:
							assert(0);
							break;
						}
						break;
					}
				}
			};

			DxcBuffer ReflectionData;
			ReflectionData.Encoding = DXC_CP_ACP;
			ReflectionData.Ptr = pShaderBytecode;
			ReflectionData.Size = (SIZE_T)BytecodeLength;

			if (stage == LIB)
			{
				ComPtr<ID3D12LibraryReflection> reflection;
				hr = dxcUtils->CreateReflection(&ReflectionData, IID_PPV_ARGS(&reflection));
				assert(SUCCEEDED(hr));

				D3D12_LIBRARY_DESC library_desc;
				hr = reflection->GetDesc(&library_desc);
				assert(SUCCEEDED(hr));

				for (UINT i = 0; i < library_desc.FunctionCount; ++i)
				{
					ID3D12FunctionReflection* function_reflection = reflection->GetFunctionByIndex((INT)i);
					assert(function_reflection != nullptr);
					D3D12_FUNCTION_DESC function_desc;
					hr = function_reflection->GetDesc(&function_desc);
					assert(SUCCEEDED(hr));

					for (UINT i = 0; i < function_desc.BoundResources; ++i)
					{
						D3D12_SHADER_INPUT_BIND_DESC desc;
						hr = function_reflection->GetResourceBindingDesc(i, &desc);
						assert(SUCCEEDED(hr));
						insert_descriptor(desc);
					}
				}
			}
			else // Shader reflection
			{
				ComPtr<ID3D12ShaderReflection > reflection;
				hr = dxcUtils->CreateReflection(&ReflectionData, IID_PPV_ARGS(&reflection));
				assert(SUCCEEDED(hr));

				D3D12_SHADER_DESC shader_desc;
				hr = reflection->GetDesc(&shader_desc);
				assert(SUCCEEDED(hr));

				for (UINT i = 0; i < shader_desc.BoundResources; ++i)
				{
					D3D12_SHADER_INPUT_BIND_DESC desc;
					hr = reflection->GetResourceBindingDesc(i, &desc);
					assert(SUCCEEDED(hr));
					insert_descriptor(desc);
				}
			}

			for (auto& sam : internal_state->staticsamplers)
			{
				switch (stage)
				{
				case MS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_MESH;
					break;
				case AS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_AMPLIFICATION;
					break;
				case VS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
					break;
				case HS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
					break;
				case DS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
					break;
				case GS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
					break;
				case PS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
					break;
				default:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					break;
				}
			}


			if (stage == CS || stage == LIB)
			{
				std::vector<D3D12_ROOT_PARAMETER1> params;

				// Split resources into root descriptors and tables:
				{
					std::vector<D3D12_DESCRIPTOR_RANGE1> resources;
					std::vector<RESOURCEBINDING> bindings;
					int i = 0;
					for (auto& x : internal_state->resources)
					{
						RESOURCEBINDING binding = internal_state->resource_bindings[i++];
						if (x.NumDescriptors == 1 && binding == CONSTANTBUFFER && internal_state->root_cbvs.size() < CONSTANT_BUFFER_AUTO_PLACEMENT_IN_ROOT)
						{
							internal_state->root_cbvs.emplace_back();
							D3D12_ROOT_DESCRIPTOR1& descriptor = internal_state->root_cbvs.back();
							descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
							descriptor.ShaderRegister = x.BaseShaderRegister;
							descriptor.RegisterSpace = x.RegisterSpace;
						}
						else
						{
							resources.push_back(x);
							bindings.push_back(binding);
						}
					}
					internal_state->resources = resources;
					internal_state->resource_bindings = bindings;
				}

				for (auto& x : internal_state->root_cbvs)
				{
					params.emplace_back();
					D3D12_ROOT_PARAMETER1& param = params.back();
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.Descriptor = x;
				}

				if (!internal_state->resources.empty())
				{
					internal_state->bindpoint_res = (uint32_t)params.size();
					params.emplace_back();
					D3D12_ROOT_PARAMETER1& param = params.back();
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->resources.size();
					param.DescriptorTable.pDescriptorRanges = internal_state->resources.data();
				}

				if (!internal_state->samplers.empty())
				{
					internal_state->bindpoint_sam = (uint32_t)params.size();
					params.emplace_back();
					D3D12_ROOT_PARAMETER1& param = params.back();
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->samplers.size();
					param.DescriptorTable.pDescriptorRanges = internal_state->samplers.data();
				}

				internal_state->root_binding_hash = 0;
				for (auto& x : internal_state->root_cbvs)
				{
					wiHelper::hash_combine(internal_state->root_binding_hash, x.Flags);
					wiHelper::hash_combine(internal_state->root_binding_hash, x.ShaderRegister);
					wiHelper::hash_combine(internal_state->root_binding_hash, x.RegisterSpace);
				}

				internal_state->resource_binding_hash = 0;
				for (auto& x : internal_state->resources)
				{
					wiHelper::hash_combine(internal_state->resource_binding_hash, x.BaseShaderRegister);
					wiHelper::hash_combine(internal_state->resource_binding_hash, x.NumDescriptors);
					wiHelper::hash_combine(internal_state->resource_binding_hash, x.Flags);
					wiHelper::hash_combine(internal_state->resource_binding_hash, x.OffsetInDescriptorsFromTableStart);
					wiHelper::hash_combine(internal_state->resource_binding_hash, x.RangeType);
					wiHelper::hash_combine(internal_state->resource_binding_hash, x.RegisterSpace);
				}

				internal_state->sampler_binding_hash = 0;
				for (auto& x : internal_state->samplers)
				{
					wiHelper::hash_combine(internal_state->sampler_binding_hash, x.BaseShaderRegister);
					wiHelper::hash_combine(internal_state->sampler_binding_hash, x.NumDescriptors);
					wiHelper::hash_combine(internal_state->sampler_binding_hash, x.Flags);
					wiHelper::hash_combine(internal_state->sampler_binding_hash, x.OffsetInDescriptorsFromTableStart);
					wiHelper::hash_combine(internal_state->sampler_binding_hash, x.RangeType);
					wiHelper::hash_combine(internal_state->sampler_binding_hash, x.RegisterSpace);
				}

				size_t rootsig_hash = 0;
				wiHelper::hash_combine(rootsig_hash, internal_state->root_binding_hash);
				wiHelper::hash_combine(rootsig_hash, internal_state->resource_binding_hash);
				wiHelper::hash_combine(rootsig_hash, internal_state->sampler_binding_hash);
				for (auto& x : internal_state->staticsamplers)
				{
					wiHelper::hash_combine(rootsig_hash, x.AddressU);
					wiHelper::hash_combine(rootsig_hash, x.AddressV);
					wiHelper::hash_combine(rootsig_hash, x.AddressW);
					wiHelper::hash_combine(rootsig_hash, x.BorderColor);
					wiHelper::hash_combine(rootsig_hash, x.ComparisonFunc);
					wiHelper::hash_combine(rootsig_hash, x.Filter);
					wiHelper::hash_combine(rootsig_hash, x.MaxAnisotropy);
					wiHelper::hash_combine(rootsig_hash, x.MaxLOD);
					wiHelper::hash_combine(rootsig_hash, x.MinLOD);
					wiHelper::hash_combine(rootsig_hash, x.MipLODBias);
					wiHelper::hash_combine(rootsig_hash, x.RegisterSpace);
					wiHelper::hash_combine(rootsig_hash, x.ShaderRegister);
					wiHelper::hash_combine(rootsig_hash, x.ShaderVisibility);
				}

				rootsignature_cache_mutex.lock();
				if (rootsignature_cache[rootsig_hash])
				{
					internal_state->rootSignature = rootsignature_cache[rootsig_hash];
				}
				else
				{
					D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc = {};
					rootSigDesc.NumStaticSamplers = (UINT)internal_state->staticsamplers.size();
					rootSigDesc.pStaticSamplers = internal_state->staticsamplers.data();
					rootSigDesc.NumParameters = (UINT)params.size();
					rootSigDesc.pParameters = params.data();
					rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

					D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned_rs = {};
					versioned_rs.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
					versioned_rs.Desc_1_1 = rootSigDesc;

					ID3DBlob* rootSigBlob;
					ID3DBlob* rootSigError;
					hr = D3D12SerializeVersionedRootSignature(&versioned_rs, &rootSigBlob, &rootSigError);
					if (FAILED(hr))
					{
						OutputDebugStringA((char*)rootSigError->GetBufferPointer());
						assert(0);
					}
					hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&internal_state->rootSignature));
					assert(SUCCEEDED(hr));
					rootsignature_cache[rootsig_hash] = internal_state->rootSignature;
				}
				rootsignature_cache_mutex.unlock();
			}
		}

		if (stage == CS)
		{
			struct PSO_STREAM
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_CS CS;
			} stream;

			if (pShader->rootSignature == nullptr)
			{
				stream.pRootSignature = internal_state->rootSignature.Get();
			}
			else
			{
				stream.pRootSignature = to_internal(pShader->rootSignature)->resource.Get();
			}
			stream.CS = { internal_state->shadercode.data(), internal_state->shadercode.size() };

			D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
			streamDesc.pPipelineStateSubobjectStream = &stream;
			streamDesc.SizeInBytes = sizeof(stream);

			hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&internal_state->resource));
			assert(SUCCEEDED(hr));
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateSampler(const SamplerDesc* pSamplerDesc, Sampler* pSamplerState)
	{
		auto internal_state = std::make_shared<Sampler_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pSamplerState->internal_state = internal_state;

		D3D12_SAMPLER_DESC desc;
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

		internal_state->descriptor.init(this, desc);

		return true;
	}
	bool GraphicsDevice_DX12::CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery)
	{
		auto internal_state = std::make_shared<Query_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pQuery->internal_state = internal_state;

		pQuery->desc = *pDesc;
		internal_state->query_type = pQuery->desc.Type;

		switch (pDesc->Type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			internal_state->query = allocationhandler->queries_timestamp.allocate();
			break;
		case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			internal_state->query = allocationhandler->queries_occlusion.allocate();
			break;
		}

		return true;
	}
	bool GraphicsDevice_DX12::CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso)
	{
		auto internal_state = std::make_shared<PipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pso->internal_state = internal_state;

		pso->desc = *pDesc;

		pso->hash = 0;
		wiHelper::hash_combine(pso->hash, pDesc->ms);
		wiHelper::hash_combine(pso->hash, pDesc->as);
		wiHelper::hash_combine(pso->hash, pDesc->vs);
		wiHelper::hash_combine(pso->hash, pDesc->ps);
		wiHelper::hash_combine(pso->hash, pDesc->hs);
		wiHelper::hash_combine(pso->hash, pDesc->ds);
		wiHelper::hash_combine(pso->hash, pDesc->gs);
		wiHelper::hash_combine(pso->hash, pDesc->il);
		wiHelper::hash_combine(pso->hash, pDesc->rs);
		wiHelper::hash_combine(pso->hash, pDesc->bs);
		wiHelper::hash_combine(pso->hash, pDesc->dss);
		wiHelper::hash_combine(pso->hash, pDesc->pt);
		wiHelper::hash_combine(pso->hash, pDesc->sampleMask);

		if (pDesc->rootSignature == nullptr)
		{
			// Root signature comes from reflection data when there is no root signature specified:

			auto insert_shader = [&](const Shader* shader)
			{
				if (shader == nullptr)
					return;

				auto shader_internal = to_internal(shader);

				if (shader_internal->resources.empty() && shader_internal->samplers.empty())
					return;

				size_t check_max = internal_state->resources.size(); // dont't check for duplicates within self table
				int b = 0;
				for (auto& x : shader_internal->resources)
				{
					RESOURCEBINDING binding = shader_internal->resource_bindings[b++];

					bool found = false;
					size_t i = 0;
					for (auto& y : internal_state->resources)
					{
						if (x.BaseShaderRegister == y.BaseShaderRegister && x.RangeType == y.RangeType)
						{
							found = true;
							break;
						}
						if (i++ >= check_max)
							break;
					}

					if (!found)
					{
						internal_state->resources.push_back(x);
						internal_state->resource_bindings.push_back(binding);
					}
				}

				check_max = internal_state->samplers.size(); // dont't check for duplicates within self table
				for (auto& x : shader_internal->samplers)
				{
					bool found = false;
					size_t i = 0;
					for (auto& y : internal_state->samplers)
					{
						if (x.BaseShaderRegister == y.BaseShaderRegister && x.RangeType == y.RangeType)
						{
							found = true;
							break;
						}
						if (i++ >= check_max)
							break;
					}

					if (!found)
					{
						internal_state->samplers.push_back(x);
					}
				}

				for (auto& x : shader_internal->staticsamplers)
				{
					internal_state->staticsamplers.push_back(x);
				}
			};

			insert_shader(pDesc->ps); // prioritize ps root descriptor assignment
			insert_shader(pDesc->ms);
			insert_shader(pDesc->as);
			insert_shader(pDesc->vs);
			insert_shader(pDesc->hs);
			insert_shader(pDesc->ds);
			insert_shader(pDesc->gs);

			std::vector<D3D12_ROOT_PARAMETER1> params;

			// Split resources into root descriptors and tables:
			{
				std::vector<D3D12_DESCRIPTOR_RANGE1> resources;
				std::vector<RESOURCEBINDING> bindings;
				int i = 0;
				for (auto& x : internal_state->resources)
				{
					RESOURCEBINDING binding = internal_state->resource_bindings[i++];
					if (x.NumDescriptors == 1 && binding == CONSTANTBUFFER && internal_state->root_cbvs.size() < CONSTANT_BUFFER_AUTO_PLACEMENT_IN_ROOT)
					{
						internal_state->root_cbvs.emplace_back();
						D3D12_ROOT_DESCRIPTOR1& descriptor = internal_state->root_cbvs.back();
						descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
						descriptor.ShaderRegister = x.BaseShaderRegister;
						descriptor.RegisterSpace = x.RegisterSpace;
					}
					else
					{
						resources.push_back(x);
						bindings.push_back(binding);
					}
				}
				internal_state->resources = resources;
				internal_state->resource_bindings = bindings;
			}

			for (auto& x : internal_state->root_cbvs)
			{
				params.emplace_back();
				D3D12_ROOT_PARAMETER1& param = params.back();
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				param.Descriptor = x;
			}

			if (!internal_state->resources.empty())
			{
				internal_state->bindpoint_res = (uint32_t)params.size();
				params.emplace_back();
				D3D12_ROOT_PARAMETER1& param = params.back();
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->resources.size();
				param.DescriptorTable.pDescriptorRanges = internal_state->resources.data();
			}

			if (!internal_state->samplers.empty())
			{
				internal_state->bindpoint_sam = (uint32_t)params.size();
				params.emplace_back();
				D3D12_ROOT_PARAMETER1& param = params.back();
				param = {};
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->samplers.size();
				param.DescriptorTable.pDescriptorRanges = internal_state->samplers.data();
			}

			internal_state->root_binding_hash = 0;
			for (auto& x : internal_state->root_cbvs)
			{
				wiHelper::hash_combine(internal_state->root_binding_hash, x.Flags);
				wiHelper::hash_combine(internal_state->root_binding_hash, x.ShaderRegister);
				wiHelper::hash_combine(internal_state->root_binding_hash, x.RegisterSpace);
			}

			internal_state->resource_binding_hash = 0;
			for (auto& x : internal_state->resources)
			{
				wiHelper::hash_combine(internal_state->resource_binding_hash, x.BaseShaderRegister);
				wiHelper::hash_combine(internal_state->resource_binding_hash, x.NumDescriptors);
				wiHelper::hash_combine(internal_state->resource_binding_hash, x.Flags);
				wiHelper::hash_combine(internal_state->resource_binding_hash, x.OffsetInDescriptorsFromTableStart);
				wiHelper::hash_combine(internal_state->resource_binding_hash, x.RangeType);
				wiHelper::hash_combine(internal_state->resource_binding_hash, x.RegisterSpace);
			}

			internal_state->sampler_binding_hash = 0;
			for (auto& x : internal_state->samplers)
			{
				wiHelper::hash_combine(internal_state->sampler_binding_hash, x.BaseShaderRegister);
				wiHelper::hash_combine(internal_state->sampler_binding_hash, x.NumDescriptors);
				wiHelper::hash_combine(internal_state->sampler_binding_hash, x.Flags);
				wiHelper::hash_combine(internal_state->sampler_binding_hash, x.OffsetInDescriptorsFromTableStart);
				wiHelper::hash_combine(internal_state->sampler_binding_hash, x.RangeType);
				wiHelper::hash_combine(internal_state->sampler_binding_hash, x.RegisterSpace);
			}

			size_t rootsig_hash = 0;
			wiHelper::hash_combine(rootsig_hash, internal_state->root_binding_hash);
			wiHelper::hash_combine(rootsig_hash, internal_state->resource_binding_hash);
			wiHelper::hash_combine(rootsig_hash, internal_state->sampler_binding_hash);
			for (auto& x : internal_state->staticsamplers)
			{
				wiHelper::hash_combine(rootsig_hash, x.AddressU);
				wiHelper::hash_combine(rootsig_hash, x.AddressV);
				wiHelper::hash_combine(rootsig_hash, x.AddressW);
				wiHelper::hash_combine(rootsig_hash, x.BorderColor);
				wiHelper::hash_combine(rootsig_hash, x.ComparisonFunc);
				wiHelper::hash_combine(rootsig_hash, x.Filter);
				wiHelper::hash_combine(rootsig_hash, x.MaxAnisotropy);
				wiHelper::hash_combine(rootsig_hash, x.MaxLOD);
				wiHelper::hash_combine(rootsig_hash, x.MinLOD);
				wiHelper::hash_combine(rootsig_hash, x.MipLODBias);
				wiHelper::hash_combine(rootsig_hash, x.RegisterSpace);
				wiHelper::hash_combine(rootsig_hash, x.ShaderRegister);
				wiHelper::hash_combine(rootsig_hash, x.ShaderVisibility);
			}

			HRESULT hr = S_OK;

			rootsignature_cache_mutex.lock();
			if (rootsignature_cache[rootsig_hash])
			{
				internal_state->rootSignature = rootsignature_cache[rootsig_hash];
			}
			else
			{
				D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc = {};
				rootSigDesc.NumStaticSamplers = (UINT)internal_state->staticsamplers.size();
				rootSigDesc.pStaticSamplers = internal_state->staticsamplers.data();
				rootSigDesc.NumParameters = (UINT)params.size();
				rootSigDesc.pParameters = params.data();
				rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

				D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned_rs = {};
				versioned_rs.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
				versioned_rs.Desc_1_1 = rootSigDesc;

				ID3DBlob* rootSigBlob;
				ID3DBlob* rootSigError;
				hr = D3D12SerializeVersionedRootSignature(&versioned_rs, &rootSigBlob, &rootSigError);
				if (FAILED(hr))
				{
					OutputDebugStringA((char*)rootSigError->GetBufferPointer());
					assert(0);
				}
				hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&internal_state->rootSignature));
				assert(SUCCEEDED(hr));
			}
			rootsignature_cache_mutex.unlock();

			return SUCCEEDED(hr);
		}

		return true;
	}
	bool GraphicsDevice_DX12::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass)
	{
		auto internal_state = std::make_shared<RenderPass_DX12>();
		renderpass->internal_state = internal_state;

		renderpass->desc = *pDesc;

		if (renderpass->desc._flags & RenderPassDesc::FLAG_ALLOW_UAV_WRITES)
		{
			internal_state->flags |= D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
		}

		renderpass->hash = 0;
		wiHelper::hash_combine(renderpass->hash, pDesc->attachments.size());
		int resolve_dst_counter = 0;
		for (auto& attachment : pDesc->attachments)
		{
			if (attachment.type == RenderPassAttachment::RENDERTARGET || attachment.type == RenderPassAttachment::DEPTH_STENCIL)
			{
				wiHelper::hash_combine(renderpass->hash, attachment.texture->desc.Format);
				wiHelper::hash_combine(renderpass->hash, attachment.texture->desc.SampleCount);
			}

			const Texture* texture = attachment.texture;
			int subresource = attachment.subresource;
			auto texture_internal = to_internal(texture);

			D3D12_CLEAR_VALUE clear_value;
			clear_value.Format = _ConvertFormat(texture->desc.Format);

			if (attachment.type == RenderPassAttachment::RENDERTARGET)
			{

				if (subresource < 0 || texture_internal->subresources_rtv.empty())
				{
					internal_state->RTVs[internal_state->rt_count].cpuDescriptor = texture_internal->rtv.handle;
				}
				else
				{
					assert(texture_internal->subresources_rtv.size() > size_t(subresource) && "Invalid RTV subresource!");
					internal_state->RTVs[internal_state->rt_count].cpuDescriptor = texture_internal->subresources_rtv[subresource].handle;
				}

				switch (attachment.loadop)
				{
				default:
				case RenderPassAttachment::LOADOP_LOAD:
					internal_state->RTVs[internal_state->rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::LOADOP_CLEAR:
					internal_state->RTVs[internal_state->rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					clear_value.Color[0] = texture->desc.clear.color[0];
					clear_value.Color[1] = texture->desc.clear.color[1];
					clear_value.Color[2] = texture->desc.clear.color[2];
					clear_value.Color[3] = texture->desc.clear.color[3];
					internal_state->RTVs[internal_state->rt_count].BeginningAccess.Clear.ClearValue = clear_value;
					break;
				case RenderPassAttachment::LOADOP_DONTCARE:
					internal_state->RTVs[internal_state->rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					break;
				}

				switch (attachment.storeop)
				{
				default:
				case RenderPassAttachment::STOREOP_STORE:
					internal_state->RTVs[internal_state->rt_count].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::STOREOP_DONTCARE:
					internal_state->RTVs[internal_state->rt_count].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					break;
				}

				internal_state->rt_count++;
			}
			else if (attachment.type == RenderPassAttachment::DEPTH_STENCIL)
			{
				if (subresource < 0 || texture_internal->subresources_dsv.empty())
				{
					internal_state->DSV.cpuDescriptor = texture_internal->dsv.handle;
				}
				else
				{
					assert(texture_internal->subresources_dsv.size() > size_t(subresource) && "Invalid DSV subresource!");
					internal_state->DSV.cpuDescriptor = texture_internal->subresources_dsv[subresource].handle;
				}

				switch (attachment.loadop)
				{
				default:
				case RenderPassAttachment::LOADOP_LOAD:
					internal_state->DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					internal_state->DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::LOADOP_CLEAR:
					internal_state->DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					internal_state->DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					clear_value.DepthStencil.Depth = texture->desc.clear.depthstencil.depth;
					clear_value.DepthStencil.Stencil = texture->desc.clear.depthstencil.stencil;
					internal_state->DSV.DepthBeginningAccess.Clear.ClearValue = clear_value;
					internal_state->DSV.StencilBeginningAccess.Clear.ClearValue = clear_value;
					break;
				case RenderPassAttachment::LOADOP_DONTCARE:
					internal_state->DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					internal_state->DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					break;
				}

				switch (attachment.storeop)
				{
				default:
				case RenderPassAttachment::STOREOP_STORE:
					internal_state->DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					internal_state->DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::STOREOP_DONTCARE:
					internal_state->DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					internal_state->DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					break;
				}
			}
			else if (attachment.type == RenderPassAttachment::RESOLVE)
			{
				if (texture != nullptr)
				{
					int resolve_src_counter = 0;
					for (auto& src : renderpass->desc.attachments)
					{
						if (src.type == RenderPassAttachment::RENDERTARGET && src.texture != nullptr)
						{
							if (resolve_src_counter == resolve_dst_counter)
							{
								auto src_internal = to_internal(src.texture);

								D3D12_RENDER_PASS_RENDER_TARGET_DESC& src_RTV = internal_state->RTVs[resolve_src_counter];
								src_RTV.EndingAccess.Resolve.PreserveResolveSource = src_RTV.EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
								src_RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
								src_RTV.EndingAccess.Resolve.Format = clear_value.Format;
								src_RTV.EndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE_AVERAGE;
								src_RTV.EndingAccess.Resolve.SubresourceCount = 1;
								src_RTV.EndingAccess.Resolve.pDstResource = texture_internal->resource.Get();
								src_RTV.EndingAccess.Resolve.pSrcResource = src_internal->resource.Get();

								src_RTV.EndingAccess.Resolve.pSubresourceParameters = &internal_state->resolve_subresources[resolve_src_counter];
								internal_state->resolve_subresources[resolve_src_counter].SrcRect.left = 0;
								internal_state->resolve_subresources[resolve_src_counter].SrcRect.right = (LONG)texture->desc.Width;
								internal_state->resolve_subresources[resolve_src_counter].SrcRect.bottom = (LONG)texture->desc.Height;
								internal_state->resolve_subresources[resolve_src_counter].SrcRect.top = 0;

								break;
							}
							resolve_src_counter++;
						}
					}
				}
				resolve_dst_counter++;
			}
			else if (attachment.type == RenderPassAttachment::SHADING_RATE_SOURCE)
			{
				internal_state->shading_rate_image = texture;
			}
		}


		// Beginning barriers:
		for (auto& attachment : renderpass->desc.attachments)
		{
			if (attachment.texture == nullptr)
				continue;

			auto texture_internal = to_internal(attachment.texture);

			D3D12_RESOURCE_BARRIER& barrierdesc = internal_state->barrierdescs_begin[internal_state->num_barriers_begin++];

			barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrierdesc.Transition.pResource = texture_internal->resource.Get();
			barrierdesc.Transition.StateBefore = _ConvertImageLayout(attachment.initial_layout);
			if (attachment.type == RenderPassAttachment::RESOLVE)
			{
				barrierdesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_DEST;
			}
			else
			{
				barrierdesc.Transition.StateAfter = _ConvertImageLayout(attachment.subpass_layout);
			}
			barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			if (barrierdesc.Transition.StateBefore == barrierdesc.Transition.StateAfter)
			{
				internal_state->num_barriers_begin--;
				continue;
			}
		}

		// Ending barriers:
		for (auto& attachment : renderpass->desc.attachments)
		{
			if (attachment.texture == nullptr)
				continue;

			auto texture_internal = to_internal(attachment.texture);

			D3D12_RESOURCE_BARRIER& barrierdesc = internal_state->barrierdescs_end[internal_state->num_barriers_end++];

			barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrierdesc.Transition.pResource = texture_internal->resource.Get();
			if (attachment.type == RenderPassAttachment::RESOLVE)
			{
				barrierdesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_DEST;
			}
			else
			{
				barrierdesc.Transition.StateBefore = _ConvertImageLayout(attachment.subpass_layout);
			}
			barrierdesc.Transition.StateAfter = _ConvertImageLayout(attachment.final_layout);
			barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			if (barrierdesc.Transition.StateBefore == barrierdesc.Transition.StateAfter)
			{
				internal_state->num_barriers_end--;
				continue;
			}
		}

		return true;
	}
	bool GraphicsDevice_DX12::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh)
	{
		auto internal_state = std::make_shared<BVH_DX12>();
		internal_state->allocationhandler = allocationhandler;
		bvh->internal_state = internal_state;
		bvh->type = GPUResource::GPU_RESOURCE_TYPE::RAYTRACING_ACCELERATION_STRUCTURE;

		bvh->desc = *pDesc;

		if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		}
		if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_COMPACTION)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
		}
		if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		}
		if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		}
		if (pDesc->_flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
		}
		

		switch (pDesc->type)
		{
		case RaytracingAccelerationStructureDesc::BOTTOMLEVEL:
		{
			internal_state->desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			internal_state->desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

			for (auto& x : pDesc->bottomlevel.geometries)
			{
				internal_state->geometries.emplace_back();
				auto& geometry = internal_state->geometries.back();
				geometry = {};

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES)
				{
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					geometry.Triangles.VertexBuffer.StartAddress = to_internal(&x.triangles.vertexBuffer)->gpu_address + (D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.vertexByteOffset;
					geometry.Triangles.VertexBuffer.StrideInBytes = (UINT64)x.triangles.vertexStride;
					geometry.Triangles.VertexCount = x.triangles.vertexCount;
					geometry.Triangles.VertexFormat = _ConvertFormat(x.triangles.vertexFormat);
					geometry.Triangles.IndexFormat = (x.triangles.indexFormat == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
					geometry.Triangles.IndexBuffer = to_internal(&x.triangles.indexBuffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.indexOffset * (x.triangles.indexFormat == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? sizeof(uint16_t) : sizeof(uint32_t));
					geometry.Triangles.IndexCount = x.triangles.indexCount;

					if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.Triangles.Transform3x4 = to_internal(&x.triangles.transform3x4Buffer)->gpu_address +
							(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.transform3x4BufferOffset;
					}
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
				{
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS; 
					geometry.AABBs.AABBs.StartAddress = to_internal(&x.aabbs.aabbBuffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.aabbs.offset;
					geometry.AABBs.AABBs.StrideInBytes = (UINT64)x.aabbs.stride;
					geometry.AABBs.AABBCount = x.aabbs.count;
				}
			}

			internal_state->desc.pGeometryDescs = internal_state->geometries.data();
			internal_state->desc.NumDescs = (UINT)internal_state->geometries.size();
		}
		break;
		case RaytracingAccelerationStructureDesc::TOPLEVEL:
		{
			internal_state->desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
			internal_state->desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

			internal_state->desc.InstanceDescs = to_internal(&pDesc->toplevel.instanceBuffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)pDesc->toplevel.offset;
			internal_state->desc.NumDescs = (UINT)pDesc->toplevel.count;
		}
		break;
		}

		device->GetRaytracingAccelerationStructurePrebuildInfo(&internal_state->desc, &internal_state->info);


		size_t alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
		size_t alignedSize = Align((size_t)internal_state->info.ResultDataMaxSizeInBytes, alignment);

		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Width = (UINT64)alignedSize;
		desc.Height = 1;
		desc.MipLevels = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.DepthOrArraySize = 1;
		desc.Alignment = 0;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		HRESULT hr = allocationhandler->allocator->CreateResource(
			&allocationDesc,
			&desc,
			resourceState,
			nullptr,
			&internal_state->allocation,
			IID_PPV_ARGS(&internal_state->resource)
		);
		assert(SUCCEEDED(hr));

		internal_state->gpu_address = internal_state->resource->GetGPUVirtualAddress();

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		srv_desc.RaytracingAccelerationStructure.Location = internal_state->gpu_address;

		internal_state->srv.init(this, srv_desc, nullptr);

		GPUBufferDesc scratch_desc;
		scratch_desc.ByteWidth = (uint32_t)std::max(internal_state->info.ScratchDataSizeInBytes, internal_state->info.UpdateScratchDataSizeInBytes);

		return CreateBuffer(&scratch_desc, nullptr, &internal_state->scratch);
	}
	bool GraphicsDevice_DX12::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso)
	{
		auto internal_state = std::make_shared<RTPipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		rtpso->internal_state = internal_state;

		rtpso->desc = *pDesc;

		D3D12_STATE_OBJECT_DESC desc = {};
		desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

		std::vector<D3D12_STATE_SUBOBJECT> subobjects;

		D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config = {};
		{
			subobjects.emplace_back();
			auto& subobject = subobjects.back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
			pipeline_config.MaxTraceRecursionDepth = pDesc->max_trace_recursion_depth;
			subobject.pDesc = &pipeline_config;
		}

		D3D12_RAYTRACING_SHADER_CONFIG shader_config = {};
		{
			subobjects.emplace_back();
			auto& subobject = subobjects.back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
			shader_config.MaxAttributeSizeInBytes = pDesc->max_attribute_size_in_bytes;
			shader_config.MaxPayloadSizeInBytes = pDesc->max_payload_size_in_bytes;
			subobject.pDesc = &shader_config;
		}

		D3D12_GLOBAL_ROOT_SIGNATURE global_rootsig = {};
		{
			subobjects.emplace_back();
			auto& subobject = subobjects.back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
			if (pDesc->rootSignature == nullptr)
			{
				auto shader_internal = to_internal(pDesc->shaderlibraries.front().shader); // think better way
				global_rootsig.pGlobalRootSignature = shader_internal->rootSignature.Get();
			}
			else
			{
				global_rootsig.pGlobalRootSignature = to_internal(pDesc->rootSignature)->resource.Get();
			}
			subobject.pDesc = &global_rootsig;
		}

		internal_state->exports.reserve(pDesc->shaderlibraries.size());
		internal_state->library_descs.reserve(pDesc->shaderlibraries.size());
		for(auto& x : pDesc->shaderlibraries)
		{
			subobjects.emplace_back();
			auto& subobject = subobjects.back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
			internal_state->library_descs.emplace_back();
			auto& library_desc = internal_state->library_descs.back();
			library_desc = {};
			auto shader_internal = to_internal(x.shader);
			library_desc.DXILLibrary.pShaderBytecode = shader_internal->shadercode.data();
			library_desc.DXILLibrary.BytecodeLength = shader_internal->shadercode.size();
			library_desc.NumExports = 1;

			internal_state->exports.emplace_back();
			D3D12_EXPORT_DESC& export_desc = internal_state->exports.back();
			internal_state->export_strings.emplace_back();
			wiHelper::StringConvert(x.function_name, internal_state->export_strings.back());
			export_desc.Name = internal_state->export_strings.back().c_str();
			library_desc.pExports = &export_desc;

			subobject.pDesc = &library_desc;
		}

		internal_state->hitgroup_descs.reserve(pDesc->hitgroups.size());
		for (auto& x : pDesc->hitgroups)
		{
			internal_state->group_strings.emplace_back();
			wiHelper::StringConvert(x.name, internal_state->group_strings.back());

			if (x.type == ShaderHitGroup::GENERAL)
				continue;
			subobjects.emplace_back();
			auto& subobject = subobjects.back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			internal_state->hitgroup_descs.emplace_back();
			auto& hitgroup_desc = internal_state->hitgroup_descs.back();
			hitgroup_desc = {};
			switch (x.type)
			{
			default:
			case ShaderHitGroup::TRIANGLES:
				hitgroup_desc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
				break;
			case ShaderHitGroup::PROCEDURAL:
				hitgroup_desc.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
				break;
			}
			if (!x.name.empty())
			{
				hitgroup_desc.HitGroupExport = internal_state->group_strings.back().c_str();
			}
			if (x.closesthit_shader != ~0)
			{
				hitgroup_desc.ClosestHitShaderImport = internal_state->exports[x.closesthit_shader].Name;
			}
			if (x.anyhit_shader != ~0)
			{
				hitgroup_desc.AnyHitShaderImport = internal_state->exports[x.anyhit_shader].Name;
			}
			if (x.intersection_shader != ~0)
			{
				hitgroup_desc.IntersectionShaderImport = internal_state->exports[x.intersection_shader].Name;
			}
			subobject.pDesc = &hitgroup_desc;
		}

		desc.NumSubobjects = (UINT)subobjects.size();
		desc.pSubobjects = subobjects.data();

		HRESULT hr = device->CreateStateObject(&desc, IID_PPV_ARGS(&internal_state->resource));
		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateDescriptorTable(DescriptorTable* table)
	{
		auto internal_state = std::make_shared<DescriptorTable_DX12>();
		internal_state->allocationhandler = allocationhandler;
		table->internal_state = internal_state;

		internal_state->resource_heap.desc.NodeMask = 0;
		internal_state->resource_heap.desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		internal_state->resource_heap.desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		size_t prefix_sum = 0;
		for (auto& x : table->resources)
		{
			if (x.binding < CONSTANTBUFFER)
			{
				internal_state->resource_heap.write_remap.push_back(prefix_sum);
				continue;
			}

			internal_state->resource_heap.ranges.emplace_back();
			auto& range = internal_state->resource_heap.ranges.back();
			range = {};
			range.BaseShaderRegister = x.slot;
			range.NumDescriptors = x.count;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			range.RegisterSpace = 0; // this will be filled by root signature depending on the table position (to mirror Vulkan behaviour)
			internal_state->resource_heap.desc.NumDescriptors += range.NumDescriptors;

			switch (x.binding)
			{
			case CONSTANTBUFFER:
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
				break;
			case RAWBUFFER:
			case STRUCTUREDBUFFER:
			case TYPEDBUFFER:
			case TEXTURE1D:
			case TEXTURE1DARRAY:
			case TEXTURE2D:
			case TEXTURE2DARRAY:
			case TEXTURECUBE:
			case TEXTURECUBEARRAY:
			case TEXTURE3D:
			case ACCELERATIONSTRUCTURE:
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				break;
			case RWRAWBUFFER:
			case RWSTRUCTUREDBUFFER:
			case RWTYPEDBUFFER:
			case RWTEXTURE1D:
			case RWTEXTURE1DARRAY:
			case RWTEXTURE2D:
			case RWTEXTURE2DARRAY:
			case RWTEXTURE3D:
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				break;
			default:
				assert(0);
				break;
			}

			internal_state->resource_heap.write_remap.push_back(prefix_sum);
			prefix_sum += (size_t)range.NumDescriptors;
		}

		internal_state->sampler_heap.desc.NodeMask = 0;
		internal_state->sampler_heap.desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		internal_state->sampler_heap.desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

		prefix_sum = 0;
		for (auto& x : table->samplers)
		{
			internal_state->sampler_heap.ranges.emplace_back();
			auto& range = internal_state->sampler_heap.ranges.back();
			range = {};
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			range.BaseShaderRegister = x.slot;
			range.NumDescriptors = x.count;
			range.RegisterSpace = 0; // this will be filled by root signature depending on the table position (to mirror Vulkan behaviour)
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			internal_state->sampler_heap.desc.NumDescriptors += range.NumDescriptors;

			internal_state->sampler_heap.write_remap.push_back(prefix_sum);
			prefix_sum += (size_t)range.NumDescriptors;
		}

		for (auto& x : table->staticsamplers)
		{
			internal_state->staticsamplers.push_back(_ConvertStaticSampler(x));
		}

		HRESULT hr = S_OK;

		if (internal_state->resource_heap.desc.NumDescriptors > 0)
		{
			hr = device->CreateDescriptorHeap(&internal_state->resource_heap.desc, IID_PPV_ARGS(&internal_state->resource_heap.heap));
			assert(SUCCEEDED(hr));
			internal_state->resource_heap.address = internal_state->resource_heap.heap->GetCPUDescriptorHandleForHeapStart();

			uint32_t slot = 0;
			for (auto& x : table->resources)
			{
				for (uint32_t i = 0; i < x.count; ++i)
				{
					WriteDescriptor(table, slot, i, (const GPUResource*)nullptr);
				}
				slot++;
			}
		}
		if (internal_state->sampler_heap.desc.NumDescriptors > 0)
		{
			hr = device->CreateDescriptorHeap(&internal_state->sampler_heap.desc, IID_PPV_ARGS(&internal_state->sampler_heap.heap));
			assert(SUCCEEDED(hr));
			internal_state->sampler_heap.address = internal_state->sampler_heap.heap->GetCPUDescriptorHandleForHeapStart();

			uint32_t slot = 0;
			for (auto& x : table->samplers)
			{
				for (uint32_t i = 0; i < x.count; ++i)
				{
					WriteDescriptor(table, slot, i, (const Sampler*)nullptr);
				}
				slot++;
			}
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateRootSignature(RootSignature* rootsig)
	{
		auto internal_state = std::make_shared<RootSignature_DX12>();
		internal_state->allocationhandler = allocationhandler;
		rootsig->internal_state = internal_state;

		internal_state->params.reserve(rootsig->tables.size());
		std::vector<D3D12_STATIC_SAMPLER_DESC> staticsamplers;

		std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> table_ranges_resource;
		std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> table_ranges_sampler;
		table_ranges_resource.reserve(rootsig->tables.size());
		table_ranges_sampler.reserve(rootsig->tables.size());

		uint32_t space = 0;
		for (auto& x : rootsig->tables)
		{
			table_ranges_resource.emplace_back();
			table_ranges_sampler.emplace_back();

			uint32_t rangeIndex = 0;
			for (auto& binding : x.resources)
			{
				if (binding.binding < CONSTANTBUFFER)
				{
					assert(binding.count == 1); // descriptor array not allowed in the root
					internal_state->root_remap.emplace_back();
					internal_state->root_remap.back().space = space; // Space assignment for Root Signature
					internal_state->root_remap.back().rangeIndex = rangeIndex;

					internal_state->params.emplace_back();
					auto& param = internal_state->params.back();
					switch (binding.binding)
					{
					case ROOT_CONSTANTBUFFER:
						param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
						break;
					case ROOT_RAWBUFFER:
					case ROOT_STRUCTUREDBUFFER:
						param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
						break;
					case ROOT_RWRAWBUFFER:
					case ROOT_RWSTRUCTUREDBUFFER:
						param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
						break;
					default:
						break;
					}
					param.ShaderVisibility = _ConvertShaderVisibility(x.stage);
					param.Descriptor.RegisterSpace = space;
					param.Descriptor.ShaderRegister = binding.slot;
				}
				else
				{
					// Space assignment for Root Signature:
					table_ranges_resource.back().emplace_back();
					D3D12_DESCRIPTOR_RANGE1& range = table_ranges_resource.back().back();
					auto table_internal = to_internal(&x);
					range = table_internal->resource_heap.ranges[rangeIndex];
					range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
					range.Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
					range.Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
					range.RegisterSpace = space;
				}
				rangeIndex++;
			}
			for (auto& binding : x.samplers)
			{
				// Space assignment for Root Signature:
				table_ranges_sampler.back().emplace_back();
				D3D12_DESCRIPTOR_RANGE1& range = table_ranges_sampler.back().back();
				auto table_internal = to_internal(&x);
				range = table_internal->sampler_heap.ranges[rangeIndex];
				range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
				range.RegisterSpace = space;
			}
			space++;
		}

		space = 0;
		uint32_t bind_point = (uint32_t)internal_state->params.size();
		for (auto& x : rootsig->tables)
		{
			auto table_internal = to_internal(&x);

			if (table_internal->resource_heap.desc.NumDescriptors == 0 &&
				table_internal->sampler_heap.desc.NumDescriptors == 0)
			{
				// No real bind point
				internal_state->table_bind_point_remap.push_back(-1);
			}
			else
			{
				internal_state->table_bind_point_remap.push_back((int)bind_point);
			}

			if (table_internal->resource_heap.desc.NumDescriptors > 0)
			{
				internal_state->params.emplace_back();
				auto& param = internal_state->params.back();
				param = {};
				param.ShaderVisibility = _ConvertShaderVisibility(x.stage);
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.DescriptorTable.pDescriptorRanges = table_ranges_resource[space].data();
				param.DescriptorTable.NumDescriptorRanges = (UINT)table_ranges_resource[space].size();
				bind_point++;
			}
			if (table_internal->sampler_heap.desc.NumDescriptors > 0)
			{
				internal_state->params.emplace_back();
				auto& param = internal_state->params.back();
				param = {};
				param.ShaderVisibility = _ConvertShaderVisibility(x.stage);
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.DescriptorTable.pDescriptorRanges = table_ranges_sampler[space].data();
				param.DescriptorTable.NumDescriptorRanges = (UINT)table_ranges_sampler[space].size();
				bind_point++;
			}

			std::vector<D3D12_STATIC_SAMPLER_DESC> tmp_staticsamplers(table_internal->staticsamplers.begin(), table_internal->staticsamplers.end());
			for (auto& sam : tmp_staticsamplers)
			{
				// Space assignment for Root Signature:
				sam.RegisterSpace = space;
			}
			staticsamplers.insert(
				staticsamplers.end(),
				tmp_staticsamplers.begin(),
				tmp_staticsamplers.end()
			);

			space++;
		}

		internal_state->root_constant_bind_remap = bind_point;
		for (auto& x : rootsig->rootconstants)
		{
			internal_state->params.emplace_back();
			auto& param = internal_state->params.back();
			param = {};
			param.ShaderVisibility = _ConvertShaderVisibility(x.stage);
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			param.Constants.ShaderRegister = x.slot;
			param.Constants.RegisterSpace = 0;
			param.Constants.Num32BitValues = x.size / sizeof(uint32_t);
		}

		D3D12_ROOT_SIGNATURE_DESC1 desc = {};
		if (rootsig->_flags & RootSignature::FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
		{
			desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		}
		desc.NumParameters = (UINT)internal_state->params.size();
		desc.pParameters = internal_state->params.data();
		desc.NumStaticSamplers = (UINT)staticsamplers.size();
		desc.pStaticSamplers = staticsamplers.data();

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned_rs = {};
		versioned_rs.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versioned_rs.Desc_1_1 = desc;

		ID3DBlob* rootSigBlob;
		ID3DBlob* rootSigError;
		HRESULT hr = D3D12SerializeVersionedRootSignature(&versioned_rs, &rootSigBlob, &rootSigError);
		if (FAILED(hr))
		{
			OutputDebugStringA((char*)rootSigError->GetBufferPointer());
			assert(0);
		}
		hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&internal_state->resource));
		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}

	int GraphicsDevice_DX12::CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
	{
		auto internal_state = to_internal(texture);

		switch (type)
		{
		case wiGraphics::SRV:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			// Try to resolve resource format:
			switch (texture->desc.Format)
			{
			case FORMAT_R16_TYPELESS:
				srv_desc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			case FORMAT_R32_TYPELESS:
				srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case FORMAT_R24G8_TYPELESS:
				srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case FORMAT_R32G8X24_TYPELESS:
				srv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			default:
				srv_desc.Format = _ConvertFormat(texture->desc.Format);
				break;
			}

			if (texture->desc.type == TextureDesc::TEXTURE_1D)
			{
				if (texture->desc.ArraySize > 1)
				{
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
					srv_desc.Texture1DArray.FirstArraySlice = firstSlice;
					srv_desc.Texture1DArray.ArraySize = sliceCount;
					srv_desc.Texture1DArray.MostDetailedMip = firstMip;
					srv_desc.Texture1DArray.MipLevels = mipCount;
				}
				else
				{
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
					srv_desc.Texture1D.MostDetailedMip = firstMip;
					srv_desc.Texture1D.MipLevels = mipCount;
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_2D)
			{
				if (texture->desc.ArraySize > 1)
				{
					if (texture->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
					{
						if (texture->desc.ArraySize > 6)
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
							srv_desc.TextureCubeArray.First2DArrayFace = firstSlice;
							srv_desc.TextureCubeArray.NumCubes = std::min(texture->desc.ArraySize, sliceCount) / 6;
							srv_desc.TextureCubeArray.MostDetailedMip = firstMip;
							srv_desc.TextureCubeArray.MipLevels = mipCount;
						}
						else
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
							srv_desc.TextureCube.MostDetailedMip = firstMip;
							srv_desc.TextureCube.MipLevels = mipCount;
						}
					}
					else
					{
						if (texture->desc.SampleCount > 1)
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
							srv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
							srv_desc.Texture2DMSArray.ArraySize = sliceCount;
						}
						else
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
							srv_desc.Texture2DArray.FirstArraySlice = firstSlice;
							srv_desc.Texture2DArray.ArraySize = sliceCount;
							srv_desc.Texture2DArray.MostDetailedMip = firstMip;
							srv_desc.Texture2DArray.MipLevels = mipCount;
						}
					}
				}
				else
				{
					if (texture->desc.SampleCount > 1)
					{
						srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						srv_desc.Texture2D.MostDetailedMip = firstMip;
						srv_desc.Texture2D.MipLevels = mipCount;
					}
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_3D)
			{
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				srv_desc.Texture3D.MostDetailedMip = firstMip;
				srv_desc.Texture3D.MipLevels = mipCount;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, srv_desc, internal_state->resource.Get());

			if (!internal_state->srv.IsValid())
			{
				internal_state->srv = descriptor;
				return -1;
			}
			internal_state->subresources_srv.push_back(descriptor);
			return int(internal_state->subresources_srv.size() - 1);
		}
		break;
		case wiGraphics::UAV:
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};

			// Try to resolve resource format:
			switch (texture->desc.Format)
			{
			case FORMAT_R16_TYPELESS:
				uav_desc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			case FORMAT_R32_TYPELESS:
				uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case FORMAT_R24G8_TYPELESS:
				uav_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case FORMAT_R32G8X24_TYPELESS:
				uav_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			default:
				uav_desc.Format = _ConvertFormat(texture->desc.Format);
				break;
			}

			if (texture->desc.type == TextureDesc::TEXTURE_1D)
			{
				if (texture->desc.ArraySize > 1)
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
					uav_desc.Texture1DArray.FirstArraySlice = firstSlice;
					uav_desc.Texture1DArray.ArraySize = sliceCount;
					uav_desc.Texture1DArray.MipSlice = firstMip;
				}
				else
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
					uav_desc.Texture1D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_2D)
			{
				if (texture->desc.ArraySize > 1)
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					uav_desc.Texture2DArray.FirstArraySlice = firstSlice;
					uav_desc.Texture2DArray.ArraySize = sliceCount;
					uav_desc.Texture2DArray.MipSlice = firstMip;
				}
				else
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					uav_desc.Texture2D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_3D)
			{
				uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				uav_desc.Texture3D.MipSlice = firstMip;
				uav_desc.Texture3D.FirstWSlice = 0;
				uav_desc.Texture3D.WSize = -1;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, uav_desc, internal_state->resource.Get());

			if (!internal_state->uav.IsValid())
			{
				internal_state->uav = descriptor;
				return -1;
			}
			internal_state->subresources_uav.push_back(descriptor);
			return int(internal_state->subresources_uav.size() - 1);
		}
		break;
		case wiGraphics::RTV:
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};

			// Try to resolve resource format:
			switch (texture->desc.Format)
			{
			case FORMAT_R16_TYPELESS:
				rtv_desc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			case FORMAT_R32_TYPELESS:
				rtv_desc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case FORMAT_R24G8_TYPELESS:
				rtv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case FORMAT_R32G8X24_TYPELESS:
				rtv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			default:
				rtv_desc.Format = _ConvertFormat(texture->desc.Format);
				break;
			}

			if (texture->desc.type == TextureDesc::TEXTURE_1D)
			{
				if (texture->desc.ArraySize > 1)
				{
					rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
					rtv_desc.Texture1DArray.FirstArraySlice = firstSlice;
					rtv_desc.Texture1DArray.ArraySize = sliceCount;
					rtv_desc.Texture1DArray.MipSlice = firstMip;
				}
				else
				{
					rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
					rtv_desc.Texture1D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_2D)
			{
				if (texture->desc.ArraySize > 1)
				{
					if (texture->desc.SampleCount > 1)
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
						rtv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
						rtv_desc.Texture2DMSArray.ArraySize = sliceCount;
					}
					else
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
						rtv_desc.Texture2DArray.FirstArraySlice = firstSlice;
						rtv_desc.Texture2DArray.ArraySize = sliceCount;
						rtv_desc.Texture2DArray.MipSlice = firstMip;
					}
				}
				else
				{
					if (texture->desc.SampleCount > 1)
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
						rtv_desc.Texture2D.MipSlice = firstMip;
					}
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_3D)
			{
				rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
				rtv_desc.Texture3D.MipSlice = firstMip;
				rtv_desc.Texture3D.FirstWSlice = 0;
				rtv_desc.Texture3D.WSize = -1;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, rtv_desc, internal_state->resource.Get());

			if (!internal_state->rtv.IsValid())
			{
				internal_state->rtv = descriptor;
				return -1;
			}
			internal_state->subresources_rtv.push_back(descriptor);
			return int(internal_state->subresources_rtv.size() - 1);
		}
		break;
		case wiGraphics::DSV:
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};

			// Try to resolve resource format:
			switch (texture->desc.Format)
			{
			case FORMAT_R16_TYPELESS:
				dsv_desc.Format = DXGI_FORMAT_D16_UNORM;
				break;
			case FORMAT_R32_TYPELESS:
				dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
				break;
			case FORMAT_R24G8_TYPELESS:
				dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;
			case FORMAT_R32G8X24_TYPELESS:
				dsv_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
				break;
			default:
				dsv_desc.Format = _ConvertFormat(texture->desc.Format);
				break;
			}

			if (texture->desc.type == TextureDesc::TEXTURE_1D)
			{
				if (texture->desc.ArraySize > 1)
				{
					dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
					dsv_desc.Texture1DArray.FirstArraySlice = firstSlice;
					dsv_desc.Texture1DArray.ArraySize = sliceCount;
					dsv_desc.Texture1DArray.MipSlice = firstMip;
				}
				else
				{
					dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
					dsv_desc.Texture1D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_2D)
			{
				if (texture->desc.ArraySize > 1)
				{
					if (texture->desc.SampleCount > 1)
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
						dsv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
						dsv_desc.Texture2DMSArray.ArraySize = sliceCount;
					}
					else
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
						dsv_desc.Texture2DArray.FirstArraySlice = firstSlice;
						dsv_desc.Texture2DArray.ArraySize = sliceCount;
						dsv_desc.Texture2DArray.MipSlice = firstMip;
					}
				}
				else
				{
					if (texture->desc.SampleCount > 1)
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
						dsv_desc.Texture2D.MipSlice = firstMip;
					}
				}
			}

			SingleDescriptor descriptor;
			descriptor.init(this, dsv_desc, internal_state->resource.Get());

			if (!internal_state->dsv.IsValid())
			{
				internal_state->dsv = descriptor;
				return -1;
			}
			internal_state->subresources_dsv.push_back(descriptor);
			return int(internal_state->subresources_dsv.size() - 1);
		}
		break;
		default:
			break;
		}
		return -1;
	}
	int GraphicsDevice_DX12::CreateSubresource(GPUBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size)
	{
		auto internal_state = to_internal(buffer);
		const GPUBufferDesc& desc = buffer->GetDesc();

		switch (type)
		{
		case wiGraphics::SRV:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			if (desc.MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer
				srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
				srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
				srv_desc.Buffer.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / sizeof(uint32_t);
			}
			else if (desc.MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer
				srv_desc.Format = DXGI_FORMAT_UNKNOWN;
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = (UINT)offset / desc.StructureByteStride;
				srv_desc.Buffer.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / desc.StructureByteStride;
				srv_desc.Buffer.StructureByteStride = desc.StructureByteStride;
			}
			else
			{
				// This is a Typed Buffer
				uint32_t stride = GetFormatStride(desc.Format);
				srv_desc.Format = _ConvertFormat(desc.Format);
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = offset / stride;
				srv_desc.Buffer.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / stride;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, srv_desc, internal_state->resource.Get());

			if (!internal_state->srv.IsValid())
			{
				internal_state->srv = descriptor;
				return -1;
			}
			internal_state->subresources_srv.push_back(descriptor);
			return int(internal_state->subresources_srv.size() - 1);
		}
		break;
		case wiGraphics::UAV:
		{

			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;

			if (desc.MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer
				uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
				uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
				uav_desc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
				uav_desc.Buffer.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / sizeof(uint32_t);
			}
			else if (desc.MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer
				uav_desc.Format = DXGI_FORMAT_UNKNOWN;
				uav_desc.Buffer.FirstElement = (UINT)offset / desc.StructureByteStride;
				uav_desc.Buffer.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / desc.StructureByteStride;
				uav_desc.Buffer.StructureByteStride = desc.StructureByteStride;
			}
			else
			{
				// This is a Typed Buffer
				uint32_t stride = GetFormatStride(desc.Format);
				uav_desc.Format = _ConvertFormat(desc.Format);
				uav_desc.Buffer.FirstElement = (UINT)offset / stride;
				uav_desc.Buffer.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / stride;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, uav_desc, internal_state->resource.Get());

			if (!internal_state->uav.IsValid())
			{
				internal_state->uav = descriptor;
				return -1;
			}
			internal_state->subresources_uav.push_back(descriptor);
			return int(internal_state->subresources_uav.size() - 1);
		}
		break;
		default:
			assert(0);
			break;
		}

		return -1;
	}

	void GraphicsDevice_DX12::WriteShadingRateValue(SHADING_RATE rate, void* dest)
	{
		D3D12_SHADING_RATE _rate = _ConvertShadingRate(rate);
		if (!features_6.AdditionalShadingRatesSupported)
		{
			_rate = std::min(_rate, D3D12_SHADING_RATE_2X2);
		}
		*(uint8_t*)dest = _rate;
	}
	void GraphicsDevice_DX12::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest)
	{
		D3D12_RAYTRACING_INSTANCE_DESC* desc = (D3D12_RAYTRACING_INSTANCE_DESC*)dest;
		desc->AccelerationStructure = to_internal(&instance->bottomlevel)->gpu_address;
		memcpy(desc->Transform, &instance->transform, sizeof(desc->Transform));
		desc->InstanceID = instance->InstanceID;
		desc->InstanceMask = instance->InstanceMask;
		desc->InstanceContributionToHitGroupIndex = instance->InstanceContributionToHitGroupIndex;
		desc->Flags = instance->Flags;
	}
	void GraphicsDevice_DX12::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest)
	{
		auto internal_state = to_internal(rtpso);

		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		HRESULT hr = internal_state->resource.As(&stateObjectProperties);
		assert(SUCCEEDED(hr));

		void* identifier = stateObjectProperties->GetShaderIdentifier(internal_state->group_strings[group_index].c_str());
		memcpy(dest, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}
	void GraphicsDevice_DX12::WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const GPUResource* resource, int subresource, uint64_t offset)
	{
		auto table_internal = to_internal(table);
		D3D12_CPU_DESCRIPTOR_HANDLE dst = table_internal->resource_heap.address;
		size_t remap = table_internal->resource_heap.write_remap[rangeIndex];
		dst.ptr += (remap + arrayIndex) * (size_t)resource_descriptor_size;

		RESOURCEBINDING binding = table->resources[rangeIndex].binding;
		switch (binding)
		{
		case CONSTANTBUFFER:
			if (resource == nullptr || !resource->IsValid())
			{
				device->CopyDescriptorsSimple(1, dst, nullCBV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
			else if (resource->IsBuffer())
			{
				const GPUBuffer* buffer = (const GPUBuffer*)resource;
				auto internal_state = to_internal(buffer);
				if (buffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
				{
					if (offset > 0)
					{
						D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = internal_state->cbv.cbv;
						cbv.BufferLocation += offset;
						device->CreateConstantBufferView(&cbv, dst);
					}
					else
					{
						device->CopyDescriptorsSimple(1, dst, internal_state->cbv.handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					}
				}
			}
			break;
		case RAWBUFFER:
		case STRUCTUREDBUFFER:
		case TYPEDBUFFER:
		case TEXTURE1D:
		case TEXTURE1DARRAY:
		case TEXTURE2D:
		case TEXTURE2DARRAY:
		case TEXTURECUBE:
		case TEXTURECUBEARRAY:
		case TEXTURE3D:
		case ACCELERATIONSTRUCTURE:
			if (resource == nullptr || !resource->IsValid())
			{
				switch (binding)
				{
				case RAWBUFFER:
				case STRUCTUREDBUFFER:
				case TYPEDBUFFER:
					device->CopyDescriptorsSimple(1, dst, nullSRV_buffer, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case TEXTURE1D:
					device->CopyDescriptorsSimple(1, dst, nullSRV_texture1d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case TEXTURE1DARRAY:
					device->CopyDescriptorsSimple(1, dst, nullSRV_texture1darray, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case TEXTURE2D:
					device->CopyDescriptorsSimple(1, dst, nullSRV_texture2d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case TEXTURE2DARRAY:
					device->CopyDescriptorsSimple(1, dst, nullSRV_texture2darray, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case TEXTURECUBE:
					device->CopyDescriptorsSimple(1, dst, nullSRV_texturecube, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case TEXTURECUBEARRAY:
					device->CopyDescriptorsSimple(1, dst, nullSRV_texturecubearray, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case TEXTURE3D:
					device->CopyDescriptorsSimple(1, dst, nullSRV_texture3d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case ACCELERATIONSTRUCTURE:
					device->CopyDescriptorsSimple(1, dst, nullSRV_accelerationstructure, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				default:
					assert(0);
					break;
				}
			}
			else if (resource->IsTexture())
			{
				auto internal_state = to_internal((const Texture*)resource);
				if (subresource < 0)
				{
					device->CopyDescriptorsSimple(1, dst, internal_state->srv.handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
				else
				{
					device->CopyDescriptorsSimple(1, dst, internal_state->subresources_srv[subresource].handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
			}
			else if (resource->IsBuffer())
			{
				const GPUBuffer* buffer = (const GPUBuffer*)resource;
				auto internal_state = to_internal(buffer);

				if (offset > 0)
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC srv = subresource < 0 ? internal_state->srv.srv : internal_state->subresources_srv[subresource].srv;
					switch (binding)
					{
					default:
					case RAWBUFFER:
						srv.Buffer.FirstElement += offset / sizeof(uint32_t);
						break;
					case STRUCTUREDBUFFER:
						srv.Buffer.FirstElement += offset / srv.Buffer.StructureByteStride;
						break;
					case TYPEDBUFFER:
						srv.Buffer.FirstElement += offset / GetFormatStride(buffer->desc.Format);
						break;
					}
					device->CreateShaderResourceView(internal_state->resource.Get(), &srv, dst);
				}
				else
				{
					device->CopyDescriptorsSimple(1, dst, internal_state->srv.handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
			}
			else if (resource->IsAccelerationStructure())
			{
				auto internal_state = to_internal((const RaytracingAccelerationStructure*)resource);
				device->CopyDescriptorsSimple(1, dst, internal_state->srv.handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
			break;
		case RWRAWBUFFER:
		case RWSTRUCTUREDBUFFER:
		case RWTYPEDBUFFER:
		case RWTEXTURE1D:
		case RWTEXTURE1DARRAY:
		case RWTEXTURE2D:
		case RWTEXTURE2DARRAY:
		case RWTEXTURE3D:
			if (resource == nullptr || !resource->IsValid())
			{
				switch (binding)
				{
				case RWRAWBUFFER:
				case RWSTRUCTUREDBUFFER:
				case RWTYPEDBUFFER:
					device->CopyDescriptorsSimple(1, dst, nullUAV_buffer, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case RWTEXTURE1D:
					device->CopyDescriptorsSimple(1, dst, nullUAV_texture1d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case RWTEXTURE1DARRAY:
					device->CopyDescriptorsSimple(1, dst, nullUAV_texture1darray, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case RWTEXTURE2D:
					device->CopyDescriptorsSimple(1, dst, nullUAV_texture2d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case RWTEXTURE2DARRAY:
					device->CopyDescriptorsSimple(1, dst, nullUAV_texture2darray, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				case RWTEXTURE3D:
					device->CopyDescriptorsSimple(1, dst, nullUAV_texture3d, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					break;
				default:
					assert(0);
					break;
				}
			}
			else if (resource->IsTexture())
			{
				auto internal_state = to_internal((const Texture*)resource);
				if (subresource < 0)
				{
					device->CopyDescriptorsSimple(1, dst, internal_state->uav.handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
				else
				{
					device->CopyDescriptorsSimple(1, dst, internal_state->subresources_uav[subresource].handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
			}
			else if (resource->IsBuffer())
			{
				const GPUBuffer* buffer = (const GPUBuffer*)resource;
				auto internal_state = to_internal(buffer);

				if (offset > 0)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC uav = subresource < 0 ? internal_state->uav.uav : internal_state->subresources_uav[subresource].uav;
					switch (binding)
					{
					default:
					case RWRAWBUFFER:
						uav.Buffer.FirstElement += offset / sizeof(uint32_t);
						break;
					case RWSTRUCTUREDBUFFER:
						uav.Buffer.FirstElement += offset / uav.Buffer.StructureByteStride;
						break;
					case RWTYPEDBUFFER:
						uav.Buffer.FirstElement += offset / GetFormatStride(buffer->desc.Format);
						break;
					}
					device->CreateUnorderedAccessView(internal_state->resource.Get(), nullptr, &uav, dst);
				}
				else
				{
					device->CopyDescriptorsSimple(1, dst, internal_state->uav.handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
			}
			break;
		default:
			break;
		}
	}
	void GraphicsDevice_DX12::WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const Sampler* sampler)
	{
		auto table_internal = to_internal(table);
		D3D12_CPU_DESCRIPTOR_HANDLE dst = table_internal->sampler_heap.address;
		size_t remap = table_internal->sampler_heap.write_remap[rangeIndex];
		dst.ptr += (remap + arrayIndex) * (size_t)sampler_descriptor_size;

		if (sampler == nullptr || !sampler->IsValid())
		{
			device->CopyDescriptorsSimple(1, dst, nullSAM, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		}
		else
		{
			auto internal_state = to_internal(sampler);
			device->CopyDescriptorsSimple(1, dst, internal_state->descriptor.handle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		}
	}

	void GraphicsDevice_DX12::Map(const GPUResource* resource, Mapping* mapping)
	{
		auto internal_state = to_internal(resource);
		D3D12_RANGE read_range = {};
		if (mapping->_flags & Mapping::FLAG_READ)
		{
			read_range.Begin = mapping->offset;
			read_range.End = mapping->size;
		}
		HRESULT hr = internal_state->resource->Map(0, &read_range, &mapping->data);
		if (SUCCEEDED(hr))
		{
			mapping->rowpitch = internal_state->footprint.Footprint.RowPitch;
		}
		else
		{
			assert(0);
			mapping->data = nullptr;
			mapping->rowpitch = 0;
		}
	}
	void GraphicsDevice_DX12::Unmap(const GPUResource* resource)
	{
		auto internal_state = to_internal(resource);
		internal_state->resource->Unmap(0, nullptr);
	}
	bool GraphicsDevice_DX12::QueryRead(const GPUQuery* query, GPUQueryResult* result)
	{
		auto internal_state = to_internal(query);

		D3D12_RANGE range;
		range.Begin = (size_t)internal_state->query.index * sizeof(uint64_t);
		range.End = range.Begin + sizeof(uint64_t);
		D3D12_RANGE nullrange = {};
		void* data = nullptr;

		switch (internal_state->query_type)
		{
		case GPU_QUERY_TYPE_EVENT:
			assert(0); // not implemented yet
			break;
		case GPU_QUERY_TYPE_TIMESTAMP:
			if (internal_state->query.index == ~0)
				return false;
			allocationhandler->queries_timestamp.blocks[internal_state->query.block].readback->Map(0, &range, &data);
			result->result_timestamp = *(uint64_t*)((size_t)data + range.Begin);
			allocationhandler->queries_timestamp.blocks[internal_state->query.block].readback->Unmap(0, &nullrange);
			break;
		case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
			directQueue->GetTimestampFrequency(&result->result_timestamp_frequency);
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
		{
			if (internal_state->query.index == ~0)
				return false;
			BOOL passed = FALSE;
			allocationhandler->queries_occlusion.blocks[internal_state->query.block].readback->Map(0, &range, &data);
			passed = *(BOOL*)((size_t)data + range.Begin);
			allocationhandler->queries_occlusion.blocks[internal_state->query.block].readback->Unmap(0, &nullrange);
			result->result_passed_sample_count = (uint64_t)passed;
			break;
		}
		case GPU_QUERY_TYPE_OCCLUSION:
			if (internal_state->query.index == ~0)
				return false;
			allocationhandler->queries_occlusion.blocks[internal_state->query.block].readback->Map(0, &range, &data);
			result->result_passed_sample_count = *(uint64_t*)((size_t)data + range.Begin);
			allocationhandler->queries_occlusion.blocks[internal_state->query.block].readback->Unmap(0, &nullrange);
			break;
		default:
			return false;
		}

		return true;
	}

	void GraphicsDevice_DX12::SetCommonSampler(const StaticSampler* sam)
	{
		common_samplers.push_back(_ConvertStaticSampler(*sam));
	}

	void GraphicsDevice_DX12::SetName(GPUResource* pResource, const char* name)
	{
		wchar_t text[256];
		if (wiHelper::StringConvert(name, text) > 0)
		{
			auto internal_state = to_internal(pResource);
			if (internal_state->resource != nullptr)
			{
				internal_state->resource->SetName(text);
			}
		}
	}

	void GraphicsDevice_DX12::PresentBegin(CommandList cmd)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = backBuffers[backbuffer_index].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		GetDirectCommandList(cmd)->ResourceBarrier(1, &barrier);

		const float clearcolor[] = { 0,0,0,1 };

		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTV = {};
		RTV.cpuDescriptor = backbufferRTV[backbuffer_index];
		RTV.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		RTV.BeginningAccess.Clear.ClearValue.Color[0] = clearcolor[0];
		RTV.BeginningAccess.Clear.ClearValue.Color[1] = clearcolor[1];
		RTV.BeginningAccess.Clear.ClearValue.Color[2] = clearcolor[2];
		RTV.BeginningAccess.Clear.ClearValue.Color[3] = clearcolor[3];
		RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		GetDirectCommandList(cmd)->BeginRenderPass(1, &RTV, nullptr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);

		active_renderpass[cmd] = &dummyRenderpass;
	}
	void GraphicsDevice_DX12::PresentEnd(CommandList cmd)
	{
		GetDirectCommandList(cmd)->EndRenderPass();

		active_renderpass[cmd] = nullptr;

		// Indicate that the back buffer will now be used to present.
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = backBuffers[backbuffer_index].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		frame_barriers[cmd].push_back(barrier);

		SubmitCommandLists();

		HRESULT hr = swapChain->Present(VSYNC, 0);
		assert(SUCCEEDED(hr));
		backbuffer_index = (backbuffer_index + 1) % BACKBUFFER_COUNT;

#if 0
		D3D12MA::Stats stats = {};
		allocationhandler->allocator->CalculateStats(&stats);
		std::cout << "D3D12MA Stats: " << stats.Total.UsedBytes;
#endif
	}

	CommandList GraphicsDevice_DX12::BeginCommandList()
	{
		CommandList cmd = cmd_count.fetch_add(1);
		if (GetDirectCommandList(cmd) == nullptr)
		{
			// need to create one more command list:
			assert(cmd < COMMANDLIST_COUNT);

			HRESULT hr;
			for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
			{
				hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frames[fr].commandAllocators[cmd]));
				hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames[fr].commandAllocators[cmd].Get(), nullptr, IID_PPV_ARGS(&frames[fr].commandLists[cmd]));
				hr = frames[fr].commandLists[cmd]->Close();

				descriptors[cmd].init(this);
				frames[fr].resourceBuffer[cmd].init(this, 1024 * 1024); // 1 MB starting size

				std::wstringstream wss;
				wss << "cmd" << cmd;
				frames[fr].commandLists[cmd]->SetName(wss.str().c_str());
			}
		}


		// Start the command list in a default state:

		HRESULT hr = GetFrameResources().commandAllocators[cmd]->Reset();
		assert(SUCCEEDED(hr));
		hr = GetDirectCommandList(cmd)->Reset(GetFrameResources().commandAllocators[cmd].Get(), nullptr);
		assert(SUCCEEDED(hr));

		ID3D12DescriptorHeap* heaps[2] = {
			descriptorheap_res.heap_GPU.Get(),
			descriptorheap_sam.heap_GPU.Get()
		};
		GetDirectCommandList(cmd)->SetDescriptorHeaps(arraysize(heaps), heaps);

		descriptors[cmd].reset();
		GetFrameResources().resourceBuffer[cmd].clear();

		D3D12_VIEWPORT vp = {};
		vp.Width = (float)RESOLUTIONWIDTH;
		vp.Height = (float)RESOLUTIONHEIGHT;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		GetDirectCommandList(cmd)->RSSetViewports(1, &vp);

		D3D12_RECT pRects[8];
		for (uint32_t i = 0; i < 8; ++i)
		{
			pRects[i].bottom = INT32_MAX;
			pRects[i].left = INT32_MIN;
			pRects[i].right = INT32_MAX;
			pRects[i].top = INT32_MIN;
		}
		GetDirectCommandList(cmd)->RSSetScissorRects(8, pRects);

		prev_pt[cmd] = PRIMITIVETOPOLOGY::UNDEFINED;
		prev_pipeline_hash[cmd] = 0;
		active_pso[cmd] = nullptr;
		active_cs[cmd] = nullptr;
		active_rt[cmd] = nullptr;
		active_rootsig_graphics[cmd] = nullptr;
		active_rootsig_compute[cmd] = nullptr;
		active_renderpass[cmd] = nullptr;
		prev_shadingrate[cmd] = SHADING_RATE_INVALID;
		dirty_pso[cmd] = false;

		return cmd;
	}
	void GraphicsDevice_DX12::SubmitCommandLists()
	{
		// Sync up copy queue:
		copyAllocator.locker.lock();
		if(copyAllocator.submitted)
		{
			HRESULT hr = directQueue->Wait(copyAllocator.fence.Get(), copyAllocator.fenceValue);
			assert(SUCCEEDED(hr));
			copyAllocator.submitted = false;
		}
		copyAllocator.locker.unlock();

		// Execute deferred command lists:
		{
			ID3D12CommandList* cmdLists[COMMANDLIST_COUNT];
			CommandList cmds[COMMANDLIST_COUNT];
			uint32_t counter = 0;

			CommandList cmd_last = cmd_count.load();
			cmd_count.store(0);
			for (CommandList cmd = 0; cmd < cmd_last; ++cmd)
			{
				query_flush(cmd);
				barrier_flush(cmd);

				HRESULT hr = GetDirectCommandList(cmd)->Close();
				assert(SUCCEEDED(hr));

				cmdLists[counter] = GetDirectCommandList(cmd);
				cmds[counter] = cmd;
				counter++;

				for (auto& x : pipelines_worker[cmd])
				{
					if (pipelines_global.count(x.first) == 0)
					{
						pipelines_global[x.first] = x.second;
					}
					else
					{
						allocationhandler->destroylocker.lock();
						allocationhandler->destroyer_pipelines.push_back(std::make_pair(x.second, FRAMECOUNT));
						allocationhandler->destroylocker.unlock();
					}
				}
				pipelines_worker[cmd].clear();
			}

			directQueue->ExecuteCommandLists(counter, cmdLists);
		}

		// This acts as a barrier, following this we will be using the next frame's resources when calling GetFrameResources()!
		FRAMECOUNT++;
		HRESULT hr = directQueue->Signal(frameFence.Get(), FRAMECOUNT);
		assert(SUCCEEDED(hr));

		// Descriptor heaps' progress is recorded by the GPU:
		descriptorheap_res.fenceValue = descriptorheap_res.allocationOffset.load();
		hr = directQueue->Signal(descriptorheap_res.fence.Get(), descriptorheap_res.fenceValue);
		assert(SUCCEEDED(hr));
		descriptorheap_res.cached_completedValue = descriptorheap_res.fence->GetCompletedValue();
		descriptorheap_sam.fenceValue = descriptorheap_sam.allocationOffset.load();
		hr = directQueue->Signal(descriptorheap_sam.fence.Get(), descriptorheap_sam.fenceValue);
		assert(SUCCEEDED(hr));
		descriptorheap_sam.cached_completedValue = descriptorheap_sam.fence->GetCompletedValue();

		// Determine the last frame that we should not wait on:
		const uint64_t lastFrameToAllowLatency = std::max(uint64_t(BACKBUFFER_COUNT - 1u), FRAMECOUNT) - (BACKBUFFER_COUNT - 1);

		// Wait if too many frames are being incomplete:
		if (frameFence->GetCompletedValue() < lastFrameToAllowLatency)
		{
			hr = frameFence->SetEventOnCompletion(lastFrameToAllowLatency, frameFenceEvent);
			WaitForSingleObject(frameFenceEvent, INFINITE);
		}

		allocationhandler->Update(FRAMECOUNT, BACKBUFFER_COUNT);

	}

	void GraphicsDevice_DX12::WaitForGPU()
	{
		directFenceValue++;
		HRESULT hr = directQueue->Signal(directFence.Get(), directFenceValue);
		assert(SUCCEEDED(hr));
		if (directFence->GetCompletedValue() < directFenceValue)
		{
			hr = directFence->SetEventOnCompletion(directFenceValue, directFenceEvent);
			WaitForSingleObject(directFenceEvent, INFINITE);
		}
		assert(SUCCEEDED(hr));
	}
	void GraphicsDevice_DX12::ClearPipelineStateCache()
	{
		allocationhandler->destroylocker.lock();

		rootsignature_cache_mutex.lock();
		for (auto& x : rootsignature_cache)
		{
			if (x.second) allocationhandler->destroyer_rootSignatures.push_back(std::make_pair(x.second, FRAMECOUNT));
		}
		rootsignature_cache.clear();
		rootsignature_cache_mutex.unlock();

		for (auto& x : pipelines_global)
		{
			allocationhandler->destroyer_pipelines.push_back(std::make_pair(x.second, FRAMECOUNT));
		}
		pipelines_global.clear();

		for (int i = 0; i < arraysize(pipelines_worker); ++i)
		{
			for (auto& x : pipelines_worker[i])
			{
				allocationhandler->destroyer_pipelines.push_back(std::make_pair(x.second, FRAMECOUNT));
			}
			pipelines_worker[i].clear();
		}
		allocationhandler->destroylocker.unlock();
	}

	void GraphicsDevice_DX12::RenderPassBegin(const RenderPass* renderpass, CommandList cmd)
	{
		active_renderpass[cmd] = renderpass;

		auto internal_state = to_internal(active_renderpass[cmd]);

		for (uint32_t i = 0; i < internal_state->num_barriers_begin; ++i)
		{
			frame_barriers[cmd].push_back(internal_state->barrierdescs_begin[i]);
		}
		barrier_flush(cmd);

		if (internal_state->shading_rate_image != nullptr)
		{
			GetDirectCommandList(cmd)->RSSetShadingRateImage(to_internal(internal_state->shading_rate_image)->resource.Get());
		}

		GetDirectCommandList(cmd)->BeginRenderPass(
			internal_state->rt_count,
			internal_state->RTVs,
			internal_state->DSV.cpuDescriptor.ptr == 0 ? nullptr : &internal_state->DSV,
			internal_state->flags
		);

	}
	void GraphicsDevice_DX12::RenderPassEnd(CommandList cmd)
	{
		GetDirectCommandList(cmd)->EndRenderPass();

		auto internal_state = to_internal(active_renderpass[cmd]);

		if (internal_state->shading_rate_image != nullptr)
		{
			GetDirectCommandList(cmd)->RSSetShadingRateImage(nullptr);
		}

		for (uint32_t i = 0; i < internal_state->num_barriers_end; ++i)
		{
			frame_barriers[cmd].push_back(internal_state->barrierdescs_end[i]);
		}

		active_renderpass[cmd] = nullptr;

		query_flush(cmd);
	}
	void GraphicsDevice_DX12::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) {
		assert(rects != nullptr);
		assert(numRects <= 8);
		D3D12_RECT pRects[8];
		for (uint32_t i = 0; i < numRects; ++i) {
			pRects[i].bottom = (LONG)rects[i].bottom;
			pRects[i].left = (LONG)rects[i].left;
			pRects[i].right = (LONG)rects[i].right;
			pRects[i].top = (LONG)rects[i].top;
		}
		GetDirectCommandList(cmd)->RSSetScissorRects(numRects, pRects);
	}
	void GraphicsDevice_DX12::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		assert(NumViewports <= 6);
		D3D12_VIEWPORT d3dViewPorts[6];
		for (uint32_t i = 0; i < NumViewports; ++i)
		{
			d3dViewPorts[i].TopLeftX = pViewports[i].TopLeftX;
			d3dViewPorts[i].TopLeftY = pViewports[i].TopLeftY;
			d3dViewPorts[i].Width = pViewports[i].Width;
			d3dViewPorts[i].Height = pViewports[i].Height;
			d3dViewPorts[i].MinDepth = pViewports[i].MinDepth;
			d3dViewPorts[i].MaxDepth = pViewports[i].MaxDepth;
		}
		GetDirectCommandList(cmd)->RSSetViewports(NumViewports, d3dViewPorts);
	}
	void GraphicsDevice_DX12::BindResource(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < GPU_RESOURCE_HEAP_SRV_COUNT);
		if (descriptors[cmd].SRV[slot] != resource || descriptors[cmd].SRV_index[slot] != subresource)
		{
			descriptors[cmd].SRV[slot] = resource;
			descriptors[cmd].SRV_index[slot] = subresource;
			descriptors[cmd].dirty_res = true;
		}
	}
	void GraphicsDevice_DX12::BindResources(SHADERSTAGE stage, const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindResource(stage, resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_DX12::BindUAV(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < GPU_RESOURCE_HEAP_UAV_COUNT);
		if (descriptors[cmd].UAV[slot] != resource || descriptors[cmd].UAV_index[slot] != subresource)
		{
			descriptors[cmd].UAV[slot] = resource;
			descriptors[cmd].UAV_index[slot] = subresource;
			descriptors[cmd].dirty_res = true;
		}
	}
	void GraphicsDevice_DX12::BindUAVs(SHADERSTAGE stage, const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindUAV(stage, resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_DX12::UnbindResources(uint32_t slot, uint32_t num, CommandList cmd)
	{
	}
	void GraphicsDevice_DX12::UnbindUAVs(uint32_t slot, uint32_t num, CommandList cmd)
	{
	}
	void GraphicsDevice_DX12::BindSampler(SHADERSTAGE stage, const Sampler* sampler, uint32_t slot, CommandList cmd)
	{
		assert(slot < GPU_SAMPLER_HEAP_COUNT);
		if (descriptors[cmd].SAM[slot] != sampler)
		{
			descriptors[cmd].SAM[slot] = sampler;
			descriptors[cmd].dirty_sam = true;
		}
	}
	void GraphicsDevice_DX12::BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, uint32_t slot, CommandList cmd)
	{
		assert(slot < GPU_RESOURCE_HEAP_CBV_COUNT);
		if (buffer->desc.Usage == USAGE_DYNAMIC || descriptors[cmd].CBV[slot] != buffer)
		{
			descriptors[cmd].CBV[slot] = buffer;
			descriptors[cmd].dirty_res = true;

			// Root constant buffer root signature state tracking:
			auto internal_state = to_internal(buffer);
			if (internal_state->cbv_mask_frame[cmd] != FRAMECOUNT)
			{
				// This is the first binding as constant buffer in this frame for this resource,
				//	so clear the cbv flags completely
				internal_state->cbv_mask_gfx[cmd] = 0;
				internal_state->cbv_mask_compute[cmd] = 0;
				internal_state->cbv_mask_frame[cmd] = FRAMECOUNT;
			}

			// CBV flag marked as bound for this slot:
			//	Also, the corresponding slot is marked dirty
			if (stage == CS)
			{
				internal_state->cbv_mask_compute[cmd] |= 1 << slot;
				descriptors[cmd].dirty_root_cbvs_compute |= 1 << slot;
			}
			else
			{
				internal_state->cbv_mask_gfx[cmd] |= 1 << slot;
				descriptors[cmd].dirty_root_cbvs_gfx |= 1 << slot;
			}
		}
	}
	void GraphicsDevice_DX12::BindVertexBuffers(const GPUBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets, CommandList cmd)
	{
		assert(count <= 8);
		D3D12_VERTEX_BUFFER_VIEW res[8] = {};
		for (uint32_t i = 0; i < count; ++i)
		{
			if (vertexBuffers[i] != nullptr)
			{
				res[i].BufferLocation = vertexBuffers[i]->IsValid() ? to_internal(vertexBuffers[i])->gpu_address : 0;
				res[i].SizeInBytes = vertexBuffers[i]->desc.ByteWidth;
				if (offsets != nullptr)
				{
					res[i].BufferLocation += (D3D12_GPU_VIRTUAL_ADDRESS)offsets[i];
					res[i].SizeInBytes -= offsets[i];
				}
				res[i].StrideInBytes = strides[i];
			}
		}
		GetDirectCommandList(cmd)->IASetVertexBuffers(static_cast<uint32_t>(slot), static_cast<uint32_t>(count), res);
	}
	void GraphicsDevice_DX12::BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, uint32_t offset, CommandList cmd)
	{
		D3D12_INDEX_BUFFER_VIEW res = {};
		if (indexBuffer != nullptr)
		{
			auto internal_state = to_internal(indexBuffer);

			res.BufferLocation = internal_state->gpu_address + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
			res.Format = (format == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
			res.SizeInBytes = indexBuffer->desc.ByteWidth;
		}
		GetDirectCommandList(cmd)->IASetIndexBuffer(&res);
	}
	void GraphicsDevice_DX12::BindStencilRef(uint32_t value, CommandList cmd)
	{
		GetDirectCommandList(cmd)->OMSetStencilRef(value);
	}
	void GraphicsDevice_DX12::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		const float blendFactor[4] = { r, g, b, a };
		GetDirectCommandList(cmd)->OMSetBlendFactor(blendFactor);
	}
	void GraphicsDevice_DX12::BindShadingRate(SHADING_RATE rate, CommandList cmd)
	{
		if (CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING) && prev_shadingrate[cmd] != rate)
		{
			prev_shadingrate[cmd] = rate;

			D3D12_SHADING_RATE _rate = D3D12_SHADING_RATE_1X1;
			WriteShadingRateValue(rate, &_rate);

			D3D12_SHADING_RATE_COMBINER combiners[] =
			{
				D3D12_SHADING_RATE_COMBINER_MAX,
				D3D12_SHADING_RATE_COMBINER_MAX,
			};
			GetDirectCommandList(cmd)->RSSetShadingRate(_rate, combiners);
		}
	}
	void GraphicsDevice_DX12::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		size_t pipeline_hash = 0;
		wiHelper::hash_combine(pipeline_hash, pso->hash);
		if (active_renderpass[cmd] != nullptr)
		{
			wiHelper::hash_combine(pipeline_hash, active_renderpass[cmd]->hash);
		}
		if (prev_pipeline_hash[cmd] == pipeline_hash)
		{
			return;
		}
		prev_pipeline_hash[cmd] = pipeline_hash;

		auto internal_state = to_internal(pso);

		if (active_rootsig_graphics[cmd] != internal_state->rootSignature.Get())
		{
			active_rootsig_graphics[cmd] = internal_state->rootSignature.Get();
			GetDirectCommandList(cmd)->SetGraphicsRootSignature(internal_state->rootSignature.Get());

			// Invalidate graphics root bindings:
			descriptors[cmd].dirty_res = true;
			descriptors[cmd].dirty_sam = true;
			descriptors[cmd].dirty_root_cbvs_gfx = ~0;
		}

		active_pso[cmd] = pso;
		dirty_pso[cmd] = true;
	}
	void GraphicsDevice_DX12::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		assert(cs->stage == CS);
		if (active_cs[cmd] != cs)
		{
			prev_pipeline_hash[cmd] = 0;

			active_cs[cmd] = cs;

			auto internal_state = to_internal(cs);
			GetDirectCommandList(cmd)->SetPipelineState(internal_state->resource.Get());

			if (active_rootsig_compute[cmd] != internal_state->rootSignature.Get())
			{
				active_rootsig_compute[cmd] = internal_state->rootSignature.Get();
				GetDirectCommandList(cmd)->SetComputeRootSignature(internal_state->rootSignature.Get());

				// Invalidate compute root bindings:
				descriptors[cmd].dirty_res = true;
				descriptors[cmd].dirty_sam = true;
				descriptors[cmd].dirty_root_cbvs_compute = ~0;
			}
		}
	}
	void GraphicsDevice_DX12::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		GetDirectCommandList(cmd)->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		GetDirectCommandList(cmd)->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		GetDirectCommandList(cmd)->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		GetDirectCommandList(cmd)->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		GetDirectCommandList(cmd)->ExecuteIndirect(drawInstancedIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		GetDirectCommandList(cmd)->ExecuteIndirect(drawIndexedInstancedIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predispatch(cmd);
		GetDirectCommandList(cmd)->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_DX12::DispatchIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		auto internal_state = to_internal(args);
		GetDirectCommandList(cmd)->ExecuteIndirect(dispatchIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predraw(cmd);
		GetDirectCommandList(cmd)->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_DX12::DispatchMeshIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		GetDirectCommandList(cmd)->ExecuteIndirect(dispatchMeshIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		barrier_flush(cmd);
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);
		D3D12_RESOURCE_DESC desc_src = internal_state_src->resource->GetDesc();
		D3D12_RESOURCE_DESC desc_dst = internal_state_dst->resource->GetDesc();
		if (desc_dst.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && desc_src.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			CD3DX12_TEXTURE_COPY_LOCATION Src(internal_state_src->resource.Get(), 0);
			CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state_dst->resource.Get(), internal_state_src->footprint);
			GetDirectCommandList(cmd)->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
		else if (desc_src.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && desc_dst.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			CD3DX12_TEXTURE_COPY_LOCATION Src(internal_state_src->resource.Get(), internal_state_dst->footprint);
			CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state_dst->resource.Get(), 0);
			GetDirectCommandList(cmd)->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
		else
		{
			GetDirectCommandList(cmd)->CopyResource(internal_state_dst->resource.Get(), internal_state_src->resource.Get());
		}
	}
	void GraphicsDevice_DX12::UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize)
	{
		assert(buffer->desc.Usage != USAGE_IMMUTABLE && "Cannot update IMMUTABLE GPUBuffer!");
		assert((int)buffer->desc.ByteWidth >= dataSize || dataSize < 0 && "Data size is too big!");

		if (dataSize == 0)
		{
			return;
		}

		dataSize = std::min((int)buffer->desc.ByteWidth, dataSize);
		dataSize = (dataSize >= 0 ? dataSize : buffer->desc.ByteWidth);

		if (buffer->desc.Usage == USAGE_DYNAMIC && buffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
		{
			// Dynamic buffer will be used from host memory directly:
			auto internal_state = to_internal(buffer);
			GPUAllocation allocation = AllocateGPU(dataSize, cmd);
			memcpy(allocation.data, data, dataSize);
			internal_state->dynamic[cmd] = allocation;

			// The proper binding slot is not tracked properly, but instead all the previous bindings are invalidated:
			descriptors[cmd].dirty_res = true;
			descriptors[cmd].dirty_root_cbvs_gfx |= internal_state->cbv_mask_gfx[cmd];
			descriptors[cmd].dirty_root_cbvs_compute |= internal_state->cbv_mask_compute[cmd];
		}
		else
		{
			assert(active_renderpass[cmd] == nullptr);

			// Contents will be transferred to device memory:
			auto internal_state_src = to_internal(&GetFrameResources().resourceBuffer[cmd].buffer);
			auto internal_state_dst = to_internal(buffer);

			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = internal_state_dst->resource.Get();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
			if (buffer->desc.BindFlags & BIND_CONSTANT_BUFFER || buffer->desc.BindFlags & BIND_VERTEX_BUFFER)
			{
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			}
			else if (buffer->desc.BindFlags & BIND_INDEX_BUFFER)
			{
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDEX_BUFFER;
			}
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			frame_barriers[cmd].push_back(barrier);
			barrier_flush(cmd);

			uint8_t* dest = GetFrameResources().resourceBuffer[cmd].allocate(dataSize, 1);
			memcpy(dest, data, dataSize);
			GetDirectCommandList(cmd)->CopyBufferRegion(
				internal_state_dst->resource.Get(), 0,
				internal_state_src->resource.Get(), GetFrameResources().resourceBuffer[cmd].calculateOffset(dest),
				dataSize
			);

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
			frame_barriers[cmd].push_back(barrier);

		}

	}
	void GraphicsDevice_DX12::QueryBegin(const GPUQuery* query, CommandList cmd)
	{
		auto internal_state = to_internal(query);

		switch (internal_state->query_type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			GetDirectCommandList(cmd)->BeginQuery(allocationhandler->queries_timestamp.blocks[internal_state->query.block].pool.Get(), D3D12_QUERY_TYPE_TIMESTAMP, internal_state->query.index);
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			GetDirectCommandList(cmd)->BeginQuery(allocationhandler->queries_occlusion.blocks[internal_state->query.block].pool.Get(), D3D12_QUERY_TYPE_BINARY_OCCLUSION, internal_state->query.index);
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
			GetDirectCommandList(cmd)->BeginQuery(allocationhandler->queries_occlusion.blocks[internal_state->query.block].pool.Get(), D3D12_QUERY_TYPE_OCCLUSION, internal_state->query.index);
			break;
		}
	}
	void GraphicsDevice_DX12::QueryEnd(const GPUQuery* query, CommandList cmd)
	{
		auto internal_state = to_internal(query);
		uint64_t resolver = QueryResolveCreate(internal_state->query.block, internal_state->query.index);

		switch (internal_state->query_type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			GetDirectCommandList(cmd)->EndQuery(
				allocationhandler->queries_timestamp.blocks[internal_state->query.block].pool.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				internal_state->query.index
			);
			query_resolves_timestamp[cmd].push_back(resolver);
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			GetDirectCommandList(cmd)->EndQuery(
				allocationhandler->queries_occlusion.blocks[internal_state->query.block].pool.Get(),
				D3D12_QUERY_TYPE_BINARY_OCCLUSION,
				internal_state->query.index
			);
			query_resolves_occlusionpred[cmd].push_back(resolver);
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
			GetDirectCommandList(cmd)->EndQuery(
				allocationhandler->queries_occlusion.blocks[internal_state->query.block].pool.Get(),
				D3D12_QUERY_TYPE_OCCLUSION,
				internal_state->query.index
			);
			query_resolves_occlusion[cmd].push_back(resolver);
			break;
		}
	}
	void GraphicsDevice_DX12::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		auto& barrierdescs = frame_barriers[cmd];

		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];

			D3D12_RESOURCE_BARRIER barrierdesc = {};

			switch (barrier.type)
			{
			default:
			case GPUBarrier::MEMORY_BARRIER:
			{
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.UAV.pResource = barrier.memory.resource == nullptr ? nullptr : to_internal(barrier.memory.resource)->resource.Get();
			}
			break;
			case GPUBarrier::IMAGE_BARRIER:
			{
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Transition.pResource = to_internal(barrier.image.texture)->resource.Get();
				barrierdesc.Transition.StateBefore = _ConvertImageLayout(barrier.image.layout_before);
				barrierdesc.Transition.StateAfter = _ConvertImageLayout(barrier.image.layout_after);
				if (barrier.image.mip >= 0 || barrier.image.slice >= 0)
				{
					barrierdesc.Transition.Subresource = D3D12CalcSubresource(
						(UINT)std::max(0, barrier.image.mip),
						(UINT)std::max(0, barrier.image.slice),
						0,
						barrier.image.texture->desc.MipLevels,
						barrier.image.texture->desc.ArraySize
					);
				}
				else
				{
					barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				}
			}
			break;
			case GPUBarrier::BUFFER_BARRIER:
			{
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Transition.pResource = to_internal(barrier.buffer.buffer)->resource.Get();
				barrierdesc.Transition.StateBefore = _ConvertBufferState(barrier.buffer.state_before);
				barrierdesc.Transition.StateAfter = _ConvertBufferState(barrier.buffer.state_after);
				barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			}
			break;
			}

			// Try to detect redundant barriers:
			bool found = false;
			for (auto& x : barrierdescs)
			{
				if (x.Type == barrierdesc.Type && x.Flags == barrierdesc.Flags)
				{
					switch (x.Type)
					{
					default:
					case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
						if (x.Transition.pResource == barrierdesc.Transition.pResource &&
							x.Transition.Subresource == barrierdesc.Transition.Subresource)
						{
							found = true;
							x.Transition.StateAfter = barrierdesc.Transition.StateAfter;
						}
						break;
					}
				}
				if (found)
					break;
			}
			if (!found)
			{
				barrierdescs.push_back(barrierdesc);
			}
		}
	}
	void GraphicsDevice_DX12::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		barrier_flush(cmd);

		auto dst_internal = to_internal(dst);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
		desc.Inputs = dst_internal->desc;

		// Make a copy of geometries, don't overwrite internal_state (thread safety)
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
		geometries = dst_internal->geometries;
		desc.Inputs.pGeometryDescs = geometries.data();

		// The real GPU addresses get filled here:
		switch (dst->desc.type)
		{
		case RaytracingAccelerationStructureDesc::BOTTOMLEVEL:
		{
			size_t i = 0;
			for (auto& x : dst->desc.bottomlevel.geometries)
			{
				auto& geometry = geometries[i++];
				if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE)
				{
					geometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
				}
				if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_NO_DUPLICATE_ANYHIT_INVOCATION)
				{
					geometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
				}

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES)
				{
					geometry.Triangles.VertexBuffer.StartAddress = to_internal(&x.triangles.vertexBuffer)->gpu_address + 
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.vertexByteOffset;
					geometry.Triangles.IndexBuffer = to_internal(&x.triangles.indexBuffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.indexOffset * (x.triangles.indexFormat == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? sizeof(uint16_t) : sizeof(uint32_t));

					if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.Triangles.Transform3x4 = to_internal(&x.triangles.transform3x4Buffer)->gpu_address +
							(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.transform3x4BufferOffset;
					}
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
				{
					geometry.AABBs.AABBs.StartAddress = to_internal(&x.aabbs.aabbBuffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.aabbs.offset;
				}
			}
		}
		break;
		case RaytracingAccelerationStructureDesc::TOPLEVEL:
		{
			desc.Inputs.InstanceDescs = to_internal(&dst->desc.toplevel.instanceBuffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)dst->desc.toplevel.offset;
		}
		break;
		}

		if (src != nullptr)
		{
			desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

			auto src_internal = to_internal(src);
			desc.SourceAccelerationStructureData = src_internal->gpu_address;
		}
		desc.DestAccelerationStructureData = dst_internal->gpu_address;
		desc.ScratchAccelerationStructureData = to_internal(&dst_internal->scratch)->gpu_address;
		GetDirectCommandList(cmd)->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
	}
	void GraphicsDevice_DX12::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		if (rtpso != active_rt[cmd])
		{
			prev_pipeline_hash[cmd] = 0;
			active_rt[cmd] = rtpso;
			descriptors[cmd].dirty_res = true;
			descriptors[cmd].dirty_sam = true;

			auto internal_state = to_internal(rtpso);
			GetDirectCommandList(cmd)->SetPipelineState1(internal_state->resource.Get());

			if (rtpso->desc.rootSignature == nullptr)
			{
				// we just take the first shader (todo: better)
				active_cs[cmd] = rtpso->desc.shaderlibraries.front().shader;
				active_rootsig_compute[cmd] = nullptr;
				GetDirectCommandList(cmd)->SetComputeRootSignature(to_internal(active_cs[cmd])->rootSignature.Get());
			}
			else if (active_rootsig_compute[cmd] != to_internal(rtpso->desc.rootSignature)->resource.Get())
			{
				active_rootsig_compute[cmd] = to_internal(rtpso->desc.rootSignature)->resource.Get();
				GetDirectCommandList(cmd)->SetComputeRootSignature(to_internal(rtpso->desc.rootSignature)->resource.Get());
			}
		}
	}
	void GraphicsDevice_DX12::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		preraytrace(cmd);

		D3D12_DISPATCH_RAYS_DESC dispatchrays_desc = {};

		dispatchrays_desc.Width = desc->Width;
		dispatchrays_desc.Height = desc->Height;
		dispatchrays_desc.Depth = desc->Depth;

		if (desc->raygeneration.buffer != nullptr)
		{
			dispatchrays_desc.RayGenerationShaderRecord.StartAddress =
				to_internal(desc->raygeneration.buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->raygeneration.offset;
			dispatchrays_desc.RayGenerationShaderRecord.SizeInBytes =
				desc->raygeneration.size;
		}

		if (desc->miss.buffer != nullptr)
		{
			dispatchrays_desc.MissShaderTable.StartAddress =
				to_internal(desc->miss.buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->miss.offset;
			dispatchrays_desc.MissShaderTable.SizeInBytes =
				desc->miss.size;
			dispatchrays_desc.MissShaderTable.StrideInBytes =
				desc->miss.stride;
		}

		if (desc->hitgroup.buffer != nullptr)
		{
			dispatchrays_desc.HitGroupTable.StartAddress =
				to_internal(desc->hitgroup.buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->hitgroup.offset;
			dispatchrays_desc.HitGroupTable.SizeInBytes =
				desc->hitgroup.size;
			dispatchrays_desc.HitGroupTable.StrideInBytes =
				desc->hitgroup.stride;
		}

		if (desc->callable.buffer != nullptr)
		{
			dispatchrays_desc.CallableShaderTable.StartAddress =
				to_internal(desc->callable.buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->callable.offset;
			dispatchrays_desc.CallableShaderTable.SizeInBytes =
				desc->callable.size;
			dispatchrays_desc.CallableShaderTable.StrideInBytes =
				desc->callable.stride;
		}

		GetDirectCommandList(cmd)->DispatchRays(&dispatchrays_desc);
	}

	void GraphicsDevice_DX12::BindDescriptorTable(BINDPOINT bindpoint, uint32_t space, const DescriptorTable* table, CommandList cmd)
	{
		descriptors[cmd].dirty_res = true;
		descriptors[cmd].dirty_sam = true;

		const RootSignature* rootsig = nullptr;
		switch (bindpoint)
		{
		default:
		case wiGraphics::GRAPHICS:
			rootsig = active_pso[cmd]->desc.rootSignature;
			descriptors[cmd].dirty_root_cbvs_gfx = ~0;
			break;
		case wiGraphics::COMPUTE:
			rootsig = active_cs[cmd]->rootSignature;
			descriptors[cmd].dirty_root_cbvs_compute = ~0;
			break;
		case wiGraphics::RAYTRACING:
			rootsig = active_rt[cmd]->desc.rootSignature;
			descriptors[cmd].dirty_root_cbvs_compute = ~0;
			break;
		}
		auto rootsig_internal = to_internal(rootsig);
		uint32_t bind_point_remap = rootsig_internal->table_bind_point_remap[space];
		auto handles = descriptors[cmd].commit(table, cmd);
		if (handles.resource_handle.ptr != 0)
		{
			switch (bindpoint)
			{
			default:
			case wiGraphics::GRAPHICS:
				GetDirectCommandList(cmd)->SetGraphicsRootDescriptorTable(bind_point_remap, handles.resource_handle);
				break;
			case wiGraphics::COMPUTE:
			case wiGraphics::RAYTRACING:
				GetDirectCommandList(cmd)->SetComputeRootDescriptorTable(bind_point_remap, handles.resource_handle);
				break;
			}
			bind_point_remap++;
		}
		if (handles.sampler_handle.ptr != 0)
		{
			switch (bindpoint)
			{
			default:
			case wiGraphics::GRAPHICS:
				GetDirectCommandList(cmd)->SetGraphicsRootDescriptorTable(bind_point_remap, handles.sampler_handle);
				break;
			case wiGraphics::COMPUTE:
			case wiGraphics::RAYTRACING:
				GetDirectCommandList(cmd)->SetComputeRootDescriptorTable(bind_point_remap, handles.sampler_handle);
				break;
			}
		}
	}
	void GraphicsDevice_DX12::BindRootDescriptor(BINDPOINT bindpoint, uint32_t index, const GPUBuffer* buffer, uint32_t offset, CommandList cmd)
	{
		descriptors[cmd].dirty_res = true;
		descriptors[cmd].dirty_sam = true;

		const RootSignature* rootsig = nullptr;
		switch (bindpoint)
		{
		default:
		case wiGraphics::GRAPHICS:
			rootsig = active_pso[cmd]->desc.rootSignature;
			descriptors[cmd].dirty_root_cbvs_gfx = ~0;
			break;
		case wiGraphics::COMPUTE:
			rootsig = active_cs[cmd]->rootSignature;
			descriptors[cmd].dirty_root_cbvs_compute = ~0;
			break;
		case wiGraphics::RAYTRACING:
			rootsig = active_rt[cmd]->desc.rootSignature;
			descriptors[cmd].dirty_root_cbvs_compute = ~0;
			break;
		}
		auto rootsig_internal = to_internal(rootsig);
		auto internal_state = to_internal(buffer);
		D3D12_GPU_VIRTUAL_ADDRESS address = internal_state->gpu_address + (UINT64)offset;

		auto remap = rootsig_internal->root_remap[index];
		auto binding = rootsig->tables[remap.space].resources[remap.rangeIndex].binding;
		switch (binding)
		{
		case ROOT_CONSTANTBUFFER:
			switch (bindpoint)
			{
			default:
			case wiGraphics::GRAPHICS:
				GetDirectCommandList(cmd)->SetGraphicsRootConstantBufferView(index, address);
				break;
			case wiGraphics::COMPUTE:
			case wiGraphics::RAYTRACING:
				GetDirectCommandList(cmd)->SetComputeRootConstantBufferView(index, address);
				break;
			}
			break;
		case ROOT_RAWBUFFER:
		case ROOT_STRUCTUREDBUFFER:
			switch (bindpoint)
			{
			default:
			case wiGraphics::GRAPHICS:
				GetDirectCommandList(cmd)->SetGraphicsRootShaderResourceView(index, address);
				break;
			case wiGraphics::COMPUTE:
			case wiGraphics::RAYTRACING:
				GetDirectCommandList(cmd)->SetComputeRootShaderResourceView(index, address);
				break;
			}
			break;
		case ROOT_RWRAWBUFFER:
		case ROOT_RWSTRUCTUREDBUFFER:
			switch (bindpoint)
			{
			default:
			case wiGraphics::GRAPHICS:
				GetDirectCommandList(cmd)->SetGraphicsRootUnorderedAccessView(index, address);
				break;
			case wiGraphics::COMPUTE:
			case wiGraphics::RAYTRACING:
				GetDirectCommandList(cmd)->SetComputeRootUnorderedAccessView(index, address);
				break;
			}
			break;
		default:
			break;
		}
	}
	void GraphicsDevice_DX12::BindRootConstants(BINDPOINT bindpoint, uint32_t index, const void* srcdata, CommandList cmd)
	{
		descriptors[cmd].dirty_res = true;
		descriptors[cmd].dirty_sam = true;

		const RootSignature* rootsig = nullptr;
		switch (bindpoint)
		{
		default:
		case wiGraphics::GRAPHICS:
			rootsig = active_pso[cmd]->desc.rootSignature;
			descriptors[cmd].dirty_root_cbvs_gfx = ~0;
			break;
		case wiGraphics::COMPUTE:
			rootsig = active_cs[cmd]->rootSignature;
			descriptors[cmd].dirty_root_cbvs_compute = ~0;
			break;
		case wiGraphics::RAYTRACING:
			rootsig = active_rt[cmd]->desc.rootSignature;
			descriptors[cmd].dirty_root_cbvs_compute = ~0;
			break;
		}
		auto rootsig_internal = to_internal(rootsig);
		const RootConstantRange& range = rootsig->rootconstants[index];

		switch (bindpoint)
		{
		default:
		case wiGraphics::GRAPHICS:
			GetDirectCommandList(cmd)->SetGraphicsRoot32BitConstants(
				rootsig_internal->root_constant_bind_remap + index,
				range.size / sizeof(uint32_t),
				srcdata,
				range.offset / sizeof(uint32_t)
			);
			break;
		case wiGraphics::COMPUTE:
		case wiGraphics::RAYTRACING:
			GetDirectCommandList(cmd)->SetComputeRoot32BitConstants(
				rootsig_internal->root_constant_bind_remap + index,
				range.size / sizeof(uint32_t),
				srcdata,
				range.offset / sizeof(uint32_t)
			);
			break;
		}
	}

	GraphicsDevice::GPUAllocation GraphicsDevice_DX12::AllocateGPU(size_t dataSize, CommandList cmd)
	{
		GPUAllocation result;
		if (dataSize == 0)
		{
			return result;
		}

		FrameResources::ResourceFrameAllocator& allocator = GetFrameResources().resourceBuffer[cmd];
		uint8_t* dest = allocator.allocate(dataSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		assert(dest != nullptr);

		result.buffer = &allocator.buffer;
		result.offset = (uint32_t)allocator.calculateOffset(dest);
		result.data = (void*)dest;
		return result;
	}

	void GraphicsDevice_DX12::EventBegin(const char* name, CommandList cmd)
	{
		wchar_t text[128];
		if (wiHelper::StringConvert(name, text) > 0)
		{
			PIXBeginEvent(GetDirectCommandList(cmd), 0xFF000000, text);
		}
	}
	void GraphicsDevice_DX12::EventEnd(CommandList cmd)
	{
		PIXEndEvent(GetDirectCommandList(cmd));
	}
	void GraphicsDevice_DX12::SetMarker(const char* name, CommandList cmd)
	{
		wchar_t text[128];
		if (wiHelper::StringConvert(name, text) > 0)
		{
			PIXSetMarker(GetDirectCommandList(cmd), 0xFFFF0000, text);
		}
	}


}

#endif // WICKEDENGINE_BUILD_DX12
