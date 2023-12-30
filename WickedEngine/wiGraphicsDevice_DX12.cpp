#include "wiGraphicsDevice_DX12.h"

#ifdef WICKEDENGINE_BUILD_DX12
#include "wiHelper.h"
#include "wiBacklog.h"
#include "wiTimer.h"
#include "wiUnorderedSet.h"

#ifndef PLATFORM_XBOX
#include "Utility/dx12/dxgiformat.h"
#include "Utility/dx12/d3dx12_default.h"
#include "Utility/dx12/d3dx12_resource_helpers.h"
#include "Utility/dx12/d3dx12_pipeline_state_stream.h"
#include "Utility/dx12/d3dx12_check_feature_support.h"
#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#pragma comment(lib,"dxguid.lib")
#endif // PLATFORM_XBOX

#include "Utility/D3D12MemAlloc.cpp" // include this here because we use D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED
#include "Utility/h264.h"
#include "Utility/dxva.h"

#include <map>
#include <string>
#include <pix.h>

#include <sstream>
#include <algorithm>
#include <intrin.h> // _BitScanReverse64

using namespace Microsoft::WRL;

namespace wi::graphics
{
namespace dx12_internal
{
	// Bindless allocation limits:
	static constexpr int BINDLESS_RESOURCE_CAPACITY = 500000;
	static constexpr int BINDLESS_SAMPLER_CAPACITY = 256;


#ifdef PLATFORM_WINDOWS_DESKTOP
	// On Windows PC we load DLLs manually because graphics device can be chosen at runtime:
	using PFN_CREATE_DXGI_FACTORY_2 = decltype(&CreateDXGIFactory2);
	static PFN_CREATE_DXGI_FACTORY_2 CreateDXGIFactory2 = nullptr;
#ifdef _DEBUG
	using PFN_DXGI_GET_DEBUG_INTERFACE1 = decltype(&DXGIGetDebugInterface1);
	static PFN_DXGI_GET_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
#endif
	static PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
	static PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer = nullptr;
#endif // PLATFORM_WINDOWS_DESKTOP

#ifdef PLATFORM_UWP
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#endif // PLATFORM_UWP

	// Engine -> Native converters
	constexpr uint32_t _ParseColorWriteMask(ColorWrite value)
	{
		uint32_t _flag = 0;

		if (value == ColorWrite::ENABLE_ALL)
		{
			return D3D12_COLOR_WRITE_ENABLE_ALL;
		}
		else
		{
			if (has_flag(value, ColorWrite::ENABLE_RED))
				_flag |= D3D12_COLOR_WRITE_ENABLE_RED;
			if (has_flag(value, ColorWrite::ENABLE_GREEN))
				_flag |= D3D12_COLOR_WRITE_ENABLE_GREEN;
			if (has_flag(value, ColorWrite::ENABLE_BLUE))
				_flag |= D3D12_COLOR_WRITE_ENABLE_BLUE;
			if (has_flag(value, ColorWrite::ENABLE_ALPHA))
				_flag |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
		}

		return _flag;
	}
	constexpr D3D12_RESOURCE_STATES _ParseResourceState(ResourceState value)
	{
		D3D12_RESOURCE_STATES ret = {};

		// MISSING STATE: UNDEFINED
		//	UNDEFINED state will be handled by DiscardResource() operation

		if (has_flag(value, ResourceState::SHADER_RESOURCE))
			ret |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (has_flag(value, ResourceState::SHADER_RESOURCE_COMPUTE))
			ret |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (has_flag(value, ResourceState::UNORDERED_ACCESS))
			ret |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if (has_flag(value, ResourceState::COPY_SRC))
			ret |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		if (has_flag(value, ResourceState::COPY_DST))
			ret |= D3D12_RESOURCE_STATE_COPY_DEST;

		if (has_flag(value, ResourceState::RENDERTARGET))
			ret |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if (has_flag(value, ResourceState::DEPTHSTENCIL))
			ret |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if (has_flag(value, ResourceState::DEPTHSTENCIL_READONLY))
			ret |= D3D12_RESOURCE_STATE_DEPTH_READ;
		if (has_flag(value, ResourceState::SHADING_RATE_SOURCE))
			ret |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

		if (has_flag(value, ResourceState::VERTEX_BUFFER))
			ret |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (has_flag(value, ResourceState::INDEX_BUFFER))
			ret |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if (has_flag(value, ResourceState::CONSTANT_BUFFER))
			ret |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (has_flag(value, ResourceState::INDIRECT_ARGUMENT))
			ret |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if (has_flag(value, ResourceState::RAYTRACING_ACCELERATION_STRUCTURE))
			ret |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if (has_flag(value, ResourceState::PREDICATION))
			ret |= D3D12_RESOURCE_STATE_PREDICATION;

		if (has_flag(value, ResourceState::VIDEO_DECODE_SRC))
			ret |= D3D12_RESOURCE_STATE_VIDEO_DECODE_READ;
		if (has_flag(value, ResourceState::VIDEO_DECODE_DST))
			ret |= D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE;

		return ret;
	}
	constexpr D3D12_FILTER _ConvertFilter(Filter value)
	{
		switch (value)
		{
		case Filter::MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
			break;
		case Filter::MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case Filter::MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case Filter::MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case Filter::MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case Filter::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case Filter::MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case Filter::MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		case Filter::ANISOTROPIC:
			return D3D12_FILTER_ANISOTROPIC;
			break;
		case Filter::COMPARISON_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
			break;
		case Filter::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case Filter::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case Filter::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case Filter::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case Filter::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case Filter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case Filter::COMPARISON_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
			break;
		case Filter::COMPARISON_ANISOTROPIC:
			return D3D12_FILTER_COMPARISON_ANISOTROPIC;
			break;
		case Filter::MINIMUM_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
			break;
		case Filter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case Filter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case Filter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case Filter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case Filter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case Filter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case Filter::MINIMUM_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
			break;
		case Filter::MINIMUM_ANISOTROPIC:
			return D3D12_FILTER_MINIMUM_ANISOTROPIC;
			break;
		case Filter::MAXIMUM_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
			break;
		case Filter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case Filter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case Filter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case Filter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case Filter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case Filter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case Filter::MAXIMUM_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
			break;
		case Filter::MAXIMUM_ANISOTROPIC:
			return D3D12_FILTER_MAXIMUM_ANISOTROPIC;
			break;
		default:
			break;
		}
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	}
	constexpr D3D_PRIMITIVE_TOPOLOGY _ConvertPrimitiveTopology(PrimitiveTopology topology, uint32_t controlPoints)
	{
		switch (topology)
		{
			case PrimitiveTopology::POINTLIST:
				return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			case PrimitiveTopology::LINELIST:
				return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case PrimitiveTopology::LINESTRIP:
				return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case PrimitiveTopology::TRIANGLELIST:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case PrimitiveTopology::TRIANGLESTRIP:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			case PrimitiveTopology::PATCHLIST:
				if (controlPoints == 0 || controlPoints > 32)
				{
					assert(false && "Invalid PatchList control points");
					return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
				}
				return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (controlPoints - 1));
			default:
				return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}
	constexpr D3D12_TEXTURE_ADDRESS_MODE _ConvertTextureAddressMode(TextureAddressMode value)
	{
		switch (value)
		{
		case TextureAddressMode::WRAP:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case TextureAddressMode::MIRROR:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case TextureAddressMode::CLAMP:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case TextureAddressMode::BORDER:
			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		case TextureAddressMode::MIRROR_ONCE:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		default:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		}
	}
	constexpr D3D12_STATIC_BORDER_COLOR _ConvertSamplerBorderColor(SamplerBorderColor value)
	{
		switch (value)
		{
		case SamplerBorderColor::TRANSPARENT_BLACK:
			return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		case SamplerBorderColor::OPAQUE_BLACK:
			return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		case SamplerBorderColor::OPAQUE_WHITE:
			return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		default:
			return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		}
	}
	constexpr D3D12_COMPARISON_FUNC _ConvertComparisonFunc(ComparisonFunc value)
	{
		switch (value)
		{
		case ComparisonFunc::NEVER:
			return D3D12_COMPARISON_FUNC_NEVER;
			break;
		case ComparisonFunc::LESS:
			return D3D12_COMPARISON_FUNC_LESS;
			break;
		case ComparisonFunc::EQUAL:
			return D3D12_COMPARISON_FUNC_EQUAL;
			break;
		case ComparisonFunc::LESS_EQUAL:
			return D3D12_COMPARISON_FUNC_LESS_EQUAL;
			break;
		case ComparisonFunc::GREATER:
			return D3D12_COMPARISON_FUNC_GREATER;
			break;
		case ComparisonFunc::NOT_EQUAL:
			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
			break;
		case ComparisonFunc::GREATER_EQUAL:
			return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			break;
		case ComparisonFunc::ALWAYS:
			return D3D12_COMPARISON_FUNC_ALWAYS;
			break;
		default:
			break;
		}
		return D3D12_COMPARISON_FUNC_NEVER;
	}
	constexpr D3D12_FILL_MODE _ConvertFillMode(FillMode value)
	{
		switch (value)
		{
		case FillMode::WIREFRAME:
			return D3D12_FILL_MODE_WIREFRAME;
			break;
		case FillMode::SOLID:
			return D3D12_FILL_MODE_SOLID;
			break;
		default:
			break;
		}
		return D3D12_FILL_MODE_WIREFRAME;
	}
	constexpr D3D12_CULL_MODE _ConvertCullMode(CullMode value)
	{
		switch (value)
		{
		case CullMode::NONE:
			return D3D12_CULL_MODE_NONE;
			break;
		case CullMode::FRONT:
			return D3D12_CULL_MODE_FRONT;
			break;
		case CullMode::BACK:
			return D3D12_CULL_MODE_BACK;
			break;
		default:
			break;
		}
		return D3D12_CULL_MODE_NONE;
	}
	constexpr D3D12_DEPTH_WRITE_MASK _ConvertDepthWriteMask(DepthWriteMask value)
	{
		switch (value)
		{
		case DepthWriteMask::ZERO:
			return D3D12_DEPTH_WRITE_MASK_ZERO;
			break;
		case DepthWriteMask::ALL:
			return D3D12_DEPTH_WRITE_MASK_ALL;
			break;
		default:
			break;
		}
		return D3D12_DEPTH_WRITE_MASK_ZERO;
	}
	constexpr D3D12_STENCIL_OP _ConvertStencilOp(StencilOp value)
	{
		switch (value)
		{
		case StencilOp::KEEP:
			return D3D12_STENCIL_OP_KEEP;
			break;
		case StencilOp::ZERO:
			return D3D12_STENCIL_OP_ZERO;
			break;
		case StencilOp::REPLACE:
			return D3D12_STENCIL_OP_REPLACE;
			break;
		case StencilOp::INCR_SAT:
			return D3D12_STENCIL_OP_INCR_SAT;
			break;
		case StencilOp::DECR_SAT:
			return D3D12_STENCIL_OP_DECR_SAT;
			break;
		case StencilOp::INVERT:
			return D3D12_STENCIL_OP_INVERT;
			break;
		case StencilOp::INCR:
			return D3D12_STENCIL_OP_INCR;
			break;
		case StencilOp::DECR:
			return D3D12_STENCIL_OP_DECR;
			break;
		default:
			break;
		}
		return D3D12_STENCIL_OP_KEEP;
	}
	constexpr D3D12_BLEND _ConvertBlend(Blend value)
	{
		switch (value)
		{
		case Blend::ZERO:
			return D3D12_BLEND_ZERO;
		case Blend::ONE:
			return D3D12_BLEND_ONE;
		case Blend::SRC_COLOR:
			return D3D12_BLEND_SRC_COLOR;
		case Blend::INV_SRC_COLOR:
			return D3D12_BLEND_INV_SRC_COLOR;
		case Blend::SRC_ALPHA:
			return D3D12_BLEND_SRC_ALPHA;
		case Blend::INV_SRC_ALPHA:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case Blend::DEST_ALPHA:
			return D3D12_BLEND_DEST_ALPHA;
		case Blend::INV_DEST_ALPHA:
			return D3D12_BLEND_INV_DEST_ALPHA;
		case Blend::DEST_COLOR:
			return D3D12_BLEND_DEST_COLOR;
		case Blend::INV_DEST_COLOR:
			return D3D12_BLEND_INV_DEST_COLOR;
		case Blend::SRC_ALPHA_SAT:
			return D3D12_BLEND_SRC_ALPHA_SAT;
		case Blend::BLEND_FACTOR:
			return D3D12_BLEND_BLEND_FACTOR;
		case Blend::INV_BLEND_FACTOR:
			return D3D12_BLEND_INV_BLEND_FACTOR;
		case Blend::SRC1_COLOR:
			return D3D12_BLEND_SRC1_COLOR;
		case Blend::INV_SRC1_COLOR:
			return D3D12_BLEND_INV_SRC1_COLOR;
		case Blend::SRC1_ALPHA:
			return D3D12_BLEND_SRC1_ALPHA;
		case Blend::INV_SRC1_ALPHA:
			return D3D12_BLEND_INV_SRC1_ALPHA;
		default:
			return D3D12_BLEND_ZERO;
		}
	}
	constexpr D3D12_BLEND _ConvertAlphaBlend(Blend value)
	{
		switch (value)
		{
			case Blend::SRC_COLOR:
				return D3D12_BLEND_SRC_ALPHA;
			case Blend::INV_SRC_COLOR:
				return D3D12_BLEND_INV_SRC_ALPHA;
			case Blend::DEST_COLOR:
				return D3D12_BLEND_DEST_ALPHA;
			case Blend::INV_DEST_COLOR:
				return D3D12_BLEND_INV_DEST_ALPHA;
			case Blend::SRC1_COLOR:
				return D3D12_BLEND_SRC1_ALPHA;
			case Blend::INV_SRC1_COLOR:
				return D3D12_BLEND_INV_SRC1_ALPHA;
			default:
				return _ConvertBlend(value);
		}
	}
	constexpr D3D12_BLEND_OP _ConvertBlendOp(BlendOp value)
	{
		switch (value)
		{
		case BlendOp::ADD:
			return D3D12_BLEND_OP_ADD;
		case BlendOp::SUBTRACT:
			return D3D12_BLEND_OP_SUBTRACT;
		case BlendOp::REV_SUBTRACT:
			return D3D12_BLEND_OP_REV_SUBTRACT;
		case BlendOp::MIN:
			return D3D12_BLEND_OP_MIN;
		case BlendOp::MAX:
			return D3D12_BLEND_OP_MAX;
		default:
			return D3D12_BLEND_OP_ADD;
		}
	}
	constexpr D3D12_INPUT_CLASSIFICATION _ConvertInputClassification(InputClassification value)
	{
		switch (value)
		{
		case InputClassification::PER_INSTANCE_DATA:
			return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
			break;
		default:
			return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		}
	}
	constexpr DXGI_FORMAT _ConvertFormat(Format value)
	{
		switch (value)
		{
		case Format::UNKNOWN:
			return DXGI_FORMAT_UNKNOWN;
		case Format::R32G32B32A32_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case Format::R32G32B32A32_UINT:
			return DXGI_FORMAT_R32G32B32A32_UINT;
		case Format::R32G32B32A32_SINT:
			return DXGI_FORMAT_R32G32B32A32_SINT;
		case Format::R32G32B32_FLOAT:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case Format::R32G32B32_UINT:
			return DXGI_FORMAT_R32G32B32_UINT;
		case Format::R32G32B32_SINT:
			return DXGI_FORMAT_R32G32B32_SINT;
		case Format::R16G16B16A16_FLOAT:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Format::R16G16B16A16_UNORM:
			return DXGI_FORMAT_R16G16B16A16_UNORM;
		case Format::R16G16B16A16_UINT:
			return DXGI_FORMAT_R16G16B16A16_UINT;
		case Format::R16G16B16A16_SNORM:
			return DXGI_FORMAT_R16G16B16A16_SNORM;
		case Format::R16G16B16A16_SINT:
			return DXGI_FORMAT_R16G16B16A16_SINT;
		case Format::R32G32_FLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;
		case Format::R32G32_UINT:
			return DXGI_FORMAT_R32G32_UINT;
		case Format::R32G32_SINT:
			return DXGI_FORMAT_R32G32_SINT;
		case Format::D32_FLOAT_S8X24_UINT:
			return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case Format::R10G10B10A2_UNORM:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case Format::R10G10B10A2_UINT:
			return DXGI_FORMAT_R10G10B10A2_UINT;
		case Format::R11G11B10_FLOAT:
			return DXGI_FORMAT_R11G11B10_FLOAT;
		case Format::R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Format::R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Format::R8G8B8A8_UINT:
			return DXGI_FORMAT_R8G8B8A8_UINT;
		case Format::R8G8B8A8_SNORM:
			return DXGI_FORMAT_R8G8B8A8_SNORM;
		case Format::R8G8B8A8_SINT:
			return DXGI_FORMAT_R8G8B8A8_SINT;
		case Format::R16G16_FLOAT:
			return DXGI_FORMAT_R16G16_FLOAT;
		case Format::R16G16_UNORM:
			return DXGI_FORMAT_R16G16_UNORM;
		case Format::R16G16_UINT:
			return DXGI_FORMAT_R16G16_UINT;
		case Format::R16G16_SNORM:
			return DXGI_FORMAT_R16G16_SNORM;
		case Format::R16G16_SINT:
			return DXGI_FORMAT_R16G16_SINT;
		case Format::D32_FLOAT:
			return DXGI_FORMAT_D32_FLOAT;
		case Format::R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case Format::R32_UINT:
			return DXGI_FORMAT_R32_UINT;
		case Format::R32_SINT:
			return DXGI_FORMAT_R32_SINT;
		case Format::D24_UNORM_S8_UINT:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Format::R9G9B9E5_SHAREDEXP:
			return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
		case Format::R8G8_UNORM:
			return DXGI_FORMAT_R8G8_UNORM;
		case Format::R8G8_UINT:
			return DXGI_FORMAT_R8G8_UINT;
		case Format::R8G8_SNORM:
			return DXGI_FORMAT_R8G8_SNORM;
		case Format::R8G8_SINT:
			return DXGI_FORMAT_R8G8_SINT;
		case Format::R16_FLOAT:
			return DXGI_FORMAT_R16_FLOAT;
		case Format::D16_UNORM:
			return DXGI_FORMAT_D16_UNORM;
		case Format::R16_UNORM:
			return DXGI_FORMAT_R16_UNORM;
		case Format::R16_UINT:
			return DXGI_FORMAT_R16_UINT;
		case Format::R16_SNORM:
			return DXGI_FORMAT_R16_SNORM;
		case Format::R16_SINT:
			return DXGI_FORMAT_R16_SINT;
		case Format::R8_UNORM:
			return DXGI_FORMAT_R8_UNORM;
		case Format::R8_UINT:
			return DXGI_FORMAT_R8_UINT;
		case Format::R8_SNORM:
			return DXGI_FORMAT_R8_SNORM;
		case Format::R8_SINT:
			return DXGI_FORMAT_R8_SINT;
		case Format::BC1_UNORM:
			return DXGI_FORMAT_BC1_UNORM;
		case Format::BC1_UNORM_SRGB:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case Format::BC2_UNORM:
			return DXGI_FORMAT_BC2_UNORM;
		case Format::BC2_UNORM_SRGB:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case Format::BC3_UNORM:
			return DXGI_FORMAT_BC3_UNORM;
		case Format::BC3_UNORM_SRGB:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case Format::BC4_UNORM:
			return DXGI_FORMAT_BC4_UNORM;
		case Format::BC4_SNORM:
			return DXGI_FORMAT_BC4_SNORM;
		case Format::BC5_UNORM:
			return DXGI_FORMAT_BC5_UNORM;
		case Format::BC5_SNORM:
			return DXGI_FORMAT_BC5_SNORM;
		case Format::B8G8R8A8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Format::B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case Format::BC6H_UF16:
			return DXGI_FORMAT_BC6H_UF16;
		case Format::BC6H_SF16:
			return DXGI_FORMAT_BC6H_SF16;
		case Format::BC7_UNORM:
			return DXGI_FORMAT_BC7_UNORM;
		case Format::BC7_UNORM_SRGB:
			return DXGI_FORMAT_BC7_UNORM_SRGB;
		case Format::NV12:
			return DXGI_FORMAT_NV12;
		}
		return DXGI_FORMAT_UNKNOWN;
	}
	constexpr DXGI_FORMAT _ConvertFormatToTypeless(DXGI_FORMAT value)
	{
		switch (value)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return DXGI_FORMAT_R32G32B32A32_TYPELESS;
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return DXGI_FORMAT_R32G32B32_TYPELESS;
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return DXGI_FORMAT_R16G16B16A16_TYPELESS;
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			return DXGI_FORMAT_R32G32_TYPELESS;
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			return DXGI_FORMAT_R32G8X24_TYPELESS;
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
			return DXGI_FORMAT_R10G10B10A2_TYPELESS;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return DXGI_FORMAT_R8G8B8A8_TYPELESS;
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
			return DXGI_FORMAT_R16G16_TYPELESS;
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
			return DXGI_FORMAT_R32_TYPELESS;
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
			return DXGI_FORMAT_R8G8_TYPELESS;
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
			return DXGI_FORMAT_R16_TYPELESS;
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
			return DXGI_FORMAT_R8_TYPELESS;
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return DXGI_FORMAT_BC1_TYPELESS;
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			return DXGI_FORMAT_BC2_TYPELESS;
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return DXGI_FORMAT_BC3_TYPELESS;
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return DXGI_FORMAT_BC4_TYPELESS;
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
			return DXGI_FORMAT_BC5_TYPELESS;
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_TYPELESS;
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8X8_TYPELESS;
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
			return DXGI_FORMAT_BC6H_TYPELESS;
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return DXGI_FORMAT_BC7_TYPELESS;
		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}
	constexpr D3D12_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
	{
		D3D12_SUBRESOURCE_DATA data = {};
		data.pData = pInitialData.data_ptr;
		data.RowPitch = pInitialData.row_pitch;
		data.SlicePitch = pInitialData.slice_pitch;

		return data;
	}
	constexpr D3D12_SHADER_VISIBILITY _ConvertShaderVisibility(ShaderStage value)
	{
		switch (value)
		{
		case ShaderStage::MS:
			return D3D12_SHADER_VISIBILITY_MESH;
		case ShaderStage::AS:
			return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
		case ShaderStage::VS:
			return D3D12_SHADER_VISIBILITY_VERTEX;
		case ShaderStage::HS:
			return D3D12_SHADER_VISIBILITY_HULL;
		case ShaderStage::DS:
			return D3D12_SHADER_VISIBILITY_DOMAIN;
		case ShaderStage::GS:
			return D3D12_SHADER_VISIBILITY_GEOMETRY;
		case ShaderStage::PS:
			return D3D12_SHADER_VISIBILITY_PIXEL;
		default:
			return D3D12_SHADER_VISIBILITY_ALL;
		}
		return D3D12_SHADER_VISIBILITY_ALL;
	}
	constexpr D3D12_SHADING_RATE _ConvertShadingRate(ShadingRate value)
	{
		switch (value)
		{
		case ShadingRate::RATE_1X1:
			return D3D12_SHADING_RATE_1X1;
		case ShadingRate::RATE_1X2:
			return D3D12_SHADING_RATE_1X2;
		case ShadingRate::RATE_2X1:
			return D3D12_SHADING_RATE_2X1;
		case ShadingRate::RATE_2X2:
			return D3D12_SHADING_RATE_2X2;
		case ShadingRate::RATE_2X4:
			return D3D12_SHADING_RATE_2X4;
		case ShadingRate::RATE_4X2:
			return D3D12_SHADING_RATE_4X2;
		case ShadingRate::RATE_4X4:
			return D3D12_SHADING_RATE_4X4;
		default:
			return D3D12_SHADING_RATE_1X1;
		}
		return D3D12_SHADING_RATE_1X1;
	}
	constexpr UINT _ImageAspectToPlane(ImageAspect value)
	{
		switch (value)
		{
		default:
		case wi::graphics::ImageAspect::COLOR:
			return 0;
		case wi::graphics::ImageAspect::DEPTH:
			return 0;
		case wi::graphics::ImageAspect::STENCIL:
			return 1;
		case wi::graphics::ImageAspect::LUMINANCE:
			return 0;
		case wi::graphics::ImageAspect::CHROMINANCE:
			return 1;
		}
	}
	constexpr D3D12_SHADER_COMPONENT_MAPPING _ConvertComponentSwizzle(ComponentSwizzle value)
	{
		switch (value)
		{
		default:
		case wi::graphics::ComponentSwizzle::R:
			return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0;
		case wi::graphics::ComponentSwizzle::G:
			return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1;
		case wi::graphics::ComponentSwizzle::B:
			return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2;
		case wi::graphics::ComponentSwizzle::A:
			return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3;
		case wi::graphics::ComponentSwizzle::ZERO:
			return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
		case wi::graphics::ComponentSwizzle::ONE:
			return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;
		}
	}
	constexpr UINT _ConvertSwizzle(Swizzle value)
	{
		return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(_ConvertComponentSwizzle(value.r), _ConvertComponentSwizzle(value.g), _ConvertComponentSwizzle(value.b), _ConvertComponentSwizzle(value.a));
	}

	// Native -> Engine converters
	constexpr Format _ConvertFormat_Inv(DXGI_FORMAT value)
	{
		switch (value)
		{
		case DXGI_FORMAT_UNKNOWN:
			return Format::UNKNOWN;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return Format::R32G32B32A32_FLOAT;
		case DXGI_FORMAT_R32G32B32A32_UINT:
			return Format::R32G32B32A32_UINT;
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return Format::R32G32B32A32_SINT;
		case DXGI_FORMAT_R32G32B32_FLOAT:
			return Format::R32G32B32_FLOAT;
		case DXGI_FORMAT_R32G32B32_UINT:
			return Format::R32G32B32_UINT;
		case DXGI_FORMAT_R32G32B32_SINT:
			return Format::R32G32B32_SINT;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return Format::R16G16B16A16_FLOAT;
		case DXGI_FORMAT_R16G16B16A16_UNORM:
			return Format::R16G16B16A16_UNORM;
		case DXGI_FORMAT_R16G16B16A16_UINT:
			return Format::R16G16B16A16_UINT;
		case DXGI_FORMAT_R16G16B16A16_SNORM:
			return Format::R16G16B16A16_SNORM;
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return Format::R16G16B16A16_SINT;
		case DXGI_FORMAT_R32G32_FLOAT:
			return Format::R32G32_FLOAT;
		case DXGI_FORMAT_R32G32_UINT:
			return Format::R32G32_UINT;
		case DXGI_FORMAT_R32G32_SINT:
			return Format::R32G32_SINT;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			return Format::D32_FLOAT_S8X24_UINT;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return Format::R10G10B10A2_UNORM;
		case DXGI_FORMAT_R10G10B10A2_UINT:
			return Format::R10G10B10A2_UINT;
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return Format::R11G11B10_FLOAT;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return Format::R8G8B8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return Format::R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT_R8G8B8A8_UINT:
			return Format::R8G8B8A8_UINT;
		case DXGI_FORMAT_R8G8B8A8_SNORM:
			return Format::R8G8B8A8_SNORM;
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return Format::R8G8B8A8_SINT;
		case DXGI_FORMAT_R16G16_FLOAT:
			return Format::R16G16_FLOAT;
		case DXGI_FORMAT_R16G16_UNORM:
			return Format::R16G16_UNORM;
		case DXGI_FORMAT_R16G16_UINT:
			return Format::R16G16_UINT;
		case DXGI_FORMAT_R16G16_SNORM:
			return Format::R16G16_SNORM;
		case DXGI_FORMAT_R16G16_SINT:
			return Format::R16G16_SINT;
		case DXGI_FORMAT_D32_FLOAT:
			return Format::D32_FLOAT;
		case DXGI_FORMAT_R32_FLOAT:
			return Format::R32_FLOAT;
		case DXGI_FORMAT_R32_UINT:
			return Format::R32_UINT;
		case DXGI_FORMAT_R32_SINT:
			return Format::R32_SINT;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return Format::D24_UNORM_S8_UINT;
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
			return Format::R9G9B9E5_SHAREDEXP;
		case DXGI_FORMAT_R8G8_UNORM:
			return Format::R8G8_UNORM;
		case DXGI_FORMAT_R8G8_UINT:
			return Format::R8G8_UINT;
		case DXGI_FORMAT_R8G8_SNORM:
			return Format::R8G8_SNORM;
		case DXGI_FORMAT_R8G8_SINT:
			return Format::R8G8_SINT;
		case DXGI_FORMAT_R16_FLOAT:
			return Format::R16_FLOAT;
		case DXGI_FORMAT_D16_UNORM:
			return Format::D16_UNORM;
		case DXGI_FORMAT_R16_UNORM:
			return Format::R16_UNORM;
		case DXGI_FORMAT_R16_UINT:
			return Format::R16_UINT;
		case DXGI_FORMAT_R16_SNORM:
			return Format::R16_SNORM;
		case DXGI_FORMAT_R16_SINT:
			return Format::R16_SINT;
		case DXGI_FORMAT_R8_UNORM:
			return Format::R8_UNORM;
		case DXGI_FORMAT_R8_UINT:
			return Format::R8_UINT;
		case DXGI_FORMAT_R8_SNORM:
			return Format::R8_SNORM;
		case DXGI_FORMAT_R8_SINT:
			return Format::R8_SINT;
		case DXGI_FORMAT_BC1_UNORM:
			return Format::BC1_UNORM;
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return Format::BC1_UNORM_SRGB;
		case DXGI_FORMAT_BC2_UNORM:
			return Format::BC2_UNORM;
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			return Format::BC2_UNORM_SRGB;
		case DXGI_FORMAT_BC3_UNORM:
			return Format::BC3_UNORM;
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return Format::BC3_UNORM_SRGB;
		case DXGI_FORMAT_BC4_UNORM:
			return Format::BC4_UNORM;
		case DXGI_FORMAT_BC4_SNORM:
			return Format::BC4_SNORM;
		case DXGI_FORMAT_BC5_UNORM:
			return Format::BC5_UNORM;
		case DXGI_FORMAT_BC5_SNORM:
			return Format::BC5_SNORM;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return Format::B8G8R8A8_UNORM;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return Format::B8G8R8A8_UNORM_SRGB;
		case DXGI_FORMAT_BC6H_UF16:
			return Format::BC6H_UF16;
		case DXGI_FORMAT_BC6H_SF16:
			return Format::BC6H_SF16;
		case DXGI_FORMAT_BC7_UNORM:
			return Format::BC7_UNORM;
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return Format::BC7_UNORM_SRGB;
		}
		return Format::UNKNOWN;
	}
	constexpr TextureDesc _ConvertTextureDesc_Inv(const D3D12_RESOURCE_DESC& desc)
	{
		TextureDesc retVal;
		
		switch (desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			retVal.type = TextureDesc::Type::TEXTURE_1D;
			retVal.array_size = desc.DepthOrArraySize;
			break;
		default:
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			retVal.type = TextureDesc::Type::TEXTURE_2D;
			retVal.array_size = desc.DepthOrArraySize;
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			retVal.type = TextureDesc::Type::TEXTURE_3D;
			retVal.depth = desc.DepthOrArraySize;
			break;
		}
		retVal.format = _ConvertFormat_Inv(desc.Format);
		retVal.width = (uint32_t)desc.Width;
		retVal.height = desc.Height;
		retVal.mip_levels = desc.MipLevels;

		return retVal;
	}

	struct RootSignatureOptimizer
	{
		static constexpr uint8_t INVALID_ROOT_PARAMETER = 0xFF;
		// These map shader registers in the binding space (space=0) to root parameters
		uint8_t CBV[DESCRIPTORBINDER_CBV_COUNT];
		uint8_t SRV[DESCRIPTORBINDER_SRV_COUNT];
		uint8_t UAV[DESCRIPTORBINDER_UAV_COUNT];
		uint8_t SAM[DESCRIPTORBINDER_SAMPLER_COUNT];
		uint8_t PUSH;
		// This is the bitflag of all root parameters:
		uint64_t root_mask = 0ull;
		// For each root parameter, store some statistics:
		struct RootParameterStatistics
		{
			uint32_t descriptorCopyCount = 0u;
			bool sampler_table = false;
		};
		wi::vector<RootParameterStatistics> root_stats;
		const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* rootsig_desc = nullptr;

		void init(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc)
		{
			rootsig_desc = &desc;

			// First, initialize all to point to invalid root parameter:
			for (size_t i = 0; i < arraysize(CBV); ++i)
			{
				CBV[i] = INVALID_ROOT_PARAMETER;
			}
			for (size_t i = 0; i < arraysize(SRV); ++i)
			{
				SRV[i] = INVALID_ROOT_PARAMETER;
			}
			for (size_t i = 0; i < arraysize(UAV); ++i)
			{
				UAV[i] = INVALID_ROOT_PARAMETER;
			}
			for (size_t i = 0; i < arraysize(SAM); ++i)
			{
				SAM[i] = INVALID_ROOT_PARAMETER;
			}
			PUSH = INVALID_ROOT_PARAMETER;

			assert(desc.Desc_1_1.NumParameters < 64u); // root parameter indices should fit into 64-bit root_mask
			root_stats.resize(desc.Desc_1_1.NumParameters); // one stat for each root parameter
			for (UINT root_parameter_index = 0; root_parameter_index < desc.Desc_1_1.NumParameters; ++root_parameter_index)
			{
				const D3D12_ROOT_PARAMETER1& param = desc.Desc_1_1.pParameters[root_parameter_index];
				RootParameterStatistics& stats = root_stats[root_parameter_index];

				if (param.ParameterType != D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) // root constant dirtyness is not tracked, because those are set immediately
				{
					root_mask |= 1ull << root_parameter_index;
				}

				switch (param.ParameterType)
				{
				case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
					for (UINT range_index = 0; range_index < param.DescriptorTable.NumDescriptorRanges; ++range_index)
					{
						const D3D12_DESCRIPTOR_RANGE1& range = param.DescriptorTable.pDescriptorRanges[range_index];
						stats.descriptorCopyCount += range.NumDescriptors == UINT_MAX ? 0 : range.NumDescriptors;
						stats.sampler_table = range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

						if (range.RegisterSpace != 0)
							continue; // we only care for the binding space (space=0)

						switch (range.RangeType)
						{
						case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
							for (UINT i = 0; i < range.NumDescriptors; ++i)
							{
								CBV[range.BaseShaderRegister + i] = (uint8_t)root_parameter_index;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
							for (UINT i = 0; i < range.NumDescriptors; ++i)
							{
								SRV[range.BaseShaderRegister + i] = (uint8_t)root_parameter_index;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
							for (UINT i = 0; i < range.NumDescriptors; ++i)
							{
								UAV[range.BaseShaderRegister + i] = (uint8_t)root_parameter_index;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
							for (UINT i = 0; i < range.NumDescriptors; ++i)
							{
								SAM[range.BaseShaderRegister + i] = (uint8_t)root_parameter_index;
							}
							break;
						default:
							assert(0);
							break;
						}
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
					if (param.Constants.RegisterSpace == 0)
					{
						assert(PUSH == INVALID_ROOT_PARAMETER); // check that push constant block was not already set, because only one is supported currently
						PUSH = (uint8_t)root_parameter_index;
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_CBV:
					if (param.Descriptor.RegisterSpace == 0)
					{
						assert(param.Descriptor.ShaderRegister < arraysize(CBV));
						CBV[param.Descriptor.ShaderRegister] = (uint8_t)root_parameter_index;
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_SRV:
					if (param.Descriptor.RegisterSpace == 0)
					{
						assert(param.Descriptor.ShaderRegister < arraysize(CBV));
						SRV[param.Descriptor.ShaderRegister] = (uint8_t)root_parameter_index;
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_UAV:
					if (param.Descriptor.RegisterSpace == 0)
					{
						assert(param.Descriptor.ShaderRegister < arraysize(CBV));
						UAV[param.Descriptor.ShaderRegister] = (uint8_t)root_parameter_index;
					}
					break;
				default:
					assert(0);
					break;
				}
			}

		}
	};

	struct SingleDescriptor
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
		D3D12_DESCRIPTOR_HEAP_TYPE type = {};
		int index = -1; // bindless

		bool IsValid() const { return handle.ptr != 0; }
		void init(const GraphicsDevice_DX12* device, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbv)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			handle = allocationhandler->descriptors_res.allocate();
			allocationhandler->device->CreateConstantBufferView(&cbv, handle);

			// bindless allocation:
			allocationhandler->destroylocker.lock();
			if (!allocationhandler->free_bindless_res.empty())
			{
				index = allocationhandler->free_bindless_res.back();
				allocationhandler->free_bindless_res.pop_back();
			}
			allocationhandler->destroylocker.unlock();
			if (index >= 0)
			{
				assert(index < BINDLESS_RESOURCE_CAPACITY);
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_res.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const GraphicsDevice_DX12* device, const D3D12_SHADER_RESOURCE_VIEW_DESC& srv, ID3D12Resource* res)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			handle = allocationhandler->descriptors_res.allocate();
			allocationhandler->device->CreateShaderResourceView(res, &srv, handle);

			// bindless allocation:
			allocationhandler->destroylocker.lock();
			if (!allocationhandler->free_bindless_res.empty())
			{
				index = allocationhandler->free_bindless_res.back();
				allocationhandler->free_bindless_res.pop_back();
			}
			allocationhandler->destroylocker.unlock();
			if (index >= 0)
			{
				assert(index < BINDLESS_RESOURCE_CAPACITY);
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_res.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const GraphicsDevice_DX12* device, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uav, ID3D12Resource* res)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			handle = allocationhandler->descriptors_res.allocate();
			allocationhandler->device->CreateUnorderedAccessView(res, nullptr, &uav, handle);

			// bindless allocation:
			allocationhandler->destroylocker.lock();
			if (!allocationhandler->free_bindless_res.empty())
			{
				index = allocationhandler->free_bindless_res.back();
				allocationhandler->free_bindless_res.pop_back();
			}
			allocationhandler->destroylocker.unlock();
			if (index >= 0)
			{
				assert(index < BINDLESS_RESOURCE_CAPACITY);
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_res.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const GraphicsDevice_DX12* device, const D3D12_SAMPLER_DESC& sam)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			handle = allocationhandler->descriptors_sam.allocate();
			allocationhandler->device->CreateSampler(&sam, handle);

			// bindless allocation:
			allocationhandler->destroylocker.lock();
			if (!allocationhandler->free_bindless_sam.empty())
			{
				index = allocationhandler->free_bindless_sam.back();
				allocationhandler->free_bindless_sam.pop_back();
			}
			allocationhandler->destroylocker.unlock();
			if (index >= 0)
			{
				assert(index < BINDLESS_SAMPLER_CAPACITY);
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_sam.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const GraphicsDevice_DX12* device, const D3D12_RENDER_TARGET_VIEW_DESC& rtv, ID3D12Resource* res)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			handle = allocationhandler->descriptors_rtv.allocate();
			allocationhandler->device->CreateRenderTargetView(res, &rtv, handle);
		}
		void init(const GraphicsDevice_DX12* device, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsv, ID3D12Resource* res)
		{
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

					// bindless free:
					if (index >= 0)
					{
						allocationhandler->destroylocker.lock();
						allocationhandler->destroyer_bindless_res.push_back(std::make_pair(index, allocationhandler->framecount));
						allocationhandler->destroylocker.unlock();
					}
					break;
				case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
					allocationhandler->descriptors_sam.free(handle);

					// bindless free:
					if (index >= 0)
					{
						allocationhandler->destroylocker.lock();
						allocationhandler->destroyer_bindless_sam.push_back(std::make_pair(index, allocationhandler->framecount));
						allocationhandler->destroylocker.unlock();
					}
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

		// These will allow to use D3D12CalcSubresource() and loop over D3D12 subresource indices which the descriptor includes:
		uint32_t firstMip = 0;
		uint32_t mipCount = 0;
		uint32_t firstSlice = 0;
		uint32_t sliceCount = 0;
	};

	struct Resource_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		ComPtr<D3D12MA::Allocation> allocation;
		ComPtr<ID3D12Resource> resource;
		SingleDescriptor srv;
		SingleDescriptor uav;
		wi::vector<SingleDescriptor> subresources_srv;
		wi::vector<SingleDescriptor> subresources_uav;
		SingleDescriptor uav_raw;

		D3D12_GPU_VIRTUAL_ADDRESS gpu_address = 0;

		UINT64 total_size = 0;
		wi::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints;
		wi::vector<UINT64> rowSizesInBytes;
		wi::vector<UINT> numRows;
		SparseTextureProperties sparse_texture_properties;

		virtual ~Resource_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (allocation) allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
			if (resource) allocationhandler->destroyer_resources.push_back(std::make_pair(resource, framecount));
			allocationhandler->destroylocker.unlock();

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
		wi::vector<SingleDescriptor> subresources_rtv;
		wi::vector<SingleDescriptor> subresources_dsv;

		wi::vector<SubresourceData> mapped_subresources;

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
	struct QueryHeap_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		ComPtr<ID3D12QueryHeap> heap;

		~QueryHeap_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (heap) allocationhandler->destroyer_queryheaps.push_back(std::make_pair(heap, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct PipelineState_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		ComPtr<ID3D12PipelineState> resource;
		ComPtr<ID3D12RootSignature> rootSignature;
		size_t hash = 0;

		wi::vector<uint8_t> shadercode;
		wi::vector<D3D12_INPUT_ELEMENT_DESC> input_elements;
		D3D_PRIMITIVE_TOPOLOGY primitiveTopology;

		ComPtr<ID3D12VersionedRootSignatureDeserializer> rootsig_deserializer;
		const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* rootsig_desc = nullptr;
		RootSignatureOptimizer rootsig_optimizer;

		struct PSO_STREAM
		{
			struct PSO_STREAM1
			{
				CD3DX12_PIPELINE_STATE_STREAM_VS VS;
				CD3DX12_PIPELINE_STATE_STREAM_HS HS;
				CD3DX12_PIPELINE_STATE_STREAM_DS DS;
				CD3DX12_PIPELINE_STATE_STREAM_GS GS;
				CD3DX12_PIPELINE_STATE_STREAM_PS PS;
				CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RS;
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 DSS;
				CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BD;
				CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PT;
				CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT IL;
				CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE STRIP;
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSFormat;
				CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS Formats;
				CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
				CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE ROOTSIG;
			} stream1 = {};

			struct PSO_STREAM2
			{
				CD3DX12_PIPELINE_STATE_STREAM_MS MS;
				CD3DX12_PIPELINE_STATE_STREAM_AS AS;
			} stream2 = {};
		} stream = {};

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
		wi::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
		GPUBuffer scratch;
	};
	struct RTPipelineState_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		ComPtr<ID3D12StateObject> resource;

		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;

		wi::vector<std::wstring> export_strings;
		wi::vector<D3D12_EXPORT_DESC> exports;
		wi::vector<D3D12_DXIL_LIBRARY_DESC> library_descs;
		wi::vector<std::wstring> group_strings;
		wi::vector<D3D12_HIT_GROUP_DESC> hitgroup_descs;

		~RTPipelineState_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_stateobjects.push_back(std::make_pair(resource, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct SwapChain_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
#ifdef PLATFORM_XBOX
		uint32_t bufferIndex = 0;
#else
		ComPtr<IDXGISwapChain3> swapChain;
#endif // PLATFORM_XBOX
		wi::vector<ComPtr<ID3D12Resource>> backBuffers;
		wi::vector<D3D12_CPU_DESCRIPTOR_HANDLE> backbufferRTV;

		Texture dummyTexture;
		ColorSpace colorSpace = ColorSpace::SRGB;

		~SwapChain_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			for (auto& x : backBuffers)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(x, framecount));
			}
			allocationhandler->destroylocker.unlock();
			for (auto& x : backbufferRTV)
			{
				allocationhandler->descriptors_rtv.free(x);
			}
		}

		inline uint32_t GetBufferIndex() const
		{
#ifdef PLATFORM_XBOX
			return bufferIndex;
#else
			return swapChain->GetCurrentBackBufferIndex();
#endif // PLATFORM_XBOX
		}
	};
	struct VideoDecoder_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		ComPtr<ID3D12VideoDecoderHeap> decoder_heap;
		ComPtr<ID3D12VideoDecoder> decoder;

		~VideoDecoder_DX12()
		{
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_video_decoder_heaps.push_back(std::make_pair(decoder_heap, framecount));
			allocationhandler->destroyer_video_decoders.push_back(std::make_pair(decoder, framecount));
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
	QueryHeap_DX12* to_internal(const GPUQueryHeap* param)
	{
		return static_cast<QueryHeap_DX12*>(param->internal_state.get());
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
	SwapChain_DX12* to_internal(const SwapChain* param)
	{
		return static_cast<SwapChain_DX12*>(param->internal_state.get());
	}
	VideoDecoder_DX12* to_internal(const VideoDecoder* param)
	{
		return static_cast<VideoDecoder_DX12*>(param->internal_state.get());
	}

#ifdef PLATFORM_WINDOWS_DESKTOP
	inline void HandleDeviceRemoved(PVOID context, BOOLEAN)
	{
		GraphicsDevice_DX12* removedDevice = (GraphicsDevice_DX12*)context;
		removedDevice->OnDeviceRemoved();
	}
#endif // PLATFORM_WINDOWS_DESKTOP
}
using namespace dx12_internal;

	

	void GraphicsDevice_DX12::CopyAllocator::init(GraphicsDevice_DX12* device)
	{
		this->device = device;
#ifdef PLATFORM_XBOX
		queue = device->queues[QUEUE_COPY].queue;
#else
		// On PC we can create secondary copy queue for background uploading tasks:
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;
		HRESULT hr = device->device->CreateCommandQueue(&desc, PPV_ARGS(queue));
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "ID3D12Device::CreateCommandQueue[CopyAllocator] failed! ERROR: 0x" << std::hex << hr;
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}
		hr = queue->SetName(L"CopyAllocator");
		assert(SUCCEEDED(hr));
#endif // PLATFORM_XBOX
	}
	GraphicsDevice_DX12::CopyAllocator::CopyCMD GraphicsDevice_DX12::CopyAllocator::allocate(uint64_t staging_size)
	{
		CopyCMD cmd;

		locker.lock();
		// Try to search for a staging buffer that can fit the request:
		for (size_t i = 0; i < freelist.size(); ++i)
		{
			if (freelist[i].uploadbuffer.desc.size >= staging_size)
			{
				if (freelist[i].IsCompleted())
				{
					cmd = std::move(freelist[i]);
					std::swap(freelist[i], freelist.back());
					freelist.pop_back();
					break;
				}
			}
		}
		locker.unlock();

		// If no buffer was found that fits the data, create one:
		if (!cmd.IsValid())
		{
			HRESULT hr = device->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, PPV_ARGS(cmd.commandAllocator));
			assert(SUCCEEDED(hr));
			hr = device->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, cmd.commandAllocator.Get(), nullptr, PPV_ARGS(cmd.commandList));
			assert(SUCCEEDED(hr));

			hr = cmd.commandList->Close();
			assert(SUCCEEDED(hr));

			hr = device->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(cmd.fence));
			assert(SUCCEEDED(hr));

			GPUBufferDesc uploadBufferDesc;
			uploadBufferDesc.size = wi::math::GetNextPowerOfTwo(staging_size);
			uploadBufferDesc.usage = Usage::UPLOAD;
			bool upload_success = device->CreateBuffer(&uploadBufferDesc, nullptr, &cmd.uploadbuffer);
			assert(upload_success);
			device->SetName(&cmd.uploadbuffer, "CopyAllocator::uploadBuffer");
		}

		// begin command list in valid state:
		HRESULT hr = cmd.commandAllocator->Reset();
		assert(SUCCEEDED(hr));
		hr = cmd.commandList->Reset(cmd.commandAllocator.Get(), nullptr);
		assert(SUCCEEDED(hr));

		return cmd;
	}
	void GraphicsDevice_DX12::CopyAllocator::submit(CopyCMD cmd)
	{
		HRESULT hr;

		locker.lock();
		cmd.fenceValueSignaled++;
		freelist.push_back(cmd);
		locker.unlock();

		cmd.commandList->Close();
		ID3D12CommandList* commandlists[] = {
			cmd.commandList.Get()
		};

#ifdef PLATFORM_XBOX
		std::scoped_lock lock(device->queue_locker); // queue operations are not thread-safe on XBOX
#endif // PLATFORM_XBOX

		queue->ExecuteCommandLists(1, commandlists);
		hr = queue->Signal(cmd.fence.Get(), cmd.fenceValueSignaled);
		assert(SUCCEEDED(hr));

		hr = device->queues[QUEUE_GRAPHICS].queue->Wait(cmd.fence.Get(), cmd.fenceValueSignaled);
		assert(SUCCEEDED(hr));
		hr = device->queues[QUEUE_COMPUTE].queue->Wait(cmd.fence.Get(), cmd.fenceValueSignaled);
		assert(SUCCEEDED(hr));
		hr = device->queues[QUEUE_COPY].queue->Wait(cmd.fence.Get(), cmd.fenceValueSignaled);
		assert(SUCCEEDED(hr));
		if (device->queues[QUEUE_VIDEO_DECODE].queue)
		{
			hr = device->queues[QUEUE_VIDEO_DECODE].queue->Wait(cmd.fence.Get(), cmd.fenceValueSignaled);
			assert(SUCCEEDED(hr));
		}
	}

	void GraphicsDevice_DX12::DescriptorBinder::init(GraphicsDevice_DX12* device)
	{
		this->device = device;

		// Reset state to empty:
		reset();
	}
	void GraphicsDevice_DX12::DescriptorBinder::reset()
	{
		table = {};
		optimizer_graphics = nullptr;
		dirty_graphics = 0ull;
		optimizer_compute = nullptr;
		dirty_compute = 0ull;
	}
	void GraphicsDevice_DX12::DescriptorBinder::flush(bool graphics, CommandList cmd)
	{
		uint64_t& dirty = graphics ? dirty_graphics : dirty_compute;
		if (dirty == 0ull)
			return;

		CommandList_DX12& commandlist = device->GetCommandList(cmd);
		ID3D12GraphicsCommandList* graphicscommandlist = commandlist.GetGraphicsCommandList();
		auto pso_internal = graphics ? to_internal(commandlist.active_pso) : to_internal(commandlist.active_cs);
		const RootSignatureOptimizer& optimizer = pso_internal->rootsig_optimizer;

		DWORD index;
		while (_BitScanReverse64(&index, dirty)) // This will make sure that only the dirty root params are iterated, bit-by-bit
		{
			const UINT root_parameter_index = (UINT)index;
			dirty ^= 1ull << root_parameter_index; // remove dirty bit of this root parameter
			const D3D12_ROOT_PARAMETER1& param = pso_internal->rootsig_desc->Desc_1_1.pParameters[root_parameter_index];
			const RootSignatureOptimizer::RootParameterStatistics& stats = optimizer.root_stats[root_parameter_index];

			switch (param.ParameterType)
			{

			case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			{
				DescriptorHeapGPU& heap = stats.sampler_table ? device->descriptorheap_sam : device->descriptorheap_res;
				D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = heap.start_gpu;

				if (stats.descriptorCopyCount == 1 && param.DescriptorTable.pDescriptorRanges[0].RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
				{
					// This is a special case of 1 descriptor per table. This doesn't need copying, but we can just reference single descriptors by their index
					//	because descriptor index was created already in shader visible descriptor heap for bindless access.
					//	We don't do this for constant buffers, because that needs to support dynamic offset (so we always create descriptor for them on the fly)
					const D3D12_DESCRIPTOR_RANGE1& range = param.DescriptorTable.pDescriptorRanges[0];
					switch (range.RangeType)
					{
					case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
						{
							const UINT reg = range.BaseShaderRegister;
							const GPUResource& resource = table.SRV[reg];
							if (resource.IsValid())
							{
								int subresource = table.SRV_index[reg];
								auto internal_state = to_internal(&resource);
								int descriptor_index = subresource < 0 ? internal_state->srv.index : internal_state->subresources_srv[subresource].index;
								gpu_handle.ptr += descriptor_index * device->resource_descriptor_size;
							}
						}
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
						{
							const UINT reg = range.BaseShaderRegister;
							const GPUResource& resource = table.UAV[reg];
							if (resource.IsValid())
							{
								int subresource = table.UAV_index[reg];
								auto internal_state = to_internal(&resource);
								int descriptor_index = subresource < 0 ? internal_state->uav.index : internal_state->subresources_uav[subresource].index;
								gpu_handle.ptr += descriptor_index * device->resource_descriptor_size;
							}
						}
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
						{
							const UINT reg = range.BaseShaderRegister;
							const Sampler& sam = table.SAM[reg];
							if (sam.IsValid())
							{
								auto internal_state = to_internal(&sam);
								int descriptor_index = internal_state->descriptor.index;
								gpu_handle.ptr += descriptor_index * device->sampler_descriptor_size;
							}
						}
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
						assert(0); // This must not be reached, constant buffers are handled below with dynamically created descriptors
						break;
					default:
						assert(0);
						break;
					}
				}
				else if (stats.descriptorCopyCount > 0)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = heap.start_cpu;
					const uint32_t descriptorSize = stats.sampler_table ? device->sampler_descriptor_size : device->resource_descriptor_size;
					const uint32_t bindless_capacity = stats.sampler_table ? BINDLESS_SAMPLER_CAPACITY : BINDLESS_RESOURCE_CAPACITY;

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

					// The reservation is the maximum amount of descriptors that can be allocated once
					static constexpr uint32_t wrap_reservation_cbv_srv_uav = DESCRIPTORBINDER_CBV_COUNT + DESCRIPTORBINDER_SRV_COUNT + DESCRIPTORBINDER_UAV_COUNT;
					static constexpr uint32_t wrap_reservation_sampler = DESCRIPTORBINDER_SAMPLER_COUNT;
					const uint32_t wrap_reservation = stats.sampler_table ? wrap_reservation_sampler : wrap_reservation_cbv_srv_uav;
					const uint32_t wrap_effective_size = heap.heapDesc.NumDescriptors - bindless_capacity - wrap_reservation;
					assert(wrap_reservation >= stats.descriptorCopyCount); // for correct lockless wrap behaviour

					const uint64_t offset = heap.allocationOffset.fetch_add(stats.descriptorCopyCount);
					const uint64_t wrapped_offset = offset % wrap_effective_size;
					const uint32_t ringoffset = (bindless_capacity + (uint32_t)wrapped_offset) * descriptorSize;
					const uint64_t wrapped_offset_end = wrapped_offset + stats.descriptorCopyCount;

					// Check that gpu offset doesn't intersect with our newly allocated range, if it does, we need to wait until gpu finishes with it:
					//	First check is with the cached completed value to avoid API call into fence object
					uint64_t wrapped_gpu_offset = heap.cached_completedValue % wrap_effective_size;
					if ((wrapped_offset < wrapped_gpu_offset) && (wrapped_gpu_offset < wrapped_offset_end))
					{
						// Second check is with current fence value with API call:
						wrapped_gpu_offset = heap.fence->GetCompletedValue() % wrap_effective_size;
						if ((wrapped_offset < wrapped_gpu_offset) && (wrapped_gpu_offset < wrapped_offset_end))
						{
							// Third step is actual wait until GPU updates fence so that requested descriptors are free:
							HRESULT hr = heap.fence->SetEventOnCompletion(heap.fenceValue, nullptr);
							assert(SUCCEEDED(hr));
						}
					}

					gpu_handle.ptr += (size_t)ringoffset;
					cpu_handle.ptr += (size_t)ringoffset;

					for (UINT i = 0; i < param.DescriptorTable.NumDescriptorRanges; ++i)
					{
						const D3D12_DESCRIPTOR_RANGE1& range = param.DescriptorTable.pDescriptorRanges[i];
						switch (range.RangeType)
						{
						case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
							assert(range.NumDescriptors <= DESCRIPTORBINDER_SRV_COUNT);
							device->device->CopyDescriptorsSimple(range.NumDescriptors, cpu_handle, device->nullSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							for (UINT idx = 0; idx < range.NumDescriptors; ++idx)
							{
								const UINT reg = range.BaseShaderRegister + idx;
								const GPUResource& resource = table.SRV[reg];
								if (resource.IsValid())
								{
									int subresource = table.SRV_index[reg];
									auto internal_state = to_internal(&resource);
									D3D12_CPU_DESCRIPTOR_HANDLE src_handle = subresource < 0 ? internal_state->srv.handle : internal_state->subresources_srv[subresource].handle;
									device->device->CopyDescriptorsSimple(1, cpu_handle, src_handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
								}
								cpu_handle.ptr += descriptorSize;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
							assert(range.NumDescriptors <= DESCRIPTORBINDER_UAV_COUNT);
							device->device->CopyDescriptorsSimple(range.NumDescriptors, cpu_handle, device->nullUAV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							for (UINT idx = 0; idx < range.NumDescriptors; ++idx)
							{
								const UINT reg = range.BaseShaderRegister + idx;
								const GPUResource& resource = table.UAV[reg];
								if (resource.IsValid())
								{
									int subresource = table.UAV_index[reg];
									auto internal_state = to_internal(&resource);
									D3D12_CPU_DESCRIPTOR_HANDLE src_handle = subresource < 0 ? internal_state->uav.handle : internal_state->subresources_uav[subresource].handle;
									device->device->CopyDescriptorsSimple(1, cpu_handle, src_handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
								}
								cpu_handle.ptr += descriptorSize;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
							assert(range.NumDescriptors <= DESCRIPTORBINDER_CBV_COUNT);
							device->device->CopyDescriptorsSimple(range.NumDescriptors, cpu_handle, device->nullCBV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
							for (UINT idx = 0; idx < range.NumDescriptors; ++idx)
							{
								const UINT reg = range.BaseShaderRegister + idx;
								const GPUBuffer& buffer = table.CBV[reg];
								if (buffer.IsValid())
								{
									uint64_t offset = table.CBV_offset[reg];
									auto internal_state = to_internal(&buffer);

									D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
									cbv.BufferLocation = internal_state->gpu_address;
									cbv.BufferLocation += offset;
									cbv.SizeInBytes = AlignTo(UINT(buffer.desc.size - offset), (UINT)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
									cbv.SizeInBytes = std::min(cbv.SizeInBytes, 65536u);

									device->device->CreateConstantBufferView(&cbv, cpu_handle);
								}
								cpu_handle.ptr += descriptorSize;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
							assert(range.NumDescriptors <= DESCRIPTORBINDER_SAMPLER_COUNT);
							device->device->CopyDescriptorsSimple(range.NumDescriptors, cpu_handle, device->nullSAM, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
							for (UINT idx = 0; idx < range.NumDescriptors; ++idx)
							{
								const UINT reg = range.BaseShaderRegister + idx;
								const Sampler& sam = table.SAM[reg];
								if (sam.IsValid())
								{
									auto internal_state = to_internal(&sam);
									D3D12_CPU_DESCRIPTOR_HANDLE src_handle = internal_state->descriptor.handle;
									device->device->CopyDescriptorsSimple(1, cpu_handle, src_handle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
								}
								cpu_handle.ptr += descriptorSize;
							}
							break;
						default:
							assert(0);
							break;
						}
					}
				}

				if (graphics)
				{
					graphicscommandlist->SetGraphicsRootDescriptorTable(
						root_parameter_index,
						gpu_handle
					);
				}
				else
				{
					graphicscommandlist->SetComputeRootDescriptorTable(
						root_parameter_index,
						gpu_handle
					);
				}
			}
			break;

			case D3D12_ROOT_PARAMETER_TYPE_CBV:
				{
					assert(param.Descriptor.ShaderRegister < DESCRIPTORBINDER_CBV_COUNT);
					const GPUBuffer& buffer = table.CBV[param.Descriptor.ShaderRegister];

					D3D12_GPU_VIRTUAL_ADDRESS address = {};
					if (buffer.IsValid())
					{
						uint64_t offset = table.CBV_offset[param.Descriptor.ShaderRegister];
						auto internal_state = to_internal(&buffer);
						address = internal_state->gpu_address;
						address += offset;
					}

					if (graphics)
					{
						graphicscommandlist->SetGraphicsRootConstantBufferView(
							root_parameter_index,
							address
						);
					}
					else
					{
						graphicscommandlist->SetComputeRootConstantBufferView(
							root_parameter_index,
							address
						);
					}
				}
				break;

			case D3D12_ROOT_PARAMETER_TYPE_SRV:
				{
					assert(param.Descriptor.ShaderRegister < DESCRIPTORBINDER_SRV_COUNT);
					const GPUResource& resource = table.SRV[param.Descriptor.ShaderRegister];
					int subresource = table.SRV_index[param.Descriptor.ShaderRegister];

					D3D12_GPU_VIRTUAL_ADDRESS address = {};
					if (resource.IsValid())
					{
						auto internal_state = to_internal(&resource);
						address = internal_state->gpu_address;
					}

					if (graphics)
					{
						graphicscommandlist->SetGraphicsRootShaderResourceView(
							root_parameter_index,
							address
						);
					}
					else
					{
						graphicscommandlist->SetComputeRootShaderResourceView(
							root_parameter_index,
							address
						);
					}
				}
				break;

			case D3D12_ROOT_PARAMETER_TYPE_UAV:
				{
					assert(param.Descriptor.ShaderRegister < DESCRIPTORBINDER_UAV_COUNT);
					const GPUResource& resource = table.UAV[param.Descriptor.ShaderRegister];
					int subresource = table.UAV_index[param.Descriptor.ShaderRegister];

					D3D12_GPU_VIRTUAL_ADDRESS address = {};
					if (resource.IsValid())
					{
						auto internal_state = to_internal(&resource);
						address = internal_state->gpu_address;
					}

					if (graphics)
					{
						graphicscommandlist->SetGraphicsRootUnorderedAccessView(
							root_parameter_index,
							address
						);
					}
					else
					{
						graphicscommandlist->SetComputeRootUnorderedAccessView(
							root_parameter_index,
							address
						);
					}
				}
				break;

			default:
				assert(0);
				break;
			}
		}

		assert(dirty == 0ull); // check that all dirty root parameters were handled
	}

	void GraphicsDevice_DX12::pso_validate(CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (!commandlist.dirty_pso)
			return;

		const PipelineState* pso = commandlist.active_pso;
		size_t pipeline_hash = commandlist.prev_pipeline_hash;

		auto internal_state = to_internal(pso);

		ID3D12PipelineState* pipeline = nullptr;
		auto it = pipelines_global.find(pipeline_hash);
		if (it == pipelines_global.end())
		{
			for (auto& x : commandlist.pipelines_worker)
			{
				if (pipeline_hash == x.first)
				{
					pipeline = x.second.Get();
					break;
				}
			}

			if (pipeline == nullptr)
			{
				// make copy, mustn't overwrite internal_state from here!
				PipelineState_DX12::PSO_STREAM stream = internal_state->stream;

				DXGI_FORMAT DSFormat = _ConvertFormat(commandlist.renderpass_info.ds_format);
				D3D12_RT_FORMAT_ARRAY formats = {};
				formats.NumRenderTargets = commandlist.renderpass_info.rt_count;
				for (uint32_t i = 0; i < commandlist.renderpass_info.rt_count; ++i)
				{
					formats.RTFormats[i] = _ConvertFormat(commandlist.renderpass_info.rt_formats[i]);
				}
				DXGI_SAMPLE_DESC sampleDesc = {};
				sampleDesc.Count = commandlist.renderpass_info.sample_count;
				sampleDesc.Quality = 0;

				stream.stream1.DSFormat = DSFormat;
				stream.stream1.Formats = formats;
				stream.stream1.SampleDesc = sampleDesc;

				D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
				streamDesc.pPipelineStateSubobjectStream = &stream;
				streamDesc.SizeInBytes = sizeof(stream.stream1);
				if (CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
				{
					streamDesc.SizeInBytes += sizeof(stream.stream2);
				}

				ComPtr<ID3D12PipelineState> newpso;
				HRESULT hr = device->CreatePipelineState(&streamDesc, PPV_ARGS(newpso));
				assert(SUCCEEDED(hr));

				commandlist.pipelines_worker.push_back(std::make_pair(pipeline_hash, newpso));
				pipeline = newpso.Get();
			}
		}
		else
		{
			pipeline = it->second.Get();
		}
		assert(pipeline != nullptr);

		commandlist.GetGraphicsCommandList()->SetPipelineState(pipeline);
		commandlist.dirty_pso = false;

		if (commandlist.prev_pt != internal_state->primitiveTopology)
		{
			commandlist.prev_pt = internal_state->primitiveTopology;

			commandlist.GetGraphicsCommandList()->IASetPrimitiveTopology(internal_state->primitiveTopology);
		}
	}

	void GraphicsDevice_DX12::predraw(CommandList cmd)
	{
		pso_validate(cmd);

		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.binder.flush(true, cmd);
	}
	void GraphicsDevice_DX12::predispatch(CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.binder.flush(false, cmd);
	}


	// Engine functions
	GraphicsDevice_DX12::GraphicsDevice_DX12(ValidationMode validationMode_, GPUPreference preference)
	{
		wi::Timer timer;

		SHADER_IDENTIFIER_SIZE = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

#ifndef PLATFORM_XBOX
		VIDEO_DECODE_BITSTREAM_ALIGNMENT = D3D12_VIDEO_DECODE_MIN_BITSTREAM_OFFSET_ALIGNMENT;
#endif // PLATFORM_XBOX

		validationMode = validationMode_;

#ifdef PLATFORM_WINDOWS_DESKTOP
		HMODULE dxgi = LoadLibraryEx(L"dxgi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (dxgi == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load dxgi.dll! ERROR: 0x" << std::hex << GetLastError();
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}

		HMODULE dx12 = LoadLibraryEx(L"d3d12.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (dx12 == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load d3d12.dll! ERROR: 0x" << std::hex << GetLastError();
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}

		CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY_2)wiGetProcAddress(dxgi, "CreateDXGIFactory2");
		assert(CreateDXGIFactory2 != nullptr);
		if (CreateDXGIFactory2 == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load CreateDXGIFactory2! ERROR: 0x" << std::hex << GetLastError();
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}

#ifdef _DEBUG
		if (validationMode != ValidationMode::Disabled)
		{
			DXGIGetDebugInterface1 = (PFN_DXGI_GET_DEBUG_INTERFACE1)wiGetProcAddress(dxgi, "DXGIGetDebugInterface1");
			assert(DXGIGetDebugInterface1 != nullptr);
		}
#endif

		D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)wiGetProcAddress(dx12, "D3D12CreateDevice");
		assert(D3D12CreateDevice != nullptr);
		if (D3D12CreateDevice == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load D3D12CreateDevice! ERROR: 0x" << std::hex << GetLastError();
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}

		D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)wiGetProcAddress(dx12, "D3D12CreateVersionedRootSignatureDeserializer");
		assert(D3D12CreateVersionedRootSignatureDeserializer != nullptr);
		if (D3D12CreateVersionedRootSignatureDeserializer == nullptr)
		{
			std::stringstream ss("");
			ss << "Failed to load D3D12CreateVersionedRootSignatureDeserializer! ERROR: 0x" << std::hex << GetLastError();
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}

		if (validationMode != ValidationMode::Disabled)
		{
			// Enable the debug layer.
			auto D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)wiGetProcAddress(dx12, "D3D12GetDebugInterface");
			if (D3D12GetDebugInterface)
			{
				ComPtr<ID3D12Debug> d3dDebug;
				if (SUCCEEDED(D3D12GetDebugInterface(PPV_ARGS(d3dDebug))))
				{
					d3dDebug->EnableDebugLayer();
					if (validationMode == ValidationMode::GPU)
					{
						ComPtr<ID3D12Debug1> d3dDebug1;
						if (SUCCEEDED(d3dDebug.As(&d3dDebug1)))
						{
							d3dDebug1->SetEnableGPUBasedValidation(TRUE);
							d3dDebug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
						}
					}
				}

				// DRED
				ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> pDredSettings;
				if (SUCCEEDED(D3D12GetDebugInterface(PPV_ARGS(pDredSettings))))
				{
					// Turn on auto-breadcrumbs and page fault reporting.
					pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
					pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
					pDredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
				}
			}

#if defined(_DEBUG)
			ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
			if (DXGIGetDebugInterface1 != nullptr && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
			{
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
				//dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);

				DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
				{
					80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
				};
				DXGI_INFO_QUEUE_FILTER filter = {};
				filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
				filter.DenyList.pIDList = hide;
				dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
			}
#endif
		}
#endif // PLATFORM_WINDOWS_DESKTOP

		HRESULT hr;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter;

#ifdef PLATFORM_XBOX

		hr = wi::graphics::xbox::CreateDevice(device, dxgiAdapter, validationMode);

#else
		hr = CreateDXGIFactory2((validationMode != ValidationMode::Disabled) ? DXGI_CREATE_FACTORY_DEBUG : 0u, PPV_ARGS(dxgiFactory));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "CreateDXGIFactory2 failed! ERROR: 0x" << std::hex << hr;
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}

		// Determines whether tearing support is available for fullscreen borderless windows.
		{
			BOOL allowTearing = FALSE;

			ComPtr<IDXGIFactory5> dxgiFactory5;
			hr = dxgiFactory.As(&dxgiFactory5);
			if (SUCCEEDED(hr))
			{
				hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
			}

			if (FAILED(hr) || !allowTearing)
			{
				tearingSupported = false;
				wi::helper::DebugOut("WARNING: Variable refresh rate displays not supported\n");
			}
			else
			{
				tearingSupported = true;
			}
		}

		// pick the highest performance adapter that is able to create the device

		ComPtr<IDXGIFactory6> dxgiFactory6;
		const bool queryByPreference = SUCCEEDED(dxgiFactory.As(&dxgiFactory6));
		auto NextAdapter = [&](uint32_t index, IDXGIAdapter1** ppAdapter)
		{
			if (queryByPreference)
			{
				return dxgiFactory6->EnumAdapterByGpuPreference(index, preference == GPUPreference::Integrated ? DXGI_GPU_PREFERENCE_MINIMUM_POWER : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(ppAdapter));
			}
			return dxgiFactory->EnumAdapters1(index, ppAdapter);
		};

		for (uint32_t i = 0; NextAdapter(i, dxgiAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 adapterDesc;
			hr = dxgiAdapter->GetDesc1(&adapterDesc);
			if (SUCCEEDED(hr))
			{
				// Don't select the Basic Render Driver adapter.
				if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}
			}

			D3D_FEATURE_LEVEL featurelevels[] = {
				D3D_FEATURE_LEVEL_12_2,
				D3D_FEATURE_LEVEL_12_1,
				D3D_FEATURE_LEVEL_12_0,
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
			};
			for (auto& featureLevel : featurelevels)
			{
				if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter.Get(), featureLevel, PPV_ARGS(device))))
				{
					break;
				}
			}
			if (device != nullptr)
				break;
		}

		assert(dxgiAdapter != nullptr);
		if (dxgiAdapter == nullptr)
		{
			wi::helper::messageBox("DXGI: No capable adapter found!", "Error!");
			wi::platform::Exit();
		}

		assert(device != nullptr);
		if (device == nullptr)
		{
			wi::helper::messageBox("D3D12: Device couldn't be created!", "Error!");
			wi::platform::Exit();
		}

		if (validationMode != ValidationMode::Disabled)
		{
			// Configure debug device (if active).
			ComPtr<ID3D12InfoQueue> d3dInfoQueue;
			if (SUCCEEDED(device.As(&d3dInfoQueue)))
			{
#ifdef _DEBUG
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
				//d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#endif

				std::vector<D3D12_MESSAGE_SEVERITY> enabledSeverities;
				std::vector<D3D12_MESSAGE_ID> disabledMessages;

				// These severities should be seen all the time
				enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_CORRUPTION);
				enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_ERROR);
				enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_WARNING);
				enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_MESSAGE);

				if (validationMode == ValidationMode::Verbose)
				{
					// Verbose only filters
					enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_INFO);
				}

				disabledMessages.push_back(D3D12_MESSAGE_ID_DRAW_EMPTY_SCISSOR_RECTANGLE);
				disabledMessages.push_back(D3D12_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS);

				D3D12_INFO_QUEUE_FILTER filter = {};
				filter.AllowList.NumSeverities = static_cast<UINT>(enabledSeverities.size());
				filter.AllowList.pSeverityList = enabledSeverities.data();
				filter.DenyList.NumIDs = static_cast<UINT>(disabledMessages.size());
				filter.DenyList.pIDList = disabledMessages.data();
				d3dInfoQueue->AddStorageFilterEntries(&filter);
			}
		}
#endif // PLATFORM_XBOX

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = device.Get();
		allocatorDesc.pAdapter = dxgiAdapter.Get();
		//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
		//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
		allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
		allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED;

		allocationhandler = std::make_shared<AllocationHandler>();
		allocationhandler->device = device;

		hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocationhandler->allocator);
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "D3D12MA::CreateAllocator failed! ERROR: 0x" << std::hex << hr;
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}

		{
			queues[QUEUE_GRAPHICS].desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			queues[QUEUE_GRAPHICS].desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queues[QUEUE_GRAPHICS].desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queues[QUEUE_GRAPHICS].desc.NodeMask = 0;
			hr = device->CreateCommandQueue(&queues[QUEUE_GRAPHICS].desc, PPV_ARGS(queues[QUEUE_GRAPHICS].queue));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateCommandQueue[QUEUE_GRAPHICS] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}
			hr = queues[QUEUE_GRAPHICS].queue->SetName(L"QUEUE_GRAPHICS");
			assert(SUCCEEDED(hr));
			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(queues[QUEUE_GRAPHICS].fence));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateFence[QUEUE_GRAPHICS] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}
		}

		{
			queues[QUEUE_COMPUTE].desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			queues[QUEUE_COMPUTE].desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queues[QUEUE_COMPUTE].desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queues[QUEUE_COMPUTE].desc.NodeMask = 0;
			hr = device->CreateCommandQueue(&queues[QUEUE_COMPUTE].desc, PPV_ARGS(queues[QUEUE_COMPUTE].queue));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateCommandQueue[QUEUE_COMPUTE] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}
			hr = queues[QUEUE_COMPUTE].queue->SetName(L"QUEUE_COMPUTE");
			assert(SUCCEEDED(hr));
			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(queues[QUEUE_COMPUTE].fence));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateFence[QUEUE_COMPUTE] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}
		}

		{
			queues[QUEUE_COPY].desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
			queues[QUEUE_COPY].desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queues[QUEUE_COPY].desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queues[QUEUE_COPY].desc.NodeMask = 0;
			hr = device->CreateCommandQueue(&queues[QUEUE_COPY].desc, PPV_ARGS(queues[QUEUE_COPY].queue));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateCommandQueue[QUEUE_COPY] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}
			hr = queues[QUEUE_COPY].queue->SetName(L"QUEUE_COPY");
			assert(SUCCEEDED(hr));
			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(queues[QUEUE_COPY].fence));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateFence[QUEUE_COPY] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}
		}


		if (SUCCEEDED(device.As(&video_device)))
		{
			queues[QUEUE_VIDEO_DECODE].desc.Type = D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE;
			queues[QUEUE_VIDEO_DECODE].desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queues[QUEUE_VIDEO_DECODE].desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queues[QUEUE_VIDEO_DECODE].desc.NodeMask = 0;
			hr = device->CreateCommandQueue(&queues[QUEUE_VIDEO_DECODE].desc, PPV_ARGS(queues[QUEUE_VIDEO_DECODE].queue));
			assert(SUCCEEDED(hr));
			if (SUCCEEDED(hr))
			{
				capabilities |= GraphicsDeviceCapability::VIDEO_DECODE_H264;
				hr = queues[QUEUE_VIDEO_DECODE].queue->SetName(L"QUEUE_VIDEO_DECODE");
				assert(SUCCEEDED(hr));
				hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(queues[QUEUE_VIDEO_DECODE].fence));
				assert(SUCCEEDED(hr));
				if (FAILED(hr))
				{
					std::stringstream ss("");
					ss << "ID3D12Device::CreateFence[QUEUE_VIDEO_DECODE] failed! ERROR: 0x" << std::hex << hr;
					wi::helper::messageBox(ss.str(), "Error!");
					wi::platform::Exit();
				}
			}
		}

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
			hr = device->CreateDescriptorHeap(&descriptorheap_res.heapDesc, PPV_ARGS(descriptorheap_res.heap_GPU));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateDescriptorHeap[CBV_SRV_UAV] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}

			descriptorheap_res.start_cpu = descriptorheap_res.heap_GPU->GetCPUDescriptorHandleForHeapStart();
			descriptorheap_res.start_gpu = descriptorheap_res.heap_GPU->GetGPUDescriptorHandleForHeapStart();

			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(descriptorheap_res.fence));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateFence[CBV_SRV_UAV] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}
			descriptorheap_res.fenceValue = descriptorheap_res.fence->GetCompletedValue();

			for (int i = 0; i < BINDLESS_RESOURCE_CAPACITY; ++i)
			{
				allocationhandler->free_bindless_res.push_back(BINDLESS_RESOURCE_CAPACITY - i - 1);
			}
		}

		// Sampler descriptor heap (shader visible):
		{
			descriptorheap_sam.heapDesc.NodeMask = 0;
			descriptorheap_sam.heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			descriptorheap_sam.heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			descriptorheap_sam.heapDesc.NumDescriptors = 2048; // tier 1 limit
			hr = device->CreateDescriptorHeap(&descriptorheap_sam.heapDesc, PPV_ARGS(descriptorheap_sam.heap_GPU));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateDescriptorHeap[SAMPLER] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}

			descriptorheap_sam.start_cpu = descriptorheap_sam.heap_GPU->GetCPUDescriptorHandleForHeapStart();
			descriptorheap_sam.start_gpu = descriptorheap_sam.heap_GPU->GetGPUDescriptorHandleForHeapStart();

			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(descriptorheap_sam.fence));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateFence[SAMPLER] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}
			descriptorheap_sam.fenceValue = descriptorheap_sam.fence->GetCompletedValue();

			for (int i = 0; i < BINDLESS_SAMPLER_CAPACITY; ++i)
			{
				allocationhandler->free_bindless_sam.push_back(BINDLESS_SAMPLER_CAPACITY - i - 1);
			}
		}

		// Create frame-resident resources:
		for (uint32_t buffer = 0; buffer < BUFFERCOUNT; ++buffer)
		{
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(frame_fence[buffer][queue]));
				assert(SUCCEEDED(hr));
				if (FAILED(hr))
				{
					std::stringstream ss("");
					ss << "ID3D12Device::CreateFence[FRAME] failed! ERROR: 0x" << std::hex << hr;
					wi::helper::messageBox(ss.str(), "Error!");
					wi::platform::Exit();
				}
			}
		}

		copyAllocator.init(this);

		// Query features:

		capabilities |= GraphicsDeviceCapability::TESSELLATION;
		capabilities |= GraphicsDeviceCapability::PREDICATION;
		capabilities |= GraphicsDeviceCapability::DEPTH_RESOLVE_MIN_MAX;
		capabilities |= GraphicsDeviceCapability::STENCIL_RESOLVE_MIN_MAX;

#ifdef PLATFORM_XBOX
		adapterName = wi::graphics::xbox::GetAdapterName();
		capabilities |= GraphicsDeviceCapability::MESH_SHADER;
		capabilities |= GraphicsDeviceCapability::DEPTH_BOUNDS_TEST;
		capabilities |= GraphicsDeviceCapability::GENERIC_SPARSE_TILE_POOL;
		capabilities |= GraphicsDeviceCapability::CACHE_COHERENT_UMA;
		casting_fully_typed_formats = false;
		resource_heap_tier = D3D12_RESOURCE_HEAP_TIER_2;
		additionalShadingRatesSupported = false;
		capabilities |= GraphicsDeviceCapability::VARIABLE_RATE_SHADING;
		capabilities |= GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2;
		VARIABLE_RATE_SHADING_TILE_SIZE = 8;
		capabilities |= GraphicsDeviceCapability::RAYTRACING;
		capabilities |= GraphicsDeviceCapability::UAV_LOAD_FORMAT_R11G11B10_FLOAT;
		capabilities |= GraphicsDeviceCapability::UAV_LOAD_FORMAT_COMMON;
		capabilities |= GraphicsDeviceCapability::RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS;
		capabilities |= GraphicsDeviceCapability::SPARSE_BUFFER;
		capabilities |= GraphicsDeviceCapability::SPARSE_TEXTURE2D;
		capabilities |= GraphicsDeviceCapability::SPARSE_NULL_MAPPING;
		capabilities |= GraphicsDeviceCapability::SAMPLER_MINMAX;
		capabilities |= GraphicsDeviceCapability::SPARSE_TEXTURE3D;
		capabilities |= GraphicsDeviceCapability::CONSERVATIVE_RASTERIZATION;
		capabilities |= GraphicsDeviceCapability::R9G9B9E5_SHAREDEXP_RENDERABLE;
		VIDEO_DECODE_BITSTREAM_ALIGNMENT = 256; // TODO
#else
		// Init feature check (https://devblogs.microsoft.com/directx/introducing-a-new-api-for-checking-feature-support-in-direct3d-12/)
		CD3DX12FeatureSupport features;
		hr = features.Init(device.Get());
		assert(SUCCEEDED(hr));

		// Init adapter properties
		{
			DXGI_ADAPTER_DESC1 adapterDesc;
			hr = dxgiAdapter->GetDesc1(&adapterDesc);
			assert(SUCCEEDED(hr));

			vendorId = adapterDesc.VendorId;
			deviceId = adapterDesc.DeviceId;
			wi::helper::StringConvert(adapterDesc.Description, adapterName);

			// Convert the adapter's D3D12 driver version to a readable string like "24.21.13.9793".
			LARGE_INTEGER umdVersion;
			if (dxgiAdapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umdVersion) != DXGI_ERROR_UNSUPPORTED)
			{
				uint64_t encodedVersion = umdVersion.QuadPart;
				std::ostringstream o;
				o << "D3D12 driver version ";
				uint16_t driverVersion[4] = {};

				for (size_t i = 0; i < 4; ++i) {
					driverVersion[i] = (encodedVersion >> (48 - 16 * i)) & 0xFFFF;
					o << driverVersion[i] << ".";
				}

				driverDescription = o.str();
			}

			// Detect adapter type.
			if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				adapterType = AdapterType::Cpu;
			}
			else
			{
				adapterType = features.UMA() ? AdapterType::IntegratedGpu : AdapterType::DiscreteGpu;
			}
		}

		if (features.HighestShaderModel() < D3D_SHADER_MODEL_6_0)
		{
			std::string error = "Shader model 6.0 is required, but not supported by your system!\n";
			error += "Adapter name: " + adapterName + "\n";
			error += "Adapter type: ";
			switch (adapterType)
			{
			case wi::graphics::AdapterType::IntegratedGpu:
				error += "Integrated GPU";
				break;
			case wi::graphics::AdapterType::DiscreteGpu:
				error += "Discrete GPU";
				break;
			case wi::graphics::AdapterType::VirtualGpu:
				error += "Virtual GPU";
				break;
			case wi::graphics::AdapterType::Cpu:
				error += "CPU";
				break;
			default:
				error += "Unknown";
				break;
			}
			error += "\nExiting.";
			wi::helper::messageBox(error, "Error!");
			wi::backlog::post(error, wi::backlog::LogLevel::Error);
			wi::platform::Exit();
		}

		if (features.ConservativeRasterizationTier() >= D3D12_CONSERVATIVE_RASTERIZATION_TIER_1)
		{
			capabilities |= GraphicsDeviceCapability::CONSERVATIVE_RASTERIZATION;
		}
		if (features.ROVsSupported() == TRUE)
		{
			capabilities |= GraphicsDeviceCapability::RASTERIZER_ORDERED_VIEWS;
		}
		if (features.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation() == TRUE)
		{
			capabilities |= GraphicsDeviceCapability::RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS;
		}
		if (features.TiledResourcesTier() >= D3D12_TILED_RESOURCES_TIER_1)
		{
			capabilities |= GraphicsDeviceCapability::SPARSE_BUFFER;
			capabilities |= GraphicsDeviceCapability::SPARSE_TEXTURE2D;

			if (features.TiledResourcesTier() >= D3D12_TILED_RESOURCES_TIER_2)
			{
				capabilities |= GraphicsDeviceCapability::SPARSE_NULL_MAPPING;

				// https://docs.microsoft.com/en-us/windows/win32/direct3d11/tiled-resources-texture-sampling-features
				capabilities |= GraphicsDeviceCapability::SAMPLER_MINMAX;

				if (features.TiledResourcesTier() >= D3D12_TILED_RESOURCES_TIER_3)
				{
					capabilities |= GraphicsDeviceCapability::SPARSE_TEXTURE3D;
				}
			}
		}

		if (features.TypedUAVLoadAdditionalFormats())
		{
			// More info about UAV format load support: https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
			capabilities |= GraphicsDeviceCapability::UAV_LOAD_FORMAT_COMMON;

			D3D12_FORMAT_SUPPORT1 formatSupport1 = D3D12_FORMAT_SUPPORT1_NONE;
			D3D12_FORMAT_SUPPORT2 formatSupport2 = D3D12_FORMAT_SUPPORT2_NONE;

			hr = features.FormatSupport(DXGI_FORMAT_R11G11B10_FLOAT, formatSupport1, formatSupport2);
			if (SUCCEEDED(hr) && (formatSupport2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
			{
				capabilities |= GraphicsDeviceCapability::UAV_LOAD_FORMAT_R11G11B10_FLOAT;
			}
		}

		if (features.RaytracingTier() >= D3D12_RAYTRACING_TIER_1_1)
		{
			capabilities |= GraphicsDeviceCapability::RAYTRACING;
		}

		if (features.VariableShadingRateTier() >= D3D12_VARIABLE_SHADING_RATE_TIER_1)
		{
			capabilities |= GraphicsDeviceCapability::VARIABLE_RATE_SHADING;
			if (features.VariableShadingRateTier() >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
			{
				capabilities |= GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2;
				VARIABLE_RATE_SHADING_TILE_SIZE = features.ShadingRateImageTileSize();
			}
		}

		additionalShadingRatesSupported = features.AdditionalShadingRatesSupported();

		if (features.MeshShaderTier() >= D3D12_MESH_SHADER_TIER_1)
		{
			capabilities |= GraphicsDeviceCapability::MESH_SHADER;
		}

		if (features.DepthBoundsTestSupported() == TRUE)
		{
			capabilities |= GraphicsDeviceCapability::DEPTH_BOUNDS_TEST;
		}

		if (features.HighestRootSignatureVersion() < D3D_ROOT_SIGNATURE_VERSION_1_1)
		{
			assert(0);
			wi::helper::messageBox("DX12: Root signature version 1.1 not supported!", "Error!");
			wi::platform::Exit();
		}

		// Fully typed casting: https://microsoft.github.io/DirectX-Specs/d3d/RelaxedCasting.html#casting-rules-for-rs2-drivers
		casting_fully_typed_formats = features.CastingFullyTypedFormatSupported();

		resource_heap_tier = features.ResourceHeapTier();

		if (resource_heap_tier >= D3D12_RESOURCE_HEAP_TIER_2)
		{
			capabilities |= GraphicsDeviceCapability::GENERIC_SPARSE_TILE_POOL;
		}

		if (features.CacheCoherentUMA())
		{
			capabilities |= GraphicsDeviceCapability::CACHE_COHERENT_UMA;

			D3D12MA::POOL_DESC pool_desc = {};
			pool_desc.HeapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
			pool_desc.HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
			pool_desc.HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
			hr = allocationhandler->allocator->CreatePool(&pool_desc, &allocationhandler->uma_pool);
			assert(SUCCEEDED(hr));
		}
#endif // PLATFORM_XBOX

#ifdef PLATFORM_WINDOWS_DESKTOP
		// Create fence to detect device removal
		{
			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(deviceRemovedFence.GetAddressOf()));
			assert(SUCCEEDED(hr));

			HANDLE deviceRemovedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
			hr = deviceRemovedFence->SetEventOnCompletion(UINT64_MAX, deviceRemovedEvent);
			assert(SUCCEEDED(hr));

			RegisterWaitForSingleObject(
				&deviceRemovedWaitHandle,
				deviceRemovedEvent,
				HandleDeviceRemoved,
				this, // Pass the device as our context
				INFINITE, // No timeout
				0 // No flags
			);
		}
#endif // PLATFORM_WINDOWS_DESKTOP

		// Create common indirect command signatures:

		D3D12_COMMAND_SIGNATURE_DESC cmd_desc = {};

		D3D12_INDIRECT_ARGUMENT_DESC dispatchArgs[1];
		dispatchArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

		D3D12_INDIRECT_ARGUMENT_DESC drawInstancedArgs[1];
		drawInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

		D3D12_INDIRECT_ARGUMENT_DESC drawIndexedInstancedArgs[1];
		drawIndexedInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		cmd_desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = dispatchArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, PPV_ARGS(dispatchIndirectCommandSignature));
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "ID3D12Device::CreateCommandSignature[dispatchIndirect] failed! ERROR: 0x" << std::hex << hr;
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}

		cmd_desc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = drawInstancedArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, PPV_ARGS(drawInstancedIndirectCommandSignature));
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "ID3D12Device::CreateCommandSignature[drawInstancedIndirect] failed! ERROR: 0x" << std::hex << hr;
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}

		cmd_desc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = drawIndexedInstancedArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, PPV_ARGS(drawIndexedInstancedIndirectCommandSignature));
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "ID3D12Device::CreateCommandSignature[drawIndexedInstancedIndirect] failed! ERROR: 0x" << std::hex << hr;
			wi::helper::messageBox(ss.str(), "Error!");
			wi::platform::Exit();
		}

		if (CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
		{
			D3D12_INDIRECT_ARGUMENT_DESC dispatchMeshArgs[1];
#ifdef PLATFORM_XBOX
			wi::graphics::xbox::FillDispatchMeshIndirectArgumentDesc(dispatchMeshArgs[0], cmd_desc);
#else
			dispatchMeshArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
			cmd_desc.ByteStride = sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
#endif // PLATFORM_XBOX
			cmd_desc.NumArgumentDescs = 1;
			cmd_desc.pArgumentDescs = dispatchMeshArgs;
			hr = device->CreateCommandSignature(&cmd_desc, nullptr, PPV_ARGS(dispatchMeshIndirectCommandSignature));
			assert(SUCCEEDED(hr));
			if (FAILED(hr))
			{
				std::stringstream ss("");
				ss << "ID3D12Device::CreateCommandSignature[dispatchMeshIndirect] failed! ERROR: 0x" << std::hex << hr;
				wi::helper::messageBox(ss.str(), "Error!");
				wi::platform::Exit();
			}
		}

		allocationhandler->descriptors_res.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
		allocationhandler->descriptors_sam.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 256);
		allocationhandler->descriptors_rtv.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 512);
		allocationhandler->descriptors_dsv.init(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 128);

		D3D12_DESCRIPTOR_HEAP_DESC nullHeapDesc = {};
		nullHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		nullHeapDesc.NumDescriptors = DESCRIPTORBINDER_CBV_COUNT + DESCRIPTORBINDER_SRV_COUNT + DESCRIPTORBINDER_UAV_COUNT;
		hr = device->CreateDescriptorHeap(&nullHeapDesc, PPV_ARGS(nulldescriptorheap_cbv_srv_uav));
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "ID3D12Device::CreateDescriptorHeap[nulldescriptorheap_cbv_srv_uav] failed! ERROR: 0x" << std::hex << hr;
			wi::helper::messageBox(ss.str(), "Error!");
		}

		nullHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		nullHeapDesc.NumDescriptors = DESCRIPTORBINDER_SAMPLER_COUNT;
		device->CreateDescriptorHeap(&nullHeapDesc, PPV_ARGS(nulldescriptorheap_sampler));
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "ID3D12Device::CreateDescriptorHeap[nulldescriptorheap_sampler] failed! ERROR: 0x" << std::hex << hr;
			wi::helper::messageBox(ss.str(), "Error!");
		}

		nullCBV = nulldescriptorheap_cbv_srv_uav->GetCPUDescriptorHandleForHeapStart();
		for (uint32_t i = 0; i < DESCRIPTORBINDER_CBV_COUNT; ++i)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
			D3D12_CPU_DESCRIPTOR_HANDLE handle = nullCBV;
			handle.ptr += i * resource_descriptor_size;
			device->CreateConstantBufferView(&cbv_desc, handle);
		}

		nullSRV = nullCBV;
		nullSRV.ptr += DESCRIPTORBINDER_CBV_COUNT * resource_descriptor_size;
		for (uint32_t i = 0; i < DESCRIPTORBINDER_SRV_COUNT; ++i)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = DXGI_FORMAT_R32_UINT;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			D3D12_CPU_DESCRIPTOR_HANDLE handle = nullSRV;
			handle.ptr += i * resource_descriptor_size;
			device->CreateShaderResourceView(nullptr, &srv_desc, handle);
		}

		nullUAV = nullSRV;
		nullUAV.ptr += DESCRIPTORBINDER_SRV_COUNT * resource_descriptor_size;
		for (uint32_t i = 0; i < DESCRIPTORBINDER_UAV_COUNT; ++i)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			D3D12_CPU_DESCRIPTOR_HANDLE handle = nullUAV;
			handle.ptr += i * resource_descriptor_size;
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, handle);
		}

		nullSAM = nulldescriptorheap_sampler->GetCPUDescriptorHandleForHeapStart();
		for (uint32_t i = 0; i < DESCRIPTORBINDER_SAMPLER_COUNT; ++i)
		{
			D3D12_SAMPLER_DESC sampler_desc = {};
			sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			D3D12_CPU_DESCRIPTOR_HANDLE handle = nullSAM;
			handle.ptr += i * sampler_descriptor_size;
			device->CreateSampler(&sampler_desc, handle);
		}

		hr = queues[QUEUE_GRAPHICS].queue->GetTimestampFrequency(&TIMESTAMP_FREQUENCY);
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "ID3D12CommandQueue::GetTimestampFrequency[QUEUE_GRAPHICS] failed! ERROR: 0x" << std::hex << hr;
			wi::helper::messageBox(ss.str(), "Warning!");
		}

		wi::backlog::post("Created GraphicsDevice_DX12 (" + std::to_string((int)std::round(timer.elapsed())) + " ms)\nAdapter: " + adapterName);
	}
	GraphicsDevice_DX12::~GraphicsDevice_DX12()
	{
		WaitForGPU();

#ifdef PLATFORM_WINDOWS_DESKTOP
		std::ignore = UnregisterWait(deviceRemovedWaitHandle);
		deviceRemovedFence.Reset();
#endif // PLATFORM_WINDOWS_DESKTOP
	}

	bool GraphicsDevice_DX12::CreateSwapChain(const SwapChainDesc* desc, wi::platform::window_type window, SwapChain* swapchain) const
	{
		auto internal_state = std::static_pointer_cast<SwapChain_DX12>(swapchain->internal_state);
		if (swapchain->internal_state == nullptr)
		{
			internal_state = std::make_shared<SwapChain_DX12>();
		}
		internal_state->allocationhandler = allocationhandler;
		swapchain->internal_state = internal_state;
		swapchain->desc = *desc;
		HRESULT hr = E_FAIL;

		if (!internal_state->backbufferRTV.empty())
		{
			// Delete back buffer resources if they exist before resizing swap chain:
			WaitForGPU();
			internal_state->backBuffers.clear();
			for (auto& x : internal_state->backbufferRTV)
			{
				allocationhandler->descriptors_rtv.free(x);
			}
			internal_state->backbufferRTV.clear();
		}

#ifdef PLATFORM_XBOX

		D3D12_HEAP_PROPERTIES heap_properties = {};
		heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

		D3D12_RESOURCE_DESC resource_desc = {};
		resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resource_desc.Format = _ConvertFormat(swapchain->desc.format);
		resource_desc.Width = swapchain->desc.width;
		resource_desc.Height = swapchain->desc.height;
		resource_desc.DepthOrArraySize = 1;
		resource_desc.MipLevels = 1;
		resource_desc.SampleDesc.Count = 1;
		resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = resource_desc.Format;
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		D3D12_CLEAR_VALUE clear_value = {};
		clear_value.Format = resource_desc.Format;
		clear_value.Color[0] = swapchain->desc.clear_color[0];
		clear_value.Color[1] = swapchain->desc.clear_color[1];
		clear_value.Color[2] = swapchain->desc.clear_color[2];
		clear_value.Color[3] = swapchain->desc.clear_color[3];

		internal_state->backBuffers.resize(swapchain->desc.buffer_count);
		internal_state->backbufferRTV.resize(swapchain->desc.buffer_count);
		for (uint32_t i = 0; i < swapchain->desc.buffer_count; ++i)
		{
			hr = device->CreateCommittedResource(
				&heap_properties,
				D3D12_HEAP_FLAG_ALLOW_DISPLAY,
				&resource_desc,
				D3D12_RESOURCE_STATE_PRESENT,
				&clear_value,
				PPV_ARGS(internal_state->backBuffers[i])
			);
			assert(SUCCEEDED(hr));

			hr = internal_state->backBuffers[i]->SetName(L"BackBufferXBOX");
			assert(SUCCEEDED(hr));

			internal_state->backbufferRTV[i] = allocationhandler->descriptors_rtv.allocate();
			device->CreateRenderTargetView(internal_state->backBuffers[i].Get(), &rtv_desc, internal_state->backbufferRTV[i]);
		}

#else
		UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		if (tearingSupported)
		{
			swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		}

		if (internal_state->swapChain == nullptr)
		{
			// Create swapchain:
			ComPtr<IDXGISwapChain1> tempSwapChain;

			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.Width = desc->width;
			swapChainDesc.Height = desc->height;
			swapChainDesc.Format = _ConvertFormat(desc->format);
			swapChainDesc.Stereo = false;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = desc->buffer_count;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			swapChainDesc.Flags = swapChainFlags;

#ifdef PLATFORM_WINDOWS_DESKTOP
			swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
			fullscreenDesc.Windowed = !desc->fullscreen;

			hr = dxgiFactory->CreateSwapChainForHwnd(
				queues[QUEUE_GRAPHICS].queue.Get(),
				window,
				&swapChainDesc,
				&fullscreenDesc,
				nullptr,
				tempSwapChain.GetAddressOf()
			);
#else
			swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

			hr = dxgiFactory->CreateSwapChainForCoreWindow(
				queues[QUEUE_GRAPHICS].queue.Get(),
				static_cast<IUnknown*>(winrt::get_abi(*window)),
				&swapChainDesc,
				nullptr,
				tempSwapChain.GetAddressOf()
			);
#endif // PLATFORM_WINDOWS_DESKTOP

			if (FAILED(hr))
			{
				return false;
			}

			hr = tempSwapChain.As(&internal_state->swapChain);
			if (FAILED(hr))
			{
				return false;
			}
		}
		else
		{
			// Resize swapchain:
			hr = internal_state->swapChain->ResizeBuffers(
				desc->buffer_count,
				desc->width,
				desc->height,
				_ConvertFormat(desc->format),
				swapChainFlags
			);
			assert(SUCCEEDED(hr));
		}

		const bool hdr = desc->allow_hdr && IsSwapChainSupportsHDR(swapchain);

		// Ensure correct color space:
		//	https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HDR/src/D3D12HDR.cpp
		{
			internal_state->colorSpace = ColorSpace::SRGB; // reset to SDR, in case anything below fails to set HDR state
			DXGI_COLOR_SPACE_TYPE colorSpace = {};

			switch (desc->format)
			{
			case Format::R10G10B10A2_UNORM:
				// This format is either HDR10 (ST.2084), or SDR (SRGB)
				colorSpace = hdr ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
				break;
			case Format::R16G16B16A16_FLOAT:
				// This format is HDR (Linear):
				colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
				break;
			default:
				// Anything else will be SDR (SRGB):
				colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
				break;
			}

			UINT colorSpaceSupport = 0;
			if (SUCCEEDED(internal_state->swapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)))
			{
				if (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
				{
					hr = internal_state->swapChain->SetColorSpace1(colorSpace);
					assert(SUCCEEDED(hr));
					if (SUCCEEDED(hr))
					{
						switch (colorSpace)
						{
						default:
						case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
							internal_state->colorSpace = ColorSpace::SRGB;
							break;
						case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
							internal_state->colorSpace = ColorSpace::HDR_LINEAR;
							break;
						case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
							internal_state->colorSpace = ColorSpace::HDR10_ST2084;
							break;
						}
					}
				}
			}
		}

		internal_state->backBuffers.resize(desc->buffer_count);
		internal_state->backbufferRTV.resize(desc->buffer_count);

		// We can create swapchain just with given supported format, thats why we specify format in RTV
		// For example: BGRA8UNorm for SwapChain BGRA8UNormSrgb for RTV.
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = _ConvertFormat(desc->format);
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		for (uint32_t i = 0; i < desc->buffer_count; ++i)
		{
			hr = internal_state->swapChain->GetBuffer(i, PPV_ARGS(internal_state->backBuffers[i]));
			assert(SUCCEEDED(hr));

			internal_state->backbufferRTV[i] = allocationhandler->descriptors_rtv.allocate();
			device->CreateRenderTargetView(internal_state->backBuffers[i].Get(), &rtvDesc, internal_state->backbufferRTV[i]);
		}
#endif // PLATFORM_XBOX

		internal_state->dummyTexture.desc.format = desc->format;
		internal_state->dummyTexture.desc.width = desc->width;
		internal_state->dummyTexture.desc.height = desc->height;
		return true;
	}
	bool GraphicsDevice_DX12::CreateBuffer2(const GPUBufferDesc* desc, const std::function<void(void*)>& init_callback, GPUBuffer* buffer) const
	{
		auto internal_state = std::make_shared<Resource_DX12>();
		internal_state->allocationhandler = allocationhandler;
		buffer->internal_state = internal_state;
		buffer->type = GPUResource::Type::BUFFER;
		buffer->mapped_data = nullptr;
		buffer->mapped_size = 0;
		buffer->desc = *desc;

		HRESULT hr = E_FAIL;

		UINT64 alignedSize = desc->size;
		if (has_flag(desc->bind_flags, BindFlag::CONSTANT_BUFFER))
		{
			alignedSize = AlignTo(alignedSize, (UINT64)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		}

		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.Width = alignedSize;
		resourceDesc.Height = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Alignment = 0;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (has_flag(desc->bind_flags, BindFlag::UNORDERED_ACCESS))
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!has_flag(desc->bind_flags, BindFlag::SHADER_RESOURCE) && !has_flag(desc->misc_flags, ResourceMiscFlag::RAY_TRACING))
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		if (desc->usage == Usage::READBACK)
		{
			allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
			resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
		else if (desc->usage == Usage::UPLOAD)
		{
			allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
			resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}

#ifdef PLATFORM_XBOX
		wi::graphics::xbox::ApplyBufferCreationFlags(*desc, resourceDesc.Flags, allocationDesc.ExtraHeapFlags);
#endif // PLATFORM_XBOX

		if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_BUFFER) ||
			has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_NON_RT_DS) ||
			has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_RT_DS))
		{
			// Sparse tile pool must not be a committed resource because that uses implicit heap which returns nullptr,
			//	thus it cannot be offsetted. This is why we create custom allocation here which will never be committed resource
			//	(since it has no resource)
			D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = {};
			allocationInfo.Alignment = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
			allocationInfo.SizeInBytes = AlignTo(desc->size, (uint64_t)D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES);

			if (resource_heap_tier >= D3D12_RESOURCE_HEAP_TIER_2)
			{
				// tile pool memory can be used for sparse buffers and textures alike (requires resource heap tier 2):
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
			}
			else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_BUFFER))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			}
			else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_NON_RT_DS))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
			}
			else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_RT_DS))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
			}

			hr = allocationhandler->allocator->AllocateMemory(
				&allocationDesc,
				&allocationInfo,
				&internal_state->allocation
			);
			assert(SUCCEEDED(hr));

			hr = device->CreatePlacedResource(
				internal_state->allocation->GetHeap(),
				internal_state->allocation->GetOffset(),
				&resourceDesc,
				resourceState,
				nullptr,
				PPV_ARGS(internal_state->resource)
			);
			assert(SUCCEEDED(hr));
		}
		else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE))
		{
			hr = device->CreateReservedResource(
				&resourceDesc,
				resourceState,
				nullptr,
				PPV_ARGS(internal_state->resource)
			);
			assert(SUCCEEDED(hr));
			buffer->sparse_page_size = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
		}
		else
		{
			hr = allocationhandler->allocator->CreateResource(
				&allocationDesc,
				&resourceDesc,
				resourceState,
				nullptr,
				&internal_state->allocation,
				PPV_ARGS(internal_state->resource)
			);
			assert(SUCCEEDED(hr));
		}

		if (!SUCCEEDED(hr))
			return false;

		internal_state->gpu_address = internal_state->resource->GetGPUVirtualAddress();

		if (desc->usage == Usage::READBACK)
		{
			hr = internal_state->resource->Map(0, nullptr, &buffer->mapped_data);
			assert(SUCCEEDED(hr));
			buffer->mapped_size = static_cast<uint32_t>(desc->size);
		}
		else if (desc->usage == Usage::UPLOAD)
		{
			D3D12_RANGE read_range = {};
			hr = internal_state->resource->Map(0, &read_range, &buffer->mapped_data);
			assert(SUCCEEDED(hr));
			buffer->mapped_size = static_cast<uint32_t>(desc->size);
		}

		// Issue data copy on request:
		if (init_callback != nullptr)
		{
			CopyAllocator::CopyCMD cmd;
			void* mapped_data = nullptr;
			if (desc->usage == Usage::UPLOAD)
			{
				mapped_data = buffer->mapped_data;
			}
			else
			{
				cmd = copyAllocator.allocate(desc->size);
				mapped_data = cmd.uploadbuffer.mapped_data;
			}

			init_callback(mapped_data);

			if (cmd.IsValid())
			{
				cmd.commandList->CopyBufferRegion(
					internal_state->resource.Get(),
					0,
					to_internal(&cmd.uploadbuffer)->resource.Get(),
					0,
					desc->size
				);
				copyAllocator.submit(cmd);
			}
		}


		// Create resource views if needed
		if (!has_flag(desc->misc_flags, ResourceMiscFlag::NO_DEFAULT_DESCRIPTORS))
		{
			if (has_flag(desc->bind_flags, BindFlag::SHADER_RESOURCE))
			{
				CreateSubresource(buffer, SubresourceType::SRV, 0);
			}
			if (has_flag(desc->bind_flags, BindFlag::UNORDERED_ACCESS))
			{
				CreateSubresource(buffer, SubresourceType::UAV, 0);
			}
		}

		if (
			has_flag(desc->bind_flags, BindFlag::UNORDERED_ACCESS) &&
			(
				!has_flag(desc->misc_flags, ResourceMiscFlag::BUFFER_RAW) ||
				has_flag(desc->misc_flags, ResourceMiscFlag::NO_DEFAULT_DESCRIPTORS)
			)
		)
		{
			// Create raw buffer if doesn't exist for ClearUAV:
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
			uav_desc.Buffer.NumElements = uint32_t(desc->size / sizeof(uint32_t));
			internal_state->uav_raw.init(this, uav_desc, internal_state->resource.Get());
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture) const
	{
		auto internal_state = std::make_shared<Texture_DX12>();
		internal_state->allocationhandler = allocationhandler;
		texture->internal_state = internal_state;
		texture->type = GPUResource::Type::TEXTURE;
		texture->mapped_data = nullptr;
		texture->mapped_size = 0;
		texture->mapped_subresources = nullptr;
		texture->mapped_subresource_count = 0;
		texture->sparse_properties = nullptr;
		texture->desc = *desc;

		bool require_format_casting = has_flag(desc->misc_flags, ResourceMiscFlag::TYPED_FORMAT_CASTING);

		HRESULT hr = E_FAIL;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourcedesc;
		resourcedesc.Format = _ConvertFormat(desc->format);
		resourcedesc.Width = desc->width;
		resourcedesc.Height = desc->height;
		resourcedesc.MipLevels = desc->mip_levels;
		resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourcedesc.DepthOrArraySize = (UINT16)desc->array_size;
		resourcedesc.SampleDesc.Count = desc->sample_count;
		resourcedesc.SampleDesc.Quality = 0;
		resourcedesc.Alignment = 0;
		resourcedesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (has_flag(desc->bind_flags, BindFlag::DEPTH_STENCIL))
		{
			resourcedesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			//allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
			if (has_flag(desc->bind_flags, BindFlag::SHADER_RESOURCE))
			{
				require_format_casting = true;
			}
			else
			{
				resourcedesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}
		if (has_flag(desc->bind_flags, BindFlag::RENDER_TARGET))
		{
			resourcedesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			//allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
		}
		if (has_flag(desc->bind_flags, BindFlag::UNORDERED_ACCESS))
		{
			resourcedesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (has_flag(desc->misc_flags, ResourceMiscFlag::VIDEO_DECODE))
		{
			// Because video queue can only transition from/to VIDEO_ and COMMON states, we will use COMMON internally and rely on implicit transition for DPB textures
			//	(See how the resource barrier on video queue overrides any user specified state into COMMON)
			resourcedesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		}

		switch (texture->desc.type)
		{
		case TextureDesc::Type::TEXTURE_1D:
			resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			break;
		case TextureDesc::Type::TEXTURE_2D:
			resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			break;
		case TextureDesc::Type::TEXTURE_3D:
			resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			resourcedesc.DepthOrArraySize = (UINT16)desc->depth;
			break;
		default:
			assert(0);
			break;
		}

		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Color[0] = texture->desc.clear.color[0];
		optimizedClearValue.Color[1] = texture->desc.clear.color[1];
		optimizedClearValue.Color[2] = texture->desc.clear.color[2];
		optimizedClearValue.Color[3] = texture->desc.clear.color[3];
		optimizedClearValue.DepthStencil.Depth = texture->desc.clear.depth_stencil.depth;
		optimizedClearValue.DepthStencil.Stencil = texture->desc.clear.depth_stencil.stencil;
		optimizedClearValue.Format = resourcedesc.Format; // Typed
		bool useClearValue = has_flag(desc->bind_flags, BindFlag::RENDER_TARGET) || has_flag(desc->bind_flags, BindFlag::DEPTH_STENCIL);

		if ((!casting_fully_typed_formats && require_format_casting) || has_flag(texture->desc.misc_flags, ResourceMiscFlag::TYPELESS_FORMAT_CASTING))
		{
			// Fallback to TYPELESS, must be AFTER optimizedClearValue.Format was set:
			resourcedesc.Format = _ConvertFormatToTypeless(resourcedesc.Format);
		}

		D3D12_RESOURCE_STATES resourceState = _ParseResourceState(texture->desc.layout);

		if (initial_data != nullptr)
		{
			resourceState = D3D12_RESOURCE_STATE_COMMON;
		}

		if (texture->desc.mip_levels == 0)
		{
			texture->desc.mip_levels = GetMipCount(texture->desc.width, texture->desc.height, texture->desc.depth);
		}

		internal_state->total_size = 0;
		internal_state->footprints.resize(desc->array_size * std::max(1u, desc->mip_levels));
		internal_state->rowSizesInBytes.resize(internal_state->footprints.size());
		internal_state->numRows.resize(internal_state->footprints.size());
		device->GetCopyableFootprints(
			&resourcedesc,
			0,
			(UINT)internal_state->footprints.size(),
			0,
			internal_state->footprints.data(),
			internal_state->numRows.data(),
			internal_state->rowSizesInBytes.data(),
			&internal_state->total_size
		);

		if (texture->desc.usage == Usage::READBACK || texture->desc.usage == Usage::UPLOAD)
		{
			resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourcedesc.Width = internal_state->total_size;
			resourcedesc.Height = 1;
			resourcedesc.DepthOrArraySize = 1;
			resourcedesc.MipLevels = 1;
			resourcedesc.Format = DXGI_FORMAT_UNKNOWN;
			resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourcedesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			if (texture->desc.usage == Usage::READBACK)
			{
				allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
				resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
			}
			else if(texture->desc.usage == Usage::UPLOAD)
			{
				allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
				resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
			}
		}

#ifndef PLATFORM_XBOX // Xbox is detected as UMA, but the UMA texture upload path is disabled for now
		if (
			initial_data != nullptr &&
			allocationDesc.HeapType == D3D12_HEAP_TYPE_DEFAULT &&
			CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA)
			)
		{
			// UMA: custom pool is like DEFAULT heap + CPU Write Combine
			//	It will be used with WriteToSubresource to avoid GPU copy from UPLOAD to DEAFULT
			allocationDesc.CustomPool = allocationhandler->uma_pool.Get();
		}
#endif // PLATFORM_XBOX

#ifdef PLATFORM_XBOX
		wi::graphics::xbox::ApplyTextureCreationFlags(texture->desc, resourcedesc.Flags, allocationDesc.ExtraHeapFlags);
#endif // PLATFORM_XBOX

		if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::SPARSE))
		{
			resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
			hr = device->CreateReservedResource(
				&resourcedesc,
				resourceState,
				useClearValue ? &optimizedClearValue : nullptr,
				PPV_ARGS(internal_state->resource)
			);
			texture->sparse_page_size = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;

			UINT num_tiles_for_entire_resource = 0;
			D3D12_PACKED_MIP_INFO packed_mip_info = {};
			D3D12_TILE_SHAPE tile_shape = {};
			UINT num_subresource_tilings = 0;

			device->GetResourceTiling(
				internal_state->resource.Get(),
				&num_tiles_for_entire_resource,
				&packed_mip_info,
				&tile_shape,
				&num_subresource_tilings,
				0,
				nullptr
			);

			SparseTextureProperties& sparse = internal_state->sparse_texture_properties;
			texture->sparse_properties = &sparse;
			sparse.tile_width = tile_shape.WidthInTexels;
			sparse.tile_height = tile_shape.HeightInTexels;
			sparse.tile_depth = tile_shape.DepthInTexels;
			sparse.total_tile_count = num_tiles_for_entire_resource;
			sparse.packed_mip_start = packed_mip_info.NumStandardMips;
			sparse.packed_mip_count = packed_mip_info.NumPackedMips;
			sparse.packed_mip_tile_offset = packed_mip_info.StartTileIndexInOverallResource;
			sparse.packed_mip_tile_count = packed_mip_info.NumTilesForPackedMips;
		}
		else
		{
			if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::SHARED))
			{
				// Dedicated memory
				allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

				// What about D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER and D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER?
				allocationDesc.ExtraHeapFlags |= D3D12_HEAP_FLAG_SHARED;
			}

			hr = allocationhandler->allocator->CreateResource(
				&allocationDesc,
				&resourcedesc,
				resourceState,
				useClearValue ? &optimizedClearValue : nullptr,
				&internal_state->allocation,
				PPV_ARGS(internal_state->resource)
			);
		}
		assert(SUCCEEDED(hr));

		if (texture->desc.usage == Usage::READBACK)
		{
			hr = internal_state->resource->Map(0, nullptr, &texture->mapped_data);
			assert(SUCCEEDED(hr));
		}
		else if(texture->desc.usage == Usage::UPLOAD)
		{
			D3D12_RANGE read_range = {};
			hr = internal_state->resource->Map(0, &read_range, &texture->mapped_data);
			assert(SUCCEEDED(hr));
		}
		else if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::SHARED))
		{
			hr = allocationhandler->device->CreateSharedHandle(
				internal_state->resource.Get(),
				nullptr,
				GENERIC_ALL,
				nullptr,
				&texture->shared_handle);

			assert(SUCCEEDED(hr));
		}

		if (texture->mapped_data != nullptr)
		{
			texture->mapped_size = internal_state->total_size;
			internal_state->mapped_subresources.resize(internal_state->footprints.size());
			for (size_t i = 0; i < internal_state->footprints.size(); ++i)
			{
				internal_state->mapped_subresources[i].data_ptr = (uint8_t*)texture->mapped_data + internal_state->footprints[i].Offset;
				internal_state->mapped_subresources[i].row_pitch = internal_state->footprints[i].Footprint.RowPitch;
				internal_state->mapped_subresources[i].slice_pitch = internal_state->footprints[i].Footprint.RowPitch * internal_state->footprints[i].Footprint.Height;
			}
			texture->mapped_subresources = internal_state->mapped_subresources.data();
			texture->mapped_subresource_count = internal_state->mapped_subresources.size();
		}

		// Issue data copy on request:
		if (initial_data != nullptr)
		{
			if (allocationDesc.CustomPool != nullptr && allocationDesc.CustomPool == allocationhandler->uma_pool.Get())
			{
				// UMA direct texture write path:
				for (size_t i = 0; i < internal_state->footprints.size(); ++i)
				{
					const SubresourceData& data = initial_data[i];

					hr = internal_state->resource->WriteToSubresource(
						(UINT)i,
						nullptr,
						data.data_ptr,
						data.row_pitch,
						data.slice_pitch
					);
					assert(SUCCEEDED(hr));
				}
			}
			else
			{
				// Discrete GPU texture upload path:
				CopyAllocator::CopyCMD cmd;
				void* mapped_data = nullptr;
				if (texture->mapped_data == nullptr)
				{
					cmd = copyAllocator.allocate(internal_state->total_size);
					mapped_data = cmd.uploadbuffer.mapped_data;
				}
				else
				{
					mapped_data = texture->mapped_data;
				}

				for (size_t i = 0; i < internal_state->footprints.size(); ++i)
				{
					D3D12_SUBRESOURCE_DATA data = _ConvertSubresourceData(initial_data[i]);

					if (internal_state->rowSizesInBytes[i] > (SIZE_T)-1)
						continue;
					D3D12_MEMCPY_DEST DestData = {};
					DestData.pData = (void*)((UINT64)mapped_data + internal_state->footprints[i].Offset);
					DestData.RowPitch = (SIZE_T)internal_state->footprints[i].Footprint.RowPitch;
					DestData.SlicePitch = (SIZE_T)internal_state->footprints[i].Footprint.RowPitch * (SIZE_T)internal_state->numRows[i];
					MemcpySubresource(&DestData, &data, (SIZE_T)internal_state->rowSizesInBytes[i], internal_state->numRows[i], internal_state->footprints[i].Footprint.Depth);

					if (cmd.IsValid())
					{
						CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state->resource.Get(), UINT(i));
						CD3DX12_TEXTURE_COPY_LOCATION Src(to_internal(&cmd.uploadbuffer)->resource.Get(), internal_state->footprints[i]);
						cmd.commandList->CopyTextureRegion(
							&Dst,
							0,
							0,
							0,
							&Src,
							nullptr
						);
					}
				}

				if (cmd.IsValid())
				{
					copyAllocator.submit(cmd);
				}
			}
		}

		if (!has_flag(desc->misc_flags, ResourceMiscFlag::NO_DEFAULT_DESCRIPTORS))
		{
			if (has_flag(texture->desc.bind_flags, BindFlag::RENDER_TARGET))
			{
				CreateSubresource(texture, SubresourceType::RTV, 0, -1, 0, -1);
			}
			if (has_flag(texture->desc.bind_flags, BindFlag::DEPTH_STENCIL))
			{
				CreateSubresource(texture, SubresourceType::DSV, 0, -1, 0, -1);
			}
			if (has_flag(texture->desc.bind_flags, BindFlag::SHADER_RESOURCE))
			{
				CreateSubresource(texture, SubresourceType::SRV, 0, -1, 0, -1);
			}
			if (has_flag(texture->desc.bind_flags, BindFlag::UNORDERED_ACCESS))
			{
				CreateSubresource(texture, SubresourceType::UAV, 0, -1, 0, -1);
			}
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader) const
	{
		auto internal_state = std::make_shared<PipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		shader->internal_state = internal_state;

		internal_state->shadercode.resize(shadercode_size);
		internal_state->hash = 0;
		std::memcpy(internal_state->shadercode.data(), shadercode, shadercode_size);
		shader->stage = stage;

		HRESULT hr = (internal_state->shadercode.empty() ? E_FAIL : S_OK);
		assert(SUCCEEDED(hr));

		hr = D3D12CreateVersionedRootSignatureDeserializer(
			internal_state->shadercode.data(),
			internal_state->shadercode.size(),
			PPV_ARGS(internal_state->rootsig_deserializer)
		);
		if (SUCCEEDED(hr))
		{
			hr = internal_state->rootsig_deserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &internal_state->rootsig_desc);
			if (SUCCEEDED(hr))
			{
				assert(internal_state->rootsig_desc->Version == D3D_ROOT_SIGNATURE_VERSION_1_1);

				hr = device->CreateRootSignature(
					0,
					internal_state->shadercode.data(),
					internal_state->shadercode.size(),
					PPV_ARGS(internal_state->rootSignature)
				);
				assert(SUCCEEDED(hr));
			}
		}

		if (stage == ShaderStage::CS || stage == ShaderStage::LIB)
		{
			assert(internal_state->rootSignature != nullptr);
			assert(internal_state->rootsig_desc != nullptr);
			internal_state->rootsig_optimizer.init(*internal_state->rootsig_desc);
		}

		if (stage == ShaderStage::CS)
		{
			struct PSO_STREAM
			{
				CD3DX12_PIPELINE_STATE_STREAM_CS CS;
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE ROOTSIG;
			} stream;

			stream.CS = { internal_state->shadercode.data(), internal_state->shadercode.size() };
			stream.ROOTSIG = internal_state->rootSignature.Get();

			D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
			streamDesc.pPipelineStateSubobjectStream = &stream;
			streamDesc.SizeInBytes = sizeof(stream);

			HRESULT hr = device->CreatePipelineState(&streamDesc, PPV_ARGS(internal_state->resource));
			assert(SUCCEEDED(hr));
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateSampler(const SamplerDesc* desc, Sampler* sampler) const
	{
		auto internal_state = std::make_shared<Sampler_DX12>();
		internal_state->allocationhandler = allocationhandler;
		sampler->internal_state = internal_state;

		D3D12_SAMPLER_DESC samplerdesc;
		samplerdesc.Filter = _ConvertFilter(desc->filter);
		samplerdesc.AddressU = _ConvertTextureAddressMode(desc->address_u);
		samplerdesc.AddressV = _ConvertTextureAddressMode(desc->address_v);
		samplerdesc.AddressW = _ConvertTextureAddressMode(desc->address_w);
		samplerdesc.MipLODBias = desc->mip_lod_bias;
		samplerdesc.MaxAnisotropy = desc->max_anisotropy;
		samplerdesc.ComparisonFunc = _ConvertComparisonFunc(desc->comparison_func);
		switch (desc->border_color)
		{
		case SamplerBorderColor::OPAQUE_BLACK:
			samplerdesc.BorderColor[0] = 0.0f;
			samplerdesc.BorderColor[1] = 0.0f;
			samplerdesc.BorderColor[2] = 0.0f;
			samplerdesc.BorderColor[3] = 1.0f;
			break;

		case SamplerBorderColor::OPAQUE_WHITE:
			samplerdesc.BorderColor[0] = 1.0f;
			samplerdesc.BorderColor[1] = 1.0f;
			samplerdesc.BorderColor[2] = 1.0f;
			samplerdesc.BorderColor[3] = 1.0f;
			break;

		default:
			samplerdesc.BorderColor[0] = 0.0f;
			samplerdesc.BorderColor[1] = 0.0f;
			samplerdesc.BorderColor[2] = 0.0f;
			samplerdesc.BorderColor[3] = 0.0f;
			break;
		}
		samplerdesc.MinLOD = desc->min_lod;
		samplerdesc.MaxLOD = desc->max_lod;

		sampler->desc = *desc;

		internal_state->descriptor.init(this, samplerdesc);

		return true;
	}
	bool GraphicsDevice_DX12::CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const
	{
		auto internal_state = std::make_shared<QueryHeap_DX12>();
		internal_state->allocationhandler = allocationhandler;
		queryheap->internal_state = internal_state;
		queryheap->desc = *desc;

		D3D12_QUERY_HEAP_DESC queryheapdesc = {};
		queryheapdesc.Count = desc->query_count;

		switch (desc->type)
		{
		default:
		case GpuQueryType::TIMESTAMP:
			queryheapdesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			break;
		case GpuQueryType::OCCLUSION:
		case GpuQueryType::OCCLUSION_BINARY:
			queryheapdesc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			break;
		}

		HRESULT hr = allocationhandler->device->CreateQueryHeap(&queryheapdesc, PPV_ARGS(internal_state->heap));
		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso, const RenderPassInfo* renderpass_info) const
	{
		auto internal_state = std::make_shared<PipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pso->internal_state = internal_state;

		pso->desc = *desc;

		internal_state->hash = 0;
		wi::helper::hash_combine(internal_state->hash, desc->ms);
		wi::helper::hash_combine(internal_state->hash, desc->as);
		wi::helper::hash_combine(internal_state->hash, desc->vs);
		wi::helper::hash_combine(internal_state->hash, desc->ps);
		wi::helper::hash_combine(internal_state->hash, desc->hs);
		wi::helper::hash_combine(internal_state->hash, desc->ds);
		wi::helper::hash_combine(internal_state->hash, desc->gs);
		wi::helper::hash_combine(internal_state->hash, desc->il);
		wi::helper::hash_combine(internal_state->hash, desc->rs);
		wi::helper::hash_combine(internal_state->hash, desc->bs);
		wi::helper::hash_combine(internal_state->hash, desc->dss);
		wi::helper::hash_combine(internal_state->hash, desc->pt);
		wi::helper::hash_combine(internal_state->hash, desc->sample_mask);

		auto& stream = internal_state->stream;
		if (pso->desc.vs != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.vs);
			stream.stream1.VS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}
		if (pso->desc.hs != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.hs);
			stream.stream1.HS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}
		if (pso->desc.ds != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.ds);
			stream.stream1.DS = { shader_internal->shadercode.data(),shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}
		if (pso->desc.gs != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.gs);
			stream.stream1.GS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}
		if (pso->desc.ps != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.ps);
			stream.stream1.PS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}

		if (pso->desc.ms != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.ms);
			stream.stream2.MS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}
		if (pso->desc.as != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.as);
			stream.stream2.AS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}

		assert(internal_state->rootSignature != nullptr);
		assert(internal_state->rootsig_desc != nullptr);
		internal_state->rootsig_optimizer.init(*internal_state->rootsig_desc);

		RasterizerState pRasterizerStateDesc = pso->desc.rs != nullptr ? *pso->desc.rs : RasterizerState();
		CD3DX12_RASTERIZER_DESC rs = {};
		rs.FillMode = _ConvertFillMode(pRasterizerStateDesc.fill_mode);
		rs.CullMode = _ConvertCullMode(pRasterizerStateDesc.cull_mode);
		rs.FrontCounterClockwise = pRasterizerStateDesc.front_counter_clockwise;
		rs.DepthBias = pRasterizerStateDesc.depth_bias;
		rs.DepthBiasClamp = pRasterizerStateDesc.depth_bias_clamp;
		rs.SlopeScaledDepthBias = pRasterizerStateDesc.slope_scaled_depth_bias;
		rs.DepthClipEnable = pRasterizerStateDesc.depth_clip_enable;
		rs.MultisampleEnable = pRasterizerStateDesc.multisample_enable;
		rs.AntialiasedLineEnable = pRasterizerStateDesc.antialiased_line_enable;
		rs.ConservativeRaster = ((CheckCapability(GraphicsDeviceCapability::CONSERVATIVE_RASTERIZATION) && pRasterizerStateDesc.conservative_rasterization_enable) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
		rs.ForcedSampleCount = pRasterizerStateDesc.forced_sample_count;
		stream.stream1.RS = rs;

		DepthStencilState pDepthStencilStateDesc = pso->desc.dss != nullptr ? *pso->desc.dss : DepthStencilState();
		CD3DX12_DEPTH_STENCIL_DESC1 dss = {};
		dss.DepthEnable = pDepthStencilStateDesc.depth_enable;
		dss.DepthWriteMask = _ConvertDepthWriteMask(pDepthStencilStateDesc.depth_write_mask);
		dss.DepthFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.depth_func);
		dss.StencilEnable = pDepthStencilStateDesc.stencil_enable;
		dss.StencilReadMask = pDepthStencilStateDesc.stencil_read_mask;
		dss.StencilWriteMask = pDepthStencilStateDesc.stencil_write_mask;
		dss.FrontFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc.front_face.stencil_depth_fail_op);
		dss.FrontFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc.front_face.stencil_fail_op);
		dss.FrontFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.front_face.stencil_func);
		dss.FrontFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc.front_face.stencil_pass_op);
		dss.BackFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc.back_face.stencil_depth_fail_op);
		dss.BackFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc.back_face.stencil_fail_op);
		dss.BackFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.back_face.stencil_func);
		dss.BackFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc.back_face.stencil_pass_op);
		if (CheckCapability(GraphicsDeviceCapability::DEPTH_BOUNDS_TEST))
		{
			dss.DepthBoundsTestEnable = pDepthStencilStateDesc.depth_bounds_test_enable;
		}
		else
		{
			dss.DepthBoundsTestEnable = FALSE;
		}
		stream.stream1.DSS = dss;

		BlendState pBlendStateDesc = pso->desc.bs != nullptr ? *pso->desc.bs : BlendState();
		CD3DX12_BLEND_DESC bd = {};
		bd.AlphaToCoverageEnable = pBlendStateDesc.alpha_to_coverage_enable;
		bd.IndependentBlendEnable = pBlendStateDesc.independent_blend_enable;
		for (int i = 0; i < 8; ++i)
		{
			bd.RenderTarget[i].BlendEnable = pBlendStateDesc.render_target[i].blend_enable;
			bd.RenderTarget[i].SrcBlend = _ConvertBlend(pBlendStateDesc.render_target[i].src_blend);
			bd.RenderTarget[i].DestBlend = _ConvertBlend(pBlendStateDesc.render_target[i].dest_blend);
			bd.RenderTarget[i].BlendOp = _ConvertBlendOp(pBlendStateDesc.render_target[i].blend_op);
			bd.RenderTarget[i].SrcBlendAlpha = _ConvertAlphaBlend(pBlendStateDesc.render_target[i].src_blend_alpha);
			bd.RenderTarget[i].DestBlendAlpha = _ConvertAlphaBlend(pBlendStateDesc.render_target[i].dest_blend_alpha);
			bd.RenderTarget[i].BlendOpAlpha = _ConvertBlendOp(pBlendStateDesc.render_target[i].blend_op_alpha);
			bd.RenderTarget[i].RenderTargetWriteMask = _ParseColorWriteMask(pBlendStateDesc.render_target[i].render_target_write_mask);
		}
		stream.stream1.BD = bd;

		auto& elements = internal_state->input_elements;
		D3D12_INPUT_LAYOUT_DESC il = {};
		if (pso->desc.il != nullptr)
		{
			il.NumElements = (uint32_t)pso->desc.il->elements.size();
			elements.resize(il.NumElements);
			for (uint32_t i = 0; i < il.NumElements; ++i)
			{
				elements[i].SemanticName = pso->desc.il->elements[i].semantic_name.c_str();
				elements[i].SemanticIndex = pso->desc.il->elements[i].semantic_index;
				elements[i].Format = _ConvertFormat(pso->desc.il->elements[i].format);
				elements[i].InputSlot = pso->desc.il->elements[i].input_slot;
				elements[i].AlignedByteOffset = pso->desc.il->elements[i].aligned_byte_offset;
				if (elements[i].AlignedByteOffset == InputLayout::APPEND_ALIGNED_ELEMENT)
					elements[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
				elements[i].InputSlotClass = _ConvertInputClassification(pso->desc.il->elements[i].input_slot_class);
				elements[i].InstanceDataStepRate = 0;
				if (elements[i].InputSlotClass == D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA)
				{
					elements[i].InstanceDataStepRate = 1;
				}
			}
		}
		il.pInputElementDescs = elements.data();
		stream.stream1.IL = il;

		stream.stream1.SampleMask = pso->desc.sample_mask;

		internal_state->primitiveTopology = _ConvertPrimitiveTopology(desc->pt, desc->patch_control_points);
		switch (pso->desc.pt)
		{
		case PrimitiveTopology::POINTLIST:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			break;
		case PrimitiveTopology::LINELIST:
		case PrimitiveTopology::LINESTRIP:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			break;
		case PrimitiveTopology::TRIANGLELIST:
		case PrimitiveTopology::TRIANGLESTRIP:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			break;
		case PrimitiveTopology::PATCHLIST:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			break;
		default:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
			break;
		}

		stream.stream1.STRIP = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

		if (renderpass_info != nullptr)
		{
			wi::helper::hash_combine(internal_state->hash, renderpass_info->get_hash());
			DXGI_FORMAT DSFormat = _ConvertFormat(renderpass_info->ds_format);
			D3D12_RT_FORMAT_ARRAY formats = {};
			formats.NumRenderTargets = renderpass_info->rt_count;
			for (uint32_t i = 0; i < renderpass_info->rt_count; ++i)
			{
				formats.RTFormats[i] = _ConvertFormat(renderpass_info->rt_formats[i]);
			}
			DXGI_SAMPLE_DESC sampleDesc = {};
			sampleDesc.Count = renderpass_info->sample_count;
			sampleDesc.Quality = 0;

			stream.stream1.DSFormat = DSFormat;
			stream.stream1.Formats = formats;
			stream.stream1.SampleDesc = sampleDesc;

			D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
			streamDesc.pPipelineStateSubobjectStream = &stream;
			streamDesc.SizeInBytes = sizeof(stream.stream1);
			if (CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
			{
				streamDesc.SizeInBytes += sizeof(stream.stream2);
			}

			HRESULT hr = device->CreatePipelineState(&streamDesc, PPV_ARGS(internal_state->resource));
			assert(SUCCEEDED(hr));
		}

		return true;
	}
	bool GraphicsDevice_DX12::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const
	{
		auto internal_state = std::make_shared<BVH_DX12>();
		internal_state->allocationhandler = allocationhandler;
		bvh->internal_state = internal_state;
		bvh->type = GPUResource::Type::RAYTRACING_ACCELERATION_STRUCTURE;
		bvh->desc = *desc;
		bvh->size = 0;

		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_COMPACTION)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
		}
		

		switch (desc->type)
		{
		case RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL:
		{
			internal_state->desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			internal_state->desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

			for (auto& x : desc->bottom_level.geometries)
			{
				auto& geometry = internal_state->geometries.emplace_back();
				geometry = {};

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES)
				{
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					geometry.Triangles.VertexBuffer.StartAddress = to_internal(&x.triangles.vertex_buffer)->gpu_address + (D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.vertex_byte_offset;
					geometry.Triangles.VertexBuffer.StrideInBytes = (UINT64)x.triangles.vertex_stride;
					geometry.Triangles.VertexCount = x.triangles.vertex_count;
					geometry.Triangles.VertexFormat = _ConvertFormat(x.triangles.vertex_format);
					geometry.Triangles.IndexFormat = (x.triangles.index_format == IndexBufferFormat::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
					geometry.Triangles.IndexBuffer = to_internal(&x.triangles.index_buffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.index_offset * (x.triangles.index_format == IndexBufferFormat::UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));
					geometry.Triangles.IndexCount = x.triangles.index_count;

					if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.Triangles.Transform3x4 = to_internal(&x.triangles.transform_3x4_buffer)->gpu_address +
							(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.transform_3x4_buffer_offset;
					}
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::PROCEDURAL_AABBS)
				{
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS; 
					geometry.AABBs.AABBs.StartAddress = to_internal(&x.aabbs.aabb_buffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.aabbs.offset;
					geometry.AABBs.AABBs.StrideInBytes = (UINT64)x.aabbs.stride;
					geometry.AABBs.AABBCount = x.aabbs.count;
				}
			}

			internal_state->desc.pGeometryDescs = internal_state->geometries.data();
			internal_state->desc.NumDescs = (UINT)internal_state->geometries.size();
		}
		break;
		case RaytracingAccelerationStructureDesc::Type::TOPLEVEL:
		{
			internal_state->desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
			internal_state->desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

			internal_state->desc.InstanceDescs = to_internal(&desc->top_level.instance_buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->top_level.offset;
			internal_state->desc.NumDescs = (UINT)desc->top_level.count;
		}
		break;
		}

		device->GetRaytracingAccelerationStructurePrebuildInfo(&internal_state->desc, &internal_state->info);


		UINT64 alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
		UINT64 alignedSize = AlignTo(internal_state->info.ResultDataMaxSizeInBytes, alignment);

		D3D12_RESOURCE_DESC resourcedesc;
		resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourcedesc.Format = DXGI_FORMAT_UNKNOWN;
		resourcedesc.Width = (UINT64)alignedSize;
		resourcedesc.Height = 1;
		resourcedesc.MipLevels = 1;
		resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourcedesc.DepthOrArraySize = 1;
		resourcedesc.Alignment = 0;
		resourcedesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		resourcedesc.SampleDesc.Count = 1;
		resourcedesc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		HRESULT hr = allocationhandler->allocator->CreateResource(
			&allocationDesc,
			&resourcedesc,
			resourceState,
			nullptr,
			&internal_state->allocation,
			PPV_ARGS(internal_state->resource)
		);
		assert(SUCCEEDED(hr));

		internal_state->gpu_address = internal_state->resource->GetGPUVirtualAddress();

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		srv_desc.RaytracingAccelerationStructure.Location = internal_state->gpu_address;

		internal_state->srv.init(this, srv_desc, nullptr);

		GPUBufferDesc scratch_desc;
		scratch_desc.size = (uint32_t)std::max(internal_state->info.ScratchDataSizeInBytes, internal_state->info.UpdateScratchDataSizeInBytes);

		bvh->size = alignedSize + scratch_desc.size;

		return CreateBuffer(&scratch_desc, nullptr, &internal_state->scratch);
	}
	bool GraphicsDevice_DX12::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const
	{
		auto internal_state = std::make_shared<RTPipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		rtpso->internal_state = internal_state;
		rtpso->desc = *desc;

		D3D12_STATE_OBJECT_DESC stateobjectdesc = {};
		stateobjectdesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

		wi::vector<D3D12_STATE_SUBOBJECT> subobjects;

		D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config = {};
		{
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
			pipeline_config.MaxTraceRecursionDepth = desc->max_trace_recursion_depth;
			subobject.pDesc = &pipeline_config;
		}

		D3D12_RAYTRACING_SHADER_CONFIG shader_config = {};
		{
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
			shader_config.MaxAttributeSizeInBytes = desc->max_attribute_size_in_bytes;
			shader_config.MaxPayloadSizeInBytes = desc->max_payload_size_in_bytes;
			subobject.pDesc = &shader_config;
		}

		D3D12_GLOBAL_ROOT_SIGNATURE global_rootsig = {};
		{
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
			auto shader_internal = to_internal(desc->shader_libraries.front().shader);
			global_rootsig.pGlobalRootSignature = shader_internal->rootSignature.Get();
			subobject.pDesc = &global_rootsig;
		}

		internal_state->exports.reserve(desc->shader_libraries.size());
		internal_state->library_descs.reserve(desc->shader_libraries.size());
		for(auto& x : desc->shader_libraries)
		{
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
			auto& library_desc = internal_state->library_descs.emplace_back();
			library_desc = {};
			auto shader_internal = to_internal(x.shader);
			library_desc.DXILLibrary.pShaderBytecode = shader_internal->shadercode.data();
			library_desc.DXILLibrary.BytecodeLength = shader_internal->shadercode.size();
			library_desc.NumExports = 1;

			D3D12_EXPORT_DESC& export_desc = internal_state->exports.emplace_back();
			wi::helper::StringConvert(x.function_name, internal_state->export_strings.emplace_back());
			export_desc.Name = internal_state->export_strings.back().c_str();
			library_desc.pExports = &export_desc;

			subobject.pDesc = &library_desc;
		}

		internal_state->hitgroup_descs.reserve(desc->hit_groups.size());
		for (auto& x : desc->hit_groups)
		{
			wi::helper::StringConvert(x.name, internal_state->group_strings.emplace_back());

			if (x.type == ShaderHitGroup::Type::GENERAL)
				continue;
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			auto& hitgroup_desc = internal_state->hitgroup_descs.emplace_back();
			hitgroup_desc = {};
			switch (x.type)
			{
			default:
			case ShaderHitGroup::Type::TRIANGLES:
				hitgroup_desc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
				break;
			case ShaderHitGroup::Type::PROCEDURAL:
				hitgroup_desc.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
				break;
			}
			if (!x.name.empty())
			{
				hitgroup_desc.HitGroupExport = internal_state->group_strings.back().c_str();
			}
			if (x.closest_hit_shader != ~0)
			{
				hitgroup_desc.ClosestHitShaderImport = internal_state->exports[x.closest_hit_shader].Name;
			}
			if (x.any_hit_shader != ~0)
			{
				hitgroup_desc.AnyHitShaderImport = internal_state->exports[x.any_hit_shader].Name;
			}
			if (x.intersection_shader != ~0)
			{
				hitgroup_desc.IntersectionShaderImport = internal_state->exports[x.intersection_shader].Name;
			}
			subobject.pDesc = &hitgroup_desc;
		}

		stateobjectdesc.NumSubobjects = (UINT)subobjects.size();
		stateobjectdesc.pSubobjects = subobjects.data();

		HRESULT hr = device->CreateStateObject(&stateobjectdesc, PPV_ARGS(internal_state->resource));
		assert(SUCCEEDED(hr));

		hr = internal_state->resource.As(&internal_state->stateObjectProperties);
		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const
	{
		if (video_device == nullptr)
			return false;

		HRESULT hr = E_FAIL;

		D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILE_COUNT video_decode_profile_count = {};
		hr = video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_PROFILE_COUNT, &video_decode_profile_count, sizeof(video_decode_profile_count));
		assert(SUCCEEDED(hr));

		wi::vector<GUID> profiles(video_decode_profile_count.ProfileCount);
		D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILES video_decode_profiles = {};
		video_decode_profiles.ProfileCount = video_decode_profile_count.ProfileCount;
		video_decode_profiles.pProfiles = profiles.data();
		hr = video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_PROFILES, &video_decode_profiles, sizeof(video_decode_profiles));
		assert(SUCCEEDED(hr));

		D3D12_VIDEO_DECODER_DESC decoder_desc = {};
		switch (desc->profile)
		{
		case VideoProfile::H264:
			decoder_desc.Configuration.DecodeProfile = D3D12_VIDEO_DECODE_PROFILE_H264;
			decoder_desc.Configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;
			break;
		//case VideoProfile::H265:
		//	decoder_desc.Configuration.DecodeProfile = D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN;
		//	break;
		default:
			assert(0); // not implemented
			break;
		}
		bool profile_valid = false;
		for (auto& x : profiles)
		{
			if (x == decoder_desc.Configuration.DecodeProfile)
			{
				profile_valid = true;
				break;
			}
		}
		if (!profile_valid)
			return false;

		D3D12_FEATURE_DATA_VIDEO_DECODE_FORMAT_COUNT video_decode_format_count = {};
		video_decode_format_count.Configuration = decoder_desc.Configuration;
		hr = video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_FORMAT_COUNT, &video_decode_format_count, sizeof(video_decode_format_count));
		assert(SUCCEEDED(hr));

		wi::vector<DXGI_FORMAT> formats(video_decode_format_count.FormatCount);
		D3D12_FEATURE_DATA_VIDEO_DECODE_FORMATS video_decode_formats = {};
		video_decode_formats.Configuration = decoder_desc.Configuration;
		video_decode_formats.FormatCount = video_decode_format_count.FormatCount;
		video_decode_formats.pOutputFormats = formats.data();
		hr = video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_FORMATS, &video_decode_formats, sizeof(video_decode_formats));
		assert(SUCCEEDED(hr));

		D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT video_decode_support = {};
		video_decode_support.Configuration = decoder_desc.Configuration;
		video_decode_support.DecodeFormat = _ConvertFormat(desc->format);
		bool format_valid = false;
		for (auto& x : formats)
		{
			if (x == video_decode_support.DecodeFormat)
			{
				format_valid = true;
				break;
			}
		}
		if (!format_valid)
			return false;
		video_decode_support.Width = desc->width;
		video_decode_support.Height = desc->height;
		video_decode_support.BitRate = desc->bit_rate;
		video_decode_support.FrameRate = { 0, 1 };
		hr = video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT, &video_decode_support, sizeof(video_decode_support));
		assert(SUCCEEDED(hr));

		bool reference_only = video_decode_support.ConfigurationFlags & D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_REFERENCE_ONLY_ALLOCATIONS_REQUIRED;
		assert(!reference_only); // Not supported currently, will need to use resource flags: D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, and do output decode conversion

		if (video_decode_support.DecodeTier < D3D12_VIDEO_DECODE_TIER_1)
			return false;

		auto internal_state = std::make_shared<VideoDecoder_DX12>();
		internal_state->allocationhandler = allocationhandler;
		video_decoder->internal_state = internal_state;
		video_decoder->desc = *desc;

		D3D12_VIDEO_DECODER_HEAP_DESC heap_desc = {};
		heap_desc.Configuration = decoder_desc.Configuration;
		heap_desc.DecodeWidth = video_decode_support.Width;
		heap_desc.DecodeHeight = video_decode_support.Height;
		heap_desc.Format = video_decode_support.DecodeFormat;
		heap_desc.FrameRate = { 0,1 };
		heap_desc.BitRate = 0;
		heap_desc.MaxDecodePictureBufferCount = desc->num_dpb_slots;

		if (video_decode_support.ConfigurationFlags & D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_HEIGHT_ALIGNMENT_MULTIPLE_32_REQUIRED)
		{
			heap_desc.DecodeWidth = AlignTo(video_decode_support.Width, 32u);
			heap_desc.DecodeHeight = AlignTo(video_decode_support.Height, 32u);
		}

#if 0
		D3D12_FEATURE_DATA_VIDEO_DECODER_HEAP_SIZE video_decoder_heap_size = {};
		video_decoder_heap_size.VideoDecoderHeapDesc = heap_desc;
		hr = video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODER_HEAP_SIZE, &video_decoder_heap_size, sizeof(video_decoder_heap_size));
		assert(SUCCEEDED(hr));
#endif

		hr = video_device->CreateVideoDecoderHeap(&heap_desc, PPV_ARGS(internal_state->decoder_heap));
		assert(SUCCEEDED(hr));
		hr = video_device->CreateVideoDecoder(&decoder_desc, PPV_ARGS(internal_state->decoder));
		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}

	int GraphicsDevice_DX12::CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change, const ImageAspect* aspect, const Swizzle* swizzle) const
	{
		auto internal_state = to_internal(texture);

		Format format = texture->GetDesc().format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}
		UINT plane = 0;
		if (aspect != nullptr)
		{
			plane = _ImageAspectToPlane(*aspect);
		}

		switch (type)
		{
		case SubresourceType::SRV:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = _ConvertSwizzle(swizzle == nullptr ? texture->desc.swizzle : *swizzle);

			// Try to resolve resource format:
			switch (format)
			{
			case Format::D16_UNORM:
				srv_desc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			case Format::D32_FLOAT:
				srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case Format::D24_UNORM_S8_UINT:
				srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case Format::D32_FLOAT_S8X24_UINT:
				srv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			case Format::NV12:
				srv_desc.Format = DXGI_FORMAT_R8_UNORM;
				break;
			default:
				srv_desc.Format = _ConvertFormat(format);
				break;
			}

			if (texture->desc.type == TextureDesc::Type::TEXTURE_1D)
			{
				if (texture->desc.array_size > 1)
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
			else if (texture->desc.type == TextureDesc::Type::TEXTURE_2D)
			{
				if (texture->desc.array_size > 1)
				{
					if (has_flag(texture->desc.misc_flags, ResourceMiscFlag::TEXTURECUBE))
					{
						if (texture->desc.array_size > 6)
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
							srv_desc.TextureCubeArray.First2DArrayFace = firstSlice;
							srv_desc.TextureCubeArray.NumCubes = std::min(texture->desc.array_size, sliceCount) / 6;
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
						if (texture->desc.sample_count > 1)
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
							srv_desc.Texture2DArray.PlaneSlice = plane;
						}
					}
				}
				else
				{
					if (texture->desc.sample_count > 1)
					{
						srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						srv_desc.Texture2D.MostDetailedMip = firstMip;
						srv_desc.Texture2D.MipLevels = mipCount;
						srv_desc.Texture2D.PlaneSlice = plane;
					}
				}
			}
			else if (texture->desc.type == TextureDesc::Type::TEXTURE_3D)
			{
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				srv_desc.Texture3D.MostDetailedMip = firstMip;
				srv_desc.Texture3D.MipLevels = mipCount;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, srv_desc, internal_state->resource.Get());
			descriptor.firstMip = firstMip;
			descriptor.mipCount = mipCount;
			descriptor.firstSlice = firstSlice;
			descriptor.sliceCount = sliceCount;

			if (!internal_state->srv.IsValid())
			{
				internal_state->srv = descriptor;
				return -1;
			}
			internal_state->subresources_srv.push_back(descriptor);
			return int(internal_state->subresources_srv.size() - 1);
		}
		break;
		case SubresourceType::UAV:
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = _ConvertFormat(format);

			if (texture->desc.type == TextureDesc::Type::TEXTURE_1D)
			{
				if (texture->desc.array_size > 1)
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
			else if (texture->desc.type == TextureDesc::Type::TEXTURE_2D)
			{
				if (texture->desc.array_size > 1)
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
			else if (texture->desc.type == TextureDesc::Type::TEXTURE_3D)
			{
				uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				uav_desc.Texture3D.MipSlice = firstMip;
				uav_desc.Texture3D.FirstWSlice = 0;
				uav_desc.Texture3D.WSize = -1;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, uav_desc, internal_state->resource.Get());
			descriptor.firstMip = firstMip;
			descriptor.mipCount = mipCount;
			descriptor.firstSlice = firstSlice;
			descriptor.sliceCount = sliceCount;

			if (!internal_state->uav.IsValid())
			{
				internal_state->uav = descriptor;
				return -1;
			}
			internal_state->subresources_uav.push_back(descriptor);
			return int(internal_state->subresources_uav.size() - 1);
		}
		break;
		case SubresourceType::RTV:
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
			rtv_desc.Format = _ConvertFormat(format);

			if (texture->desc.type == TextureDesc::Type::TEXTURE_1D)
			{
				if (texture->desc.array_size > 1)
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
			else if (texture->desc.type == TextureDesc::Type::TEXTURE_2D)
			{
				if (texture->desc.array_size > 1)
				{
					if (texture->desc.sample_count > 1)
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
					if (texture->desc.sample_count > 1)
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
			else if (texture->desc.type == TextureDesc::Type::TEXTURE_3D)
			{
				rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
				rtv_desc.Texture3D.MipSlice = firstMip;
				rtv_desc.Texture3D.FirstWSlice = 0;
				rtv_desc.Texture3D.WSize = -1;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, rtv_desc, internal_state->resource.Get());
			descriptor.firstMip = firstMip;
			descriptor.mipCount = mipCount;
			descriptor.firstSlice = firstSlice;
			descriptor.sliceCount = sliceCount;

			if (!internal_state->rtv.IsValid())
			{
				internal_state->rtv = descriptor;
				return -1;
			}
			internal_state->subresources_rtv.push_back(descriptor);
			return int(internal_state->subresources_rtv.size() - 1);
		}
		break;
		case SubresourceType::DSV:
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
			dsv_desc.Format = _ConvertFormat(format);

			if (texture->desc.type == TextureDesc::Type::TEXTURE_1D)
			{
				if (texture->desc.array_size > 1)
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
			else if (texture->desc.type == TextureDesc::Type::TEXTURE_2D)
			{
				if (texture->desc.array_size > 1)
				{
					if (texture->desc.sample_count > 1)
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
					if (texture->desc.sample_count > 1)
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
			descriptor.firstMip = firstMip;
			descriptor.mipCount = mipCount;
			descriptor.firstSlice = firstSlice;
			descriptor.sliceCount = sliceCount;

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
	int GraphicsDevice_DX12::CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size, const Format* format_change, const uint32_t* structuredbuffer_stride_change) const
	{
		auto internal_state = to_internal(buffer);
		const GPUBufferDesc& desc = buffer->GetDesc();

		Format format = desc.format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}
		if (type == SubresourceType::UAV)
		{
			// RW resource can't be SRGB
			format = GetFormatNonSRGB(format);
		}

		switch (type)
		{

		case SubresourceType::SRV:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			if (format == Format::UNKNOWN)
			{
				if (has_flag(desc.misc_flags, ResourceMiscFlag::BUFFER_STRUCTURED) || (structuredbuffer_stride_change != nullptr))
				{
					// This is a Structured Buffer
					uint32_t stride = desc.stride;
					if (structuredbuffer_stride_change != nullptr)
					{
						stride = *structuredbuffer_stride_change;
					}
					assert(IsAligned(offset, (uint64_t)stride)); // structured buffer offset must be aligned to structure stride!
					srv_desc.Format = DXGI_FORMAT_UNKNOWN;
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					srv_desc.Buffer.FirstElement = UINT(offset / stride);
					srv_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / stride);
					srv_desc.Buffer.StructureByteStride = stride;
				}
				else
				{
					// This is a Raw Buffer
					assert(has_flag(desc.misc_flags, ResourceMiscFlag::BUFFER_RAW));
					srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					srv_desc.Buffer.FirstElement = UINT(offset / sizeof(uint32_t));
					srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
					srv_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / sizeof(uint32_t));
				}
			}
			else
			{
				// This is a Typed Buffer
				uint32_t stride = GetFormatStride(format);
				srv_desc.Format = _ConvertFormat(format);
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = UINT(offset / stride);
				srv_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / stride);
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
		case SubresourceType::UAV:
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;

			if (format == Format::UNKNOWN)
			{
				if (has_flag(desc.misc_flags, ResourceMiscFlag::BUFFER_STRUCTURED) || (structuredbuffer_stride_change != nullptr))
				{
					// This is a Structured Buffer
					uint32_t stride = desc.stride;
					if (structuredbuffer_stride_change != nullptr)
					{
						stride = *structuredbuffer_stride_change;
					}
					assert(IsAligned(offset, (uint64_t)stride)); // structured buffer offset must be aligned to structure stride!
					uav_desc.Format = DXGI_FORMAT_UNKNOWN;
					uav_desc.Buffer.FirstElement = UINT(offset / stride);
					uav_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / stride);
					uav_desc.Buffer.StructureByteStride = stride;
				}
				else
				{
					// This is a Raw Buffer
					assert(has_flag(desc.misc_flags, ResourceMiscFlag::BUFFER_RAW));
					uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
					uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
					uav_desc.Buffer.FirstElement = UINT(offset / sizeof(uint32_t));
					uav_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / sizeof(uint32_t));
				}
			}
			else
			{
				// This is a Typed Buffer
				uint32_t stride = GetFormatStride(format);
				uav_desc.Format = _ConvertFormat(format);
				uav_desc.Buffer.FirstElement = UINT(offset / stride);
				uav_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / stride);
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

	int GraphicsDevice_DX12::GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource) const
	{
		if (resource == nullptr || !resource->IsValid())
			return -1;

		auto internal_state = to_internal(resource);

		switch (type)
		{
		default:
		case SubresourceType::SRV:
			if (subresource < 0)
			{
				return internal_state->srv.index;
			}
			else
			{
				return internal_state->subresources_srv[subresource].index;
			}
			break;
		case SubresourceType::UAV:
			if (subresource < 0)
			{
				return internal_state->uav.index;
			}
			else
			{
				return internal_state->subresources_uav[subresource].index;
			}
			break;
		}

		return -1;
	}
	int GraphicsDevice_DX12::GetDescriptorIndex(const Sampler* sampler) const
	{
		if (sampler == nullptr || !sampler->IsValid())
			return -1;

		auto internal_state = to_internal(sampler);
		return internal_state->descriptor.index;
	}

	void GraphicsDevice_DX12::WriteShadingRateValue(ShadingRate rate, void* dest) const
	{
		D3D12_SHADING_RATE _rate = _ConvertShadingRate(rate);
		if (!additionalShadingRatesSupported)
		{
			_rate = std::min(_rate, D3D12_SHADING_RATE_2X2);
		}
		*(uint8_t*)dest = _rate;
	}
	void GraphicsDevice_DX12::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const
	{
		D3D12_RAYTRACING_INSTANCE_DESC tmp;
		tmp.AccelerationStructure = to_internal(instance->bottom_level)->gpu_address;
		std::memcpy(tmp.Transform, &instance->transform, sizeof(tmp.Transform));
		tmp.InstanceID = instance->instance_id;
		tmp.InstanceMask = instance->instance_mask;
		tmp.InstanceContributionToHitGroupIndex = instance->instance_contribution_to_hit_group_index;
		tmp.Flags = instance->flags;

		std::memcpy(dest, &tmp, sizeof(D3D12_RAYTRACING_INSTANCE_DESC)); // memcpy whole structure into mapped pointer to avoid read from uncached memory
	}
	void GraphicsDevice_DX12::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const
	{
		auto internal_state = to_internal(rtpso);

		const void* identifier = internal_state->stateObjectProperties->GetShaderIdentifier(internal_state->group_strings[group_index].c_str());
		std::memcpy(dest, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}
	
	void GraphicsDevice_DX12::SetName(GPUResource* pResource, const char* name) const
	{
		wchar_t text[256];
		if (wi::helper::StringConvert(name, text, arraysize(text)) > 0)
		{
			auto internal_state = to_internal(pResource);
			if (internal_state->resource != nullptr)
			{
				internal_state->resource->SetName(text);
			}
		}
	}

	CommandList GraphicsDevice_DX12::BeginCommandList(QUEUE_TYPE queue)
	{
		HRESULT hr;

		cmd_locker.lock();
		uint32_t cmd_current = cmd_count++;
		if (cmd_current >= commandlists.size())
		{
			commandlists.push_back(std::make_unique<CommandList_DX12>());
		}
		CommandList cmd;
		cmd.internal_state = commandlists[cmd_current].get();
		cmd_locker.unlock();

		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.reset(GetBufferIndex());
		commandlist.queue = queue;
		commandlist.id = cmd_current;
		commandlist.waited_on.store(false);

		if (commandlist.GetCommandList() == nullptr)
		{
			// need to create one more command list:

			for (uint32_t buffer = 0; buffer < BUFFERCOUNT; ++buffer)
			{
				hr = device->CreateCommandAllocator(queues[queue].desc.Type, PPV_ARGS(commandlist.commandAllocators[buffer][queue]));
				assert(SUCCEEDED(hr));
			}

			if (queue == QUEUE_VIDEO_DECODE)
			{
				ComPtr<ID3D12VideoDecodeCommandList> videoCommandList;
#ifdef PLATFORM_XBOX
				hr = device->CreateCommandList(0, queues[queue].desc.Type, commandlist.commandAllocators[0][queue].Get(), nullptr, PPV_ARGS(videoCommandList));
				assert(SUCCEEDED(hr));
				hr = videoCommandList->Close();
#else
				hr = device->CreateCommandList1(0, queues[queue].desc.Type, D3D12_COMMAND_LIST_FLAG_NONE, PPV_ARGS(videoCommandList));
#endif // PLATFORM_XBOX
				commandlist.commandLists[queue] = videoCommandList;
			}
			else if (queue == QUEUE_COPY)
			{
				ComPtr<ID3D12GraphicsCommandList> copyCommandList;
#ifdef PLATFORM_XBOX
				hr = device->CreateCommandList(0, queues[queue].desc.Type, commandlist.commandAllocators[0][queue].Get(), nullptr, PPV_ARGS(copyCommandList));
				assert(SUCCEEDED(hr));
				hr = copyCommandList->Close();
#else
				hr = device->CreateCommandList1(0, queues[queue].desc.Type, D3D12_COMMAND_LIST_FLAG_NONE, PPV_ARGS(copyCommandList));
#endif // PLATFORM_XBOX
				commandlist.commandLists[queue] = copyCommandList;
			}
			else
			{
				ComPtr<CommandList_DX12::graphics_command_list_version> graphicsCommandList;
#ifdef PLATFORM_XBOX
				hr = device->CreateCommandList(0, queues[queue].desc.Type, commandlist.commandAllocators[0][queue].Get(), nullptr, PPV_ARGS(graphicsCommandList));
				assert(SUCCEEDED(hr));
				hr = graphicsCommandList->Close();
#else
				hr = device->CreateCommandList1(0, queues[queue].desc.Type, D3D12_COMMAND_LIST_FLAG_NONE, PPV_ARGS(graphicsCommandList));
#endif // PLATFORM_XBOX
				commandlist.commandLists[queue] = graphicsCommandList;
			}
			assert(SUCCEEDED(hr));

			std::wstring ws = L"cmd" + std::to_wstring(commandlist.id);
			commandlist.GetCommandList()->SetName(ws.c_str());

			commandlist.binder.init(this);
		}

		// Start the command list in a default state:
		hr = commandlist.GetCommandAllocator()->Reset();
		assert(SUCCEEDED(hr));

		if (queue == QUEUE_VIDEO_DECODE)
		{
			hr = commandlist.GetVideoDecodeCommandList()->Reset(commandlist.GetCommandAllocator());
			assert(SUCCEEDED(hr));
		}
		else
		{
			hr = commandlist.GetGraphicsCommandList()->Reset(commandlist.GetCommandAllocator(), nullptr);
			assert(SUCCEEDED(hr));
		}

		if (queue == QUEUE_GRAPHICS || queue == QUEUE_COMPUTE)
		{
			ID3D12DescriptorHeap* heaps[] = {
				descriptorheap_res.heap_GPU.Get(),
				descriptorheap_sam.heap_GPU.Get()
			};
			commandlist.GetGraphicsCommandList()->SetDescriptorHeaps(arraysize(heaps), heaps);
		}

		if (queue == QUEUE_GRAPHICS)
		{
			D3D12_RECT pRects[D3D12_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1];
			for (uint32_t i = 0; i < arraysize(pRects); ++i)
			{
				pRects[i].left = 0;
				pRects[i].right = 16384;
				pRects[i].top = 0;
				pRects[i].bottom = 16384;
			}
			commandlist.GetGraphicsCommandList()->RSSetScissorRects(arraysize(pRects), pRects);
		}

		return cmd;
	}
	void GraphicsDevice_DX12::SubmitCommandLists()
	{
#ifdef PLATFORM_XBOX
		std::scoped_lock lock(queue_locker); // queue operations are not thread-safe on XBOX
#endif // PLATFORM_XBOX

		HRESULT hr;

		// Submit current frame:
		{
			uint32_t cmd_last = cmd_count;
			cmd_count = 0;
			for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
			{
				CommandList_DX12& commandlist = *commandlists[cmd].get();
				if (commandlist.queue == QUEUE_VIDEO_DECODE)
				{
					hr = commandlist.GetVideoDecodeCommandList()->Close();
				}
				else
				{
					hr = commandlist.GetGraphicsCommandList()->Close();
				}
				assert(SUCCEEDED(hr));

				CommandQueue& queue = queues[commandlist.queue];
				queue.submit_cmds.push_back(commandlist.GetCommandList());

				if (commandlist.waited_on.load() || !commandlist.waits.empty())
				{
					for (auto& wait : commandlist.waits)
					{
						// record wait for signal on a previous submit:
						const CommandList_DX12& waitcommandlist = GetCommandList(wait);
						hr = queue.queue->Wait(
							queues[waitcommandlist.queue].fence.Get(),
							FRAMECOUNT * commandlists.size() + (uint64_t)waitcommandlist.id
						);
						assert(SUCCEEDED(hr));
					}

					if (!queue.submit_cmds.empty())
					{
						queue.queue->ExecuteCommandLists(
							(UINT)queue.submit_cmds.size(),
							queue.submit_cmds.data()
						);
						queue.submit_cmds.clear();
					}

					if (commandlist.waited_on.load())
					{
						hr = queue.queue->Signal(
							queue.fence.Get(),
							FRAMECOUNT * commandlists.size() + (uint64_t)commandlist.id
						);
						assert(SUCCEEDED(hr));
					}
				}

				for (auto& x : commandlist.pipelines_worker)
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
				commandlist.pipelines_worker.clear();
			}

			// Mark the completion of queues for this frame:
			for (int q = 0; q < QUEUE_COUNT; ++q)
			{
				CommandQueue& queue = queues[q];

				if (!queue.submit_cmds.empty())
				{
					queue.queue->ExecuteCommandLists(
						(UINT)queue.submit_cmds.size(),
						queue.submit_cmds.data()
					);
					queue.submit_cmds.clear();
				}

				hr = queue.queue->Signal(frame_fence[GetBufferIndex()][q].Get(), 1);
				assert(SUCCEEDED(hr));
			}

			for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
			{
				CommandList_DX12& commandlist = *commandlists[cmd].get();
				for (auto& swapchain : commandlist.swapchains)
				{
					auto swapchain_internal = to_internal(swapchain);

#ifdef PLATFORM_XBOX

					wi::graphics::xbox::Present(
						device.Get(),
						queues[QUEUE_GRAPHICS].queue.Get(),
						swapchain_internal->backBuffers[swapchain_internal->bufferIndex].Get(),
						swapchain->desc.vsync
					);

					swapchain_internal->bufferIndex = (swapchain_internal->bufferIndex + 1) % (uint32_t)swapchain_internal->backBuffers.size();

#else
					UINT presentFlags = 0;
					if (!swapchain->desc.vsync && !swapchain->desc.fullscreen)
					{
						presentFlags = DXGI_PRESENT_ALLOW_TEARING;
					}

					hr = swapchain_internal->swapChain->Present(swapchain->desc.vsync, presentFlags);

					// If the device was reset we must completely reinitialize the renderer.
					if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
					{
#ifdef _DEBUG
						char buff[64] = {};
						sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
							static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? device->GetDeviceRemovedReason() : hr));
						OutputDebugStringA(buff);
#endif
						// Handle device lost
						OnDeviceRemoved();
					}
#endif // PLATFORM_XBOX
				}
			}
		}

		descriptorheap_res.SignalGPU(queues[QUEUE_GRAPHICS].queue.Get());
		descriptorheap_sam.SignalGPU(queues[QUEUE_GRAPHICS].queue.Get());

		// From here, we begin a new frame, this affects GetBufferIndex()!
		FRAMECOUNT++;

		// Initiate stalling CPU when GPU is not yet finished with next frame:
		const uint32_t bufferindex = GetBufferIndex();
		for (int queue = 0; queue < QUEUE_COUNT; ++queue)
		{
			if (FRAMECOUNT >= BUFFERCOUNT && frame_fence[bufferindex][queue]->GetCompletedValue() < 1)
			{
				// NULL event handle will simply wait immediately:
				//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
				hr = frame_fence[bufferindex][queue]->SetEventOnCompletion(1, NULL);
				assert(SUCCEEDED(hr));
			}
			hr = frame_fence[bufferindex][queue]->Signal(0);
		}
		assert(SUCCEEDED(hr));

		allocationhandler->Update(FRAMECOUNT, BUFFERCOUNT);
	}

	void GraphicsDevice_DX12::OnDeviceRemoved()
	{
#ifdef PLATFORM_WINDOWS_DESKTOP
		std::lock_guard<std::mutex> lock(onDeviceRemovedMutex);

		if (deviceRemoved)
		{
			return;
		}
		deviceRemoved = true;

		const char* removedReasonString;
		HRESULT removedReason = device->GetDeviceRemovedReason();

		switch (removedReason)
		{
		case DXGI_ERROR_DEVICE_HUNG:
			removedReasonString = "DXGI_ERROR_DEVICE_HUNG";
			break;
		case DXGI_ERROR_DEVICE_REMOVED:
			removedReasonString = "DXGI_ERROR_DEVICE_REMOVED";
			break;
		case DXGI_ERROR_DEVICE_RESET:
			removedReasonString = "DXGI_ERROR_DEVICE_RESET";
			break;
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			removedReasonString = "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
			break;
		case DXGI_ERROR_INVALID_CALL:
			removedReasonString = "DXGI_ERROR_INVALID_CALL";
			break;
		case DXGI_ERROR_ACCESS_DENIED:
			removedReasonString = "DXGI_ERROR_ACCESS_DENIED";
			break;
		default:
			removedReasonString = "Unknown DXGI error";
			break;
		}


		// Based on: https://github.com/simco50/D3D12_Research/blob/869373829444f029eb3a6ce95dd3aa541c665bb2/D3D12/Graphics/RHI/Graphics.cpp

		//D3D12_AUTO_BREADCRUMB_OP
		constexpr const char* OpNames[] =
		{
			"SetMarker",
			"BeginEvent",
			"EndEvent",
			"DrawInstanced",
			"DrawIndexedInstanced",
			"ExecuteIndirect",
			"Dispatch",
			"CopyBufferRegion",
			"CopyTextureRegion",
			"CopyResource",
			"CopyTiles",
			"ResolveSubresource",
			"ClearRenderTargetView",
			"ClearUnorderedAccessView",
			"ClearDepthStencilView",
			"ResourceBarrier",
			"ExecuteBundle",
			"Present",
			"ResolveQueryData",
			"BeginSubmission",
			"EndSubmission",
			"DecodeFrame",
			"ProcessFrames",
			"AtomicCopyBufferUint",
			"AtomicCopyBufferUint64",
			"ResolveSubresourceRegion",
			"WriteBufferImmediate",
			"DecodeFrame1",
			"SetProtectedResourceSession",
			"DecodeFrame2",
			"ProcessFrames1",
			"BuildRaytracingAccelerationStructure",
			"EmitRaytracingAccelerationStructurePostBuildInfo",
			"CopyRaytracingAccelerationStructure",
			"DispatchRays",
			"InitializeMetaCommand",
			"ExecuteMetaCommand",
			"EstimateMotion",
			"ResolveMotionVectorHeap",
			"SetPipelineState1",
			"InitializeExtensionCommand",
			"ExecuteExtensionCommand",
			"DispatchMesh",
			"EncodeFrame",
			"ResolveEncoderOutputMetadata",
		};
		static_assert(ARRAYSIZE(OpNames) == D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA + 1, "OpNames array length mismatch");

		//D3D12_DRED_ALLOCATION_TYPE
		constexpr const char* AllocTypesNames[] =
		{
			"CommandQueue",
			"CommandAllocator",
			"PipelineState",
			"CommandList",
			"Fence",
			"DescriptorHeap",
			"Heap",
			"Unknown",
			"QueryHeap",
			"CommandSignature",
			"PipelineLibrary",
			"VideoDecoder",
			"Unknown",
			"VideoProcessor",
			"Unknown",
			"Resource",
			"Pass",
			"CryptoSession",
			"CryptoSessionPolicy",
			"ProtectedResourceSession",
			"VideoDecoderHeap",
			"CommandPool",
			"CommandRecorder",
			"StateObjectr",
			"MetaCommand",
			"SchedulingGroup",
			"VideoMotionEstimator",
			"VideoMotionVectorHeap",
			"VideoExtensionCommand",
			"VideoEncoder",
			"VideoEncoderHeap",
		};
		static_assert(ARRAYSIZE(AllocTypesNames) == D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP - D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE + 1, "AllocTypes array length mismatch");

		std::string log;

		ComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
		if (SUCCEEDED(device->QueryInterface(PPV_ARGS(pDred))))
		{
			D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 pDredAutoBreadcrumbsOutput = {};
			if (SUCCEEDED(pDred->GetAutoBreadcrumbsOutput1(&pDredAutoBreadcrumbsOutput)))
			{
				log += "[DRED] Last tracked GPU operations:\n";

				std::map<int, const wchar_t*> contextStrings;

				const D3D12_AUTO_BREADCRUMB_NODE1* pNode = pDredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
				while (pNode && pNode->pLastBreadcrumbValue)
				{
					int lastCompletedOp = *pNode->pLastBreadcrumbValue;

					log += std::string("[DRED] Commandlist = [") + (pNode->pCommandListDebugNameA == nullptr ? "-" : pNode->pCommandListDebugNameA) + std::string("], CommandQueue = [") + (pNode->pCommandQueueDebugNameA == nullptr ? "-" : pNode->pCommandQueueDebugNameA) + std::string("], lastCompletedOp = [") + std::to_string(lastCompletedOp) + "], BreadCrumbCount = [" + std::to_string(pNode->BreadcrumbCount) + "]\n";

					int firstOp = std::max(lastCompletedOp - 100, 0);
					int lastOp = std::min(lastCompletedOp + 20, int(pNode->BreadcrumbCount) - 1);

					contextStrings.clear();
					for (uint32_t breadcrumbContext = firstOp; breadcrumbContext < pNode->BreadcrumbContextsCount; ++breadcrumbContext)
					{
						const D3D12_DRED_BREADCRUMB_CONTEXT& context = pNode->pBreadcrumbContexts[breadcrumbContext];
						contextStrings[context.BreadcrumbIndex] = context.pContextString;
					}

					for (int op = firstOp; op <= lastOp; ++op)
					{
						D3D12_AUTO_BREADCRUMB_OP breadcrumbOp = pNode->pCommandHistory[op];

						std::string contextString;
						auto it = contextStrings.find(op);
						if (it != contextStrings.end())
						{
							wi::helper::StringConvert(it->second, contextString);
						}

						const char* opName = (breadcrumbOp < arraysize(OpNames)) ? OpNames[breadcrumbOp] : "Unknown Op";
						log += "\tOp: " + std::to_string(op) + " " + opName + " " + contextString + "\n";
					}

					if (lastCompletedOp != (int)pNode->BreadcrumbCount)
					{
						log += "ERROR: lastCompletedOp = " + std::to_string(lastCompletedOp) + ", BreadcrumbCount = " + std::to_string((int)pNode->BreadcrumbCount) + "\n";
					}

					pNode = pNode->pNext;
				}
			}

			D3D12_DRED_PAGE_FAULT_OUTPUT1 DredPageFaultOutput = {};
			if (SUCCEEDED(pDred->GetPageFaultAllocationOutput1(&DredPageFaultOutput)) && DredPageFaultOutput.PageFaultVA != 0)
			{
				log += "[DRED] PageFault at VA GPUAddress " + std::to_string(DredPageFaultOutput.PageFaultVA) + "\n";

				const D3D12_DRED_ALLOCATION_NODE1* pNode = DredPageFaultOutput.pHeadExistingAllocationNode;
				if (pNode)
				{
					log += "[DRED] Active objects with VA ranges that match the faulting VA:\n";
					while (pNode)
					{
						uint32_t alloc_type_index = pNode->AllocationType - D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE;
						const char* AllocTypeName = (alloc_type_index < arraysize(AllocTypesNames)) ? AllocTypesNames[alloc_type_index] : "Unknown Alloc";
						log += std::string("\tName: ") + pNode->ObjectNameA + std::string(" ") + AllocTypeName + std::string("\n");
						pNode = pNode->pNext;
					}
				}

				pNode = DredPageFaultOutput.pHeadRecentFreedAllocationNode;
				if (pNode)
				{
					log +=  "[DRED] Recent freed objects with VA ranges that match the faulting VA:\n";
					while (pNode)
					{
						uint32_t allocTypeIndex = pNode->AllocationType - D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE;
						const char* AllocTypeName = (allocTypeIndex < arraysize(AllocTypesNames)) ? AllocTypesNames[allocTypeIndex] : "Unknown Alloc";
						log += std::string("\tName: ") + pNode->ObjectNameA + std::string(" (Type: ") + AllocTypeName + std::string(")\n");
						pNode = pNode->pNext;
					}
				}
			}
		}

		if (!log.empty())
		{
			wi::backlog::post(log, wi::backlog::LogLevel::Error);
		}

		std::string message = "D3D12: device removed, cause: ";
		message += removedReasonString;
		wi::helper::messageBox(message, "Error!");
		wi::platform::Exit();
#endif // PLATFORM_WINDOWS_DESKTOP
	}

	void GraphicsDevice_DX12::WaitForGPU() const
	{
		ComPtr<ID3D12Fence> fence;
		HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(fence));
		assert(SUCCEEDED(hr));

		for (auto& queue : queues)
		{
			hr = queue.queue->Signal(fence.Get(), 1);
			assert(SUCCEEDED(hr));
			if (fence->GetCompletedValue() < 1)
			{
				hr = fence->SetEventOnCompletion(1, NULL);
				assert(SUCCEEDED(hr));
			}
			fence->Signal(0);
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

		for (auto& x : commandlists)
		{
			for (auto& y : x->pipelines_worker)
			{
				allocationhandler->destroyer_pipelines.push_back(std::make_pair(y.second, FRAMECOUNT));
			}
			x->pipelines_worker.clear();
		}
		allocationhandler->destroylocker.unlock();
	}

	Texture GraphicsDevice_DX12::GetBackBuffer(const SwapChain* swapchain) const
	{
		auto swapchain_internal = to_internal(swapchain);

		auto internal_state = std::make_shared<Texture_DX12>();
		internal_state->allocationhandler = allocationhandler;
		internal_state->resource = swapchain_internal->backBuffers[swapchain_internal->GetBufferIndex()];

		D3D12_RESOURCE_DESC resourcedesc = internal_state->resource->GetDesc();
		internal_state->total_size = 0;
		internal_state->footprints.resize(resourcedesc.DepthOrArraySize * resourcedesc.MipLevels);
		internal_state->rowSizesInBytes.resize(internal_state->footprints.size());
		internal_state->numRows.resize(internal_state->footprints.size());
		device->GetCopyableFootprints(
			&resourcedesc,
			0,
			(UINT)internal_state->footprints.size(),
			0,
			internal_state->footprints.data(),
			internal_state->numRows.data(),
			internal_state->rowSizesInBytes.data(),
			&internal_state->total_size
		);

		Texture result;
		result.type = GPUResource::Type::TEXTURE;
		result.internal_state = internal_state;
		result.desc = _ConvertTextureDesc_Inv(resourcedesc);
		return result;
	}

	ColorSpace GraphicsDevice_DX12::GetSwapChainColorSpace(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);
		return internal_state->colorSpace;
	}
	bool GraphicsDevice_DX12::IsSwapChainSupportsHDR(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);

#ifdef PLATFORM_XBOX
		// TODO
#else
		// HDR display query: https://docs.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
		ComPtr<IDXGIOutput> dxgiOutput;
		if (SUCCEEDED(internal_state->swapChain->GetContainingOutput(&dxgiOutput)))
		{
			ComPtr<IDXGIOutput6> output6;
			if (SUCCEEDED(dxgiOutput.As(&output6)))
			{
				DXGI_OUTPUT_DESC1 desc1;
				if (SUCCEEDED(output6->GetDesc1(&desc1)))
				{
					if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
					{
						return true;
					}
				}
			}
		}
#endif // PLATFORM_XBOX
		return false;
	}

	void GraphicsDevice_DX12::SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count)
	{
		CommandQueue& q = queues[queue];

		thread_local wi::vector<D3D12_TILED_RESOURCE_COORDINATE> tiled_resource_coordinates;
		thread_local wi::vector<D3D12_TILE_REGION_SIZE> tiled_region_sizes;
		thread_local wi::vector<D3D12_TILE_RANGE_FLAGS> tile_range_flags;
		thread_local wi::vector<uint32_t> range_start_offsets;

		for (uint32_t c = 0; c < command_count; ++c)
		{
			const SparseUpdateCommand& command = commands[c];
			auto internal_sparse_resource = to_internal(command.sparse_resource);
			uint32_t mip_count = 0;
			uint32_t array_size = 0;
			if (command.sparse_resource->IsTexture())
			{
				const Texture* sparse_texture = (const Texture*)command.sparse_resource;
				mip_count = sparse_texture->desc.mip_levels;
				array_size = sparse_texture->desc.array_size;
			}
			ID3D12Heap* heap = nullptr;
			uint32_t heap_page_offset = 0;
			if (command.tile_pool != nullptr)
			{
				auto internal_tile_pool = to_internal(command.tile_pool);
				heap = internal_tile_pool->allocation->GetHeap();
				heap_page_offset = uint32_t(internal_tile_pool->allocation->GetOffset() / D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
			}
			tiled_resource_coordinates.resize(command.num_resource_regions);
			tiled_region_sizes.resize(command.num_resource_regions);
			for (uint32_t i = 0; i < command.num_resource_regions; ++i)
			{
				const SparseResourceCoordinate& in_coordinate = command.coordinates[i];
				const SparseRegionSize& in_size = command.sizes[i];
				D3D12_TILED_RESOURCE_COORDINATE& out_coordinate = tiled_resource_coordinates[i];
				D3D12_TILE_REGION_SIZE& out_size = tiled_region_sizes[i];
				out_coordinate.X = in_coordinate.x;
				out_coordinate.Y = in_coordinate.y;
				out_coordinate.Z = in_coordinate.z;
				out_coordinate.Subresource = D3D12CalcSubresource(in_coordinate.mip, in_coordinate.slice, 0, mip_count, array_size);
				out_size.UseBox = in_coordinate.mip < internal_sparse_resource->sparse_texture_properties.packed_mip_start; // only for non packed mips
				out_size.Width = in_size.width;
				out_size.Height = in_size.height;
				out_size.Depth = in_size.depth;
				out_size.NumTiles = out_size.Width * out_size.Height * out_size.Depth;
			}
			tile_range_flags.resize(command.num_resource_regions);
			range_start_offsets.resize(command.num_resource_regions);
			for (uint32_t i = 0; i < command.num_resource_regions; ++i)
			{
				const TileRangeFlags& in_flags = command.range_flags[i];
				D3D12_TILE_RANGE_FLAGS& out_flags = tile_range_flags[i];
				switch (in_flags)
				{
				default:
				case TileRangeFlags::None:
					out_flags = D3D12_TILE_RANGE_FLAG_NONE;
					break;
				case TileRangeFlags::Null:
					out_flags = D3D12_TILE_RANGE_FLAG_NULL;
					break;
				}
				const uint32_t in_offset = command.range_start_offsets[i];
				uint32_t& out_offset = range_start_offsets[i];
				out_offset = heap_page_offset + in_offset;
			}

#ifdef PLATFORM_XBOX
			std::scoped_lock lock(queue_locker); // queue operations are not thread-safe on XBOX
#endif // PLATFORM_XBOX
			q.queue->UpdateTileMappings(
				internal_sparse_resource->resource.Get(),
				command.num_resource_regions,
				tiled_resource_coordinates.data(),
				tiled_region_sizes.data(),
				heap,
				command.num_resource_regions,
				tile_range_flags.data(),
				range_start_offsets.data(),
				command.range_tile_counts,
				D3D12_TILE_MAPPING_FLAG_NONE
			);
		}
	}

	void GraphicsDevice_DX12::WaitCommandList(CommandList cmd, CommandList wait_for)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		CommandList_DX12& commandlist_wait_for = GetCommandList(wait_for);
		assert(commandlist_wait_for.id < commandlist.id); // can't wait for future command list!
		commandlist.waits.push_back(wait_for);
		commandlist_wait_for.waited_on.store(true);
	}
	void GraphicsDevice_DX12::RenderPassBegin(const SwapChain* swapchain, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.renderpass_barriers_begin.clear();
		commandlist.renderpass_barriers_end.clear();
		commandlist.swapchains.push_back(swapchain);
		auto internal_state = to_internal(swapchain);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = internal_state->backBuffers[internal_state->GetBufferIndex()].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		commandlist.GetGraphicsCommandList()->ResourceBarrier(1, &barrier);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandlist.renderpass_barriers_end.push_back(barrier);

#ifdef PLATFORM_XBOX
		commandlist.GetGraphicsCommandList()->OMSetRenderTargets(
			1,
			&internal_state->backbufferRTV[internal_state->GetBufferIndex()],
			TRUE,
			nullptr
		);
		commandlist.GetGraphicsCommandList()->ClearRenderTargetView(
			internal_state->backbufferRTV[internal_state->GetBufferIndex()],
			swapchain->desc.clear_color,
			0,
			nullptr
		);
#else
		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTV = {};
		RTV.cpuDescriptor = internal_state->backbufferRTV[internal_state->GetBufferIndex()];
		RTV.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		RTV.BeginningAccess.Clear.ClearValue.Color[0] = swapchain->desc.clear_color[0];
		RTV.BeginningAccess.Clear.ClearValue.Color[1] = swapchain->desc.clear_color[1];
		RTV.BeginningAccess.Clear.ClearValue.Color[2] = swapchain->desc.clear_color[2];
		RTV.BeginningAccess.Clear.ClearValue.Color[3] = swapchain->desc.clear_color[3];
		RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		commandlist.GetGraphicsCommandListLatest()->BeginRenderPass(1, &RTV, nullptr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);
#endif // PLATFORM_XBOX

		commandlist.renderpass_info = RenderPassInfo::from(swapchain->desc);
	}
	void GraphicsDevice_DX12::RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.renderpass_barriers_begin.clear();
		commandlist.renderpass_barriers_end.clear();

		for (uint32_t rt = 0; rt < commandlist.renderpass_info.rt_count; ++rt)
		{
			commandlist.resolve_subresources[rt].clear();
			commandlist.resolve_dst[rt] = nullptr;
			commandlist.resolve_src[rt] = nullptr;
		}
		commandlist.resolve_subresources_dsv.clear();
		commandlist.resolve_dst_ds = nullptr;
		commandlist.resolve_src_ds = nullptr;

		uint32_t rt_count = 0;
		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTVs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DSV = {};

		// Some bookkeeping for resolves:
		uint32_t rt_resolve_count = 0;
		struct ResolveSourceInfo
		{
			ID3D12Resource* resource = nullptr;
			BOOL preserve = FALSE;
			uint32_t firstMip = 0;
			uint32_t mipCount = 0;
			uint32_t firstSlice = 0;
			uint32_t sliceCount = 0;
			uint32_t total_mipCount = 0;
			uint32_t total_sliceCount = 0;
		};
		ResolveSourceInfo RT_resolve_src_infos[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
		ResolveSourceInfo DS_resolve_src_info;

		for (uint32_t i = 0; i < image_count; ++i)
		{
			const RenderPassImage& image = images[i];
			const Texture* texture = image.texture;
			const TextureDesc& desc = texture->GetDesc();
			int subresource = image.subresource;
			auto internal_state = to_internal(texture);

			D3D12_CLEAR_VALUE clear_value;
			clear_value.Format = _ConvertFormat(desc.format);

			D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE beginning_access_type;
			switch (image.loadop)
			{
			default:
			case RenderPassImage::LoadOp::LOAD:
				beginning_access_type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
				break;
			case RenderPassImage::LoadOp::CLEAR:
				beginning_access_type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
				break;
			case RenderPassImage::LoadOp::DONTCARE:
				beginning_access_type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
				break;
			}

			D3D12_RENDER_PASS_ENDING_ACCESS_TYPE ending_access_type;
			switch (image.storeop)
			{
			default:
			case RenderPassImage::StoreOp::STORE:
				ending_access_type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				break;
			case RenderPassImage::StoreOp::DONTCARE:
				ending_access_type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
				break;
			}

			SingleDescriptor descriptor;

			switch (image.type)
			{
			case RenderPassImage::Type::RENDERTARGET:
			{
				descriptor = subresource < 0 ? internal_state->rtv : internal_state->subresources_rtv[subresource];
				ResolveSourceInfo& resolve_src_info = RT_resolve_src_infos[rt_count];
				D3D12_RENDER_PASS_RENDER_TARGET_DESC& RTV = RTVs[rt_count++];
				RTV.cpuDescriptor = descriptor.handle;
				RTV.BeginningAccess.Type = beginning_access_type;
				RTV.EndingAccess.Type = ending_access_type;
				clear_value.Color[0] = desc.clear.color[0];
				clear_value.Color[1] = desc.clear.color[1];
				clear_value.Color[2] = desc.clear.color[2];
				clear_value.Color[3] = desc.clear.color[3];
				RTV.BeginningAccess.Clear.ClearValue = clear_value;
				resolve_src_info.resource = internal_state->resource.Get();
				resolve_src_info.preserve = ending_access_type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				resolve_src_info.firstMip = descriptor.firstMip;
				resolve_src_info.mipCount = descriptor.mipCount;
				resolve_src_info.firstSlice = descriptor.firstSlice;
				resolve_src_info.sliceCount = descriptor.sliceCount;
				resolve_src_info.total_mipCount = desc.mip_levels;
				resolve_src_info.total_sliceCount = desc.array_size;
			}
			break;

			case RenderPassImage::Type::RESOLVE:
			{
				descriptor = subresource < 0 ? internal_state->srv : internal_state->subresources_srv[subresource];
				ResolveSourceInfo& resolve_src_info = RT_resolve_src_infos[rt_resolve_count];
				D3D12_RENDER_PASS_RENDER_TARGET_DESC& RTV = RTVs[rt_resolve_count];
				RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
				RTV.EndingAccess.Resolve.Format = clear_value.Format;
				RTV.EndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE_AVERAGE;
				RTV.EndingAccess.Resolve.pDstResource = internal_state->resource.Get();
				RTV.EndingAccess.Resolve.pSrcResource = resolve_src_info.resource;
				RTV.EndingAccess.Resolve.PreserveResolveSource = resolve_src_info.preserve;
				for (uint32_t mip = 0; mip < std::min(desc.mip_levels, descriptor.mipCount); ++mip)
				{
					for (uint32_t slice = 0; slice < std::min(desc.array_size, descriptor.sliceCount); ++slice)
					{
						D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS& params = commandlist.resolve_subresources[rt_resolve_count].emplace_back();
						params.SrcSubresource = D3D12CalcSubresource(resolve_src_info.firstMip + mip, resolve_src_info.firstSlice + slice, 0, resolve_src_info.total_mipCount, resolve_src_info.total_sliceCount);
						params.DstSubresource = D3D12CalcSubresource(descriptor.firstMip + mip, descriptor.firstSlice + slice, 0, desc.mip_levels, desc.array_size);
						params.SrcRect.left = 0;
						params.SrcRect.top = 0;
						params.SrcRect.right = (LONG)desc.width;
						params.SrcRect.bottom = (LONG)desc.height;
					}
				}
				RTV.EndingAccess.Resolve.pSubresourceParameters = commandlist.resolve_subresources[rt_resolve_count].data();
				RTV.EndingAccess.Resolve.SubresourceCount = (UINT)commandlist.resolve_subresources[rt_resolve_count].size();
				commandlist.resolve_src[rt_resolve_count] = RTV.EndingAccess.Resolve.pSrcResource;
				commandlist.resolve_dst[rt_resolve_count] = RTV.EndingAccess.Resolve.pDstResource;
				commandlist.resolve_formats[rt_resolve_count] = RTV.EndingAccess.Resolve.Format;
				rt_resolve_count++;
			}
			break;

			case RenderPassImage::Type::DEPTH_STENCIL:
			{
				descriptor = subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[subresource];
				DSV.cpuDescriptor = descriptor.handle;
				DSV.DepthBeginningAccess.Type = beginning_access_type;
				DSV.DepthEndingAccess.Type = ending_access_type;
				clear_value.DepthStencil.Depth = desc.clear.depth_stencil.depth;
				clear_value.DepthStencil.Stencil = desc.clear.depth_stencil.stencil;
				DSV.DepthBeginningAccess.Clear.ClearValue = clear_value;
				DSV.DepthEndingAccess.Resolve.pSrcResource = internal_state->resource.Get();
				DSV.DepthEndingAccess.Resolve.PreserveResolveSource = ending_access_type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				DS_resolve_src_info.resource = internal_state->resource.Get();
				DS_resolve_src_info.preserve = ending_access_type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				DS_resolve_src_info.firstMip = descriptor.firstMip;
				DS_resolve_src_info.mipCount = descriptor.mipCount;
				DS_resolve_src_info.firstSlice = descriptor.firstSlice;
				DS_resolve_src_info.sliceCount = descriptor.sliceCount;
				DS_resolve_src_info.total_mipCount = desc.mip_levels;
				DS_resolve_src_info.total_sliceCount = desc.array_size;
				if (IsFormatStencilSupport(desc.format))
				{
					DSV.StencilBeginningAccess = DSV.DepthBeginningAccess;
					DSV.StencilEndingAccess = DSV.DepthEndingAccess;
				}
			}
			break;

			case RenderPassImage::Type::RESOLVE_DEPTH:
			{
				descriptor = subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[subresource];
				DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
				DSV.DepthEndingAccess.Resolve.Format = clear_value.Format;
				switch (image.depth_resolve_mode)
				{
				default:
				case RenderPassImage::DepthResolveMode::Min:
					DSV.DepthEndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE_MIN;
					break;
				case RenderPassImage::DepthResolveMode::Max:
					DSV.DepthEndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE_MAX;
					break;
				}
				DSV.DepthEndingAccess.Resolve.pDstResource = internal_state->resource.Get();
				DSV.DepthEndingAccess.Resolve.pSrcResource = DS_resolve_src_info.resource;
				DSV.DepthEndingAccess.Resolve.PreserveResolveSource = DS_resolve_src_info.preserve;
				for (uint32_t mip = 0; mip < std::min(desc.mip_levels, descriptor.mipCount); ++mip)
				{
					for (uint32_t slice = 0; slice < std::min(desc.array_size, descriptor.sliceCount); ++slice)
					{
						D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS& params = commandlist.resolve_subresources_dsv.emplace_back();
						params.SrcSubresource = D3D12CalcSubresource(DS_resolve_src_info.firstMip + mip, DS_resolve_src_info.firstSlice + slice, 0, DS_resolve_src_info.total_mipCount, DS_resolve_src_info.total_sliceCount);
						params.DstSubresource = D3D12CalcSubresource(descriptor.firstMip + mip, descriptor.firstSlice + slice, 0, desc.mip_levels, desc.array_size);
						params.SrcRect.left = 0;
						params.SrcRect.top = 0;
						params.SrcRect.right = (LONG)desc.width;
						params.SrcRect.bottom = (LONG)desc.height;
					}
				}
				DSV.DepthEndingAccess.Resolve.pSubresourceParameters = commandlist.resolve_subresources_dsv.data();
				DSV.DepthEndingAccess.Resolve.SubresourceCount = (UINT)commandlist.resolve_subresources_dsv.size();
				if (IsFormatStencilSupport(desc.format))
				{
					DSV.StencilEndingAccess = DSV.DepthEndingAccess;
				}
				commandlist.resolve_dst_ds = DSV.DepthEndingAccess.Resolve.pDstResource;
				commandlist.resolve_src_ds = DSV.DepthEndingAccess.Resolve.pSrcResource;
				commandlist.resolve_ds_format = DSV.DepthEndingAccess.Resolve.Format;
			}
			break;

			case RenderPassImage::Type::SHADING_RATE_SOURCE:
				commandlist.shading_rate_image = internal_state->resource.Get(); // will be set after barriers
				break;

			default:
				break;
			}


			// Beginning barriers:
			{
				D3D12_RESOURCE_STATES before = _ParseResourceState(image.layout_before);
				D3D12_RESOURCE_STATES after = _ParseResourceState(image.layout);
				if (image.type == RenderPassImage::Type::RESOLVE || image.type == RenderPassImage::Type::RESOLVE_DEPTH)
				{
					after = D3D12_RESOURCE_STATE_RESOLVE_DEST;
				}
				if (before != after)
				{
					D3D12_RESOURCE_BARRIER barrierdesc = {};
					barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					barrierdesc.Transition.pResource = internal_state->resource.Get();
					barrierdesc.Transition.StateBefore = before;
					barrierdesc.Transition.StateAfter = after;

					if (subresource >= 0)
					{
						// Need to unroll descriptor into multiple subresource barriers:
						for (uint32_t mip = descriptor.firstMip; mip < std::min(desc.mip_levels, descriptor.firstMip + descriptor.mipCount); ++mip)
						{
							for (uint32_t slice = descriptor.firstSlice; slice < std::min(desc.array_size, descriptor.firstSlice + descriptor.sliceCount); ++slice)
							{
								barrierdesc.Transition.Subresource = D3D12CalcSubresource(mip, slice, 0, desc.mip_levels, desc.array_size);
								commandlist.renderpass_barriers_begin.push_back(barrierdesc);
							}
						}
					}
					else
					{
						// Single barrier for whole resource:
						barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						commandlist.renderpass_barriers_begin.push_back(barrierdesc);
					}
				}
			}

			// Ending barriers:
			{
				D3D12_RESOURCE_STATES before = _ParseResourceState(image.layout);
				D3D12_RESOURCE_STATES after = _ParseResourceState(image.layout_after);
				if (image.type == RenderPassImage::Type::RESOLVE || image.type == RenderPassImage::Type::RESOLVE_DEPTH)
				{
					before = D3D12_RESOURCE_STATE_RESOLVE_DEST;
				}
				if (before != after)
				{
					D3D12_RESOURCE_BARRIER barrierdesc = {};
					barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					barrierdesc.Transition.pResource = internal_state->resource.Get();
					barrierdesc.Transition.StateBefore = before;
					barrierdesc.Transition.StateAfter = after;

					if (subresource >= 0)
					{
						// Need to unroll descriptor into multiple subresource barriers:
						for (uint32_t mip = descriptor.firstMip; mip < std::min(desc.mip_levels, descriptor.firstMip + descriptor.mipCount); ++mip)
						{
							for (uint32_t slice = descriptor.firstSlice; slice < std::min(desc.array_size, descriptor.firstSlice + descriptor.sliceCount); ++slice)
							{
								barrierdesc.Transition.Subresource = D3D12CalcSubresource(mip, slice, 0, desc.mip_levels, desc.array_size);
								commandlist.renderpass_barriers_end.push_back(barrierdesc);
							}
						}
					}
					else
					{
						// Single barrier for whole resource:
						barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						commandlist.renderpass_barriers_end.push_back(barrierdesc);
					}
				}
			}
		}

		if (!commandlist.renderpass_barriers_begin.empty())
		{
			commandlist.GetGraphicsCommandList()->ResourceBarrier((UINT)commandlist.renderpass_barriers_begin.size(), commandlist.renderpass_barriers_begin.data());
		}

		if (commandlist.shading_rate_image != nullptr)
		{
			commandlist.GetGraphicsCommandListLatest()->RSSetShadingRateImage(commandlist.shading_rate_image);
		}

#ifdef PLATFORM_XBOX

		D3D12_CPU_DESCRIPTOR_HANDLE rt_descriptors[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		for (uint32_t i = 0; i < rt_count; ++i)
		{
			rt_descriptors[i] = RTVs[i].cpuDescriptor;
		}

		commandlist.GetGraphicsCommandList()->OMSetRenderTargets(
			rt_count,
			rt_descriptors,
			FALSE,
			DSV.cpuDescriptor.ptr == 0 ? nullptr : &DSV.cpuDescriptor
		);

		for (uint32_t i = 0; i < rt_count; ++i)
		{
			if (RTVs[i].BeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
			{
				commandlist.GetGraphicsCommandList()->ClearRenderTargetView(
					RTVs[i].cpuDescriptor,
					RTVs[i].BeginningAccess.Clear.ClearValue.Color,
					0,
					nullptr
				);
			}
		}
		if (DSV.DepthBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR || DSV.StencilBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
		{
			D3D12_CLEAR_FLAGS clear_flags = {};
			if (DSV.DepthBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
			{
				clear_flags |= D3D12_CLEAR_FLAG_DEPTH;
			}
			if (DSV.StencilBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
			{
				clear_flags |= D3D12_CLEAR_FLAG_STENCIL;
			}
			commandlist.GetGraphicsCommandList()->ClearDepthStencilView(
				DSV.cpuDescriptor,
				clear_flags,
				DSV.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth,
				DSV.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil,
				0,
				nullptr
			);
		}

#else
		D3D12_RENDER_PASS_FLAGS FLAGS = D3D12_RENDER_PASS_FLAG_NONE;
		if (has_flag(flags, RenderPassFlags::ALLOW_UAV_WRITES))
		{
			FLAGS |= D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
		}
		if (has_flag(flags, RenderPassFlags::SUSPENDING))
		{
			FLAGS |= D3D12_RENDER_PASS_FLAG_SUSPENDING_PASS;
		}
		if (has_flag(flags, RenderPassFlags::RESUMING))
		{
			FLAGS |= D3D12_RENDER_PASS_FLAG_RESUMING_PASS;
		}

		commandlist.GetGraphicsCommandListLatest()->BeginRenderPass(
			rt_count,
			RTVs,
			DSV.cpuDescriptor.ptr == 0 ? nullptr : &DSV,
			FLAGS
		);
#endif // PLATFORM_XBOX

		commandlist.renderpass_info = RenderPassInfo::from(images, image_count);
	}
	void GraphicsDevice_DX12::RenderPassEnd(CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);

#ifdef PLATFORM_XBOX
		// Batch up resolve SRC barriers since XBOX cannot do it with RenderPass:
		commandlist.resolve_src_barriers.clear();
		for (uint32_t rt = 0; rt < commandlist.renderpass_info.rt_count; ++rt)
		{
			for (auto& resolve : commandlist.resolve_subresources[rt])
			{
				D3D12_RESOURCE_BARRIER& barrier = commandlist.resolve_src_barriers.emplace_back();
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = commandlist.resolve_src[rt];
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
				barrier.Transition.Subresource = resolve.SrcSubresource;
			}
		}
		if (commandlist.resolve_dst_ds != nullptr)
		{
			for (auto& resolve : commandlist.resolve_subresources_dsv)
			{
				D3D12_RESOURCE_BARRIER& barrier = commandlist.resolve_src_barriers.emplace_back();
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = commandlist.resolve_src_ds;
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
				barrier.Transition.Subresource = resolve.SrcSubresource;
			}
		}
		commandlist.GetGraphicsCommandList()->ResourceBarrier((UINT)commandlist.resolve_src_barriers.size(), commandlist.resolve_src_barriers.data());

		// Perform all resolves:
		for (uint32_t rt = 0; rt < commandlist.renderpass_info.rt_count; ++rt)
		{
			for (auto& resolve : commandlist.resolve_subresources[rt])
			{
				commandlist.GetGraphicsCommandList()->ResolveSubresource(
					commandlist.resolve_dst[rt],
					resolve.DstSubresource,
					commandlist.resolve_src[rt],
					resolve.SrcSubresource,
					commandlist.resolve_formats[rt]
				);
			}
		}
		if (commandlist.resolve_dst_ds != nullptr)
		{
			for (auto& resolve : commandlist.resolve_subresources_dsv)
			{
				commandlist.GetGraphicsCommandList()->ResolveSubresource(
					commandlist.resolve_dst_ds,
					resolve.DstSubresource,
					commandlist.resolve_src_ds,
					resolve.SrcSubresource,
					commandlist.resolve_ds_format
				);
			}
		}

#else
		commandlist.GetGraphicsCommandListLatest()->EndRenderPass();
#endif // PLATFORM_XBOX

		if (commandlist.shading_rate_image != nullptr)
		{
			commandlist.GetGraphicsCommandListLatest()->RSSetShadingRateImage(nullptr);
			commandlist.shading_rate_image = nullptr;
		}

		if (!commandlist.renderpass_barriers_end.empty())
		{
			commandlist.GetGraphicsCommandList()->ResourceBarrier(
				(UINT)commandlist.renderpass_barriers_end.size(),
				commandlist.renderpass_barriers_end.data()
			);
		}

		commandlist.renderpass_info = {};
	}
	void GraphicsDevice_DX12::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd)
	{
		// Ensure that engine side Rect structure matches the D3D12_RECT structure
		static_assert(sizeof(Rect) == sizeof(D3D12_RECT));
		static_assert(offsetof(Rect, left) == offsetof(D3D12_RECT, left));
		static_assert(offsetof(Rect, right) == offsetof(D3D12_RECT, right));
		static_assert(offsetof(Rect, top) == offsetof(D3D12_RECT, top));
		static_assert(offsetof(Rect, bottom) == offsetof(D3D12_RECT, bottom));

		assert(rects != nullptr);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->RSSetScissorRects(numRects, (const D3D12_RECT*)rects);
	}
	void GraphicsDevice_DX12::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		// Ensure that engine side Viewport structure matches the D3D12_VIEWPORT structure
		static_assert(sizeof(Viewport) == sizeof(D3D12_VIEWPORT));
		static_assert(offsetof(Viewport, top_left_x) == offsetof(D3D12_VIEWPORT, TopLeftX));
		static_assert(offsetof(Viewport, top_left_y) == offsetof(D3D12_VIEWPORT, TopLeftY));
		static_assert(offsetof(Viewport, width) == offsetof(D3D12_VIEWPORT, Width));
		static_assert(offsetof(Viewport, height) == offsetof(D3D12_VIEWPORT, Height));
		static_assert(offsetof(Viewport, min_depth) == offsetof(D3D12_VIEWPORT, MinDepth));
		static_assert(offsetof(Viewport, max_depth) == offsetof(D3D12_VIEWPORT, MaxDepth));

		assert(pViewports != nullptr);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->RSSetViewports(NumViewports, (const D3D12_VIEWPORT*)pViewports);
	}
	void GraphicsDevice_DX12::BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < DESCRIPTORBINDER_SRV_COUNT);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto& binder = commandlist.binder;
		if (binder.table.SRV[slot].internal_state != resource->internal_state || binder.table.SRV_index[slot] != subresource)
		{
			binder.table.SRV[slot] = *resource;
			binder.table.SRV_index[slot] = subresource;

			if (binder.optimizer_graphics != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_graphics;
				if (optimizer.SRV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_graphics |= 1ull << optimizer.SRV[slot];
				}
			}
			if (binder.optimizer_compute != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_compute;
				if (optimizer.SRV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_compute |= 1ull << optimizer.SRV[slot];
				}
			}
		}
	}
	void GraphicsDevice_DX12::BindResources(const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindResource(resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_DX12::BindUAV(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < DESCRIPTORBINDER_UAV_COUNT);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto& binder = commandlist.binder;
		if (binder.table.UAV[slot].internal_state != resource->internal_state || binder.table.UAV_index[slot] != subresource)
		{
			binder.table.UAV[slot] = *resource;
			binder.table.UAV_index[slot] = subresource;

			if (binder.optimizer_graphics != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_graphics;
				if (optimizer.UAV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_graphics |= 1ull << optimizer.UAV[slot];
				}
			}
			if (binder.optimizer_compute != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_compute;
				if (optimizer.UAV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_compute |= 1ull << optimizer.UAV[slot];
				}
			}
		}
	}
	void GraphicsDevice_DX12::BindUAVs(const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindUAV(resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_DX12::BindSampler(const Sampler* sampler, uint32_t slot, CommandList cmd)
	{
		assert(slot < DESCRIPTORBINDER_SAMPLER_COUNT);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto& binder = commandlist.binder;
		if (binder.table.SAM[slot].internal_state != sampler->internal_state)
		{
			binder.table.SAM[slot] = *sampler;

			if (binder.optimizer_graphics != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_graphics;
				if (optimizer.SAM[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_graphics |= 1ull << optimizer.SAM[slot];
				}
			}
			if (binder.optimizer_compute != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_compute;
				if (optimizer.SAM[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_compute |= 1ull << optimizer.SAM[slot];
				}
			}
		}
	}
	void GraphicsDevice_DX12::BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset)
	{
		assert(slot < DESCRIPTORBINDER_CBV_COUNT);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto& binder = commandlist.binder;
		if (binder.table.CBV[slot].internal_state != buffer->internal_state || binder.table.CBV_offset[slot] != offset)
		{
			binder.table.CBV[slot] = *buffer;
			binder.table.CBV_offset[slot] = offset;

			if (binder.optimizer_graphics != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_graphics;
				if (optimizer.CBV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_graphics |= 1ull << optimizer.CBV[slot];
				}
			}
			if (binder.optimizer_compute != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_compute;
				if (optimizer.CBV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_compute |= 1ull << optimizer.CBV[slot];
				}
			}
		}
	}
	void GraphicsDevice_DX12::BindVertexBuffers(const GPUBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint64_t* offsets, CommandList cmd)
	{
		assert(count <= 8);
		D3D12_VERTEX_BUFFER_VIEW res[8] = {};
		for (uint32_t i = 0; i < count; ++i)
		{
			if (vertexBuffers[i] != nullptr)
			{
				res[i].BufferLocation = vertexBuffers[i]->IsValid() ? to_internal(vertexBuffers[i])->gpu_address : 0;
				res[i].SizeInBytes = (UINT)vertexBuffers[i]->desc.size;
				if (offsets != nullptr)
				{
					res[i].BufferLocation += offsets[i];
					res[i].SizeInBytes -= (UINT)offsets[i];
				}
				res[i].StrideInBytes = strides[i];
			}
		}
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->IASetVertexBuffers(slot, count, res);
	}
	void GraphicsDevice_DX12::BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd)
	{
		D3D12_INDEX_BUFFER_VIEW res = {};
		if (indexBuffer != nullptr)
		{
			auto internal_state = to_internal(indexBuffer);

			res.BufferLocation = internal_state->gpu_address + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
			res.Format = (format == IndexBufferFormat::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
			res.SizeInBytes = (UINT)(indexBuffer->desc.size - offset);
		}
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->IASetIndexBuffer(&res);
	}
	void GraphicsDevice_DX12::BindStencilRef(uint32_t value, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->OMSetStencilRef(value);
	}
	void GraphicsDevice_DX12::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		const float blendFactor[4] = { r, g, b, a };
		commandlist.GetGraphicsCommandList()->OMSetBlendFactor(blendFactor);
	}
	void GraphicsDevice_DX12::BindShadingRate(ShadingRate rate, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING) && commandlist.prev_shadingrate != rate)
		{
			commandlist.prev_shadingrate = rate;

			D3D12_SHADING_RATE _rate = D3D12_SHADING_RATE_1X1;
			WriteShadingRateValue(rate, &_rate);

			D3D12_SHADING_RATE_COMBINER combiners[] =
			{
				D3D12_SHADING_RATE_COMBINER_MAX,
				D3D12_SHADING_RATE_COMBINER_MAX,
			};
			commandlist.GetGraphicsCommandListLatest()->RSSetShadingRate(_rate, combiners);
		}
	}
	void GraphicsDevice_DX12::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (commandlist.active_pso == pso)
		{
			return;
		}
		commandlist.active_cs = nullptr;
		commandlist.active_rt = nullptr;

		auto internal_state = to_internal(pso);
		if (internal_state->resource != nullptr)
		{
			commandlist.GetGraphicsCommandList()->SetPipelineState(internal_state->resource.Get());

			if (commandlist.prev_pt != internal_state->primitiveTopology)
			{
				commandlist.prev_pt = internal_state->primitiveTopology;

				commandlist.GetGraphicsCommandList()->IASetPrimitiveTopology(internal_state->primitiveTopology);
			}

			commandlist.prev_pipeline_hash = 0;
			commandlist.dirty_pso = false;
		}
		else
		{
			size_t pipeline_hash = 0;
			wi::helper::hash_combine(pipeline_hash, internal_state->hash);
			wi::helper::hash_combine(pipeline_hash, commandlist.renderpass_info.get_hash());
			if (commandlist.prev_pipeline_hash == pipeline_hash)
			{
				return;
			}
			commandlist.prev_pipeline_hash = pipeline_hash;
			commandlist.dirty_pso = true;
		}

		if (commandlist.active_rootsig_graphics != internal_state->rootSignature.Get())
		{
			commandlist.active_rootsig_graphics = internal_state->rootSignature.Get();
			commandlist.GetGraphicsCommandList()->SetGraphicsRootSignature(internal_state->rootSignature.Get());

			auto& binder = commandlist.binder;
			binder.optimizer_graphics = &internal_state->rootsig_optimizer;
			binder.dirty_graphics = internal_state->rootsig_optimizer.root_mask; // invalidates all root bindings
		}

		commandlist.active_pso = pso;
	}
	void GraphicsDevice_DX12::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (commandlist.active_cs == cs)
		{
			return;
		}
		commandlist.active_pso = nullptr;
		commandlist.active_rt = nullptr;

		assert(cs->stage == ShaderStage::CS || cs->stage == ShaderStage::LIB);

		commandlist.prev_pipeline_hash = 0;

		commandlist.active_cs = cs;

		auto internal_state = to_internal(cs);

		if (cs->stage == ShaderStage::CS)
		{
			commandlist.GetGraphicsCommandList()->SetPipelineState(internal_state->resource.Get());
		}

		if (commandlist.active_rootsig_compute != internal_state->rootSignature.Get())
		{
			commandlist.active_rootsig_compute = internal_state->rootSignature.Get();
			commandlist.GetGraphicsCommandList()->SetComputeRootSignature(internal_state->rootSignature.Get());

			auto& binder = commandlist.binder;
			binder.optimizer_compute = &internal_state->rootsig_optimizer;
			binder.dirty_compute = internal_state->rootsig_optimizer.root_mask; // invalidates all root bindings
		}
	}
	void GraphicsDevice_DX12::BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability::DEPTH_BOUNDS_TEST))
		{
			CommandList_DX12& commandlist = GetCommandList(cmd);
			commandlist.GetGraphicsCommandListLatest()->OMSetDepthBounds(min_bounds, max_bounds);
		}
	}
	void GraphicsDevice_DX12::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(drawInstancedIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(drawIndexedInstancedIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DrawInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(drawInstancedIndirectCommandSignature.Get(), max_count, args_internal->resource.Get(), args_offset, count_internal->resource.Get(), count_offset);
	}
	void GraphicsDevice_DX12::DrawIndexedInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(drawIndexedInstancedIndirectCommandSignature.Get(), max_count, args_internal->resource.Get(), args_offset, count_internal->resource.Get(), count_offset);
	}
	void GraphicsDevice_DX12::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predispatch(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_DX12::DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		auto internal_state = to_internal(args);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(dispatchIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandListLatest()->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_DX12::DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(dispatchMeshIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DispatchMeshIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(dispatchMeshIndirectCommandSignature.Get(), max_count, args_internal->resource.Get(), args_offset, count_internal->resource.Get(), count_offset);
	}
	void GraphicsDevice_DX12::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);

		const TextureDesc& src_desc = ((const Texture*)pSrc)->GetDesc();
		const TextureDesc& dst_desc = ((const Texture*)pDst)->GetDesc();

		if (src_desc.usage == Usage::UPLOAD)
		{
			for (uint32_t layer = 0; layer < dst_desc.array_size; ++layer)
			{
				for (uint32_t mip = 0; mip < dst_desc.mip_levels; ++mip)
				{
					UINT subresource = D3D12CalcSubresource(mip, layer, 0, dst_desc.mip_levels, dst_desc.array_size);
					CD3DX12_TEXTURE_COPY_LOCATION Src(internal_state_src->resource.Get(), internal_state_src->footprints[layer * dst_desc.mip_levels + mip]);
					CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state_dst->resource.Get(), subresource);
					commandlist.GetGraphicsCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
				}
			}
		}
		else if (dst_desc.usage == Usage::READBACK)
		{
			for (uint32_t layer = 0; layer < dst_desc.array_size; ++layer)
			{
				for (uint32_t mip = 0; mip < dst_desc.mip_levels; ++mip)
				{
					UINT subresource = D3D12CalcSubresource(mip, layer, 0, dst_desc.mip_levels, dst_desc.array_size);
					CD3DX12_TEXTURE_COPY_LOCATION Src(internal_state_src->resource.Get(), subresource);
					CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state_dst->resource.Get(), internal_state_dst->footprints[layer * dst_desc.mip_levels + mip]);
					commandlist.GetGraphicsCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
				}
			}
		}
		else
		{
			commandlist.GetGraphicsCommandList()->CopyResource(internal_state_dst->resource.Get(), internal_state_src->resource.Get());
		}
	}
	void GraphicsDevice_DX12::CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto src_internal = to_internal((const GPUBuffer*)pSrc);
		auto dst_internal = to_internal((const GPUBuffer*)pDst);

		commandlist.GetGraphicsCommandList()->CopyBufferRegion(dst_internal->resource.Get(), dst_offset, src_internal->resource.Get(), src_offset, size);
	}
	void GraphicsDevice_DX12::CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox, ImageAspect dst_aspect, ImageAspect src_aspect)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto src_internal = to_internal((const GPUBuffer*)src);
		auto dst_internal = to_internal((const GPUBuffer*)dst);
		UINT srcPlane = src_aspect == ImageAspect::STENCIL ? 1 : 0;
		UINT dstPlane = dst_aspect == ImageAspect::STENCIL ? 1 : 0;
		CD3DX12_TEXTURE_COPY_LOCATION src_location(src_internal->resource.Get(), D3D12CalcSubresource(srcMip, srcSlice, srcPlane, src->desc.mip_levels, src->desc.array_size));
		CD3DX12_TEXTURE_COPY_LOCATION dst_location(dst_internal->resource.Get(), D3D12CalcSubresource(dstMip, dstSlice, dstPlane, dst->desc.mip_levels, dst->desc.array_size));
		if (srcbox == nullptr)
		{
			commandlist.GetGraphicsCommandList()->CopyTextureRegion(
				&dst_location,
				dstX,
				dstY,
				dstZ,
				&src_location,
				nullptr
			);
		}
		else
		{
			D3D12_BOX d3dbox = {};
			d3dbox.left = srcbox->left;
			d3dbox.top = srcbox->top;
			d3dbox.front = srcbox->front;
			d3dbox.right = srcbox->right;
			d3dbox.bottom = srcbox->bottom;
			d3dbox.back = srcbox->back;
			commandlist.GetGraphicsCommandList()->CopyTextureRegion(
				&dst_location,
				dstX,
				dstY,
				dstZ,
				&src_location,
				&d3dbox
			);
		}

	}
	void GraphicsDevice_DX12::QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType::TIMESTAMP:
			commandlist.GetGraphicsCommandList()->BeginQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				index
			);
			break;
		case GpuQueryType::OCCLUSION_BINARY:
			commandlist.GetGraphicsCommandList()->BeginQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_BINARY_OCCLUSION,
				index
			);
			break;
		case GpuQueryType::OCCLUSION:
			commandlist.GetGraphicsCommandList()->BeginQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_OCCLUSION,
				index
			);
			break;
		}
	}
	void GraphicsDevice_DX12::QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType::TIMESTAMP:
			commandlist.GetGraphicsCommandList()->EndQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				index
			);
			break;
		case GpuQueryType::OCCLUSION_BINARY:
			commandlist.GetGraphicsCommandList()->EndQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_BINARY_OCCLUSION,
				index
			);
			break;
		case GpuQueryType::OCCLUSION:
			commandlist.GetGraphicsCommandList()->EndQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_OCCLUSION,
				index
			);
			break;
		}
	}
	void GraphicsDevice_DX12::QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);
		auto dst_internal = to_internal(dest);

		switch (heap->desc.type)
		{
		case GpuQueryType::TIMESTAMP:
			commandlist.GetGraphicsCommandList()->ResolveQueryData(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				index,
				count,
				dst_internal->resource.Get(),
				dest_offset
			);
			break;
		case GpuQueryType::OCCLUSION_BINARY:
			commandlist.GetGraphicsCommandList()->ResolveQueryData(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_BINARY_OCCLUSION,
				index,
				count,
				dst_internal->resource.Get(),
				dest_offset
			);
			break;
		case GpuQueryType::OCCLUSION:
			commandlist.GetGraphicsCommandList()->ResolveQueryData(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_OCCLUSION,
				index,
				count,
				dst_internal->resource.Get(),
				dest_offset
			);
			break;
		}
	}
	void GraphicsDevice_DX12::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);

		auto& barrierdescs = commandlist.frame_barriers;

		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];

			if (barrier.type == GPUBarrier::Type::IMAGE && (barrier.image.texture == nullptr || !barrier.image.texture->IsValid()))
				continue;
			if (barrier.type == GPUBarrier::Type::BUFFER && (barrier.buffer.buffer == nullptr || !barrier.buffer.buffer->IsValid()))
				continue;

			D3D12_RESOURCE_BARRIER barrierdesc = {};

			switch (barrier.type)
			{
			default:
			case GPUBarrier::Type::MEMORY:
			{
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.UAV.pResource = barrier.memory.resource == nullptr ? nullptr : to_internal(barrier.memory.resource)->resource.Get();
			}
			break;
			case GPUBarrier::Type::IMAGE:
			{
				auto internal_state = to_internal(barrier.image.texture);
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Transition.pResource = internal_state->resource.Get();
				barrierdesc.Transition.StateBefore = _ParseResourceState(barrier.image.layout_before);
				barrierdesc.Transition.StateAfter = _ParseResourceState(barrier.image.layout_after);
				if (barrier.image.mip >= 0 || barrier.image.slice >= 0 || barrier.image.aspect != nullptr)
				{
					const UINT PlaneSlice = barrier.image.aspect != nullptr ? _ImageAspectToPlane(*barrier.image.aspect) : 0;
					barrierdesc.Transition.Subresource = D3D12CalcSubresource(
						(UINT)std::max(0, barrier.image.mip),
						(UINT)std::max(0, barrier.image.slice),
						PlaneSlice,
						barrier.image.texture->desc.mip_levels,
						barrier.image.texture->desc.array_size
					);
				}
				else
				{
					barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				}

				if (barrier.image.layout_before == ResourceState::UNDEFINED)
				{
					CommandList_DX12::Discard& discard = commandlist.discards.emplace_back();
					discard.resource = internal_state->resource.Get();

					if (barrier.image.mip >= 0 || barrier.image.slice >= 0)
					{
						discard.region.FirstSubresource = D3D12CalcSubresource(
							(UINT)std::max(0, barrier.image.mip),
							(UINT)std::max(0, barrier.image.slice),
							0,
							barrier.image.texture->desc.mip_levels,
							barrier.image.texture->desc.array_size
						);
						discard.region.NumSubresources = 1;
						discard.region.NumRects = 0;
						discard.region.pRects = nullptr;
					}

					barrierdesc.Transition.StateBefore = _ParseResourceState(barrier.image.texture->desc.layout);
				}
			}
			break;
			case GPUBarrier::Type::BUFFER:
			{
				auto internal_state = to_internal(barrier.buffer.buffer);
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Transition.pResource = internal_state->resource.Get();
				barrierdesc.Transition.StateBefore = _ParseResourceState(barrier.buffer.state_before);
				barrierdesc.Transition.StateAfter = _ParseResourceState(barrier.buffer.state_after);
				barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				if (barrier.buffer.state_before == ResourceState::UNDEFINED)
				{
					CommandList_DX12::Discard& discard = commandlist.discards.emplace_back();
					discard.resource = internal_state->resource.Get();
				}
			}
			break;
			}

			if (barrierdesc.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION && commandlist.queue > QUEUE_GRAPHICS)
			{
				// Only graphics queue can do pixel shader state:
				barrierdesc.Transition.StateBefore &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				barrierdesc.Transition.StateAfter &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}

			barrierdescs.push_back(barrierdesc);
		}

		if (!barrierdescs.empty())
		{
			if (commandlist.queue == QUEUE_VIDEO_DECODE)
			{
				// Video queue can only transition from/to VIDEO_ and COMMON states, so we will overwrite anything else to COMMON,
				//	but the video textures will be using D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS so that implicit transition can happen from COMMON in a different queue
				for (auto& barrier : barrierdescs)
				{
					if (barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
					{
						if (barrier.Transition.StateBefore != D3D12_RESOURCE_STATE_VIDEO_DECODE_READ && barrier.Transition.StateBefore != D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE)
						{
							barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
						}
						if (barrier.Transition.StateAfter != D3D12_RESOURCE_STATE_VIDEO_DECODE_READ && barrier.Transition.StateAfter != D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE)
						{
							barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
						}
					}
				}
				commandlist.GetVideoDecodeCommandList()->ResourceBarrier(
					(UINT)barrierdescs.size(),
					barrierdescs.data()
				);
			}
			else
			{
				commandlist.GetGraphicsCommandList()->ResourceBarrier(
					(UINT)barrierdescs.size(),
					barrierdescs.data()
				);
			}
			barrierdescs.clear();
		}

		if (!commandlist.discards.empty() && commandlist.queue != QUEUE_VIDEO_DECODE)
		{
			for (auto& discard : commandlist.discards)
			{
				commandlist.GetGraphicsCommandList()->DiscardResource(discard.resource, discard.region.NumSubresources > 0 ? &discard.region : nullptr);
			}
			commandlist.discards.clear();
		}
	}
	void GraphicsDevice_DX12::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto dst_internal = to_internal(dst);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
		desc.Inputs = dst_internal->desc;

		// Make a copy of geometries, don't overwrite internal_state (thread safety)
		commandlist.accelerationstructure_build_geometries = dst_internal->geometries;
		desc.Inputs.pGeometryDescs = commandlist.accelerationstructure_build_geometries.data();

		// The real GPU addresses get filled here:
		switch (dst->desc.type)
		{
		case RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL:
		{
			size_t i = 0;
			for (auto& x : dst->desc.bottom_level.geometries)
			{
				auto& geometry = commandlist.accelerationstructure_build_geometries[i++];
				if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE)
				{
					geometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
				}
				if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_NO_DUPLICATE_ANYHIT_INVOCATION)
				{
					geometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
				}

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES)
				{
					geometry.Triangles.VertexBuffer.StartAddress = to_internal(&x.triangles.vertex_buffer)->gpu_address + 
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.vertex_byte_offset;
					geometry.Triangles.IndexBuffer = to_internal(&x.triangles.index_buffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.index_offset * (x.triangles.index_format == IndexBufferFormat::UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));

					if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.Triangles.Transform3x4 = to_internal(&x.triangles.transform_3x4_buffer)->gpu_address +
							(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.transform_3x4_buffer_offset;
					}
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::PROCEDURAL_AABBS)
				{
					geometry.AABBs.AABBs.StartAddress = to_internal(&x.aabbs.aabb_buffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.aabbs.offset;
				}
			}
		}
		break;
		case RaytracingAccelerationStructureDesc::Type::TOPLEVEL:
		{
			desc.Inputs.InstanceDescs = to_internal(&dst->desc.top_level.instance_buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)dst->desc.top_level.offset;
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
		commandlist.GetGraphicsCommandListLatest()->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
	}
	void GraphicsDevice_DX12::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.active_cs = nullptr;
		commandlist.active_pso = nullptr;
		commandlist.prev_pipeline_hash = 0;
		commandlist.active_rt = rtpso;

		BindComputeShader(rtpso->desc.shader_libraries.front().shader, cmd);

		auto internal_state = to_internal(rtpso);
		commandlist.GetGraphicsCommandListLatest()->SetPipelineState1(internal_state->resource.Get());
	}
	void GraphicsDevice_DX12::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		predispatch(cmd);

		D3D12_DISPATCH_RAYS_DESC dispatchrays_desc = {};

		dispatchrays_desc.Width = desc->width;
		dispatchrays_desc.Height = desc->height;
		dispatchrays_desc.Depth = desc->depth;

		if (desc->ray_generation.buffer != nullptr)
		{
			dispatchrays_desc.RayGenerationShaderRecord.StartAddress =
				to_internal(desc->ray_generation.buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->ray_generation.offset;
			dispatchrays_desc.RayGenerationShaderRecord.SizeInBytes =
				desc->ray_generation.size;
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

		if (desc->hit_group.buffer != nullptr)
		{
			dispatchrays_desc.HitGroupTable.StartAddress =
				to_internal(desc->hit_group.buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->hit_group.offset;
			dispatchrays_desc.HitGroupTable.SizeInBytes =
				desc->hit_group.size;
			dispatchrays_desc.HitGroupTable.StrideInBytes =
				desc->hit_group.stride;
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

		commandlist.GetGraphicsCommandListLatest()->DispatchRays(&dispatchrays_desc);
	}
	void GraphicsDevice_DX12::PushConstants(const void* data, uint32_t size, CommandList cmd, uint32_t offset)
	{
		assert(size % sizeof(uint32_t) == 0);
		assert(offset % sizeof(uint32_t) == 0);

		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto& binder = commandlist.binder;
		if (commandlist.active_pso != nullptr)
		{
			const RootSignatureOptimizer* optimizer = (const RootSignatureOptimizer*)binder.optimizer_graphics;
			const D3D12_ROOT_PARAMETER1& param = optimizer->rootsig_desc->Desc_1_1.pParameters[optimizer->PUSH];
			assert(param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);
			assert(size <= param.Constants.Num32BitValues * sizeof(uint32_t)); // if this fires, not enough root constants were declared in root signature!
			commandlist.GetGraphicsCommandList()->SetGraphicsRoot32BitConstants(
				optimizer->PUSH,
				size / sizeof(uint32_t),
				data,
				offset / sizeof(uint32_t)
			);
			return;
		}
		if (commandlist.active_cs != nullptr)
		{
			const RootSignatureOptimizer* optimizer = (const RootSignatureOptimizer*)binder.optimizer_compute;
			const D3D12_ROOT_PARAMETER1& param = optimizer->rootsig_desc->Desc_1_1.pParameters[optimizer->PUSH];
			assert(param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);
			assert(size <= param.Constants.Num32BitValues * sizeof(uint32_t)); // if this fires, not enough root constants were declared in root signature!
			commandlist.GetGraphicsCommandList()->SetComputeRoot32BitConstants(
				optimizer->PUSH,
				size / sizeof(uint32_t),
				data,
				offset / sizeof(uint32_t)
			);
			return;
		}
		assert(0); // there was no active pipeline!
	}
	void GraphicsDevice_DX12::PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(buffer);
		D3D12_PREDICATION_OP operation;
		switch (op)
		{
		default:
		case PredicationOp::EQUAL_ZERO:
			operation = D3D12_PREDICATION_OP_EQUAL_ZERO;
			break;
		case PredicationOp::NOT_EQUAL_ZERO:
			operation = D3D12_PREDICATION_OP_NOT_EQUAL_ZERO;
			break;
		}
		commandlist.GetGraphicsCommandList()->SetPredication(internal_state->resource.Get(), offset, operation);
	}
	void GraphicsDevice_DX12::PredicationEnd(CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->SetPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);
	}
	void GraphicsDevice_DX12::ClearUAV(const GPUResource* resource, uint32_t value, CommandList cmd)
	{
		auto internal_state = to_internal(resource);
		// We cannot clear eg. a StructuredBuffer, so in those cases we must clear the RAW view with uav_raw
		const SingleDescriptor& descriptor = internal_state->uav_raw.IsValid() ? internal_state->uav_raw : internal_state->uav;
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = descriptorheap_res.start_gpu;
		gpu_handle.ptr += descriptor.index * resource_descriptor_size;
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = descriptor.handle;

		const UINT values[4] = { value,value,value,value };

		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ClearUnorderedAccessViewUint(
			gpu_handle,
			cpu_handle,
			internal_state->resource.Get(),
			values,
			0,
			nullptr
		);
	}
	void GraphicsDevice_DX12::VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd)
	{
		auto decoder_internal = to_internal(video_decoder);
		auto stream_internal = to_internal(op->stream);
		auto dpb_internal = to_internal(op->DPB);
		D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS output = {};
		D3D12_VIDEO_DECODE_INPUT_STREAM_ARGUMENTS input = {};

		output.pOutputTexture2D = dpb_internal->resource.Get();
		output.OutputSubresource = D3D12CalcSubresource(0, op->current_dpb, 0, 1, op->DPB->desc.array_size);

		const h264::SliceHeader* slice_header = (const h264::SliceHeader*)op->slice_header;
		const h264::PPS* pps = (const h264::PPS*)op->pps;
		const h264::SPS* sps = (const h264::SPS*)op->sps;

		ID3D12Resource* reference_frames[16] = {};
		UINT reference_subresources[16] = {};
		for (size_t i = 0; i < op->dpb_reference_count; ++i)
		{
			reference_frames[i] = dpb_internal->resource.Get();
			reference_subresources[i] = D3D12CalcSubresource(0, op->dpb_reference_slots[i], 0, 1, op->DPB->desc.array_size);
		}
		input.ReferenceFrames.NumTexture2Ds = (UINT)op->dpb_reference_count;
		input.ReferenceFrames.ppTexture2Ds = input.ReferenceFrames.NumTexture2Ds > 0 ? reference_frames : nullptr;
		input.ReferenceFrames.pSubresources = input.ReferenceFrames.NumTexture2Ds > 0 ? reference_subresources : nullptr;

		input.CompressedBitstream.pBuffer = stream_internal->resource.Get();
		input.CompressedBitstream.Offset = op->stream_offset;
		input.CompressedBitstream.Size = op->stream_size;
		input.pHeap = decoder_internal->decoder_heap.Get();

		// DirectX Video Acceleration for H.264/MPEG-4 AVC Decoding, Microsoft, Updated 2010, Page 21
		//	https://www.microsoft.com/en-us/download/details.aspx?id=11323
		//	Also: https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/gallium/drivers/d3d12/d3d12_video_dec_h264.cpp
		DXVA_PicParams_H264 pic_params = {};
		pic_params.wFrameWidthInMbsMinus1 = sps->pic_width_in_mbs_minus1;
		pic_params.wFrameHeightInMbsMinus1 = sps->pic_height_in_map_units_minus1;
		pic_params.IntraPicFlag = op->frame_type == VideoFrameType::Intra ? 1 : 0;
		pic_params.MbaffFrameFlag = sps->mb_adaptive_frame_field_flag && !slice_header->field_pic_flag;
		pic_params.field_pic_flag = slice_header->field_pic_flag; // 0 = full frame (top and bottom field)
		//pic_params.bottom_field_flag = 0; // missing??
		pic_params.chroma_format_idc = 1; // sps->chroma_format_idc; // only 1 is supported (YUV420)
		pic_params.bit_depth_chroma_minus8 = sps->bit_depth_chroma_minus8;
		assert(pic_params.bit_depth_chroma_minus8 == 0);   // Only support for NV12 now
		pic_params.bit_depth_luma_minus8 = sps->bit_depth_luma_minus8;
		assert(pic_params.bit_depth_luma_minus8 == 0);   // Only support for NV12 now
		pic_params.residual_colour_transform_flag = sps->separate_colour_plane_flag; // https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/gallium/drivers/d3d12/d3d12_video_dec_h264.cpp#L328
		if (pic_params.field_pic_flag)
		{
			pic_params.CurrPic.AssociatedFlag = slice_header->bottom_field_flag ? 1 : 0; // if pic_params.field_pic_flag == 1, then this is 0 = top field, 1 = bottom_field
		}
		else
		{
			pic_params.CurrPic.AssociatedFlag = 0;
		}
		pic_params.CurrPic.Index7Bits = (UCHAR)op->current_dpb;
		pic_params.CurrFieldOrderCnt[0] = op->poc[0];
		pic_params.CurrFieldOrderCnt[1] = op->poc[1];
		for (uint32_t i = 0; i < 16; ++i)
		{
			pic_params.RefFrameList[i].bPicEntry = 0xFF;
			pic_params.FieldOrderCntList[i][0] = 0;
			pic_params.FieldOrderCntList[i][1] = 0;
			pic_params.FrameNumList[i] = 0;
		}
		for (size_t i = 0; i < op->dpb_reference_count; ++i)
		{
			uint32_t ref_slot = op->dpb_reference_slots[i];
			pic_params.RefFrameList[i].AssociatedFlag = 0; // 0 = short term, 1 = long term reference
			pic_params.RefFrameList[i].Index7Bits = (UCHAR)ref_slot;
			pic_params.FieldOrderCntList[i][0] = op->dpb_poc[ref_slot];
			pic_params.FieldOrderCntList[i][1] = op->dpb_poc[ref_slot];
			pic_params.UsedForReferenceFlags |= 1 << (i * 2 + 0);
			pic_params.UsedForReferenceFlags |= 1 << (i * 2 + 1);
			pic_params.FrameNumList[i] = op->dpb_framenum[ref_slot];
		}
		pic_params.weighted_pred_flag = pps->weighted_pred_flag;
		pic_params.weighted_bipred_idc = pps->weighted_bipred_idc;
		pic_params.sp_for_switch_flag = 0;

		pic_params.transform_8x8_mode_flag = pps->transform_8x8_mode_flag;
		pic_params.constrained_intra_pred_flag = pps->constrained_intra_pred_flag;
		pic_params.num_ref_frames = sps->num_ref_frames;
		pic_params.MbsConsecutiveFlag = 1; // The value shall be 1 unless the restricted-mode profile in use explicitly supports the value 0.
		pic_params.frame_mbs_only_flag = sps->frame_mbs_only_flag;
		pic_params.MinLumaBipredSize8x8Flag = 1;
		pic_params.RefPicFlag = op->reference_priority > 0 ? 1 : 0;
		pic_params.frame_num = slice_header->frame_num;
		pic_params.pic_init_qp_minus26 = pps->pic_init_qp_minus26;
		pic_params.pic_init_qs_minus26 = pps->pic_init_qs_minus26;
		pic_params.chroma_qp_index_offset = pps->chroma_qp_index_offset;
		pic_params.second_chroma_qp_index_offset = pps->second_chroma_qp_index_offset;
		pic_params.log2_max_frame_num_minus4 = sps->log2_max_frame_num_minus4;
		pic_params.pic_order_cnt_type = sps->pic_order_cnt_type;
		pic_params.log2_max_pic_order_cnt_lsb_minus4 = sps->log2_max_pic_order_cnt_lsb_minus4;
		pic_params.delta_pic_order_always_zero_flag = sps->delta_pic_order_always_zero_flag;
		pic_params.direct_8x8_inference_flag = sps->direct_8x8_inference_flag;
		pic_params.entropy_coding_mode_flag = pps->entropy_coding_mode_flag;
		pic_params.pic_order_present_flag = pps->pic_order_present_flag;
		pic_params.num_slice_groups_minus1 = pps->num_slice_groups_minus1;
		assert(pic_params.num_slice_groups_minus1 == 0);   // FMO Not supported by VA
		pic_params.slice_group_map_type = pps->slice_group_map_type;
		pic_params.deblocking_filter_control_present_flag = pps->deblocking_filter_control_present_flag;
		pic_params.redundant_pic_cnt_present_flag = pps->redundant_pic_cnt_present_flag;
		pic_params.slice_group_change_rate_minus1 = pps->slice_group_change_rate_minus1;
		pic_params.Reserved16Bits = 3; // DXVA spec
		pic_params.StatusReportFeedbackNumber = (UINT)op->decoded_frame_index + 1; // shall not be 0
		assert(pic_params.StatusReportFeedbackNumber > 0);
		pic_params.ContinuationFlag = 1;
		input.FrameArguments[input.NumFrameArguments].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_PICTURE_PARAMETERS;
		input.FrameArguments[input.NumFrameArguments].Size = sizeof(pic_params);
		input.FrameArguments[input.NumFrameArguments].pData = &pic_params;
		input.NumFrameArguments++;

		// DirectX Video Acceleration for H.264/MPEG-4 AVC Decoding, Microsoft, Updated 2010, Page 29
		//	Also: https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/gallium/drivers/d3d12/d3d12_video_dec_h264.cpp#L548
		static constexpr int vl_zscan_normal_16[] =
		{
			/* Zig-Zag scan pattern */
			0, 1, 4, 8, 5, 2, 3, 6,
			9, 12, 13, 10, 7, 11, 14, 15
		};
		static constexpr int vl_zscan_normal[] =
		{
			/* Zig-Zag scan pattern */
			 0, 1, 8,16, 9, 2, 3,10,
			17,24,32,25,18,11, 4, 5,
			12,19,26,33,40,48,41,34,
			27,20,13, 6, 7,14,21,28,
			35,42,49,56,57,50,43,36,
			29,22,15,23,30,37,44,51,
			58,59,52,45,38,31,39,46,
			53,60,61,54,47,55,62,63
		};
		DXVA_Qmatrix_H264 qmatrix = {};
		for (int i = 0; i < 6; ++i)
		{
			for (int j = 0; j < 16; ++j)
			{
				qmatrix.bScalingLists4x4[i][j] = pps->ScalingList4x4[i][vl_zscan_normal_16[j]];
			}
		}
		for (int i = 0; i < 64; ++i)
		{
			qmatrix.bScalingLists8x8[0][i] = pps->ScalingList8x8[0][vl_zscan_normal[i]];
			qmatrix.bScalingLists8x8[1][i] = pps->ScalingList8x8[1][vl_zscan_normal[i]];
		}
		input.FrameArguments[input.NumFrameArguments].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_INVERSE_QUANTIZATION_MATRIX;
		input.FrameArguments[input.NumFrameArguments].Size = sizeof(qmatrix);
		input.FrameArguments[input.NumFrameArguments].pData = &qmatrix;
		input.NumFrameArguments++;

		// DirectX Video Acceleration for H.264/MPEG-4 AVC Decoding, Microsoft, Updated 2010, Page 31
#if 1
		DXVA_Slice_H264_Short sliceinfo = {};
		sliceinfo.BSNALunitDataLocation = 0;
		sliceinfo.SliceBytesInBuffer = (UINT)op->stream_size;
		sliceinfo.wBadSliceChopping = 0; // whole slice is in the buffer
		input.FrameArguments[input.NumFrameArguments].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL;
		input.FrameArguments[input.NumFrameArguments].Size = sizeof(sliceinfo);
		input.FrameArguments[input.NumFrameArguments].pData = &sliceinfo;
		input.NumFrameArguments++;
#else
		DXVA_Slice_H264_Long sliceinfo = {};
		//sliceinfo.BSNALunitDataLocation = (UINT)op->stream_offset;
		sliceinfo.BSNALunitDataLocation = 0;
		sliceinfo.SliceBytesInBuffer = (UINT)op->stream_size;
		sliceinfo.wBadSliceChopping = 0;
		sliceinfo.first_mb_in_slice = slice_header->first_mb_in_slice;
		sliceinfo.NumMbsForSlice = 0;
		sliceinfo.BitOffsetToSliceData = 0;
		sliceinfo.slice_type = slice_header->slice_type;
		sliceinfo.luma_log2_weight_denom = slice_header->pwt.luma_log2_weight_denom;
		sliceinfo.chroma_log2_weight_denom = slice_header->pwt.chroma_log2_weight_denom;
		sliceinfo.num_ref_idx_l0_active_minus1 = slice_header->num_ref_idx_l0_active_minus1;
		sliceinfo.num_ref_idx_l1_active_minus1 = slice_header->num_ref_idx_l1_active_minus1;
		sliceinfo.slice_alpha_c0_offset_div2 = slice_header->slice_alpha_c0_offset_div2;
		sliceinfo.slice_beta_offset_div2 = slice_header->slice_beta_offset_div2;
		sliceinfo.slice_qs_delta = slice_header->slice_qs_delta;
		sliceinfo.slice_qp_delta = slice_header->slice_qp_delta;
		sliceinfo.redundant_pic_cnt = slice_header->redundant_pic_cnt;
		sliceinfo.direct_spatial_mv_pred_flag = slice_header->direct_spatial_mv_pred_flag;
		sliceinfo.cabac_init_idc = slice_header->cabac_init_idc;
		sliceinfo.disable_deblocking_filter_idc = slice_header->disable_deblocking_filter_idc;
		sliceinfo.slice_id = 0; // if picture has multiple slices, this identifies the slice id (not supported currently)
		std::memset(sliceinfo.RefPicList, 0xFF, sizeof(sliceinfo.RefPicList));
		// L0:
		switch (sliceinfo.slice_type)
		{
		case 2:
		case 4:
		case 7:
		case 9:
			// keep 0xFF
			break;
		default:
			for (int j = 0; j < sliceinfo.num_ref_idx_l0_active_minus1; ++j)
			{
				sliceinfo.RefPicList[0][j] = {};
				sliceinfo.RefPicList[0][j].Index7Bits = op->dpb_reference_slots[j];
			}
			break;
		}
		// L1:
		switch (sliceinfo.slice_type)
		{
		case 0:
		case 2:
		case 3:
		case 4:
		case 5:
		case 7:
		case 8:
		case 9:
			// keep 0xFF
			break;
		default:
			for (int j = 0; j < sliceinfo.num_ref_idx_l1_active_minus1; ++j)
			{
				sliceinfo.RefPicList[1][j] = {};
				sliceinfo.RefPicList[1][j].Index7Bits = op->dpb_reference_slots[j];
			}
			break;
		}
		// L0:
		for (int j = 0; j < 32; ++j) // weights/offsets
		{
			for (int k = 0; k < 3; ++k) // y, cb, cr
			{
				switch (k)
				{
				default:
				case 0:
					sliceinfo.Weights[0][j][k][0] = slice_header->pwt.luma_weight_l0[j];
					sliceinfo.Weights[0][j][k][1] = slice_header->pwt.luma_offset_l0[j];
					break;
				case 1:
					sliceinfo.Weights[0][j][k][0] = slice_header->pwt.chroma_weight_l0[j][0];
					sliceinfo.Weights[0][j][k][1] = slice_header->pwt.chroma_offset_l0[j][0];
					break;
				case 2:
					sliceinfo.Weights[0][j][k][0] = slice_header->pwt.chroma_weight_l0[j][1];
					sliceinfo.Weights[0][j][k][1] = slice_header->pwt.chroma_offset_l0[j][1];
					break;
				}
			}
		}
		// L1:
		for (int j = 0; j < 32; ++j) // weights/offsets
		{
			for (int k = 0; k < 3; ++k) // y, cb, cr
			{
				switch (k)
				{
				default:
				case 0:
					sliceinfo.Weights[1][j][k][0] = slice_header->pwt.luma_weight_l1[j];
					sliceinfo.Weights[1][j][k][1] = slice_header->pwt.luma_offset_l1[j];
					break;
				case 1:
					sliceinfo.Weights[1][j][k][0] = slice_header->pwt.chroma_weight_l1[j][0];
					sliceinfo.Weights[1][j][k][1] = slice_header->pwt.chroma_offset_l1[j][0];
					break;
				case 2:
					sliceinfo.Weights[1][j][k][0] = slice_header->pwt.chroma_weight_l1[j][1];
					sliceinfo.Weights[1][j][k][1] = slice_header->pwt.chroma_offset_l1[j][1];
					break;
				}
			}
		}

		input.FrameArguments[input.NumFrameArguments].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL;
		input.FrameArguments[input.NumFrameArguments].Size = sizeof(sliceinfo);
		input.FrameArguments[input.NumFrameArguments].pData = &sliceinfo;
		input.NumFrameArguments++;
#endif

		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetVideoDecodeCommandList()->DecodeFrame(
			decoder_internal->decoder.Get(),
			&output,
			&input
		);

		// Debug printout for pic params:
#if 0
		AllocConsole();
		AttachConsole(GetCurrentProcessId());
		HWND Handle = GetConsoleWindow();
		freopen("CON", "w", stdout);
		const DXVA_PicParams_H264* pPicParams = &pic_params;
		printf("\n=============================================\n");
		printf("wFrameWidthInMbsMinus1 = %d\n", pPicParams->wFrameWidthInMbsMinus1);
		printf("wFrameHeightInMbsMinus1 = %d\n", pPicParams->wFrameHeightInMbsMinus1);
		printf("CurrPic.Index7Bits = %d\n", pPicParams->CurrPic.Index7Bits);
		printf("CurrPic.AssociatedFlag = %d\n", pPicParams->CurrPic.AssociatedFlag);
		printf("num_ref_frames = %d\n", pPicParams->num_ref_frames);
		printf("sp_for_switch_flag = %d\n", pPicParams->sp_for_switch_flag);
		printf("field_pic_flag = %d\n", pPicParams->field_pic_flag);
		printf("MbaffFrameFlag = %d\n", pPicParams->MbaffFrameFlag);
		printf("residual_colour_transform_flag = %d\n", pPicParams->residual_colour_transform_flag);
		printf("chroma_format_idc = %d\n", pPicParams->chroma_format_idc);
		printf("RefPicFlag = %d\n", pPicParams->RefPicFlag);
		printf("IntraPicFlag = %d\n", pPicParams->IntraPicFlag);
		printf("constrained_intra_pred_flag = %d\n", pPicParams->constrained_intra_pred_flag);
		printf("MinLumaBipredSize8x8Flag = %d\n", pPicParams->MinLumaBipredSize8x8Flag);
		printf("weighted_pred_flag = %d\n", pPicParams->weighted_pred_flag);
		printf("weighted_bipred_idc = %d\n", pPicParams->weighted_bipred_idc);
		printf("MbsConsecutiveFlag = %d\n", pPicParams->MbsConsecutiveFlag);
		printf("frame_mbs_only_flag = %d\n", pPicParams->frame_mbs_only_flag);
		printf("transform_8x8_mode_flag = %d\n", pPicParams->transform_8x8_mode_flag);
		printf("StatusReportFeedbackNumber = %d\n", pPicParams->StatusReportFeedbackNumber);
		printf("CurrFieldOrderCnt[0] = %d\n", pPicParams->CurrFieldOrderCnt[0]);
		printf("CurrFieldOrderCnt[1] = %d\n", pPicParams->CurrFieldOrderCnt[1]);
		printf("chroma_qp_index_offset = %d\n", pPicParams->chroma_qp_index_offset);
		printf("second_chroma_qp_index_offset = %d\n", pPicParams->second_chroma_qp_index_offset);
		printf("ContinuationFlag = %d\n", pPicParams->ContinuationFlag);
		printf("pic_init_qp_minus26 = %d\n", pPicParams->pic_init_qp_minus26);
		printf("pic_init_qs_minus26 = %d\n", pPicParams->pic_init_qs_minus26);
		printf("num_ref_idx_l0_active_minus1 = %d\n", pPicParams->num_ref_idx_l0_active_minus1);
		printf("num_ref_idx_l1_active_minus1 = %d\n", pPicParams->num_ref_idx_l1_active_minus1);
		printf("frame_num = %d\n", pPicParams->frame_num);
		printf("log2_max_frame_num_minus4 = %d\n", pPicParams->log2_max_frame_num_minus4);
		printf("pic_order_cnt_type = %d\n", pPicParams->pic_order_cnt_type);
		printf("log2_max_pic_order_cnt_lsb_minus4 = %d\n", pPicParams->log2_max_pic_order_cnt_lsb_minus4);
		printf("delta_pic_order_always_zero_flag = %d\n", pPicParams->delta_pic_order_always_zero_flag);
		printf("direct_8x8_inference_flag = %d\n", pPicParams->direct_8x8_inference_flag);
		printf("entropy_coding_mode_flag = %d\n", pPicParams->entropy_coding_mode_flag);
		printf("pic_order_present_flag = %d\n", pPicParams->pic_order_present_flag);
		printf("deblocking_filter_control_present_flag = %d\n", pPicParams->deblocking_filter_control_present_flag);
		printf("redundant_pic_cnt_present_flag = %d\n", pPicParams->redundant_pic_cnt_present_flag);
		printf("num_slice_groups_minus1 = %d\n", pPicParams->num_slice_groups_minus1);
		printf("slice_group_map_type = %d\n", pPicParams->slice_group_map_type);
		printf("slice_group_change_rate_minus1 = %d\n", pPicParams->slice_group_change_rate_minus1);
		printf("Reserved8BitsB = %d\n", pPicParams->Reserved8BitsB);
		printf("UsedForReferenceFlags 0x%08x\n", pPicParams->UsedForReferenceFlags);
		printf("NonExistingFrameFlags 0x%08x\n", pPicParams->NonExistingFrameFlags);
#endif

		// Debug immediate submit-wait:
#if 0
		ComPtr<ID3D12Fence> fence;
		HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, PPV_ARGS(fence));
		assert(SUCCEEDED(hr));

		D3D12_RESOURCE_BARRIER bar = {};
		bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		bar.Transition.pResource = dpb_internal->resource.Get();
		bar.Transition.StateBefore = D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE;
		bar.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
		bar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandlist.GetVideoDecodeCommandList()->ResourceBarrier(1, &bar);

		hr = commandlist.GetVideoDecodeCommandList()->Close();
		assert(SUCCEEDED(hr));

		CommandQueue& queue = queues[commandlist.queue];
		queue.submit_cmds.push_back(commandlist.GetCommandList());
		queue.queue->ExecuteCommandLists(
			(UINT)queue.submit_cmds.size(),
			queue.submit_cmds.data()
		);
		queue.submit_cmds.clear();

		hr = queue.queue->Signal(fence.Get(), 1);
		assert(SUCCEEDED(hr));
		if (fence->GetCompletedValue() < 1)
		{
			hr = fence->SetEventOnCompletion(1, NULL);
			assert(SUCCEEDED(hr));
		}
		fence->Signal(0);

		hr = commandlist.GetCommandAllocator()->Reset();
		assert(SUCCEEDED(hr));
		hr = commandlist.GetVideoDecodeCommandList()->Reset(commandlist.GetCommandAllocator());
		assert(SUCCEEDED(hr));
#endif
	}

	void GraphicsDevice_DX12::EventBegin(const char* name, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (commandlist.queue == QUEUE_VIDEO_DECODE)
			return;
		wchar_t text[128];
		if (wi::helper::StringConvert(name, text) > 0)
		{
			PIXBeginEvent(commandlist.GetGraphicsCommandList(), 0xFF000000, text);
		}
	}
	void GraphicsDevice_DX12::EventEnd(CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (commandlist.queue == QUEUE_VIDEO_DECODE)
			return;
		PIXEndEvent(commandlist.GetGraphicsCommandList());
	}
	void GraphicsDevice_DX12::SetMarker(const char* name, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (commandlist.queue == QUEUE_VIDEO_DECODE)
			return;
		wchar_t text[128];
		if (wi::helper::StringConvert(name, text) > 0)
		{
			PIXSetMarker(commandlist.GetGraphicsCommandList(), 0xFFFF0000, text);
		}
	}

}

#endif // WICKEDENGINE_BUILD_DX12
