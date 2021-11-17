#include "wiGraphicsDevice_DX12.h"

#ifdef WICKEDENGINE_BUILD_DX12

#include "wiHelper.h"
#include "wiBackLog.h"
#include "wiTimer.h"

#include "Utility/dx12/d3dx12.h"
#include "Utility/D3D12MemAlloc.h"

#include "Utility/dxcapi.h"
#include "Utility/dx12/d3d12shader.h"

#include <string>
#include <unordered_set>

#include <pix.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")
#endif

#include <sstream>
#include <algorithm>

using namespace Microsoft::WRL;

namespace wiGraphics
{

namespace DX12_Internal
{
	// Bindless allocation limits:
#define BINDLESS_RESOURCE_CAPACITY		500000
#define BINDLESS_SAMPLER_CAPACITY		256

// Choose how many constant buffers will be placed in root in auto root signature:
#define CONSTANT_BUFFER_AUTO_PLACEMENT_IN_ROOT 4
	static_assert(DESCRIPTORBINDER_CBV_COUNT < 32, "cbv root mask must fit into uint32_t!");


#ifdef PLATFORM_UWP
	// UWP will use static link + /DELAYLOAD linker feature for the dlls (optionally)
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#else
	using PFN_CREATE_DXGI_FACTORY_2 = decltype(&CreateDXGIFactory2);
	static PFN_CREATE_DXGI_FACTORY_2 CreateDXGIFactory2 = nullptr;

#ifdef _DEBUG
	using PFN_DXGI_GET_DEBUG_INTERFACE1 = decltype(&DXGIGetDebugInterface1);
	static PFN_DXGI_GET_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
#endif

	static PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
	static PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature = nullptr;
#endif // PLATFORM_UWP

	ComPtr<IDxcUtils> dxcUtils;

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
			if (has(value, ColorWrite::ENABLE_RED))
				_flag |= D3D12_COLOR_WRITE_ENABLE_RED;
			if (has(value, ColorWrite::ENABLE_GREEN))
				_flag |= D3D12_COLOR_WRITE_ENABLE_GREEN;
			if (has(value, ColorWrite::ENABLE_BLUE))
				_flag |= D3D12_COLOR_WRITE_ENABLE_BLUE;
			if (has(value, ColorWrite::ENABLE_ALPHA))
				_flag |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
		}

		return _flag;
	}
	constexpr D3D12_RESOURCE_STATES _ParseResourceState(ResourceState value)
	{
		D3D12_RESOURCE_STATES ret = {};

		if (has(value, ResourceState::UNDEFINED))
			ret |= D3D12_RESOURCE_STATE_COMMON;
		if (has(value, ResourceState::SHADER_RESOURCE))
			ret |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (has(value, ResourceState::SHADER_RESOURCE_COMPUTE))
			ret |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (has(value, ResourceState::UNORDERED_ACCESS))
			ret |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if (has(value, ResourceState::COPY_SRC))
			ret |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		if (has(value, ResourceState::COPY_DST))
			ret |= D3D12_RESOURCE_STATE_COPY_DEST;

		if (has(value, ResourceState::RENDERTARGET))
			ret |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if (has(value, ResourceState::DEPTHSTENCIL))
			ret |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if (has(value, ResourceState::DEPTHSTENCIL_READONLY))
			ret |= D3D12_RESOURCE_STATE_DEPTH_READ;
		if (has(value, ResourceState::SHADING_RATE_SOURCE))
			ret |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

		if (has(value, ResourceState::VERTEX_BUFFER))
			ret |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (has(value, ResourceState::INDEX_BUFFER))
			ret |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if (has(value, ResourceState::CONSTANT_BUFFER))
			ret |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (has(value, ResourceState::INDIRECT_ARGUMENT))
			ret |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if (has(value, ResourceState::RAYTRACING_ACCELERATION_STRUCTURE))
			ret |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if (has(value, ResourceState::PREDICATION))
			ret |= D3D12_RESOURCE_STATE_PREDICATION;

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
			break;
		case Format::R32G32B32A32_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
			break;
		case Format::R32G32B32A32_UINT:
			return DXGI_FORMAT_R32G32B32A32_UINT;
			break;
		case Format::R32G32B32A32_SINT:
			return DXGI_FORMAT_R32G32B32A32_SINT;
			break;
		case Format::R32G32B32_FLOAT:
			return DXGI_FORMAT_R32G32B32_FLOAT;
			break;
		case Format::R32G32B32_UINT:
			return DXGI_FORMAT_R32G32B32_UINT;
			break;
		case Format::R32G32B32_SINT:
			return DXGI_FORMAT_R32G32B32_SINT;
			break;
		case Format::R16G16B16A16_FLOAT:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
			break;
		case Format::R16G16B16A16_UNORM:
			return DXGI_FORMAT_R16G16B16A16_UNORM;
			break;
		case Format::R16G16B16A16_UINT:
			return DXGI_FORMAT_R16G16B16A16_UINT;
			break;
		case Format::R16G16B16A16_SNORM:
			return DXGI_FORMAT_R16G16B16A16_SNORM;
			break;
		case Format::R16G16B16A16_SINT:
			return DXGI_FORMAT_R16G16B16A16_SINT;
			break;
		case Format::R32G32_FLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;
			break;
		case Format::R32G32_UINT:
			return DXGI_FORMAT_R32G32_UINT;
			break;
		case Format::R32G32_SINT:
			return DXGI_FORMAT_R32G32_SINT;
			break;
		case Format::R32G8X24_TYPELESS:
			return DXGI_FORMAT_R32G8X24_TYPELESS;
			break;
		case Format::D32_FLOAT_S8X24_UINT:
			return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			break;
		case Format::R10G10B10A2_UNORM:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
			break;
		case Format::R10G10B10A2_UINT:
			return DXGI_FORMAT_R10G10B10A2_UINT;
			break;
		case Format::R11G11B10_FLOAT:
			return DXGI_FORMAT_R11G11B10_FLOAT;
			break;
		case Format::R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case Format::R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			break;
		case Format::R8G8B8A8_UINT:
			return DXGI_FORMAT_R8G8B8A8_UINT;
			break;
		case Format::R8G8B8A8_SNORM:
			return DXGI_FORMAT_R8G8B8A8_SNORM;
			break;
		case Format::R8G8B8A8_SINT:
			return DXGI_FORMAT_R8G8B8A8_SINT;
			break;
		case Format::R16G16_FLOAT:
			return DXGI_FORMAT_R16G16_FLOAT;
			break;
		case Format::R16G16_UNORM:
			return DXGI_FORMAT_R16G16_UNORM;
			break;
		case Format::R16G16_UINT:
			return DXGI_FORMAT_R16G16_UINT;
			break;
		case Format::R16G16_SNORM:
			return DXGI_FORMAT_R16G16_SNORM;
			break;
		case Format::R16G16_SINT:
			return DXGI_FORMAT_R16G16_SINT;
			break;
		case Format::R32_TYPELESS:
			return DXGI_FORMAT_R32_TYPELESS;
			break;
		case Format::D32_FLOAT:
			return DXGI_FORMAT_D32_FLOAT;
			break;
		case Format::R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
			break;
		case Format::R32_UINT:
			return DXGI_FORMAT_R32_UINT;
			break;
		case Format::R32_SINT:
			return DXGI_FORMAT_R32_SINT;
			break;
		case Format::R8G8_UNORM:
			return DXGI_FORMAT_R8G8_UNORM;
			break;
		case Format::R8G8_UINT:
			return DXGI_FORMAT_R8G8_UINT;
			break;
		case Format::R8G8_SNORM:
			return DXGI_FORMAT_R8G8_SNORM;
			break;
		case Format::R8G8_SINT:
			return DXGI_FORMAT_R8G8_SINT;
			break;
		case Format::R16_TYPELESS:
			return DXGI_FORMAT_R16_TYPELESS;
			break;
		case Format::R16_FLOAT:
			return DXGI_FORMAT_R16_FLOAT;
			break;
		case Format::D16_UNORM:
			return DXGI_FORMAT_D16_UNORM;
			break;
		case Format::R16_UNORM:
			return DXGI_FORMAT_R16_UNORM;
			break;
		case Format::R16_UINT:
			return DXGI_FORMAT_R16_UINT;
			break;
		case Format::R16_SNORM:
			return DXGI_FORMAT_R16_SNORM;
			break;
		case Format::R16_SINT:
			return DXGI_FORMAT_R16_SINT;
			break;
		case Format::R8_UNORM:
			return DXGI_FORMAT_R8_UNORM;
			break;
		case Format::R8_UINT:
			return DXGI_FORMAT_R8_UINT;
			break;
		case Format::R8_SNORM:
			return DXGI_FORMAT_R8_SNORM;
			break;
		case Format::R8_SINT:
			return DXGI_FORMAT_R8_SINT;
			break;
		case Format::BC1_UNORM:
			return DXGI_FORMAT_BC1_UNORM;
			break;
		case Format::BC1_UNORM_SRGB:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
			break;
		case Format::BC2_UNORM:
			return DXGI_FORMAT_BC2_UNORM;
			break;
		case Format::BC2_UNORM_SRGB:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
			break;
		case Format::BC3_UNORM:
			return DXGI_FORMAT_BC3_UNORM;
			break;
		case Format::BC3_UNORM_SRGB:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
			break;
		case Format::BC4_UNORM:
			return DXGI_FORMAT_BC4_UNORM;
			break;
		case Format::BC4_SNORM:
			return DXGI_FORMAT_BC4_SNORM;
			break;
		case Format::BC5_UNORM:
			return DXGI_FORMAT_BC5_UNORM;
			break;
		case Format::BC5_SNORM:
			return DXGI_FORMAT_BC5_SNORM;
			break;
		case Format::B8G8R8A8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
			break;
		case Format::B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
			break;
		case Format::BC6H_UF16:
			return DXGI_FORMAT_BC6H_UF16;
			break;
		case Format::BC6H_SF16:
			return DXGI_FORMAT_BC6H_SF16;
			break;
		case Format::BC7_UNORM:
			return DXGI_FORMAT_BC7_UNORM;
			break;
		case Format::BC7_UNORM_SRGB:
			return DXGI_FORMAT_BC7_UNORM_SRGB;
			break;
		}
		return DXGI_FORMAT_UNKNOWN;
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
	constexpr D3D12_STATIC_SAMPLER_DESC _ConvertStaticSampler(const StaticSampler& x)
	{
		D3D12_STATIC_SAMPLER_DESC desc = {};
		desc.ShaderRegister = x.slot;
		desc.Filter = _ConvertFilter(x.sampler.desc.filter);
		desc.AddressU = _ConvertTextureAddressMode(x.sampler.desc.address_u);
		desc.AddressV = _ConvertTextureAddressMode(x.sampler.desc.address_v);
		desc.AddressW = _ConvertTextureAddressMode(x.sampler.desc.address_w);
		desc.MipLODBias = x.sampler.desc.mip_lod_bias;
		desc.MaxAnisotropy = x.sampler.desc.max_anisotropy;
		desc.ComparisonFunc = _ConvertComparisonFunc(x.sampler.desc.comparison_func);
		desc.BorderColor = _ConvertSamplerBorderColor(x.sampler.desc.border_color);
		desc.MinLOD = x.sampler.desc.min_lod;
		desc.MaxLOD = x.sampler.desc.max_lod;
		desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		return desc;
	}

	// Native -> Engine converters
	constexpr Format _ConvertFormat_Inv(DXGI_FORMAT value)
	{
		switch (value)
		{
		case DXGI_FORMAT_UNKNOWN:
			return Format::UNKNOWN;
			break;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return Format::R32G32B32A32_FLOAT;
			break;
		case DXGI_FORMAT_R32G32B32A32_UINT:
			return Format::R32G32B32A32_UINT;
			break;
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return Format::R32G32B32A32_SINT;
			break;
		case DXGI_FORMAT_R32G32B32_FLOAT:
			return Format::R32G32B32_FLOAT;
			break;
		case DXGI_FORMAT_R32G32B32_UINT:
			return Format::R32G32B32_UINT;
			break;
		case DXGI_FORMAT_R32G32B32_SINT:
			return Format::R32G32B32_SINT;
			break;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return Format::R16G16B16A16_FLOAT;
			break;
		case DXGI_FORMAT_R16G16B16A16_UNORM:
			return Format::R16G16B16A16_UNORM;
			break;
		case DXGI_FORMAT_R16G16B16A16_UINT:
			return Format::R16G16B16A16_UINT;
			break;
		case DXGI_FORMAT_R16G16B16A16_SNORM:
			return Format::R16G16B16A16_SNORM;
			break;
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return Format::R16G16B16A16_SINT;
			break;
		case DXGI_FORMAT_R32G32_FLOAT:
			return Format::R32G32_FLOAT;
			break;
		case DXGI_FORMAT_R32G32_UINT:
			return Format::R32G32_UINT;
			break;
		case DXGI_FORMAT_R32G32_SINT:
			return Format::R32G32_SINT;
			break;
		case DXGI_FORMAT_R32G8X24_TYPELESS:
			return Format::R32G8X24_TYPELESS;
			break;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			return Format::D32_FLOAT_S8X24_UINT;
			break;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return Format::R10G10B10A2_UNORM;
			break;
		case DXGI_FORMAT_R10G10B10A2_UINT:
			return Format::R10G10B10A2_UINT;
			break;
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return Format::R11G11B10_FLOAT;
			break;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return Format::R8G8B8A8_UNORM;
			break;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return Format::R8G8B8A8_UNORM_SRGB;
			break;
		case DXGI_FORMAT_R8G8B8A8_UINT:
			return Format::R8G8B8A8_UINT;
			break;
		case DXGI_FORMAT_R8G8B8A8_SNORM:
			return Format::R8G8B8A8_SNORM;
			break;
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return Format::R8G8B8A8_SINT;
			break;
		case DXGI_FORMAT_R16G16_FLOAT:
			return Format::R16G16_FLOAT;
			break;
		case DXGI_FORMAT_R16G16_UNORM:
			return Format::R16G16_UNORM;
			break;
		case DXGI_FORMAT_R16G16_UINT:
			return Format::R16G16_UINT;
			break;
		case DXGI_FORMAT_R16G16_SNORM:
			return Format::R16G16_SNORM;
			break;
		case DXGI_FORMAT_R16G16_SINT:
			return Format::R16G16_SINT;
			break;
		case DXGI_FORMAT_R32_TYPELESS:
			return Format::R32_TYPELESS;
			break;
		case DXGI_FORMAT_D32_FLOAT:
			return Format::D32_FLOAT;
			break;
		case DXGI_FORMAT_R32_FLOAT:
			return Format::R32_FLOAT;
			break;
		case DXGI_FORMAT_R32_UINT:
			return Format::R32_UINT;
			break;
		case DXGI_FORMAT_R32_SINT:
			return Format::R32_SINT;
			break;
		case DXGI_FORMAT_R8G8_UNORM:
			return Format::R8G8_UNORM;
			break;
		case DXGI_FORMAT_R8G8_UINT:
			return Format::R8G8_UINT;
			break;
		case DXGI_FORMAT_R8G8_SNORM:
			return Format::R8G8_SNORM;
			break;
		case DXGI_FORMAT_R8G8_SINT:
			return Format::R8G8_SINT;
			break;
		case DXGI_FORMAT_R16_TYPELESS:
			return Format::R16_TYPELESS;
			break;
		case DXGI_FORMAT_R16_FLOAT:
			return Format::R16_FLOAT;
			break;
		case DXGI_FORMAT_D16_UNORM:
			return Format::D16_UNORM;
			break;
		case DXGI_FORMAT_R16_UNORM:
			return Format::R16_UNORM;
			break;
		case DXGI_FORMAT_R16_UINT:
			return Format::R16_UINT;
			break;
		case DXGI_FORMAT_R16_SNORM:
			return Format::R16_SNORM;
			break;
		case DXGI_FORMAT_R16_SINT:
			return Format::R16_SINT;
			break;
		case DXGI_FORMAT_R8_UNORM:
			return Format::R8_UNORM;
			break;
		case DXGI_FORMAT_R8_UINT:
			return Format::R8_UINT;
			break;
		case DXGI_FORMAT_R8_SNORM:
			return Format::R8_SNORM;
			break;
		case DXGI_FORMAT_R8_SINT:
			return Format::R8_SINT;
			break;
		case DXGI_FORMAT_BC1_UNORM:
			return Format::BC1_UNORM;
			break;
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return Format::BC1_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC2_UNORM:
			return Format::BC2_UNORM;
			break;
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			return Format::BC2_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC3_UNORM:
			return Format::BC3_UNORM;
			break;
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return Format::BC3_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC4_UNORM:
			return Format::BC4_UNORM;
			break;
		case DXGI_FORMAT_BC4_SNORM:
			return Format::BC4_SNORM;
			break;
		case DXGI_FORMAT_BC5_UNORM:
			return Format::BC5_UNORM;
			break;
		case DXGI_FORMAT_BC5_SNORM:
			return Format::BC5_SNORM;
			break;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return Format::B8G8R8A8_UNORM;
			break;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return Format::B8G8R8A8_UNORM_SRGB;
			break;
		case DXGI_FORMAT_BC6H_UF16:
			return Format::BC6H_UF16;
			break;
		case DXGI_FORMAT_BC6H_SF16:
			return Format::BC6H_SF16;
			break;
		case DXGI_FORMAT_BC7_UNORM:
			return Format::BC7_UNORM;
			break;
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return Format::BC7_UNORM_SRGB;
			break;
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


	enum RESOURCEBINDING
	{
		CONSTANTBUFFER,
		RAWBUFFER,
		STRUCTUREDBUFFER,
		TYPEDBUFFER,
		TEXTURE1D,
		TEXTURE1DARRAY,
		TEXTURE2D,
		TEXTURE2DARRAY,
		TEXTURECUBE,
		TEXTURECUBEARRAY,
		TEXTURE3D,
		ACCELERATIONSTRUCTURE,
		RWRAWBUFFER,
		RWSTRUCTUREDBUFFER,
		RWTYPEDBUFFER,
		RWTEXTURE1D,
		RWTEXTURE1DARRAY,
		RWTEXTURE2D,
		RWTEXTURE2DARRAY,
		RWTEXTURE3D,

		RESOURCEBINDING_COUNT
	};

	struct SingleDescriptor
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
		D3D12_DESCRIPTOR_HEAP_TYPE type = {};
		int index = -1; // bindless
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
		void init(const GraphicsDevice_DX12* device, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbv)
		{
			this->cbv = cbv;
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
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_res.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const GraphicsDevice_DX12* device, const D3D12_SHADER_RESOURCE_VIEW_DESC& srv, ID3D12Resource* res)
		{
			this->srv = srv;
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
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_res.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const GraphicsDevice_DX12* device, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uav, ID3D12Resource* res)
		{
			this->uav = uav;
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
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_res.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const GraphicsDevice_DX12* device, const D3D12_SAMPLER_DESC& sam)
		{
			this->sam = sam;
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
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_sam.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const GraphicsDevice_DX12* device, const D3D12_RENDER_TARGET_VIEW_DESC& rtv, ID3D12Resource* res)
		{
			this->rtv = rtv;
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			handle = allocationhandler->descriptors_rtv.allocate();
			allocationhandler->device->CreateRenderTargetView(res, &rtv, handle);
		}
		void init(const GraphicsDevice_DX12* device, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsv, ID3D12Resource* res)
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
	};

	struct Resource_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		D3D12MA::Allocation* allocation = nullptr;
		ComPtr<ID3D12Resource> resource;
		SingleDescriptor srv;
		SingleDescriptor uav;
		std::vector<SingleDescriptor> subresources_srv;
		std::vector<SingleDescriptor> subresources_uav;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;

		D3D12_GPU_VIRTUAL_ADDRESS gpu_address = 0;

		uint64_t cbv_mask_frame[COMMANDLIST_COUNT] = {};
		uint32_t cbv_mask[COMMANDLIST_COUNT] = {};

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
	struct QueryHeap_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		Microsoft::WRL::ComPtr<ID3D12QueryHeap> heap;

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

		std::vector<D3D12_ROOT_DESCRIPTOR1> root_cbvs;
		std::vector<D3D12_DESCRIPTOR_RANGE1> resources;
		std::vector<D3D12_DESCRIPTOR_RANGE1> samplers;

		uint32_t resource_binding_count_unrolled = 0;
		uint32_t sampler_binding_count_unrolled = 0;

		std::vector<RESOURCEBINDING> resource_bindings;

		std::vector<D3D12_DESCRIPTOR_RANGE1> bindless_res;
		std::vector<D3D12_DESCRIPTOR_RANGE1> bindless_sam;

		D3D12_ROOT_PARAMETER1 rootconstants;

		std::vector<D3D12_STATIC_SAMPLER_DESC> staticsamplers;

		uint32_t bindpoint_rootconstant = 0;
		uint32_t bindpoint_rootdescriptor = 0;
		uint32_t bindpoint_res = 0;
		uint32_t bindpoint_sam = 0;
		uint32_t bindpoint_bindless = 0;

		std::vector<uint8_t> shadercode;
		std::vector<D3D12_INPUT_ELEMENT_DESC> input_elements;
		D3D_PRIMITIVE_TOPOLOGY primitiveTopology;

		struct PSO_STREAM
		{
			struct PSO_STREAM1
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
	struct SwapChain_DX12
	{
		std::shared_ptr<GraphicsDevice_DX12::AllocationHandler> allocationhandler;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> backbufferRTV;

		Texture dummyTexture;
		RenderPass renderpass;

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
	RenderPass_DX12* to_internal(const RenderPass* param)
	{
		return static_cast<RenderPass_DX12*>(param->internal_state.get());
	}
	SwapChain_DX12* to_internal(const SwapChain* param)
	{
		return static_cast<SwapChain_DX12*>(param->internal_state.get());
	}
}
using namespace DX12_Internal;

	// Allocators:

	void GraphicsDevice_DX12::CopyAllocator::init(GraphicsDevice_DX12* device)
	{
		this->device = device;

		D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
		copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		copyQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		copyQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		copyQueueDesc.NodeMask = 0;
		HRESULT hr = device->device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&queue));
		assert(SUCCEEDED(hr));
	}
	void GraphicsDevice_DX12::CopyAllocator::destroy()
	{
		ComPtr<ID3D12Fence> fence;
		HRESULT hr = device->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		assert(SUCCEEDED(hr));

		hr = queue->Signal(fence.Get(), 1);
		assert(SUCCEEDED(hr));
		hr = fence->SetEventOnCompletion(1, nullptr);
		assert(SUCCEEDED(hr));
	}
	GraphicsDevice_DX12::CopyAllocator::CopyCMD GraphicsDevice_DX12::CopyAllocator::allocate(uint64_t staging_size)
	{
		locker.lock();

		// create a new command list if there are no free ones:
		if (freelist.empty())
		{
			CopyCMD cmd;

			HRESULT hr = device->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&cmd.commandAllocator));
			assert(SUCCEEDED(hr));
			hr = device->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, cmd.commandAllocator.Get(), nullptr, IID_PPV_ARGS(&cmd.commandList));
			assert(SUCCEEDED(hr));

			hr = cmd.commandList->Close();
			assert(SUCCEEDED(hr));

			hr = device->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&cmd.fence));
			assert(SUCCEEDED(hr));

			freelist.push_back(cmd);
		}

		CopyCMD cmd = freelist.back();
		if (cmd.uploadbuffer.desc.size < staging_size)
		{
			// Try to search for a staging buffer that can fit the request:
			for (size_t i = 0; i < freelist.size(); ++i)
			{
				if (freelist[i].uploadbuffer.desc.size >= staging_size)
				{
					cmd = freelist[i];
					std::swap(freelist[i], freelist.back());
					break;
				}
			}
		}
		freelist.pop_back();
		locker.unlock();

		// If no buffer was found that fits the data, create one:
		if (cmd.uploadbuffer.desc.size < staging_size)
		{
			GPUBufferDesc uploadBufferDesc;
			uploadBufferDesc.size = wiMath::GetNextPowerOfTwo(staging_size);
			uploadBufferDesc.usage = Usage::UPLOAD;
			bool upload_success = device->CreateBuffer(&uploadBufferDesc, nullptr, &cmd.uploadbuffer);
			assert(upload_success);
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

		cmd.commandList->Close();
		ID3D12CommandList* commandlists[] = {
			cmd.commandList.Get()
		};
		queue->ExecuteCommandLists(1, commandlists);
		hr = queue->Signal(cmd.fence.Get(), 1);
		assert(SUCCEEDED(hr));
		hr = cmd.fence->SetEventOnCompletion(1, nullptr);
		assert(SUCCEEDED(hr));
		hr = cmd.fence->Signal(0);
		assert(SUCCEEDED(hr));

		locker.lock();
		freelist.push_back(cmd);
		locker.unlock();
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
		dirty_res = true;
		dirty_sam = true;
		ringOffset_res = 0;
		ringOffset_sam = 0;
	}
	void GraphicsDevice_DX12::DescriptorBinder::flush(bool graphics, CommandList cmd)
	{
		auto pso_internal = graphics ? to_internal(device->active_pso[cmd]) : to_internal(device->active_cs[cmd]);

		// Bind root descriptors:
		if (dirty_root_cbvs != 0)
		{
			uint32_t root_param = pso_internal->bindpoint_rootdescriptor;
			for (auto& x : pso_internal->root_cbvs)
			{
				bool dirty = dirty_root_cbvs & (1 << x.ShaderRegister);
				if (!dirty)
				{
					root_param++;
					continue;
				}

				const GPUBuffer& buffer = table.CBV[x.ShaderRegister];
				uint64_t offset = table.CBV_offset[x.ShaderRegister];

				D3D12_GPU_VIRTUAL_ADDRESS address;

				if (!buffer.IsValid())
				{
					// this must not happen, root descriptor must be always valid!
					// this happens when constant buffer was not bound by engine
					//	TODO: use a null initialized GPU VA here for safety?
					address = 0;
					assert(0);
				}
				else
				{
					auto internal_state = to_internal(&buffer);
					address = internal_state->gpu_address;
					address += offset;
				}

				if (graphics)
				{
					device->GetCommandList(cmd)->SetGraphicsRootConstantBufferView(root_param, address);
				}
				else
				{
					device->GetCommandList(cmd)->SetComputeRootConstantBufferView(root_param, address);
				}
				root_param++;
			}

			dirty_root_cbvs = 0;
		}


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


		// Resources:
		if (pso_internal->resource_binding_count_unrolled > 0 && dirty_res)
		{
			uint32_t resources = pso_internal->resource_binding_count_unrolled;
			if (resources > 0)
			{
				// The reservation is the maximum amount of descriptors that can be allocated once
				//	It can be increased if needed
				const uint32_t wrap_reservation = 1000;
				const uint32_t wrap_effective_size = device->descriptorheap_res.heapDesc.NumDescriptors - BINDLESS_RESOURCE_CAPACITY - wrap_reservation;
				assert(wrap_reservation > resources); // for correct lockless wrap behaviour

				const uint64_t offset = device->descriptorheap_res.allocationOffset.fetch_add(resources);
				const uint64_t wrapped_offset = BINDLESS_RESOURCE_CAPACITY + offset % wrap_effective_size;
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

			dirty_res = false;
			auto& heap = device->descriptorheap_res;
			D3D12_GPU_DESCRIPTOR_HANDLE binding_table = heap.start_gpu;
			binding_table.ptr += (UINT64)ringOffset_res * (UINT64)device->resource_descriptor_size;

			int i = 0;
			for (auto& x : pso_internal->resources)
			{
				RESOURCEBINDING binding = pso_internal->resource_bindings[i++];

				for (UINT descriptor_index = 0; descriptor_index < x.NumDescriptors; ++descriptor_index)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst = heap.start_cpu;
					uint32_t ringOffset = ringOffset_res++;
					dst.ptr += ringOffset * device->resource_descriptor_size;

					UINT ShaderRegister = x.BaseShaderRegister + descriptor_index;

					switch (x.RangeType)
					{
					default:
					case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
					{
						const GPUResource& resource = table.SRV[ShaderRegister];
						const int subresource = table.SRV_index[ShaderRegister];
						if (!resource.IsValid())
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
							auto internal_state = to_internal(&resource);

							if (resource.IsAccelerationStructure())
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
						const GPUResource& resource = table.UAV[ShaderRegister];
						const int subresource = table.UAV_index[ShaderRegister];
						if (!resource.IsValid())
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
							auto internal_state = to_internal(&resource);

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
						const GPUBuffer& buffer = table.CBV[ShaderRegister];
						uint64_t offset = table.CBV_offset[ShaderRegister];

						if (!buffer.IsValid())
						{
							device->device->CopyDescriptorsSimple(1, dst, device->nullCBV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
						}
						else
						{
							auto internal_state = to_internal(&buffer);

							D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
							cbv.BufferLocation = internal_state->gpu_address;
							cbv.BufferLocation += offset;
							cbv.SizeInBytes = AlignTo((UINT)buffer.desc.size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

							device->device->CreateConstantBufferView(&cbv, dst);
						}
					}
					break;
					}
				}
			}

			if (graphics)
			{
				device->GetCommandList(cmd)->SetGraphicsRootDescriptorTable(pso_internal->bindpoint_res, binding_table);
			}
			else
			{
				device->GetCommandList(cmd)->SetComputeRootDescriptorTable(pso_internal->bindpoint_res, binding_table);
			}
		}

		// Samplers:
		if (pso_internal->sampler_binding_count_unrolled > 0 && dirty_sam)
		{
			uint32_t samplers = pso_internal->sampler_binding_count_unrolled;
			if (samplers > 0)
			{
				// The reservation is the maximum amount of descriptors that can be allocated once
				//	It can be increased if needed
				const uint32_t wrap_reservation = 16;
				const uint32_t wrap_effective_size = device->descriptorheap_sam.heapDesc.NumDescriptors - BINDLESS_SAMPLER_CAPACITY - wrap_reservation;
				assert(wrap_reservation > samplers); // for correct lockless wrap behaviour

				const uint64_t offset = device->descriptorheap_sam.allocationOffset.fetch_add(samplers);
				const uint64_t wrapped_offset = BINDLESS_SAMPLER_CAPACITY + offset % wrap_effective_size;
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

			dirty_sam = false;
			auto& heap = device->descriptorheap_sam;
			D3D12_GPU_DESCRIPTOR_HANDLE binding_table = heap.start_gpu;
			binding_table.ptr += (UINT64)ringOffset_sam * (UINT64)device->sampler_descriptor_size;

			for (auto& x : pso_internal->samplers)
			{
				for (UINT descriptor_index = 0; descriptor_index < x.NumDescriptors; ++descriptor_index)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst = heap.start_cpu;
					uint32_t ringOffset = ringOffset_sam++;
					dst.ptr += ringOffset * device->sampler_descriptor_size;

					UINT ShaderRegister = x.BaseShaderRegister + descriptor_index;

					const Sampler& sampler = table.SAM[ShaderRegister];
					if (!sampler.IsValid())
					{
						device->device->CopyDescriptorsSimple(1, dst, device->nullSAM, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
					}
					else
					{
						auto internal_state = to_internal(&sampler);
						device->device->CopyDescriptorsSimple(1, dst, internal_state->descriptor.handle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
					}
				}
			}

			if (graphics)
			{
				device->GetCommandList(cmd)->SetGraphicsRootDescriptorTable(pso_internal->bindpoint_sam, binding_table);
			}
			else
			{
				device->GetCommandList(cmd)->SetComputeRootDescriptorTable(pso_internal->bindpoint_sam, binding_table);
			}
		}

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
				// make copy, mustn't overwrite internal_state from here!
				PipelineState_DX12::PSO_STREAM stream = internal_state->stream;

				DXGI_FORMAT DSFormat = DXGI_FORMAT_UNKNOWN;
				D3D12_RT_FORMAT_ARRAY formats = {};
				formats.NumRenderTargets = 0;
				DXGI_SAMPLE_DESC sampleDesc = {};
				sampleDesc.Count = 1;
				sampleDesc.Quality = 0;
				for (auto& attachment : active_renderpass[cmd]->desc.attachments)
				{
					if (attachment.type == RenderPassAttachment::Type::RESOLVE ||
						attachment.type == RenderPassAttachment::Type::SHADING_RATE_SOURCE ||
						attachment.texture == nullptr)
						continue;

					switch (attachment.type)
					{
					case RenderPassAttachment::Type::RENDERTARGET:
						switch (attachment.texture->desc.format)
						{
						case Format::R16_TYPELESS:
							formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R16_UNORM;
							break;
						case Format::R32_TYPELESS:
							formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT;
							break;
						case Format::R24G8_TYPELESS:
							formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
							break;
						case Format::R32G8X24_TYPELESS:
							formats.RTFormats[formats.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
							break;
						default:
							formats.RTFormats[formats.NumRenderTargets] = _ConvertFormat(attachment.texture->desc.format);
							break;
						}
						formats.NumRenderTargets++;
						break;
					case RenderPassAttachment::Type::DEPTH_STENCIL:
						switch (attachment.texture->desc.format)
						{
						case Format::R16_TYPELESS:
							DSFormat = DXGI_FORMAT_D16_UNORM;
							break;
						case Format::R32_TYPELESS:
							DSFormat = DXGI_FORMAT_D32_FLOAT;
							break;
						case Format::R24G8_TYPELESS:
							DSFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
							break;
						case Format::R32G8X24_TYPELESS:
							DSFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
							break;
						default:
							DSFormat = _ConvertFormat(attachment.texture->desc.format);
							break;
						}
						break;
					default:
						assert(0);
						break;
					}

					sampleDesc.Count = attachment.texture->desc.sample_count;
					sampleDesc.Quality = 0;
				}
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

		GetCommandList(cmd)->SetPipelineState(pipeline);
		dirty_pso[cmd] = false;

		if (prev_pt[cmd] != internal_state->primitiveTopology)
		{
			prev_pt[cmd] = internal_state->primitiveTopology;

			GetCommandList(cmd)->IASetPrimitiveTopology(internal_state->primitiveTopology);
		}
	}

	void GraphicsDevice_DX12::predraw(CommandList cmd)
	{
		pso_validate(cmd);

		binders[cmd].flush(true, cmd);

		auto pso_internal = to_internal(active_pso[cmd]);
		if (pso_internal->rootconstants.Constants.Num32BitValues > 0)
		{
			GetCommandList(cmd)->SetGraphicsRoot32BitConstants(
				pso_internal->bindpoint_rootconstant,
				pso_internal->rootconstants.Constants.Num32BitValues,
				pushconstants[cmd].data,
				0
			);
		}
	}
	void GraphicsDevice_DX12::predispatch(CommandList cmd)
	{
		binders[cmd].flush(false, cmd);

		auto cs_internal = to_internal(active_cs[cmd]);
		if (cs_internal->rootconstants.Constants.Num32BitValues > 0)
		{
			GetCommandList(cmd)->SetComputeRoot32BitConstants(
				cs_internal->bindpoint_rootconstant,
				cs_internal->rootconstants.Constants.Num32BitValues,
				pushconstants[cmd].data,
				0
			);
		}
	}


	// Engine functions
	GraphicsDevice_DX12::GraphicsDevice_DX12(bool debuglayer, bool gpuvalidation)
	{
		wiTimer timer;

		ALLOCATION_MIN_ALIGNMENT = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		SHADER_IDENTIFIER_SIZE = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

		DEBUGDEVICE = debuglayer;

		HMODULE dxcompiler = wiLoadLibrary("dxcompiler.dll");

#ifdef PLATFORM_UWP
#else
		HMODULE dxgi = LoadLibraryExW(L"dxgi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		HMODULE dx12 = LoadLibraryExW(L"d3d12.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

		CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY_2)GetProcAddress(dxgi, "CreateDXGIFactory2");
		assert(CreateDXGIFactory2 != nullptr);

#ifdef _DEBUG
		DXGIGetDebugInterface1 = (PFN_DXGI_GET_DEBUG_INTERFACE1)GetProcAddress(dxgi, "DXGIGetDebugInterface1");
#endif

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
				ComPtr<ID3D12Debug> d3dDebug;
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug))))
				{
					d3dDebug->EnableDebugLayer();
					if (gpuvalidation)
					{
						ComPtr<ID3D12Debug1> d3dDebug1;
						if (SUCCEEDED(d3dDebug.As(&d3dDebug1)))
						{
							d3dDebug1->SetEnableGPUBasedValidation(TRUE);
						}
					}
				}
			}

#if defined(_DEBUG)
			ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
			{
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

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
#endif

		hr = CreateDXGIFactory2(debuglayer ? DXGI_CREATE_FACTORY_DEBUG : 0u, IID_PPV_ARGS(&dxgiFactory));
		if (FAILED(hr))
		{
			std::stringstream ss("");
			ss << "Failed to create DXGI factory! ERROR: " << std::hex << hr;
			wiHelper::messageBox(ss.str(), "Error!");
			assert(0);
			wiPlatform::Exit();
		}

		// Determines whether tearing support is available for fullscreen borderless windows.
		{
			BOOL allowTearing = FALSE;

			ComPtr<IDXGIFactory5> dxgiFactory5;
			HRESULT hr = dxgiFactory.As(&dxgiFactory5);
			if (SUCCEEDED(hr))
			{
				hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
			}

			if (FAILED(hr) || !allowTearing)
			{
				tearingSupported = false;
#ifdef _DEBUG
				OutputDebugStringA("WARNING: Variable refresh rate displays not supported\n");
#endif
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
				return dxgiFactory6->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(ppAdapter));
			else
				return dxgiFactory->EnumAdapters1(index, ppAdapter);
		};

		for (uint32_t i = 0; NextAdapter(i, dxgiAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 adapterDesc;
			dxgiAdapter->GetDesc1(&adapterDesc);

			// Don't select the Basic Render Driver adapter.
			if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				continue;
			}

			D3D_FEATURE_LEVEL featurelevels[] = {
				D3D_FEATURE_LEVEL_12_2,
				D3D_FEATURE_LEVEL_12_1,
				D3D_FEATURE_LEVEL_12_0,
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
			};
			for (auto& featurelevel : featurelevels)
			{
				if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter.Get(), featurelevel, IID_PPV_ARGS(&device))))
				{
					break;
				}
			}
			if (device != nullptr)
				break;
		}

		if (dxgiAdapter == nullptr)
		{
			wiHelper::messageBox("DXGI: No capable adapter found!", "Error!");
			assert(0);
			wiPlatform::Exit();
		}

		if (device == nullptr)
		{
			wiHelper::messageBox("D3D12: Device couldn't be created!", "Error!");
			assert(0);
			wiPlatform::Exit();
		}

		if (debuglayer)
		{
			// Configure debug device (if active).
			ComPtr<ID3D12InfoQueue> d3dInfoQueue;
			if (SUCCEEDED(device.As(&d3dInfoQueue)))
			{
#ifdef _DEBUG
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
				d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
#endif

				D3D12_MESSAGE_ID hide[] =
				{
					D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
					D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
					D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
					D3D12_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
					// Add more message IDs here as needed
				};

				D3D12_INFO_QUEUE_FILTER filter = {};
				filter.DenyList.NumIDs = _countof(hide);
				filter.DenyList.pIDList = hide;
				d3dInfoQueue->AddStorageFilterEntries(&filter);
			}
		}

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = device.Get();
		allocatorDesc.pAdapter = dxgiAdapter.Get();

		allocationhandler = std::make_shared<AllocationHandler>();
		allocationhandler->device = device;

		hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocationhandler->allocator);
		assert(SUCCEEDED(hr));

		queues[QUEUE_GRAPHICS].desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queues[QUEUE_GRAPHICS].desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queues[QUEUE_GRAPHICS].desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queues[QUEUE_GRAPHICS].desc.NodeMask = 0;
		hr = device->CreateCommandQueue(&queues[QUEUE_GRAPHICS].desc, IID_PPV_ARGS(&queues[QUEUE_GRAPHICS].queue));
		assert(SUCCEEDED(hr));
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&queues[QUEUE_GRAPHICS].fence));
		assert(SUCCEEDED(hr));

		queues[QUEUE_COMPUTE].desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		queues[QUEUE_COMPUTE].desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queues[QUEUE_COMPUTE].desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queues[QUEUE_COMPUTE].desc.NodeMask = 0;
		hr = device->CreateCommandQueue(&queues[QUEUE_COMPUTE].desc, IID_PPV_ARGS(&queues[QUEUE_COMPUTE].queue));
		assert(SUCCEEDED(hr));
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&queues[QUEUE_COMPUTE].fence));
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
			hr = device->CreateDescriptorHeap(&descriptorheap_sam.heapDesc, IID_PPV_ARGS(&descriptorheap_sam.heap_GPU));
			assert(SUCCEEDED(hr));

			descriptorheap_sam.start_cpu = descriptorheap_sam.heap_GPU->GetCPUDescriptorHandleForHeapStart();
			descriptorheap_sam.start_gpu = descriptorheap_sam.heap_GPU->GetGPUDescriptorHandleForHeapStart();

			hr = device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&descriptorheap_sam.fence));
			assert(SUCCEEDED(hr));
			descriptorheap_sam.fenceValue = descriptorheap_sam.fence->GetCompletedValue();

			for (int i = 0; i < BINDLESS_SAMPLER_CAPACITY; ++i)
			{
				allocationhandler->free_bindless_sam.push_back(BINDLESS_SAMPLER_CAPACITY - i - 1);
			}
		}

		// Create frame-resident resources:
		for (uint32_t fr = 0; fr < BUFFERCOUNT; ++fr)
		{
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frames[fr].fence[queue]));
				assert(SUCCEEDED(hr));
			}
		}

		copyAllocator.init(this);

		// Query features:

		capabilities |= GraphicsDeviceCapability::TESSELLATION;
		capabilities |= GraphicsDeviceCapability::PREDICATION;

		// Init feature check (https://devblogs.microsoft.com/directx/introducing-a-new-api-for-checking-feature-support-in-direct3d-12/)
		CD3DX12FeatureSupport features;
		hr = features.Init(device.Get());
		assert(SUCCEEDED(hr));

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
		if (features.TiledResourcesTier() >= D3D12_TILED_RESOURCES_TIER_2)
		{
			// https://docs.microsoft.com/en-us/windows/win32/direct3d11/tiled-resources-texture-sampling-features
			capabilities |= GraphicsDeviceCapability::SAMPLER_MINMAX;
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

		if (features.HighestRootSignatureVersion() < D3D_ROOT_SIGNATURE_VERSION_1_1)
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

		if (CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
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
		if(CheckCapability(GraphicsDeviceCapability::RAYTRACING))
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


		hr = queues[QUEUE_GRAPHICS].queue->GetTimestampFrequency(&TIMESTAMP_FREQUENCY);
		assert(SUCCEEDED(hr));

		wiBackLog::post("Created GraphicsDevice_DX12 (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}
	GraphicsDevice_DX12::~GraphicsDevice_DX12()
	{
		WaitForGPU();
		copyAllocator.destroy();
	}

	bool GraphicsDevice_DX12::CreateSwapChain(const SwapChainDesc* pDesc, wiPlatform::window_type window, SwapChain* swapChain) const
	{
		auto internal_state = std::static_pointer_cast<SwapChain_DX12>(swapChain->internal_state);
		if (swapChain->internal_state == nullptr)
		{
			internal_state = std::make_shared<SwapChain_DX12>();
		}
		internal_state->allocationhandler = allocationhandler;
		swapChain->internal_state = internal_state;
		swapChain->desc = *pDesc;
		HRESULT hr;

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
			swapChainDesc.Width = pDesc->width;
			swapChainDesc.Height = pDesc->height;
			swapChainDesc.Format = _ConvertFormat(pDesc->format);
			swapChainDesc.Stereo = false;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = pDesc->buffer_count;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			swapChainDesc.Flags = swapChainFlags;

#ifndef PLATFORM_UWP
			swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
			fullscreenDesc.Windowed = !pDesc->fullscreen;

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
#endif

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
			WaitForGPU();
			internal_state->backBuffers.clear();
			for (auto& x : internal_state->backbufferRTV)
			{
				allocationhandler->descriptors_rtv.free(x);
			}
			internal_state->backbufferRTV.clear();

			hr = internal_state->swapChain->ResizeBuffers(
				pDesc->buffer_count,
				pDesc->width,
				pDesc->height,
				_ConvertFormat(pDesc->format),
				swapChainFlags
			);
			assert(SUCCEEDED(hr));
		}

		const bool hdr = pDesc->allow_hdr && IsSwapChainSupportsHDR(swapChain);

		// Ensure correct color space:
		//	https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HDR/src/D3D12HDR.cpp
		{
			internal_state->colorSpace = ColorSpace::SRGB; // reset to SDR, in case anything below fails to set HDR state
			DXGI_COLOR_SPACE_TYPE colorSpace = {};

			switch (pDesc->format)
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

		internal_state->backBuffers.resize(pDesc->buffer_count);
		internal_state->backbufferRTV.resize(pDesc->buffer_count);

		// We can create swapchain just with given supported format, thats why we specify format in RTV
		// For example: BGRA8UNorm for SwapChain BGRA8UNormSrgb for RTV.
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = _ConvertFormat(pDesc->format);
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		for (uint32_t i = 0; i < pDesc->buffer_count; ++i)
		{
			hr = internal_state->swapChain->GetBuffer(i, IID_PPV_ARGS(&internal_state->backBuffers[i]));
			assert(SUCCEEDED(hr));

			internal_state->backbufferRTV[i] = allocationhandler->descriptors_rtv.allocate();
			device->CreateRenderTargetView(internal_state->backBuffers[i].Get(), &rtvDesc, internal_state->backbufferRTV[i]);
		}

		internal_state->dummyTexture.desc.format = pDesc->format;
		internal_state->renderpass = RenderPass();
		wiHelper::hash_combine(internal_state->renderpass.hash, pDesc->format);
		internal_state->renderpass.desc.attachments.push_back(RenderPassAttachment::RenderTarget(&internal_state->dummyTexture));

		return true;
	}
	bool GraphicsDevice_DX12::CreateBuffer(const GPUBufferDesc* pDesc, const void* pInitialData, GPUBuffer* pBuffer) const
	{
		auto internal_state = std::make_shared<Resource_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pBuffer->internal_state = internal_state;
		pBuffer->type = GPUResource::Type::BUFFER;
		pBuffer->mapped_data = nullptr;
		pBuffer->mapped_rowpitch = 0;

		pBuffer->desc = *pDesc;

		HRESULT hr = E_FAIL;

		UINT64 alignedSize = pDesc->size;
		if (has(pDesc->bind_flags, BindFlag::CONSTANT_BUFFER))
		{
			alignedSize = AlignTo(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
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
		if (has(pDesc->bind_flags, BindFlag::UNORDERED_ACCESS))
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		if (!has(pDesc->bind_flags, BindFlag::SHADER_RESOURCE) && !has(pDesc->misc_flags, ResourceMiscFlag::RAY_TRACING))
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		if (pDesc->usage == Usage::READBACK)
		{
			allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
			resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
		else if (pDesc->usage == Usage::UPLOAD)
		{
			allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
			resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}

		device->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &internal_state->footprint, nullptr, nullptr, nullptr);

		hr = allocationhandler->allocator->CreateResource(
			&allocationDesc,
			&resourceDesc,
			resourceState,
			nullptr,
			&internal_state->allocation,
			IID_PPV_ARGS(&internal_state->resource)
		);
		assert(SUCCEEDED(hr));

		internal_state->gpu_address = internal_state->resource->GetGPUVirtualAddress();

		if (pDesc->usage == Usage::READBACK)
		{
			hr = internal_state->resource->Map(0, nullptr, &pBuffer->mapped_data);
			assert(SUCCEEDED(hr));
			pBuffer->mapped_rowpitch = static_cast<uint32_t>(pDesc->size);
		}
		else if (pDesc->usage == Usage::UPLOAD)
		{
			D3D12_RANGE read_range = {};
			hr = internal_state->resource->Map(0, &read_range, &pBuffer->mapped_data);
			assert(SUCCEEDED(hr));
			pBuffer->mapped_rowpitch = static_cast<uint32_t>(pDesc->size);
		}

		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			auto cmd = copyAllocator.allocate(pDesc->size);

			memcpy(cmd.uploadbuffer.mapped_data, pInitialData, pDesc->size);

			cmd.commandList->CopyBufferRegion(
				internal_state->resource.Get(),
				0,
				to_internal(&cmd.uploadbuffer)->resource.Get(),
				0,
				pDesc->size
			);

			copyAllocator.submit(cmd);
		}


		// Create resource views if needed
		if (has(pDesc->bind_flags, BindFlag::SHADER_RESOURCE))
		{
			CreateSubresource(pBuffer, SubresourceType::SRV, 0);
		}
		if (has(pDesc->bind_flags, BindFlag::UNORDERED_ACCESS))
		{
			CreateSubresource(pBuffer, SubresourceType::UAV, 0);
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture) const
	{
		auto internal_state = std::make_shared<Texture_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pTexture->internal_state = internal_state;
		pTexture->type = GPUResource::Type::TEXTURE;
		pTexture->mapped_data = nullptr;
		pTexture->mapped_rowpitch = 0;

		pTexture->desc = *pDesc;


		HRESULT hr = E_FAIL;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC desc;
		desc.Format = _ConvertFormat(pDesc->format);
		desc.Width = pDesc->width;
		desc.Height = pDesc->height;
		desc.MipLevels = pDesc->mip_levels;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.DepthOrArraySize = (UINT16)pDesc->array_size;
		desc.SampleDesc.Count = pDesc->sample_count;
		desc.SampleDesc.Quality = 0;
		desc.Alignment = 0;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (has(pDesc->bind_flags, BindFlag::DEPTH_STENCIL))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			//allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
			if (!has(pDesc->bind_flags, BindFlag::SHADER_RESOURCE))
			{
				desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}
		if (has(pDesc->bind_flags, BindFlag::RENDER_TARGET))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			//allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
		}
		if (has(pDesc->bind_flags, BindFlag::UNORDERED_ACCESS))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		switch (pTexture->desc.type)
		{
		case TextureDesc::Type::TEXTURE_1D:
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			break;
		case TextureDesc::Type::TEXTURE_2D:
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			break;
		case TextureDesc::Type::TEXTURE_3D:
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			desc.DepthOrArraySize = (UINT16)pDesc->depth;
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
		optimizedClearValue.DepthStencil.Depth = pTexture->desc.clear.depth_stencil.depth;
		optimizedClearValue.DepthStencil.Stencil = pTexture->desc.clear.depth_stencil.stencil;
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
		bool useClearValue = has(pDesc->bind_flags, BindFlag::RENDER_TARGET) || has(pDesc->bind_flags, BindFlag::DEPTH_STENCIL);

		D3D12_RESOURCE_STATES resourceState = _ParseResourceState(pTexture->desc.layout);

		if (pInitialData != nullptr)
		{
			resourceState = D3D12_RESOURCE_STATE_COMMON;
		}

		if (pTexture->desc.usage == Usage::READBACK || pTexture->desc.usage == Usage::UPLOAD)
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

			if (pTexture->desc.usage == Usage::READBACK)
			{
				allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
				resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
			}
			else if(pTexture->desc.usage == Usage::UPLOAD)
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

		if (pTexture->desc.usage == Usage::READBACK)
		{
			hr = internal_state->resource->Map(0, nullptr, &pTexture->mapped_data);
			assert(SUCCEEDED(hr));
			pTexture->mapped_rowpitch = internal_state->footprint.Footprint.RowPitch;
		}
		else if(pTexture->desc.usage == Usage::UPLOAD)
		{
			D3D12_RANGE read_range = {};
			hr = internal_state->resource->Map(0, &read_range, &pTexture->mapped_data);
			assert(SUCCEEDED(hr));
			pTexture->mapped_rowpitch = internal_state->footprint.Footprint.RowPitch;
		}

		if (pTexture->desc.mip_levels == 0)
		{
			pTexture->desc.mip_levels = (uint32_t)log2(std::max(pTexture->desc.width, pTexture->desc.height)) + 1;
		}

		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			uint32_t dataCount = pDesc->array_size * std::max(1u, pDesc->mip_levels);
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

			auto cmd = copyAllocator.allocate(RequiredSize);

			for (uint32_t i = 0; i < dataCount; ++i)
			{
				if (rowSizesInBytes[i] > (SIZE_T)-1)
					continue;
				D3D12_MEMCPY_DEST DestData = {};
				DestData.pData = (void*)((UINT64)cmd.uploadbuffer.mapped_data + layouts[i].Offset);
				DestData.RowPitch = (SIZE_T)layouts[i].Footprint.RowPitch;
				DestData.SlicePitch = (SIZE_T)layouts[i].Footprint.RowPitch * (SIZE_T)numRows[i];
				MemcpySubresource(&DestData, &data[i], (SIZE_T)rowSizesInBytes[i], numRows[i], layouts[i].Footprint.Depth);
			}

			for (UINT i = 0; i < dataCount; ++i)
			{
				CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state->resource.Get(), i);
				CD3DX12_TEXTURE_COPY_LOCATION Src(to_internal(&cmd.uploadbuffer)->resource.Get(), layouts[i]);
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

		if (has(pTexture->desc.bind_flags, BindFlag::RENDER_TARGET))
		{
			CreateSubresource(pTexture, SubresourceType::RTV, 0, -1, 0, -1);
		}
		if (has(pTexture->desc.bind_flags, BindFlag::DEPTH_STENCIL))
		{
			CreateSubresource(pTexture, SubresourceType::DSV, 0, -1, 0, -1);
		}
		if (has(pTexture->desc.bind_flags, BindFlag::SHADER_RESOURCE))
		{
			CreateSubresource(pTexture, SubresourceType::SRV, 0, -1, 0, -1);
		}
		if (has(pTexture->desc.bind_flags, BindFlag::UNORDERED_ACCESS))
		{
			CreateSubresource(pTexture, SubresourceType::UAV, 0, -1, 0, -1);
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateShader(ShaderStage stage, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader) const
	{
		auto internal_state = std::make_shared<PipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pShader->internal_state = internal_state;

		internal_state->shadercode.resize(BytecodeLength);
		std::memcpy(internal_state->shadercode.data(), pShaderBytecode, BytecodeLength);
		pShader->stage = stage;

		HRESULT hr = (internal_state->shadercode.empty() ? E_FAIL : S_OK);
		assert(SUCCEEDED(hr));

		std::unordered_set<std::string> library_binding_resolver;

		{
			auto insert_descriptor = [&](const D3D12_SHADER_INPUT_BIND_DESC& desc, const D3D12_SHADER_BUFFER_DESC& bufferdesc)
			{
				if (library_binding_resolver.count(desc.Name) != 0)
				{
					return;
				}
				library_binding_resolver.insert(desc.Name);

				if (desc.Type == D3D_SIT_CBUFFER && desc.BindPoint >= 999)
				{
					internal_state->rootconstants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
					internal_state->rootconstants.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					internal_state->rootconstants.Constants.ShaderRegister = desc.BindPoint;
					internal_state->rootconstants.Constants.RegisterSpace = desc.Space;
					internal_state->rootconstants.Constants.Num32BitValues = bufferdesc.Size / sizeof(uint32_t);
					return;
				}

				const bool bindless = desc.Space > 0;

				if (desc.Type == D3D_SIT_SAMPLER)
				{
					if (!bindless && desc.Space == 0)
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
					}

					D3D12_DESCRIPTOR_RANGE1& descriptor = bindless ? internal_state->bindless_sam.emplace_back() : internal_state->samplers.emplace_back();

					descriptor.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

					if (bindless)
					{
						descriptor.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
					}

					descriptor.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
					descriptor.BaseShaderRegister = desc.BindPoint;
					descriptor.NumDescriptors = desc.BindCount == 0 ? ~0 : desc.BindCount;
					descriptor.RegisterSpace = desc.Space;
					descriptor.OffsetInDescriptorsFromTableStart = bindless ? 0 : D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				}
				else
				{
					D3D12_DESCRIPTOR_RANGE1& descriptor = bindless ? internal_state->bindless_res.emplace_back() : internal_state->resources.emplace_back();

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

					if (bindless)
					{
						// bindless is always volatile
						descriptor.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
					}

					descriptor.BaseShaderRegister = desc.BindPoint;
					descriptor.NumDescriptors = desc.BindCount == 0 ? ~0 : desc.BindCount;
					descriptor.RegisterSpace = desc.Space;
					descriptor.OffsetInDescriptorsFromTableStart = bindless ? 0 : D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

					if (bindless)
					{
						return;
					}

					RESOURCEBINDING& binding = internal_state->resource_bindings.emplace_back();

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

			if (stage == ShaderStage::LIB)
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
						D3D12_SHADER_BUFFER_DESC bufferdesc = {};
						if (desc.Type == D3D_SIT_CBUFFER)
						{
							auto constantbuffer = function_reflection->GetConstantBufferByIndex(i);
							if (constantbuffer != nullptr)
							{
								hr = constantbuffer->GetDesc(&bufferdesc);
								assert(SUCCEEDED(hr));
							}
						}
						insert_descriptor(desc, bufferdesc);
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
					D3D12_SHADER_BUFFER_DESC bufferdesc = {};
					if (desc.Type == D3D_SIT_CBUFFER)
					{
						auto constantbuffer = reflection->GetConstantBufferByIndex(i);
						if (constantbuffer != nullptr)
						{
							hr = constantbuffer->GetDesc(&bufferdesc);
							assert(SUCCEEDED(hr));
						}
					}
					insert_descriptor(desc, bufferdesc);
				}
			}

			for (auto& sam : internal_state->staticsamplers)
			{
				switch (stage)
				{
				case ShaderStage::MS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_MESH;
					break;
				case ShaderStage::AS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_AMPLIFICATION;
					break;
				case ShaderStage::VS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
					break;
				case ShaderStage::HS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
					break;
				case ShaderStage::DS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
					break;
				case ShaderStage::GS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
					break;
				case ShaderStage::PS:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
					break;
				default:
					sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					break;
				}
			}


			if (stage == ShaderStage::CS || stage == ShaderStage::LIB)
			{
				std::vector<D3D12_ROOT_PARAMETER1> params;

				internal_state->bindpoint_rootconstant = (uint32_t)params.size();
				if (internal_state->rootconstants.Constants.Num32BitValues > 0)
				{
					auto& param = params.emplace_back();
					param = internal_state->rootconstants;
				}

				// Split resources into root descriptors and tables:
				{
					std::vector<D3D12_DESCRIPTOR_RANGE1> resources;
					std::vector<RESOURCEBINDING> bindings;
					int i = 0;
					for (auto& x : internal_state->resources)
					{
						RESOURCEBINDING binding = internal_state->resource_bindings[i++];
						if (x.NumDescriptors == 1 && x.RegisterSpace == 0 && binding == CONSTANTBUFFER && internal_state->root_cbvs.size() < CONSTANT_BUFFER_AUTO_PLACEMENT_IN_ROOT)
						{
							D3D12_ROOT_DESCRIPTOR1& descriptor = internal_state->root_cbvs.emplace_back();
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

				for (auto& x : internal_state->resources)
				{
					internal_state->resource_binding_count_unrolled += x.NumDescriptors;
				}
				for (auto& x : internal_state->samplers)
				{
					internal_state->sampler_binding_count_unrolled += x.NumDescriptors;
				}

				internal_state->bindpoint_rootdescriptor = (uint32_t)params.size();
				for (auto& x : internal_state->root_cbvs)
				{
					D3D12_ROOT_PARAMETER1& param = params.emplace_back();
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.Descriptor = x;
				}

				internal_state->bindpoint_res = (uint32_t)params.size();
				if (!internal_state->resources.empty())
				{
					D3D12_ROOT_PARAMETER1& param = params.emplace_back();
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->resources.size();
					param.DescriptorTable.pDescriptorRanges = internal_state->resources.data();
				}

				internal_state->bindpoint_sam = (uint32_t)params.size();
				if (!internal_state->samplers.empty())
				{
					D3D12_ROOT_PARAMETER1& param = params.emplace_back();
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->samplers.size();
					param.DescriptorTable.pDescriptorRanges = internal_state->samplers.data();
				}

				internal_state->bindpoint_bindless = (uint32_t)params.size();
				if(!internal_state->bindless_res.empty())
				{
					D3D12_ROOT_PARAMETER1& param = params.emplace_back();
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->bindless_res.size();
					param.DescriptorTable.pDescriptorRanges = internal_state->bindless_res.data();
				}
				if (!internal_state->bindless_sam.empty())
				{
					D3D12_ROOT_PARAMETER1& param = params.emplace_back();
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->bindless_sam.size();
					param.DescriptorTable.pDescriptorRanges = internal_state->bindless_sam.data();
				}

				size_t rootconstant_hash = 0;
				wiHelper::hash_combine(rootconstant_hash, internal_state->rootconstants.ShaderVisibility);
				wiHelper::hash_combine(rootconstant_hash, internal_state->rootconstants.ParameterType);
				wiHelper::hash_combine(rootconstant_hash, internal_state->rootconstants.Constants.Num32BitValues);
				wiHelper::hash_combine(rootconstant_hash, internal_state->rootconstants.Constants.RegisterSpace);
				wiHelper::hash_combine(rootconstant_hash, internal_state->rootconstants.Constants.ShaderRegister);

				size_t root_binding_hash = 0;
				for (auto& x : internal_state->root_cbvs)
				{
					wiHelper::hash_combine(root_binding_hash, x.Flags);
					wiHelper::hash_combine(root_binding_hash, x.ShaderRegister);
					wiHelper::hash_combine(root_binding_hash, x.RegisterSpace);
				}

				size_t resource_binding_hash = 0;
				for (auto& x : internal_state->resources)
				{
					wiHelper::hash_combine(resource_binding_hash, x.BaseShaderRegister);
					wiHelper::hash_combine(resource_binding_hash, x.NumDescriptors);
					wiHelper::hash_combine(resource_binding_hash, x.Flags);
					wiHelper::hash_combine(resource_binding_hash, x.OffsetInDescriptorsFromTableStart);
					wiHelper::hash_combine(resource_binding_hash, x.RangeType);
					wiHelper::hash_combine(resource_binding_hash, x.RegisterSpace);
				}

				size_t sampler_binding_hash = 0;
				for (auto& x : internal_state->samplers)
				{
					wiHelper::hash_combine(sampler_binding_hash, x.BaseShaderRegister);
					wiHelper::hash_combine(sampler_binding_hash, x.NumDescriptors);
					wiHelper::hash_combine(sampler_binding_hash, x.Flags);
					wiHelper::hash_combine(sampler_binding_hash, x.OffsetInDescriptorsFromTableStart);
					wiHelper::hash_combine(sampler_binding_hash, x.RangeType);
					wiHelper::hash_combine(sampler_binding_hash, x.RegisterSpace);
				}

				size_t bindless_hash = 0;
				for (auto& x : internal_state->bindless_res)
				{
					wiHelper::hash_combine(bindless_hash, x.BaseShaderRegister);
					wiHelper::hash_combine(bindless_hash, x.NumDescriptors);
					wiHelper::hash_combine(bindless_hash, x.Flags);
					wiHelper::hash_combine(bindless_hash, x.OffsetInDescriptorsFromTableStart);
					wiHelper::hash_combine(bindless_hash, x.RangeType);
					wiHelper::hash_combine(bindless_hash, x.RegisterSpace);
				}
				for (auto& x : internal_state->bindless_sam)
				{
					wiHelper::hash_combine(bindless_hash, x.BaseShaderRegister);
					wiHelper::hash_combine(bindless_hash, x.NumDescriptors);
					wiHelper::hash_combine(bindless_hash, x.Flags);
					wiHelper::hash_combine(bindless_hash, x.OffsetInDescriptorsFromTableStart);
					wiHelper::hash_combine(bindless_hash, x.RangeType);
					wiHelper::hash_combine(bindless_hash, x.RegisterSpace);
				}

				size_t rootsig_hash = 0;
				wiHelper::hash_combine(rootsig_hash, rootconstant_hash);
				wiHelper::hash_combine(rootsig_hash, root_binding_hash);
				wiHelper::hash_combine(rootsig_hash, resource_binding_hash);
				wiHelper::hash_combine(rootsig_hash, sampler_binding_hash);
				wiHelper::hash_combine(rootsig_hash, bindless_hash);
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
					if (SUCCEEDED(hr))
					{
						rootsignature_cache[rootsig_hash] = internal_state->rootSignature;
					}
				}
				rootsignature_cache_mutex.unlock();
			}
		}

		if (stage == ShaderStage::CS)
		{
			struct PSO_STREAM
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_CS CS;
			} stream;

			stream.pRootSignature = internal_state->rootSignature.Get();
			stream.CS = { internal_state->shadercode.data(), internal_state->shadercode.size() };

			D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
			streamDesc.pPipelineStateSubobjectStream = &stream;
			streamDesc.SizeInBytes = sizeof(stream);

			hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&internal_state->resource));
			assert(SUCCEEDED(hr));
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateSampler(const SamplerDesc* pSamplerDesc, Sampler* pSamplerState) const
	{
		auto internal_state = std::make_shared<Sampler_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pSamplerState->internal_state = internal_state;

		D3D12_SAMPLER_DESC desc;
		desc.Filter = _ConvertFilter(pSamplerDesc->filter);
		desc.AddressU = _ConvertTextureAddressMode(pSamplerDesc->address_u);
		desc.AddressV = _ConvertTextureAddressMode(pSamplerDesc->address_v);
		desc.AddressW = _ConvertTextureAddressMode(pSamplerDesc->address_w);
		desc.MipLODBias = pSamplerDesc->mip_lod_bias;
		desc.MaxAnisotropy = pSamplerDesc->max_anisotropy;
		desc.ComparisonFunc = _ConvertComparisonFunc(pSamplerDesc->comparison_func);
		switch (pSamplerDesc->border_color)
		{
		case SamplerBorderColor::OPAQUE_BLACK:
			desc.BorderColor[0] = 0.0f;
			desc.BorderColor[1] = 0.0f;
			desc.BorderColor[2] = 0.0f;
			desc.BorderColor[3] = 1.0f;
			break;

		case SamplerBorderColor::OPAQUE_WHITE:
			desc.BorderColor[0] = 1.0f;
			desc.BorderColor[1] = 1.0f;
			desc.BorderColor[2] = 1.0f;
			desc.BorderColor[3] = 1.0f;
			break;

		default:
			desc.BorderColor[0] = 0.0f;
			desc.BorderColor[1] = 0.0f;
			desc.BorderColor[2] = 0.0f;
			desc.BorderColor[3] = 0.0f;
			break;
		}
		desc.MinLOD = pSamplerDesc->min_lod;
		desc.MaxLOD = pSamplerDesc->max_lod;

		pSamplerState->desc = *pSamplerDesc;

		internal_state->descriptor.init(this, desc);

		return true;
	}
	bool GraphicsDevice_DX12::CreateQueryHeap(const GPUQueryHeapDesc* pDesc, GPUQueryHeap* pQueryHeap) const
	{
		auto internal_state = std::make_shared<QueryHeap_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pQueryHeap->internal_state = internal_state;

		pQueryHeap->desc = *pDesc;

		D3D12_QUERY_HEAP_DESC desc = {};
		desc.Count = pDesc->query_count;

		switch (pDesc->type)
		{
		default:
		case GpuQueryType::TIMESTAMP:
			desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			break;
		case GpuQueryType::OCCLUSION:
		case GpuQueryType::OCCLUSION_BINARY:
			desc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			break;
		}

		HRESULT hr = allocationhandler->device->CreateQueryHeap(&desc, IID_PPV_ARGS(&internal_state->heap));
		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso) const
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
		wiHelper::hash_combine(pso->hash, pDesc->sample_mask);

		HRESULT hr = S_OK;

		{
			// Root signature comes from reflection data when there is no root signature specified:

			auto insert_shader = [&](const Shader* shader)
			{
				if (shader == nullptr)
					return;

				auto shader_internal = to_internal(shader);

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

				if (shader_internal->rootconstants.Constants.Num32BitValues > 0)
				{
					internal_state->rootconstants = shader_internal->rootconstants;
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

			internal_state->bindpoint_rootconstant = (uint32_t)params.size();
			if (internal_state->rootconstants.Constants.Num32BitValues > 0)
			{
				auto& param = params.emplace_back();
				param = internal_state->rootconstants;
			}

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
						D3D12_ROOT_DESCRIPTOR1& descriptor = internal_state->root_cbvs.emplace_back();
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

			for (auto& x : internal_state->resources)
			{
				internal_state->resource_binding_count_unrolled += x.NumDescriptors;
			}
			for (auto& x : internal_state->samplers)
			{
				internal_state->sampler_binding_count_unrolled += x.NumDescriptors;
			}

			internal_state->bindpoint_rootdescriptor = (uint32_t)params.size();
			for (auto& x : internal_state->root_cbvs)
			{
				D3D12_ROOT_PARAMETER1& param = params.emplace_back();
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				param.Descriptor = x;
			}

			internal_state->bindpoint_res = (uint32_t)params.size();
			if (!internal_state->resources.empty())
			{
				D3D12_ROOT_PARAMETER1& param = params.emplace_back();
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->resources.size();
				param.DescriptorTable.pDescriptorRanges = internal_state->resources.data();
			}

			internal_state->bindpoint_sam = (uint32_t)params.size();
			if (!internal_state->samplers.empty())
			{
				D3D12_ROOT_PARAMETER1& param = params.emplace_back();
				param = {};
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->samplers.size();
				param.DescriptorTable.pDescriptorRanges = internal_state->samplers.data();
			}

			internal_state->bindpoint_bindless = (uint32_t)params.size();

			auto insert_shader_bindless = [&](const Shader* shader, D3D12_SHADER_VISIBILITY stage) {
				if (shader == nullptr)
					return;

				auto shader_internal = to_internal(shader);

				for (auto& x : shader_internal->bindless_res)
				{
					bool found = false;
					for (auto& y : internal_state->bindless_res)
					{
						if (x.RegisterSpace == y.RegisterSpace)
						{
							found = true;
							break;
						}
					}
					if (!found)
						internal_state->bindless_res.push_back(x);
				}

				for (auto& x : shader_internal->bindless_sam)
				{
					bool found = false;
					for (auto& y : internal_state->bindless_sam)
					{
						if (x.RegisterSpace == y.RegisterSpace)
						{
							found = true;
							break;
						}
					}
					if (!found)
						internal_state->bindless_sam.push_back(x);
				}
			};

			insert_shader_bindless(pDesc->ms, D3D12_SHADER_VISIBILITY_MESH);
			insert_shader_bindless(pDesc->as, D3D12_SHADER_VISIBILITY_AMPLIFICATION);
			insert_shader_bindless(pDesc->vs, D3D12_SHADER_VISIBILITY_VERTEX);
			insert_shader_bindless(pDesc->hs, D3D12_SHADER_VISIBILITY_HULL);
			insert_shader_bindless(pDesc->ds, D3D12_SHADER_VISIBILITY_DOMAIN);
			insert_shader_bindless(pDesc->gs, D3D12_SHADER_VISIBILITY_GEOMETRY);
			insert_shader_bindless(pDesc->ps, D3D12_SHADER_VISIBILITY_PIXEL);

			internal_state->bindpoint_bindless = (uint32_t)params.size();
			if (!internal_state->bindless_res.empty())
			{
				D3D12_ROOT_PARAMETER1& param = params.emplace_back();
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->bindless_res.size();
				param.DescriptorTable.pDescriptorRanges = internal_state->bindless_res.data();
			}
			if (!internal_state->bindless_sam.empty())
			{
				D3D12_ROOT_PARAMETER1& param = params.emplace_back();
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				param.DescriptorTable.NumDescriptorRanges = (UINT)internal_state->bindless_sam.size();
				param.DescriptorTable.pDescriptorRanges = internal_state->bindless_sam.data();
			}

			size_t rootconstant_hash = 0;
			wiHelper::hash_combine(rootconstant_hash, internal_state->rootconstants.ShaderVisibility);
			wiHelper::hash_combine(rootconstant_hash, internal_state->rootconstants.ParameterType);
			wiHelper::hash_combine(rootconstant_hash, internal_state->rootconstants.Constants.Num32BitValues);
			wiHelper::hash_combine(rootconstant_hash, internal_state->rootconstants.Constants.RegisterSpace);
			wiHelper::hash_combine(rootconstant_hash, internal_state->rootconstants.Constants.ShaderRegister);

			size_t root_binding_hash = 0;
			for (auto& x : internal_state->root_cbvs)
			{
				wiHelper::hash_combine(root_binding_hash, x.Flags);
				wiHelper::hash_combine(root_binding_hash, x.ShaderRegister);
				wiHelper::hash_combine(root_binding_hash, x.RegisterSpace);
			}

			size_t resource_binding_hash = 0;
			for (auto& x : internal_state->resources)
			{
				wiHelper::hash_combine(resource_binding_hash, x.BaseShaderRegister);
				wiHelper::hash_combine(resource_binding_hash, x.NumDescriptors);
				wiHelper::hash_combine(resource_binding_hash, x.Flags);
				wiHelper::hash_combine(resource_binding_hash, x.OffsetInDescriptorsFromTableStart);
				wiHelper::hash_combine(resource_binding_hash, x.RangeType);
				wiHelper::hash_combine(resource_binding_hash, x.RegisterSpace);
			}

			size_t sampler_binding_hash = 0;
			for (auto& x : internal_state->samplers)
			{
				wiHelper::hash_combine(sampler_binding_hash, x.BaseShaderRegister);
				wiHelper::hash_combine(sampler_binding_hash, x.NumDescriptors);
				wiHelper::hash_combine(sampler_binding_hash, x.Flags);
				wiHelper::hash_combine(sampler_binding_hash, x.OffsetInDescriptorsFromTableStart);
				wiHelper::hash_combine(sampler_binding_hash, x.RangeType);
				wiHelper::hash_combine(sampler_binding_hash, x.RegisterSpace);
			}

			size_t bindless_hash = 0;
			for (auto& x : internal_state->bindless_res)
			{
				wiHelper::hash_combine(bindless_hash, x.BaseShaderRegister);
				wiHelper::hash_combine(bindless_hash, x.NumDescriptors);
				wiHelper::hash_combine(bindless_hash, x.Flags);
				wiHelper::hash_combine(bindless_hash, x.OffsetInDescriptorsFromTableStart);
				wiHelper::hash_combine(bindless_hash, x.RangeType);
				wiHelper::hash_combine(bindless_hash, x.RegisterSpace);
			}
			for (auto& x : internal_state->bindless_sam)
			{
				wiHelper::hash_combine(bindless_hash, x.BaseShaderRegister);
				wiHelper::hash_combine(bindless_hash, x.NumDescriptors);
				wiHelper::hash_combine(bindless_hash, x.Flags);
				wiHelper::hash_combine(bindless_hash, x.OffsetInDescriptorsFromTableStart);
				wiHelper::hash_combine(bindless_hash, x.RangeType);
				wiHelper::hash_combine(bindless_hash, x.RegisterSpace);
			}

			size_t rootsig_hash = 0;
			wiHelper::hash_combine(rootsig_hash, pDesc->il);
			wiHelper::hash_combine(rootsig_hash, rootconstant_hash);
			wiHelper::hash_combine(rootsig_hash, root_binding_hash);
			wiHelper::hash_combine(rootsig_hash, resource_binding_hash);
			wiHelper::hash_combine(rootsig_hash, sampler_binding_hash);
			wiHelper::hash_combine(rootsig_hash, bindless_hash);
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
				if (pDesc->il != nullptr)
				{
					rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
				}

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
					rootsignature_cache_mutex.unlock();
					return false;
				}
				hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&internal_state->rootSignature));
				assert(SUCCEEDED(hr));
				if (SUCCEEDED(hr))
				{
					rootsignature_cache[rootsig_hash] = internal_state->rootSignature;
				}
			}
			rootsignature_cache_mutex.unlock();
		}

		auto& stream = internal_state->stream;
		if (pso->desc.vs != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.vs);
			stream.stream1.VS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
		}
		if (pso->desc.hs != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.hs);
			stream.stream1.HS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
		}
		if (pso->desc.ds != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.ds);
			stream.stream1.DS = { shader_internal->shadercode.data(),shader_internal->shadercode.size() };
		}
		if (pso->desc.gs != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.gs);
			stream.stream1.GS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
		}
		if (pso->desc.ps != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.ps);
			stream.stream1.PS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
		}

		if (pso->desc.ms != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.ms);
			stream.stream2.MS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
		}
		if (pso->desc.as != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.as);
			stream.stream2.AS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
		}

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
		CD3DX12_DEPTH_STENCIL_DESC dss = {};
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

		internal_state->primitiveTopology = _ConvertPrimitiveTopology(pDesc->pt, pDesc->patch_control_points);
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

		stream.stream1.pRootSignature = internal_state->rootSignature.Get();

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) const
	{
		auto internal_state = std::make_shared<RenderPass_DX12>();
		renderpass->internal_state = internal_state;

		renderpass->desc = *pDesc;

		if (has(renderpass->desc.flags, RenderPassDesc::Flags::ALLOW_UAV_WRITES))
		{
			internal_state->flags |= D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
		}

		renderpass->hash = 0;
		wiHelper::hash_combine(renderpass->hash, pDesc->attachments.size());
		int resolve_dst_counter = 0;
		for (auto& attachment : pDesc->attachments)
		{
			if (attachment.type == RenderPassAttachment::Type::RENDERTARGET || attachment.type == RenderPassAttachment::Type::DEPTH_STENCIL)
			{
				wiHelper::hash_combine(renderpass->hash, attachment.texture->desc.format);
				wiHelper::hash_combine(renderpass->hash, attachment.texture->desc.sample_count);
			}

			const Texture* texture = attachment.texture;
			int subresource = attachment.subresource;
			auto texture_internal = to_internal(texture);

			D3D12_CLEAR_VALUE clear_value;
			clear_value.Format = _ConvertFormat(texture->desc.format);

			if (attachment.type == RenderPassAttachment::Type::RENDERTARGET)
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
				case RenderPassAttachment::LoadOp::LOAD:
					internal_state->RTVs[internal_state->rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::LoadOp::CLEAR:
					internal_state->RTVs[internal_state->rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					clear_value.Color[0] = texture->desc.clear.color[0];
					clear_value.Color[1] = texture->desc.clear.color[1];
					clear_value.Color[2] = texture->desc.clear.color[2];
					clear_value.Color[3] = texture->desc.clear.color[3];
					internal_state->RTVs[internal_state->rt_count].BeginningAccess.Clear.ClearValue = clear_value;
					break;
				case RenderPassAttachment::LoadOp::DONTCARE:
					internal_state->RTVs[internal_state->rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					break;
				}

				switch (attachment.storeop)
				{
				default:
				case RenderPassAttachment::StoreOp::STORE:
					internal_state->RTVs[internal_state->rt_count].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::StoreOp::DONTCARE:
					internal_state->RTVs[internal_state->rt_count].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					break;
				}

				internal_state->rt_count++;
			}
			else if (attachment.type == RenderPassAttachment::Type::DEPTH_STENCIL)
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
				case RenderPassAttachment::LoadOp::LOAD:
					internal_state->DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					internal_state->DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::LoadOp::CLEAR:
					internal_state->DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					internal_state->DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					clear_value.DepthStencil.Depth = texture->desc.clear.depth_stencil.depth;
					clear_value.DepthStencil.Stencil = texture->desc.clear.depth_stencil.stencil;
					internal_state->DSV.DepthBeginningAccess.Clear.ClearValue = clear_value;
					internal_state->DSV.StencilBeginningAccess.Clear.ClearValue = clear_value;
					break;
				case RenderPassAttachment::LoadOp::DONTCARE:
					internal_state->DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					internal_state->DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					break;
				}

				switch (attachment.storeop)
				{
				default:
				case RenderPassAttachment::StoreOp::STORE:
					internal_state->DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					internal_state->DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::StoreOp::DONTCARE:
					internal_state->DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					internal_state->DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					break;
				}
			}
			else if (attachment.type == RenderPassAttachment::Type::RESOLVE)
			{
				if (texture != nullptr)
				{
					int resolve_src_counter = 0;
					for (auto& src : renderpass->desc.attachments)
					{
						if (src.type == RenderPassAttachment::Type::RENDERTARGET && src.texture != nullptr)
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
								internal_state->resolve_subresources[resolve_src_counter].SrcRect.right = (LONG)texture->desc.width;
								internal_state->resolve_subresources[resolve_src_counter].SrcRect.bottom = (LONG)texture->desc.height;
								internal_state->resolve_subresources[resolve_src_counter].SrcRect.top = 0;

								break;
							}
							resolve_src_counter++;
						}
					}
				}
				resolve_dst_counter++;
			}
			else if (attachment.type == RenderPassAttachment::Type::SHADING_RATE_SOURCE)
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
			barrierdesc.Transition.StateBefore = _ParseResourceState(attachment.initial_layout);
			if (attachment.type == RenderPassAttachment::Type::RESOLVE)
			{
				barrierdesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_DEST;
			}
			else
			{
				barrierdesc.Transition.StateAfter = _ParseResourceState(attachment.subpass_layout);
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
			if (attachment.type == RenderPassAttachment::Type::RESOLVE)
			{
				barrierdesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_DEST;
			}
			else
			{
				barrierdesc.Transition.StateBefore = _ParseResourceState(attachment.subpass_layout);
			}
			barrierdesc.Transition.StateAfter = _ParseResourceState(attachment.final_layout);
			barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			if (barrierdesc.Transition.StateBefore == barrierdesc.Transition.StateAfter)
			{
				internal_state->num_barriers_end--;
				continue;
			}
		}

		return true;
	}
	bool GraphicsDevice_DX12::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh) const
	{
		auto internal_state = std::make_shared<BVH_DX12>();
		internal_state->allocationhandler = allocationhandler;
		bvh->internal_state = internal_state;
		bvh->type = GPUResource::Type::RAYTRACING_ACCELERATION_STRUCTURE;

		bvh->desc = *pDesc;

		if (pDesc->flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		}
		if (pDesc->flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_COMPACTION)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
		}
		if (pDesc->flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		}
		if (pDesc->flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		}
		if (pDesc->flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
		}
		

		switch (pDesc->type)
		{
		case RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL:
		{
			internal_state->desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			internal_state->desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

			for (auto& x : pDesc->bottom_level.geometries)
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

			internal_state->desc.InstanceDescs = to_internal(&pDesc->top_level.instance_buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)pDesc->top_level.offset;
			internal_state->desc.NumDescs = (UINT)pDesc->top_level.count;
		}
		break;
		}

		device->GetRaytracingAccelerationStructurePrebuildInfo(&internal_state->desc, &internal_state->info);


		UINT64 alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
		UINT64 alignedSize = AlignTo(internal_state->info.ResultDataMaxSizeInBytes, alignment);

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
		scratch_desc.size = (uint32_t)std::max(internal_state->info.ScratchDataSizeInBytes, internal_state->info.UpdateScratchDataSizeInBytes);

		return CreateBuffer(&scratch_desc, nullptr, &internal_state->scratch);
	}
	bool GraphicsDevice_DX12::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso) const
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
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
			pipeline_config.MaxTraceRecursionDepth = pDesc->max_trace_recursion_depth;
			subobject.pDesc = &pipeline_config;
		}

		D3D12_RAYTRACING_SHADER_CONFIG shader_config = {};
		{
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
			shader_config.MaxAttributeSizeInBytes = pDesc->max_attribute_size_in_bytes;
			shader_config.MaxPayloadSizeInBytes = pDesc->max_payload_size_in_bytes;
			subobject.pDesc = &shader_config;
		}

		D3D12_GLOBAL_ROOT_SIGNATURE global_rootsig = {};
		{
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
			auto shader_internal = to_internal(pDesc->shader_libraries.front().shader); // think better way
			global_rootsig.pGlobalRootSignature = shader_internal->rootSignature.Get();
			subobject.pDesc = &global_rootsig;
		}

		internal_state->exports.reserve(pDesc->shader_libraries.size());
		internal_state->library_descs.reserve(pDesc->shader_libraries.size());
		for(auto& x : pDesc->shader_libraries)
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
			wiHelper::StringConvert(x.function_name, internal_state->export_strings.emplace_back());
			export_desc.Name = internal_state->export_strings.back().c_str();
			library_desc.pExports = &export_desc;

			subobject.pDesc = &library_desc;
		}

		internal_state->hitgroup_descs.reserve(pDesc->hit_groups.size());
		for (auto& x : pDesc->hit_groups)
		{
			wiHelper::StringConvert(x.name, internal_state->group_strings.emplace_back());

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

		desc.NumSubobjects = (UINT)subobjects.size();
		desc.pSubobjects = subobjects.data();

		HRESULT hr = device->CreateStateObject(&desc, IID_PPV_ARGS(&internal_state->resource));
		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}

	int GraphicsDevice_DX12::CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) const
	{
		auto internal_state = to_internal(texture);

		switch (type)
		{
		case SubresourceType::SRV:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			// Try to resolve resource format:
			switch (texture->desc.format)
			{
			case Format::R16_TYPELESS:
				srv_desc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			case Format::R32_TYPELESS:
				srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case Format::R24G8_TYPELESS:
				srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case Format::R32G8X24_TYPELESS:
				srv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			default:
				srv_desc.Format = _ConvertFormat(texture->desc.format);
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
					if (has(texture->desc.misc_flags, ResourceMiscFlag::TEXTURECUBE))
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

			// Try to resolve resource format:
			switch (texture->desc.format)
			{
			case Format::R16_TYPELESS:
				uav_desc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			case Format::R32_TYPELESS:
				uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case Format::R24G8_TYPELESS:
				uav_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case Format::R32G8X24_TYPELESS:
				uav_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			default:
				uav_desc.Format = _ConvertFormat(texture->desc.format);
				break;
			}

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

			// Try to resolve resource format:
			switch (texture->desc.format)
			{
			case Format::R16_TYPELESS:
				rtv_desc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			case Format::R32_TYPELESS:
				rtv_desc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case Format::R24G8_TYPELESS:
				rtv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case Format::R32G8X24_TYPELESS:
				rtv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			default:
				rtv_desc.Format = _ConvertFormat(texture->desc.format);
				break;
			}

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

			// Try to resolve resource format:
			switch (texture->desc.format)
			{
			case Format::R16_TYPELESS:
				dsv_desc.Format = DXGI_FORMAT_D16_UNORM;
				break;
			case Format::R32_TYPELESS:
				dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
				break;
			case Format::R24G8_TYPELESS:
				dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;
			case Format::R32G8X24_TYPELESS:
				dsv_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
				break;
			default:
				dsv_desc.Format = _ConvertFormat(texture->desc.format);
				break;
			}

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
	int GraphicsDevice_DX12::CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size) const
	{
		auto internal_state = to_internal(buffer);
		const GPUBufferDesc& desc = buffer->GetDesc();

		switch (type)
		{

		case SubresourceType::SRV:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			if (has(desc.misc_flags, ResourceMiscFlag::BUFFER_RAW))
			{
				// This is a Raw Buffer
				srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
				srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
				srv_desc.Buffer.NumElements = (UINT)std::min(size, desc.size - offset) / sizeof(uint32_t);
			}
			else if (has(desc.misc_flags, ResourceMiscFlag::BUFFER_STRUCTURED))
			{
				// This is a Structured Buffer
				srv_desc.Format = DXGI_FORMAT_UNKNOWN;
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = (UINT)offset / desc.stride;
				srv_desc.Buffer.NumElements = (UINT)std::min(size, desc.size - offset) / desc.stride;
				srv_desc.Buffer.StructureByteStride = desc.stride;
			}
			else
			{
				// This is a Typed Buffer
				uint32_t stride = GetFormatStride(desc.format);
				srv_desc.Format = _ConvertFormat(desc.format);
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = offset / stride;
				srv_desc.Buffer.NumElements = (UINT)std::min(size, desc.size - offset) / stride;
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

			if (has(desc.misc_flags, ResourceMiscFlag::BUFFER_RAW))
			{
				// This is a Raw Buffer
				uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
				uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
				uav_desc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
				uav_desc.Buffer.NumElements = (UINT)std::min(size, desc.size - offset) / sizeof(uint32_t);
			}
			else if (has(desc.misc_flags, ResourceMiscFlag::BUFFER_STRUCTURED))
			{
				// This is a Structured Buffer
				uav_desc.Format = DXGI_FORMAT_UNKNOWN;
				uav_desc.Buffer.FirstElement = (UINT)offset / desc.stride;
				uav_desc.Buffer.NumElements = (UINT)std::min(size, desc.size - offset) / desc.stride;
				uav_desc.Buffer.StructureByteStride = desc.stride;
			}
			else
			{
				// This is a Typed Buffer
				uint32_t stride = GetFormatStride(desc.format);
				uav_desc.Format = _ConvertFormat(desc.format);
				uav_desc.Buffer.FirstElement = (UINT)offset / stride;
				uav_desc.Buffer.NumElements = (UINT)std::min(size, desc.size - offset) / stride;
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
		D3D12_RAYTRACING_INSTANCE_DESC* desc = (D3D12_RAYTRACING_INSTANCE_DESC*)dest;
		desc->AccelerationStructure = to_internal(&instance->bottom_level)->gpu_address;
		memcpy(desc->Transform, &instance->transform, sizeof(desc->Transform));
		desc->InstanceID = instance->instance_id;
		desc->InstanceMask = instance->instance_mask;
		desc->InstanceContributionToHitGroupIndex = instance->instance_contribution_to_hit_group_index;
		desc->Flags = instance->flags;
	}
	void GraphicsDevice_DX12::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const
	{
		auto internal_state = to_internal(rtpso);

		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		HRESULT hr = internal_state->resource.As(&stateObjectProperties);
		assert(SUCCEEDED(hr));

		void* identifier = stateObjectProperties->GetShaderIdentifier(internal_state->group_strings[group_index].c_str());
		memcpy(dest, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
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

	CommandList GraphicsDevice_DX12::BeginCommandList(QUEUE_TYPE queue)
	{
		HRESULT hr;

		CommandList cmd{ cmd_count.fetch_add(1) };
		assert(cmd < COMMANDLIST_COUNT);
		cmd_meta[cmd].queue = queue;
		cmd_meta[cmd].waits.clear();

		if (GetCommandList(cmd) == nullptr)
		{
			// need to create one more command list:

			for (uint32_t fr = 0; fr < BUFFERCOUNT; ++fr)
			{
				hr = device->CreateCommandAllocator(queues[queue].desc.Type, IID_PPV_ARGS(&frames[fr].commandAllocators[cmd][queue]));
				assert(SUCCEEDED(hr));

			}

			hr = device->CreateCommandList1(0, queues[queue].desc.Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandLists[cmd][queue]));
			assert(SUCCEEDED(hr));

			std::wstringstream wss;
			wss << "cmd" << cmd;
			commandLists[cmd][queue]->SetName(wss.str().c_str());

			binders[cmd].init(this);
		}

		// Start the command list in a default state:
		hr = GetFrameResources().commandAllocators[cmd][queue]->Reset();
		assert(SUCCEEDED(hr));
		hr = GetCommandList(cmd)->Reset(GetFrameResources().commandAllocators[cmd][queue].Get(), nullptr);
		assert(SUCCEEDED(hr));

		ID3D12DescriptorHeap* heaps[2] = {
			descriptorheap_res.heap_GPU.Get(),
			descriptorheap_sam.heap_GPU.Get()
		};
		GetCommandList(cmd)->SetDescriptorHeaps(arraysize(heaps), heaps);

		binders[cmd].reset();

		if (queue == QUEUE_GRAPHICS)
		{
			D3D12_RECT pRects[D3D12_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1];
			for (uint32_t i = 0; i < arraysize(pRects); ++i)
			{
				pRects[i].bottom = D3D12_VIEWPORT_BOUNDS_MAX;
				pRects[i].left = D3D12_VIEWPORT_BOUNDS_MIN;
				pRects[i].right = D3D12_VIEWPORT_BOUNDS_MAX;
				pRects[i].top = D3D12_VIEWPORT_BOUNDS_MIN;
			}
			GetCommandList(cmd)->RSSetScissorRects(arraysize(pRects), pRects);
		}

		prev_pt[cmd] = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		prev_pipeline_hash[cmd] = 0;
		active_pso[cmd] = nullptr;
		active_cs[cmd] = nullptr;
		active_rt[cmd] = nullptr;
		active_rootsig_graphics[cmd] = nullptr;
		active_rootsig_compute[cmd] = nullptr;
		active_renderpass[cmd] = nullptr;
		prev_shadingrate[cmd] = ShadingRate::RATE_INVALID;
		dirty_pso[cmd] = false;
		pushconstants[cmd] = {};
		swapchains[cmd].clear();
		active_backbuffer[cmd] = nullptr;

		return cmd;
	}
	void GraphicsDevice_DX12::SubmitCommandLists()
	{
		HRESULT hr;

		// Submit current frame:
		{
			auto& frame = GetFrameResources();

			QUEUE_TYPE submit_queue = QUEUE_COUNT;

			CommandList::index_type cmd_last = cmd_count.load();
			cmd_count.store(0);
			for (CommandList::index_type cmd = 0; cmd < cmd_last; ++cmd)
			{
				hr = GetCommandList(cmd)->Close();
				assert(SUCCEEDED(hr));

				const CommandListMetadata& meta = cmd_meta[cmd];
				if (submit_queue == QUEUE_COUNT)
				{
					submit_queue = meta.queue;
				}
				if (meta.queue != submit_queue || !meta.waits.empty()) // new queue type or wait breaks submit batch
				{
					// submit previous cmd batch:
					if (queues[submit_queue].submit_count > 0)
					{
						queues[submit_queue].queue->ExecuteCommandLists(
							queues[submit_queue].submit_count,
							queues[submit_queue].submit_cmds
						);
						queues[submit_queue].submit_count = 0;
					}

					// signal status in case any future waits needed:
					hr = queues[submit_queue].queue->Signal(
						queues[submit_queue].fence.Get(),
						FRAMECOUNT * COMMANDLIST_COUNT + (uint64_t)cmd
					);
					assert(SUCCEEDED(hr));

					submit_queue = meta.queue;

					for (auto& wait : meta.waits)
					{
						// record wait for signal on a previous submit:
						const CommandListMetadata& wait_meta = cmd_meta[wait];
						hr = queues[submit_queue].queue->Wait(
							queues[wait_meta.queue].fence.Get(),
							FRAMECOUNT * COMMANDLIST_COUNT + (uint64_t)wait
						);
						assert(SUCCEEDED(hr));
					}
				}

				assert(submit_queue < QUEUE_COUNT);
				queues[submit_queue].submit_cmds[queues[submit_queue].submit_count++] = GetCommandList(cmd);

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

			// submit last cmd batch:
			assert(submit_queue < QUEUE_COUNT);
			assert(queues[submit_queue].submit_count > 0);
			queues[submit_queue].queue->ExecuteCommandLists(
				queues[submit_queue].submit_count,
				queues[submit_queue].submit_cmds
			);
			queues[submit_queue].submit_count = 0;

			// Mark the completion of queues for this frame:
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				hr = queues[queue].queue->Signal(frame.fence[queue].Get(), 1);
				assert(SUCCEEDED(hr));
			}

			for (CommandList::index_type cmd = 0; cmd < cmd_last; ++cmd)
			{
				for (auto& swapchain : swapchains[cmd])
				{
					UINT presentFlags = 0;
					if (!swapchain->desc.vsync && !swapchain->desc.fullscreen)
					{
						presentFlags = DXGI_PRESENT_ALLOW_TEARING;
					}

					hr = to_internal(swapchain)->swapChain->Present(swapchain->desc.vsync, presentFlags);

					// If the device was reset we must completely reinitialize the renderer.
					if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
					{
#ifdef _DEBUG
						char buff[64] = {};
						sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
							static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? device->GetDeviceRemovedReason() : hr));
						OutputDebugStringA(buff);
#endif

						// TODO: Handle device lost
						// HandleDeviceLost();
					}
				}
			}
		}

		// From here, we begin a new frame, this affects GetFrameResources()!
		FRAMECOUNT++;

		// Begin next frame:
		{
			auto& frame = GetFrameResources();

			// Initiate stalling CPU when GPU is not yet finished with next frame:
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				if (FRAMECOUNT >= BUFFERCOUNT && frame.fence[queue]->GetCompletedValue() < 1)
				{
					// NULL event handle will simply wait immediately:
					//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
					hr = frame.fence[queue]->SetEventOnCompletion(1, NULL);
					assert(SUCCEEDED(hr));
				}
				hr = frame.fence[queue]->Signal(0);
			}
			assert(SUCCEEDED(hr));

			// Descriptor heaps' progress is recorded by the GPU:
			descriptorheap_res.fenceValue = descriptorheap_res.allocationOffset.load();
			hr = queues[QUEUE_GRAPHICS].queue->Signal(descriptorheap_res.fence.Get(), descriptorheap_res.fenceValue);
			assert(SUCCEEDED(hr));
			descriptorheap_res.cached_completedValue = descriptorheap_res.fence->GetCompletedValue();
			descriptorheap_sam.fenceValue = descriptorheap_sam.allocationOffset.load();
			hr = queues[QUEUE_GRAPHICS].queue->Signal(descriptorheap_sam.fence.Get(), descriptorheap_sam.fenceValue);
			assert(SUCCEEDED(hr));
			descriptorheap_sam.cached_completedValue = descriptorheap_sam.fence->GetCompletedValue();

			allocationhandler->Update(FRAMECOUNT, BUFFERCOUNT);
		}
	}

	void GraphicsDevice_DX12::WaitForGPU() const
	{
		ComPtr<ID3D12Fence> fence;
		HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
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

	Texture GraphicsDevice_DX12::GetBackBuffer(const SwapChain* swapchain) const
	{
		auto swapchain_internal = to_internal(swapchain);

		auto internal_state = std::make_shared<Texture_DX12>();
		internal_state->allocationhandler = allocationhandler;
		internal_state->resource = swapchain_internal->backBuffers[swapchain_internal->swapChain->GetCurrentBackBufferIndex()];

		D3D12_RESOURCE_DESC desc = internal_state->resource->GetDesc();
		device->GetCopyableFootprints(&desc, 0, 1, 0, &internal_state->footprint, nullptr, nullptr, nullptr);

		Texture result;
		result.type = GPUResource::Type::TEXTURE;
		result.internal_state = internal_state;
		result.desc = _ConvertTextureDesc_Inv(desc);
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
		return false;
	}

	void GraphicsDevice_DX12::WaitCommandList(CommandList cmd, CommandList wait_for)
	{
		assert(wait_for < cmd); // command list cannot wait for future command list!
		cmd_meta[cmd].waits.push_back(wait_for);
	}
	void GraphicsDevice_DX12::RenderPassBegin(const SwapChain* swapchain, CommandList cmd)
	{
		swapchains[cmd].push_back(swapchain);
		auto internal_state = to_internal(swapchain);
		active_renderpass[cmd] = &internal_state->renderpass;
		active_backbuffer[cmd] = internal_state->backBuffers[internal_state->swapChain->GetCurrentBackBufferIndex()];

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = internal_state->backBuffers[internal_state->swapChain->GetCurrentBackBufferIndex()].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		GetCommandList(cmd)->ResourceBarrier(1, &barrier);
		
		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTV = {};
		RTV.cpuDescriptor = internal_state->backbufferRTV[internal_state->swapChain->GetCurrentBackBufferIndex()];
		RTV.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		RTV.BeginningAccess.Clear.ClearValue.Color[0] = swapchain->desc.clear_color[0];
		RTV.BeginningAccess.Clear.ClearValue.Color[1] = swapchain->desc.clear_color[1];
		RTV.BeginningAccess.Clear.ClearValue.Color[2] = swapchain->desc.clear_color[2];
		RTV.BeginningAccess.Clear.ClearValue.Color[3] = swapchain->desc.clear_color[3];
		RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		GetCommandList(cmd)->BeginRenderPass(1, &RTV, nullptr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);

	}
	void GraphicsDevice_DX12::RenderPassBegin(const RenderPass* renderpass, CommandList cmd)
	{
		active_renderpass[cmd] = renderpass;

		auto internal_state = to_internal(active_renderpass[cmd]);

		GetCommandList(cmd)->ResourceBarrier(internal_state->num_barriers_begin, internal_state->barrierdescs_begin);

		if (internal_state->shading_rate_image != nullptr)
		{
			GetCommandList(cmd)->RSSetShadingRateImage(to_internal(internal_state->shading_rate_image)->resource.Get());
		}

		GetCommandList(cmd)->BeginRenderPass(
			internal_state->rt_count,
			internal_state->RTVs,
			internal_state->DSV.cpuDescriptor.ptr == 0 ? nullptr : &internal_state->DSV,
			internal_state->flags
		);

	}
	void GraphicsDevice_DX12::RenderPassEnd(CommandList cmd)
	{
		GetCommandList(cmd)->EndRenderPass();

		auto internal_state = to_internal(active_renderpass[cmd]);

		if (internal_state != nullptr)
		{
			if (internal_state->shading_rate_image != nullptr)
			{
				GetCommandList(cmd)->RSSetShadingRateImage(nullptr);
			}

			GetCommandList(cmd)->ResourceBarrier(internal_state->num_barriers_end, internal_state->barrierdescs_end);
		}

		active_renderpass[cmd] = nullptr;

		if (active_backbuffer[cmd])
		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = active_backbuffer[cmd].Get();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			GetCommandList(cmd)->ResourceBarrier(1, &barrier);

			active_backbuffer[cmd] = nullptr;
		}
	}
	void GraphicsDevice_DX12::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd)
	{
		assert(rects != nullptr);
		D3D12_RECT pRects[D3D12_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1];
		assert(numRects < arraysize(pRects));
		for (uint32_t i = 0; i < numRects; ++i)
		{
			pRects[i].bottom = (LONG)rects[i].bottom;
			pRects[i].left = (LONG)rects[i].left;
			pRects[i].right = (LONG)rects[i].right;
			pRects[i].top = (LONG)rects[i].top;
		}
		GetCommandList(cmd)->RSSetScissorRects(numRects, pRects);
	}
	void GraphicsDevice_DX12::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		assert(pViewports != nullptr);
		D3D12_VIEWPORT d3dViewPorts[D3D12_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1];
		assert(NumViewports < arraysize(d3dViewPorts));
		for (uint32_t i = 0; i < NumViewports; ++i)
		{
			d3dViewPorts[i].TopLeftX = pViewports[i].top_left_x;
			d3dViewPorts[i].TopLeftY = pViewports[i].top_left_y;
			d3dViewPorts[i].Width = pViewports[i].width;
			d3dViewPorts[i].Height = pViewports[i].height;
			d3dViewPorts[i].MinDepth = pViewports[i].min_depth;
			d3dViewPorts[i].MaxDepth = pViewports[i].max_depth;
		}
		GetCommandList(cmd)->RSSetViewports(NumViewports, d3dViewPorts);
	}
	void GraphicsDevice_DX12::BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < DESCRIPTORBINDER_SRV_COUNT);
		auto& binder = binders[cmd];
		if (binder.table.SRV[slot].internal_state != resource->internal_state || binder.table.SRV_index[slot] != subresource)
		{
			binder.table.SRV[slot] = *resource;
			binder.table.SRV_index[slot] = subresource;
			binder.dirty_res = true;
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
		auto& binder = binders[cmd];
		if (binder.table.UAV[slot].internal_state != resource->internal_state || binder.table.UAV_index[slot] != subresource)
		{
			binder.table.UAV[slot] = *resource;
			binder.table.UAV_index[slot] = subresource;
			binder.dirty_res = true;
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
		auto& binder = binders[cmd];
		if (binder.table.SAM[slot].internal_state != sampler->internal_state)
		{
			binder.table.SAM[slot] = *sampler;
			binder.dirty_sam = true;
		}
	}
	void GraphicsDevice_DX12::BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset)
	{
		assert(slot < DESCRIPTORBINDER_CBV_COUNT);
		auto& binder = binders[cmd];
		if (binder.table.CBV[slot].internal_state != buffer->internal_state || binder.table.CBV_offset[slot] != offset)
		{
			binder.table.CBV[slot] = *buffer;
			binder.table.CBV_offset[slot] = offset;
			binder.dirty_res = true;

			// Root constant buffer root signature state tracking:
			auto internal_state = to_internal(buffer);
			if (internal_state->cbv_mask_frame[cmd] != FRAMECOUNT)
			{
				// This is the first binding as constant buffer in this frame for this resource,
				//	so clear the cbv flags completely
				internal_state->cbv_mask[cmd] = 0;
				internal_state->cbv_mask_frame[cmd] = FRAMECOUNT;
			}

			// CBV flag marked as bound for this slot:
			//	Also, the corresponding slot is marked dirty
			internal_state->cbv_mask[cmd] |= 1 << slot;
			binder.dirty_root_cbvs |= 1 << slot;
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
		GetCommandList(cmd)->IASetVertexBuffers(slot, count, res);
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
		GetCommandList(cmd)->IASetIndexBuffer(&res);
	}
	void GraphicsDevice_DX12::BindStencilRef(uint32_t value, CommandList cmd)
	{
		GetCommandList(cmd)->OMSetStencilRef(value);
	}
	void GraphicsDevice_DX12::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		const float blendFactor[4] = { r, g, b, a };
		GetCommandList(cmd)->OMSetBlendFactor(blendFactor);
	}
	void GraphicsDevice_DX12::BindShadingRate(ShadingRate rate, CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING) && prev_shadingrate[cmd] != rate)
		{
			prev_shadingrate[cmd] = rate;

			D3D12_SHADING_RATE _rate = D3D12_SHADING_RATE_1X1;
			WriteShadingRateValue(rate, &_rate);

			D3D12_SHADING_RATE_COMBINER combiners[] =
			{
				D3D12_SHADING_RATE_COMBINER_MAX,
				D3D12_SHADING_RATE_COMBINER_MAX,
			};
			GetCommandList(cmd)->RSSetShadingRate(_rate, combiners);
		}
	}
	void GraphicsDevice_DX12::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		active_cs[cmd] = nullptr;
		active_rt[cmd] = nullptr;

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
			GetCommandList(cmd)->SetGraphicsRootSignature(internal_state->rootSignature.Get());

			// Invalidate graphics root bindings:
			binders[cmd].dirty_res = true;
			binders[cmd].dirty_sam = true;
			binders[cmd].dirty_root_cbvs = ~0;

			// Set the bindless tables:
			uint32_t bindpoint = internal_state->bindpoint_bindless;
			if (!internal_state->bindless_res.empty())
			{
				GetCommandList(cmd)->SetGraphicsRootDescriptorTable(bindpoint++, descriptorheap_res.start_gpu);
			}
			if (!internal_state->bindless_sam.empty())
			{
				GetCommandList(cmd)->SetGraphicsRootDescriptorTable(bindpoint++, descriptorheap_sam.start_gpu);
			}
		}

		active_pso[cmd] = pso;
		dirty_pso[cmd] = true;
	}
	void GraphicsDevice_DX12::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		active_pso[cmd] = nullptr;
		active_rt[cmd] = nullptr;

		assert(cs->stage == ShaderStage::CS || cs->stage == ShaderStage::LIB);
		if (active_cs[cmd] != cs)
		{
			prev_pipeline_hash[cmd] = 0;

			active_cs[cmd] = cs;

			auto internal_state = to_internal(cs);

			if (cs->stage == ShaderStage::CS)
			{
				GetCommandList(cmd)->SetPipelineState(internal_state->resource.Get());
			}

			if (active_rootsig_compute[cmd] != internal_state->rootSignature.Get())
			{
				active_rootsig_compute[cmd] = internal_state->rootSignature.Get();
				GetCommandList(cmd)->SetComputeRootSignature(internal_state->rootSignature.Get());

				// Invalidate compute root bindings:
				binders[cmd].dirty_res = true;
				binders[cmd].dirty_sam = true;
				binders[cmd].dirty_root_cbvs = ~0;

				// Set the bindless tables:
				uint32_t bindpoint = internal_state->bindpoint_bindless;
				if (!internal_state->bindless_res.empty())
				{
					GetCommandList(cmd)->SetComputeRootDescriptorTable(bindpoint++, descriptorheap_res.start_gpu);
				}
				if (!internal_state->bindless_sam.empty())
				{
					GetCommandList(cmd)->SetComputeRootDescriptorTable(bindpoint++, descriptorheap_sam.start_gpu);
				}
			}

		}
	}
	void GraphicsDevice_DX12::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		GetCommandList(cmd)->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		GetCommandList(cmd)->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		GetCommandList(cmd)->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		GetCommandList(cmd)->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		GetCommandList(cmd)->ExecuteIndirect(drawInstancedIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		GetCommandList(cmd)->ExecuteIndirect(drawIndexedInstancedIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predispatch(cmd);
		GetCommandList(cmd)->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_DX12::DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		auto internal_state = to_internal(args);
		GetCommandList(cmd)->ExecuteIndirect(dispatchIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predraw(cmd);
		GetCommandList(cmd)->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_DX12::DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		GetCommandList(cmd)->ExecuteIndirect(dispatchMeshIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);
		D3D12_RESOURCE_DESC desc_src = internal_state_src->resource->GetDesc();
		D3D12_RESOURCE_DESC desc_dst = internal_state_dst->resource->GetDesc();
		if (desc_dst.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && desc_src.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			CD3DX12_TEXTURE_COPY_LOCATION Src(internal_state_src->resource.Get(), 0);
			CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state_dst->resource.Get(), internal_state_dst->footprint);
			GetCommandList(cmd)->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
		else if (desc_src.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && desc_dst.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			CD3DX12_TEXTURE_COPY_LOCATION Src(internal_state_src->resource.Get(), internal_state_src->footprint);
			CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state_dst->resource.Get(), 0);
			GetCommandList(cmd)->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
		else
		{
			GetCommandList(cmd)->CopyResource(internal_state_dst->resource.Get(), internal_state_src->resource.Get());
		}
	}
	void GraphicsDevice_DX12::CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd)
	{
		auto src_internal = to_internal((const GPUBuffer*)pSrc);
		auto dst_internal = to_internal((const GPUBuffer*)pDst);

		GetCommandList(cmd)->CopyBufferRegion(dst_internal->resource.Get(), dst_offset, src_internal->resource.Get(), src_offset, size);
	}
	void GraphicsDevice_DX12::QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType::TIMESTAMP:
			GetCommandList(cmd)->BeginQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				index
			);
			break;
		case GpuQueryType::OCCLUSION_BINARY:
			GetCommandList(cmd)->BeginQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_BINARY_OCCLUSION,
				index
			);
			break;
		case GpuQueryType::OCCLUSION:
			GetCommandList(cmd)->BeginQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_OCCLUSION,
				index
			);
			break;
		}
	}
	void GraphicsDevice_DX12::QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType::TIMESTAMP:
			GetCommandList(cmd)->EndQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				index
			);
			break;
		case GpuQueryType::OCCLUSION_BINARY:
			GetCommandList(cmd)->EndQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_BINARY_OCCLUSION,
				index
			);
			break;
		case GpuQueryType::OCCLUSION:
			GetCommandList(cmd)->EndQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_OCCLUSION,
				index
			);
			break;
		}
	}
	void GraphicsDevice_DX12::QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd)
	{
		assert(active_renderpass[cmd] == nullptr); // Can't resolve inside renderpass!

		auto internal_state = to_internal(heap);
		auto dst_internal = to_internal(dest);

		switch (heap->desc.type)
		{
		case GpuQueryType::TIMESTAMP:
			GetCommandList(cmd)->ResolveQueryData(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				index,
				count,
				dst_internal->resource.Get(),
				dest_offset
			);
			break;
		case GpuQueryType::OCCLUSION_BINARY:
			GetCommandList(cmd)->ResolveQueryData(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_BINARY_OCCLUSION,
				index,
				count,
				dst_internal->resource.Get(),
				dest_offset
			);
			break;
		case GpuQueryType::OCCLUSION:
			GetCommandList(cmd)->ResolveQueryData(
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
		assert(active_renderpass[cmd] == nullptr);

		auto& barrierdescs = frame_barriers[cmd];

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
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Transition.pResource = to_internal(barrier.image.texture)->resource.Get();
				barrierdesc.Transition.StateBefore = _ParseResourceState(barrier.image.layout_before);
				barrierdesc.Transition.StateAfter = _ParseResourceState(barrier.image.layout_after);
				if (barrier.image.mip >= 0 || barrier.image.slice >= 0)
				{
					barrierdesc.Transition.Subresource = D3D12CalcSubresource(
						(UINT)std::max(0, barrier.image.mip),
						(UINT)std::max(0, barrier.image.slice),
						0,
						barrier.image.texture->desc.mip_levels,
						barrier.image.texture->desc.array_size
					);
				}
				else
				{
					barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				}
			}
			break;
			case GPUBarrier::Type::BUFFER:
			{
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Transition.pResource = to_internal(barrier.buffer.buffer)->resource.Get();
				barrierdesc.Transition.StateBefore = _ParseResourceState(barrier.buffer.state_before);
				barrierdesc.Transition.StateAfter = _ParseResourceState(barrier.buffer.state_after);
				barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			}
			break;
			}

			if (barrierdesc.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION &&
				cmd_meta[cmd].queue > QUEUE_GRAPHICS)
			{
				// Only graphics queue can do pixel shader state:
				barrierdesc.Transition.StateBefore &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				barrierdesc.Transition.StateAfter &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}

			barrierdescs.push_back(barrierdesc);
		}

		if (!barrierdescs.empty())
		{
			GetCommandList(cmd)->ResourceBarrier(
				(UINT)barrierdescs.size(),
				barrierdescs.data()
			);
			barrierdescs.clear();
		}
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
		case RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL:
		{
			size_t i = 0;
			for (auto& x : dst->desc.bottom_level.geometries)
			{
				auto& geometry = geometries[i++];
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
		GetCommandList(cmd)->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
	}
	void GraphicsDevice_DX12::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		active_cs[cmd] = nullptr;
		active_pso[cmd] = nullptr;
		prev_pipeline_hash[cmd] = 0;
		active_rt[cmd] = rtpso;

		BindComputeShader(rtpso->desc.shader_libraries.front().shader, cmd);

		auto internal_state = to_internal(rtpso);
		GetCommandList(cmd)->SetPipelineState1(internal_state->resource.Get());
	}
	void GraphicsDevice_DX12::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
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

		GetCommandList(cmd)->DispatchRays(&dispatchrays_desc);
	}
	void GraphicsDevice_DX12::PushConstants(const void* data, uint32_t size, CommandList cmd)
	{
		std::memcpy(pushconstants[cmd].data, data, size);
		pushconstants[cmd].size = size;
	}
	void GraphicsDevice_DX12::PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd)
	{
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
		GetCommandList(cmd)->SetPredication(internal_state->resource.Get(), offset, operation);
	}
	void GraphicsDevice_DX12::PredicationEnd(CommandList cmd)
	{
		GetCommandList(cmd)->SetPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);
	}

	void GraphicsDevice_DX12::EventBegin(const char* name, CommandList cmd)
	{
		wchar_t text[128];
		if (wiHelper::StringConvert(name, text) > 0)
		{
			PIXBeginEvent(GetCommandList(cmd), 0xFF000000, text);
		}
	}
	void GraphicsDevice_DX12::EventEnd(CommandList cmd)
	{
		PIXEndEvent(GetCommandList(cmd));
	}
	void GraphicsDevice_DX12::SetMarker(const char* name, CommandList cmd)
	{
		wchar_t text[128];
		if (wiHelper::StringConvert(name, text) > 0)
		{
			PIXSetMarker(GetCommandList(cmd), 0xFFFF0000, text);
		}
	}


}

#endif // WICKEDENGINE_BUILD_DX12
