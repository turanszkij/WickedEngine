#include "wiGraphicsDevice_DX11.h"

#ifdef WICKEDENGINE_BUILD_DX11

#include "wiHelper.h"
#include "ResourceMapping.h"
#include "wiBackLog.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"Dxgi.lib")
#pragma comment(lib,"dxguid.lib")

#include <sstream>
#include <algorithm>

using namespace Microsoft::WRL;

namespace wiGraphics
{

namespace DX11_Internal
{
	// Engine -> Native converters

	constexpr uint32_t _ParseBindFlags(uint32_t value)
	{
		uint32_t _flag = 0;

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
	constexpr uint32_t _ParseCPUAccessFlags(uint32_t value)
	{
		uint32_t _flag = 0;

		if (value & CPU_ACCESS_WRITE)
			_flag |= D3D11_CPU_ACCESS_WRITE;
		if (value & CPU_ACCESS_READ)
			_flag |= D3D11_CPU_ACCESS_READ;

		return _flag;
	}
	constexpr uint32_t _ParseResourceMiscFlags(uint32_t value)
	{
		uint32_t _flag = 0;

		if (value & RESOURCE_MISC_SHARED)
			_flag |= D3D11_RESOURCE_MISC_SHARED;
		if (value & RESOURCE_MISC_TEXTURECUBE)
			_flag |= D3D11_RESOURCE_MISC_TEXTURECUBE;
		if (value & RESOURCE_MISC_INDIRECT_ARGS)
			_flag |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
		if (value & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			_flag |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		if (value & RESOURCE_MISC_BUFFER_STRUCTURED)
			_flag |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		if (value & RESOURCE_MISC_TILED)
			_flag |= D3D11_RESOURCE_MISC_TILED;

		return _flag;
	}
	constexpr uint32_t _ParseColorWriteMask(uint32_t value)
	{
		uint32_t _flag = 0;

		if (value == D3D11_COLOR_WRITE_ENABLE_ALL)
		{
			return D3D11_COLOR_WRITE_ENABLE_ALL;
		}
		else
		{
			if (value & COLOR_WRITE_ENABLE_RED)
				_flag |= D3D11_COLOR_WRITE_ENABLE_RED;
			if (value & COLOR_WRITE_ENABLE_GREEN)
				_flag |= D3D11_COLOR_WRITE_ENABLE_GREEN;
			if (value & COLOR_WRITE_ENABLE_BLUE)
				_flag |= D3D11_COLOR_WRITE_ENABLE_BLUE;
			if (value & COLOR_WRITE_ENABLE_ALPHA)
				_flag |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
		}

		return _flag;
	}

	constexpr D3D11_FILTER _ConvertFilter(FILTER value)
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
	constexpr D3D11_TEXTURE_ADDRESS_MODE _ConvertTextureAddressMode(TEXTURE_ADDRESS_MODE value)
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
		default:
			break;
		}
		return D3D11_TEXTURE_ADDRESS_WRAP;
	}
	constexpr D3D11_COMPARISON_FUNC _ConvertComparisonFunc(COMPARISON_FUNC value)
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
	constexpr D3D11_FILL_MODE _ConvertFillMode(FILL_MODE value)
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
	constexpr D3D11_CULL_MODE _ConvertCullMode(CULL_MODE value)
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
	constexpr D3D11_DEPTH_WRITE_MASK _ConvertDepthWriteMask(DEPTH_WRITE_MASK value)
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
	constexpr D3D11_STENCIL_OP _ConvertStencilOp(STENCIL_OP value)
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
	constexpr D3D11_BLEND _ConvertBlend(BLEND value)
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
	constexpr D3D11_BLEND_OP _ConvertBlendOp(BLEND_OP value)
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
	constexpr D3D11_USAGE _ConvertUsage(USAGE value)
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
	constexpr D3D11_INPUT_CLASSIFICATION _ConvertInputClassification(INPUT_CLASSIFICATION value)
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
		case FORMAT_R24G8_TYPELESS:
			return DXGI_FORMAT_R24G8_TYPELESS;
			break;
		case FORMAT_D24_UNORM_S8_UINT:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
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

	inline D3D11_TEXTURE1D_DESC _ConvertTextureDesc1D(const TextureDesc* pDesc)
	{
		D3D11_TEXTURE1D_DESC desc;
		desc.Width = pDesc->Width;
		desc.MipLevels = pDesc->MipLevels;
		desc.ArraySize = pDesc->ArraySize;
		desc.Format = _ConvertFormat(pDesc->Format);
		desc.Usage = _ConvertUsage(pDesc->Usage);
		desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
		desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
		desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);

		return desc;
	}
	inline D3D11_TEXTURE2D_DESC _ConvertTextureDesc2D(const TextureDesc* pDesc)
	{
		D3D11_TEXTURE2D_DESC desc;
		desc.Width = pDesc->Width;
		desc.Height = pDesc->Height;
		desc.MipLevels = pDesc->MipLevels;
		desc.ArraySize = pDesc->ArraySize;
		desc.Format = _ConvertFormat(pDesc->Format);
		desc.SampleDesc.Count = pDesc->SampleCount;
		desc.SampleDesc.Quality = 0;
		desc.Usage = _ConvertUsage(pDesc->Usage);
		desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
		desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
		desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);

		return desc;
	}
	inline D3D11_TEXTURE3D_DESC _ConvertTextureDesc3D(const TextureDesc* pDesc)
	{
		D3D11_TEXTURE3D_DESC desc;
		desc.Width = pDesc->Width;
		desc.Height = pDesc->Height;
		desc.Depth = pDesc->Depth;
		desc.MipLevels = pDesc->MipLevels;
		desc.Format = _ConvertFormat(pDesc->Format);
		desc.Usage = _ConvertUsage(pDesc->Usage);
		desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
		desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
		desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);

		return desc;
	}
	inline D3D11_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
	{
		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = pInitialData.pSysMem;
		data.SysMemPitch = pInitialData.SysMemPitch;
		data.SysMemSlicePitch = pInitialData.SysMemSlicePitch;

		return data;
	}


	// Native -> Engine converters

	constexpr uint32_t _ParseBindFlags_Inv(uint32_t value)
	{
		uint32_t _flag = 0;

		if (value & D3D11_BIND_VERTEX_BUFFER)
			_flag |= BIND_VERTEX_BUFFER;
		if (value & D3D11_BIND_INDEX_BUFFER)
			_flag |= BIND_INDEX_BUFFER;
		if (value & D3D11_BIND_CONSTANT_BUFFER)
			_flag |= BIND_CONSTANT_BUFFER;
		if (value & D3D11_BIND_SHADER_RESOURCE)
			_flag |= BIND_SHADER_RESOURCE;
		if (value & D3D11_BIND_STREAM_OUTPUT)
			_flag |= BIND_STREAM_OUTPUT;
		if (value & D3D11_BIND_RENDER_TARGET)
			_flag |= BIND_RENDER_TARGET;
		if (value & D3D11_BIND_DEPTH_STENCIL)
			_flag |= BIND_DEPTH_STENCIL;
		if (value & D3D11_BIND_UNORDERED_ACCESS)
			_flag |= BIND_UNORDERED_ACCESS;

		return _flag;
	}
	constexpr uint32_t _ParseCPUAccessFlags_Inv(uint32_t value)
	{
		uint32_t _flag = 0;

		if (value & D3D11_CPU_ACCESS_WRITE)
			_flag |= CPU_ACCESS_WRITE;
		if (value & D3D11_CPU_ACCESS_READ)
			_flag |= CPU_ACCESS_READ;

		return _flag;
	}
	constexpr uint32_t _ParseResourceMiscFlags_Inv(uint32_t value)
	{
		uint32_t _flag = 0;

		if (value & D3D11_RESOURCE_MISC_SHARED)
			_flag |= RESOURCE_MISC_SHARED;
		if (value & D3D11_RESOURCE_MISC_TEXTURECUBE)
			_flag |= RESOURCE_MISC_TEXTURECUBE;
		if (value & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
			_flag |= RESOURCE_MISC_INDIRECT_ARGS;
		if (value & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			_flag |= RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		if (value & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
			_flag |= RESOURCE_MISC_BUFFER_STRUCTURED;
		if (value & D3D11_RESOURCE_MISC_TILED)
			_flag |= RESOURCE_MISC_TILED;

		return _flag;
	}

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
		case DXGI_FORMAT_R24G8_TYPELESS:
			return FORMAT_R24G8_TYPELESS;
			break;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return FORMAT_D24_UNORM_S8_UINT;
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
	constexpr USAGE _ConvertUsage_Inv(D3D11_USAGE value)
	{
		switch (value)
		{
		case D3D11_USAGE_DEFAULT:
			return USAGE_DEFAULT;
			break;
		case D3D11_USAGE_IMMUTABLE:
			return USAGE_IMMUTABLE;
			break;
		case D3D11_USAGE_DYNAMIC:
			return USAGE_DYNAMIC;
			break;
		case D3D11_USAGE_STAGING:
			return USAGE_STAGING;
			break;
		default:
			break;
		}
		return USAGE_DEFAULT;
	}

	inline TextureDesc _ConvertTextureDesc_Inv(const D3D11_TEXTURE1D_DESC* pDesc)
	{
		TextureDesc desc;
		desc.Width = pDesc->Width;
		desc.MipLevels = pDesc->MipLevels;
		desc.ArraySize = pDesc->ArraySize;
		desc.Format = _ConvertFormat_Inv(pDesc->Format);
		desc.Usage = _ConvertUsage_Inv(pDesc->Usage);
		desc.BindFlags = _ParseBindFlags_Inv(pDesc->BindFlags);
		desc.CPUAccessFlags = _ParseCPUAccessFlags_Inv(pDesc->CPUAccessFlags);
		desc.MiscFlags = _ParseResourceMiscFlags_Inv(pDesc->MiscFlags);

		return desc;
	}
	inline TextureDesc _ConvertTextureDesc_Inv(const D3D11_TEXTURE2D_DESC* pDesc)
	{
		TextureDesc desc;
		desc.Width = pDesc->Width;
		desc.Height = pDesc->Height;
		desc.MipLevels = pDesc->MipLevels;
		desc.ArraySize = pDesc->ArraySize;
		desc.Format = _ConvertFormat_Inv(pDesc->Format);
		desc.SampleCount = pDesc->SampleDesc.Count;
		desc.Usage = _ConvertUsage_Inv(pDesc->Usage);
		desc.BindFlags = _ParseBindFlags_Inv(pDesc->BindFlags);
		desc.CPUAccessFlags = _ParseCPUAccessFlags_Inv(pDesc->CPUAccessFlags);
		desc.MiscFlags = _ParseResourceMiscFlags_Inv(pDesc->MiscFlags);

		return desc;
	}
	inline TextureDesc _ConvertTextureDesc_Inv(const D3D11_TEXTURE3D_DESC* pDesc)
	{
		TextureDesc desc;
		desc.Width = pDesc->Width;
		desc.Height = pDesc->Height;
		desc.Depth = pDesc->Depth;
		desc.MipLevels = pDesc->MipLevels;
		desc.Format = _ConvertFormat_Inv(pDesc->Format);
		desc.Usage = _ConvertUsage_Inv(pDesc->Usage);
		desc.BindFlags = _ParseBindFlags_Inv(pDesc->BindFlags);
		desc.CPUAccessFlags = _ParseCPUAccessFlags_Inv(pDesc->CPUAccessFlags);
		desc.MiscFlags = _ParseResourceMiscFlags_Inv(pDesc->MiscFlags);

		return desc;
	}


	// Local Helpers:
	const void* const __nullBlob[128] = {}; // this is initialized to nullptrs and used to unbind resources!


	struct Resource_DX11
	{
		ComPtr<ID3D11Resource> resource;
		ComPtr<ID3D11ShaderResourceView> srv;
		ComPtr<ID3D11UnorderedAccessView> uav;
		std::vector<ComPtr<ID3D11ShaderResourceView>> subresources_srv;
		std::vector<ComPtr<ID3D11UnorderedAccessView>> subresources_uav;
	};
	struct Texture_DX11 : public Resource_DX11
	{
		ComPtr<ID3D11RenderTargetView> rtv;
		ComPtr<ID3D11DepthStencilView> dsv;
		std::vector<ComPtr<ID3D11RenderTargetView>> subresources_rtv;
		std::vector<ComPtr<ID3D11DepthStencilView>> subresources_dsv;
	};
	struct InputLayout_DX11
	{
		ComPtr<ID3D11InputLayout> resource;
	};
	struct VertexShader_DX11
	{
		ComPtr<ID3D11VertexShader> resource;
	};
	struct HullShader_DX11
	{
		ComPtr<ID3D11HullShader> resource;
	};
	struct DomainShader_DX11
	{
		ComPtr<ID3D11DomainShader> resource;
	};
	struct GeometryShader_DX11
	{
		ComPtr<ID3D11GeometryShader> resource;
	};
	struct PixelShader_DX11
	{
		ComPtr<ID3D11PixelShader> resource;
	};
	struct ComputeShader_DX11
	{
		ComPtr<ID3D11ComputeShader> resource;
	};
	struct BlendState_DX11
	{
		ComPtr<ID3D11BlendState> resource;
	};
	struct DepthStencilState_DX11
	{
		ComPtr<ID3D11DepthStencilState> resource;
	};
	struct RasterizerState_DX11
	{
		ComPtr<ID3D11RasterizerState> resource;
	};
	struct Sampler_DX11
	{
		ComPtr<ID3D11SamplerState> resource;
	};
	struct Query_DX11
	{
		ComPtr<ID3D11Query> resource;
	};

	Resource_DX11* to_internal(const GPUResource* param)
	{
		return static_cast<Resource_DX11*>(param->internal_state.get());
	}
	Resource_DX11* to_internal(const GPUBuffer* param)
	{
		return static_cast<Resource_DX11*>(param->internal_state.get());
	}
	Texture_DX11* to_internal(const Texture* param)
	{
		return static_cast<Texture_DX11*>(param->internal_state.get());
	}
	InputLayout_DX11* to_internal(const InputLayout* param)
	{
		return static_cast<InputLayout_DX11*>(param->internal_state.get());
	}
	BlendState_DX11* to_internal(const BlendState* param)
	{
		return static_cast<BlendState_DX11*>(param->internal_state.get());
	}
	DepthStencilState_DX11* to_internal(const DepthStencilState* param)
	{
		return static_cast<DepthStencilState_DX11*>(param->internal_state.get());
	}
	RasterizerState_DX11* to_internal(const RasterizerState* param)
	{
		return static_cast<RasterizerState_DX11*>(param->internal_state.get());
	}
	Sampler_DX11* to_internal(const Sampler* param)
	{
		return static_cast<Sampler_DX11*>(param->internal_state.get());
	}
	Query_DX11* to_internal(const GPUQuery* param)
	{
		return static_cast<Query_DX11*>(param->internal_state.get());
	}
}
using namespace DX11_Internal;

void GraphicsDevice_DX11::pso_validate(CommandList cmd)
{
	if (!dirty_pso[cmd])
		return;

	const PipelineState* pso = active_pso[cmd];
	const PipelineStateDesc& desc = pso != nullptr ? pso->GetDesc() : PipelineStateDesc();

	ID3D11VertexShader* vs = desc.vs == nullptr ? nullptr : static_cast<VertexShader_DX11*>(desc.vs->internal_state.get())->resource.Get();
	if (vs != prev_vs[cmd])
	{
		deviceContexts[cmd]->VSSetShader(vs, nullptr, 0);
		prev_vs[cmd] = vs;
	}
	ID3D11PixelShader* ps = desc.ps == nullptr ? nullptr : static_cast<PixelShader_DX11*>(desc.ps->internal_state.get())->resource.Get();
	if (ps != prev_ps[cmd])
	{
		deviceContexts[cmd]->PSSetShader(ps, nullptr, 0);
		prev_ps[cmd] = ps;
	}
	ID3D11HullShader* hs = desc.hs == nullptr ? nullptr : static_cast<HullShader_DX11*>(desc.hs->internal_state.get())->resource.Get();
	if (hs != prev_hs[cmd])
	{
		deviceContexts[cmd]->HSSetShader(hs, nullptr, 0);
		prev_hs[cmd] = hs;
	}
	ID3D11DomainShader* ds = desc.ds == nullptr ? nullptr : static_cast<DomainShader_DX11*>(desc.ds->internal_state.get())->resource.Get();
	if (ds != prev_ds[cmd])
	{
		deviceContexts[cmd]->DSSetShader(ds, nullptr, 0);
		prev_ds[cmd] = ds;
	}
	ID3D11GeometryShader* gs = desc.gs == nullptr ? nullptr : static_cast<GeometryShader_DX11*>(desc.gs->internal_state.get())->resource.Get();
	if (gs != prev_gs[cmd])
	{
		deviceContexts[cmd]->GSSetShader(gs, nullptr, 0);
		prev_gs[cmd] = gs;
	}

	ID3D11BlendState* bs = desc.bs == nullptr ? nullptr : to_internal(desc.bs)->resource.Get();
	if (bs != prev_bs[cmd] || desc.sampleMask != prev_samplemask[cmd] ||
		blendFactor[cmd].x != prev_blendfactor[cmd].x ||
		blendFactor[cmd].y != prev_blendfactor[cmd].y ||
		blendFactor[cmd].z != prev_blendfactor[cmd].z ||
		blendFactor[cmd].w != prev_blendfactor[cmd].w
		)
	{
		const float fact[4] = { blendFactor[cmd].x, blendFactor[cmd].y, blendFactor[cmd].z, blendFactor[cmd].w };
		deviceContexts[cmd]->OMSetBlendState(bs, fact, desc.sampleMask);
		prev_bs[cmd] = bs;
		prev_blendfactor[cmd] = blendFactor[cmd];
		prev_samplemask[cmd] = desc.sampleMask;
	}

	ID3D11RasterizerState* rs = desc.rs == nullptr ? nullptr : to_internal(desc.rs)->resource.Get();
	if (rs != prev_rs[cmd])
	{
		deviceContexts[cmd]->RSSetState(rs);
		prev_rs[cmd] = rs;
	}

	ID3D11DepthStencilState* dss = desc.dss == nullptr ? nullptr : to_internal(desc.dss)->resource.Get();
	if (dss != prev_dss[cmd] || stencilRef[cmd] != prev_stencilRef[cmd])
	{
		deviceContexts[cmd]->OMSetDepthStencilState(dss, stencilRef[cmd]);
		prev_dss[cmd] = dss;
		prev_stencilRef[cmd] = stencilRef[cmd];
	}

	ID3D11InputLayout* il = desc.il == nullptr ? nullptr : to_internal(desc.il)->resource.Get();
	if (il != prev_il[cmd])
	{
		deviceContexts[cmd]->IASetInputLayout(il);
		prev_il[cmd] = il;
	}

	if (prev_pt[cmd] != desc.pt)
	{
		D3D11_PRIMITIVE_TOPOLOGY d3dType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		switch (desc.pt)
		{
		case TRIANGLELIST:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case TRIANGLESTRIP:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
		case POINTLIST:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case LINELIST:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case LINESTRIP:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case PATCHLIST:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
			break;
		default:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
			break;
		};
		deviceContexts[cmd]->IASetPrimitiveTopology(d3dType);

		prev_pt[cmd] = desc.pt;
	}
}

// Engine functions
GraphicsDevice_DX11::GraphicsDevice_DX11(wiPlatform::window_type window, bool fullscreen, bool debuglayer)
{
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

	uint32_t createDeviceFlags = 0;

	if (debuglayer)
	{
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	uint32_t numDriverTypes = arraysize(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	uint32_t numFeatureLevels = arraysize(featureLevels);

	for (uint32_t driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &device
			, &featureLevel, &immediateContext);

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
	{
		std::stringstream ss("");
		ss << "Failed to create the graphics device! ERROR: " << std::hex << hr;
		wiHelper::messageBox(ss.str(), "Error!");
		wiPlatform::Exit();
	}

	ComPtr<IDXGIDevice2> pDXGIDevice;
	hr = device.As(&pDXGIDevice);

	ComPtr<IDXGIAdapter> pDXGIAdapter;
	hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pDXGIAdapter);

	ComPtr<IDXGIFactory2> pIDXGIFactory;
	pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&pIDXGIFactory);


	DXGI_SWAP_CHAIN_DESC1 sd = { 0 };
	sd.Width = RESOLUTIONWIDTH;
	sd.Height = RESOLUTIONHEIGHT;
	sd.Format = _ConvertFormat(GetBackBufferFormat());
	sd.Stereo = false;
	sd.SampleDesc.Count = 1; // Don't use multi-sampling.
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2; // Use double-buffering to minimize latency.
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
	hr = pIDXGIFactory->CreateSwapChainForHwnd(device.Get(), window, &sd, &fullscreenDesc, nullptr, &swapChain);
#else
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
	sd.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

	hr = pIDXGIFactory->CreateSwapChainForCoreWindow(device.Get(), reinterpret_cast<IUnknown*>(window.Get()), &sd, nullptr, &swapChain);
#endif

	if (FAILED(hr))
	{
		wiHelper::messageBox("Failed to create a swapchain for the graphics device!", "Error!");
		wiPlatform::Exit();
	}

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
	// ensures that the application will only render after each VSync, minimizing power consumption.
	hr = pDXGIDevice->SetMaximumFrameLatency(1);


	D3D_FEATURE_LEVEL aquiredFeatureLevel = device->GetFeatureLevel();
	TESSELLATION = ((aquiredFeatureLevel >= D3D_FEATURE_LEVEL_11_0) ? true : false);

	//D3D11_FEATURE_DATA_D3D11_OPTIONS features_0;
	//hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &features_0, sizeof(features_0));

	//D3D11_FEATURE_DATA_D3D11_OPTIONS1 features_1;
	//hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &features_1, sizeof(features_1));

	D3D11_FEATURE_DATA_D3D11_OPTIONS2 features_2;
	hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &features_2, sizeof(features_2));
	CONSERVATIVE_RASTERIZATION = features_2.ConservativeRasterizationTier >= D3D11_CONSERVATIVE_RASTERIZATION_TIER_1;
	RASTERIZER_ORDERED_VIEWS = features_2.ROVsSupported == TRUE;

	if (features_2.TypedUAVLoadAdditionalFormats)
	{
		// More info about UAV format load support: https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
		UAV_LOAD_FORMAT_COMMON = true;

		D3D11_FEATURE_DATA_FORMAT_SUPPORT2 FormatSupport = {};
		FormatSupport.InFormat = DXGI_FORMAT_R11G11B10_FLOAT;
		hr = device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &FormatSupport, sizeof(FormatSupport));
		if (SUCCEEDED(hr) && (FormatSupport.OutFormatSupport2 & D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
		{
			UAV_LOAD_FORMAT_R11G11B10_FLOAT = true;
		}
	}

	D3D11_FEATURE_DATA_D3D11_OPTIONS3 features_3;
	hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &features_3, sizeof(features_3));
	RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS = features_3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer == TRUE;

	CreateBackBufferResources();

	emptyresource = std::make_shared<EmptyResourceHandle>();

	wiBackLog::post("Created GraphicsDevice_DX11");
}

void GraphicsDevice_DX11::CreateBackBufferResources()
{
	HRESULT hr;

	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
	if (FAILED(hr)) {
		wiHelper::messageBox("BackBuffer creation Failed!", "Error!");
		wiPlatform::Exit();
	}

	hr = device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView);
	if (FAILED(hr)) {
		wiHelper::messageBox("Main Rendertarget creation Failed!", "Error!");
		wiPlatform::Exit();
	}
}

void GraphicsDevice_DX11::SetResolution(int width, int height)
{
	if ((width != RESOLUTIONWIDTH || height != RESOLUTIONHEIGHT) && width > 0 && height > 0)
	{
		RESOLUTIONWIDTH = width;
		RESOLUTIONHEIGHT = height;

		backBuffer.Reset();
		renderTargetView.Reset();

		HRESULT hr = swapChain->ResizeBuffers(GetBackBufferCount(), width, height, _ConvertFormat(GetBackBufferFormat()), 0);
		assert(SUCCEEDED(hr));

		CreateBackBufferResources();
	}
}

Texture GraphicsDevice_DX11::GetBackBuffer()
{
	auto internal_state = std::make_shared<Texture_DX11>();
	internal_state->resource = backBuffer;

	Texture result;
	result.internal_state = internal_state;
	result.type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;

	D3D11_TEXTURE2D_DESC desc;
	backBuffer->GetDesc(&desc);
	result.desc = _ConvertTextureDesc_Inv(&desc);

	return result;
}

bool GraphicsDevice_DX11::CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer)
{
	auto internal_state = std::make_shared<Resource_DX11>();
	pBuffer->internal_state = internal_state;
	pBuffer->type = GPUResource::GPU_RESOURCE_TYPE::BUFFER;

	D3D11_BUFFER_DESC desc; 
	desc.ByteWidth = pDesc->ByteWidth;
	desc.Usage = _ConvertUsage(pDesc->Usage);
	desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);
	desc.StructureByteStride = pDesc->StructureByteStride;

	D3D11_SUBRESOURCE_DATA data;
	if (pInitialData != nullptr)
	{
		data = _ConvertSubresourceData(*pInitialData);
	}

	pBuffer->desc = *pDesc;
	HRESULT hr = device->CreateBuffer(&desc, pInitialData == nullptr ? nullptr : &data, (ID3D11Buffer**)internal_state->resource.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(hr) && "GPUBuffer creation failed!");

	if (SUCCEEDED(hr))
	{
		// Create resource views if needed
		if (pDesc->BindFlags & BIND_SHADER_RESOURCE)
		{
			CreateSubresource(pBuffer, SRV, 0);
		}
		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			CreateSubresource(pBuffer, UAV, 0);
		}
	}

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture)
{
	auto internal_state = std::make_shared<Texture_DX11>();
	pTexture->internal_state = internal_state;
	pTexture->type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;

	pTexture->desc = *pDesc;

	std::vector<D3D11_SUBRESOURCE_DATA> data;
	if (pInitialData != nullptr)
	{
		uint32_t dataCount = pDesc->ArraySize * std::max(1u, pDesc->MipLevels);
		data.resize(dataCount);
		for (uint32_t slice = 0; slice < dataCount; ++slice)
		{
			data[slice] = _ConvertSubresourceData(pInitialData[slice]);
		}
	}

	HRESULT hr = S_OK;

	switch (pTexture->desc.type)
	{
	case TextureDesc::TEXTURE_1D:
	{
		D3D11_TEXTURE1D_DESC desc = _ConvertTextureDesc1D(&pTexture->desc);
		hr = device->CreateTexture1D(&desc, data.data(), (ID3D11Texture1D**)internal_state->resource.ReleaseAndGetAddressOf());
	}
	break;
	case TextureDesc::TEXTURE_2D:
	{
		D3D11_TEXTURE2D_DESC desc = _ConvertTextureDesc2D(&pTexture->desc);
		hr = device->CreateTexture2D(&desc, data.data(), (ID3D11Texture2D**)internal_state->resource.ReleaseAndGetAddressOf());
	}
	break;
	case TextureDesc::TEXTURE_3D:
	{
		D3D11_TEXTURE3D_DESC desc = _ConvertTextureDesc3D(&pTexture->desc);
		hr = device->CreateTexture3D(&desc, data.data(), (ID3D11Texture3D**)internal_state->resource.ReleaseAndGetAddressOf());
	}
	break;
	default:
		assert(0);
		break;
	}

	assert(SUCCEEDED(hr));
	if (FAILED(hr))
		return SUCCEEDED(hr);

	if (pTexture->desc.MipLevels == 0)
	{
		pTexture->desc.MipLevels = (uint32_t)log2(std::max(pTexture->desc.Width, pTexture->desc.Height)) + 1;
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
bool GraphicsDevice_DX11::CreateInputLayout(const InputLayoutDesc *pInputElementDescs, uint32_t NumElements, const Shader* shader, InputLayout *pInputLayout)
{
	auto internal_state = std::make_shared<InputLayout_DX11>();
	pInputLayout->internal_state = internal_state;

	pInputLayout->desc.reserve((size_t)NumElements);

	std::vector<D3D11_INPUT_ELEMENT_DESC> desc(NumElements);
	for (uint32_t i = 0; i < NumElements; ++i)
	{
		desc[i].SemanticName = pInputElementDescs[i].SemanticName.c_str();
		desc[i].SemanticIndex = pInputElementDescs[i].SemanticIndex;
		desc[i].Format = _ConvertFormat(pInputElementDescs[i].Format);
		desc[i].InputSlot = pInputElementDescs[i].InputSlot;
		desc[i].AlignedByteOffset = pInputElementDescs[i].AlignedByteOffset;
		if (desc[i].AlignedByteOffset == InputLayoutDesc::APPEND_ALIGNED_ELEMENT)
			desc[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		desc[i].InputSlotClass = _ConvertInputClassification(pInputElementDescs[i].InputSlotClass);
		desc[i].InstanceDataStepRate = pInputElementDescs[i].InstanceDataStepRate;

		pInputLayout->desc.push_back(pInputElementDescs[i]);
	}

	HRESULT hr = device->CreateInputLayout(desc.data(), NumElements, shader->code.data(), shader->code.size(), &internal_state->resource);

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreateShader(SHADERSTAGE stage, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader)
{
	pShader->code.resize(BytecodeLength);
	std::memcpy(pShader->code.data(), pShaderBytecode, BytecodeLength);
	pShader->stage = stage;

	HRESULT hr = E_FAIL;

	switch (stage)
	{
	case wiGraphics::VS:
	{
		auto internal_state = std::make_shared<VertexShader_DX11>();
		pShader->internal_state = internal_state;
		hr = device->CreateVertexShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
	}
	break;
	case wiGraphics::HS:
	{
		auto internal_state = std::make_shared<HullShader_DX11>();
		pShader->internal_state = internal_state;
		hr = device->CreateHullShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
	}
	break;
	case wiGraphics::DS:
	{
		auto internal_state = std::make_shared<DomainShader_DX11>();
		pShader->internal_state = internal_state;
		hr = device->CreateDomainShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
	}
	break;
	case wiGraphics::GS:
	{
		auto internal_state = std::make_shared<GeometryShader_DX11>();
		pShader->internal_state = internal_state;
		hr = device->CreateGeometryShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
	}
	break;
	case wiGraphics::PS:
	{
		auto internal_state = std::make_shared<PixelShader_DX11>();
		pShader->internal_state = internal_state;
		hr = device->CreatePixelShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
	}
	break;
	case wiGraphics::CS:
	{
		auto internal_state = std::make_shared<ComputeShader_DX11>();
		pShader->internal_state = internal_state;
		hr = device->CreateComputeShader(pShaderBytecode, BytecodeLength, nullptr, &internal_state->resource);
	}
	break;
	}

	assert(SUCCEEDED(hr));

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState)
{
	auto internal_state = std::make_shared<BlendState_DX11>();
	pBlendState->internal_state = internal_state;

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
		desc.RenderTarget[i].RenderTargetWriteMask = _ParseColorWriteMask(pBlendStateDesc->RenderTarget[i].RenderTargetWriteMask);
	}

	pBlendState->desc = *pBlendStateDesc;
	HRESULT hr = device->CreateBlendState(&desc, &internal_state->resource);
	assert(SUCCEEDED(hr));

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState)
{
	auto internal_state = std::make_shared<DepthStencilState_DX11>();
	pDepthStencilState->internal_state = internal_state;

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
	HRESULT hr = device->CreateDepthStencilState(&desc, &internal_state->resource);
	assert(SUCCEEDED(hr));

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState)
{
	auto internal_state = std::make_shared<RasterizerState_DX11>();
	pRasterizerState->internal_state = internal_state;

	pRasterizerState->desc = *pRasterizerStateDesc;

	D3D11_RASTERIZER_DESC desc;
	desc.FillMode = _ConvertFillMode(pRasterizerStateDesc->FillMode);
	desc.CullMode = _ConvertCullMode(pRasterizerStateDesc->CullMode);
	desc.FrontCounterClockwise = pRasterizerStateDesc->FrontCounterClockwise;
	desc.DepthBias = pRasterizerStateDesc->DepthBias;
	desc.DepthBiasClamp = pRasterizerStateDesc->DepthBiasClamp;
	desc.SlopeScaledDepthBias = pRasterizerStateDesc->SlopeScaledDepthBias;
	desc.DepthClipEnable = pRasterizerStateDesc->DepthClipEnable;
	desc.ScissorEnable = true;
	desc.MultisampleEnable = pRasterizerStateDesc->MultisampleEnable;
	desc.AntialiasedLineEnable = pRasterizerStateDesc->AntialiasedLineEnable;


	if (CONSERVATIVE_RASTERIZATION && pRasterizerStateDesc->ConservativeRasterizationEnable == TRUE)
	{
		ComPtr<ID3D11Device3> device3;
		if (SUCCEEDED(device.As(&device3)))
		{
			D3D11_RASTERIZER_DESC2 desc2;
			desc2.FillMode = desc.FillMode;
			desc2.CullMode = desc.CullMode;
			desc2.FrontCounterClockwise = desc.FrontCounterClockwise;
			desc2.DepthBias = desc.DepthBias;
			desc2.DepthBiasClamp = desc.DepthBiasClamp;
			desc2.SlopeScaledDepthBias = desc.SlopeScaledDepthBias;
			desc2.DepthClipEnable = desc.DepthClipEnable;
			desc2.ScissorEnable = desc.ScissorEnable;
			desc2.MultisampleEnable = desc.MultisampleEnable;
			desc2.AntialiasedLineEnable = desc.AntialiasedLineEnable;
			desc2.ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON;
			desc2.ForcedSampleCount = (RASTERIZER_ORDERED_VIEWS ? pRasterizerStateDesc->ForcedSampleCount : 0);

			pRasterizerState->desc = *pRasterizerStateDesc;

			ComPtr<ID3D11RasterizerState2> rasterizer2;
			HRESULT hr = device3->CreateRasterizerState2(&desc2, &rasterizer2);
			assert(SUCCEEDED(hr));

			internal_state->resource = rasterizer2;

			return SUCCEEDED(hr);
		}
	}
	else if (RASTERIZER_ORDERED_VIEWS && pRasterizerStateDesc->ForcedSampleCount > 0)
	{
		ComPtr<ID3D11Device1> device1;
		if (SUCCEEDED(device.As(&device1)))
		{
			D3D11_RASTERIZER_DESC1 desc1;
			desc1.FillMode = desc.FillMode;
			desc1.CullMode = desc.CullMode;
			desc1.FrontCounterClockwise = desc.FrontCounterClockwise;
			desc1.DepthBias = desc.DepthBias;
			desc1.DepthBiasClamp = desc.DepthBiasClamp;
			desc1.SlopeScaledDepthBias = desc.SlopeScaledDepthBias;
			desc1.DepthClipEnable = desc.DepthClipEnable;
			desc1.ScissorEnable = desc.ScissorEnable;
			desc1.MultisampleEnable = desc.MultisampleEnable;
			desc1.AntialiasedLineEnable = desc.AntialiasedLineEnable;
			desc1.ForcedSampleCount = pRasterizerStateDesc->ForcedSampleCount;

			pRasterizerState->desc = *pRasterizerStateDesc;

			ComPtr<ID3D11RasterizerState1> rasterizer1;
			HRESULT hr = device1->CreateRasterizerState1(&desc1, &rasterizer1);
			assert(SUCCEEDED(hr));

			internal_state->resource = rasterizer1;

			return SUCCEEDED(hr);
		}
	}

	HRESULT hr = device->CreateRasterizerState(&desc, &internal_state->resource);
	assert(SUCCEEDED(hr));

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreateSampler(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState)
{
	auto internal_state = std::make_shared<Sampler_DX11>();
	pSamplerState->internal_state = internal_state;

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
	HRESULT hr = device->CreateSamplerState(&desc, &internal_state->resource);
	assert(SUCCEEDED(hr));

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery)
{
	auto internal_state = std::make_shared<Query_DX11>();
	pQuery->internal_state = internal_state;

	pQuery->desc = *pDesc;

	D3D11_QUERY_DESC desc;
	desc.MiscFlags = 0;
	desc.Query = D3D11_QUERY_EVENT;
	if (pDesc->Type == GPU_QUERY_TYPE_EVENT)
	{
		desc.Query = D3D11_QUERY_EVENT;
	}
	else if (pDesc->Type == GPU_QUERY_TYPE_OCCLUSION)
	{
		desc.Query = D3D11_QUERY_OCCLUSION;
	}
	else if (pDesc->Type == GPU_QUERY_TYPE_OCCLUSION_PREDICATE)
	{
		desc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
	}
	else if (pDesc->Type == GPU_QUERY_TYPE_TIMESTAMP)
	{
		desc.Query = D3D11_QUERY_TIMESTAMP;
	}
	else if (pDesc->Type == GPU_QUERY_TYPE_TIMESTAMP_DISJOINT)
	{
		desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	}

	HRESULT hr = device->CreateQuery(&desc, &internal_state->resource);
	assert(SUCCEEDED(hr));

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso)
{
	pso->internal_state = emptyresource;

	pso->desc = *pDesc;

	return true;
}
bool GraphicsDevice_DX11::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass)
{
	renderpass->internal_state = emptyresource;

	renderpass->desc = *pDesc;

	return true;
}

int GraphicsDevice_DX11::CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	auto internal_state = to_internal(texture);

	switch (type)
	{
	case wiGraphics::SRV:
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

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
				srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
				srv_desc.Texture1DArray.FirstArraySlice = firstSlice;
				srv_desc.Texture1DArray.ArraySize = sliceCount;
				srv_desc.Texture1DArray.MostDetailedMip = firstMip;
				srv_desc.Texture1DArray.MipLevels = mipCount;
			}
			else
			{
				srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
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
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
						srv_desc.TextureCubeArray.First2DArrayFace = firstSlice;
						srv_desc.TextureCubeArray.NumCubes = std::min(texture->desc.ArraySize, sliceCount) / 6;
						srv_desc.TextureCubeArray.MostDetailedMip = firstMip;
						srv_desc.TextureCubeArray.MipLevels = mipCount;
					}
					else
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
						srv_desc.TextureCube.MostDetailedMip = firstMip;
						srv_desc.TextureCube.MipLevels = mipCount;
					}
				}
				else
				{
					if (texture->desc.SampleCount > 1)
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
						srv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
						srv_desc.Texture2DMSArray.ArraySize = sliceCount;
					}
					else
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
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
					srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					srv_desc.Texture2D.MostDetailedMip = firstMip;
					srv_desc.Texture2D.MipLevels = mipCount;
				}
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_3D)
		{
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			srv_desc.Texture3D.MostDetailedMip = firstMip;
			srv_desc.Texture3D.MipLevels = mipCount;
		}

		ComPtr<ID3D11ShaderResourceView> srv;
		HRESULT hr = device->CreateShaderResourceView(internal_state->resource.Get(), &srv_desc, &srv);
		if (SUCCEEDED(hr))
		{
			if (!internal_state->srv)
			{
				internal_state->srv = srv;
				return -1;
			}
			internal_state->subresources_srv.push_back(srv);
			return int(internal_state->subresources_srv.size() - 1);
		}
		else
		{
			assert(0);
		}
	}
	break;
	case wiGraphics::UAV:
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};

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
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
				uav_desc.Texture1DArray.FirstArraySlice = firstSlice;
				uav_desc.Texture1DArray.ArraySize = sliceCount;
				uav_desc.Texture1DArray.MipSlice = firstMip;
			}
			else
			{
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
				uav_desc.Texture1D.MipSlice = firstMip;
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_2D)
		{
			if (texture->desc.ArraySize > 1)
			{
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
				uav_desc.Texture2DArray.FirstArraySlice = firstSlice;
				uav_desc.Texture2DArray.ArraySize = sliceCount;
				uav_desc.Texture2DArray.MipSlice = firstMip;
			}
			else
			{
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
				uav_desc.Texture2D.MipSlice = firstMip;
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_3D)
		{
			uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
			uav_desc.Texture3D.MipSlice = firstMip;
			uav_desc.Texture3D.FirstWSlice = 0;
			uav_desc.Texture3D.WSize = -1;
		}
		
		ComPtr<ID3D11UnorderedAccessView> uav;
		HRESULT hr = device->CreateUnorderedAccessView(internal_state->resource.Get(), &uav_desc, &uav);
		if (SUCCEEDED(hr))
		{
			if (!internal_state->uav)
			{
				internal_state->uav = uav;
				return -1;
			}
			internal_state->subresources_uav.push_back(uav);
			return int(internal_state->subresources_uav.size() - 1);
		}
		else
		{
			assert(0);
		}
	}
	break;
	case wiGraphics::RTV:
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};

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
				rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
				rtv_desc.Texture1DArray.FirstArraySlice = firstSlice;
				rtv_desc.Texture1DArray.ArraySize = sliceCount;
				rtv_desc.Texture1DArray.MipSlice = firstMip;
			}
			else
			{
				rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
				rtv_desc.Texture1D.MipSlice = firstMip;
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_2D)
		{
			if (texture->desc.ArraySize > 1)
			{
				if (texture->desc.SampleCount > 1)
				{
					rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
					rtv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
					rtv_desc.Texture2DMSArray.ArraySize = sliceCount;
				}
				else
				{
					rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
					rtv_desc.Texture2DArray.FirstArraySlice = firstSlice;
					rtv_desc.Texture2DArray.ArraySize = sliceCount;
					rtv_desc.Texture2DArray.MipSlice = firstMip;
				}
			}
			else
			{
				if (texture->desc.SampleCount > 1)
				{
					rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
					rtv_desc.Texture2D.MipSlice = firstMip;
				}
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_3D)
		{
			rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
			rtv_desc.Texture3D.MipSlice = firstMip;
			rtv_desc.Texture3D.FirstWSlice = 0;
			rtv_desc.Texture3D.WSize = -1;
		}

		ComPtr<ID3D11RenderTargetView> rtv;
		HRESULT hr = device->CreateRenderTargetView(internal_state->resource.Get(), &rtv_desc, &rtv);
		if (SUCCEEDED(hr))
		{
			if (!internal_state->rtv)
			{
				internal_state->rtv = rtv;
				return -1;
			}
			internal_state->subresources_rtv.push_back(rtv);
			return int(internal_state->subresources_rtv.size() - 1);
		}
		else
		{
			assert(0);
		}
	}
	break;
	case wiGraphics::DSV:
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};

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
				dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
				dsv_desc.Texture1DArray.FirstArraySlice = firstSlice;
				dsv_desc.Texture1DArray.ArraySize = sliceCount;
				dsv_desc.Texture1DArray.MipSlice = firstMip;
			}
			else
			{
				dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
				dsv_desc.Texture1D.MipSlice = firstMip;
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_2D)
		{
			if (texture->desc.ArraySize > 1)
			{
				if (texture->desc.SampleCount > 1)
				{
					dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
					dsv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
					dsv_desc.Texture2DMSArray.ArraySize = sliceCount;
				}
				else
				{
					dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
					dsv_desc.Texture2DArray.FirstArraySlice = firstSlice;
					dsv_desc.Texture2DArray.ArraySize = sliceCount;
					dsv_desc.Texture2DArray.MipSlice = firstMip;
				}
			}
			else
			{
				if (texture->desc.SampleCount > 1)
				{
					dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
					dsv_desc.Texture2D.MipSlice = firstMip;
				}
			}
		}

		ComPtr<ID3D11DepthStencilView> dsv;
		HRESULT hr = device->CreateDepthStencilView(internal_state->resource.Get(), &dsv_desc, &dsv);
		if (SUCCEEDED(hr))
		{
			if (!internal_state->dsv)
			{
				internal_state->dsv = dsv;
				return -1;
			}
			internal_state->subresources_dsv.push_back(dsv);
			return int(internal_state->subresources_dsv.size() - 1);
		}
		else
		{
			assert(0);
		}
	}
	break;
	default:
		break;
	}
	return -1;
}
int GraphicsDevice_DX11::CreateSubresource(GPUBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size)
{
	auto internal_state = to_internal(buffer);
	const GPUBufferDesc& desc = buffer->GetDesc();
	HRESULT hr = E_FAIL;

	switch (type)
	{
	case wiGraphics::SRV:
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

		if (desc.MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
		{
			// This is a Raw Buffer
			srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			srv_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
			srv_desc.BufferEx.FirstElement = (UINT)offset / sizeof(uint32_t);
			srv_desc.BufferEx.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / sizeof(uint32_t);
		}
		else if (desc.MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
		{
			// This is a Structured Buffer
			srv_desc.Format = DXGI_FORMAT_UNKNOWN;
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			srv_desc.BufferEx.FirstElement = (UINT)offset / desc.StructureByteStride;
			srv_desc.BufferEx.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / desc.StructureByteStride;
		}
		else
		{
			// This is a Typed Buffer
			uint32_t stride = GetFormatStride(desc.Format);
			srv_desc.Format = _ConvertFormat(desc.Format);
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srv_desc.Buffer.FirstElement = (UINT)offset / stride;
			srv_desc.Buffer.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / stride;
		}

		ComPtr<ID3D11ShaderResourceView> srv;
		hr = device->CreateShaderResourceView(internal_state->resource.Get(), &srv_desc, &srv);

		if (SUCCEEDED(hr))
		{
			if (internal_state->srv == nullptr)
			{
				internal_state->srv = srv;
				return -1;
			}
			else
			{
				internal_state->subresources_srv.push_back(srv);
				return int(internal_state->subresources_srv.size() - 1);
			}
		}
		else
		{
			assert(0);
		}
	}
	break;
	case wiGraphics::UAV:
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
		uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

		if (desc.MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
		{
			// This is a Raw Buffer
			uav_desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
			uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
			uav_desc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
			uav_desc.Buffer.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / sizeof(uint32_t);
		}
		else if (desc.MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
		{
			// This is a Structured Buffer
			uav_desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
			uav_desc.Buffer.FirstElement = (UINT)offset / desc.StructureByteStride;
			uav_desc.Buffer.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / desc.StructureByteStride;
		}
		else
		{
			// This is a Typed Buffer
			uint32_t stride = GetFormatStride(desc.Format);
			uav_desc.Format = _ConvertFormat(desc.Format);
			uav_desc.Buffer.FirstElement = (UINT)offset / stride;
			uav_desc.Buffer.NumElements = std::min((UINT)size, desc.ByteWidth - (UINT)offset) / stride;
		}

		ComPtr<ID3D11UnorderedAccessView> uav;
		hr = device->CreateUnorderedAccessView(internal_state->resource.Get(), &uav_desc, &uav);

		if (SUCCEEDED(hr))
		{
			if (internal_state->uav == nullptr)
			{
				internal_state->uav = uav;
				return -1;
			}
			else
			{
				internal_state->subresources_uav.push_back(uav);
				return int(internal_state->subresources_uav.size() - 1);
			}
		}
		else
		{
			assert(0);
		}
	}
	break;
	default:
		assert(0);
		break;
	}
	return -1;
}

void GraphicsDevice_DX11::Map(const GPUResource* resource, Mapping* mapping)
{
	auto internal_state = to_internal(resource);

	D3D11_MAPPED_SUBRESOURCE map_result = {};
	D3D11_MAP map_type = D3D11_MAP_READ_WRITE;
	if (mapping->_flags & Mapping::FLAG_READ)
	{
		if (mapping->_flags & Mapping::FLAG_WRITE)
		{
			map_type = D3D11_MAP_READ_WRITE;
		}
		else
		{
			map_type = D3D11_MAP_READ;
		}
	}
	else if (mapping->_flags & Mapping::FLAG_WRITE)
	{
		map_type = D3D11_MAP_WRITE_NO_OVERWRITE;
	}
	HRESULT hr = immediateContext->Map(internal_state->resource.Get(), 0, map_type, D3D11_MAP_FLAG_DO_NOT_WAIT, &map_result);
	if (SUCCEEDED(hr))
	{
		mapping->data = map_result.pData;
		mapping->rowpitch = map_result.RowPitch;
	}
	else
	{
		assert(0);
		mapping->data = nullptr;
		mapping->rowpitch = 0;
	}
}
void GraphicsDevice_DX11::Unmap(const GPUResource* resource)
{
	auto internal_state = to_internal(resource);
	immediateContext->Unmap(internal_state->resource.Get(), 0);
}
bool GraphicsDevice_DX11::QueryRead(const GPUQuery* query, GPUQueryResult* result)
{
	const uint32_t _flags = D3D11_ASYNC_GETDATA_DONOTFLUSH;

	auto internal_state = to_internal(query);
	ID3D11Query* QUERY = internal_state->resource.Get();

	HRESULT hr = S_OK;
	switch (query->desc.Type)
	{
	case GPU_QUERY_TYPE_TIMESTAMP:
		hr = immediateContext->GetData(QUERY, &result->result_timestamp, sizeof(uint64_t), _flags);
		break;
	case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
	{
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT _temp;
		hr = immediateContext->GetData(QUERY, &_temp, sizeof(_temp), _flags);
		result->result_timestamp_frequency = _temp.Frequency;
	}
	break;
	case GPU_QUERY_TYPE_EVENT:
	case GPU_QUERY_TYPE_OCCLUSION:
		hr = immediateContext->GetData(QUERY, &result->result_passed_sample_count, sizeof(uint64_t), _flags);
		break;
	case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
	{
		BOOL passed = FALSE;
		hr = immediateContext->GetData(QUERY, &passed, sizeof(BOOL), _flags);
		result->result_passed_sample_count = (uint64_t)passed;
		break;
	}
	}

	return hr != S_FALSE;
}

void GraphicsDevice_DX11::SetName(GPUResource* pResource, const char* name)
{
	auto internal_state = to_internal(pResource);
	internal_state->resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name), name);
}

void GraphicsDevice_DX11::PresentBegin(CommandList cmd)
{
	ID3D11RenderTargetView* RTV = renderTargetView.Get();
	deviceContexts[cmd]->OMSetRenderTargets(1, &RTV, 0);
	float ClearColor[4] = { 0, 0, 0, 1.0f }; // red,green,blue,alpha
	deviceContexts[cmd]->ClearRenderTargetView(RTV, ClearColor);
}
void GraphicsDevice_DX11::PresentEnd(CommandList cmd)
{
	SubmitCommandLists();

	swapChain->Present(VSYNC, 0);
}


CommandList GraphicsDevice_DX11::BeginCommandList()
{
	CommandList cmd = cmd_count.fetch_add(1);
	if (deviceContexts[cmd] == nullptr)
	{
		// need to create one more command list:
		assert(cmd < COMMANDLIST_COUNT);

		HRESULT hr = device->CreateDeferredContext(0, &deviceContexts[cmd]);
		assert(SUCCEEDED(hr));

		hr = deviceContexts[cmd].As(&userDefinedAnnotations[cmd]);
		assert(SUCCEEDED(hr));

		// Temporary allocations will use the following buffer type:
		GPUBufferDesc frameAllocatorDesc;
		frameAllocatorDesc.ByteWidth = 1024 * 1024; // 1 MB starting size
		frameAllocatorDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_INDEX_BUFFER | BIND_VERTEX_BUFFER;
		frameAllocatorDesc.Usage = USAGE_DYNAMIC;
		frameAllocatorDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
		frameAllocatorDesc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		bool success = CreateBuffer(&frameAllocatorDesc, nullptr, &frame_allocators[cmd].buffer);
		assert(success);
		SetName(&frame_allocators[cmd].buffer, "frame_allocator");

	}

	BindPipelineState(nullptr, cmd);
	BindComputeShader(nullptr, cmd);

	D3D11_VIEWPORT vp = {};
	vp.Width = (float)RESOLUTIONWIDTH;
	vp.Height = (float)RESOLUTIONHEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	deviceContexts[cmd]->RSSetViewports(1, &vp);

	D3D11_RECT pRects[8];
	for (uint32_t i = 0; i < 8; ++i)
	{
		pRects[i].bottom = INT32_MAX;
		pRects[i].left = INT32_MIN;
		pRects[i].right = INT32_MAX;
		pRects[i].top = INT32_MIN;
	}
	deviceContexts[cmd]->RSSetScissorRects(8, pRects);

	stencilRef[cmd] = 0;
	blendFactor[cmd] = XMFLOAT4(1, 1, 1, 1);

	prev_vs[cmd] = {};
	prev_ps[cmd] = {};
	prev_hs[cmd] = {};
	prev_ds[cmd] = {};
	prev_gs[cmd] = {};
	prev_cs[cmd] = {};
	prev_blendfactor[cmd] = {};
	prev_samplemask[cmd] = {};
	prev_bs[cmd] = {};
	prev_rs[cmd] = {};
	prev_stencilRef[cmd] = {};
	prev_dss[cmd] = {};
	prev_il[cmd] = {};
	prev_pt[cmd] = {};

	memset(raster_uavs[cmd], 0, sizeof(raster_uavs[cmd]));
	raster_uavs_slot[cmd] = {};
	raster_uavs_count[cmd] = {};

	active_pso[cmd] = nullptr;
	dirty_pso[cmd] = false;
	active_renderpass[cmd] = nullptr;

	return cmd;
}
void GraphicsDevice_DX11::SubmitCommandLists()
{
	// Execute deferred command lists:
	{
		CommandList cmd_last = cmd_count.load();
		cmd_count.store(0);
		for (CommandList cmd = 0; cmd < cmd_last; ++cmd)
		{
			deviceContexts[cmd]->FinishCommandList(false, &commandLists[cmd]);
			immediateContext->ExecuteCommandList(commandLists[cmd].Get(), false);
			commandLists[cmd].Reset();
		}
	}
	immediateContext->ClearState();

	FRAMECOUNT++;
}

void GraphicsDevice_DX11::WaitForGPU()
{
	immediateContext->Flush();

	GPUQuery query;
	GPUQueryDesc desc;
	desc.Type = GPU_QUERY_TYPE_EVENT;
	bool success = CreateQuery(&desc, &query);
	assert(success);
	auto internal_state = to_internal(&query);
	immediateContext->End(internal_state->resource.Get());
	BOOL result;
	while (immediateContext->GetData(internal_state->resource.Get(), &result, sizeof(result), 0) == S_FALSE);
	assert(result == TRUE);
}


void GraphicsDevice_DX11::commit_allocations(CommandList cmd)
{
	// DX11 needs to unmap allocations before it can execute safely

	if (frame_allocators[cmd].dirty)
	{
		auto internal_state = to_internal(&frame_allocators[cmd].buffer);
		deviceContexts[cmd]->Unmap(internal_state->resource.Get(), 0);
		frame_allocators[cmd].dirty = false;
	}
}


void GraphicsDevice_DX11::RenderPassBegin(const RenderPass* renderpass, CommandList cmd)
{
	active_renderpass[cmd] = renderpass;
	const RenderPassDesc& desc = renderpass->GetDesc();

	uint32_t rt_count = 0;
	ID3D11RenderTargetView* RTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
	ID3D11DepthStencilView* DSV = nullptr;
	assert(desc.attachments.size() < arraysize(RTVs) + 1);
	for (auto& attachment : desc.attachments)
	{
		const Texture* texture = attachment.texture;
		int subresource = attachment.subresource;
		auto internal_state = to_internal(texture);

		if (attachment.type == RenderPassAttachment::RENDERTARGET)
		{
			if (subresource < 0 || internal_state->subresources_rtv.empty())
			{
				RTVs[rt_count] = internal_state->rtv.Get();
			}
			else
			{
				assert(internal_state->subresources_rtv.size() > size_t(subresource) && "Invalid RTV subresource!");
				RTVs[rt_count] = internal_state->subresources_rtv[subresource].Get();
			}

			if (attachment.loadop == RenderPassAttachment::LOADOP_CLEAR)
			{
				deviceContexts[cmd]->ClearRenderTargetView(RTVs[rt_count], texture->desc.clear.color);
			}

			rt_count++;
		}
		else if (attachment.type == RenderPassAttachment::DEPTH_STENCIL)
		{
			if (subresource < 0 || internal_state->subresources_dsv.empty())
			{
				DSV = internal_state->dsv.Get();
			}
			else
			{
				assert(internal_state->subresources_dsv.size() > size_t(subresource) && "Invalid DSV subresource!");
				DSV = internal_state->subresources_dsv[subresource].Get();
			}

			if (attachment.loadop == RenderPassAttachment::LOADOP_CLEAR)
			{
				uint32_t _flags = D3D11_CLEAR_DEPTH;
				if (IsFormatStencilSupport(texture->desc.Format))
					_flags |= D3D11_CLEAR_STENCIL;
				deviceContexts[cmd]->ClearDepthStencilView(DSV, _flags, texture->desc.clear.depthstencil.depth, texture->desc.clear.depthstencil.stencil);
			}
		}
	}

	if (raster_uavs_count[cmd] > 0)
	{
		// UAVs:
		const uint32_t count = raster_uavs_count[cmd];
		const uint32_t slot = raster_uavs_slot[cmd];

		deviceContexts[cmd]->OMSetRenderTargetsAndUnorderedAccessViews(rt_count, RTVs, DSV, slot, count, &raster_uavs[cmd][slot], nullptr);

		raster_uavs_count[cmd] = 0;
		raster_uavs_slot[cmd] = 8;
	}
	else
	{
		deviceContexts[cmd]->OMSetRenderTargets(rt_count, RTVs, DSV);
	}
}
void GraphicsDevice_DX11::RenderPassEnd(CommandList cmd)
{
	deviceContexts[cmd]->OMSetRenderTargets(0, nullptr, nullptr);

	// Perform resolves:
	int dst_counter = 0;
	for (auto& attachment : active_renderpass[cmd]->desc.attachments)
	{
		if (attachment.type == RenderPassAttachment::RESOLVE)
		{
			if (attachment.texture != nullptr)
			{
				auto dst_internal = to_internal(attachment.texture);

				int src_counter = 0;
				for (auto& src : active_renderpass[cmd]->desc.attachments)
				{
					if (src.type == RenderPassAttachment::RENDERTARGET && src.texture != nullptr)
					{
						if (src_counter == dst_counter)
						{
							auto src_internal = to_internal(src.texture);
							deviceContexts[cmd]->ResolveSubresource(dst_internal->resource.Get(), 0, src_internal->resource.Get(), 0, _ConvertFormat(attachment.texture->desc.Format));
							break;
						}
						src_counter++;
					}
				}
			}

			dst_counter++;
		}
	}
	active_renderpass[cmd] = nullptr;
}
void GraphicsDevice_DX11::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) {
	assert(rects != nullptr);
	assert(numRects <= 8);
	D3D11_RECT pRects[8];
	for(uint32_t i = 0; i < numRects; ++i) {
		pRects[i].bottom = (LONG)rects[i].bottom;
		pRects[i].left = (LONG)rects[i].left;
		pRects[i].right = (LONG)rects[i].right;
		pRects[i].top = (LONG)rects[i].top;
	}
	deviceContexts[cmd]->RSSetScissorRects(numRects, pRects);
}
void GraphicsDevice_DX11::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
{
	assert(NumViewports <= 6);
	D3D11_VIEWPORT d3dViewPorts[6];
	for (uint32_t i = 0; i < NumViewports; ++i)
	{
		d3dViewPorts[i].TopLeftX = pViewports[i].TopLeftX;
		d3dViewPorts[i].TopLeftY = pViewports[i].TopLeftY;
		d3dViewPorts[i].Width = pViewports[i].Width;
		d3dViewPorts[i].Height = pViewports[i].Height;
		d3dViewPorts[i].MinDepth = pViewports[i].MinDepth;
		d3dViewPorts[i].MaxDepth = pViewports[i].MaxDepth;
	}
	deviceContexts[cmd]->RSSetViewports(NumViewports, d3dViewPorts);
}
void GraphicsDevice_DX11::BindResource(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
{
	if (resource != nullptr && resource->IsValid())
	{
		auto internal_state = to_internal(resource);
		ID3D11ShaderResourceView* SRV;

		if (subresource < 0)
		{
			SRV = internal_state->srv.Get();
		}
		else
		{
			assert(internal_state->subresources_srv.size() > static_cast<size_t>(subresource) && "Invalid subresource!");
			SRV = internal_state->subresources_srv[subresource].Get();
		}

		switch (stage)
		{
		case wiGraphics::VS:
			deviceContexts[cmd]->VSSetShaderResources(slot, 1, &SRV);
			break;
		case wiGraphics::HS:
			deviceContexts[cmd]->HSSetShaderResources(slot, 1, &SRV);
			break;
		case wiGraphics::DS:
			deviceContexts[cmd]->DSSetShaderResources(slot, 1, &SRV);
			break;
		case wiGraphics::GS:
			deviceContexts[cmd]->GSSetShaderResources(slot, 1, &SRV);
			break;
		case wiGraphics::PS:
			deviceContexts[cmd]->PSSetShaderResources(slot, 1, &SRV);
			break;
		case wiGraphics::CS:
			deviceContexts[cmd]->CSSetShaderResources(slot, 1, &SRV);
			break;
		default:
			assert(0);
			break;
		}
	}
}
void GraphicsDevice_DX11::BindResources(SHADERSTAGE stage, const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
{
	assert(count <= 16);
	ID3D11ShaderResourceView* srvs[16];
	for (uint32_t i = 0; i < count; ++i)
	{
		srvs[i] = resources[i] != nullptr && resources[i]->IsValid() ? to_internal(resources[i])->srv.Get() : nullptr;
	}

	switch (stage)
	{
	case wiGraphics::VS:
		deviceContexts[cmd]->VSSetShaderResources(slot, count, srvs);
		break;
	case wiGraphics::HS:
		deviceContexts[cmd]->HSSetShaderResources(slot, count, srvs);
		break;
	case wiGraphics::DS:
		deviceContexts[cmd]->DSSetShaderResources(slot, count, srvs);
		break;
	case wiGraphics::GS:
		deviceContexts[cmd]->GSSetShaderResources(slot, count, srvs);
		break;
	case wiGraphics::PS:
		deviceContexts[cmd]->PSSetShaderResources(slot, count, srvs);
		break;
	case wiGraphics::CS:
		deviceContexts[cmd]->CSSetShaderResources(slot, count, srvs);
		break;
	default:
		assert(0);
		break;
	}
}
void GraphicsDevice_DX11::BindUAV(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
{
	if (resource != nullptr && resource->IsValid())
	{
		auto internal_state = to_internal(resource);
		ID3D11UnorderedAccessView* UAV;

		if (subresource < 0)
		{
			UAV = internal_state->uav.Get();
		}
		else
		{
			assert(internal_state->subresources_uav.size() > static_cast<size_t>(subresource) && "Invalid subresource!");
			UAV = internal_state->subresources_uav[subresource].Get();
		}

		if (stage == CS)
		{
			deviceContexts[cmd]->CSSetUnorderedAccessViews(slot, 1, &UAV, nullptr);
		}
		else
		{
			raster_uavs[cmd][slot] = UAV;
			raster_uavs_slot[cmd] = std::min(raster_uavs_slot[cmd], uint8_t(slot));
			raster_uavs_count[cmd] = std::max(raster_uavs_count[cmd], uint8_t(1));
		}
	}
}
void GraphicsDevice_DX11::BindUAVs(SHADERSTAGE stage, const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
{
	assert(slot + count <= 8);
	ID3D11UnorderedAccessView* uavs[8];
	for (uint32_t i = 0; i < count; ++i)
	{
		uavs[i] = resources[i] != nullptr && resources[i]->IsValid() ? to_internal(resources[i])->uav.Get() : nullptr;

		if(stage != CS)
		{
			raster_uavs[cmd][slot + i] = uavs[i];
		}
	}

	if(stage == CS)
	{
		deviceContexts[cmd]->CSSetUnorderedAccessViews(static_cast<uint32_t>(slot), static_cast<uint32_t>(count), uavs, nullptr);
	}
	else
	{
		raster_uavs_slot[cmd] = std::min(raster_uavs_slot[cmd], uint8_t(slot));
		raster_uavs_count[cmd] = std::max(raster_uavs_count[cmd], uint8_t(count));
	}
}
void GraphicsDevice_DX11::UnbindResources(uint32_t slot, uint32_t num, CommandList cmd)
{
	assert(num <= arraysize(__nullBlob) && "Extend nullBlob to support more resource unbinding!");
	deviceContexts[cmd]->PSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
	deviceContexts[cmd]->VSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
	deviceContexts[cmd]->GSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
	deviceContexts[cmd]->HSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
	deviceContexts[cmd]->DSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
	deviceContexts[cmd]->CSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
}
void GraphicsDevice_DX11::UnbindUAVs(uint32_t slot, uint32_t num, CommandList cmd)
{
	assert(num <= arraysize(__nullBlob) && "Extend nullBlob to support more resource unbinding!");
	deviceContexts[cmd]->CSSetUnorderedAccessViews(slot, num, (ID3D11UnorderedAccessView**)__nullBlob, 0);

	raster_uavs_count[cmd] = 0;
	raster_uavs_slot[cmd] = 8;
}
void GraphicsDevice_DX11::BindSampler(SHADERSTAGE stage, const Sampler* sampler, uint32_t slot, CommandList cmd)
{
	if (sampler != nullptr && sampler->IsValid())
	{
		auto internal_state = to_internal(sampler);
		ID3D11SamplerState* SAM = internal_state->resource.Get();

		switch (stage)
		{
		case wiGraphics::VS:
			deviceContexts[cmd]->VSSetSamplers(slot, 1, &SAM);
			break;
		case wiGraphics::HS:
			deviceContexts[cmd]->HSSetSamplers(slot, 1, &SAM);
			break;
		case wiGraphics::DS:
			deviceContexts[cmd]->DSSetSamplers(slot, 1, &SAM);
			break;
		case wiGraphics::GS:
			deviceContexts[cmd]->GSSetSamplers(slot, 1, &SAM);
			break;
		case wiGraphics::PS:
			deviceContexts[cmd]->PSSetSamplers(slot, 1, &SAM);
			break;
		case wiGraphics::CS:
			deviceContexts[cmd]->CSSetSamplers(slot, 1, &SAM);
			break;
		case MS:
		case AS:
			break;
		default:
			assert(0);
			break;
		}
	}
}
void GraphicsDevice_DX11::BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, uint32_t slot, CommandList cmd)
{
	ID3D11Buffer* res = buffer != nullptr && buffer->IsValid() ? (ID3D11Buffer*)to_internal(buffer)->resource.Get() : nullptr;
	switch (stage)
	{
	case wiGraphics::VS:
		deviceContexts[cmd]->VSSetConstantBuffers(slot, 1, &res);
		break;
	case wiGraphics::HS:
		deviceContexts[cmd]->HSSetConstantBuffers(slot, 1, &res);
		break;
	case wiGraphics::DS:
		deviceContexts[cmd]->DSSetConstantBuffers(slot, 1, &res);
		break;
	case wiGraphics::GS:
		deviceContexts[cmd]->GSSetConstantBuffers(slot, 1, &res);
		break;
	case wiGraphics::PS:
		deviceContexts[cmd]->PSSetConstantBuffers(slot, 1, &res);
		break;
	case wiGraphics::CS:
		deviceContexts[cmd]->CSSetConstantBuffers(slot, 1, &res);
		break;
	case MS:
	case AS:
		break;
	default:
		assert(0);
		break;
	}
}
void GraphicsDevice_DX11::BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets, CommandList cmd)
{
	assert(count <= 8);
	ID3D11Buffer* res[8] = { 0 };
	for (uint32_t i = 0; i < count; ++i)
	{
		res[i] = vertexBuffers[i] != nullptr && vertexBuffers[i]->IsValid() ? (ID3D11Buffer*)to_internal(vertexBuffers[i])->resource.Get() : nullptr;
	}
	deviceContexts[cmd]->IASetVertexBuffers(slot, count, res, strides, (offsets != nullptr ? offsets : reinterpret_cast<const uint32_t*>(__nullBlob)));
}
void GraphicsDevice_DX11::BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, uint32_t offset, CommandList cmd)
{
	ID3D11Buffer* res = indexBuffer != nullptr && indexBuffer->IsValid() ? (ID3D11Buffer*)to_internal(indexBuffer)->resource.Get() : nullptr;
	deviceContexts[cmd]->IASetIndexBuffer(res, (format == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT), offset);
}
void GraphicsDevice_DX11::BindStencilRef(uint32_t value, CommandList cmd)
{
	stencilRef[cmd] = value;
}
void GraphicsDevice_DX11::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
{
	blendFactor[cmd].x = r;
	blendFactor[cmd].y = g;
	blendFactor[cmd].z = b;
	blendFactor[cmd].w = a;
}
void GraphicsDevice_DX11::BindPipelineState(const PipelineState* pso, CommandList cmd)
{
	if (active_pso[cmd] == pso)
		return;

	active_pso[cmd] = pso;
	dirty_pso[cmd] = true;
}
void GraphicsDevice_DX11::BindComputeShader(const Shader* cs, CommandList cmd)
{
	ID3D11ComputeShader* _cs = cs == nullptr ? nullptr : static_cast<ComputeShader_DX11*>(cs->internal_state.get())->resource.Get();
	if (_cs != prev_cs[cmd])
	{
		deviceContexts[cmd]->CSSetShader(_cs, nullptr, 0);
		prev_cs[cmd] = _cs;
	}
}
void GraphicsDevice_DX11::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) 
{
	pso_validate(cmd);
	commit_allocations(cmd);

	deviceContexts[cmd]->Draw(vertexCount, startVertexLocation);
}
void GraphicsDevice_DX11::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, CommandList cmd)
{
	pso_validate(cmd);
	commit_allocations(cmd);

	deviceContexts[cmd]->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
}
void GraphicsDevice_DX11::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) 
{
	pso_validate(cmd);
	commit_allocations(cmd);

	deviceContexts[cmd]->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
}
void GraphicsDevice_DX11::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
{
	pso_validate(cmd);
	commit_allocations(cmd);

	deviceContexts[cmd]->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}
void GraphicsDevice_DX11::DrawInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
{
	pso_validate(cmd);
	commit_allocations(cmd);

	deviceContexts[cmd]->DrawInstancedIndirect((ID3D11Buffer*)to_internal(args)->resource.Get(), args_offset);
}
void GraphicsDevice_DX11::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
{
	pso_validate(cmd);
	commit_allocations(cmd);

	deviceContexts[cmd]->DrawIndexedInstancedIndirect((ID3D11Buffer*)to_internal(args)->resource.Get(), args_offset);
}
void GraphicsDevice_DX11::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
{
	commit_allocations(cmd);

	deviceContexts[cmd]->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
void GraphicsDevice_DX11::DispatchIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
{
	commit_allocations(cmd);

	deviceContexts[cmd]->DispatchIndirect((ID3D11Buffer*)to_internal(args)->resource.Get(), args_offset);
}
void GraphicsDevice_DX11::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
{
	assert(pDst != nullptr && pSrc != nullptr);
	auto internal_state_src = to_internal(pSrc);
	auto internal_state_dst = to_internal(pDst);
	deviceContexts[cmd]->CopyResource(internal_state_dst->resource.Get(), internal_state_src->resource.Get());
}
void GraphicsDevice_DX11::UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize)
{
	assert(buffer->desc.Usage != USAGE_IMMUTABLE && "Cannot update IMMUTABLE GPUBuffer!");
	assert((int)buffer->desc.ByteWidth >= dataSize || dataSize < 0 && "Data size is too big!");

	if (dataSize == 0)
	{
		return;
	}

	auto internal_state = to_internal(buffer);

	dataSize = std::min((int)buffer->desc.ByteWidth, dataSize);

	if (buffer->desc.Usage == USAGE_DYNAMIC)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = deviceContexts[cmd]->Map(internal_state->resource.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		assert(SUCCEEDED(hr) && "GPUBuffer mapping failed!");
		memcpy(mappedResource.pData, data, (dataSize >= 0 ? dataSize : buffer->desc.ByteWidth));
		deviceContexts[cmd]->Unmap(internal_state->resource.Get(), 0);
	}
	else if (buffer->desc.BindFlags & BIND_CONSTANT_BUFFER || dataSize < 0)
	{
		deviceContexts[cmd]->UpdateSubresource(internal_state->resource.Get(), 0, nullptr, data, 0, 0);
	}
	else
	{
		D3D11_BOX box = {};
		box.left = 0;
		box.right = static_cast<uint32_t>(dataSize);
		box.top = 0;
		box.bottom = 1;
		box.front = 0;
		box.back = 1;
		deviceContexts[cmd]->UpdateSubresource(internal_state->resource.Get(), 0, &box, data, 0, 0);
	}
}
void GraphicsDevice_DX11::QueryBegin(const GPUQuery* query, CommandList cmd)
{
	auto internal_state = to_internal(query);
	deviceContexts[cmd]->Begin(internal_state->resource.Get());
}
void GraphicsDevice_DX11::QueryEnd(const GPUQuery* query, CommandList cmd)
{
	auto internal_state = to_internal(query);
	deviceContexts[cmd]->End(internal_state->resource.Get());
}

GraphicsDevice::GPUAllocation GraphicsDevice_DX11::AllocateGPU(size_t dataSize, CommandList cmd)
{
	GPUAllocation result;
	if (dataSize == 0)
	{
		return result;
	}

	GPUAllocator& allocator = frame_allocators[cmd];
	if (allocator.buffer.desc.ByteWidth <= dataSize)
	{
		// If allocation too large, grow the allocator:
		allocator.buffer.desc.ByteWidth = uint32_t((dataSize + 1) * 2);
		bool success = CreateBuffer(&allocator.buffer.desc, nullptr, &allocator.buffer);
		assert(success);
		SetName(&allocator.buffer, "frame_allocator");
		allocator.byteOffset = 0;
	}

	auto internal_state = to_internal(&allocator.buffer);

	allocator.dirty = true;

	size_t position = allocator.byteOffset;
	bool wrap = position == 0 || position + dataSize > allocator.buffer.desc.ByteWidth || allocator.residentFrame != FRAMECOUNT;
	position = wrap ? 0 : position;

	// Issue buffer rename (realloc) on wrap, otherwise just append data:
	D3D11_MAP mapping = wrap ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = deviceContexts[cmd]->Map(internal_state->resource.Get(), 0, mapping, 0, &mappedResource);
	assert(SUCCEEDED(hr) && "GPUBuffer mapping failed!");

	allocator.byteOffset = position + dataSize;
	allocator.residentFrame = FRAMECOUNT;

	result.buffer = &allocator.buffer;
	result.offset = (uint32_t)position;
	result.data = (void*)((size_t)mappedResource.pData + position);
	return result;
}

void GraphicsDevice_DX11::EventBegin(const char* name, CommandList cmd)
{
	wchar_t text[128];
	if (wiHelper::StringConvert(name, text) > 0)
	{
		userDefinedAnnotations[cmd]->BeginEvent(text);
	}
}
void GraphicsDevice_DX11::EventEnd(CommandList cmd)
{
	userDefinedAnnotations[cmd]->EndEvent();
}
void GraphicsDevice_DX11::SetMarker(const char* name, CommandList cmd)
{
	wchar_t text[128];
	if (wiHelper::StringConvert(name, text) > 0)
	{
		userDefinedAnnotations[cmd]->SetMarker(text);
	}
}

}

#endif // WICKEDENGINE_BUILD_DX11
