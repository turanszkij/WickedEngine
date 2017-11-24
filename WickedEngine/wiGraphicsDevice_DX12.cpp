#include "wiGraphicsDevice_DX12.h"
#include "Include_DX12.h"
#include "wiHelper.h"
#include "ResourceMapping.h"

#include "Utility/WicTextureLoader.h"
#include "Utility/DDSTextureLoader.h"
#include "Utility/ScreenGrab.h"

#include <sstream>
#include <wincodec.h>

using namespace std;

namespace wiGraphicsTypes
{
	// Engine -> Native converters

	inline UINT _ParseColorWriteMask(UINT value)
	{
		UINT _flag = 0;

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

	inline D3D12_FILTER _ConvertFilter(FILTER value)
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
	inline D3D12_TEXTURE_ADDRESS_MODE _ConvertTextureAddressMode(TEXTURE_ADDRESS_MODE value)
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
		case TEXTURE_ADDRESS_MIRROR_ONCE:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			break;
		default:
			break;
		}
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
	inline D3D12_COMPARISON_FUNC _ConvertComparisonFunc(COMPARISON_FUNC value)
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
	inline D3D12_FILL_MODE _ConvertFillMode(FILL_MODE value)
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
	inline D3D12_CULL_MODE _ConvertCullMode(CULL_MODE value)
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
	inline D3D12_DEPTH_WRITE_MASK _ConvertDepthWriteMask(DEPTH_WRITE_MASK value)
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
	inline D3D12_STENCIL_OP _ConvertStencilOp(STENCIL_OP value)
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
	inline D3D12_BLEND _ConvertBlend(BLEND value)
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
	inline D3D12_BLEND_OP _ConvertBlendOp(BLEND_OP value)
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
	inline D3D12_INPUT_CLASSIFICATION _ConvertInputClassification(INPUT_CLASSIFICATION value)
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


	// Native -> Engine converters

	inline FORMAT _ConvertFormat_Inv(DXGI_FORMAT value)
	{
		switch (value)
		{
		case DXGI_FORMAT_UNKNOWN:
			return FORMAT_UNKNOWN;
			break;
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			return FORMAT_R32G32B32A32_TYPELESS;
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
		case DXGI_FORMAT_R32G32B32_TYPELESS:
			return FORMAT_R32G32B32_TYPELESS;
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
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			return FORMAT_R16G16B16A16_TYPELESS;
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
		case DXGI_FORMAT_R32G32_TYPELESS:
			return FORMAT_R32G32_TYPELESS;
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
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			return FORMAT_R32_FLOAT_X8X24_TYPELESS;
			break;
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return FORMAT_X32_TYPELESS_G8X24_UINT;
			break;
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
			return FORMAT_R10G10B10A2_TYPELESS;
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
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			return FORMAT_R8G8B8A8_TYPELESS;
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
		case DXGI_FORMAT_R16G16_TYPELESS:
			return FORMAT_R16G16_TYPELESS;
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
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			return FORMAT_R24_UNORM_X8_TYPELESS;
			break;
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			return FORMAT_X24_TYPELESS_G8_UINT;
			break;
		case DXGI_FORMAT_R8G8_TYPELESS:
			return FORMAT_R8G8_TYPELESS;
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
		case DXGI_FORMAT_R8_TYPELESS:
			return FORMAT_R8_TYPELESS;
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
		case DXGI_FORMAT_A8_UNORM:
			return FORMAT_A8_UNORM;
			break;
		case DXGI_FORMAT_R1_UNORM:
			return FORMAT_R1_UNORM;
			break;
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
			return FORMAT_R9G9B9E5_SHAREDEXP;
			break;
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
			return FORMAT_R8G8_B8G8_UNORM;
			break;
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
			return FORMAT_G8R8_G8B8_UNORM;
			break;
		case DXGI_FORMAT_BC1_TYPELESS:
			return FORMAT_BC1_TYPELESS;
			break;
		case DXGI_FORMAT_BC1_UNORM:
			return FORMAT_BC1_UNORM;
			break;
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return FORMAT_BC1_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC2_TYPELESS:
			return FORMAT_BC2_TYPELESS;
			break;
		case DXGI_FORMAT_BC2_UNORM:
			return FORMAT_BC2_UNORM;
			break;
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			return FORMAT_BC2_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC3_TYPELESS:
			return FORMAT_BC3_TYPELESS;
			break;
		case DXGI_FORMAT_BC3_UNORM:
			return FORMAT_BC3_UNORM;
			break;
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return FORMAT_BC3_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC4_TYPELESS:
			return FORMAT_BC4_TYPELESS;
			break;
		case DXGI_FORMAT_BC4_UNORM:
			return FORMAT_BC4_UNORM;
			break;
		case DXGI_FORMAT_BC4_SNORM:
			return FORMAT_BC4_SNORM;
			break;
		case DXGI_FORMAT_BC5_TYPELESS:
			return FORMAT_BC5_TYPELESS;
			break;
		case DXGI_FORMAT_BC5_UNORM:
			return FORMAT_BC5_UNORM;
			break;
		case DXGI_FORMAT_BC5_SNORM:
			return FORMAT_BC5_SNORM;
			break;
		case DXGI_FORMAT_B5G6R5_UNORM:
			return FORMAT_B5G6R5_UNORM;
			break;
		case DXGI_FORMAT_B5G5R5A1_UNORM:
			return FORMAT_B5G5R5A1_UNORM;
			break;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return FORMAT_B8G8R8A8_UNORM;
			break;
		case DXGI_FORMAT_B8G8R8X8_UNORM:
			return FORMAT_B8G8R8X8_UNORM;
			break;
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
			return FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
			break;
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			return FORMAT_B8G8R8A8_TYPELESS;
			break;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return FORMAT_B8G8R8A8_UNORM_SRGB;
			break;
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
			return FORMAT_B8G8R8X8_TYPELESS;
			break;
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return FORMAT_B8G8R8X8_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC6H_TYPELESS:
			return FORMAT_BC6H_TYPELESS;
			break;
		case DXGI_FORMAT_BC6H_UF16:
			return FORMAT_BC6H_UF16;
			break;
		case DXGI_FORMAT_BC6H_SF16:
			return FORMAT_BC6H_SF16;
			break;
		case DXGI_FORMAT_BC7_TYPELESS:
			return FORMAT_BC7_TYPELESS;
			break;
		case DXGI_FORMAT_BC7_UNORM:
			return FORMAT_BC7_UNORM;
			break;
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return FORMAT_BC7_UNORM_SRGB;
			break;
		case DXGI_FORMAT_AYUV:
			return FORMAT_AYUV;
			break;
		case DXGI_FORMAT_Y410:
			return FORMAT_Y410;
			break;
		case DXGI_FORMAT_Y416:
			return FORMAT_Y416;
			break;
		case DXGI_FORMAT_NV12:
			return FORMAT_NV12;
			break;
		case DXGI_FORMAT_P010:
			return FORMAT_P010;
			break;
		case DXGI_FORMAT_P016:
			return FORMAT_P016;
			break;
		case DXGI_FORMAT_420_OPAQUE:
			return FORMAT_420_OPAQUE;
			break;
		case DXGI_FORMAT_YUY2:
			return FORMAT_YUY2;
			break;
		case DXGI_FORMAT_Y210:
			return FORMAT_Y210;
			break;
		case DXGI_FORMAT_Y216:
			return FORMAT_Y216;
			break;
		case DXGI_FORMAT_NV11:
			return FORMAT_NV11;
			break;
		case DXGI_FORMAT_AI44:
			return FORMAT_AI44;
			break;
		case DXGI_FORMAT_IA44:
			return FORMAT_IA44;
			break;
		case DXGI_FORMAT_P8:
			return FORMAT_P8;
			break;
		case DXGI_FORMAT_A8P8:
			return FORMAT_A8P8;
			break;
		case DXGI_FORMAT_B4G4R4A4_UNORM:
			return FORMAT_B4G4R4A4_UNORM;
			break;
		case DXGI_FORMAT_FORCE_UINT:
			return FORMAT_FORCE_UINT;
			break;
		default:
			break;
		}
		return FORMAT_UNKNOWN;
	}
	


	// Local Helpers:
	const void* const __nullBlob[1024] = { 0 }; // this is initialized to nullptrs!



	// Engine functions

	GraphicsDevice_DX12::GraphicsDevice_DX12(wiWindowRegistration::window_type window, bool fullscreen) : GraphicsDevice()
	{
		FULLSCREEN = fullscreen;

#ifndef WINSTORE_SUPPORT
		RECT rect = RECT();
		GetClientRect(window, &rect);
		SCREENWIDTH = rect.right - rect.left;
		SCREENHEIGHT = rect.bottom - rect.top;
#else WINSTORE_SUPPORT
		SCREENWIDTH = (int)window->Bounds.Width;
		SCREENHEIGHT = (int)window->Bounds.Height;
#endif

		HRESULT hr = E_FAIL;

		for (int i = 0; i < GRAPHICSTHREAD_COUNT; i++)
		{
			SAFE_INIT(commandLists[i]);
		}

		UINT createDeviceFlags = 0;
		//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

		D3D_DRIVER_TYPE driverTypes[] =
		{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE,
		};
		UINT numDriverTypes = ARRAYSIZE(driverTypes);

		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
		};
		UINT numFeatureLevels = ARRAYSIZE(featureLevels);

		for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
		{
			hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device);

			if (SUCCEEDED(hr))
				break;
		}
		if (FAILED(hr))
		{
			stringstream ss("");
			ss << "Failed to create the graphics device! ERROR: " << std::hex << hr;
			wiHelper::messageBox(ss.str(), "Error!");
			exit(1);
		}


		// Create command queue
		D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
		commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		commandQueueDesc.NodeMask = 0;
		hr = device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&commandQueue);



		// Create swapchain

		IDXGIFactory4 * pIDXGIFactory;
		hr = CreateDXGIFactory1(IID_PPV_ARGS(&pIDXGIFactory));

		IDXGISwapChain1* _swapChain;

		DXGI_SWAP_CHAIN_DESC1 sd = { 0 };
		sd.Width = SCREENWIDTH;
		sd.Height = SCREENHEIGHT;
		sd.Format = _ConvertFormat(GetBackBufferFormat());
		sd.Stereo = false;
		sd.SampleDesc.Count = 1; // Don't use multi-sampling.
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 2; // Use double-buffering to minimize latency.
		sd.Flags = 0;
		sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

#ifndef WINSTORE_SUPPORT
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		sd.Scaling = DXGI_SCALING_STRETCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
		fullscreenDesc.RefreshRate.Numerator = 60;
		fullscreenDesc.RefreshRate.Denominator = 1;
		fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // needs to be unspecified for correct fullscreen scaling!
		fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
		fullscreenDesc.Windowed = !fullscreen;
		hr = pIDXGIFactory->CreateSwapChainForHwnd(commandQueue, window, &sd, &fullscreenDesc, nullptr, &_swapChain);
#else
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
		sd.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

		hr = pIDXGIFactory->CreateSwapChainForCoreWindow(commandQueue, reinterpret_cast<IUnknown*>(window), &sd, nullptr, &_swapChain);
#endif

		if (FAILED(hr))
		{
			wiHelper::messageBox("Failed to create a swapchain for the graphics device!", "Error!");
			exit(1);
		}

		hr = _swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapChain);

		SAFE_RELEASE(pIDXGIFactory);



		// Create render target for backbuffer

		D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc = {};
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = {};

		renderTargetViewHeapDesc.NumDescriptors = 2;
		renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		hr = device->CreateDescriptorHeap(&renderTargetViewHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&renderTargetHeap);
		renderTargetViewHandle = renderTargetHeap->GetCPUDescriptorHandleForHeapStart();

		UINT renderTargetViewDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		hr = swapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&backBuffer[0]);
		device->CreateRenderTargetView(backBuffer[0], nullptr, renderTargetViewHandle);

		renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
		hr = swapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&backBuffer[1]);
		device->CreateRenderTargetView(backBuffer[1], nullptr, renderTargetViewHandle);

		backBufferIndex = swapChain->GetCurrentBackBufferIndex();


		//Create command lists

		for (int i = 0; i < GRAPHICSTHREAD_COUNT; ++i)
		{
			hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&commandAllocators[i]);
			hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[i], nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&commandLists[i]);
			hr = static_cast<ID3D12GraphicsCommandList*>(commandLists[i])->Close();

			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&commandFences[i]);
			commandFenceEvents[i] = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
			commandFenceValues[i] = 1;
		}



		// Query features:

		MULTITHREADED_RENDERING = true;
		TESSELLATION = true;

		D3D12_FEATURE_DATA_D3D12_OPTIONS features_0;
		hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &features_0, sizeof(features_0));
		CONSERVATIVE_RASTERIZATION = features_0.ConservativeRasterizationTier >= D3D12_CONSERVATIVE_RASTERIZATION_TIER_1;
		RASTERIZER_ORDERED_VIEWS = features_0.ROVsSupported == TRUE;
		UNORDEREDACCESSTEXTURE_LOAD_EXT = features_0.TypedUAVLoadAdditionalFormats == TRUE;

		// Setup the main viewport
		viewPort.Width = (FLOAT)SCREENWIDTH;
		viewPort.Height = (FLOAT)SCREENHEIGHT;
		viewPort.MinDepth = 0.0f;
		viewPort.MaxDepth = 1.0f;
		viewPort.TopLeftX = 0;
		viewPort.TopLeftY = 0;

	}
	GraphicsDevice_DX12::~GraphicsDevice_DX12()
	{
		SAFE_RELEASE(swapChain);

		for (int i = 0; i<GRAPHICSTHREAD_COUNT; i++) 
		{
			SAFE_RELEASE(commandLists[i]);
			SAFE_RELEASE(commandAllocators[i]);
			SAFE_RELEASE(commandFences[i]);
			CloseHandle(commandFenceEvents[i]);
		}

		for (int i = 0; i < ARRAYSIZE(backBuffer); ++i)
		{
			SAFE_RELEASE(backBuffer[i]);
		}
		SAFE_RELEASE(renderTargetHeap);

		SAFE_RELEASE(commandQueue);
		SAFE_RELEASE(device);
	}

	void GraphicsDevice_DX12::SetResolution(int width, int height)
	{
		if (width != SCREENWIDTH || height != SCREENHEIGHT)
		{
			SCREENWIDTH = width;
			SCREENHEIGHT = height;
			swapChain->ResizeBuffers(2, width, height, _ConvertFormat(GetBackBufferFormat()), 0);
			RESOLUTIONCHANGED = true;
		}
	}

	Texture2D GraphicsDevice_DX12::GetBackBuffer()
	{
		uint64_t resourceIdx = FRAMECOUNT % ARRAYSIZE(backBuffer);

		Texture2D result;
		result.texture2D_DX12 = backBuffer[resourceIdx];
		backBuffer[resourceIdx]->AddRef();
		return result;
	}

	HRESULT GraphicsDevice_DX12::CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateTexture1D(const Texture1DDesc* pDesc, const SubresourceData *pInitialData, Texture1D **ppTexture1D)
	{
		if ((*ppTexture1D) == nullptr)
		{
			(*ppTexture1D) = new Texture1D;
		}
		(*ppTexture1D)->desc = *pDesc;


		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D)
	{
		if ((*ppTexture2D) == nullptr)
		{
			(*ppTexture2D) = new Texture2D;
		}
		(*ppTexture2D)->desc = *pDesc;


		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateTexture3D(const Texture3DDesc* pDesc, const SubresourceData *pInitialData, Texture3D **ppTexture3D)
	{
		if ((*ppTexture3D) == nullptr)
		{
			(*ppTexture3D) = new Texture3D;
		}
		(*ppTexture3D)->desc = *pDesc;


		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateShaderResourceView(Texture1D* pTexture)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateShaderResourceView(Texture2D* pTexture)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateShaderResourceView(Texture3D* pTexture)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateRenderTargetView(Texture2D* pTexture)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateRenderTargetView(Texture3D* pTexture)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateDepthStencilView(Texture2D* pTexture)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
		const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout)
	{
		D3D12_INPUT_ELEMENT_DESC* desc = new D3D12_INPUT_ELEMENT_DESC[NumElements];
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


		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, VertexShader *pVertexShader)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, PixelShader *pPixelShader)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, GeometryShader *pGeometryShader)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, HullShader *pHullShader)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, DomainShader *pDomainShader)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ComputeShader *pComputeShader)
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState)
	{
		D3D12_BLEND_DESC desc;
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

		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState)
	{
		D3D12_DEPTH_STENCIL_DESC desc;
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

		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState)
	{
		pRasterizerState->desc = *pRasterizerStateDesc;

		D3D12_RASTERIZER_DESC desc;
		desc.FillMode = _ConvertFillMode(pRasterizerStateDesc->FillMode);
		desc.CullMode = _ConvertCullMode(pRasterizerStateDesc->CullMode);
		desc.FrontCounterClockwise = pRasterizerStateDesc->FrontCounterClockwise;
		desc.DepthBias = pRasterizerStateDesc->DepthBias;
		desc.DepthBiasClamp = pRasterizerStateDesc->DepthBiasClamp;
		desc.SlopeScaledDepthBias = pRasterizerStateDesc->SlopeScaledDepthBias;
		desc.DepthClipEnable = pRasterizerStateDesc->DepthClipEnable;
		//desc.ScissorEnable = pRasterizerStateDesc->ScissorEnable;
		desc.MultisampleEnable = pRasterizerStateDesc->MultisampleEnable;
		desc.AntialiasedLineEnable = pRasterizerStateDesc->AntialiasedLineEnable;
		desc.ConservativeRaster = ((CONSERVATIVE_RASTERIZATION && pRasterizerStateDesc->ConservativeRasterizationEnable) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
		desc.ForcedSampleCount = pRasterizerStateDesc->ForcedSampleCount;

		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState)
	{
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

		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery)
	{
		HRESULT hr = E_FAIL;

		pQuery->desc = *pDesc;
		pQuery->async_frameshift = pQuery->desc.async_latency;

		return hr;
	}


	void GraphicsDevice_DX12::PresentBegin()
	{
		LOCK();

		BindViewports(1, &viewPort, GRAPHICSTHREAD_IMMEDIATE);

		HRESULT result;
		D3D12_RESOURCE_BARRIER barrier;
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;
		unsigned int renderTargetViewDescriptorSize;
		float color[4];


		// Reset (re-use) the memory associated command allocator.
		result = commandAllocators[0]->Reset();

		// Reset the command list, use empty pipeline state for now since there are no shaders and we are just clearing the screen.
		result = static_cast<ID3D12GraphicsCommandList*>(commandLists[0])->Reset(commandAllocators[0], nullptr);


		// Record commands in the command list now.
		// Start by setting the resource barrier.
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = backBuffer[backBufferIndex];
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		static_cast<ID3D12GraphicsCommandList*>(commandLists[0])->ResourceBarrier(1, &barrier);



		// Get the render target view handle for the current back buffer.
		renderTargetViewHandle = renderTargetHeap->GetCPUDescriptorHandleForHeapStart();
		renderTargetViewDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		if (backBufferIndex == 1)
		{
			renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
		}

		// Set the back buffer as the render target.
		static_cast<ID3D12GraphicsCommandList*>(commandLists[0])->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, NULL);


		// Then set the color to clear the window to.
		color[0] = 0.5;
		color[1] = 0.5;
		color[2] = 0.5;
		color[3] = 1.0;
		static_cast<ID3D12GraphicsCommandList*>(commandLists[0])->ClearRenderTargetView(renderTargetViewHandle, color, 0, NULL);

		// Indicate that the back buffer will now be used to present.
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		static_cast<ID3D12GraphicsCommandList*>(commandLists[0])->ResourceBarrier(1, &barrier);



		// Close the list of commands.
		result = static_cast<ID3D12GraphicsCommandList*>(commandLists[0])->Close();

		// Execute the list of commands.
		commandQueue->ExecuteCommandLists(1, commandLists);


	}
	void GraphicsDevice_DX12::PresentEnd()
	{
		swapChain->Present(VSYNC, 0);


		// Signal and increment the fence value.
		UINT64 fenceToWaitFor = commandFenceValues[0];
		HRESULT result = commandQueue->Signal(commandFences[0], fenceToWaitFor);
		commandFenceValues[0]++;

		// Wait until the GPU is done rendering.
		if (commandFences[0]->GetCompletedValue() < fenceToWaitFor)
		{
			result = commandFences[0]->SetEventOnCompletion(fenceToWaitFor, commandFenceEvents[0]);
			WaitForSingleObject(commandFenceEvents[0], INFINITE);
		}

		// Alternate the back buffer index back and forth between 0 and 1 each frame.
		backBufferIndex == 0 ? backBufferIndex = 1 : backBufferIndex = 0;



		FRAMECOUNT++;

		RESOLUTIONCHANGED = false;

		UNLOCK();
	}

	void GraphicsDevice_DX12::ExecuteDeferredContexts()
	{
		commandQueue->ExecuteCommandLists(GRAPHICSTHREAD_COUNT, commandLists);
	}
	void GraphicsDevice_DX12::FinishCommandList(GRAPHICSTHREAD threadID)
	{
		if (threadID == GRAPHICSTHREAD_IMMEDIATE)
			return;
		static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->Close();
	}

	void GraphicsDevice_DX12::BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID)
	{
		assert(NumViewports <= 6);
		D3D12_VIEWPORT d3dViewPorts[6];
		for (UINT i = 0; i < NumViewports; ++i)
		{
			d3dViewPorts[i].TopLeftX = pViewports[i].TopLeftX;
			d3dViewPorts[i].TopLeftY = pViewports[i].TopLeftY;
			d3dViewPorts[i].Width = pViewports[i].Width;
			d3dViewPorts[i].Height = pViewports[i].Height;
			d3dViewPorts[i].MinDepth = pViewports[i].MinDepth;
			d3dViewPorts[i].MaxDepth = pViewports[i].MaxDepth;
		}
		static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->RSSetViewports(NumViewports, d3dViewPorts);
	}
	void GraphicsDevice_DX12::BindRenderTargetsUAVs(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GPUUnorderedResource* const *ppUAVs, int slotUAV, int countUAV,
		GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_DX12::BindRenderTargets(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_DX12::ClearRenderTarget(Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_DX12::ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		UINT _flags = 0;
		if (ClearFlags & CLEAR_DEPTH)
			_flags |= D3D12_CLEAR_FLAG_DEPTH;
		if (ClearFlags & CLEAR_STENCIL)
			_flags |= D3D12_CLEAR_FLAG_STENCIL;

	}
	void GraphicsDevice_DX12::BindResourcePS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_DX12::BindResourceVS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_DX12::BindResourceGS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_DX12::BindResourceDS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_DX12::BindResourceHS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_DX12::BindResourceCS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_DX12::BindResourcesPS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindResourcesVS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindResourcesGS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindResourcesDS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindResourcesHS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindResourcesCS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindUnorderedAccessResourceCS(const GPUUnorderedResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
	}
	void GraphicsDevice_DX12::BindUnorderedAccessResourcesCS(const GPUUnorderedResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::UnBindResources(int slot, int num, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::UnBindUnorderedAccessResources(int slot, int num, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindSamplerPS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindSamplerVS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindSamplerGS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindSamplerHS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindSamplerDS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindSamplerCS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindConstantBufferPS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindConstantBufferVS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindConstantBufferGS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindConstantBufferDS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindConstantBufferHS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindConstantBufferCS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindVertexBuffers(const GPUBuffer* const *vertexBuffers, int slot, int count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID)
	{
		D3D_PRIMITIVE_TOPOLOGY d3dType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		switch (type)
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
		case PATCHLIST:
			d3dType = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
			break;
		default:
			break;
		};
	}
	void GraphicsDevice_DX12::BindVertexLayout(const VertexLayout* layout, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindBlendState(const BlendState* state, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindBlendStateEx(const BlendState* state, const XMFLOAT4& blendFactor, UINT sampleMask, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindDepthStencilState(const DepthStencilState* state, UINT stencilRef, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindRasterizerState(const RasterizerState* state, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindPS(const PixelShader* shader, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindVS(const VertexShader* shader, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindGS(const GeometryShader* shader, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindHS(const HullShader* shader, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindDS(const DomainShader* shader, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::BindCS(const ComputeShader* shader, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::Draw(int vertexCount, UINT startVertexLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::DrawIndexed(int indexCount, UINT startIndexLocation, UINT baseVertexLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::DrawInstanced(int vertexCount, int instanceCount, UINT startVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::DrawIndexedInstanced(int indexCount, int instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::DrawInstancedIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::DrawIndexedInstancedIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::DispatchIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::GenerateMips(Texture* texture, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::CopyTexture2D(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::CopyTexture2D_Region(Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, const Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::MSAAResolve(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize)
	{
	}
	void* GraphicsDevice_DX12::AllocateFromRingBuffer(GPURingBuffer* buffer, size_t dataSize, UINT& offsetIntoBuffer, GRAPHICSTHREAD threadID)
	{
		offsetIntoBuffer = 0;
		return nullptr;
	}
	void GraphicsDevice_DX12::InvalidateBufferAccess(GPUBuffer* buffer, GRAPHICSTHREAD threadID)
	{
	}
	bool GraphicsDevice_DX12::DownloadBuffer(GPUBuffer* bufferToDownload, GPUBuffer* bufferDest, void* dataDest, GRAPHICSTHREAD threadID)
	{
		return false;
	}
	void GraphicsDevice_DX12::SetScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID)
	{
		if (rects != nullptr)
		{
			D3D12_RECT* pRects = new D3D12_RECT[numRects];
			for (UINT i = 0; i < numRects; ++i)
			{
				pRects[i].bottom = rects[i].bottom;
				pRects[i].left = rects[i].left;
				pRects[i].right = rects[i].right;
				pRects[i].top = rects[i].top;
			}
			static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->RSSetScissorRects(numRects, pRects);
			SAFE_DELETE_ARRAY(pRects);
		}
		else
		{
			static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->RSSetScissorRects(numRects, nullptr);
		}
	}

	void GraphicsDevice_DX12::QueryBegin(GPUQuery *query, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::QueryEnd(GPUQuery *query, GRAPHICSTHREAD threadID)
	{
	}
	bool GraphicsDevice_DX12::QueryRead(GPUQuery *query, GRAPHICSTHREAD threadID)
	{
		return true;
	}


	HRESULT GraphicsDevice_DX12::CreateTextureFromFile(const std::string& fileName, Texture2D **ppTexture, bool mipMaps, GRAPHICSTHREAD threadID)
	{
		(*ppTexture) = new Texture2D();

		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::SaveTexturePNG(const std::string& fileName, Texture2D *pTexture, GRAPHICSTHREAD threadID)
	{
		return E_FAIL;
	}
	HRESULT GraphicsDevice_DX12::SaveTextureDDS(const std::string& fileName, Texture *pTexture, GRAPHICSTHREAD threadID)
	{
		return E_FAIL;
	}

	void GraphicsDevice_DX12::EventBegin(const std::string& name, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::EventEnd(GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::SetMarker(const std::string& name, GRAPHICSTHREAD threadID)
	{
	}

}
