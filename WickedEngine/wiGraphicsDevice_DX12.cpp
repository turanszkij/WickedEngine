#include "wiGraphicsDevice_DX12.h"
#include "Include_DX12.h"
#include "wiHelper.h"
#include "ResourceMapping.h"

#include "Utility/WicTextureLoader.h"
#include "Utility/DDSTextureLoader.h"
#include "Utility/ScreenGrab.h"

#include <sstream>
#include <wincodec.h>

#include <unordered_map>

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
	inline D3D12_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
	{
		D3D12_SUBRESOURCE_DATA data;
		data.pData = pInitialData.pSysMem;
		data.RowPitch = pInitialData.SysMemPitch;
		data.SlicePitch = pInitialData.SysMemSlicePitch;

		return data;
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

	size_t Align(size_t uLocation, size_t uAlign)
	{
		if ((0 == uAlign) || (uAlign & (uAlign - 1)))
		{
			assert(0);
		}

		return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
	}

	// Allocator heaps:

	GraphicsDevice_DX12::DescriptorAllocator::DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible, UINT maxCount)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = maxCount;
		heapDesc.Type = type;
		heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRESULT hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&heap);
		assert(SUCCEEDED(hr));

		itemSize = device->GetDescriptorHandleIncrementSize(type);
		itemCount.store(0);
		this->maxCount = maxCount;
	}
	GraphicsDevice_DX12::DescriptorAllocator::~DescriptorAllocator()
	{
		SAFE_RELEASE(heap);
	}
	size_t GraphicsDevice_DX12::DescriptorAllocator::allocate()
	{
		return heap->GetCPUDescriptorHandleForHeapStart().ptr + itemCount.fetch_add(1) * itemSize;
	}



	GraphicsDevice_DX12::UploadBuffer::UploadBuffer(ID3D12Device* device, size_t size)
	{
		HRESULT hr = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			__uuidof(ID3D12Resource), (void**)&resource);

		assert(SUCCEEDED(hr));

		void* pData;
		//
		// No CPU reads will be done from the resource.
		//
		CD3DX12_RANGE readRange(0, 0);
		resource->Map(0, &readRange, &pData);
		dataCur = dataBegin = reinterpret_cast< UINT8* >(pData);
		dataEnd = dataBegin + size;
	}
	GraphicsDevice_DX12::UploadBuffer::~UploadBuffer()
	{
		SAFE_RELEASE(resource);
	}
	uint8_t* GraphicsDevice_DX12::UploadBuffer::allocate(size_t dataSize, size_t alignment)
	{
		LOCK();

		dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));
		assert(dataCur + dataSize <= dataEnd);

		uint8_t* retVal = dataCur;

		dataCur += dataSize;

		UNLOCK();

		return retVal;
	}
	void GraphicsDevice_DX12::UploadBuffer::clear()
	{
		dataCur = dataBegin;
	}
	uint64_t GraphicsDevice_DX12::UploadBuffer::calculateOffset(uint8_t* address)
	{
		assert(address >= dataBegin && address < dataEnd);
		return static_cast<uint64_t>(address - dataBegin);
	}


#define GPU_RESOURCE_HEAP_CBV_COUNT		15
#define GPU_RESOURCE_HEAP_SRV_COUNT		64
#define GPU_RESOURCE_HEAP_UAV_COUNT		8
#define GPU_RESOURCE_HEAP_COUNT			(GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + GPU_RESOURCE_HEAP_UAV_COUNT)
#define GPU_SAMPLER_HEAP_COUNT			16

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

#if defined(_DEBUG) && !defined(WINSTORE_SUPPORT)
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
#endif

		hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device);
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



		// Create common descriptor heaps
		RTAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false, 128);
		DSAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false, 128);
		ResourceAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false, 4096);
		SamplerAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, false, 64);
		for (int i = 0; i < GRAPHICSTHREAD_COUNT; ++i)
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NodeMask = 0;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			heapDesc.NumDescriptors = GPU_RESOURCE_HEAP_COUNT * SHADERSTAGE_COUNT;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			HRESULT hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&ResourceDescriptorHeapGPU[i]);
			assert(SUCCEEDED(hr));

			heapDesc.NumDescriptors = GPU_SAMPLER_HEAP_COUNT * SHADERSTAGE_COUNT;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&SamplerDescriptorHeapGPU[i]);
			assert(SUCCEEDED(hr));
		}

		// Create null resources for unbinding:
		{

			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R32_UINT;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			nullSRV = new D3D12_CPU_DESCRIPTOR_HANDLE;
			nullSRV->ptr = ResourceAllocator->allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, *nullSRV);


			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			nullUAV = new D3D12_CPU_DESCRIPTOR_HANDLE;
			nullUAV->ptr = ResourceAllocator->allocate();
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, *nullUAV);
		}

		// Create resource upload buffer
		uploadBuffer = new UploadBuffer(device, 64 * 1024 * 1024);


		// Create render target for backbuffer
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = {};
		renderTargetViewHandle.ptr = RTAllocator->allocate();

		hr = swapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&backBuffer[0]);
		device->CreateRenderTargetView(backBuffer[0], nullptr, renderTargetViewHandle);

		renderTargetViewHandle.ptr = RTAllocator->allocate();
		hr = swapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&backBuffer[1]);
		device->CreateRenderTargetView(backBuffer[1], nullptr, renderTargetViewHandle);

		backBufferIndex = swapChain->GetCurrentBackBufferIndex();


		//Create command lists
		for (int i = 0; i < GRAPHICSTHREAD_COUNT; ++i)
		{
			hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&commandAllocators[i]);
			hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[i], nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&commandLists[i]);
			hr = static_cast<ID3D12GraphicsCommandList*>(commandLists[i])->Close();
			hr = commandAllocators[i]->Reset();
			hr = static_cast<ID3D12GraphicsCommandList*>(commandLists[i])->Reset(commandAllocators[i], nullptr);

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


		// Generate default root signature:
		SAFE_INIT(graphicsRootSig);
		SAFE_INIT(computeRootSig);

		D3D12_DESCRIPTOR_RANGE samplerRange = {};
		samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		samplerRange.BaseShaderRegister = 0;
		samplerRange.RegisterSpace = 0;
		samplerRange.NumDescriptors = GPU_SAMPLER_HEAP_COUNT;
		samplerRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		{
			UINT descriptorRangeCount = 3;
			D3D12_DESCRIPTOR_RANGE* descriptorRanges = new D3D12_DESCRIPTOR_RANGE[descriptorRangeCount];

			descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			descriptorRanges[0].BaseShaderRegister = 0;
			descriptorRanges[0].RegisterSpace = 0;
			descriptorRanges[0].NumDescriptors = GPU_RESOURCE_HEAP_CBV_COUNT;
			descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRanges[1].BaseShaderRegister = 0;
			descriptorRanges[1].RegisterSpace = 0;
			descriptorRanges[1].NumDescriptors = GPU_RESOURCE_HEAP_SRV_COUNT;
			descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			descriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRanges[2].BaseShaderRegister = 0;
			descriptorRanges[2].RegisterSpace = 0;
			descriptorRanges[2].NumDescriptors = GPU_RESOURCE_HEAP_UAV_COUNT;
			descriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



			UINT paramCount = 2 * (SHADERSTAGE_COUNT - 1); // 2: resource,sampler;   5: vs,hs,ds,gs,ps
			D3D12_ROOT_PARAMETER* params = new D3D12_ROOT_PARAMETER[paramCount];
			params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
			params[0].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[0].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
			params[1].DescriptorTable.NumDescriptorRanges = 1;
			params[1].DescriptorTable.pDescriptorRanges = &samplerRange;

			params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
			params[2].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[2].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
			params[3].DescriptorTable.NumDescriptorRanges = 1;
			params[3].DescriptorTable.pDescriptorRanges = &samplerRange;

			params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
			params[4].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[4].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
			params[5].DescriptorTable.NumDescriptorRanges = 1;
			params[5].DescriptorTable.pDescriptorRanges = &samplerRange;

			params[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
			params[6].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[6].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
			params[7].DescriptorTable.NumDescriptorRanges = 1;
			params[7].DescriptorTable.pDescriptorRanges = &samplerRange;

			params[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			params[8].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[8].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			params[9].DescriptorTable.NumDescriptorRanges = 1;
			params[9].DescriptorTable.pDescriptorRanges = &samplerRange;



			D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
			rootSigDesc.NumStaticSamplers = 0;
			rootSigDesc.NumParameters = paramCount;
			rootSigDesc.pParameters = params;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			ID3DBlob* rootSigBlob;
			ID3DBlob* rootSigError;
			hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &rootSigError);
			if (FAILED(hr))
			{
				assert(0);
				OutputDebugStringA((char*)rootSigError->GetBufferPointer());
			}
			hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&graphicsRootSig);
			assert(SUCCEEDED(hr));

			SAFE_DELETE_ARRAY(descriptorRanges);
			SAFE_DELETE_ARRAY(params);
		}
		{
			UINT descriptorRangeCount = 3;
			D3D12_DESCRIPTOR_RANGE* descriptorRanges = new D3D12_DESCRIPTOR_RANGE[descriptorRangeCount];

			descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			descriptorRanges[0].BaseShaderRegister = 0;
			descriptorRanges[0].RegisterSpace = 0;
			descriptorRanges[0].NumDescriptors = GPU_RESOURCE_HEAP_CBV_COUNT;
			descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRanges[1].BaseShaderRegister = 0;
			descriptorRanges[1].RegisterSpace = 0;
			descriptorRanges[1].NumDescriptors = GPU_RESOURCE_HEAP_SRV_COUNT;
			descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			descriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRanges[2].BaseShaderRegister = 0;
			descriptorRanges[2].RegisterSpace = 0;
			descriptorRanges[2].NumDescriptors = GPU_RESOURCE_HEAP_UAV_COUNT;
			descriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



			UINT paramCount = 2;
			D3D12_ROOT_PARAMETER* params = new D3D12_ROOT_PARAMETER[paramCount];
			params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			params[0].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[0].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			params[1].DescriptorTable.NumDescriptorRanges = 1;
			params[1].DescriptorTable.pDescriptorRanges = &samplerRange;



			D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
			rootSigDesc.NumStaticSamplers = 0;
			rootSigDesc.NumParameters = paramCount;
			rootSigDesc.pParameters = params;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			ID3DBlob* rootSigBlob;
			ID3DBlob* rootSigError;
			hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &rootSigError);
			if (FAILED(hr))
			{
				assert(0);
				OutputDebugStringA((char*)rootSigError->GetBufferPointer());
			}
			hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&computeRootSig);
			assert(SUCCEEDED(hr));

			SAFE_DELETE_ARRAY(descriptorRanges);
			SAFE_DELETE_ARRAY(params);
		}


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

			SAFE_RELEASE(ResourceDescriptorHeapGPU[i]);
			SAFE_RELEASE(SamplerDescriptorHeapGPU[i]);
		}

		SAFE_DELETE(nullSRV);
		SAFE_DELETE(nullUAV);

		for (int i = 0; i < ARRAYSIZE(backBuffer); ++i)
		{
			SAFE_RELEASE(backBuffer[i]);
		}

		SAFE_RELEASE(graphicsRootSig);
		SAFE_RELEASE(computeRootSig);

		SAFE_DELETE(RTAllocator);
		SAFE_DELETE(DSAllocator);
		SAFE_DELETE(ResourceAllocator);
		SAFE_DELETE(SamplerAllocator);

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

		ppBuffer->desc = *pDesc;

		uint32_t alignment = 1;
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		}
		UINT64 alignedSize = Align(pDesc->ByteWidth, alignment);

		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.CreationNodeMask = 0;
		heapDesc.VisibleNodeMask = 0;

		D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;

		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Width = alignedSize;
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
		//desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER || pDesc->BindFlags & BIND_VERTEX_BUFFER)
		{
			resourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		}
		else if (pDesc->BindFlags & BIND_INDEX_BUFFER)
		{
			resourceState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		}
		if (pDesc->BindFlags & BIND_SHADER_RESOURCE)
		{
			resourceState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			resourceState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}
	
		// Issue main resource creation:
		hr = device->CreateCommittedResource(&heapDesc, heapFlags, &desc, pInitialData == nullptr ? resourceState : D3D12_RESOURCE_STATE_COPY_DEST, 
			nullptr, __uuidof(ID3D12Resource), (void**)&ppBuffer->resource_DX12);
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		

		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			uint8_t* dest = uploadBuffer->allocate(pDesc->ByteWidth, alignment);
			memcpy(dest, pInitialData->pSysMem, pDesc->ByteWidth);
			static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->CopyBufferRegion(
				ppBuffer->resource_DX12, 0, uploadBuffer->resource, uploadBuffer->calculateOffset(dest), pDesc->ByteWidth);

			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = ppBuffer->resource_DX12;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = resourceState;
			barrier.Transition.Subresource = 0;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->ResourceBarrier(1, &barrier);
		}


		// Create resource views if needed
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
			cbv_desc.SizeInBytes = (UINT)alignedSize;
			cbv_desc.BufferLocation = ppBuffer->resource_DX12->GetGPUVirtualAddress();

			ppBuffer->CBV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
			ppBuffer->CBV_DX12->ptr = ResourceAllocator->allocate();
			device->CreateConstantBufferView(&cbv_desc, *ppBuffer->CBV_DX12);
		}

		if (pDesc->BindFlags & BIND_SHADER_RESOURCE)
		{

			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer

				srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = 0;
				srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
				srv_desc.Buffer.NumElements = pDesc->ByteWidth / 4;
			}
			else if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer

				srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = 0;
				srv_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
			}
			else
			{
				// This is a Typed Buffer

				srv_desc.Format = _ConvertFormat(pDesc->Format);
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = 0;
				srv_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
			}

			ppBuffer->SRV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
			ppBuffer->SRV_DX12->ptr = ResourceAllocator->allocate();
			device->CreateShaderResourceView(ppBuffer->resource_DX12, &srv_desc, *ppBuffer->SRV_DX12);
		}

		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;

			if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer

				uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
				uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
				uav_desc.Buffer.NumElements = pDesc->ByteWidth / 4;
			}
			else if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer

				uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
				uav_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
			}
			else
			{
				// This is a Typed Buffer

				uav_desc.Format = _ConvertFormat(pDesc->Format);
				uav_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
			}

			ppBuffer->UAV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
			ppBuffer->UAV_DX12->ptr = ResourceAllocator->allocate();
			device->CreateUnorderedAccessView(ppBuffer->resource_DX12, nullptr, &uav_desc, *ppBuffer->UAV_DX12);
		}

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

		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.CreationNodeMask = 0;
		heapDesc.VisibleNodeMask = 0;

		D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;

		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Format = _ConvertFormat(pDesc->Format);
		desc.Width = pDesc->Width;
		desc.Height = pDesc->Height;
		desc.MipLevels = pDesc->MipLevels;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.DepthOrArraySize = (UINT16)pDesc->ArraySize;
		desc.Alignment = 0;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		//desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		if (pDesc->BindFlags & BIND_RENDER_TARGET)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (pDesc->BindFlags & BIND_DEPTH_STENCIL)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!(pDesc->BindFlags & BIND_SHADER_RESOURCE))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
		desc.SampleDesc.Count = pDesc->SampleDesc.Count;
		desc.SampleDesc.Quality = pDesc->SampleDesc.Quality;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
		if (pDesc->BindFlags & BIND_SHADER_RESOURCE)
		{
			resourceState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			resourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}

		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Color[0] = 0;
		optimizedClearValue.Color[1] = 0;
		optimizedClearValue.Color[2] = 0;
		optimizedClearValue.Color[3] = 0;
		optimizedClearValue.DepthStencil.Depth = 0.0f;
		optimizedClearValue.DepthStencil.Stencil = 0;
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

		// Issue main resource creation:
		hr = device->CreateCommittedResource(&heapDesc, heapFlags, &desc, pInitialData == nullptr ? resourceState : D3D12_RESOURCE_STATE_COPY_DEST, 
			useClearValue ? &optimizedClearValue : nullptr, 
			__uuidof(ID3D12Resource), (void**)&(*ppTexture2D)->texture2D_DX12);
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		if ((*ppTexture2D)->desc.MipLevels == 0)
		{
			(*ppTexture2D)->desc.MipLevels = (UINT)log2(max((*ppTexture2D)->desc.Width, (*ppTexture2D)->desc.Height));
		}


		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			D3D12_SUBRESOURCE_DATA* data = new D3D12_SUBRESOURCE_DATA[pDesc->ArraySize];
			for (UINT slice = 0; slice < pDesc->ArraySize; ++slice)
			{
				data[slice] = _ConvertSubresourceData(pInitialData[slice]);
			}

			UINT NumSubresources = pDesc->ArraySize;
			UINT FirstSubresource = 0;


			// Determine size of resource:
			UINT64 RequiredSize = 0;
			UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
			if (MemToAlloc > SIZE_MAX)
			{
				return 0;
			}
			void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
			if (pMem == NULL)
			{
				return 0;
			}
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
			UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
			UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);

			device->GetCopyableFootprints(&desc, FirstSubresource, NumSubresources, 0, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);


			// Issue CPU copy to staging resource:
			uint8_t* dest = uploadBuffer->allocate(RequiredSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
			for (UINT i = 0; i < NumSubresources; ++i)
			{
				if (pRowSizesInBytes[i] >(SIZE_T)-1) return 0;
				D3D12_MEMCPY_DEST DestData = { dest + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
				MemcpySubresource(&DestData, &data[i], (SIZE_T)pRowSizesInBytes[i], pNumRows[i], pLayouts[i].Footprint.Depth);
			}



			// Issue GPU copy:
			for (UINT i = 0; i < NumSubresources; ++i)
			{
				CD3DX12_TEXTURE_COPY_LOCATION Dst((*ppTexture2D)->texture2D_DX12, i + FirstSubresource);
				CD3DX12_TEXTURE_COPY_LOCATION Src(uploadBuffer->resource, pLayouts[i]);
				static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

				D3D12_RESOURCE_BARRIER barrier = {};
				barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrier.Transition.pResource = (*ppTexture2D)->texture2D_DX12;
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				barrier.Transition.StateAfter = resourceState;
				barrier.Transition.Subresource = i;
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->ResourceBarrier(1, &barrier);
			}





			HeapFree(GetProcessHeap(), 0, pMem);

			SAFE_DELETE_ARRAY(data);
		}


		// Issue creation of additional descriptors for the resource:

		if ((*ppTexture2D)->desc.BindFlags & BIND_RENDER_TARGET)
		{
			UINT arraySize = (*ppTexture2D)->desc.ArraySize;
			UINT sampleCount = (*ppTexture2D)->desc.SampleDesc.Count;
			bool multisampled = sampleCount > 1;

			D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
			renderTargetViewDesc.Format = _ConvertFormat((*ppTexture2D)->desc.Format);
			renderTargetViewDesc.Texture2DArray.MipSlice = 0;

			if ((*ppTexture2D)->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
			{
				// TextureCube, TextureCubeArray...
				UINT slices = arraySize / 6;

				renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				renderTargetViewDesc.Texture2DArray.MipSlice = 0;

				if ((*ppTexture2D)->independentRTVCubemapFaces)
				{
					// independent faces
					for (UINT i = 0; i < arraySize; ++i)
					{
						renderTargetViewDesc.Texture2DArray.FirstArraySlice = i;
						renderTargetViewDesc.Texture2DArray.ArraySize = 1;

						(*ppTexture2D)->additionalRTVs_DX12.push_back(new D3D12_CPU_DESCRIPTOR_HANDLE);
						(*ppTexture2D)->additionalRTVs_DX12.back()->ptr = RTAllocator->allocate();
						device->CreateRenderTargetView((*ppTexture2D)->texture2D_DX12, &renderTargetViewDesc, *(*ppTexture2D)->additionalRTVs_DX12[i]);
					}
				}
				else if ((*ppTexture2D)->independentRTVArraySlices)
				{
					// independent slices
					for (UINT i = 0; i < slices; ++i)
					{
						renderTargetViewDesc.Texture2DArray.FirstArraySlice = i * 6;
						renderTargetViewDesc.Texture2DArray.ArraySize = 6;

						(*ppTexture2D)->additionalRTVs_DX12.push_back(new D3D12_CPU_DESCRIPTOR_HANDLE);
						(*ppTexture2D)->additionalRTVs_DX12.back()->ptr = RTAllocator->allocate();
						device->CreateRenderTargetView((*ppTexture2D)->texture2D_DX12, &renderTargetViewDesc, *(*ppTexture2D)->additionalRTVs_DX12[i]);
					}
				}

				{
					// Create full-resource RTVs:
					renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
					renderTargetViewDesc.Texture2DArray.ArraySize = arraySize;

					(*ppTexture2D)->RTV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
					(*ppTexture2D)->RTV_DX12->ptr = RTAllocator->allocate();
					device->CreateRenderTargetView((*ppTexture2D)->texture2D_DX12, &renderTargetViewDesc, *(*ppTexture2D)->RTV_DX12);
				}
			}
			else
			{
				// Texture2D, Texture2DArray...
				if (arraySize > 1 && (*ppTexture2D)->independentRTVArraySlices)
				{
					// Create subresource RTVs:
					for (UINT i = 0; i < arraySize; ++i)
					{
						if (multisampled)
						{
							renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
							renderTargetViewDesc.Texture2DMSArray.FirstArraySlice = i;
							renderTargetViewDesc.Texture2DMSArray.ArraySize = 1;
						}
						else
						{
							renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
							renderTargetViewDesc.Texture2DArray.FirstArraySlice = i;
							renderTargetViewDesc.Texture2DArray.ArraySize = 1;
							renderTargetViewDesc.Texture2DArray.MipSlice = 0;
						}

						(*ppTexture2D)->additionalRTVs_DX12.push_back(new D3D12_CPU_DESCRIPTOR_HANDLE);
						(*ppTexture2D)->additionalRTVs_DX12.back()->ptr = RTAllocator->allocate();
						device->CreateRenderTargetView((*ppTexture2D)->texture2D_DX12, &renderTargetViewDesc, *(*ppTexture2D)->additionalRTVs_DX12[i]);
					}
				}

				{
					// Create the full-resource RTV:
					if (arraySize > 1)
					{
						if (multisampled)
						{
							renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
							renderTargetViewDesc.Texture2DMSArray.FirstArraySlice = 0;
							renderTargetViewDesc.Texture2DMSArray.ArraySize = arraySize;
						}
						else
						{
							renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
							renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
							renderTargetViewDesc.Texture2DArray.ArraySize = arraySize;
							renderTargetViewDesc.Texture2DArray.MipSlice = 0;
						}
					}
					else
					{
						if (multisampled)
						{
							renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
						}
						else
						{
							renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
							renderTargetViewDesc.Texture2D.MipSlice = 0;
						}
					}

					(*ppTexture2D)->RTV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
					(*ppTexture2D)->RTV_DX12->ptr = RTAllocator->allocate();
					device->CreateRenderTargetView((*ppTexture2D)->texture2D_DX12, &renderTargetViewDesc, *(*ppTexture2D)->RTV_DX12);
				}
			}
		}

		if ((*ppTexture2D)->desc.BindFlags & BIND_DEPTH_STENCIL)
		{
			UINT arraySize = (*ppTexture2D)->desc.ArraySize;
			UINT sampleCount = (*ppTexture2D)->desc.SampleDesc.Count;
			bool multisampled = sampleCount > 1;

			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
			depthStencilViewDesc.Texture2DArray.MipSlice = 0;
			depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

			// Try to resolve depth stencil format:
			switch ((*ppTexture2D)->desc.Format)
			{
			case FORMAT_R16_TYPELESS:
				depthStencilViewDesc.Format = DXGI_FORMAT_D16_UNORM;
				break;
			case FORMAT_R32_TYPELESS:
				depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
				break;
			case FORMAT_R24G8_TYPELESS:
				depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;
			case FORMAT_R32G8X24_TYPELESS:
				depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
				break;
			default:
				depthStencilViewDesc.Format = _ConvertFormat((*ppTexture2D)->desc.Format);
				break;
			}

			if ((*ppTexture2D)->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
			{
				// TextureCube, TextureCubeArray...
				UINT slices = arraySize / 6;

				depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				depthStencilViewDesc.Texture2DArray.MipSlice = 0;

				if ((*ppTexture2D)->independentRTVCubemapFaces)
				{
					// independent faces
					for (UINT i = 0; i < arraySize; ++i)
					{
						depthStencilViewDesc.Texture2DArray.FirstArraySlice = i;
						depthStencilViewDesc.Texture2DArray.ArraySize = 1;

						(*ppTexture2D)->additionalDSVs_DX12.push_back(new D3D12_CPU_DESCRIPTOR_HANDLE);
						(*ppTexture2D)->additionalDSVs_DX12.back()->ptr = DSAllocator->allocate();
						device->CreateDepthStencilView((*ppTexture2D)->texture2D_DX12, &depthStencilViewDesc, *(*ppTexture2D)->additionalDSVs_DX12[i]);
					}
				}
				else if ((*ppTexture2D)->independentRTVArraySlices)
				{
					// independent slices
					for (UINT i = 0; i < slices; ++i)
					{
						depthStencilViewDesc.Texture2DArray.FirstArraySlice = i * 6;
						depthStencilViewDesc.Texture2DArray.ArraySize = 6;

						(*ppTexture2D)->additionalDSVs_DX12.push_back(new D3D12_CPU_DESCRIPTOR_HANDLE);
						(*ppTexture2D)->additionalDSVs_DX12.back()->ptr = DSAllocator->allocate();
						device->CreateDepthStencilView((*ppTexture2D)->texture2D_DX12, &depthStencilViewDesc, *(*ppTexture2D)->additionalDSVs_DX12[i]);
					}
				}

				{
					// Create full-resource DSV:
					depthStencilViewDesc.Texture2DArray.FirstArraySlice = 0;
					depthStencilViewDesc.Texture2DArray.ArraySize = arraySize;

					(*ppTexture2D)->DSV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
					(*ppTexture2D)->DSV_DX12->ptr = DSAllocator->allocate();
					device->CreateDepthStencilView((*ppTexture2D)->texture2D_DX12, &depthStencilViewDesc, *(*ppTexture2D)->DSV_DX12);
				}
			}
			else
			{
				// Texture2D, Texture2DArray...
				if (arraySize > 1 && (*ppTexture2D)->independentRTVArraySlices)
				{
					// Create subresource DSVs:
					for (UINT i = 0; i < arraySize; ++i)
					{
						if (multisampled)
						{
							depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
							depthStencilViewDesc.Texture2DMSArray.FirstArraySlice = i;
							depthStencilViewDesc.Texture2DMSArray.ArraySize = 1;
						}
						else
						{
							depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
							depthStencilViewDesc.Texture2DArray.MipSlice = 0;
							depthStencilViewDesc.Texture2DArray.FirstArraySlice = i;
							depthStencilViewDesc.Texture2DArray.ArraySize = 1;
						}

						(*ppTexture2D)->additionalDSVs_DX12.push_back(new D3D12_CPU_DESCRIPTOR_HANDLE);
						(*ppTexture2D)->additionalDSVs_DX12.back()->ptr = DSAllocator->allocate();
						device->CreateDepthStencilView((*ppTexture2D)->texture2D_DX12, &depthStencilViewDesc, *(*ppTexture2D)->additionalDSVs_DX12[i]);
					}
				}
				else
				{
					// Create full-resource DSV:
					if (arraySize > 1)
					{
						if (multisampled)
						{
							depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
							depthStencilViewDesc.Texture2DMSArray.FirstArraySlice = 0;
							depthStencilViewDesc.Texture2DMSArray.ArraySize = arraySize;
						}
						else
						{
							depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
							depthStencilViewDesc.Texture2DArray.FirstArraySlice = 0;
							depthStencilViewDesc.Texture2DArray.ArraySize = arraySize;
							depthStencilViewDesc.Texture2DArray.MipSlice = 0;
						}
					}
					else
					{
						if (multisampled)
						{
							depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
						}
						else
						{
							depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
							depthStencilViewDesc.Texture2D.MipSlice = 0;
						}
					}

					(*ppTexture2D)->DSV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
					(*ppTexture2D)->DSV_DX12->ptr = DSAllocator->allocate();
					device->CreateDepthStencilView((*ppTexture2D)->texture2D_DX12, &depthStencilViewDesc, *(*ppTexture2D)->DSV_DX12);
				}
			}
		}

		if ((*ppTexture2D)->desc.BindFlags & BIND_SHADER_RESOURCE)
		{
			UINT arraySize = (*ppTexture2D)->desc.ArraySize;
			UINT sampleCount = (*ppTexture2D)->desc.SampleDesc.Count;
			bool multisampled = sampleCount > 1;

			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			// Try to resolve shader resource format:
			switch ((*ppTexture2D)->desc.Format)
			{
			case FORMAT_R16_TYPELESS:
				shaderResourceViewDesc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			case FORMAT_R32_TYPELESS:
				shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case FORMAT_R24G8_TYPELESS:
				shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case FORMAT_R32G8X24_TYPELESS:
				shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			default:
				shaderResourceViewDesc.Format = _ConvertFormat((*ppTexture2D)->desc.Format);
				break;
			}

			if (arraySize > 1)
			{
				if ((*ppTexture2D)->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
				{
					if (arraySize > 6)
					{
						shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
						shaderResourceViewDesc.TextureCubeArray.First2DArrayFace = 0;
						shaderResourceViewDesc.TextureCubeArray.NumCubes = arraySize / 6;
						shaderResourceViewDesc.TextureCubeArray.MostDetailedMip = 0; //from most detailed...
						shaderResourceViewDesc.TextureCubeArray.MipLevels = -1; //...to least detailed
					}
					else
					{
						shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
						shaderResourceViewDesc.TextureCube.MostDetailedMip = 0; //from most detailed...
						shaderResourceViewDesc.TextureCube.MipLevels = -1; //...to least detailed
					}
				}
				else
				{
					if (multisampled)
					{
						shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
						shaderResourceViewDesc.Texture2DMSArray.FirstArraySlice = 0;
						shaderResourceViewDesc.Texture2DMSArray.ArraySize = arraySize;
					}
					else
					{
						shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
						shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
						shaderResourceViewDesc.Texture2DArray.ArraySize = arraySize;
						shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0; //from most detailed...
						shaderResourceViewDesc.Texture2DArray.MipLevels = -1; //...to least detailed
					}
				}
				(*ppTexture2D)->SRV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
				(*ppTexture2D)->SRV_DX12->ptr = ResourceAllocator->allocate();
				device->CreateShaderResourceView((*ppTexture2D)->texture2D_DX12, &shaderResourceViewDesc, *(*ppTexture2D)->SRV_DX12);
			}
			else
			{
				if (multisampled)
				{
					shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
					(*ppTexture2D)->SRV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
					(*ppTexture2D)->SRV_DX12->ptr = ResourceAllocator->allocate();
					device->CreateShaderResourceView((*ppTexture2D)->texture2D_DX12, &shaderResourceViewDesc, *(*ppTexture2D)->SRV_DX12);
				}
				else
				{
					shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

					if ((*ppTexture2D)->independentSRVMIPs)
					{
						// Create subresource SRVs:
						UINT miplevels = (*ppTexture2D)->desc.MipLevels;
						for (UINT i = 0; i < miplevels; ++i)
						{
							shaderResourceViewDesc.Texture2D.MostDetailedMip = i;
							shaderResourceViewDesc.Texture2D.MipLevels = 1;

							(*ppTexture2D)->additionalSRVs_DX12.push_back(new D3D12_CPU_DESCRIPTOR_HANDLE);
							(*ppTexture2D)->additionalSRVs_DX12.back()->ptr = ResourceAllocator->allocate();
							device->CreateShaderResourceView((*ppTexture2D)->texture2D_DX12, &shaderResourceViewDesc, *(*ppTexture2D)->additionalSRVs_DX12[i]);
						}
					}

					{
						// Create full-resource SRV:
						shaderResourceViewDesc.Texture2D.MostDetailedMip = 0; //from most detailed...
						shaderResourceViewDesc.Texture2D.MipLevels = -1; //...to least detailed
						(*ppTexture2D)->SRV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
						(*ppTexture2D)->SRV_DX12->ptr = ResourceAllocator->allocate();
						device->CreateShaderResourceView((*ppTexture2D)->texture2D_DX12, &shaderResourceViewDesc, *(*ppTexture2D)->SRV_DX12);
					}
				}
			}
		}


		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			assert((*ppTexture2D)->independentRTVArraySlices == false && "TextureArray UAV not implemented!");

			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

			if ((*ppTexture2D)->independentUAVMIPs)
			{
				// Create subresource UAVs:
				UINT miplevels = (*ppTexture2D)->desc.MipLevels;
				for (UINT i = 0; i < miplevels; ++i)
				{
					uav_desc.Texture2D.MipSlice = i;

					(*ppTexture2D)->additionalUAVs_DX12.push_back(new D3D12_CPU_DESCRIPTOR_HANDLE);
					(*ppTexture2D)->additionalUAVs_DX12.back()->ptr = ResourceAllocator->allocate();
					device->CreateUnorderedAccessView((*ppTexture2D)->texture2D_DX12, nullptr, &uav_desc, *(*ppTexture2D)->additionalUAVs_DX12[i]);
				}
			}

			{
				// Create main resource UAV:
				uav_desc.Texture2D.MipSlice = 0;
				(*ppTexture2D)->UAV_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
				(*ppTexture2D)->UAV_DX12->ptr = ResourceAllocator->allocate();
				device->CreateUnorderedAccessView((*ppTexture2D)->texture2D_DX12, nullptr, &uav_desc, *(*ppTexture2D)->UAV_DX12);
			}
		}

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
	HRESULT GraphicsDevice_DX12::CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
		const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout)
	{
		pInputLayout->desc.reserve((size_t)NumElements);
		for (UINT i = 0; i < NumElements; ++i)
		{
			pInputLayout->desc.push_back(pInputElementDescs[i]);
		}

		return S_OK;
	}
	HRESULT GraphicsDevice_DX12::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, VertexShader *pVertexShader)
	{
		pVertexShader->code.data = new BYTE[BytecodeLength];
		memcpy(pVertexShader->code.data, pShaderBytecode, BytecodeLength);
		pVertexShader->code.size = BytecodeLength;

		return (pVertexShader->code.data != nullptr && pVertexShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, PixelShader *pPixelShader)
	{
		pPixelShader->code.data = new BYTE[BytecodeLength];
		memcpy(pPixelShader->code.data, pShaderBytecode, BytecodeLength);
		pPixelShader->code.size = BytecodeLength;

		return (pPixelShader->code.data != nullptr && pPixelShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, GeometryShader *pGeometryShader)
	{
		pGeometryShader->code.data = new BYTE[BytecodeLength];
		memcpy(pGeometryShader->code.data, pShaderBytecode, BytecodeLength);
		pGeometryShader->code.size = BytecodeLength;

		return (pGeometryShader->code.data != nullptr && pGeometryShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, HullShader *pHullShader)
	{
		pHullShader->code.data = new BYTE[BytecodeLength];
		memcpy(pHullShader->code.data, pShaderBytecode, BytecodeLength);
		pHullShader->code.size = BytecodeLength;

		return (pHullShader->code.data != nullptr && pHullShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, DomainShader *pDomainShader)
	{
		pDomainShader->code.data = new BYTE[BytecodeLength];
		memcpy(pDomainShader->code.data, pShaderBytecode, BytecodeLength);
		pDomainShader->code.size = BytecodeLength;

		return (pDomainShader->code.data != nullptr && pDomainShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ComputeShader *pComputeShader)
	{
		pComputeShader->code.data = new BYTE[BytecodeLength];
		memcpy(pComputeShader->code.data, pShaderBytecode, BytecodeLength);
		pComputeShader->code.size = BytecodeLength;

		return (pComputeShader->code.data != nullptr && pComputeShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState)
	{
		pBlendState->desc = *pBlendStateDesc;
		return S_OK;
	}
	HRESULT GraphicsDevice_DX12::CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState)
	{
		pDepthStencilState->desc = *pDepthStencilStateDesc;
		return S_OK;
	}
	HRESULT GraphicsDevice_DX12::CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState)
	{
		pRasterizerState->desc = *pRasterizerStateDesc;
		return S_OK;
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

		pSamplerState->resource_DX12 = new D3D12_CPU_DESCRIPTOR_HANDLE;
		pSamplerState->resource_DX12->ptr = SamplerAllocator->allocate();
		device->CreateSampler(&desc, *pSamplerState->resource_DX12);

		return S_OK;
	}
	HRESULT GraphicsDevice_DX12::CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery)
	{
		HRESULT hr = E_FAIL;

		pQuery->desc = *pDesc;
		pQuery->async_frameshift = pQuery->desc.async_latency;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateGraphicsPSO(const GraphicsPSODesc* pDesc, GraphicsPSO* pso)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

		if (pDesc->vs != nullptr)
		{
			desc.VS.pShaderBytecode = pDesc->vs->code.data;
			desc.VS.BytecodeLength = pDesc->vs->code.size;
		}
		if (pDesc->hs != nullptr)
		{
			desc.HS.pShaderBytecode = pDesc->hs->code.data;
			desc.HS.BytecodeLength = pDesc->hs->code.size;
		}
		if (pDesc->ds != nullptr)
		{
			desc.DS.pShaderBytecode = pDesc->ds->code.data;
			desc.DS.BytecodeLength = pDesc->ds->code.size;
		}
		if (pDesc->gs != nullptr)
		{
			desc.GS.pShaderBytecode = pDesc->gs->code.data;
			desc.GS.BytecodeLength = pDesc->gs->code.size;
		}
		if (pDesc->ps != nullptr)
		{
			desc.PS.BytecodeLength = pDesc->ps->code.size;
			desc.PS.pShaderBytecode = pDesc->ps->code.data;
		}

		RasterizerStateDesc pRasterizerStateDesc = pDesc->rs != nullptr ? pDesc->rs->GetDesc() : RasterizerStateDesc();
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


		DepthStencilStateDesc pDepthStencilStateDesc = pDesc->dss != nullptr ? pDesc->dss->GetDesc() : DepthStencilStateDesc();
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


		BlendStateDesc pBlendStateDesc = pDesc->bs != nullptr ? pDesc->bs->GetDesc() : BlendStateDesc();
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

		D3D12_INPUT_ELEMENT_DESC* elements = nullptr;
		if (pDesc->il != nullptr)
		{
			desc.InputLayout.NumElements = (UINT)pDesc->il->desc.size();
			elements = new D3D12_INPUT_ELEMENT_DESC[desc.InputLayout.NumElements];
			for (UINT i = 0; i < desc.InputLayout.NumElements; ++i)
			{
				elements[i].SemanticName = pDesc->il->desc[i].SemanticName;
				elements[i].SemanticIndex = pDesc->il->desc[i].SemanticIndex;
				elements[i].Format = _ConvertFormat(pDesc->il->desc[i].Format);
				elements[i].InputSlot = pDesc->il->desc[i].InputSlot;
				elements[i].AlignedByteOffset = pDesc->il->desc[i].AlignedByteOffset;
				if (elements[i].AlignedByteOffset == APPEND_ALIGNED_ELEMENT)
					elements[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
				elements[i].InputSlotClass = _ConvertInputClassification(pDesc->il->desc[i].InputSlotClass);
				elements[i].InstanceDataStepRate = pDesc->il->desc[i].InstanceDataStepRate;
			}
		}
		desc.InputLayout.pInputElementDescs = elements;

		desc.NumRenderTargets = pDesc->numRTs;
		for (UINT i = 0; i < desc.NumRenderTargets; ++i)
		{
			desc.RTVFormats[i] = _ConvertFormat(pDesc->RTFormats[i]);
		}
		desc.DSVFormat = _ConvertFormat(pDesc->DSFormat);

		desc.SampleDesc.Count = pDesc->sampleDesc.Count;
		desc.SampleDesc.Quality = pDesc->sampleDesc.Quality;
		desc.SampleMask = pDesc->sampleMask;

		switch (pDesc->ptt)
		{
		case PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED:
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
			break;
		case PRIMITIVE_TOPOLOGY_TYPE_POINT:
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			break;
		case PRIMITIVE_TOPOLOGY_TYPE_LINE:
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			break;
		case PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			break;
		case PRIMITIVE_TOPOLOGY_TYPE_PATCH:
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			break;
		}

		desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

		desc.pRootSignature = graphicsRootSig;

		HRESULT hr = device->CreateGraphicsPipelineState(&desc, __uuidof(ID3D12PipelineState), (void**)&pso->resource_DX12);
		assert(SUCCEEDED(hr));

		SAFE_DELETE_ARRAY(elements);

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateComputePSO(const ComputePSODesc* pDesc, ComputePSO* pso)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};

		if (pDesc->cs != nullptr)
		{
			desc.CS.pShaderBytecode = pDesc->cs->code.data;
			desc.CS.BytecodeLength = pDesc->cs->code.size;
		}

		desc.pRootSignature = computeRootSig;

		HRESULT hr = device->CreateComputePipelineState(&desc, __uuidof(ID3D12PipelineState), (void**)&pso->resource_DX12);
		assert(SUCCEEDED(hr));

		return hr;
	}


	void GraphicsDevice_DX12::PresentBegin()
	{
		LOCK();

		BindViewports(1, &viewPort, GRAPHICSTHREAD_IMMEDIATE);


		// Record commands in the command list now.
		// Start by setting the resource barrier.
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = backBuffer[backBufferIndex];
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->ResourceBarrier(1, &barrier);



		// Get the render target view handle for the current back buffer.
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = RTAllocator->heap->GetCPUDescriptorHandleForHeapStart();
		UINT renderTargetViewDescriptorSize = RTAllocator->itemSize;
		if (backBufferIndex == 1)
		{
			renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
		}


		// Set the back buffer as the render target.
		static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, NULL);


		// Then set the color to clear the window to.
		float color[4];
		color[0] = 0.5;
		color[1] = 0.5;
		color[2] = 0.5;
		color[3] = 1.0;
		static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->ClearRenderTargetView(renderTargetViewHandle, color, 0, NULL);


	}
	void GraphicsDevice_DX12::PresentEnd()
	{
		HRESULT result;


		// Indicate that the back buffer will now be used to present.
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = backBuffer[backBufferIndex];
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->ResourceBarrier(1, &barrier);

		// Close the list of commands.
		result = static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->Close();

		// Execute the list of commands.
		commandQueue->ExecuteCommandLists(1, commandLists);


		swapChain->Present(VSYNC, 0);


		// Signal and increment the fence value.
		UINT64 fenceToWaitFor = commandFenceValues[GRAPHICSTHREAD_IMMEDIATE];
		result = commandQueue->Signal(commandFences[GRAPHICSTHREAD_IMMEDIATE], fenceToWaitFor);
		commandFenceValues[GRAPHICSTHREAD_IMMEDIATE]++;

		// Wait until the GPU is done rendering.
		if (commandFences[GRAPHICSTHREAD_IMMEDIATE]->GetCompletedValue() < fenceToWaitFor)
		{
			result = commandFences[GRAPHICSTHREAD_IMMEDIATE]->SetEventOnCompletion(fenceToWaitFor, commandFenceEvents[GRAPHICSTHREAD_IMMEDIATE]);
			WaitForSingleObject(commandFenceEvents[GRAPHICSTHREAD_IMMEDIATE], INFINITE);
		}

		// Alternate the back buffer index back and forth between 0 and 1 each frame.
		backBufferIndex == 0 ? backBufferIndex = 1 : backBufferIndex = 0;



		// Reset (re-use) the memory associated command allocator.
		result = commandAllocators[GRAPHICSTHREAD_IMMEDIATE]->Reset();

		// Reset the command list, use empty pipeline state for now since there are no shaders and we are just clearing the screen.
		result = static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->Reset(commandAllocators[GRAPHICSTHREAD_IMMEDIATE], nullptr);


		ID3D12DescriptorHeap* heaps[] = {
			ResourceDescriptorHeapGPU[GRAPHICSTHREAD_IMMEDIATE], SamplerDescriptorHeapGPU[GRAPHICSTHREAD_IMMEDIATE]
		};
		static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

		static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->SetGraphicsRootSignature(graphicsRootSig);
		static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->SetComputeRootSignature(computeRootSig);

		for (UINT shader = VS; shader < SHADERSTAGE_COUNT - 1; ++shader)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE resource_table = ResourceDescriptorHeapGPU[GRAPHICSTHREAD_IMMEDIATE]->GetGPUDescriptorHandleForHeapStart();
			resource_table.ptr += shader * GPU_RESOURCE_HEAP_COUNT * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->SetGraphicsRootDescriptorTable(shader * 2 + 0,
				resource_table);

			D3D12_GPU_DESCRIPTOR_HANDLE sampler_table = SamplerDescriptorHeapGPU[GRAPHICSTHREAD_IMMEDIATE]->GetGPUDescriptorHandleForHeapStart();
			sampler_table.ptr += shader * GPU_SAMPLER_HEAP_COUNT * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->SetGraphicsRootDescriptorTable(shader * 2 + 1,
				sampler_table);
		}

		D3D12_GPU_DESCRIPTOR_HANDLE resource_table = ResourceDescriptorHeapGPU[GRAPHICSTHREAD_IMMEDIATE]->GetGPUDescriptorHandleForHeapStart();
		resource_table.ptr += CS * GPU_RESOURCE_HEAP_COUNT * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->SetComputeRootDescriptorTable(0,
			resource_table);

		D3D12_GPU_DESCRIPTOR_HANDLE sampler_table = SamplerDescriptorHeapGPU[GRAPHICSTHREAD_IMMEDIATE]->GetGPUDescriptorHandleForHeapStart();
		sampler_table.ptr += CS * GPU_SAMPLER_HEAP_COUNT * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		static_cast<ID3D12GraphicsCommandList*>(commandLists[GRAPHICSTHREAD_IMMEDIATE])->SetComputeRootDescriptorTable(1,
			sampler_table);


		FRAMECOUNT++;

		RESOLUTIONCHANGED = false;

		UNLOCK();
	}

	void GraphicsDevice_DX12::ExecuteDeferredContexts()
	{
		commandQueue->ExecuteCommandLists(GRAPHICSTHREAD_COUNT - 1, &commandLists[1]);
		for (int i = GRAPHICSTHREAD_IMMEDIATE + 1; i < GRAPHICSTHREAD_COUNT; ++i)
		{
			HRESULT hr = commandAllocators[i]->Reset();
			assert(SUCCEEDED(hr));
			hr = static_cast<ID3D12GraphicsCommandList*>(commandLists[i])->Reset(commandAllocators[i], nullptr);
			assert(SUCCEEDED(hr));
		}
	}
	void GraphicsDevice_DX12::FinishCommandList(GRAPHICSTHREAD threadID)
	{
		if (threadID == GRAPHICSTHREAD_IMMEDIATE)
			return;
		HRESULT hr = static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->Close();
		assert(SUCCEEDED(hr));
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

	void GraphicsDevice_DX12::BindResources(SHADERSTAGE stage, const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
		if (resources != nullptr)
		{
			for (int i = 0; i < count; ++i)
			{
				if (resources[i] != nullptr && resources[i]->SRV_DX12 != nullptr)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst = ResourceDescriptorHeapGPU[threadID]->GetCPUDescriptorHandleForHeapStart();
					int offset = stage * GPU_RESOURCE_HEAP_COUNT + GPU_RESOURCE_HEAP_CBV_COUNT + slot + i;
					dst.ptr += (SIZE_T)(offset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

					device->CopyDescriptorsSimple(1, dst, *resources[i]->SRV_DX12, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
			}
		}
	}
	void GraphicsDevice_DX12::BindResource(SHADERSTAGE stage, const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		if (resource != nullptr)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE dst = ResourceDescriptorHeapGPU[threadID]->GetCPUDescriptorHandleForHeapStart();
			int offset = stage * GPU_RESOURCE_HEAP_COUNT + GPU_RESOURCE_HEAP_CBV_COUNT + slot;
			dst.ptr += (SIZE_T)(offset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

			if (arrayIndex < 0)
			{
				if (resource->SRV_DX12 != nullptr)
				{
					device->CopyDescriptorsSimple(1, dst, *resource->SRV_DX12, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
			}
			else
			{
				assert(resource->additionalSRVs_DX12.size() > static_cast<size_t>(arrayIndex) && "Invalid arrayIndex!");
				device->CopyDescriptorsSimple(1, dst, *resource->additionalSRVs_DX12[arrayIndex], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
		}
	}
	void GraphicsDevice_DX12::BindSampler(SHADERSTAGE stage, const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
		if (sampler != nullptr && sampler->resource_DX12 != nullptr)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE dst = SamplerDescriptorHeapGPU[threadID]->GetCPUDescriptorHandleForHeapStart();
			int offset = stage * GPU_SAMPLER_HEAP_COUNT + slot;
			dst.ptr += (SIZE_T)(offset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));

			device->CopyDescriptorsSimple(1, dst, *sampler->resource_DX12, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		}
	}
	void GraphicsDevice_DX12::BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
		if (buffer != nullptr && buffer->CBV_DX12 != nullptr)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE dst = ResourceDescriptorHeapGPU[threadID]->GetCPUDescriptorHandleForHeapStart();
			int offset = stage * GPU_RESOURCE_HEAP_COUNT + slot;
			dst.ptr += (SIZE_T)(offset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

			device->CopyDescriptorsSimple(1, dst, *buffer->CBV_DX12, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}

	void GraphicsDevice_DX12::BindResourcePS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		BindResource(PS, resource, slot, threadID, arrayIndex);
	}
	void GraphicsDevice_DX12::BindResourceVS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		BindResource(VS, resource, slot, threadID, arrayIndex);
	}
	void GraphicsDevice_DX12::BindResourceGS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		BindResource(GS, resource, slot, threadID, arrayIndex);
	}
	void GraphicsDevice_DX12::BindResourceDS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		BindResource(DS, resource, slot, threadID, arrayIndex);
	}
	void GraphicsDevice_DX12::BindResourceHS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		BindResource(HS, resource, slot, threadID, arrayIndex);
	}
	void GraphicsDevice_DX12::BindResourceCS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		BindResource(CS, resource, slot, threadID, arrayIndex);
	}
	void GraphicsDevice_DX12::BindResourcesPS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
		BindResources(PS, resources, slot, count, threadID);
	}
	void GraphicsDevice_DX12::BindResourcesVS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
		BindResources(VS, resources, slot, count, threadID);
	}
	void GraphicsDevice_DX12::BindResourcesGS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
		BindResources(GS, resources, slot, count, threadID);
	}
	void GraphicsDevice_DX12::BindResourcesDS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
		BindResources(DS, resources, slot, count, threadID);
	}
	void GraphicsDevice_DX12::BindResourcesHS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
		BindResources(HS, resources, slot, count, threadID);
	}
	void GraphicsDevice_DX12::BindResourcesCS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
		BindResources(CS, resources, slot, count, threadID);
	}
	void GraphicsDevice_DX12::BindUnorderedAccessResourceCS(const GPUUnorderedResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		if (resource != nullptr)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE dst = ResourceDescriptorHeapGPU[threadID]->GetCPUDescriptorHandleForHeapStart();
			int offset = CS * GPU_RESOURCE_HEAP_COUNT + GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + slot;
			dst.ptr += (SIZE_T)(offset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

			if (arrayIndex < 0)
			{
				if (resource->UAV_DX12 != nullptr)
				{
					device->CopyDescriptorsSimple(1, dst, *resource->UAV_DX12, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
			}
			else
			{
				assert(resource->additionalUAVs_DX12.size() > static_cast<size_t>(arrayIndex) && "Invalid arrayIndex!");
				device->CopyDescriptorsSimple(1, dst, *resource->additionalUAVs_DX12[arrayIndex], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
		}
	}
	void GraphicsDevice_DX12::BindUnorderedAccessResourcesCS(const GPUUnorderedResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID)
	{
		if (resources != nullptr)
		{
			for (int i = 0; i < count; ++i)
			{
				if (resources[i] != nullptr && resources[i]->UAV_DX12 != nullptr)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst = ResourceDescriptorHeapGPU[threadID]->GetCPUDescriptorHandleForHeapStart();
					int offset = CS * GPU_RESOURCE_HEAP_COUNT + GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + slot + i;
					dst.ptr += (SIZE_T)(offset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

					device->CopyDescriptorsSimple(1, dst, *resources[i]->UAV_DX12, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
			}
		}
	}
	void GraphicsDevice_DX12::UnBindResources(int slot, int num, GRAPHICSTHREAD threadID)
	{
		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			for (int i = 0; i < num; ++i)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE dst = ResourceDescriptorHeapGPU[threadID]->GetCPUDescriptorHandleForHeapStart();
				int offset = stage * GPU_RESOURCE_HEAP_COUNT + GPU_RESOURCE_HEAP_CBV_COUNT + slot + i;
				dst.ptr += (SIZE_T)(offset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

				device->CopyDescriptorsSimple(1, dst, *nullSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
		}
	}
	void GraphicsDevice_DX12::UnBindUnorderedAccessResources(int slot, int num, GRAPHICSTHREAD threadID)
	{
		for (int i = 0; i < num; ++i)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE dst = ResourceDescriptorHeapGPU[threadID]->GetCPUDescriptorHandleForHeapStart();
			int offset = CS * GPU_RESOURCE_HEAP_COUNT + GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + slot + i;
			dst.ptr += (SIZE_T)(offset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

			device->CopyDescriptorsSimple(1, dst, *nullUAV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}
	void GraphicsDevice_DX12::BindSamplerPS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
		BindSampler(PS, sampler, slot, threadID);
	}
	void GraphicsDevice_DX12::BindSamplerVS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
		BindSampler(VS, sampler, slot, threadID);
	}
	void GraphicsDevice_DX12::BindSamplerGS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
		BindSampler(GS, sampler, slot, threadID);
	}
	void GraphicsDevice_DX12::BindSamplerHS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
		BindSampler(HS, sampler, slot, threadID);
	}
	void GraphicsDevice_DX12::BindSamplerDS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
		BindSampler(DS, sampler, slot, threadID);
	}
	void GraphicsDevice_DX12::BindSamplerCS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID)
	{
		BindSampler(CS, sampler, slot, threadID);
	}
	void GraphicsDevice_DX12::BindConstantBufferPS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
		BindConstantBuffer(PS, buffer, slot, threadID);
	}
	void GraphicsDevice_DX12::BindConstantBufferVS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
		BindConstantBuffer(VS, buffer, slot, threadID);
	}
	void GraphicsDevice_DX12::BindConstantBufferGS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
		BindConstantBuffer(GS, buffer, slot, threadID);
	}
	void GraphicsDevice_DX12::BindConstantBufferDS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
		BindConstantBuffer(DS, buffer, slot, threadID);
	}
	void GraphicsDevice_DX12::BindConstantBufferHS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
		BindConstantBuffer(HS, buffer, slot, threadID);
	}
	void GraphicsDevice_DX12::BindConstantBufferCS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID)
	{
		BindConstantBuffer(CS, buffer, slot, threadID);
	}
	void GraphicsDevice_DX12::BindVertexBuffers(const GPUBuffer* const *vertexBuffers, int slot, int count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID)
	{
		assert(count <= 8);
		D3D12_VERTEX_BUFFER_VIEW res[8] = { 0 };
		for (int i = 0; i < count; ++i)
		{
			if (vertexBuffers[i] != nullptr)
			{
				res[i].BufferLocation = vertexBuffers[i]->resource_DX12->GetGPUVirtualAddress();
				if (offsets != nullptr)
				{
					res[i].BufferLocation += (D3D12_GPU_VIRTUAL_ADDRESS)offsets[i];
				}
				res[i].SizeInBytes = vertexBuffers[i]->desc.ByteWidth;
				res[i].StrideInBytes = strides[i];
			}
		}
		static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->IASetVertexBuffers(static_cast<UINT>(slot), static_cast<UINT>(count), res);
	}
	void GraphicsDevice_DX12::BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID)
	{
		D3D12_INDEX_BUFFER_VIEW res = {};
		if (indexBuffer != nullptr)
		{
			res.BufferLocation = indexBuffer->resource_DX12->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
			res.Format = (format == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
			res.SizeInBytes = indexBuffer->desc.ByteWidth;
		}
		static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->IASetIndexBuffer(&res);
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
	void GraphicsDevice_DX12::BindStencilRef(UINT value, GRAPHICSTHREAD threadID)
	{
		static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->OMSetStencilRef(value);
	}
	void GraphicsDevice_DX12::BindBlendFactor(XMFLOAT4 value, GRAPHICSTHREAD threadID)
	{
		const float blendFactor[4] = { value.x, value.y, value.z, value.w };
		static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->OMSetBlendFactor(blendFactor);
	}
	void GraphicsDevice_DX12::BindGraphicsPSO(const GraphicsPSO* pso, GRAPHICSTHREAD threadID)
	{
		static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->SetPipelineState(pso->resource_DX12);
	}
	void GraphicsDevice_DX12::BindComputePSO(const ComputePSO* pso, GRAPHICSTHREAD threadID)
	{
		static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->SetPipelineState(pso->resource_DX12);
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
		assert(rects != nullptr);
		assert(numRects <= 8);
		D3D12_RECT pRects[8];
		for (UINT i = 0; i < numRects; ++i)
		{
			pRects[i].bottom = rects[i].bottom;
			pRects[i].left = rects[i].left;
			pRects[i].right = rects[i].right;
			pRects[i].top = rects[i].top;
		}
		static_cast<ID3D12GraphicsCommandList*>(commandLists[threadID])->RSSetScissorRects(numRects, pRects);
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
