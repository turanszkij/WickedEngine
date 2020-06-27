#include "wiGraphicsDevice_DX12.h"

#ifdef WICKEDENGINE_BUILD_DX12

#include "wiGraphicsDevice_SharedInternals.h"
#include "wiHelper.h"
#include "ResourceMapping.h"
#include "wiBackLog.h"

#include "Utility/d3dx12.h"
#include "Utility/D3D12MemAlloc.h"

#include <pix.h>
#include <dxcapi.h>
#include <d3d12shader.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"Dxgi.lib")
#pragma comment(lib,"dxguid.lib")

#ifndef PLATFORM_UWP
#pragma comment(lib,"dxcompiler.lib")
#endif // PLATFORM_UWP

#ifdef _DEBUG
#include <d3d12sdklayers.h>
#endif // _DEBUG

#include <sstream>
#include <algorithm>
#include <wincodec.h>

// Uncomment this to enable DX12 renderpass feature:
//#define DX12_REAL_RENDERPASS

using namespace Microsoft::WRL;

namespace wiGraphics
{

namespace DX12_Internal
{
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
		case wiGraphics::IMAGE_LAYOUT_GENERAL:
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
		}

		return D3D12_RESOURCE_STATE_COMMON;
	}
	constexpr D3D12_RESOURCE_STATES _ConvertBufferState(BUFFER_STATE value)
	{
		switch (value)
		{
		case wiGraphics::BUFFER_STATE_GENERAL:
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


	struct Resource_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		D3D12MA::Allocation* allocation = nullptr;
		ComPtr<ID3D12Resource> resource;
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = {};
		D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
		D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> subresources_srv;
		std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> subresources_uav;

		virtual ~Resource_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (allocation) allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
			if (resource) allocationhandler->destroyer_resources.push_back(std::make_pair(resource, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Texture_DX12 : public Resource_DX12
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtv = {};
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		std::vector<D3D12_RENDER_TARGET_VIEW_DESC> subresources_rtv;
		std::vector<D3D12_DEPTH_STENCIL_VIEW_DESC> subresources_dsv;

		~Texture_DX12() override
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Sampler_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		D3D12_SAMPLER_DESC descriptor;

		~Sampler_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Query_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		GPU_QUERY_TYPE query_type = GPU_QUERY_TYPE_INVALID;
		uint32_t query_index = ~0;

		~Query_DX12()
		{
			if (query_index != ~0) 
			{
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				switch (query_type)
				{
				case wiGraphics::GPU_QUERY_TYPE_OCCLUSION:
				case wiGraphics::GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
					allocationhandler->destroyer_queries_occlusion.push_back(std::make_pair(query_index, framecount));
					break;
				case wiGraphics::GPU_QUERY_TYPE_TIMESTAMP:
					allocationhandler->destroyer_queries_timestamp.push_back(std::make_pair(query_index, framecount));
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

		struct Table
		{
			std::vector<D3D12_DESCRIPTOR_RANGE> resources;
			std::vector<D3D12_DESCRIPTOR_RANGE> samplers;
		};
		std::vector<Table> tables;

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

		~BVH_DX12() override
		{
		}
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

	void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::init(GraphicsDevice_DX12* device)
	{
		this->device = device;

		// Reset state to empty:
		reset();
	}
	void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::reset()
	{
		dirty_graphics_compute[0] = true;
		dirty_graphics_compute[1] = true;
		heaps_bound = false;
		currentheap_resource = 0;
		currentheap_sampler = 0;
		for (DescriptorHeap& heap : heaps_resource)
		{
			heap.ringOffset = 0;
		}
		for (DescriptorHeap& heap : heaps_sampler)
		{
			heap.ringOffset = 0;
		}
		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			tables[stage].reset();
		}
	}
	void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::create_or_bind_heaps_on_demand(CommandList cmd)
	{
		// Create new heaps if needed:
		if (currentheap_resource >= heaps_resource.size())
		{
			heaps_resource.emplace_back();
			DescriptorHeap& heap = heaps_resource.back();

			heap.heapDesc.NodeMask = 0;
			heap.heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heap.heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heap.heapDesc.NumDescriptors = 1024 * 256;
			HRESULT hr = device->device->CreateDescriptorHeap(&heap.heapDesc, IID_PPV_ARGS(&heap.heap_GPU));
			assert(SUCCEEDED(hr));


			// Save heap properties:
			heap.start_cpu = heap.heap_GPU->GetCPUDescriptorHandleForHeapStart();
			heap.start_gpu = heap.heap_GPU->GetGPUDescriptorHandleForHeapStart();

			heaps_bound = false;
		}
		if (currentheap_sampler >= heaps_sampler.size())
		{
			heaps_sampler.emplace_back();
			DescriptorHeap& heap = heaps_sampler.back();

			heap.heapDesc.NodeMask = 0;
			heap.heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			heap.heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heap.heapDesc.NumDescriptors = 1024;
			HRESULT hr = device->device->CreateDescriptorHeap(&heap.heapDesc, IID_PPV_ARGS(&heap.heap_GPU));
			assert(SUCCEEDED(hr));


			// Save heap properties:
			heap.start_cpu = heap.heap_GPU->GetCPUDescriptorHandleForHeapStart();
			heap.start_gpu = heap.heap_GPU->GetGPUDescriptorHandleForHeapStart();

			heaps_bound = false;
		}

		if (!heaps_bound)
		{
			ID3D12DescriptorHeap* heaps[] = {
				heaps_resource[currentheap_resource].heap_GPU.Get(), heaps_sampler[currentheap_sampler].heap_GPU.Get()
			};
			device->GetDirectCommandList(cmd)->SetDescriptorHeaps(arraysize(heaps), heaps);
		}
	}
	void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::validate(bool graphics, CommandList cmd)
	{
		if (graphics && !dirty_graphics_compute[0])
			return;
		if (graphics && device->active_pso[cmd] == nullptr)
			return;
		if (!graphics && !dirty_graphics_compute[1])
			return;
		if (!graphics && device->active_cs[cmd] == nullptr)
			return;

		create_or_bind_heaps_on_demand(cmd);

		UINT root_parameter_index = 0;

		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			Table& table = tables[stage];
			if (!dirty_graphics_compute[stage == CS])
				continue;
			if (graphics && stage == CS)
				continue;
			if (!graphics && stage != CS)
				continue;

			const Shader* shader = nullptr;
			switch (stage)
			{
			case VS:
				shader = device->active_pso[cmd]->desc.vs;
				break;
			case HS:
				shader = device->active_pso[cmd]->desc.hs;
				break;
			case DS:
				shader = device->active_pso[cmd]->desc.ds;
				break;
			case GS:
				shader = device->active_pso[cmd]->desc.gs;
				break;
			case PS:
				shader = device->active_pso[cmd]->desc.ps;
				break;
			case CS:
				shader = device->active_cs[cmd];
				break;
			}
			if (shader == nullptr)
			{
				continue;
			}
			auto shader_internal = to_internal(shader);

			for (auto& PSO_table : shader_internal->tables)
			{

				// Resources:
				if (!PSO_table.resources.empty())
				{
					DescriptorHeap& heap = heaps_resource[currentheap_resource];
					if ((heap.ringOffset + (uint32_t)PSO_table.resources.size()) >= heap.heapDesc.NumDescriptors)
					{
						// start new heap if the current one is full
						currentheap_resource++;
						heaps_bound = false;
						create_or_bind_heaps_on_demand(cmd);
					}

					D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
					D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
					srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					srv_desc.Format = DXGI_FORMAT_R32_UINT;
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

					D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
					uav_desc.Format = DXGI_FORMAT_R32_UINT;
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

					uint32_t index = 0;
					for (auto& x : PSO_table.resources)
					{
						D3D12_CPU_DESCRIPTOR_HANDLE dst = heap.start_cpu;
						dst.ptr += (heap.ringOffset + index) * device->resource_descriptor_size;

						switch (x.RangeType)
						{
						default:
						case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
						{
							const GPUResource* resource = table.SRV[x.BaseShaderRegister];
							const int subresource = table.SRV_index[x.BaseShaderRegister];
							if (resource == nullptr || !resource->IsValid())
							{
								device->device->CreateShaderResourceView(nullptr, &srv_desc, dst);
							}
							else
							{
								auto internal_state = to_internal(resource);

								if (resource->IsAccelerationStructure())
								{
									device->device->CreateShaderResourceView(nullptr, &internal_state->srv, dst);
								}
								else
								{
									if (subresource < 0)
									{
										device->device->CreateShaderResourceView(internal_state->resource.Get(), &internal_state->srv, dst);
									}
									else
									{
										device->device->CreateShaderResourceView(internal_state->resource.Get(), &internal_state->subresources_srv[subresource], dst);
									}
								}
							}
						}
						break;

						case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
						{
							const GPUResource* resource = table.UAV[x.BaseShaderRegister];
							const int subresource = table.UAV_index[x.BaseShaderRegister];
							if (resource == nullptr || !resource->IsValid())
							{
								device->device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, dst);
							}
							else
							{
								auto internal_state = to_internal(resource);

								D3D12_CPU_DESCRIPTOR_HANDLE src = {};
								if (subresource < 0)
								{
									device->device->CreateUnorderedAccessView(internal_state->resource.Get(), nullptr, &internal_state->uav, dst);
								}
								else
								{
									device->device->CreateUnorderedAccessView(internal_state->resource.Get(), nullptr, &internal_state->subresources_uav[subresource], dst);
								}

								if (src.ptr != 0)
								{
									device->device->CopyDescriptorsSimple(1, dst, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
								}

							}
						}
						break;

						case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
						{
							const GPUBuffer* buffer = table.CBV[x.BaseShaderRegister];
							if (buffer == nullptr || !buffer->IsValid())
							{
								device->device->CreateConstantBufferView(&cbv_desc, dst);
							}
							else
							{
								auto internal_state = to_internal(buffer);

								if (buffer->desc.Usage == USAGE_DYNAMIC)
								{
									auto it = device->dynamic_constantbuffers[cmd].find(buffer);
									if (it != device->dynamic_constantbuffers[cmd].end())
									{
										DynamicResourceState& state = it->second;
										state.binding[stage] = true;
										D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
										cbv.BufferLocation = to_internal(state.allocation.buffer)->resource->GetGPUVirtualAddress();
										cbv.BufferLocation += (D3D12_GPU_VIRTUAL_ADDRESS)state.allocation.offset;
										cbv.SizeInBytes = (uint32_t)Align((size_t)buffer->desc.ByteWidth, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

										device->device->CreateConstantBufferView(&cbv, dst);
									}
								}
								else
								{
									device->device->CreateConstantBufferView(&internal_state->cbv, dst);
								}
							}
						}
						break;
						}

						index++;
					}

					D3D12_GPU_DESCRIPTOR_HANDLE binding_table = heap.start_gpu;
					binding_table.ptr += (UINT64)heap.ringOffset * (UINT64)device->resource_descriptor_size;

					if (stage == CS)
					{
						device->GetDirectCommandList(cmd)->SetComputeRootDescriptorTable(root_parameter_index, binding_table);
					}
					else
					{
						device->GetDirectCommandList(cmd)->SetGraphicsRootDescriptorTable(root_parameter_index, binding_table);
					}

					heap.ringOffset += (uint32_t)PSO_table.resources.size();
					root_parameter_index++;
				}

				// Samplers:
				if (!PSO_table.samplers.empty())
				{
					DescriptorHeap& heap = heaps_sampler[currentheap_sampler];
					if ((heap.ringOffset + (uint32_t)PSO_table.samplers.size()) >= heap.heapDesc.NumDescriptors)
					{
						// start new heap if the current one is full
						currentheap_sampler++;
						heaps_bound = false;
						create_or_bind_heaps_on_demand(cmd);
					}

					D3D12_SAMPLER_DESC sampler_desc = {};
					sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
					sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

					size_t index = 0;
					for (auto& x : PSO_table.samplers)
					{
						D3D12_CPU_DESCRIPTOR_HANDLE dst = heap.start_cpu;
						dst.ptr += (heap.ringOffset + index) * device->sampler_descriptor_size;

						const Sampler* sampler = table.SAM[x.BaseShaderRegister];
						if (sampler == nullptr || !sampler->IsValid())
						{
							device->device->CreateSampler(&sampler_desc, dst);
						}
						else
						{
							auto internal_state = to_internal(sampler);
							device->device->CreateSampler(&internal_state->descriptor, dst);
						}

						index++;
					}

					D3D12_GPU_DESCRIPTOR_HANDLE binding_table = heap.start_gpu;
					binding_table.ptr += (UINT64)heap.ringOffset * (UINT64)device->sampler_descriptor_size;

					if (stage == CS)
					{
						device->GetDirectCommandList(cmd)->SetComputeRootDescriptorTable(root_parameter_index, binding_table);
					}
					else
					{
						device->GetDirectCommandList(cmd)->SetGraphicsRootDescriptorTable(root_parameter_index, binding_table);
					}

					heap.ringOffset += (uint32_t)PSO_table.samplers.size();
					root_parameter_index++;
				}

			}

		}

		dirty_graphics_compute[0] = false;
		dirty_graphics_compute[1] = false;
	}



	// Engine functions
	GraphicsDevice_DX12::GraphicsDevice_DX12(wiPlatform::window_type window, bool fullscreen, bool debuglayer)
	{
		SHADER_IDENTIFIER_SIZE = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

		DEBUGDEVICE = debuglayer;
		FULLSCREEN = fullscreen;

#ifndef PLATFORM_UWP
		RECT rect = RECT();
		GetClientRect(window, &rect);
		RESOLUTIONWIDTH = rect.right - rect.left;
		RESOLUTIONHEIGHT = rect.bottom - rect.top;
#else PLATFORM_UWP
		float dpiscale = wiPlatform::GetDPIScaling();
		RESOLUTIONWIDTH = int(window->Bounds.Width * dpiscale);
		RESOLUTIONHEIGHT = int(window->Bounds.Height * dpiscale);
#endif

		HRESULT hr = E_FAIL;

#if !defined(PLATFORM_UWP)
		if (debuglayer)
		{
			// Enable the debug layer.
			HMODULE dx12 = LoadLibraryEx(L"d3d12.dll",
				nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			auto pD3D12GetDebugInterface =
				reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(
					GetProcAddress(dx12, "D3D12GetDebugInterface"));
			if (pD3D12GetDebugInterface)
			{
				ID3D12Debug* debugController;
				if (SUCCEEDED(pD3D12GetDebugInterface(
					IID_PPV_ARGS(&debugController))))
				{
					debugController->EnableDebugLayer();
				}
			}
		}
#endif

		hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "Failed to create the graphics device! ERROR: " << std::hex << hr;
			wiHelper::messageBox(ss.str(), "Error!");
			assert(0);
			wiPlatform::Exit();
		}

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = device.Get();

		allocationhandler = std::make_shared<AllocationHandler>();
		allocationhandler->device = device;

		hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocationhandler->allocator);
		assert(SUCCEEDED(hr));

		// Create command queue
		D3D12_COMMAND_QUEUE_DESC directQueueDesc = {};
		directQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		directQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		directQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		directQueueDesc.NodeMask = 0;
		hr = device->CreateCommandQueue(&directQueueDesc, IID_PPV_ARGS(&directQueue));
		assert(SUCCEEDED(hr));

		// Create fences for command queue:
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFence));
		assert(SUCCEEDED(hr));
		frameFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);



		// Create swapchain

		ComPtr<IDXGIFactory4> pIDXGIFactory;
		hr = CreateDXGIFactory1(IID_PPV_ARGS(&pIDXGIFactory));
		assert(SUCCEEDED(hr));

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
		hr = pIDXGIFactory->CreateSwapChainForHwnd(directQueue.Get(), window, &sd, &fullscreenDesc, nullptr, &_swapChain);
#else
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
		sd.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

		hr = pIDXGIFactory->CreateSwapChainForCoreWindow(directQueue.Get(), reinterpret_cast<IUnknown*>(window.Get()), &sd, nullptr, &_swapChain);
#endif

		if (FAILED(hr))
		{
			wiHelper::messageBox("Failed to create a swapchain for the graphics device!", "Error!");
			assert(0);
			wiPlatform::Exit();
		}

		hr = _swapChain.As(&swapChain);
		assert(SUCCEEDED(hr));



		// Create common descriptor heaps

		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NodeMask = 0;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			heapDesc.NumDescriptors = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT * COMMANDLIST_COUNT;
			HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorheap_RTV));
			assert(SUCCEEDED(hr));
			rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			rtv_descriptor_heap_start = descriptorheap_RTV->GetCPUDescriptorHandleForHeapStart();
		}
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NodeMask = 0;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			heapDesc.NumDescriptors = COMMANDLIST_COUNT;
			HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorheap_DSV));
			assert(SUCCEEDED(hr));
			dsv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			dsv_descriptor_heap_start = descriptorheap_DSV->GetCPUDescriptorHandleForHeapStart();
		}

		resource_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		sampler_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);


		// Create frame-resident resources:
		for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
		{
			hr = swapChain->GetBuffer(fr, IID_PPV_ARGS(&frames[fr].backBuffer));
			assert(SUCCEEDED(hr));
		}


		// Create copy queue:
		{
			D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
			copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
			copyQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			copyQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			copyQueueDesc.NodeMask = 0;
			hr = device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&copyQueue));
			hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&copyAllocator));
			hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, copyAllocator.Get(), nullptr, IID_PPV_ARGS(&copyCommandList));

			hr = static_cast<ID3D12GraphicsCommandList*>(copyCommandList.Get())->Close();
			hr = copyAllocator->Reset();
			hr = static_cast<ID3D12GraphicsCommandList*>(copyCommandList.Get())->Reset(copyAllocator.Get(), nullptr);

			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyFence));
			copyFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
			copyFenceValue = 1;
		}

		// Query features:

		TESSELLATION = true;

		D3D12_FEATURE_DATA_D3D12_OPTIONS features_0;
		hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &features_0, sizeof(features_0));
		CONSERVATIVE_RASTERIZATION = features_0.ConservativeRasterizationTier >= D3D12_CONSERVATIVE_RASTERIZATION_TIER_1;
		RASTERIZER_ORDERED_VIEWS = features_0.ROVsSupported == TRUE;
		RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS = features_0.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation == TRUE;

		if (features_0.TypedUAVLoadAdditionalFormats)
		{
			// More info about UAV format load support: https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
			UAV_LOAD_FORMAT_COMMON = true;

			D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport = { DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
			hr = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
			if (SUCCEEDED(hr) && (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
			{
				UAV_LOAD_FORMAT_R11G11B10_FLOAT = true;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 features_5;
		hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features_5, sizeof(features_5));
		RAYTRACING = features_5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;


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

		// GPU Queries:
		{
			D3D12_QUERY_HEAP_DESC queryheapdesc = {};
			queryheapdesc.NodeMask = 0;

			for (uint32_t i = 0; i < timestamp_query_count; ++i)
			{
				allocationhandler->free_timestampqueries.push_back(i);
			}
			queryheapdesc.Count = timestamp_query_count;
			queryheapdesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			hr = device->CreateQueryHeap(&queryheapdesc, IID_PPV_ARGS(&querypool_timestamp));
			assert(SUCCEEDED(hr));

			for (uint32_t i = 0; i < occlusion_query_count; ++i)
			{
				allocationhandler->free_occlusionqueries.push_back(i);
			}
			queryheapdesc.Count = occlusion_query_count;
			queryheapdesc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			hr = device->CreateQueryHeap(&queryheapdesc, IID_PPV_ARGS(&querypool_occlusion));
			assert(SUCCEEDED(hr));


			D3D12MA::ALLOCATION_DESC allocationDesc = {};
			allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;

			CD3DX12_RESOURCE_DESC resdesc = CD3DX12_RESOURCE_DESC::Buffer(timestamp_query_count * sizeof(uint64_t));

			hr = allocationhandler->allocator->CreateResource(
				&allocationDesc,
				&resdesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				&allocation_querypool_timestamp_readback,
				IID_PPV_ARGS(&querypool_timestamp_readback)
			);
			assert(SUCCEEDED(hr));

			resdesc = CD3DX12_RESOURCE_DESC::Buffer(occlusion_query_count * sizeof(uint64_t));

			hr = allocationhandler->allocator->CreateResource(
				&allocationDesc,
				&resdesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				&allocation_querypool_occlusion_readback,
				IID_PPV_ARGS(&querypool_occlusion_readback)
			);
			assert(SUCCEEDED(hr));
		}

		wiBackLog::post("Created GraphicsDevice_DX12");
	}
	GraphicsDevice_DX12::~GraphicsDevice_DX12()
	{
		WaitForGPU();

		CloseHandle(frameFenceEvent);
		CloseHandle(copyFenceEvent);

		allocation_querypool_timestamp_readback->Release();
		allocation_querypool_occlusion_readback->Release();
	}

	void GraphicsDevice_DX12::SetResolution(int width, int height)
	{
		if ((width != RESOLUTIONWIDTH || height != RESOLUTIONHEIGHT) && width > 0 && height > 0)
		{
			RESOLUTIONWIDTH = width;
			RESOLUTIONHEIGHT = height;

			WaitForGPU();

			for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
			{
				frames[fr].backBuffer.Reset();
			}

			HRESULT hr = swapChain->ResizeBuffers(GetBackBufferCount(), width, height, _ConvertFormat(GetBackBufferFormat()), 0);
			assert(SUCCEEDED(hr));

			for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
			{
				hr = swapChain->GetBuffer(fr, IID_PPV_ARGS(&frames[fr].backBuffer));
				assert(SUCCEEDED(hr));
			}

			RESOLUTIONCHANGED = true;
		}
	}

	Texture GraphicsDevice_DX12::GetBackBuffer()
	{
		auto internal_state = std::make_shared<Texture_DX12>();
		internal_state->allocationhandler = allocationhandler;
		internal_state->resource = GetFrameResources().backBuffer;
		internal_state->rtv = {};
		internal_state->rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		Texture result;
		result.type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;
		result.internal_state = internal_state;
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

		uint32_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		}
		size_t alignedSize = Align(pDesc->ByteWidth, alignment);

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
			allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
			resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}

		hr = allocationhandler->allocator->CreateResource(
			&allocationDesc,
			&desc,
			resourceState,
			nullptr,
			&internal_state->allocation,
			IID_PPV_ARGS(&internal_state->resource)
		);
		assert(SUCCEEDED(hr));


		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			GPUBufferDesc uploaddesc;
			uploaddesc.ByteWidth = pDesc->ByteWidth;
			uploaddesc.Usage = USAGE_STAGING;
			GPUBuffer uploadbuffer;
			bool upload_success = CreateBuffer(&uploaddesc, nullptr, &uploadbuffer);
			assert(upload_success);
			ID3D12Resource* upload_resource = to_internal(&uploadbuffer)->resource.Get();

			void* pData;
			CD3DX12_RANGE readRange(0, 0);
			hr = upload_resource->Map(0, &readRange, &pData);
			assert(SUCCEEDED(hr));

			copyQueueLock.lock();
			{
				memcpy(pData, pInitialData->pSysMem, pDesc->ByteWidth);
				static_cast<ID3D12GraphicsCommandList*>(copyCommandList.Get())->CopyBufferRegion(
					internal_state->resource.Get(), 0, upload_resource, 0, pDesc->ByteWidth);
			}
			copyQueueLock.unlock();
		}


		// Create resource views if needed
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
			cbv_desc.SizeInBytes = (uint32_t)alignedSize;
			cbv_desc.BufferLocation = internal_state->resource->GetGPUVirtualAddress();

			internal_state->cbv = cbv_desc;
		}

		if (pDesc->BindFlags & BIND_SHADER_RESOURCE)
		{

			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer

				srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = 0;
				srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
				srv_desc.Buffer.NumElements = pDesc->ByteWidth / 4;
			}
			else if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer

				srv_desc.Format = DXGI_FORMAT_UNKNOWN;
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = 0;
				srv_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
				srv_desc.Buffer.StructureByteStride = pDesc->StructureByteStride;
			}
			else
			{
				// This is a Typed Buffer

				srv_desc.Format = _ConvertFormat(pDesc->Format);
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = 0;
				srv_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
			}

			internal_state->srv = srv_desc;
		}

		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;

			if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer

				uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
				uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
				uav_desc.Buffer.FirstElement = 0;
				uav_desc.Buffer.NumElements = pDesc->ByteWidth / 4;
			}
			else if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer

				uav_desc.Format = DXGI_FORMAT_UNKNOWN;
				uav_desc.Buffer.FirstElement = 0;
				uav_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
				uav_desc.Buffer.StructureByteStride = pDesc->StructureByteStride;
			}
			else
			{
				// This is a Typed Buffer

				uav_desc.Format = _ConvertFormat(pDesc->Format);
				uav_desc.Buffer.FirstElement = 0;
				uav_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
			}

			internal_state->uav = uav_desc;
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

		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.CreationNodeMask = 0;
		heapDesc.VisibleNodeMask = 0;

		D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;

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
		}
		else if (desc.SampleDesc.Count == 1)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		}
		if (pDesc->BindFlags & BIND_RENDER_TARGET)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
		}
		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!(pDesc->BindFlags & BIND_SHADER_RESOURCE))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		D3D12_RESOURCE_STATES resourceState = _ConvertImageLayout(pTexture->desc.layout);

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
			pTexture->desc.MipLevels = (uint32_t)log2(std::max(pTexture->desc.Width, pTexture->desc.Height));
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

			uint32_t FirstSubresource = 0;

			UINT64 RequiredSize = 0;
			device->GetCopyableFootprints(&desc, 0, dataCount, 0, nullptr, nullptr, nullptr, &RequiredSize);


			copyQueueLock.lock();
			{
				GPUBufferDesc uploaddesc;
				uploaddesc.ByteWidth = (uint32_t)RequiredSize;
				uploaddesc.Usage = USAGE_STAGING;
				GPUBuffer uploadbuffer;
				bool upload_success = CreateBuffer(&uploaddesc, nullptr, &uploadbuffer);
				assert(upload_success);
				ID3D12Resource* upload_resource = to_internal(&uploadbuffer)->resource.Get();

				void* pData;
				CD3DX12_RANGE readRange(0, 0);
				hr = upload_resource->Map(0, &readRange, &pData);
				assert(SUCCEEDED(hr));
				
				UINT64 dataSize = UpdateSubresources(static_cast<ID3D12GraphicsCommandList*>(copyCommandList.Get()), internal_state->resource.Get(),
					upload_resource, 0, 0, dataCount, data.data());
			}
			copyQueueLock.unlock();
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
	bool GraphicsDevice_DX12::CreateInputLayout(const InputLayoutDesc* pInputElementDescs, uint32_t NumElements, const Shader* shader, InputLayout* pInputLayout)
	{
		pInputLayout->internal_state = allocationhandler;

		pInputLayout->desc.clear();
		pInputLayout->desc.reserve((size_t)NumElements);
		for (uint32_t i = 0; i < NumElements; ++i)
		{
			pInputLayout->desc.push_back(pInputElementDescs[i]);
		}

		return true;
	}
	bool GraphicsDevice_DX12::CreateShader(SHADERSTAGE stage, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader)
	{
		auto internal_state = std::make_shared<PipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pShader->internal_state = internal_state;

		pShader->code.resize(BytecodeLength);
		std::memcpy(pShader->code.data(), pShaderBytecode, BytecodeLength);
		pShader->stage = stage;

		HRESULT hr = (pShader->code.empty() ? E_FAIL : S_OK);
		assert(SUCCEEDED(hr));


#ifndef PLATFORM_UWP // TODO: Can't use dxcompiler.dll in UWP, so can't use shader reflection
		struct ShaderBlob : public IDxcBlob
		{
			LPVOID address;
			SIZE_T size;
			LPVOID STDMETHODCALLTYPE GetBufferPointer() override { return address; }
			SIZE_T STDMETHODCALLTYPE GetBufferSize() override { return size; }
			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) { return E_FAIL; }
			ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
			ULONG STDMETHODCALLTYPE Release(void) { return 0; }
		};
		ShaderBlob blob;
		blob.address = (LPVOID)pShaderBytecode;
		blob.size = BytecodeLength;

		ComPtr<IDxcContainerReflection> container_reflection;
		hr = DxcCreateInstance(CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), (void**)&container_reflection);
		assert(SUCCEEDED(hr));
		hr = container_reflection->Load(&blob);
		assert(SUCCEEDED(hr));

		UINT32 shaderIdx;
		hr = container_reflection->FindFirstPartKind('LIXD', &shaderIdx); // Say 'DXIL' in Little-Endian
		assert(SUCCEEDED(hr));

		auto instert_descriptor = [&](PipelineState_DX12::Table& table, const D3D12_SHADER_INPUT_BIND_DESC& desc)
		{
			if (desc.Type == D3D_SIT_SAMPLER)
			{
				table.samplers.emplace_back();
				D3D12_DESCRIPTOR_RANGE& descriptor = table.samplers.back();

				descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
				descriptor.BaseShaderRegister = desc.BindPoint;
				descriptor.NumDescriptors = 1;
				descriptor.RegisterSpace = 0;
				descriptor.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			}
			else
			{
				table.resources.emplace_back();
				D3D12_DESCRIPTOR_RANGE& descriptor = table.resources.back();

				switch (desc.Type)
				{
				default:
				case D3D_SIT_CBUFFER:
					descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
					break;
				case D3D_SIT_TBUFFER:
				case D3D_SIT_TEXTURE:
				case D3D_SIT_STRUCTURED:
				case D3D_SIT_BYTEADDRESS:
				case D3D_SIT_RTACCELERATIONSTRUCTURE:
					descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					break;
				case D3D_SIT_UAV_RWTYPED:
				case D3D_SIT_UAV_RWSTRUCTURED:
				case D3D_SIT_UAV_RWBYTEADDRESS:
				case D3D_SIT_UAV_APPEND_STRUCTURED:
				case D3D_SIT_UAV_CONSUME_STRUCTURED:
				case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
				case D3D_SIT_UAV_FEEDBACKTEXTURE:
					descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
					break;
				}
				descriptor.BaseShaderRegister = desc.BindPoint;
				descriptor.NumDescriptors = 1;
				descriptor.RegisterSpace = 0;
				descriptor.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			}
		};

		if (stage == SHADERSTAGE_COUNT) // Library reflection
		{
			ComPtr<ID3D12LibraryReflection> reflection;
			hr = container_reflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&reflection));
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

				internal_state->tables.emplace_back();
				PipelineState_DX12::Table& table = internal_state->tables.back();

				for (UINT i = 0; i < function_desc.BoundResources; ++i)
				{
					D3D12_SHADER_INPUT_BIND_DESC desc;
					hr = function_reflection->GetResourceBindingDesc(i, &desc);
					assert(SUCCEEDED(hr));
					instert_descriptor(table, desc);
				}
			}
		}
		else // Shader reflection
		{
			ComPtr<ID3D12ShaderReflection> reflection;
			hr = container_reflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&reflection));
			assert(SUCCEEDED(hr));

			D3D12_SHADER_DESC shader_desc;
			hr = reflection->GetDesc(&shader_desc);
			assert(SUCCEEDED(hr));

			internal_state->tables.emplace_back();
			PipelineState_DX12::Table& table = internal_state->tables.back();

			for (UINT i = 0; i < shader_desc.BoundResources; ++i)
			{
				D3D12_SHADER_INPUT_BIND_DESC desc;
				hr = reflection->GetResourceBindingDesc(i, &desc);
				assert(SUCCEEDED(hr));
				instert_descriptor(table, desc);
			}
		}
#endif // PLATFORM_UWP


		if (stage == CS || stage == SHADERSTAGE_COUNT)
		{
			std::vector<D3D12_ROOT_PARAMETER> params;

			for (auto& table : internal_state->tables)
			{
				if (!table.resources.empty())
				{
					params.emplace_back();
					D3D12_ROOT_PARAMETER& param = params.back();
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.DescriptorTable.NumDescriptorRanges = (UINT)table.resources.size();
					param.DescriptorTable.pDescriptorRanges = table.resources.data();
				}

				if (!table.samplers.empty())
				{
					params.emplace_back();
					D3D12_ROOT_PARAMETER& param = params.back();
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.DescriptorTable.NumDescriptorRanges = (UINT)table.samplers.size();
					param.DescriptorTable.pDescriptorRanges = table.samplers.data();
				}
			}

			D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
			rootSigDesc.NumStaticSamplers = 0;
			rootSigDesc.NumParameters = (UINT)params.size();
			rootSigDesc.pParameters = params.data();
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

			ID3DBlob* rootSigBlob;
			ID3DBlob* rootSigError;
			hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &rootSigError);
			if (FAILED(hr))
			{
				assert(0);
				OutputDebugStringA((char*)rootSigError->GetBufferPointer());
			}
			hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&internal_state->rootSignature));
			assert(SUCCEEDED(hr));

			if (stage == CS)
			{
				D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
				desc.CS.pShaderBytecode = pShader->code.data();
				desc.CS.BytecodeLength = pShader->code.size();
				desc.pRootSignature = internal_state->rootSignature.Get();

				hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&internal_state->resource));
				assert(SUCCEEDED(hr));
			}
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateBlendState(const BlendStateDesc* pBlendStateDesc, BlendState* pBlendState)
	{
		pBlendState->internal_state = allocationhandler;

		pBlendState->desc = *pBlendStateDesc;
		return true;
	}
	bool GraphicsDevice_DX12::CreateDepthStencilState(const DepthStencilStateDesc* pDepthStencilStateDesc, DepthStencilState* pDepthStencilState)
	{
		pDepthStencilState->internal_state = allocationhandler;

		pDepthStencilState->desc = *pDepthStencilStateDesc;
		return true;
	}
	bool GraphicsDevice_DX12::CreateRasterizerState(const RasterizerStateDesc* pRasterizerStateDesc, RasterizerState* pRasterizerState)
	{
		pRasterizerState->internal_state = allocationhandler;

		pRasterizerState->desc = *pRasterizerStateDesc;
		return true;
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

		internal_state->descriptor = desc;

		return true;
	}
	bool GraphicsDevice_DX12::CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery)
	{
		auto internal_state = std::make_shared<Query_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pQuery->internal_state = internal_state;

		HRESULT hr = E_FAIL;

		pQuery->desc = *pDesc;
		internal_state->query_type = pQuery->desc.Type;

		switch (pDesc->Type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			if (allocationhandler->free_timestampqueries.pop_front(internal_state->query_index))
			{
				hr = S_OK;
			}
			else
			{
				internal_state->query_type = GPU_QUERY_TYPE_INVALID;
				assert(0);
			}
			break;
		case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
			hr = S_OK;
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			if (allocationhandler->free_occlusionqueries.pop_front(internal_state->query_index))
			{
				hr = S_OK;
			}
			else
			{
				internal_state->query_type = GPU_QUERY_TYPE_INVALID;
				assert(0);
			}
			break;
		}

		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso)
	{
		auto internal_state = std::make_shared<PipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pso->internal_state = internal_state;

		pso->desc = *pDesc;

		pso->hash = 0;
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


		std::vector<D3D12_ROOT_PARAMETER> params;

		auto insert_shader = [&](const Shader* shader, D3D12_SHADER_VISIBILITY visibility)
		{
			if (shader == nullptr)
				return;

			auto internal_state = to_internal(shader);

			if (!internal_state->tables.back().resources.empty())
			{
				params.emplace_back();
				D3D12_ROOT_PARAMETER& param = params.back();
				param = {};
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.ShaderVisibility = visibility;
				param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->tables.back().resources.size();
				param.DescriptorTable.pDescriptorRanges = internal_state->tables.back().resources.data();
			}

			if (!internal_state->tables.back().samplers.empty())
			{
				params.emplace_back();
				D3D12_ROOT_PARAMETER& param = params.back();
				param = {};
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.ShaderVisibility = visibility;
				param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->tables.back().samplers.size();
				param.DescriptorTable.pDescriptorRanges = internal_state->tables.back().samplers.data();
			}
		};

		insert_shader(pDesc->vs, D3D12_SHADER_VISIBILITY_VERTEX);
		insert_shader(pDesc->hs, D3D12_SHADER_VISIBILITY_HULL);
		insert_shader(pDesc->ds, D3D12_SHADER_VISIBILITY_DOMAIN);
		insert_shader(pDesc->gs, D3D12_SHADER_VISIBILITY_GEOMETRY);
		insert_shader(pDesc->ps, D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumStaticSamplers = 0;
		rootSigDesc.NumParameters = (UINT)params.size();
		rootSigDesc.pParameters = params.data();
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ID3DBlob* rootSigBlob;
		ID3DBlob* rootSigError;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &rootSigError);
		if (FAILED(hr))
		{
			assert(0);
			OutputDebugStringA((char*)rootSigError->GetBufferPointer());
		}
		hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&internal_state->rootSignature));
		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass)
	{
		renderpass->internal_state = allocationhandler;

		renderpass->desc = *pDesc;

		renderpass->hash = 0;
		wiHelper::hash_combine(renderpass->hash, pDesc->numAttachments);
		for (uint32_t i = 0; i < pDesc->numAttachments; ++i)
		{
			wiHelper::hash_combine(renderpass->hash, pDesc->attachments[i].texture->desc.Format);
			wiHelper::hash_combine(renderpass->hash, pDesc->attachments[i].texture->desc.SampleCount);
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
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					geometry.Triangles.VertexBuffer.StartAddress = to_internal(&x.triangles.vertexBuffer)->resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.vertexByteOffset;
					geometry.Triangles.VertexBuffer.StrideInBytes = (UINT64)x.triangles.vertexStride;
					geometry.Triangles.VertexCount = x.triangles.vertexCount;
					geometry.Triangles.VertexFormat = _ConvertFormat(x.triangles.vertexFormat);
					geometry.Triangles.IndexFormat = (x.triangles.indexFormat == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
					geometry.Triangles.IndexBuffer = to_internal(&x.triangles.indexBuffer)->resource->GetGPUVirtualAddress() +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.indexOffset * (x.triangles.indexFormat == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? sizeof(uint16_t) : sizeof(uint32_t));
					geometry.Triangles.IndexCount = x.triangles.indexCount;

					if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.Triangles.Transform3x4 = to_internal(&x.triangles.transform3x4Buffer)->resource->GetGPUVirtualAddress() +
							(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.transform3x4BufferOffset;
					}
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
				{
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS; 
					geometry.AABBs.AABBs.StartAddress = to_internal(&x.aabbs.aabbBuffer)->resource->GetGPUVirtualAddress() +
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

			internal_state->desc.InstanceDescs = to_internal(&pDesc->toplevel.instanceBuffer)->resource->GetGPUVirtualAddress() +
				(D3D12_GPU_VIRTUAL_ADDRESS)pDesc->toplevel.offset;
			internal_state->desc.NumDescs = (UINT)pDesc->toplevel.count;
		}
		break;
		}

		device->GetRaytracingAccelerationStructurePrebuildInfo(&internal_state->desc, &internal_state->info);


		uint32_t alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
		size_t alignedSize = Align(internal_state->info.ResultDataMaxSizeInBytes, alignment);

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
		allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

		HRESULT hr = allocationhandler->allocator->CreateResource(
			&allocationDesc,
			&desc,
			resourceState,
			nullptr,
			&internal_state->allocation,
			IID_PPV_ARGS(&internal_state->resource)
		);
		assert(SUCCEEDED(hr));

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		srv_desc.RaytracingAccelerationStructure.Location = internal_state->resource->GetGPUVirtualAddress();

		internal_state->srv = srv_desc;

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

			auto shader_internal = to_internal(pDesc->shaderlibraries.front().shader); // think better way
			subobjects.emplace_back();
			auto& subobject = subobjects.back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
			global_rootsig.pGlobalRootSignature = shader_internal->rootSignature.Get();
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
			library_desc.DXILLibrary.pShaderBytecode = x.shader->code.data();
			library_desc.DXILLibrary.BytecodeLength = x.shader->code.size();
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
				hitgroup_desc.ClosestHitShaderImport = internal_state->exports[x.anyhit_shader].Name;
			}
			if (x.intersection_shader != ~0)
			{
				hitgroup_desc.ClosestHitShaderImport = internal_state->exports[x.intersection_shader].Name;
			}
			subobject.pDesc = &hitgroup_desc;
		}

		desc.NumSubobjects = (UINT)subobjects.size();
		desc.pSubobjects = subobjects.data();

		HRESULT hr = device->CreateStateObject(&desc, IID_PPV_ARGS(&internal_state->resource));
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

			if (internal_state->srv.ViewDimension == D3D12_SRV_DIMENSION_UNKNOWN)
			{
				internal_state->srv = srv_desc;
				return -1;
			}
			internal_state->subresources_srv.push_back(srv_desc);
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

			if (internal_state->uav.ViewDimension == D3D12_UAV_DIMENSION_UNKNOWN)
			{
				internal_state->uav = uav_desc;
				return -1;
			}
			internal_state->subresources_uav.push_back(uav_desc);
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

			if (internal_state->rtv.ViewDimension == D3D12_RTV_DIMENSION_UNKNOWN)
			{
				internal_state->rtv = rtv_desc;
				return -1;
			}
			internal_state->subresources_rtv.push_back(rtv_desc);
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

			if (internal_state->dsv.ViewDimension == D3D12_DSV_DIMENSION_UNKNOWN)
			{
				internal_state->dsv = dsv_desc;
				return -1;
			}
			internal_state->subresources_dsv.push_back(dsv_desc);
			return int(internal_state->subresources_dsv.size() - 1);
		}
		break;
		default:
			break;
		}
		return -1;
	}

	void GraphicsDevice_DX12::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest)
	{
		D3D12_RAYTRACING_INSTANCE_DESC* desc = (D3D12_RAYTRACING_INSTANCE_DESC*)dest;
		desc->AccelerationStructure = to_internal(&instance->bottomlevel)->resource->GetGPUVirtualAddress();
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

	bool GraphicsDevice_DX12::DownloadResource(const GPUResource* resourceToDownload, const GPUResource* resourceDest, void* dataDest)
	{
		return false;
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
		barrier.Transition.pResource = GetFrameResources().backBuffer.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		GetDirectCommandList(cmd)->ResourceBarrier(1, &barrier);

		const float clearcolor[] = { 0,0,0,1 };
		device->CreateRenderTargetView(GetFrameResources().backBuffer.Get(), nullptr, rtv_descriptor_heap_start);

#ifdef DX12_REAL_RENDERPASS

		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTV = {};
		RTV.cpuDescriptor = rtv_descriptor_heap_start;
		RTV.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		RTV.BeginningAccess.Clear.ClearValue.Color[0] = clearcolor[0];
		RTV.BeginningAccess.Clear.ClearValue.Color[1] = clearcolor[1];
		RTV.BeginningAccess.Clear.ClearValue.Color[2] = clearcolor[2];
		RTV.BeginningAccess.Clear.ClearValue.Color[3] = clearcolor[3];
		RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		GetDirectCommandList(cmd)->BeginRenderPass(1, &RTV, nullptr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);

#else

		GetDirectCommandList(cmd)->OMSetRenderTargets(1, &rtv_descriptor_heap_start, TRUE, nullptr);
		GetDirectCommandList(cmd)->ClearRenderTargetView(rtv_descriptor_heap_start, clearcolor, 0, nullptr);

#endif // DX12_REAL_RENDERPASS

	}
	void GraphicsDevice_DX12::PresentEnd(CommandList cmd)
	{
#ifdef DX12_REAL_RENDERPASS
		GetDirectCommandList(cmd)->EndRenderPass();
#endif // DX12_REAL_RENDERPASS

		HRESULT result;

		// Indicate that the back buffer will now be used to present.
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = GetFrameResources().backBuffer.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		GetDirectCommandList(cmd)->ResourceBarrier(1, &barrier);



		// Sync up copy queue:
		copyQueueLock.lock();
		{
			static_cast<ID3D12GraphicsCommandList*>(copyCommandList.Get())->Close();
			ID3D12CommandList* commandlists[] = {
				copyCommandList.Get()
			};
			copyQueue->ExecuteCommandLists(1, commandlists);

			// Signal and increment the fence value.
			UINT64 fenceToWaitFor = copyFenceValue;
			HRESULT result = copyQueue->Signal(copyFence.Get(), fenceToWaitFor);
			copyFenceValue++;

			// Wait until the GPU is done copying.
			if (copyFence->GetCompletedValue() < fenceToWaitFor)
			{
				result = copyFence->SetEventOnCompletion(fenceToWaitFor, copyFenceEvent);
				WaitForSingleObject(copyFenceEvent, INFINITE);
			}

			result = copyAllocator->Reset();
			result = static_cast<ID3D12GraphicsCommandList*>(copyCommandList.Get())->Reset(copyAllocator.Get(), nullptr);

		}
		copyQueueLock.unlock();

		// Execute deferred command lists:
		{
			ID3D12CommandList* cmdLists[COMMANDLIST_COUNT];
			CommandList cmds[COMMANDLIST_COUNT];
			uint32_t counter = 0;

			CommandList cmd;
			while (active_commandlists.pop_front(cmd))
			{
				// Perform query resolves (must be outside of render pass):
				for (auto& x : query_resolves[cmd])
				{
					switch (x.type)
					{
					case GPU_QUERY_TYPE_TIMESTAMP:
						GetDirectCommandList(cmd)->ResolveQueryData(querypool_timestamp.Get(), D3D12_QUERY_TYPE_TIMESTAMP, x.index, 1, querypool_timestamp_readback.Get(), (uint64_t)x.index * sizeof(uint64_t));
						break;
					case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
						GetDirectCommandList(cmd)->ResolveQueryData(querypool_occlusion.Get(), D3D12_QUERY_TYPE_BINARY_OCCLUSION, x.index, 1, querypool_occlusion_readback.Get(), (uint64_t)x.index * sizeof(uint64_t));
						break;
					case GPU_QUERY_TYPE_OCCLUSION:
						GetDirectCommandList(cmd)->ResolveQueryData(querypool_occlusion.Get(), D3D12_QUERY_TYPE_OCCLUSION, x.index, 1, querypool_occlusion_readback.Get(), (uint64_t)x.index * sizeof(uint64_t));
						break;
					}
				}
				query_resolves[cmd].clear();

				HRESULT hr = GetDirectCommandList(cmd)->Close();
				assert(SUCCEEDED(hr));

				cmdLists[counter] = GetDirectCommandList(cmd);
				cmds[counter] = cmd;
				counter++;

				free_commandlists.push_back(cmd);

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


		swapChain->Present(VSYNC, 0);



		// This acts as a barrier, following this we will be using the next frame's resources when calling GetFrameResources()!
		FRAMECOUNT++;



		// Record the latest processed framecount in the fence
		result = directQueue->Signal(frameFence.Get(), FRAMECOUNT);

		// Determine the last frame that we should not wait on:
		const uint64_t lastFrameToAllowLatency = std::max(uint64_t(BACKBUFFER_COUNT - 1u), FRAMECOUNT) - (BACKBUFFER_COUNT - 1);

		// Wait if too many frames are being incomplete:
		if (frameFence->GetCompletedValue() < lastFrameToAllowLatency)
		{
			result = frameFence->SetEventOnCompletion(lastFrameToAllowLatency, frameFenceEvent);
			WaitForSingleObject(frameFenceEvent, INFINITE);
		}

		memset(prev_pt, 0, sizeof(prev_pt));

		allocationhandler->Update(FRAMECOUNT, BACKBUFFER_COUNT);

		RESOLUTIONCHANGED = false;
	}

	CommandList GraphicsDevice_DX12::BeginCommandList()
	{
		CommandList cmd;
		if (!free_commandlists.pop_front(cmd))
		{
			// need to create one more command list:
			cmd = commandlist_count.fetch_add(1);
			assert(cmd < COMMANDLIST_COUNT);

			HRESULT hr;
			for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
			{
				hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frames[fr].commandAllocators[cmd]));
				hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames[fr].commandAllocators[cmd].Get(), nullptr, IID_PPV_ARGS(&frames[fr].commandLists[cmd]));
				hr = static_cast<ID3D12GraphicsCommandList5*>(frames[fr].commandLists[cmd].Get())->Close();

				frames[fr].descriptors[cmd].init(this);
				frames[fr].resourceBuffer[cmd].init(this, 1024 * 1024); // 1 MB starting size
			}
		}


		// Start the command list in a default state:

		HRESULT hr = GetFrameResources().commandAllocators[cmd]->Reset();
		assert(SUCCEEDED(hr));
		hr = GetDirectCommandList(cmd)->Reset(GetFrameResources().commandAllocators[cmd].Get(), nullptr);
		assert(SUCCEEDED(hr));

		GetFrameResources().descriptors[cmd].reset();
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

		prev_pipeline_hash[cmd] = 0;
		active_pso[cmd] = nullptr;
		active_cs[cmd] = nullptr;
		active_renderpass[cmd] = nullptr;

		active_commandlists.push_back(cmd);
		return cmd;
	}

	void GraphicsDevice_DX12::WaitForGPU()
	{
		if (frameFence->GetCompletedValue() < FRAMECOUNT)
		{
			HRESULT result = frameFence->SetEventOnCompletion(FRAMECOUNT, frameFenceEvent);
			WaitForSingleObject(frameFenceEvent, INFINITE);
		}
	}
	void GraphicsDevice_DX12::ClearPipelineStateCache()
	{
		allocationhandler->destroylocker.lock();
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

		D3D12_CPU_DESCRIPTOR_HANDLE descriptors_RTV = rtv_descriptor_heap_start;
		descriptors_RTV.ptr += rtv_descriptor_size * D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT * cmd;

		D3D12_CPU_DESCRIPTOR_HANDLE descriptors_DSV = dsv_descriptor_heap_start;
		descriptors_DSV.ptr += dsv_descriptor_size * cmd;

		const RenderPassDesc& desc = renderpass->GetDesc();

#ifdef DX12_REAL_RENDERPASS

		uint32_t rt_count = 0;
		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTVs[8] = {};
		bool dsv = false;
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DSV = {};
		for (uint32_t i = 0; i < desc.numAttachments; ++i)
		{
			const RenderPassAttachment& attachment = desc.attachments[i];
			const Texture* texture = attachment.texture;
			int subresource = attachment.subresource;
			auto internal_state = to_internal(texture);

			D3D12_CLEAR_VALUE clear_value;
			clear_value.Format = _ConvertFormat(texture->desc.Format);

			if (attachment.type == RenderPassAttachment::RENDERTARGET)
			{
				RTVs[rt_count].cpuDescriptor = descriptors_RTV;
				RTVs[rt_count].cpuDescriptor.ptr += rtv_descriptor_size * rt_count;

				if (subresource < 0 || internal_state->subresources_rtv.empty())
				{
					device->CreateRenderTargetView(internal_state->resource.Get(), &internal_state->rtv, RTVs[rt_count].cpuDescriptor);
				}
				else
				{
					assert(internal_state->subresources_rtv.size() > size_t(subresource) && "Invalid RTV subresource!");
					device->CreateRenderTargetView(internal_state->resource.Get(), &internal_state->subresources_rtv[subresource], RTVs[rt_count].cpuDescriptor);
				}

				switch (attachment.loadop)
				{
				default:
				case RenderPassAttachment::LOADOP_LOAD:
					RTVs[rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::LOADOP_CLEAR:
					RTVs[rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					clear_value.Color[0] = texture->desc.clear.color[0];
					clear_value.Color[1] = texture->desc.clear.color[1];
					clear_value.Color[2] = texture->desc.clear.color[2];
					clear_value.Color[3] = texture->desc.clear.color[3];
					RTVs[rt_count].BeginningAccess.Clear.ClearValue = clear_value;
					break;
				case RenderPassAttachment::LOADOP_DONTCARE:
					RTVs[rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					break;
				}

				switch (attachment.storeop)
				{
				default:
				case RenderPassAttachment::STOREOP_STORE:
					RTVs[rt_count].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::STOREOP_DONTCARE:
					RTVs[rt_count].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					break;
				}

				rt_count++;
			}
			else
			{
				dsv = true;

				DSV.cpuDescriptor = descriptors_DSV;

				if (subresource < 0 || internal_state->subresources_dsv.empty())
				{
					device->CreateDepthStencilView(internal_state->resource.Get(), &internal_state->dsv, DSV.cpuDescriptor);
				}
				else
				{
					assert(internal_state->subresources_dsv.size() > size_t(subresource) && "Invalid DSV subresource!");
					device->CreateDepthStencilView(internal_state->resource.Get(), &internal_state->subresources_dsv[subresource], DSV.cpuDescriptor);
				}

				switch (attachment.loadop)
				{
				default:
				case RenderPassAttachment::LOADOP_LOAD:
					DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::LOADOP_CLEAR:
					DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					clear_value.DepthStencil.Depth = texture->desc.clear.depthstencil.depth;
					clear_value.DepthStencil.Stencil = texture->desc.clear.depthstencil.stencil;
					DSV.DepthBeginningAccess.Clear.ClearValue = clear_value;
					DSV.StencilBeginningAccess.Clear.ClearValue = clear_value;
					break;
				case RenderPassAttachment::LOADOP_DONTCARE:
					DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					break;
				}

				switch (attachment.storeop)
				{
				default:
				case RenderPassAttachment::STOREOP_STORE:
					DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::STOREOP_DONTCARE:
					DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					break;
				}
			}
		}

		GetDirectCommandList(cmd)->BeginRenderPass(rt_count, RTVs, dsv ? &DSV : nullptr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);

#else

		uint32_t rt_count = 0;
		D3D12_RENDER_TARGET_VIEW_DESC RTVs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		D3D12_DEPTH_STENCIL_VIEW_DESC* DSV = nullptr;
		for (uint32_t i = 0; i < desc.numAttachments; ++i)
		{
			const RenderPassAttachment& attachment = desc.attachments[i];
			const Texture* texture = attachment.texture;
			int subresource = attachment.subresource;
			auto internal_state = to_internal(texture);

			if (attachment.type == RenderPassAttachment::RENDERTARGET)
			{
				if (subresource < 0 || internal_state->subresources_rtv.empty())
				{
					RTVs[rt_count] = internal_state->rtv;
				}
				else
				{
					assert(internal_state->subresources_rtv.size() > size_t(subresource) && "Invalid RTV subresource!");
					RTVs[rt_count] = internal_state->subresources_rtv[subresource];
				}

				D3D12_CPU_DESCRIPTOR_HANDLE descriptor = descriptors_RTV;
				descriptor.ptr += rtv_descriptor_size * rt_count;
				device->CreateRenderTargetView(internal_state->resource.Get(), &RTVs[rt_count], descriptor);

				if (attachment.loadop == RenderPassAttachment::LOADOP_CLEAR)
				{
					GetDirectCommandList(cmd)->ClearRenderTargetView(descriptor, texture->desc.clear.color, 0, nullptr);
				}

				rt_count++;
			}
			else
			{
				if (subresource < 0 || internal_state->subresources_dsv.empty())
				{
					DSV = &internal_state->dsv;
				}
				else
				{
					assert(internal_state->subresources_dsv.size() > size_t(subresource) && "Invalid DSV subresource!");
					DSV = &internal_state->subresources_dsv[subresource];
				}

				D3D12_CPU_DESCRIPTOR_HANDLE descriptor = descriptors_DSV;
				device->CreateDepthStencilView(internal_state->resource.Get(), DSV, descriptor);

				if (attachment.loadop == RenderPassAttachment::LOADOP_CLEAR)
				{
					uint32_t _flags = D3D12_CLEAR_FLAG_DEPTH;
					if (IsFormatStencilSupport(texture->desc.Format))
						_flags |= D3D12_CLEAR_FLAG_STENCIL;
					GetDirectCommandList(cmd)->ClearDepthStencilView(descriptor, (D3D12_CLEAR_FLAGS)_flags, texture->desc.clear.depthstencil.depth, texture->desc.clear.depthstencil.stencil, 0, nullptr);
				}
			}
		}

		GetDirectCommandList(cmd)->OMSetRenderTargets(rt_count, &descriptors_RTV, TRUE, DSV == nullptr ? nullptr : &descriptors_DSV);

#endif // DX12_REAL_RENDERPASS
	}
	void GraphicsDevice_DX12::RenderPassEnd(CommandList cmd)
	{
#ifdef DX12_REAL_RENDERPASS
		GetDirectCommandList(cmd)->EndRenderPass();
#else
		GetDirectCommandList(cmd)->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
#endif // DX12_REAL_RENDERPASS


		// Perform render pass transitions:
		D3D12_RESOURCE_BARRIER barrierdescs[9];
		uint32_t numBarriers = 0;
		for (uint32_t i = 0; i < active_renderpass[cmd]->desc.numAttachments; ++i)
		{
			const RenderPassAttachment& attachment = active_renderpass[cmd]->desc.attachments[i];
			if (attachment.initial_layout == attachment.final_layout)
			{
				continue;
			}
			auto internal_state = to_internal(attachment.texture);

			D3D12_RESOURCE_BARRIER& barrierdesc = barrierdescs[numBarriers++];

			barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrierdesc.Transition.pResource = internal_state->resource.Get();
			barrierdesc.Transition.StateBefore = _ConvertImageLayout(attachment.initial_layout);
			barrierdesc.Transition.StateAfter = _ConvertImageLayout(attachment.final_layout);
			barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		}
		if (numBarriers > 0)
		{
			GetDirectCommandList(cmd)->ResourceBarrier(numBarriers, barrierdescs);
		}

		active_renderpass[cmd] = nullptr;
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
		auto& descriptors = GetFrameResources().descriptors[cmd];
		auto& table = descriptors.tables[stage];
		if (table.SRV[slot] != resource || table.SRV_index[slot] != subresource)
		{
			table.SRV[slot] = resource;
			table.SRV_index[slot] = subresource;
			descriptors.dirty_graphics_compute[stage == CS] = true;
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
		auto& descriptors = GetFrameResources().descriptors[cmd];
		auto& table = descriptors.tables[stage];
		if (table.UAV[slot] != resource || table.UAV_index[slot] != subresource)
		{
			table.UAV[slot] = resource;
			table.UAV_index[slot] = subresource;
			descriptors.dirty_graphics_compute[stage == CS] = true;
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
		auto& descriptors = GetFrameResources().descriptors[cmd];
		auto& table = descriptors.tables[stage];
		if (table.SAM[slot] != sampler)
		{
			table.SAM[slot] = sampler;
			descriptors.dirty_graphics_compute[stage == CS] = true;
		}
	}
	void GraphicsDevice_DX12::BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, uint32_t slot, CommandList cmd)
	{
		assert(slot < GPU_RESOURCE_HEAP_CBV_COUNT);
		auto& descriptors = GetFrameResources().descriptors[cmd];
		auto& table = descriptors.tables[stage];
		if (buffer->desc.Usage == USAGE_DYNAMIC || table.CBV[slot] != buffer)
		{
			table.CBV[slot] = buffer;
			descriptors.dirty_graphics_compute[stage == CS] = true;
		}
	}
	void GraphicsDevice_DX12::BindVertexBuffers(const GPUBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets, CommandList cmd)
	{
		assert(count <= 8);
		D3D12_VERTEX_BUFFER_VIEW res[8] = { 0 };
		for (uint32_t i = 0; i < count; ++i)
		{
			if (vertexBuffers[i] != nullptr)
			{
				res[i].BufferLocation = vertexBuffers[i]->IsValid() ? to_internal(vertexBuffers[i])->resource->GetGPUVirtualAddress() : 0;
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

			res.BufferLocation = internal_state->resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
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

		GetFrameResources().descriptors[cmd].dirty_graphics_compute[0] = true;
		active_pso[cmd] = pso;

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
				D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

				if (pso->desc.vs != nullptr)
				{
					desc.VS.pShaderBytecode = pso->desc.vs->code.data();
					desc.VS.BytecodeLength = pso->desc.vs->code.size();
				}
				if (pso->desc.hs != nullptr)
				{
					desc.HS.pShaderBytecode = pso->desc.hs->code.data();
					desc.HS.BytecodeLength = pso->desc.hs->code.size();
				}
				if (pso->desc.ds != nullptr)
				{
					desc.DS.pShaderBytecode = pso->desc.ds->code.data();
					desc.DS.BytecodeLength = pso->desc.ds->code.size();
				}
				if (pso->desc.gs != nullptr)
				{
					desc.GS.pShaderBytecode = pso->desc.gs->code.data();
					desc.GS.BytecodeLength = pso->desc.gs->code.size();
				}
				if (pso->desc.ps != nullptr)
				{
					desc.PS.BytecodeLength = pso->desc.ps->code.size();
					desc.PS.pShaderBytecode = pso->desc.ps->code.data();
				}

				RasterizerStateDesc pRasterizerStateDesc = pso->desc.rs != nullptr ? pso->desc.rs->GetDesc() : RasterizerStateDesc();
				desc.RasterizerState.FillMode = _ConvertFillMode(pRasterizerStateDesc.FillMode);
				desc.RasterizerState.CullMode = _ConvertCullMode(pRasterizerStateDesc.CullMode);
				desc.RasterizerState.FrontCounterClockwise = pRasterizerStateDesc.FrontCounterClockwise;
				desc.RasterizerState.DepthBias = pRasterizerStateDesc.DepthBias;
				desc.RasterizerState.DepthBiasClamp = pRasterizerStateDesc.DepthBiasClamp;
				desc.RasterizerState.SlopeScaledDepthBias = pRasterizerStateDesc.SlopeScaledDepthBias;
				desc.RasterizerState.DepthClipEnable = pRasterizerStateDesc.DepthClipEnable;
				desc.RasterizerState.MultisampleEnable = pRasterizerStateDesc.MultisampleEnable;
				desc.RasterizerState.AntialiasedLineEnable = pRasterizerStateDesc.AntialiasedLineEnable;
				desc.RasterizerState.ConservativeRaster = ((CONSERVATIVE_RASTERIZATION && pRasterizerStateDesc.ConservativeRasterizationEnable) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
				desc.RasterizerState.ForcedSampleCount = pRasterizerStateDesc.ForcedSampleCount;


				DepthStencilStateDesc pDepthStencilStateDesc = pso->desc.dss != nullptr ? pso->desc.dss->GetDesc() : DepthStencilStateDesc();
				desc.DepthStencilState.DepthEnable = pDepthStencilStateDesc.DepthEnable;
				desc.DepthStencilState.DepthWriteMask = _ConvertDepthWriteMask(pDepthStencilStateDesc.DepthWriteMask);
				desc.DepthStencilState.DepthFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.DepthFunc);
				desc.DepthStencilState.StencilEnable = pDepthStencilStateDesc.StencilEnable;
				desc.DepthStencilState.StencilReadMask = pDepthStencilStateDesc.StencilReadMask;
				desc.DepthStencilState.StencilWriteMask = pDepthStencilStateDesc.StencilWriteMask;
				desc.DepthStencilState.FrontFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc.FrontFace.StencilDepthFailOp);
				desc.DepthStencilState.FrontFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc.FrontFace.StencilFailOp);
				desc.DepthStencilState.FrontFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.FrontFace.StencilFunc);
				desc.DepthStencilState.FrontFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc.FrontFace.StencilPassOp);
				desc.DepthStencilState.BackFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc.BackFace.StencilDepthFailOp);
				desc.DepthStencilState.BackFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc.BackFace.StencilFailOp);
				desc.DepthStencilState.BackFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.BackFace.StencilFunc);
				desc.DepthStencilState.BackFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc.BackFace.StencilPassOp);


				BlendStateDesc pBlendStateDesc = pso->desc.bs != nullptr ? pso->desc.bs->GetDesc() : BlendStateDesc();
				desc.BlendState.AlphaToCoverageEnable = pBlendStateDesc.AlphaToCoverageEnable;
				desc.BlendState.IndependentBlendEnable = pBlendStateDesc.IndependentBlendEnable;
				for (int i = 0; i < 8; ++i)
				{
					desc.BlendState.RenderTarget[i].BlendEnable = pBlendStateDesc.RenderTarget[i].BlendEnable;
					desc.BlendState.RenderTarget[i].SrcBlend = _ConvertBlend(pBlendStateDesc.RenderTarget[i].SrcBlend);
					desc.BlendState.RenderTarget[i].DestBlend = _ConvertBlend(pBlendStateDesc.RenderTarget[i].DestBlend);
					desc.BlendState.RenderTarget[i].BlendOp = _ConvertBlendOp(pBlendStateDesc.RenderTarget[i].BlendOp);
					desc.BlendState.RenderTarget[i].SrcBlendAlpha = _ConvertBlend(pBlendStateDesc.RenderTarget[i].SrcBlendAlpha);
					desc.BlendState.RenderTarget[i].DestBlendAlpha = _ConvertBlend(pBlendStateDesc.RenderTarget[i].DestBlendAlpha);
					desc.BlendState.RenderTarget[i].BlendOpAlpha = _ConvertBlendOp(pBlendStateDesc.RenderTarget[i].BlendOpAlpha);
					desc.BlendState.RenderTarget[i].RenderTargetWriteMask = _ParseColorWriteMask(pBlendStateDesc.RenderTarget[i].RenderTargetWriteMask);
				}

				std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
				if (pso->desc.il != nullptr)
				{
					desc.InputLayout.NumElements = (uint32_t)pso->desc.il->desc.size();
					elements.resize(desc.InputLayout.NumElements);
					for (uint32_t i = 0; i < desc.InputLayout.NumElements; ++i)
					{
						elements[i].SemanticName = pso->desc.il->desc[i].SemanticName.c_str();
						elements[i].SemanticIndex = pso->desc.il->desc[i].SemanticIndex;
						elements[i].Format = _ConvertFormat(pso->desc.il->desc[i].Format);
						elements[i].InputSlot = pso->desc.il->desc[i].InputSlot;
						elements[i].AlignedByteOffset = pso->desc.il->desc[i].AlignedByteOffset;
						if (elements[i].AlignedByteOffset == InputLayoutDesc::APPEND_ALIGNED_ELEMENT)
							elements[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
						elements[i].InputSlotClass = _ConvertInputClassification(pso->desc.il->desc[i].InputSlotClass);
						elements[i].InstanceDataStepRate = pso->desc.il->desc[i].InstanceDataStepRate;
					}
				}
				desc.InputLayout.pInputElementDescs = elements.data();

				desc.NumRenderTargets = 0;
				desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				if (active_renderpass[cmd] == nullptr)
				{
					desc.NumRenderTargets = 1;
					desc.RTVFormats[0] = _ConvertFormat(BACKBUFFER_FORMAT);
				}
				else
				{
					for (uint32_t i = 0; i < active_renderpass[cmd]->desc.numAttachments; ++i)
					{
						const RenderPassAttachment& attachment = active_renderpass[cmd]->desc.attachments[i];

						switch (attachment.type)
						{
						case RenderPassAttachment::RENDERTARGET:
							switch (attachment.texture->desc.Format)
							{
							case FORMAT_R16_TYPELESS:
								desc.RTVFormats[desc.NumRenderTargets] = DXGI_FORMAT_R16_UNORM;
								break;
							case FORMAT_R32_TYPELESS:
								desc.RTVFormats[desc.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT;
								break;
							case FORMAT_R24G8_TYPELESS:
								desc.RTVFormats[desc.NumRenderTargets] = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
								break;
							case FORMAT_R32G8X24_TYPELESS:
								desc.RTVFormats[desc.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
								break;
							default:
								desc.RTVFormats[desc.NumRenderTargets] = _ConvertFormat(attachment.texture->desc.Format);
								break;
							}
							desc.NumRenderTargets++;
							break;
						case RenderPassAttachment::DEPTH_STENCIL:
							switch (attachment.texture->desc.Format)
							{
							case FORMAT_R16_TYPELESS:
								desc.DSVFormat = DXGI_FORMAT_D16_UNORM;
								break;
							case FORMAT_R32_TYPELESS:
								desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
								break;
							case FORMAT_R24G8_TYPELESS:
								desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
								break;
							case FORMAT_R32G8X24_TYPELESS:
								desc.DSVFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
								break;
							default:
								desc.DSVFormat = _ConvertFormat(attachment.texture->desc.Format);
								break;
							}
							break;
						default:
							assert(0);
							break;
						}

						desc.SampleDesc.Count = attachment.texture->desc.SampleCount;
						desc.SampleDesc.Quality = 0;
					}
				}
				desc.SampleMask = pso->desc.sampleMask;

				switch (pso->desc.pt)
				{
				case POINTLIST:
					desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
					break;
				case LINELIST:
				case LINESTRIP:
					desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
					break;
				case TRIANGLELIST:
				case TRIANGLESTRIP:
					desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
					break;
				case PATCHLIST:
					desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
					break;
				default:
					desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
					break;
				}

				desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

				desc.pRootSignature = internal_state->rootSignature.Get();

				ComPtr<ID3D12PipelineState> newpso;
				HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&newpso));
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

		GetDirectCommandList(cmd)->SetGraphicsRootSignature(internal_state->rootSignature.Get());

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
	void GraphicsDevice_DX12::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		assert(cs->stage == CS);
		if (active_cs[cmd] != cs)
		{
			prev_pipeline_hash[cmd] = 0;
			GetFrameResources().descriptors[cmd].dirty_graphics_compute[1] = true;
			active_cs[cmd] = cs;

			auto internal_state = to_internal(cs);
			GetDirectCommandList(cmd)->SetPipelineState(internal_state->resource.Get());
			GetDirectCommandList(cmd)->SetComputeRootSignature(internal_state->rootSignature.Get());
		}
	}
	void GraphicsDevice_DX12::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd].validate(true, cmd);
		GetDirectCommandList(cmd)->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd].validate(true, cmd);
		GetDirectCommandList(cmd)->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd].validate(true, cmd);
		GetDirectCommandList(cmd)->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd].validate(true, cmd);
		GetDirectCommandList(cmd)->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		auto internal_state = to_internal(args);
		GetFrameResources().descriptors[cmd].validate(true, cmd);
		GetDirectCommandList(cmd)->ExecuteIndirect(drawInstancedIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		auto internal_state = to_internal(args);
		GetFrameResources().descriptors[cmd].validate(true, cmd);
		GetDirectCommandList(cmd)->ExecuteIndirect(drawIndexedInstancedIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd].validate(false, cmd);
		GetDirectCommandList(cmd)->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_DX12::DispatchIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		auto internal_state = to_internal(args);
		GetFrameResources().descriptors[cmd].validate(false, cmd);
		GetDirectCommandList(cmd)->ExecuteIndirect(dispatchIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);
		GetDirectCommandList(cmd)->CopyResource(internal_state_dst->resource.Get(), internal_state_src->resource.Get());
	}
	void GraphicsDevice_DX12::CopyTexture2D_Region(const Texture* pDst, uint32_t dstMip, uint32_t dstX, uint32_t dstY, const Texture* pSrc, uint32_t srcMip, CommandList cmd)
	{
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);

		D3D12_RESOURCE_DESC src_desc = internal_state_src->resource->GetDesc();
		D3D12_RESOURCE_DESC dst_desc = internal_state_dst->resource->GetDesc();

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = internal_state_src->resource.Get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = D3D12CalcSubresource(srcMip, 0, 0, src_desc.MipLevels, src_desc.DepthOrArraySize);

		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = internal_state_dst->resource.Get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = D3D12CalcSubresource(dstMip, 0, 0, dst_desc.MipLevels, dst_desc.DepthOrArraySize);


		GetDirectCommandList(cmd)->CopyTextureRegion(&dst, dstX, dstY, 0, &src, nullptr);
	}
	void GraphicsDevice_DX12::MSAAResolve(const Texture* pDst, const Texture* pSrc, CommandList cmd)
	{
	}
	void GraphicsDevice_DX12::UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize)
	{
		// This will fully update the buffer on the GPU timeline
		//	But on the CPU side we need to keep the in flight data versioned, and we use the temporary buffer for that
		//	This is like a USAGE_DEFAULT buffer updater on DX11
		// TODO: USAGE_DYNAMIC would not use GPU copies, but then it would need to handle assigning descriptor pointer dynamically. 
		//	However, now descriptors are created ahead of time in CreateBuffer

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
			DynamicResourceState& state = dynamic_constantbuffers[cmd][buffer];
			state.allocation = AllocateGPU(dataSize, cmd);
			memcpy(state.allocation.data, data, dataSize);

			for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
			{
				if (state.binding[stage])
				{
					GetFrameResources().descriptors[cmd].dirty_graphics_compute[stage == CS] = true;
				}
			}
		}
		else
		{
			// Contents will be transferred to device memory:
			auto internal_state_src = std::static_pointer_cast<Resource_DX12>(GetFrameResources().resourceBuffer[cmd].buffer.internal_state);
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
			GetDirectCommandList(cmd)->ResourceBarrier(1, &barrier);

			uint8_t* dest = GetFrameResources().resourceBuffer[cmd].allocate(dataSize, 1);
			memcpy(dest, data, dataSize);
			GetDirectCommandList(cmd)->CopyBufferRegion(
				internal_state_dst->resource.Get(), 0,
				internal_state_src->resource.Get(), GetFrameResources().resourceBuffer[cmd].calculateOffset(dest),
				dataSize
			);

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
			GetDirectCommandList(cmd)->ResourceBarrier(1, &barrier);

		}

	}
	void GraphicsDevice_DX12::QueryBegin(const GPUQuery* query, CommandList cmd)
	{
		auto internal_state = to_internal(query);

		switch (query->desc.Type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			GetDirectCommandList(cmd)->BeginQuery(querypool_timestamp.Get(), D3D12_QUERY_TYPE_TIMESTAMP, internal_state->query_index);
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			GetDirectCommandList(cmd)->BeginQuery(querypool_occlusion.Get(), D3D12_QUERY_TYPE_BINARY_OCCLUSION, internal_state->query_index);
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
			GetDirectCommandList(cmd)->BeginQuery(querypool_occlusion.Get(), D3D12_QUERY_TYPE_OCCLUSION, internal_state->query_index);
			break;
		}
	}
	void GraphicsDevice_DX12::QueryEnd(const GPUQuery* query, CommandList cmd)
	{
		auto internal_state = to_internal(query);

		switch (query->desc.Type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			GetDirectCommandList(cmd)->EndQuery(querypool_timestamp.Get(), D3D12_QUERY_TYPE_TIMESTAMP, internal_state->query_index);
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			GetDirectCommandList(cmd)->EndQuery(querypool_occlusion.Get(), D3D12_QUERY_TYPE_BINARY_OCCLUSION, internal_state->query_index);
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
			GetDirectCommandList(cmd)->EndQuery(querypool_occlusion.Get(), D3D12_QUERY_TYPE_OCCLUSION, internal_state->query_index);
			break;
		}

		query_resolves[cmd].emplace_back();
		Query_Resolve& resolver = query_resolves[cmd].back();
		resolver.type = query->desc.Type;
		resolver.index = internal_state->query_index;
	}
	bool GraphicsDevice_DX12::QueryRead(const GPUQuery* query, GPUQueryResult* result)
	{
		auto internal_state = to_internal(query);

		D3D12_RANGE range;
		range.Begin = (size_t)internal_state->query_index * sizeof(size_t);
		range.End = range.Begin + sizeof(uint64_t);
		D3D12_RANGE nullrange = {};
		void* data = nullptr;

		switch (query->desc.Type)
		{
		case GPU_QUERY_TYPE_EVENT:
			assert(0); // not implemented yet
			break;
		case GPU_QUERY_TYPE_TIMESTAMP:
			querypool_timestamp_readback->Map(0, &range, &data);
			result->result_timestamp = *(uint64_t*)((size_t)data + range.Begin);
			querypool_timestamp_readback->Unmap(0, &nullrange);
			break;
		case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
			directQueue->GetTimestampFrequency(&result->result_timestamp_frequency);
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
		{
			BOOL passed = FALSE;
			querypool_occlusion_readback->Map(0, &range, &data);
			passed = *(BOOL*)((size_t)data + range.Begin);
			querypool_occlusion_readback->Unmap(0, &nullrange);
			result->result_passed_sample_count = (uint64_t)passed;
			break;
		}
		case GPU_QUERY_TYPE_OCCLUSION:
			querypool_occlusion_readback->Map(0, &range, &data);
			result->result_passed_sample_count = *(uint64_t*)((size_t)data + range.Begin);
			querypool_occlusion_readback->Unmap(0, &nullrange);
			break;
		}

		return true;
	}
	void GraphicsDevice_DX12::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		D3D12_RESOURCE_BARRIER barrierdescs[8];

		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];
			D3D12_RESOURCE_BARRIER& barrierdesc = barrierdescs[i];

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
				barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
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
		}

		GetDirectCommandList(cmd)->ResourceBarrier(numBarriers, barrierdescs);
	}
	void GraphicsDevice_DX12::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
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

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES)
				{
					geometry.Triangles.VertexBuffer.StartAddress = to_internal(&x.triangles.vertexBuffer)->resource->GetGPUVirtualAddress() + 
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.vertexByteOffset;
					geometry.Triangles.IndexBuffer = to_internal(&x.triangles.indexBuffer)->resource->GetGPUVirtualAddress() +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.indexOffset * (x.triangles.indexFormat == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? sizeof(uint16_t) : sizeof(uint32_t));

					if (x._flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.Triangles.Transform3x4 = to_internal(&x.triangles.transform3x4Buffer)->resource->GetGPUVirtualAddress() +
							(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.transform3x4BufferOffset;
					}
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::PROCEDURAL_AABBS)
				{
					geometry.AABBs.AABBs.StartAddress = to_internal(&x.aabbs.aabbBuffer)->resource->GetGPUVirtualAddress() +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.aabbs.offset;
				}
			}
		}
		break;
		case RaytracingAccelerationStructureDesc::TOPLEVEL:
		{
			desc.Inputs.InstanceDescs = to_internal(&dst->desc.toplevel.instanceBuffer)->resource->GetGPUVirtualAddress() +
				(D3D12_GPU_VIRTUAL_ADDRESS)dst->desc.toplevel.offset;
		}
		break;
		}

		if (src != nullptr)
		{
			desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

			auto src_internal = to_internal(src);
			desc.SourceAccelerationStructureData = src_internal->resource->GetGPUVirtualAddress();
		}
		desc.DestAccelerationStructureData = dst_internal->resource->GetGPUVirtualAddress();
		desc.ScratchAccelerationStructureData = to_internal(&dst_internal->scratch)->resource->GetGPUVirtualAddress();
		GetDirectCommandList(cmd)->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
	}
	void GraphicsDevice_DX12::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		prev_pipeline_hash[cmd] = 0;
		GetFrameResources().descriptors[cmd].dirty_graphics_compute[1] = true;
		active_cs[cmd] = rtpso->desc.shaderlibraries.front().shader; // we just take the first shader (todo: better)

		auto internal_state = to_internal(rtpso);
		GetDirectCommandList(cmd)->SetPipelineState1(internal_state->resource.Get());
		GetDirectCommandList(cmd)->SetComputeRootSignature(to_internal(active_cs[cmd])->rootSignature.Get());
	}
	void GraphicsDevice_DX12::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd].validate(false, cmd);

		D3D12_DISPATCH_RAYS_DESC dispatchrays_desc = {};

		dispatchrays_desc.Width = desc->Width;
		dispatchrays_desc.Height = desc->Height;
		dispatchrays_desc.Depth = desc->Depth;

		if (desc->raygeneration.buffer != nullptr)
		{
			dispatchrays_desc.RayGenerationShaderRecord.StartAddress =
				to_internal(desc->raygeneration.buffer)->resource->GetGPUVirtualAddress() +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->raygeneration.offset;
			dispatchrays_desc.RayGenerationShaderRecord.SizeInBytes =
				desc->raygeneration.size;
		}

		if (desc->miss.buffer != nullptr)
		{
			dispatchrays_desc.MissShaderTable.StartAddress =
				to_internal(desc->miss.buffer)->resource->GetGPUVirtualAddress() +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->miss.offset;
			dispatchrays_desc.MissShaderTable.SizeInBytes =
				desc->miss.size;
			dispatchrays_desc.MissShaderTable.StrideInBytes =
				desc->miss.stride;
		}

		if (desc->hitgroup.buffer != nullptr)
		{
			dispatchrays_desc.HitGroupTable.StartAddress =
				to_internal(desc->hitgroup.buffer)->resource->GetGPUVirtualAddress() +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->hitgroup.offset;
			dispatchrays_desc.HitGroupTable.SizeInBytes =
				desc->hitgroup.size;
			dispatchrays_desc.HitGroupTable.StrideInBytes =
				desc->hitgroup.stride;
		}

		if (desc->callable.buffer != nullptr)
		{
			dispatchrays_desc.CallableShaderTable.StartAddress =
				to_internal(desc->callable.buffer)->resource->GetGPUVirtualAddress() +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->callable.offset;
			dispatchrays_desc.CallableShaderTable.SizeInBytes =
				desc->callable.size;
			dispatchrays_desc.CallableShaderTable.StrideInBytes =
				desc->callable.stride;
		}

		GetDirectCommandList(cmd)->DispatchRays(&dispatchrays_desc);
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
