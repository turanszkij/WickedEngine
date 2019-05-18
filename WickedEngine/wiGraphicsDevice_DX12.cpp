#include "wiGraphicsDevice_DX12.h"
#include "wiGraphicsDevice_SharedInternals.h"
#include "wiHelper.h"
#include "ResourceMapping.h"
#include "wiBackLog.h"

#include "Utility/d3dx12.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"Dxgi.lib")
#pragma comment(lib,"dxguid.lib")

#ifdef _DEBUG
#include <d3d12sdklayers.h>
#endif // _DEBUG

#include <sstream>
#include <algorithm>
#include <wincodec.h>

using namespace std;

namespace wiGraphics
{
	// Engine -> Native converters

	inline D3D12_CPU_DESCRIPTOR_HANDLE ToNativeHandle(wiCPUHandle handle)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE native;
		native.ptr = (SIZE_T)handle;
		return native;
	}

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

	inline D3D12_RESOURCE_STATES _ConvertResourceStates(RESOURCE_STATES value)
	{
		return static_cast<D3D12_RESOURCE_STATES>(value);
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
	inline RESOURCE_STATES _ConvertResourceStates_Inv(D3D12_RESOURCE_STATES value)
	{
		return static_cast<RESOURCE_STATES>(value);
	}
	
	inline TextureDesc _ConvertTextureDesc_Inv(const D3D12_RESOURCE_DESC& desc)
	{
		TextureDesc retVal;

		retVal.Format = _ConvertFormat_Inv(desc.Format);
		retVal.Width = (UINT)desc.Width;
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

	// Allocator heaps:

	GraphicsDevice_DX12::DescriptorAllocator::DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT maxCount)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = maxCount;
		heapDesc.Type = type;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRESULT hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&heap);
		assert(SUCCEEDED(hr));

		heap_begin = heap->GetCPUDescriptorHandleForHeapStart().ptr;
		itemSize = device->GetDescriptorHandleIncrementSize(type);
		itemCount = 0;
		this->maxCount = maxCount;

		itemsAlive = new bool[maxCount];
		for (uint32_t i = 0; i < maxCount; ++i)
		{
			itemsAlive[i] = false;
		}
		lastAlloc = 0;
	}
	GraphicsDevice_DX12::DescriptorAllocator::~DescriptorAllocator()
	{
		SAFE_RELEASE(heap);
		SAFE_DELETE_ARRAY(itemsAlive);
	}
	size_t GraphicsDevice_DX12::DescriptorAllocator::allocate()
	{
		size_t addr = 0;

		lock.lock();
		if (itemCount < maxCount - 1)
		{
			while (itemsAlive[lastAlloc] == true)
			{
				lastAlloc = (lastAlloc + 1) % maxCount;
			}
			addr = heap_begin + lastAlloc * itemSize;
			itemsAlive[lastAlloc] = true;

			itemCount++;
		}
		else
		{
			assert(0);
		}
		lock.unlock();

		return addr;
	}
	void GraphicsDevice_DX12::DescriptorAllocator::clear()
	{
		lock.lock();
		for (uint32_t i = 0; i < maxCount; ++i)
		{
			itemsAlive[i] = false;
		}
		itemCount = 0;
		lock.unlock();
	}
	void GraphicsDevice_DX12::DescriptorAllocator::free(wiCPUHandle descriptorHandle)
	{
		if (descriptorHandle == WI_NULL_HANDLE)
		{
			return;
		}

		assert(descriptorHandle >= heap_begin);
		assert(descriptorHandle < heap_begin + maxCount * itemSize);
		uint32_t offset = (uint32_t)(descriptorHandle - heap_begin);
		assert(offset % itemSize == 0);
		offset = offset / itemSize;

		lock.lock();
		itemsAlive[offset] = false;
		assert(itemCount > 0);
		itemCount--;
		lock.unlock();
	}



	GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::DescriptorTableFrameAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT maxRenameCount)
	{
		if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
		{
			itemCount = GPU_SAMPLER_HEAP_COUNT;
		}
		else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		{
			itemCount = (GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + GPU_RESOURCE_HEAP_UAV_COUNT);
		}
		else
		{
			assert(0);
		}

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NodeMask = 0;
		heapDesc.Type = type;

		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NumDescriptors = itemCount * SHADERSTAGE_COUNT;
		HRESULT hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&heap_CPU);

		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NumDescriptors = itemCount * SHADERSTAGE_COUNT * maxRenameCount;
		hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&heap_GPU);
		assert(SUCCEEDED(hr));

		descriptorType = type;
		itemSize = device->GetDescriptorHandleIncrementSize(type);

		boundDescriptors = new wiCPUHandle[SHADERSTAGE_COUNT * itemCount];
	}
	GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::~DescriptorTableFrameAllocator()
	{
		SAFE_RELEASE(heap_CPU);
		SAFE_RELEASE(heap_GPU);
		SAFE_DELETE_ARRAY(boundDescriptors);
	}
	void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::reset(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE* nullDescriptorsSamplerCBVSRVUAV)
	{
		memset(boundDescriptors, 0, sizeof(wiCPUHandle)*SHADERSTAGE_COUNT*itemCount);

		ringOffset = 0;

		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			dirty[stage] = true;

			// Fill staging tables with null descriptors (TODO make nicer and reduce copy ops):
			if (descriptorType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
			{
				for (int slot = 0; slot < GPU_RESOURCE_HEAP_CBV_COUNT; ++slot)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst_staging = heap_CPU->GetCPUDescriptorHandleForHeapStart();
					dst_staging.ptr += (stage * itemCount + slot) * itemSize;

					device->CopyDescriptorsSimple(1, dst_staging, nullDescriptorsSamplerCBVSRVUAV[1], (D3D12_DESCRIPTOR_HEAP_TYPE)descriptorType);
				}
				for (int slot = 0; slot < GPU_RESOURCE_HEAP_SRV_COUNT; ++slot)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst_staging = heap_CPU->GetCPUDescriptorHandleForHeapStart();
					dst_staging.ptr += (stage * itemCount + GPU_RESOURCE_HEAP_CBV_COUNT + slot) * itemSize;

					device->CopyDescriptorsSimple(1, dst_staging, nullDescriptorsSamplerCBVSRVUAV[2], (D3D12_DESCRIPTOR_HEAP_TYPE)descriptorType);
				}
				for (int slot = 0; slot < GPU_RESOURCE_HEAP_UAV_COUNT; ++slot)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst_staging = heap_CPU->GetCPUDescriptorHandleForHeapStart();
					dst_staging.ptr += (stage * itemCount + GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + slot) * itemSize;

					device->CopyDescriptorsSimple(1, dst_staging, nullDescriptorsSamplerCBVSRVUAV[3], (D3D12_DESCRIPTOR_HEAP_TYPE)descriptorType);
				}
			}
			else
			{
				for (int slot = 0; slot < GPU_SAMPLER_HEAP_COUNT; ++slot)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst_staging = heap_CPU->GetCPUDescriptorHandleForHeapStart();
					dst_staging.ptr += (stage * itemCount + slot) * itemSize;

					device->CopyDescriptorsSimple(1, dst_staging, nullDescriptorsSamplerCBVSRVUAV[0], (D3D12_DESCRIPTOR_HEAP_TYPE)descriptorType);
				}
			}

		}
	}
	void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::update(SHADERSTAGE stage, UINT offset, wiCPUHandle descriptor, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
	{
		if (descriptor == WI_NULL_HANDLE)
		{
			return;
		}
		UINT idx = stage * itemCount + offset;

		if (boundDescriptors[idx] == descriptor)
		{
			return;
		}

		boundDescriptors[idx] = descriptor;

		dirty[stage] = true;

		D3D12_CPU_DESCRIPTOR_HANDLE dst_staging = heap_CPU->GetCPUDescriptorHandleForHeapStart();
		dst_staging.ptr += idx * itemSize;

		device->CopyDescriptorsSimple(1, dst_staging, ToNativeHandle(descriptor), (D3D12_DESCRIPTOR_HEAP_TYPE)descriptorType);
	}
	void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::validate(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
	{
		for (int stage = VS; stage < SHADERSTAGE_COUNT; ++stage)
		{
			if (dirty[stage])
			{
				// copy prev table contents to new dest
				D3D12_CPU_DESCRIPTOR_HANDLE dst = heap_GPU->GetCPUDescriptorHandleForHeapStart();
				dst.ptr += ringOffset;
				D3D12_CPU_DESCRIPTOR_HANDLE src = heap_CPU->GetCPUDescriptorHandleForHeapStart();
				src.ptr += (stage * itemCount) * itemSize;
				device->CopyDescriptorsSimple(itemCount, dst, src, (D3D12_DESCRIPTOR_HEAP_TYPE)descriptorType);

				// bind table to root sig
				D3D12_GPU_DESCRIPTOR_HANDLE table = heap_GPU->GetGPUDescriptorHandleForHeapStart();
				table.ptr += ringOffset;

				if (stage == CS)
				{
					// compute descriptor heap:

					if (descriptorType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
					{
						commandList->SetComputeRootDescriptorTable(0, table);
					}
					else
					{
						commandList->SetComputeRootDescriptorTable(1, table);
					}
				}
				else
				{
					// graphics descriptor heap:

					if (descriptorType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
					{
						commandList->SetGraphicsRootDescriptorTable(stage * 2 + 0, table);
					}
					else
					{
						commandList->SetGraphicsRootDescriptorTable(stage * 2 + 1, table);
					}
				}

				// mark this as up to date
				dirty[stage] = false;

				// allocate next chunk
				ringOffset += itemCount*itemSize;
			}
		}
	}


	GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::ResourceFrameAllocator(ID3D12Device* device, size_t size)
	{
		HRESULT hr = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			__uuidof(ID3D12Resource), (void**)&buffer.resource);

		assert(SUCCEEDED(hr));

		void* pData;
		//
		// No CPU reads will be done from the resource.
		//
		CD3DX12_RANGE readRange(0, 0);
		((ID3D12Resource*)buffer.resource)->Map(0, &readRange, &pData);
		dataCur = dataBegin = reinterpret_cast<uint8_t*>(pData);
		dataEnd = dataBegin + size;

		// Because the "buffer" is created by hand in this, fill the desc to indicate how it can be used:
		buffer.desc.ByteWidth = (UINT)((size_t)dataEnd - (size_t)dataBegin);
		buffer.desc.Usage = USAGE_DYNAMIC;
		buffer.desc.BindFlags = BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
		buffer.desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	}
	GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::~ResourceFrameAllocator()
	{
		if (buffer.resource != WI_NULL_HANDLE)
		{
			((ID3D12Resource*)buffer.resource)->Release();
			buffer.resource = WI_NULL_HANDLE;
		}
	}
	uint8_t* GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::allocate(size_t dataSize, size_t alignment)
	{
		dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));

		if (dataCur + dataSize > dataEnd)
		{
			return nullptr; // failed allocation. TODO: create new heap chunk and allocate from that
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
		lock.lock();

		//dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));

		dataSize = Align(dataSize, alignment);
		assert(dataCur + dataSize <= dataEnd);

		uint8_t* retVal = dataCur;

		dataCur += dataSize;

		lock.unlock();

		return retVal;
	}
	void GraphicsDevice_DX12::UploadBuffer::clear()
	{
		lock.lock();
		dataCur = dataBegin;
		lock.unlock();
	}
	uint64_t GraphicsDevice_DX12::UploadBuffer::calculateOffset(uint8_t* address)
	{
		assert(address >= dataBegin && address < dataEnd);
		return static_cast<uint64_t>(address - dataBegin);
	}

	// Engine functions
	ID3D12GraphicsCommandList* GraphicsDevice_DX12::GetDirectCommandList(GRAPHICSTHREAD threadID) { return static_cast<ID3D12GraphicsCommandList*>(GetFrameResources().commandLists[threadID]); }

	GraphicsDevice_DX12::GraphicsDevice_DX12(wiWindowRegistration::window_type window, bool fullscreen, bool debuglayer)
	{
		DEBUGDEVICE = debuglayer;
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

#if !defined(WINSTORE_SUPPORT)
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

		hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device);
		if (FAILED(hr))
		{
			stringstream ss("");
			ss << "Failed to create the graphics device! ERROR: " << std::hex << hr;
			wiHelper::messageBox(ss.str(), "Error!");
			exit(1);
		}

		// Create command queue
		D3D12_COMMAND_QUEUE_DESC directQueueDesc = {};
		directQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		directQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		directQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		directQueueDesc.NodeMask = 0;
		hr = device->CreateCommandQueue(&directQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&directQueue);

		// Create fences for command queue:
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&frameFence);
		frameFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);



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
		sd.BufferCount = BACKBUFFER_COUNT;
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
		hr = pIDXGIFactory->CreateSwapChainForHwnd(directQueue, window, &sd, &fullscreenDesc, nullptr, &_swapChain);
#else
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
		sd.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

		hr = pIDXGIFactory->CreateSwapChainForCoreWindow(directQueue, reinterpret_cast<IUnknown*>(window), &sd, nullptr, &_swapChain);
#endif

		if (FAILED(hr))
		{
			wiHelper::messageBox("Failed to create a swapchain for the graphics device!", "Error!");
			exit(1);
		}

		hr = _swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapChain);

		SAFE_RELEASE(pIDXGIFactory);



		// Create common descriptor heaps
		RTAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 128);
		DSAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 128);
		ResourceAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16384);
		SamplerAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 64);

		// Create null resources for unbinding:
		{
			D3D12_SAMPLER_DESC sampler_desc = {};
			sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			nullSampler.ptr = SamplerAllocator->allocate();
			device->CreateSampler(&sampler_desc, nullSampler);


			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
			nullCBV.ptr = ResourceAllocator->allocate();
			device->CreateConstantBufferView(&cbv_desc, nullCBV);


			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R32_UINT;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			nullSRV.ptr = ResourceAllocator->allocate();
			device->CreateShaderResourceView(nullptr, &srv_desc, nullSRV);


			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			nullUAV.ptr = ResourceAllocator->allocate();
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, nullUAV);
		}

		// Create resource upload buffer
		bufferUploader = new UploadBuffer(device, 256 * 1024 * 1024);
		textureUploader = new UploadBuffer(device, 256 * 1024 * 1024);


		// Create frame-resident resources:
		for (UINT fr = 0; fr < BACKBUFFER_COUNT; ++fr)
		{
			hr = swapChain->GetBuffer(fr, __uuidof(ID3D12Resource), (void**)&frames[fr].backBuffer);
			frames[fr].backBufferRTV.ptr = RTAllocator->allocate(); 
			device->CreateRenderTargetView(frames[fr].backBuffer, nullptr, frames[fr].backBufferRTV);

			for (int i = 0; i < GRAPHICSTHREAD_COUNT; ++i)
			{
				hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&frames[fr].commandAllocators[i]);
				hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames[fr].commandAllocators[i], nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&frames[fr].commandLists[i]);
				hr = static_cast<ID3D12GraphicsCommandList*>(frames[fr].commandLists[i])->Close();

				frames[fr].ResourceDescriptorsGPU[i] = new FrameResources::DescriptorTableFrameAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024);
				frames[fr].SamplerDescriptorsGPU[i] = new FrameResources::DescriptorTableFrameAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 16);
				frames[fr].resourceBuffer[i] = new FrameResources::ResourceFrameAllocator(device, 1024 * 1024 * 4);
			}
		}


		// Create copy queue:
		{
			D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
			copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
			copyQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			copyQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			copyQueueDesc.NodeMask = 0;
			hr = device->CreateCommandQueue(&copyQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&copyQueue);
			hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, __uuidof(ID3D12CommandAllocator), (void**)&copyAllocator);
			hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, copyAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&copyCommandList);
		
			hr = static_cast<ID3D12GraphicsCommandList*>(copyCommandList)->Close();
			hr = copyAllocator->Reset();
			hr = static_cast<ID3D12GraphicsCommandList*>(copyCommandList)->Reset(copyAllocator, nullptr);

			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&copyFence);
			copyFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
			copyFenceValue = 1;
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




			UINT paramCount = 2 * (SHADERSTAGE_COUNT - 1); // 2: resource,sampler;   5: vs,hs,ds,gs,ps;
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
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

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
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, __uuidof(ID3D12CommandSignature), (void**)&dispatchIndirectCommandSignature);
		assert(SUCCEEDED(hr));

		cmd_desc.ByteStride = sizeof(IndirectDrawArgsInstanced);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = drawInstancedArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, __uuidof(ID3D12CommandSignature), (void**)&drawInstancedIndirectCommandSignature);
		assert(SUCCEEDED(hr));

		cmd_desc.ByteStride = sizeof(IndirectDrawArgsIndexedInstanced);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = drawIndexedInstancedArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, __uuidof(ID3D12CommandSignature), (void**)&drawIndexedInstancedIndirectCommandSignature);
		assert(SUCCEEDED(hr));


		// Set the starting device state:

		hr = GetFrameResources().commandAllocators[GRAPHICSTHREAD_IMMEDIATE]->Reset();
		hr = GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)->Reset(GetFrameResources().commandAllocators[GRAPHICSTHREAD_IMMEDIATE], nullptr);

		ID3D12DescriptorHeap* heaps[] = {
			GetFrameResources().ResourceDescriptorsGPU[GRAPHICSTHREAD_IMMEDIATE]->heap_GPU, GetFrameResources().SamplerDescriptorsGPU[GRAPHICSTHREAD_IMMEDIATE]->heap_GPU
		};
		GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

		GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)->SetGraphicsRootSignature(graphicsRootSig);
		GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)->SetComputeRootSignature(computeRootSig);

		D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptors[] = {
			nullSampler,nullCBV,nullSRV,nullUAV
		};
		GetFrameResources().ResourceDescriptorsGPU[GRAPHICSTHREAD_IMMEDIATE]->reset(device, nullDescriptors);
		GetFrameResources().SamplerDescriptorsGPU[GRAPHICSTHREAD_IMMEDIATE]->reset(device, nullDescriptors);


		D3D12_RECT pRects[8];
		for (UINT i = 0; i < 8; ++i)
		{
			pRects[i].bottom = INT32_MAX;
			pRects[i].left = INT32_MIN;
			pRects[i].right = INT32_MAX;
			pRects[i].top = INT32_MIN;
		}
		GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)->RSSetScissorRects(8, pRects);

		wiBackLog::post("Created GraphicsDevice_DX12");
	}
	GraphicsDevice_DX12::~GraphicsDevice_DX12()
	{
		WaitForGPU();

		SAFE_RELEASE(swapChain);

		for (UINT fr = 0; fr < BACKBUFFER_COUNT; ++fr)
		{
			SAFE_RELEASE(frames[fr].backBuffer);

			for (int i = 0; i < GRAPHICSTHREAD_COUNT; i++)
			{
				SAFE_RELEASE(frames[fr].commandLists[i]);
				SAFE_RELEASE(frames[fr].commandAllocators[i]);
				SAFE_DELETE(frames[fr].ResourceDescriptorsGPU[i]);
				SAFE_DELETE(frames[fr].SamplerDescriptorsGPU[i]);
				SAFE_DELETE(frames[fr].resourceBuffer[i]);
			}
		}

		SAFE_RELEASE(frameFence);
		CloseHandle(frameFenceEvent);

		SAFE_RELEASE(copyQueue);
		SAFE_RELEASE(copyAllocator);
		SAFE_RELEASE(copyCommandList);
		SAFE_RELEASE(copyFence);
		CloseHandle(copyFenceEvent);

		SAFE_RELEASE(graphicsRootSig);
		SAFE_RELEASE(computeRootSig);

		SAFE_RELEASE(dispatchIndirectCommandSignature);
		SAFE_RELEASE(drawInstancedIndirectCommandSignature);
		SAFE_RELEASE(drawIndexedInstancedIndirectCommandSignature);

		SAFE_DELETE(RTAllocator);
		SAFE_DELETE(DSAllocator);
		SAFE_DELETE(ResourceAllocator);
		SAFE_DELETE(SamplerAllocator);

		SAFE_DELETE(bufferUploader);
		SAFE_DELETE(textureUploader);

		SAFE_RELEASE(directQueue);
		SAFE_RELEASE(device);
	}

	void GraphicsDevice_DX12::SetResolution(int width, int height)
	{
		if ((width != SCREENWIDTH || height != SCREENHEIGHT) && width > 0 && height > 0)
		{
			SCREENWIDTH = width;
			SCREENHEIGHT = height;

			WaitForGPU();

			for (UINT fr = 0; fr < BACKBUFFER_COUNT; ++fr)
			{
				SAFE_RELEASE(frames[fr].backBuffer);
			}

			HRESULT hr = swapChain->ResizeBuffers(GetBackBufferCount(), width, height, _ConvertFormat(GetBackBufferFormat()), 0);
			assert(SUCCEEDED(hr));

			for (UINT fr = 0; fr < BACKBUFFER_COUNT; ++fr)
			{
				hr = swapChain->GetBuffer(fr, __uuidof(ID3D12Resource), (void**)&frames[fr].backBuffer);
				assert(SUCCEEDED(hr));
				device->CreateRenderTargetView(frames[fr].backBuffer, nullptr, frames[fr].backBufferRTV);
			}

			RESOLUTIONCHANGED = true;
		}
	}

	Texture2D GraphicsDevice_DX12::GetBackBuffer()
	{
		Texture2D result;
		result.resource = (wiCPUHandle)GetFrameResources().backBuffer;
		GetFrameResources().backBuffer->AddRef();
		return result;
	}

	HRESULT GraphicsDevice_DX12::CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer)
	{
		DestroyBuffer(pBuffer);
		DestroyResource(pBuffer);
		pBuffer->type = GPUResource::BUFFER;
		pBuffer->Register(this);

		HRESULT hr = E_FAIL;

		pBuffer->desc = *pDesc;

		uint32_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
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
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
	
		// Issue main resource creation:
		hr = device->CreateCommittedResource(&heapDesc, heapFlags, &desc, resourceState, 
			nullptr, __uuidof(ID3D12Resource), (void**)&pBuffer->resource);
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		

		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			copyQueueLock.lock();
			{
				uint8_t* dest = bufferUploader->allocate(pDesc->ByteWidth, alignment);
				memcpy(dest, pInitialData->pSysMem, pDesc->ByteWidth);
				static_cast<ID3D12GraphicsCommandList*>(copyCommandList)->CopyBufferRegion(
					(ID3D12Resource*)pBuffer->resource, 0, bufferUploader->resource, bufferUploader->calculateOffset(dest), pDesc->ByteWidth);
			}
			copyQueueLock.unlock();
		}


		// Create resource views if needed
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
			cbv_desc.SizeInBytes = (UINT)alignedSize;
			cbv_desc.BufferLocation = ((ID3D12Resource*)pBuffer->resource)->GetGPUVirtualAddress();

			pBuffer->CBV = ResourceAllocator->allocate();
			device->CreateConstantBufferView(&cbv_desc, ToNativeHandle(pBuffer->CBV));
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

			pBuffer->SRV = ResourceAllocator->allocate();
			device->CreateShaderResourceView((ID3D12Resource*)pBuffer->resource, &srv_desc, ToNativeHandle(pBuffer->SRV));
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

			pBuffer->UAV = ResourceAllocator->allocate();
			device->CreateUnorderedAccessView((ID3D12Resource*)pBuffer->resource, nullptr, &uav_desc, ToNativeHandle(pBuffer->UAV));
		}

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateTexture1D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture1D *pTexture1D)
	{
		DestroyTexture1D(pTexture1D);
		DestroyResource(pTexture1D);
		pTexture1D->type = GPUResource::TEXTURE_1D;
		pTexture1D->Register(this);

		pTexture1D->desc = *pDesc;


		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateTexture2D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture2D *pTexture2D)
	{
		DestroyTexture2D(pTexture2D);
		DestroyResource(pTexture2D);
		pTexture2D->type = GPUResource::TEXTURE_2D;
		pTexture2D->Register(this);

		pTexture2D->desc = *pDesc;


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
		if (pDesc->BindFlags & BIND_DEPTH_STENCIL)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		else
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		}
		if (pDesc->BindFlags & BIND_RENDER_TARGET)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
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
		hr = device->CreateCommittedResource(&heapDesc, heapFlags, &desc, resourceState, 
			useClearValue ? &optimizedClearValue : nullptr, 
			__uuidof(ID3D12Resource), (void**)&pTexture2D->resource);
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		if (pTexture2D->desc.MipLevels == 0)
		{
			pTexture2D->desc.MipLevels = (UINT)log2(std::max(pTexture2D->desc.Width, pTexture2D->desc.Height));
		}


		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			UINT dataCount = pDesc->ArraySize * std::max(1u, pDesc->MipLevels);
			D3D12_SUBRESOURCE_DATA* data = new D3D12_SUBRESOURCE_DATA[dataCount];
			for (UINT slice = 0; slice < dataCount; ++slice)
			{
				data[slice] = _ConvertSubresourceData(pInitialData[slice]);
			}

			UINT FirstSubresource = 0;

			UINT64 RequiredSize = 0;
			device->GetCopyableFootprints(&desc, 0, dataCount, 0, nullptr, nullptr, nullptr, &RequiredSize);


			copyQueueLock.lock();
			{
				uint8_t* dest = textureUploader->allocate(static_cast<size_t>(RequiredSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

				UINT64 dataSize = UpdateSubresources(static_cast<ID3D12GraphicsCommandList*>(copyCommandList), (ID3D12Resource*)pTexture2D->resource,
					textureUploader->resource, textureUploader->calculateOffset(dest), 0, dataCount, data);
			}
			copyQueueLock.unlock();

			SAFE_DELETE_ARRAY(data);
		}


		// Issue creation of additional descriptors for the resource:

		if (pTexture2D->desc.BindFlags & BIND_RENDER_TARGET)
		{
			UINT arraySize = pTexture2D->desc.ArraySize;
			UINT sampleCount = pTexture2D->desc.SampleDesc.Count;
			bool multisampled = sampleCount > 1;

			D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
			renderTargetViewDesc.Format = _ConvertFormat(pTexture2D->desc.Format);
			renderTargetViewDesc.Texture2DArray.MipSlice = 0;

			if (pTexture2D->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
			{
				// TextureCube, TextureCubeArray...
				UINT slices = arraySize / 6;

				renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				renderTargetViewDesc.Texture2DArray.MipSlice = 0;

				if (pTexture2D->independentRTVCubemapFaces)
				{
					// independent faces
					for (UINT i = 0; i < arraySize; ++i)
					{
						renderTargetViewDesc.Texture2DArray.FirstArraySlice = i;
						renderTargetViewDesc.Texture2DArray.ArraySize = 1;

						pTexture2D->additionalRTVs.push_back(RTAllocator->allocate());
						device->CreateRenderTargetView((ID3D12Resource*)pTexture2D->resource, &renderTargetViewDesc, ToNativeHandle(pTexture2D->additionalRTVs.back()));
					}
				}
				else if (pTexture2D->independentRTVArraySlices)
				{
					// independent slices
					for (UINT i = 0; i < slices; ++i)
					{
						renderTargetViewDesc.Texture2DArray.FirstArraySlice = i * 6;
						renderTargetViewDesc.Texture2DArray.ArraySize = 6;

						pTexture2D->additionalRTVs.push_back(RTAllocator->allocate());
						device->CreateRenderTargetView((ID3D12Resource*)pTexture2D->resource, &renderTargetViewDesc, ToNativeHandle(pTexture2D->additionalRTVs.back()));
					}
				}

				{
					// Create full-resource RTVs:
					renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
					renderTargetViewDesc.Texture2DArray.ArraySize = arraySize;

					pTexture2D->RTV = RTAllocator->allocate();
					device->CreateRenderTargetView((ID3D12Resource*)pTexture2D->resource, &renderTargetViewDesc, ToNativeHandle(pTexture2D->RTV));
				}
			}
			else
			{
				// Texture2D, Texture2DArray...
				if (arraySize > 1 && pTexture2D->independentRTVArraySlices)
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

						pTexture2D->additionalRTVs.push_back(RTAllocator->allocate());
						device->CreateRenderTargetView((ID3D12Resource*)pTexture2D->resource, &renderTargetViewDesc, ToNativeHandle(pTexture2D->additionalRTVs.back()));
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

					pTexture2D->RTV = RTAllocator->allocate();
					device->CreateRenderTargetView((ID3D12Resource*)pTexture2D->resource, &renderTargetViewDesc, ToNativeHandle(pTexture2D->RTV));
				}
			}
		}

		if (pTexture2D->desc.BindFlags & BIND_DEPTH_STENCIL)
		{
			UINT arraySize = pTexture2D->desc.ArraySize;
			UINT sampleCount = pTexture2D->desc.SampleDesc.Count;
			bool multisampled = sampleCount > 1;

			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
			depthStencilViewDesc.Texture2DArray.MipSlice = 0;
			depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

			// Try to resolve depth stencil format:
			switch (pTexture2D->desc.Format)
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
				depthStencilViewDesc.Format = _ConvertFormat(pTexture2D->desc.Format);
				break;
			}

			if (pTexture2D->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
			{
				// TextureCube, TextureCubeArray...
				UINT slices = arraySize / 6;

				depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				depthStencilViewDesc.Texture2DArray.MipSlice = 0;

				if (pTexture2D->independentRTVCubemapFaces)
				{
					// independent faces
					for (UINT i = 0; i < arraySize; ++i)
					{
						depthStencilViewDesc.Texture2DArray.FirstArraySlice = i;
						depthStencilViewDesc.Texture2DArray.ArraySize = 1;

						pTexture2D->additionalDSVs.push_back(DSAllocator->allocate());
						device->CreateDepthStencilView((ID3D12Resource*)pTexture2D->resource, &depthStencilViewDesc, ToNativeHandle(pTexture2D->additionalDSVs.back()));
					}
				}
				else if (pTexture2D->independentRTVArraySlices)
				{
					// independent slices
					for (UINT i = 0; i < slices; ++i)
					{
						depthStencilViewDesc.Texture2DArray.FirstArraySlice = i * 6;
						depthStencilViewDesc.Texture2DArray.ArraySize = 6;

						pTexture2D->additionalDSVs.push_back(DSAllocator->allocate());
						device->CreateDepthStencilView((ID3D12Resource*)pTexture2D->resource, &depthStencilViewDesc, ToNativeHandle(pTexture2D->additionalDSVs.back()));
					}
				}

				{
					// Create full-resource DSV:
					depthStencilViewDesc.Texture2DArray.FirstArraySlice = 0;
					depthStencilViewDesc.Texture2DArray.ArraySize = arraySize;

					pTexture2D->DSV = DSAllocator->allocate();
					device->CreateDepthStencilView((ID3D12Resource*)pTexture2D->resource, &depthStencilViewDesc, ToNativeHandle(pTexture2D->DSV));
				}
			}
			else
			{
				// Texture2D, Texture2DArray...
				if (arraySize > 1 && pTexture2D->independentRTVArraySlices)
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

						pTexture2D->additionalDSVs.push_back(DSAllocator->allocate());
						device->CreateDepthStencilView((ID3D12Resource*)pTexture2D->resource, &depthStencilViewDesc, ToNativeHandle(pTexture2D->additionalDSVs.back()));
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

					pTexture2D->DSV = DSAllocator->allocate();
					device->CreateDepthStencilView((ID3D12Resource*)pTexture2D->resource, &depthStencilViewDesc, ToNativeHandle(pTexture2D->DSV));
				}
			}
		}

		if (pTexture2D->desc.BindFlags & BIND_SHADER_RESOURCE)
		{
			UINT arraySize = pTexture2D->desc.ArraySize;
			UINT sampleCount = pTexture2D->desc.SampleDesc.Count;
			bool multisampled = sampleCount > 1;

			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			// Try to resolve shader resource format:
			switch (pTexture2D->desc.Format)
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
				shaderResourceViewDesc.Format = _ConvertFormat(pTexture2D->desc.Format);
				break;
			}

			if (arraySize > 1)
			{
				if (pTexture2D->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
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
				pTexture2D->SRV = ResourceAllocator->allocate();
				device->CreateShaderResourceView((ID3D12Resource*)pTexture2D->resource, &shaderResourceViewDesc, ToNativeHandle(pTexture2D->SRV));


				if (pTexture2D->independentSRVMIPs)
				{
					if (pTexture2D->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
					{
						// independent MIPs
						shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
						shaderResourceViewDesc.TextureCubeArray.First2DArrayFace = 0;
						shaderResourceViewDesc.TextureCubeArray.NumCubes = arraySize / 6;

						for (UINT j = 0; j < pTexture2D->desc.MipLevels; ++j)
						{
							shaderResourceViewDesc.TextureCubeArray.MostDetailedMip = j;
							shaderResourceViewDesc.TextureCubeArray.MipLevels = 1;

							pTexture2D->additionalSRVs.push_back(ResourceAllocator->allocate());
							device->CreateShaderResourceView((ID3D12Resource*)pTexture2D->resource, &shaderResourceViewDesc, ToNativeHandle(pTexture2D->additionalSRVs.back()));
						}
					}
					else
					{
						UINT slices = arraySize;

						shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
						shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
						shaderResourceViewDesc.Texture2DArray.ArraySize = pTexture2D->desc.ArraySize;

						for (UINT j = 0; j < pTexture2D->desc.MipLevels; ++j)
						{
							shaderResourceViewDesc.Texture2DArray.MostDetailedMip = j;
							shaderResourceViewDesc.Texture2DArray.MipLevels = 1;

							pTexture2D->additionalSRVs.push_back(ResourceAllocator->allocate());
							device->CreateShaderResourceView((ID3D12Resource*)pTexture2D->resource, &shaderResourceViewDesc, ToNativeHandle(pTexture2D->additionalSRVs.back()));
						}
					}
				}

				if (pTexture2D->independentSRVArraySlices)
				{
					if (pTexture2D->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
					{
						UINT slices = arraySize / 6;

						// independent cubemaps
						for (UINT i = 0; i < slices; ++i)
						{
							shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
							shaderResourceViewDesc.TextureCubeArray.First2DArrayFace = i * 6;
							shaderResourceViewDesc.TextureCubeArray.NumCubes = 1;
							shaderResourceViewDesc.TextureCubeArray.MostDetailedMip = 0; //from most detailed...
							shaderResourceViewDesc.TextureCubeArray.MipLevels = -1; //...to least detailed

							pTexture2D->additionalSRVs.push_back(ResourceAllocator->allocate());
							device->CreateShaderResourceView((ID3D12Resource*)pTexture2D->resource, &shaderResourceViewDesc, ToNativeHandle(pTexture2D->additionalSRVs.back()));
						}
					}
					else
					{
						UINT slices = arraySize;

						// independent slices
						for (UINT i = 0; i < slices; ++i)
						{
							if (multisampled)
							{
								shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
								shaderResourceViewDesc.Texture2DMSArray.FirstArraySlice = i;
								shaderResourceViewDesc.Texture2DMSArray.ArraySize = 1;
							}
							else
							{
								shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
								shaderResourceViewDesc.Texture2DArray.FirstArraySlice = i;
								shaderResourceViewDesc.Texture2DArray.ArraySize = 1;
								shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0; //from most detailed...
								shaderResourceViewDesc.Texture2DArray.MipLevels = -1; //...to least detailed
							}

							pTexture2D->additionalSRVs.push_back(ResourceAllocator->allocate());
							device->CreateShaderResourceView((ID3D12Resource*)pTexture2D->resource, &shaderResourceViewDesc, ToNativeHandle(pTexture2D->additionalSRVs.back()));
						}
					}
				}
			}
			else
			{
				if (multisampled)
				{
					shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;

					pTexture2D->SRV = ResourceAllocator->allocate();
					device->CreateShaderResourceView((ID3D12Resource*)pTexture2D->resource, &shaderResourceViewDesc, ToNativeHandle(pTexture2D->SRV));
				}
				else
				{
					shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

					if (pTexture2D->independentSRVMIPs)
					{
						// Create subresource SRVs:
						UINT miplevels = pTexture2D->desc.MipLevels;
						for (UINT i = 0; i < miplevels; ++i)
						{
							shaderResourceViewDesc.Texture2D.MostDetailedMip = i;
							shaderResourceViewDesc.Texture2D.MipLevels = 1;

							pTexture2D->additionalSRVs.push_back(ResourceAllocator->allocate());
							device->CreateShaderResourceView((ID3D12Resource*)pTexture2D->resource, &shaderResourceViewDesc, ToNativeHandle(pTexture2D->additionalSRVs.back()));
						}
					}

					{
						// Create full-resource SRV:
						shaderResourceViewDesc.Texture2D.MostDetailedMip = 0; //from most detailed...
						shaderResourceViewDesc.Texture2D.MipLevels = -1; //...to least detailed

						pTexture2D->SRV = ResourceAllocator->allocate();
						device->CreateShaderResourceView((ID3D12Resource*)pTexture2D->resource, &shaderResourceViewDesc, ToNativeHandle(pTexture2D->SRV));
					}
				}
			}
		}


		if (pTexture2D->desc.BindFlags & BIND_UNORDERED_ACCESS)
		{
			if (pTexture2D->desc.ArraySize > 1)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
				ZeroMemory(&uav_desc, sizeof(uav_desc));
				uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uav_desc.Texture2DArray.FirstArraySlice = 0;
				uav_desc.Texture2DArray.ArraySize = pTexture2D->desc.ArraySize;
				uav_desc.Texture2DArray.MipSlice = 0;

				if (pTexture2D->independentUAVMIPs)
				{
					// Create subresource UAVs:
					UINT miplevels = pTexture2D->desc.MipLevels;
					for (UINT i = 0; i < miplevels; ++i)
					{
						uav_desc.Texture2DArray.MipSlice = i;

						pTexture2D->additionalUAVs.push_back(ResourceAllocator->allocate());
						device->CreateUnorderedAccessView((ID3D12Resource*)pTexture2D->resource, nullptr, &uav_desc, ToNativeHandle(pTexture2D->additionalUAVs.back()));
					}
				}

				{
					// Create main resource UAV:
					uav_desc.Texture2D.MipSlice = 0;

					pTexture2D->UAV = ResourceAllocator->allocate();
					device->CreateUnorderedAccessView((ID3D12Resource*)pTexture2D->resource, nullptr, &uav_desc, ToNativeHandle(pTexture2D->UAV));
				}
			}
			else
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
				uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

				if (pTexture2D->independentUAVMIPs)
				{
					// Create subresource UAVs:
					UINT miplevels = pTexture2D->desc.MipLevels;
					for (UINT i = 0; i < miplevels; ++i)
					{
						uav_desc.Texture2D.MipSlice = i;


						pTexture2D->additionalUAVs.push_back(ResourceAllocator->allocate());
						device->CreateUnorderedAccessView((ID3D12Resource*)pTexture2D->resource, nullptr, &uav_desc, ToNativeHandle(pTexture2D->additionalUAVs.back()));
					}
				}

				{
					// Create main resource UAV:
					uav_desc.Texture2D.MipSlice = 0;

					pTexture2D->UAV = ResourceAllocator->allocate();
					device->CreateUnorderedAccessView((ID3D12Resource*)pTexture2D->resource, nullptr, &uav_desc, ToNativeHandle(pTexture2D->UAV));
				}
			}
		}

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateTexture3D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture3D *pTexture3D)
	{
		DestroyTexture3D(pTexture3D);
		DestroyResource(pTexture3D);
		pTexture3D->type = GPUResource::TEXTURE_3D;
		pTexture3D->Register(this);

		pTexture3D->desc = *pDesc;


		HRESULT hr = E_FAIL;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements, const ShaderByteCode* shaderCode, VertexLayout *pInputLayout)
	{
		DestroyInputLayout(pInputLayout);
		pInputLayout->Register(this);

		pInputLayout->desc.reserve((size_t)NumElements);
		for (UINT i = 0; i < NumElements; ++i)
		{
			pInputLayout->desc.push_back(pInputElementDescs[i]);
		}

		return S_OK;
	}
	HRESULT GraphicsDevice_DX12::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, VertexShader *pVertexShader)
	{
		DestroyVertexShader(pVertexShader);
		pVertexShader->Register(this);

		pVertexShader->code.data = new BYTE[BytecodeLength];
		memcpy(pVertexShader->code.data, pShaderBytecode, BytecodeLength);
		pVertexShader->code.size = BytecodeLength;

		return (pVertexShader->code.data != nullptr && pVertexShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, PixelShader *pPixelShader)
	{
		DestroyPixelShader(pPixelShader);
		pPixelShader->Register(this);

		pPixelShader->code.data = new BYTE[BytecodeLength];
		memcpy(pPixelShader->code.data, pShaderBytecode, BytecodeLength);
		pPixelShader->code.size = BytecodeLength;

		return (pPixelShader->code.data != nullptr && pPixelShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, GeometryShader *pGeometryShader)
	{
		DestroyGeometryShader(pGeometryShader);
		pGeometryShader->Register(this);

		pGeometryShader->code.data = new BYTE[BytecodeLength];
		memcpy(pGeometryShader->code.data, pShaderBytecode, BytecodeLength);
		pGeometryShader->code.size = BytecodeLength;

		return (pGeometryShader->code.data != nullptr && pGeometryShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, HullShader *pHullShader)
	{
		DestroyHullShader(pHullShader);
		pHullShader->Register(this);

		pHullShader->code.data = new BYTE[BytecodeLength];
		memcpy(pHullShader->code.data, pShaderBytecode, BytecodeLength);
		pHullShader->code.size = BytecodeLength;

		return (pHullShader->code.data != nullptr && pHullShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, DomainShader *pDomainShader)
	{
		DestroyDomainShader(pDomainShader);
		pDomainShader->Register(this);

		pDomainShader->code.data = new BYTE[BytecodeLength];
		memcpy(pDomainShader->code.data, pShaderBytecode, BytecodeLength);
		pDomainShader->code.size = BytecodeLength;

		return (pDomainShader->code.data != nullptr && pDomainShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ComputeShader *pComputeShader)
	{
		DestroyComputeShader(pComputeShader);
		pComputeShader->Register(this);

		pComputeShader->code.data = new BYTE[BytecodeLength];
		memcpy(pComputeShader->code.data, pShaderBytecode, BytecodeLength);
		pComputeShader->code.size = BytecodeLength;

		return (pComputeShader->code.data != nullptr && pComputeShader->code.size > 0 ? S_OK : E_FAIL);
	}
	HRESULT GraphicsDevice_DX12::CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState)
	{
		DestroyBlendState(pBlendState);
		pBlendState->Register(this);

		pBlendState->desc = *pBlendStateDesc;
		return S_OK;
	}
	HRESULT GraphicsDevice_DX12::CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState)
	{
		DestroyDepthStencilState(pDepthStencilState);
		pDepthStencilState->Register(this);

		pDepthStencilState->desc = *pDepthStencilStateDesc;
		return S_OK;
	}
	HRESULT GraphicsDevice_DX12::CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState)
	{
		DestroyRasterizerState(pRasterizerState);
		pRasterizerState->Register(this);

		pRasterizerState->desc = *pRasterizerStateDesc;
		return S_OK;
	}
	HRESULT GraphicsDevice_DX12::CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState)
	{
		DestroySamplerState(pSamplerState);
		pSamplerState->Register(this);

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

		pSamplerState->resource = SamplerAllocator->allocate();
		device->CreateSampler(&desc, ToNativeHandle(pSamplerState->resource));

		return S_OK;
	}
	HRESULT GraphicsDevice_DX12::CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery)
	{
		// TODO!
		//DestroyQuery(pQuery);
		//pQuery->Register(this);

		HRESULT hr = E_FAIL;

		pQuery->desc = *pDesc;

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateGraphicsPSO(const GraphicsPSODesc* pDesc, GraphicsPSO* pso)
	{
		DestroyGraphicsPSO(pso);
		pso->Register(this);

		pso->desc = *pDesc;

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
				if (elements[i].AlignedByteOffset == VertexLayoutDesc::APPEND_ALIGNED_ELEMENT)
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

		switch (pDesc->pt)
		{
		case POINTLIST:
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			break;
		case LINELIST:
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

		desc.pRootSignature = graphicsRootSig;

		HRESULT hr = device->CreateGraphicsPipelineState(&desc, __uuidof(ID3D12PipelineState), (void**)&pso->pipeline);
		assert(SUCCEEDED(hr));

		SAFE_DELETE_ARRAY(elements);

		return hr;
	}
	HRESULT GraphicsDevice_DX12::CreateComputePSO(const ComputePSODesc* pDesc, ComputePSO* pso)
	{
		DestroyComputePSO(pso);
		pso->Register(this);

		pso->desc = *pDesc;

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};

		if (pDesc->cs != nullptr)
		{
			desc.CS.pShaderBytecode = pDesc->cs->code.data;
			desc.CS.BytecodeLength = pDesc->cs->code.size;
		}

		desc.pRootSignature = computeRootSig;

		HRESULT hr = device->CreateComputePipelineState(&desc, __uuidof(ID3D12PipelineState), (void**)&pso->pipeline);
		assert(SUCCEEDED(hr));

		return hr;
	}


	void GraphicsDevice_DX12::DestroyResource(GPUResource* pResource)
	{
		if (pResource->resource != WI_NULL_HANDLE)
		{
			((ID3D12Resource*)pResource->resource)->Release();
			pResource->resource = WI_NULL_HANDLE;
		}

		ResourceAllocator->free(pResource->SRV);
		pResource->SRV = WI_NULL_HANDLE;
		for (auto& x : pResource->additionalSRVs)
		{
			ResourceAllocator->free(x);
		}
		pResource->additionalSRVs.clear();

		ResourceAllocator->free(pResource->UAV);
		pResource->UAV = WI_NULL_HANDLE;
		for (auto& x : pResource->additionalUAVs)
		{
			ResourceAllocator->free(x);
		}
		pResource->additionalUAVs.clear();
	}
	void GraphicsDevice_DX12::DestroyBuffer(GPUBuffer *pBuffer)
	{
		ResourceAllocator->free(pBuffer->CBV);
		pBuffer->CBV = WI_NULL_HANDLE;
	}
	void GraphicsDevice_DX12::DestroyTexture1D(Texture1D *pTexture1D)
	{
		RTAllocator->free(pTexture1D->RTV);
		pTexture1D->RTV = WI_NULL_HANDLE;
		for (auto& x : pTexture1D->additionalRTVs)
		{
			RTAllocator->free(x);
		}
		pTexture1D->additionalRTVs.clear();
	}
	void GraphicsDevice_DX12::DestroyTexture2D(Texture2D *pTexture2D)
	{
		RTAllocator->free(pTexture2D->RTV);
		pTexture2D->RTV = WI_NULL_HANDLE;
		for (auto& x : pTexture2D->additionalRTVs)
		{
			RTAllocator->free(x);
		}
		pTexture2D->additionalRTVs.clear();

		DSAllocator->free(pTexture2D->DSV);
		pTexture2D->DSV = WI_NULL_HANDLE;
		for (auto& x : pTexture2D->additionalDSVs)
		{
			DSAllocator->free(x);
		}
		pTexture2D->additionalDSVs.clear();
	}
	void GraphicsDevice_DX12::DestroyTexture3D(Texture3D *pTexture3D)
	{
		RTAllocator->free(pTexture3D->RTV);
		pTexture3D->RTV = WI_NULL_HANDLE;
		for (auto& x : pTexture3D->additionalRTVs)
		{
			RTAllocator->free(x);
		}
		pTexture3D->additionalRTVs.clear();
	}
	void GraphicsDevice_DX12::DestroyInputLayout(VertexLayout *pInputLayout)
	{

	}
	void GraphicsDevice_DX12::DestroyVertexShader(VertexShader *pVertexShader)
	{

	}
	void GraphicsDevice_DX12::DestroyPixelShader(PixelShader *pPixelShader)
	{

	}
	void GraphicsDevice_DX12::DestroyGeometryShader(GeometryShader *pGeometryShader)
	{

	}
	void GraphicsDevice_DX12::DestroyHullShader(HullShader *pHullShader)
	{

	}
	void GraphicsDevice_DX12::DestroyDomainShader(DomainShader *pDomainShader)
	{

	}
	void GraphicsDevice_DX12::DestroyComputeShader(ComputeShader *pComputeShader)
	{

	}
	void GraphicsDevice_DX12::DestroyBlendState(BlendState *pBlendState)
	{

	}
	void GraphicsDevice_DX12::DestroyDepthStencilState(DepthStencilState *pDepthStencilState)
	{

	}
	void GraphicsDevice_DX12::DestroyRasterizerState(RasterizerState *pRasterizerState)
	{

	}
	void GraphicsDevice_DX12::DestroySamplerState(Sampler *pSamplerState)
	{
		SamplerAllocator->free(pSamplerState->resource);
	}
	void GraphicsDevice_DX12::DestroyQuery(GPUQuery *pQuery)
	{

	}
	void GraphicsDevice_DX12::DestroyGraphicsPSO(GraphicsPSO* pso)
	{
		if (pso->pipeline != WI_NULL_HANDLE)
		{
			((ID3D12PipelineState*)pso->pipeline)->Release();
			pso->pipeline = WI_NULL_HANDLE;
		}
	}
	void GraphicsDevice_DX12::DestroyComputePSO(ComputePSO* pso)
	{
		if (pso->pipeline != WI_NULL_HANDLE)
		{
			((ID3D12PipelineState*)pso->pipeline)->Release();
			pso->pipeline = WI_NULL_HANDLE;
		}
	}


	void GraphicsDevice_DX12::SetName(GPUResource* pResource, const std::string& name)
	{
		((ID3D12Resource*)pResource->resource)->SetName(wstring(name.begin(), name.end()).c_str());
	}


	void GraphicsDevice_DX12::PresentBegin()
	{
		// Sync up copy queue:
		copyQueueLock.lock();
		{
			static_cast<ID3D12GraphicsCommandList*>(copyCommandList)->Close();
			copyQueue->ExecuteCommandLists(1, &copyCommandList);

			// Signal and increment the fence value.
			UINT64 fenceToWaitFor = copyFenceValue;
			HRESULT result = copyQueue->Signal(copyFence, fenceToWaitFor);
			copyFenceValue++;

			// Wait until the GPU is done copying.
			if (copyFence->GetCompletedValue() < fenceToWaitFor)
			{
				result = copyFence->SetEventOnCompletion(fenceToWaitFor, copyFenceEvent);
				WaitForSingleObject(copyFenceEvent, INFINITE);
			}

			result = copyAllocator->Reset();
			result = static_cast<ID3D12GraphicsCommandList*>(copyCommandList)->Reset(copyAllocator, nullptr);

			bufferUploader->clear();
			textureUploader->clear();
		}
		copyQueueLock.unlock();


		BindViewports(1, &viewPort, GRAPHICSTHREAD_IMMEDIATE);


		// Record commands in the command list now.
		// Start by setting the resource barrier.
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = GetFrameResources().backBuffer;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)->ResourceBarrier(1, &barrier);


		// Set the back buffer as the render target.
		GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)->OMSetRenderTargets(1, &GetFrameResources().backBufferRTV, FALSE, NULL);


		// Then set the color to clear the window to.
		float color[4];
		color[0] = 0.0;
		color[1] = 0.0;
		color[2] = 0.0;
		color[3] = 1.0;
		GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)->ClearRenderTargetView(GetFrameResources().backBufferRTV, color, 0, NULL);


	}
	void GraphicsDevice_DX12::PresentEnd()
	{
		HRESULT result;

		// Indicate that the back buffer will now be used to present.
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = GetFrameResources().backBuffer;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)->ResourceBarrier(1, &barrier);

		// Close the list of commands.
		result = GetDirectCommandList(GRAPHICSTHREAD_IMMEDIATE)->Close();

		// Execute the list of commands.
		directQueue->ExecuteCommandLists(1, GetFrameResources().commandLists);


		swapChain->Present(VSYNC, 0);



		// This acts as a barrier, following this we will be using the next frame's resources when calling GetFrameResources()!
		FRAMECOUNT++;



		// Record the latest processed framecount in the fence
		result = directQueue->Signal(frameFence, FRAMECOUNT);

		// Determine the last frame that we should not wait on:
		const uint64_t lastFrameToAllowLatency = std::max(uint64_t(BACKBUFFER_COUNT - 1u), FRAMECOUNT) - (BACKBUFFER_COUNT - 1);

		// Wait if too many frames are being incomplete:
		if (frameFence->GetCompletedValue() < lastFrameToAllowLatency)
		{
			result = frameFence->SetEventOnCompletion(lastFrameToAllowLatency, frameFenceEvent);
			WaitForSingleObject(frameFenceEvent, INFINITE);
		}



		for (int threadID = 0; threadID < GRAPHICSTHREAD_IMMEDIATE + 1; ++threadID) // todo: all command lists
		{
			// Reset (re-use) the memory associated command allocator.
			HRESULT hr = result = GetFrameResources().commandAllocators[threadID]->Reset();
			assert(SUCCEEDED(hr));
			// Reset the command list, use empty pipeline state for now since there are no shaders and we are just clearing the screen.
			hr = static_cast<ID3D12GraphicsCommandList*>(GetFrameResources().commandLists[threadID])->Reset(GetFrameResources().commandAllocators[threadID], nullptr);
			assert(SUCCEEDED(hr));


			ID3D12DescriptorHeap* heaps[] = {
				GetFrameResources().ResourceDescriptorsGPU[threadID]->heap_GPU, GetFrameResources().SamplerDescriptorsGPU[threadID]->heap_GPU
			};
			GetDirectCommandList((GRAPHICSTHREAD)threadID)->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

			GetDirectCommandList((GRAPHICSTHREAD)threadID)->SetGraphicsRootSignature(graphicsRootSig);
			GetDirectCommandList((GRAPHICSTHREAD)threadID)->SetComputeRootSignature(computeRootSig);

			D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptors[] = {
				nullSampler,nullCBV,nullSRV,nullUAV
			};
			GetFrameResources().ResourceDescriptorsGPU[threadID]->reset(device, nullDescriptors);
			GetFrameResources().SamplerDescriptorsGPU[threadID]->reset(device, nullDescriptors);
			GetFrameResources().resourceBuffer[threadID]->clear();

			D3D12_RECT pRects[8];
			for (UINT i = 0; i < 8; ++i)
			{
				pRects[i].bottom = INT32_MAX;
				pRects[i].left = INT32_MIN;
				pRects[i].right = INT32_MAX;
				pRects[i].top = INT32_MIN;
			}
			GetDirectCommandList((GRAPHICSTHREAD)threadID)->RSSetScissorRects(8, pRects);
		}

		memset(prev_pt, 0, sizeof(prev_pt));

		RESOLUTIONCHANGED = false;
	}

	void GraphicsDevice_DX12::CreateCommandLists()
	{

	}
	void GraphicsDevice_DX12::ExecuteCommandLists()
	{
		directQueue->ExecuteCommandLists(GRAPHICSTHREAD_COUNT - 1, &GetFrameResources().commandLists[1]);
	}
	void GraphicsDevice_DX12::FinishCommandList(GRAPHICSTHREAD threadID)
	{
		if (threadID == GRAPHICSTHREAD_IMMEDIATE)
			return;
		HRESULT hr = GetDirectCommandList(threadID)->Close();
		assert(SUCCEEDED(hr));
	}

	void GraphicsDevice_DX12::WaitForGPU()
	{
		if (frameFence->GetCompletedValue() < FRAMECOUNT)
		{
			HRESULT result = frameFence->SetEventOnCompletion(FRAMECOUNT, frameFenceEvent);
			WaitForSingleObject(frameFenceEvent, INFINITE);
		}
	}


	void GraphicsDevice_DX12::BindScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID) {
		assert(rects != nullptr);
		assert(numRects <= 8);
		D3D12_RECT pRects[8];
		for(UINT i = 0; i < numRects; ++i) {
			pRects[i].bottom = rects[i].bottom;
			pRects[i].left = rects[i].left;
			pRects[i].right = rects[i].right;
			pRects[i].top = rects[i].top;
		}
		GetDirectCommandList(threadID)->RSSetScissorRects(numRects, pRects);
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
		GetDirectCommandList(threadID)->RSSetViewports(NumViewports, d3dViewPorts);
	}
	void GraphicsDevice_DX12::BindRenderTargets(const UINT NumViews, const Texture2D* const *ppRenderTargets, const Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE descriptors[8] = {};
		for (UINT i = 0; i < NumViews; ++i)
		{
			if (ppRenderTargets[i] != nullptr)
			{
				if (arrayIndex < 0 || ppRenderTargets[i]->additionalRTVs.empty())
				{
					descriptors[i] = ToNativeHandle(ppRenderTargets[i]->RTV);
				}
				else
				{
					assert(ppRenderTargets[i]->additionalRTVs.size() > static_cast<size_t>(arrayIndex) && "Invalid rendertarget arrayIndex!");
					descriptors[i] = ToNativeHandle(ppRenderTargets[i]->additionalRTVs[arrayIndex]);
				}
			}
		}

		D3D12_CPU_DESCRIPTOR_HANDLE* DSV = nullptr;
		if (depthStencilTexture != nullptr)
		{
			if (arrayIndex < 0 || depthStencilTexture->additionalDSVs.empty())
			{
				DSV = &ToNativeHandle(depthStencilTexture->DSV);
			}
			else
			{
				assert(depthStencilTexture->additionalDSVs.size() > static_cast<size_t>(arrayIndex) && "Invalid depthstencil arrayIndex!");
				DSV = &ToNativeHandle(depthStencilTexture->additionalDSVs[arrayIndex]);
			}
		}

		GetDirectCommandList(threadID)->OMSetRenderTargets(NumViews, descriptors, FALSE, DSV);
	}
	void GraphicsDevice_DX12::ClearRenderTarget(const Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex)
	{
		if (pTexture != nullptr)
		{
			if (arrayIndex < 0)
			{
				GetDirectCommandList(threadID)->ClearRenderTargetView(ToNativeHandle(pTexture->RTV), ColorRGBA, 0, nullptr);
			}
			else
			{
				GetDirectCommandList(threadID)->ClearRenderTargetView(ToNativeHandle(pTexture->additionalRTVs[arrayIndex]), ColorRGBA, 0, nullptr);
			}
		}
	}
	void GraphicsDevice_DX12::ClearDepthStencil(const Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		if (pTexture != nullptr)
		{
			UINT _flags = 0;
			if (ClearFlags & CLEAR_DEPTH)
				_flags |= D3D12_CLEAR_FLAG_DEPTH;
			if (ClearFlags & CLEAR_STENCIL)
				_flags |= D3D12_CLEAR_FLAG_STENCIL;

			if (arrayIndex < 0)
			{
				GetDirectCommandList(threadID)->ClearDepthStencilView(ToNativeHandle(pTexture->DSV), (D3D12_CLEAR_FLAGS)_flags, Depth, Stencil, 0, nullptr);
			}
			else
			{
				GetDirectCommandList(threadID)->ClearDepthStencilView(ToNativeHandle(pTexture->additionalDSVs[arrayIndex]), (D3D12_CLEAR_FLAGS)_flags, Depth, Stencil, 0, nullptr);
			}
		}
	}
	void GraphicsDevice_DX12::BindResource(SHADERSTAGE stage, const GPUResource* resource, UINT slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		assert(slot < GPU_RESOURCE_HEAP_SRV_COUNT);

		if (resource != nullptr && resource->resource != WI_NULL_HANDLE)
		{
			if (arrayIndex < 0)
			{
				if (resource->SRV != WI_NULL_HANDLE)
				{
					GetFrameResources().ResourceDescriptorsGPU[threadID]->update(stage, GPU_RESOURCE_HEAP_CBV_COUNT + slot,
						resource->SRV, device, GetDirectCommandList(threadID));
				}
			}
			else
			{
				assert(resource->additionalSRVs.size() > static_cast<size_t>(arrayIndex) && "Invalid arrayIndex!");
				GetFrameResources().ResourceDescriptorsGPU[threadID]->update(stage, GPU_RESOURCE_HEAP_CBV_COUNT + slot,
					resource->additionalSRVs[arrayIndex], device, GetDirectCommandList(threadID));
			}
		}
	}
	void GraphicsDevice_DX12::BindResources(SHADERSTAGE stage, const GPUResource *const* resources, UINT slot, UINT count, GRAPHICSTHREAD threadID)
	{
		if (resources != nullptr)
		{
			for (UINT i = 0; i < count; ++i)
			{
				BindResource(stage, resources[i], slot + i, threadID, -1);
			}
		}
	}
	void GraphicsDevice_DX12::BindUAV(SHADERSTAGE stage, const GPUResource* resource, UINT slot, GRAPHICSTHREAD threadID, int arrayIndex)
	{
		assert(slot < GPU_RESOURCE_HEAP_UAV_COUNT);

		if (resource != nullptr && resource->resource != WI_NULL_HANDLE)
		{
			if (arrayIndex < 0)
			{
				if (resource->UAV != WI_NULL_HANDLE)
				{
					GetFrameResources().ResourceDescriptorsGPU[threadID]->update(stage, GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + slot,
						resource->UAV, device, GetDirectCommandList(threadID));
				}
			}
			else
			{
				assert(resource->additionalUAVs.size() > static_cast<size_t>(arrayIndex) && "Invalid arrayIndex!");
				GetFrameResources().ResourceDescriptorsGPU[threadID]->update(stage, GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + slot,
					resource->additionalUAVs[arrayIndex], device, GetDirectCommandList(threadID));
			}
		}
	}
	void GraphicsDevice_DX12::BindUAVs(SHADERSTAGE stage, const GPUResource *const* resources, UINT slot, UINT count, GRAPHICSTHREAD threadID)
	{
		if (resources != nullptr)
		{
			for (UINT i = 0; i < count; ++i)
			{
				BindUAV(stage, resources[i], slot + i, threadID, -1);
			}
		}
	}
	void GraphicsDevice_DX12::UnbindResources(UINT slot, UINT num, GRAPHICSTHREAD threadID)
	{
		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			for (UINT i = 0; i < num; ++i)
			{
				GetFrameResources().ResourceDescriptorsGPU[threadID]->update((SHADERSTAGE)stage, GPU_RESOURCE_HEAP_CBV_COUNT + slot + i,
					nullSRV.ptr, device, GetDirectCommandList(threadID));
			}
		}
	}
	void GraphicsDevice_DX12::UnbindUAVs(UINT slot, UINT num, GRAPHICSTHREAD threadID)
	{
		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			for (UINT i = 0; i < num; ++i)
			{
				GetFrameResources().ResourceDescriptorsGPU[threadID]->update(CS, GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + slot + i,
					nullUAV.ptr, device, GetDirectCommandList(threadID));
			}
		}
	}
	void GraphicsDevice_DX12::BindSampler(SHADERSTAGE stage, const Sampler* sampler, UINT slot, GRAPHICSTHREAD threadID)
	{
		assert(slot < GPU_SAMPLER_HEAP_COUNT);

		if (sampler != nullptr && sampler->resource != WI_NULL_HANDLE)
		{
			GetFrameResources().SamplerDescriptorsGPU[threadID]->update(stage, slot,
				sampler->resource, device, GetDirectCommandList(threadID));
		}
	}
	void GraphicsDevice_DX12::BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, UINT slot, GRAPHICSTHREAD threadID)
	{
		assert(slot < GPU_RESOURCE_HEAP_CBV_COUNT);

		if (buffer != nullptr && buffer->CBV != WI_NULL_HANDLE)
		{
			GetFrameResources().ResourceDescriptorsGPU[threadID]->update(stage, slot,
				buffer->CBV, device, GetDirectCommandList(threadID));
		}
	}
	void GraphicsDevice_DX12::BindVertexBuffers(const GPUBuffer *const* vertexBuffers, UINT slot, UINT count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID)
	{
		assert(count <= 8);
		D3D12_VERTEX_BUFFER_VIEW res[8] = { 0 };
		for (UINT i = 0; i < count; ++i)
		{
			if (vertexBuffers[i] != nullptr)
			{
				res[i].BufferLocation = ((ID3D12Resource*)vertexBuffers[i]->resource)->GetGPUVirtualAddress();
				res[i].SizeInBytes = vertexBuffers[i]->desc.ByteWidth;
				if (offsets != nullptr)
				{
					res[i].BufferLocation += (D3D12_GPU_VIRTUAL_ADDRESS)offsets[i];
					res[i].SizeInBytes -= offsets[i];
				}
				res[i].StrideInBytes = strides[i];
			}
		}
		GetDirectCommandList(threadID)->IASetVertexBuffers(static_cast<UINT>(slot), static_cast<UINT>(count), res);
	}
	void GraphicsDevice_DX12::BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID)
	{
		D3D12_INDEX_BUFFER_VIEW res = {};
		if (indexBuffer != nullptr)
		{
			res.BufferLocation = ((ID3D12Resource*)indexBuffer->resource)->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
			res.Format = (format == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
			res.SizeInBytes = indexBuffer->desc.ByteWidth;
		}
		GetDirectCommandList(threadID)->IASetIndexBuffer(&res);
	}
	void GraphicsDevice_DX12::BindStencilRef(UINT value, GRAPHICSTHREAD threadID)
	{
		GetDirectCommandList(threadID)->OMSetStencilRef(value);
	}
	void GraphicsDevice_DX12::BindBlendFactor(float r, float g, float b, float a, GRAPHICSTHREAD threadID)
	{
		const float blendFactor[4] = { r, g, b, a };
		GetDirectCommandList(threadID)->OMSetBlendFactor(blendFactor);
	}
	void GraphicsDevice_DX12::BindGraphicsPSO(const GraphicsPSO* pso, GRAPHICSTHREAD threadID)
	{
		GetDirectCommandList(threadID)->SetPipelineState((ID3D12PipelineState*)pso->pipeline);

		if (prev_pt[threadID] != pso->desc.pt)
		{
			prev_pt[threadID] = pso->desc.pt;

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
			case PATCHLIST:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
				break;
			default:
				break;
			};
			GetDirectCommandList(threadID)->IASetPrimitiveTopology(d3dType);
		}
	}
	void GraphicsDevice_DX12::BindComputePSO(const ComputePSO* pso, GRAPHICSTHREAD threadID)
	{
		GetDirectCommandList(threadID)->SetPipelineState((ID3D12PipelineState*)pso->pipeline);
	}
	void GraphicsDevice_DX12::Draw(UINT vertexCount, UINT startVertexLocation, GRAPHICSTHREAD threadID)
	{
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetFrameResources().SamplerDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetDirectCommandList(threadID)->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawIndexed(UINT indexCount, UINT startIndexLocation, UINT baseVertexLocation, GRAPHICSTHREAD threadID)
	{
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetFrameResources().SamplerDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetDirectCommandList(threadID)->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawInstanced(UINT vertexCount, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID)
	{
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetFrameResources().SamplerDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetDirectCommandList(threadID)->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawIndexedInstanced(UINT indexCount, UINT instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID)
	{
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetFrameResources().SamplerDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetDirectCommandList(threadID)->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawInstancedIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetFrameResources().SamplerDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetDirectCommandList(threadID)->ExecuteIndirect(drawInstancedIndirectCommandSignature, 1, (ID3D12Resource*)args->resource, args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DrawIndexedInstancedIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetFrameResources().SamplerDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetDirectCommandList(threadID)->ExecuteIndirect(drawIndexedInstancedIndirectCommandSignature, 1, (ID3D12Resource*)args->resource, args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID)
	{
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetFrameResources().SamplerDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetDirectCommandList(threadID)->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_DX12::DispatchIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID)
	{
		GetFrameResources().ResourceDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetFrameResources().SamplerDescriptorsGPU[threadID]->validate(device, GetDirectCommandList(threadID));
		GetDirectCommandList(threadID)->ExecuteIndirect(dispatchIndirectCommandSignature, 1, (ID3D12Resource*)args->resource, args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::CopyTexture2D(const Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID)
	{
		GetDirectCommandList(threadID)->CopyResource((ID3D12Resource*)pDst->resource, (ID3D12Resource*)pSrc->resource);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = (ID3D12Resource*)pDst->resource;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		GetDirectCommandList(threadID)->ResourceBarrier(1, &barrier);
	}
	void GraphicsDevice_DX12::CopyTexture2D_Region(const Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, const Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID)
	{
		D3D12_RESOURCE_DESC dst_desc = ((ID3D12Resource*)pDst->resource)->GetDesc();
		D3D12_RESOURCE_DESC src_desc = ((ID3D12Resource*)pSrc->resource)->GetDesc();

		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = (ID3D12Resource*)pDst->resource;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = D3D12CalcSubresource(dstMip, 0, 0, dst_desc.MipLevels, dst_desc.DepthOrArraySize);

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = (ID3D12Resource*)pSrc->resource;
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = D3D12CalcSubresource(srcMip, 0, 0, src_desc.MipLevels, src_desc.DepthOrArraySize);


		GetDirectCommandList(threadID)->CopyTextureRegion(&dst, dstX, dstY, 0, &src, nullptr);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = (ID3D12Resource*)pDst->resource;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		GetDirectCommandList(threadID)->ResourceBarrier(1, &barrier);
	}
	void GraphicsDevice_DX12::MSAAResolve(const Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::UpdateBuffer(const GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize)
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

		size_t alignment = buffer->desc.BindFlags & BIND_CONSTANT_BUFFER ? D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = (ID3D12Resource*)buffer->resource;
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
		GetDirectCommandList(threadID)->ResourceBarrier(1, &barrier);

		uint8_t* dest = GetFrameResources().resourceBuffer[threadID]->allocate(dataSize, alignment);
		memcpy(dest, data, dataSize);
		GetDirectCommandList(threadID)->CopyBufferRegion(
			(ID3D12Resource*)buffer->resource, 0,
			(ID3D12Resource*)GetFrameResources().resourceBuffer[threadID]->buffer.resource, GetFrameResources().resourceBuffer[threadID]->calculateOffset(dest),
			dataSize
		);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
		GetDirectCommandList(threadID)->ResourceBarrier(1, &barrier);

	}
	bool GraphicsDevice_DX12::DownloadResource(const GPUResource* resourceToDownload, const GPUResource* resourceDest, void* dataDest, GRAPHICSTHREAD threadID)
	{
		return false;
	}

	void GraphicsDevice_DX12::QueryBegin(const GPUQuery *query, GRAPHICSTHREAD threadID)
	{
	}
	void GraphicsDevice_DX12::QueryEnd(const GPUQuery *query, GRAPHICSTHREAD threadID)
	{
	}
	bool GraphicsDevice_DX12::QueryRead(const GPUQuery* query, GPUQueryResult* result)
	{
		return true;
	}

	void GraphicsDevice_DX12::UAVBarrier(const GPUResource *const* uavs, UINT NumBarriers, GRAPHICSTHREAD threadID)
	{
		D3D12_RESOURCE_BARRIER barriers[8];
		for (UINT i = 0; i < NumBarriers; ++i)
		{
			barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barriers[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barriers[i].UAV.pResource = (uavs == nullptr ? nullptr : (ID3D12Resource*)uavs[i]->resource);
		}
		GetDirectCommandList(threadID)->ResourceBarrier(NumBarriers, barriers);
	}
	void GraphicsDevice_DX12::TransitionBarrier(const GPUResource *const* resources, UINT NumBarriers, RESOURCE_STATES stateBefore, RESOURCE_STATES stateAfter, GRAPHICSTHREAD threadID)
	{
		if (resources != nullptr)
		{
			D3D12_RESOURCE_BARRIER barriers[8];
			for (UINT i = 0; i < NumBarriers; ++i)
			{
				barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barriers[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barriers[i].Transition.pResource = (ID3D12Resource*)resources[i]->resource;
				barriers[i].Transition.StateBefore = _ConvertResourceStates(stateBefore);
				barriers[i].Transition.StateAfter = _ConvertResourceStates(stateAfter);
				barriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			}
			GetDirectCommandList(threadID)->ResourceBarrier(NumBarriers, barriers);
		}
	}

	GraphicsDevice::GPUAllocation GraphicsDevice_DX12::AllocateGPU(size_t dataSize, GRAPHICSTHREAD threadID)
	{
		// This case allocates a CPU write access and GPU read access memory from the temporary buffer
		// The application can write into this, but better to not read from it

		FrameResources::ResourceFrameAllocator& allocator = *GetFrameResources().resourceBuffer[threadID];
		GPUAllocation result;

		if (dataSize == 0)
		{
			return result;
		}

		uint8_t* dest = allocator.allocate(dataSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

		assert(dest != nullptr); // todo: this needs to be handled as well

		result.buffer = &allocator.buffer;
		result.offset = (UINT)allocator.calculateOffset(dest);
		result.data = (void*)dest;
		return result;
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
