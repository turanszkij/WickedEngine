#ifdef __APPLE__
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define IR_PRIVATE_IMPLEMENTATION
#include "wiGraphicsDevice_Metal.h"
#include "wiTimer.h"
#include "wiBacklog.h"

namespace wi::graphics
{

namespace metal_internal
{
	static constexpr uint64_t bindless_resource_capacity = 500000;
	static constexpr uint64_t bindless_sampler_capacity = 256;

	constexpr MTL::AttributeFormat _ConvertAttributeFormat(Format value)
	{
		switch (value) {
			case Format::R32G32B32A32_FLOAT:
				return MTL::AttributeFormatFloat4;
			case Format::R32G32B32_FLOAT:
				return MTL::AttributeFormatFloat3;
			case Format::R32G32_FLOAT:
				return MTL::AttributeFormatFloat2;
			case Format::R32_FLOAT:
				return MTL::AttributeFormatFloat;
				
			case Format::R16G16B16A16_FLOAT:
				return MTL::AttributeFormatHalf4;
			case Format::R16G16_FLOAT:
				return MTL::AttributeFormatHalf2;
			case Format::R16_FLOAT:
				return MTL::AttributeFormatHalf;
				
			case Format::R32G32B32A32_UINT:
				return MTL::AttributeFormatUInt4;
			case Format::R32G32B32_UINT:
				return MTL::AttributeFormatUInt3;
			case Format::R32G32_UINT:
				return MTL::AttributeFormatUInt2;
			case Format::R32_UINT:
				return MTL::AttributeFormatUInt;
				
			case Format::R32G32B32A32_SINT:
				return MTL::AttributeFormatInt4;
			case Format::R32G32B32_SINT:
				return MTL::AttributeFormatInt3;
			case Format::R32G32_SINT:
				return MTL::AttributeFormatInt2;
			case Format::R32_SINT:
				return MTL::AttributeFormatInt;
				
			case Format::R16G16B16A16_UINT:
				return MTL::AttributeFormatUShort4;
			case Format::R16G16_UINT:
				return MTL::AttributeFormatUShort2;
			case Format::R16_UINT:
				return MTL::AttributeFormatUShort;
				
			case Format::R16G16B16A16_SINT:
				return MTL::AttributeFormatShort4;
			case Format::R16G16_SINT:
				return MTL::AttributeFormatShort2;
			case Format::R16_SINT:
				return MTL::AttributeFormatShort;
				
			case Format::R16G16B16A16_UNORM:
				return MTL::AttributeFormatUShort4Normalized;
			case Format::R16G16_UNORM:
				return MTL::AttributeFormatUShort2Normalized;
			case Format::R16_UNORM:
				return MTL::AttributeFormatUShortNormalized;
				
			case Format::R16G16B16A16_SNORM:
				return MTL::AttributeFormatShort4Normalized;
			case Format::R16G16_SNORM:
				return MTL::AttributeFormatShort2Normalized;
			case Format::R16_SNORM:
				return MTL::AttributeFormatShortNormalized;
				
			case Format::R8G8B8A8_UINT:
				return MTL::AttributeFormatUChar4;
			case Format::R8G8_UINT:
				return MTL::AttributeFormatUChar2;
			case Format::R8_UINT:
				return MTL::AttributeFormatUChar;
				
			case Format::B8G8R8A8_UNORM:
				return MTL::AttributeFormatUChar4Normalized_BGRA;
			case Format::R8G8B8A8_UNORM:
				return MTL::AttributeFormatUChar4Normalized;
			case Format::R8G8_UNORM:
				return MTL::AttributeFormatUChar2Normalized;
			case Format::R8_UNORM:
				return MTL::AttributeFormatUCharNormalized;
				
			case Format::R8G8B8A8_SNORM:
				return MTL::AttributeFormatChar4Normalized;
			case Format::R8G8_SNORM:
				return MTL::AttributeFormatChar2Normalized;
			case Format::R8_SNORM:
				return MTL::AttributeFormatCharNormalized;
				
			case Format::R10G10B10A2_UNORM:
				return MTL::AttributeFormatUInt1010102Normalized;
			case Format::R11G11B10_FLOAT:
				return MTL::AttributeFormatFloatRG11B10;
			case Format::R9G9B9E5_SHAREDEXP:
				return MTL::AttributeFormatFloatRGB9E5;
				
			default:
				break;
		}
		return MTL::AttributeFormatInvalid;
	}
	constexpr MTL::VertexFormat _ConvertVertexFormat(Format value)
	{
		switch (value)
		{
		default:
		case Format::UNKNOWN:
			return MTL::VertexFormatInvalid;
		case Format::R32G32B32A32_FLOAT:
			return MTL::VertexFormatFloat4;
		case Format::R32G32B32A32_UINT:
			return MTL::VertexFormatUInt4;
		case Format::R32G32B32A32_SINT:
			return MTL::VertexFormatInt4;
		case Format::R32G32B32_FLOAT:
			return MTL::VertexFormatFloat3;
		case Format::R32G32B32_UINT:
			return MTL::VertexFormatUInt3;
		case Format::R32G32B32_SINT:
			return MTL::VertexFormatInt3;
		case Format::R16G16B16A16_FLOAT:
			return MTL::VertexFormatHalf4;
		case Format::R16G16B16A16_UNORM:
			return MTL::VertexFormatUShort4Normalized;
		case Format::R16G16B16A16_UINT:
			return MTL::VertexFormatUShort4;
		case Format::R16G16B16A16_SNORM:
			return MTL::VertexFormatShort4Normalized;
		case Format::R16G16B16A16_SINT:
			return MTL::VertexFormatShort4;
		case Format::R32G32_FLOAT:
			return MTL::VertexFormatFloat2;
		case Format::R32G32_UINT:
			return MTL::VertexFormatUInt2;
		case Format::R32G32_SINT:
			return MTL::VertexFormatInt2;
		case Format::D32_FLOAT_S8X24_UINT:
			return MTL::VertexFormatInvalid;
		case Format::R10G10B10A2_UNORM:
			return MTL::VertexFormatUInt1010102Normalized;
		case Format::R10G10B10A2_UINT:
			return MTL::VertexFormatInvalid;
		case Format::R11G11B10_FLOAT:
			return MTL::VertexFormatFloatRG11B10;
		case Format::R8G8B8A8_UNORM:
			return MTL::VertexFormatUChar4Normalized;
		case Format::R8G8B8A8_UNORM_SRGB:
			return MTL::VertexFormatInvalid;
		case Format::R8G8B8A8_UINT:
			return MTL::VertexFormatUChar4;
		case Format::R8G8B8A8_SNORM:
			return MTL::VertexFormatChar4Normalized;
		case Format::R8G8B8A8_SINT:
			return MTL::VertexFormatChar4;
		case Format::R16G16_FLOAT:
			return MTL::VertexFormatHalf2;
		case Format::R16G16_UNORM:
			return MTL::VertexFormatUShort2Normalized;
		case Format::R16G16_UINT:
			return MTL::VertexFormatUShort2;
		case Format::R16G16_SNORM:
			return MTL::VertexFormatShort2Normalized;
		case Format::R16G16_SINT:
			return MTL::VertexFormatShort2;
		case Format::D32_FLOAT:
			return MTL::VertexFormatInvalid;
		case Format::R32_FLOAT:
			return MTL::VertexFormatFloat;
		case Format::R32_UINT:
			return MTL::VertexFormatUInt;
		case Format::R32_SINT:
			return MTL::VertexFormatInt;
		case Format::D24_UNORM_S8_UINT:
			return MTL::VertexFormatInvalid;
		case Format::R9G9B9E5_SHAREDEXP:
			return MTL::VertexFormatFloatRGB9E5;
		case Format::R8G8_UNORM:
			return MTL::VertexFormatUChar2Normalized;
		case Format::R8G8_UINT:
			return MTL::VertexFormatUChar2;
		case Format::R8G8_SNORM:
			return MTL::VertexFormatChar2Normalized;
		case Format::R8G8_SINT:
			return MTL::VertexFormatChar2;
		case Format::R16_FLOAT:
			return MTL::VertexFormatHalf;
		case Format::D16_UNORM:
			return MTL::VertexFormatInvalid;
		case Format::R16_UNORM:
			return MTL::VertexFormatUShortNormalized;
		case Format::R16_UINT:
			return MTL::VertexFormatUShort;
		case Format::R16_SNORM:
			return MTL::VertexFormatShortNormalized;
		case Format::R16_SINT:
			return MTL::VertexFormatShort;
		case Format::R8_UNORM:
			return MTL::VertexFormatUCharNormalized;
		case Format::R8_UINT:
			return MTL::VertexFormatUChar;
		case Format::R8_SNORM:
			return MTL::VertexFormatCharNormalized;
		case Format::R8_SINT:
			return MTL::VertexFormatChar;
		}
		return MTL::VertexFormatInvalid;
	}
	constexpr MTL::PixelFormat _ConvertPixelFormat(Format value)
	{
	   switch (value)
	   {
	   case Format::UNKNOWN:
		   return MTL::PixelFormatInvalid;
	   case Format::R32G32B32A32_FLOAT:
		   return MTL::PixelFormatRGBA32Float;
	   case Format::R32G32B32A32_UINT:
		   return MTL::PixelFormatRGBA32Uint;
	   case Format::R32G32B32A32_SINT:
		   return MTL::PixelFormatRGBA32Sint;
	   case Format::R32G32B32_FLOAT:
		   return MTL::PixelFormatInvalid;
	   case Format::R32G32B32_UINT:
		   return MTL::PixelFormatInvalid;
	   case Format::R32G32B32_SINT:
		   return MTL::PixelFormatInvalid;
	   case Format::R16G16B16A16_FLOAT:
		   return MTL::PixelFormatRGBA16Float;
	   case Format::R16G16B16A16_UNORM:
		   return MTL::PixelFormatRGBA16Unorm;
	   case Format::R16G16B16A16_UINT:
		   return MTL::PixelFormatRGBA16Uint;
	   case Format::R16G16B16A16_SNORM:
		   return MTL::PixelFormatRGBA16Snorm;
	   case Format::R16G16B16A16_SINT:
		   return MTL::PixelFormatRGBA16Sint;
	   case Format::R32G32_FLOAT:
		   return MTL::PixelFormatRG32Float;
	   case Format::R32G32_UINT:
		   return MTL::PixelFormatRG32Uint;
	   case Format::R32G32_SINT:
		   return MTL::PixelFormatRG32Sint;
	   case Format::D32_FLOAT_S8X24_UINT:
		   return MTL::PixelFormatDepth32Float_Stencil8;
	   case Format::R10G10B10A2_UNORM:
		   return MTL::PixelFormatBGR10A2Unorm;
	   case Format::R10G10B10A2_UINT:
		   return MTL::PixelFormatInvalid;
	   case Format::R11G11B10_FLOAT:
		   return MTL::PixelFormatRG11B10Float;
	   case Format::R8G8B8A8_UNORM:
		   return MTL::PixelFormatRGBA8Unorm;
	   case Format::R8G8B8A8_UNORM_SRGB:
		   return MTL::PixelFormatRGBA8Unorm_sRGB;
	   case Format::R8G8B8A8_UINT:
		   return MTL::PixelFormatRGBA8Uint;
	   case Format::R8G8B8A8_SNORM:
		   return MTL::PixelFormatRGBA8Snorm;
	   case Format::R8G8B8A8_SINT:
		   return MTL::PixelFormatRGBA8Sint;
	   case Format::R16G16_FLOAT:
		   return MTL::PixelFormatRG16Float;
	   case Format::R16G16_UNORM:
		   return MTL::PixelFormatRG16Unorm;
	   case Format::R16G16_UINT:
		   return MTL::PixelFormatRG16Uint;
	   case Format::R16G16_SNORM:
		   return MTL::PixelFormatRG16Snorm;
	   case Format::R16G16_SINT:
		   return MTL::PixelFormatRG16Sint;
	   case Format::D32_FLOAT:
		   return MTL::PixelFormatDepth32Float;
	   case Format::R32_FLOAT:
		   return MTL::PixelFormatR32Float;
	   case Format::R32_UINT:
		   return MTL::PixelFormatR32Uint;
	   case Format::R32_SINT:
		   return MTL::PixelFormatR32Sint;
	   case Format::D24_UNORM_S8_UINT:
		   return MTL::PixelFormatDepth24Unorm_Stencil8;
	   case Format::R9G9B9E5_SHAREDEXP:
		   return MTL::PixelFormatRGB9E5Float;
	   case Format::R8G8_UNORM:
		   return MTL::PixelFormatRG8Unorm;
	   case Format::R8G8_UINT:
		   return MTL::PixelFormatRG8Uint;
	   case Format::R8G8_SNORM:
		   return MTL::PixelFormatRG8Snorm;
	   case Format::R8G8_SINT:
		   return MTL::PixelFormatRG8Sint;
	   case Format::R16_FLOAT:
		   return MTL::PixelFormatR16Float;
	   case Format::D16_UNORM:
		   return MTL::PixelFormatDepth16Unorm;
	   case Format::R16_UNORM:
		   return MTL::PixelFormatR16Unorm;
	   case Format::R16_UINT:
		   return MTL::PixelFormatR16Uint;
	   case Format::R16_SNORM:
		   return MTL::PixelFormatR16Snorm;
	   case Format::R16_SINT:
		   return MTL::PixelFormatR16Sint;
	   case Format::R8_UNORM:
		   return MTL::PixelFormatR8Unorm;
	   case Format::R8_UINT:
		   return MTL::PixelFormatR8Uint;
	   case Format::R8_SNORM:
		   return MTL::PixelFormatR8Snorm;
	   case Format::R8_SINT:
		   return MTL::PixelFormatR8Sint;
	   case Format::BC1_UNORM:
		   return MTL::PixelFormatBC1_RGBA;
	   case Format::BC1_UNORM_SRGB:
		   return MTL::PixelFormatBC1_RGBA_sRGB;
	   case Format::BC2_UNORM:
		   return MTL::PixelFormatBC2_RGBA;
	   case Format::BC2_UNORM_SRGB:
		   return MTL::PixelFormatBC2_RGBA_sRGB;
	   case Format::BC3_UNORM:
		   return MTL::PixelFormatBC3_RGBA;
	   case Format::BC3_UNORM_SRGB:
		   return MTL::PixelFormatBC3_RGBA_sRGB;
	   case Format::BC4_UNORM:
		   return MTL::PixelFormatBC4_RUnorm;
	   case Format::BC4_SNORM:
		   return MTL::PixelFormatBC4_RSnorm;
	   case Format::BC5_UNORM:
		   return MTL::PixelFormatBC5_RGUnorm;
	   case Format::BC5_SNORM:
		   return MTL::PixelFormatBC5_RGSnorm;
	   case Format::B8G8R8A8_UNORM:
		   return MTL::PixelFormatBGRA8Unorm;
	   case Format::B8G8R8A8_UNORM_SRGB:
		   return MTL::PixelFormatBGRA8Unorm_sRGB;
	   case Format::BC6H_UF16:
		   return MTL::PixelFormatBC6H_RGBUfloat;
	   case Format::BC6H_SF16:
		   return MTL::PixelFormatBC6H_RGBFloat;
	   case Format::BC7_UNORM:
		   return MTL::PixelFormatBC7_RGBAUnorm;
	   case Format::BC7_UNORM_SRGB:
		   return MTL::PixelFormatBC7_RGBAUnorm_sRGB;
	   case Format::NV12:
		   return MTL::PixelFormatInvalid;
	   }
	   return MTL::PixelFormatInvalid;
	}
	constexpr MTL::BlendOperation _ConvertBlendOp(BlendOp value)
	{
		switch (value)
		{
			case BlendOp::ADD:
				return MTL::BlendOperationAdd;
			case BlendOp::SUBTRACT:
				return MTL::BlendOperationSubtract;
			case BlendOp::REV_SUBTRACT:
				return MTL::BlendOperationReverseSubtract;
			case BlendOp::MIN:
				return MTL::BlendOperationMin;
			case BlendOp::MAX:
				return MTL::BlendOperationMax;
		}
		return MTL::BlendOperationUnspecialized;
	}
	constexpr MTL::BlendFactor _ConvertBlendFactor(Blend value)
	{
		switch (value)
		{
			case Blend::ZERO:
				return MTL::BlendFactorZero;
			case Blend::ONE:
				return MTL::BlendFactorOne;
			case Blend::BLEND_FACTOR:
				return MTL::BlendFactorBlendColor;
			case Blend::INV_BLEND_FACTOR:
				return MTL::BlendFactorOneMinusBlendColor;
			case Blend::DEST_ALPHA:
				return MTL::BlendFactorDestinationAlpha;
			case Blend::DEST_COLOR:
				return MTL::BlendFactorDestinationColor;
			case Blend::INV_DEST_ALPHA:
				return MTL::BlendFactorOneMinusDestinationAlpha;
			case Blend::INV_DEST_COLOR:
				return MTL::BlendFactorOneMinusDestinationColor;
			case Blend::SRC_ALPHA:
				return MTL::BlendFactorSourceAlpha;
			case Blend::SRC_COLOR:
				return MTL::BlendFactorSourceColor;
			case Blend::INV_SRC_ALPHA:
				return MTL::BlendFactorOneMinusSourceAlpha;
			case Blend::INV_SRC_COLOR:
				return MTL::BlendFactorOneMinusSourceColor;
			case Blend::SRC1_ALPHA:
				return MTL::BlendFactorSource1Alpha;
			case Blend::SRC1_COLOR:
				return MTL::BlendFactorSource1Color;
			case Blend::INV_SRC1_ALPHA:
				return MTL::BlendFactorOneMinusSource1Alpha;
			case Blend::INV_SRC1_COLOR:
				return MTL::BlendFactorOneMinusSource1Color;
			case Blend::SRC_ALPHA_SAT:
				return MTL::BlendFactorSourceAlphaSaturated;
		}
		return MTL::BlendFactorUnspecialized;
	}
	constexpr MTL::CompareFunction _ConvertCompareFunction(ComparisonFunc value)
	{
		switch (value)
		{
			case ComparisonFunc::ALWAYS:
				return MTL::CompareFunctionAlways;
			case ComparisonFunc::NEVER:
				return MTL::CompareFunctionNever;
			case ComparisonFunc::EQUAL:
				return MTL::CompareFunctionEqual;
			case ComparisonFunc::NOT_EQUAL:
				return MTL::CompareFunctionNotEqual;
			case ComparisonFunc::LESS:
				return MTL::CompareFunctionLess;
			case ComparisonFunc::LESS_EQUAL:
				return MTL::CompareFunctionLessEqual;
			case ComparisonFunc::GREATER:
				return MTL::CompareFunctionGreater;
			case ComparisonFunc::GREATER_EQUAL:
				return MTL::CompareFunctionGreaterEqual;
		}
		return MTL::CompareFunctionNever;
	}
	constexpr MTL::StencilOperation _ConvertStencilOperation(StencilOp value)
	{
		switch (value)
		{
			case StencilOp::KEEP:
				return MTL::StencilOperationKeep;
			case StencilOp::REPLACE:
				return MTL::StencilOperationReplace;
			case StencilOp::ZERO:
				return MTL::StencilOperationZero;
			case StencilOp::INVERT:
				return MTL::StencilOperationInvert;
			case StencilOp::INCR:
				return MTL::StencilOperationIncrementWrap;
			case StencilOp::INCR_SAT:
				return MTL::StencilOperationIncrementClamp;
			case StencilOp::DECR:
				return MTL::StencilOperationDecrementWrap;
			case StencilOp::DECR_SAT:
				return MTL::StencilOperationDecrementClamp;
		}
		return MTL::StencilOperationKeep;
	}
	constexpr MTL::TextureSwizzle _ConvertComponentSwizzle(ComponentSwizzle value)
	{
		switch (value)
		{
			case ComponentSwizzle::R:
				return MTL::TextureSwizzleRed;
			case ComponentSwizzle::G:
				return MTL::TextureSwizzleGreen;
			case ComponentSwizzle::B:
				return MTL::TextureSwizzleBlue;
			case ComponentSwizzle::A:
				return MTL::TextureSwizzleAlpha;
			case ComponentSwizzle::ONE:
				return MTL::TextureSwizzleOne;
			case ComponentSwizzle::ZERO:
				return MTL::TextureSwizzleZero;
			default:
				break;
		}
		return MTL::TextureSwizzleOne;
	}
	constexpr MTL::SamplerAddressMode _ConvertAddressMode(TextureAddressMode value)
	{
		switch (value) {
			case TextureAddressMode::CLAMP:
				return MTL::SamplerAddressModeClampToEdge;
			case TextureAddressMode::WRAP:
				return MTL::SamplerAddressModeRepeat;
			case TextureAddressMode::MIRROR_ONCE:
				return MTL::SamplerAddressModeMirrorClampToEdge;
			case TextureAddressMode::MIRROR:
				return MTL::SamplerAddressModeMirrorRepeat;
			case TextureAddressMode::BORDER:
				return MTL::SamplerAddressModeClampToBorderColor;
			default:
				break;
		}
		return MTL::SamplerAddressModeClampToEdge;
	}

	IRDescriptorTableEntry create_entry(MTL::Texture* res, float min_lod_clamp = 0, uint32_t metadata = 0)
	{
		IRDescriptorTableEntry entry;
		IRDescriptorTableSetTexture(&entry, res, min_lod_clamp, metadata);
		return entry;
	}
	IRDescriptorTableEntry create_entry(MTL::ResourceID resourceID, float min_lod_clamp = 0, uint32_t metadata = 0)
	{
		// This is the expanded version of IRDescriptorTableSetTexture, made to be compatible with texture view pooling
		IRDescriptorTableEntry entry;
		entry.gpuVA = 0;
		entry.textureViewID = resourceID._impl;
		entry.metadata = (uint64_t)(*((uint32_t*)&min_lod_clamp) | ((uint64_t)metadata) << 32);
		return entry;
	}
	IRDescriptorTableEntry create_entry(MTL::Buffer* res, uint64_t size, uint64_t offset = 0, MTL::Texture* texture_buffer_view = nullptr, Format format = Format::UNKNOWN, bool structured = false)
	{
		IRDescriptorTableEntry entry;
		IRBufferView view = {};
		view.buffer = res;
		view.bufferSize = size;
		view.bufferOffset = offset;
		if (texture_buffer_view == nullptr)
		{
			view.typedBuffer = structured || (format != Format::UNKNOWN);
		}
		else
		{
			view.typedBuffer = true;
			view.textureBufferView = texture_buffer_view;
			view.textureViewOffsetInElements = uint32_t(offset / (uint64_t)GetFormatStride(format));
		}
		IRDescriptorTableSetBufferView(&entry, &view);
		return entry;
	}
	IRDescriptorTableEntry create_entry(MTL::Buffer* res, uint64_t size, uint64_t offset = 0, MTL::ResourceID resourceID = {}, Format format = Format::UNKNOWN, bool structured = false)
	{
		// This is the expanded version of IRDescriptorTableSetBufferView, made to be compatible with texture view pooling
		IRDescriptorTableEntry entry;
		entry.gpuVA = res->gpuAddress() + offset;
		entry.textureViewID = resourceID._impl;
		entry.metadata = (size & kIRBufSizeMask) << kIRBufSizeOffset;
		bool typedBuffer = false;
		uint64_t textureViewOffsetInElements = 0;
		if (entry.textureViewID == 0)
		{
			typedBuffer = structured || (format != Format::UNKNOWN);
		}
		else
		{
			typedBuffer = true;
			textureViewOffsetInElements = uint32_t(offset / (uint64_t)GetFormatStride(format));
		}
		entry.metadata |= ((uint64_t)textureViewOffsetInElements & kIRTexViewMask) << kIRTexViewOffset;
		entry.metadata |= (uint64_t)typedBuffer << kIRTypedBufferOffset;
		return entry;
	}
	IRDescriptorTableEntry create_entry(MTL::SamplerState* sam, float lod_bias = 0)
	{
		IRDescriptorTableEntry entry;
		IRDescriptorTableSetSampler(&entry, sam, lod_bias);
		return entry;
	}

	struct Buffer_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::Buffer> buffer;
		MTL::GPUAddress gpu_address = {};
		
		struct Subresource
		{
			IRDescriptorTableEntry entry = {};
			uint64_t offset = 0;
			uint64_t size = 0;
			int index = -1;
#ifndef USE_TEXTURE_VIEW_POOL
			NS::SharedPtr<MTL::Texture> view;
#endif // USE_TEXTURE_VIEW_POOL
			
			bool IsValid() const { return entry.gpuVA != 0; }
		};
		Subresource srv;
		Subresource uav;
		wi::vector<Subresource> subresources_srv;
		wi::vector<Subresource> subresources_uav;
		
		void destroy_subresources()
		{
			uint64_t framecount = allocationhandler->framecount;
			
#ifndef USE_TEXTURE_VIEW_POOL
			allocationhandler->destroyer_resources.push_back(std::make_pair(srv.view, framecount));
			allocationhandler->destroyer_resources.push_back(std::make_pair(uav.view, framecount));
			for (auto& x : subresources_srv)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(x.view, framecount));
			}
			for (auto& x : subresources_uav)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(x.view, framecount));
			}
#endif // USE_TEXTURE_VIEW_POOL
			
			if (srv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(srv.index, framecount));
			srv = {};
			
			if (uav.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(uav.index, framecount));
			uav = {};
			
			for (auto& x : subresources_srv)
			{
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_srv.clear();
			for (auto& x : subresources_uav)
			{
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_uav.clear();
		}

		~Buffer_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(buffer), framecount));
			destroy_subresources();
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Texture_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::Texture> texture;
		
		// Things for readback and upload texture types, linear tiling, using buffer:
		NS::SharedPtr<MTL::Buffer> buffer;
		wi::vector<SubresourceData> mapped_subresources;
		
		struct Subresource
		{
			IRDescriptorTableEntry entry = {};
			int index = -1;
#ifndef USE_TEXTURE_VIEW_POOL
			NS::SharedPtr<MTL::Texture> view;
#endif // USE_TEXTURE_VIEW_POOL
			
			uint32_t firstMip = 0;
			uint32_t mipCount = 0;
			uint32_t firstSlice = 0;
			uint32_t sliceCount = 0;
			
			bool IsValid() const { return entry.textureViewID != 0; }
		};
		Subresource srv;
		Subresource uav;
		Subresource rtv;
		Subresource dsv;
		wi::vector<Subresource> subresources_srv;
		wi::vector<Subresource> subresources_uav;
		wi::vector<Subresource> subresources_rtv;
		wi::vector<Subresource> subresources_dsv;
		
		SparseTextureProperties sparse_properties;
		
		void destroy_subresources()
		{
			uint64_t framecount = allocationhandler->framecount;
			
#ifndef USE_TEXTURE_VIEW_POOL
			allocationhandler->destroyer_resources.push_back(std::make_pair(srv.view, framecount));
			allocationhandler->destroyer_resources.push_back(std::make_pair(uav.view, framecount));
			for (auto& x : subresources_srv)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(x.view, framecount));
			}
			for (auto& x : subresources_uav)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(x.view, framecount));
			}
#endif // USE_TEXTURE_VIEW_POOL
			
			if (srv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(srv.index, framecount));
			srv = {};
			
			if (uav.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(uav.index, framecount));
			uav = {};
			
			if (rtv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(rtv.index, framecount));
			
			if (dsv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(dsv.index, framecount));
			
			for (auto& x : subresources_srv)
			{
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_srv.clear();
			for (auto& x : subresources_uav)
			{
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_uav.clear();
			
			// RTV and DSV can just be cleared, they don't have any gpu-referenced resources:
			subresources_rtv.clear();
			subresources_dsv.clear();
		}

		~Texture_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (texture.get() != nullptr)
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(texture), framecount));
			if (buffer.get() != nullptr)
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(buffer), framecount));
			destroy_subresources();
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Sampler_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		int index = -1;
		NS::SharedPtr<MTL::SamplerState> sampler;
		IRDescriptorTableEntry entry = {};

		~Sampler_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_samplers.push_back(std::make_pair(std::move(sampler), framecount));
			if (index >= 0)
				allocationhandler->destroyer_bindless_sam.push_back(std::make_pair(index, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct QueryHeap_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::Buffer> buffer;
		NS::SharedPtr<MTL4::CounterHeap> counter_heap;

		~QueryHeap_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(buffer), framecount));
			allocationhandler->destroyer_counter_heaps.push_back(std::make_pair(std::move(counter_heap), framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Shader_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::Library> library;
		NS::SharedPtr<MTL::Function> function;
		NS::SharedPtr<MTL::ComputePipelineState> compute_pipeline;
		MTL::Size numthreads = {};
		bool needs_draw_params = false;

		~Shader_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_libraries.push_back(std::make_pair(std::move(library), framecount));
			allocationhandler->destroyer_functions.push_back(std::make_pair(std::move(function), framecount));
			allocationhandler->destroyer_compute_pipelines.push_back(std::make_pair(std::move(compute_pipeline), framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct PipelineState_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::RenderPipelineDescriptor> descriptor;
		NS::SharedPtr<MTL::RenderPipelineState> render_pipeline;
		NS::SharedPtr<MTL::DepthStencilState> depth_stencil_state;
		bool needs_draw_params = false;

		~PipelineState_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_render_pipelines.push_back(std::make_pair(std::move(render_pipeline), framecount));
			allocationhandler->destroyer_depth_stencil_states.push_back(std::make_pair(std::move(depth_stencil_state), framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct BVH_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::AccelerationStructure> acceleration_structure;
		NS::SharedPtr<MTL::Buffer> scratch;
		NS::SharedPtr<MTL::Buffer> tlas_header_instancecontributions;
		int tlas_descriptor_index = -1;
		IRDescriptorTableEntry tlas_entry = {};
		MTL::ResourceID resourceid = {};

		~BVH_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(acceleration_structure), framecount));
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(scratch), framecount));
			if (tlas_header_instancecontributions.get() != nullptr)
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(tlas_header_instancecontributions), framecount));
			if (tlas_descriptor_index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(tlas_descriptor_index, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct RTPipelineState_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;

		~RTPipelineState_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroylocker.unlock();
		}
	};
	struct SwapChain_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		ColorSpace colorSpace = ColorSpace::SRGB;
		CA::MetalLayer* layer = nullptr;
		NS::SharedPtr<CA::MetalDrawable> current_drawable;
		NS::SharedPtr<MTL::Texture> current_texture;

		~SwapChain_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroylocker.unlock();
		}
	};
	struct VideoDecoder_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;

		~VideoDecoder_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroylocker.unlock();
		}
	};

	template<typename T> struct MetalType;
	template<> struct MetalType<GPUBuffer> { using type = Buffer_Metal; };
	template<> struct MetalType<Texture> { using type = Texture_Metal; };
	template<> struct MetalType<Sampler> { using type = Sampler_Metal; };
	template<> struct MetalType<GPUQueryHeap> { using type = QueryHeap_Metal; };
	template<> struct MetalType<Shader> { using type = Shader_Metal; };
	template<> struct MetalType<RaytracingAccelerationStructure> { using type = BVH_Metal; };
	template<> struct MetalType<PipelineState> { using type = PipelineState_Metal; };
	template<> struct MetalType<RaytracingPipelineState> { using type = RTPipelineState_Metal; };
	template<> struct MetalType<SwapChain> { using type = SwapChain_Metal; };
	template<> struct MetalType<VideoDecoder> { using type = VideoDecoder_Metal; };


	template<typename T>
	typename MetalType<T>::type* to_internal(const T* param)
	{
		return static_cast<typename MetalType<T>::type*>(param->internal_state.get());
	}

	template<typename T>
	typename MetalType<T>::type* to_internal(const GPUResource* res)
	{
		return static_cast<typename MetalType<T>::type*>(res->internal_state.get());
	}
}
using namespace metal_internal;

	void GraphicsDevice_Metal::binder_flush(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		if (commandlist.dirty_root)
		{
			// root CBVs:
			for (uint32_t i = 0; i < arraysize(RootLayout::root_cbvs); ++i)
			{
				if (!commandlist.binding_table.CBV[i].IsValid())
					continue;
				auto internal_state = to_internal(&commandlist.binding_table.CBV[i]);
				commandlist.root.root_cbvs[i] = internal_state->gpu_address + commandlist.binding_table.CBV_offset[i];
				assert(is_aligned(commandlist.root.root_cbvs[i], MTL::GPUAddress(256)));
			}
			
			if (commandlist.dirty_resource)
			{
				commandlist.dirty_resource = false;
				ResourceTable resource_table = {};
				GPUAllocation resource_table_allocation = AllocateGPU(sizeof(ResourceTable), cmd);
				auto resource_table_allocation_internal = to_internal(&resource_table_allocation.buffer);
				commandlist.root.resource_table_ptr = resource_table_allocation_internal->gpu_address + resource_table_allocation.offset;
				
				for (uint32_t i = arraysize(RootLayout::root_cbvs); i < arraysize(DescriptorBindingTable::CBV); ++i)
				{
					if (!commandlist.binding_table.CBV[i].IsValid())
						continue;
					
					auto internal_state = to_internal(&commandlist.binding_table.CBV[i]);
					const uint64_t offset = commandlist.binding_table.CBV_offset[i];
					const MTL::GPUAddress gpu_address = internal_state->gpu_address + offset;
					const uint64_t metadata = commandlist.binding_table.CBV[i].desc.size;
					IRDescriptorTableSetBuffer(&resource_table.cbvs[i - arraysize(RootLayout::root_cbvs)], gpu_address, metadata);
				}
				for (uint32_t i = 0; i < arraysize(commandlist.binding_table.SRV); ++i)
				{
					if (!commandlist.binding_table.SRV[i].IsValid())
						continue;
					
					if (commandlist.binding_table.SRV[i].IsBuffer())
					{
						auto internal_state = to_internal<GPUBuffer>(&commandlist.binding_table.SRV[i]);
						const int subresource_index = commandlist.binding_table.SRV_index[i];
						const auto& subresource = subresource_index < 0 ? internal_state->srv : internal_state->subresources_srv[subresource_index];
						resource_table.srvs[i] = subresource.entry;
					}
					else if (commandlist.binding_table.SRV[i].IsTexture())
					{
						auto internal_state = to_internal<Texture>(&commandlist.binding_table.SRV[i]);
						const int subresource_index = commandlist.binding_table.SRV_index[i];
						const auto& subresource = subresource_index < 0 ? internal_state->srv : internal_state->subresources_srv[subresource_index];
						resource_table.srvs[i] = subresource.entry;
					}
					else if (commandlist.binding_table.SRV[i].IsAccelerationStructure())
					{
						auto internal_state = to_internal<RaytracingAccelerationStructure>(&commandlist.binding_table.SRV[i]);
						resource_table.srvs[i] = internal_state->tlas_entry;
					}
				}
				for (uint32_t i = 0; i < arraysize(commandlist.binding_table.UAV); ++i)
				{
					if (!commandlist.binding_table.UAV[i].IsValid())
						continue;
					
					if (commandlist.binding_table.UAV[i].IsBuffer())
					{
						auto internal_state = to_internal<GPUBuffer>(&commandlist.binding_table.UAV[i]);
						const int subresource_index = commandlist.binding_table.UAV_index[i];
						const auto& subresource = subresource_index < 0 ? internal_state->uav : internal_state->subresources_uav[subresource_index];
						resource_table.uavs[i] = subresource.entry;
					}
					else if (commandlist.binding_table.UAV[i].IsTexture())
					{
						auto internal_state = to_internal<Texture>(&commandlist.binding_table.UAV[i]);
						const int subresource_index = commandlist.binding_table.UAV_index[i];
						const auto& subresource = subresource_index < 0 ? internal_state->uav : internal_state->subresources_uav[subresource_index];
						resource_table.uavs[i] = subresource.entry;
					}
				}
				
				std::memcpy(resource_table_allocation.data, &resource_table, sizeof(resource_table));
			}
			if (commandlist.dirty_sampler)
			{
				commandlist.dirty_sampler = false;
				SamplerTable sampler_table = {};
				sampler_table.static_samplers = static_sampler_descriptors;
				GPUAllocation sampler_table_allocation = AllocateGPU(sizeof(SamplerTable), cmd);
				auto sampler_table_allocation_internal = to_internal(&sampler_table_allocation.buffer);
				commandlist.root.sampler_table_ptr = sampler_table_allocation_internal->gpu_address + sampler_table_allocation.offset;
				
				for (uint32_t i = 0; i < arraysize(commandlist.binding_table.SAM); ++i)
				{
					if (!commandlist.binding_table.SAM[i].IsValid())
						continue;
					auto internal_state = to_internal(&commandlist.binding_table.SAM[i]);
					sampler_table.samplers[i] = internal_state->entry;
				}
				
				std::memcpy(sampler_table_allocation.data, &sampler_table, sizeof(sampler_table));
			}
			
			auto alloc = AllocateGPU(sizeof(commandlist.root), cmd);
			std::memcpy(alloc.data, &commandlist.root, sizeof(commandlist.root));
			auto alloc_internal = to_internal(&alloc.buffer);
			commandlist.argument_table->setAddress(alloc_internal->gpu_address + alloc.offset, kIRArgumentBufferBindPoint);
		}
		if (commandlist.dirty_vb && commandlist.active_pso != nullptr && commandlist.active_pso->desc.il != nullptr)
		{
			const InputLayout& il = *commandlist.active_pso->desc.il;
			for (size_t i = 0; i < il.elements.size(); ++i)
			{
				auto& element = il.elements[i];
				auto& vb = commandlist.vertex_buffers[element.input_slot];
				if (vb.gpu_address == 0)
					continue;
				commandlist.argument_table->setAddress(vb.gpu_address, vb.stride, kIRVertexBufferBindPoint + i);
			}
		}
		
		// Flushing argument table updates to encoder:
		if (commandlist.dirty_root || commandlist.dirty_vb || commandlist.dirty_drawargs)
		{
			commandlist.dirty_root = false;
			commandlist.dirty_vb = false;
			commandlist.dirty_drawargs = false;
			
			if (commandlist.render_encoder != nullptr)
			{
				commandlist.render_encoder->setArgumentTable(commandlist.argument_table.get(), MTL::RenderStageVertex | MTL::RenderStageFragment);
			}
			else if (commandlist.compute_encoder != nullptr)
			{
				commandlist.compute_encoder->setArgumentTable(commandlist.argument_table.get());
			}
		}
	}

	void GraphicsDevice_Metal::pso_validate(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (!commandlist.dirty_pso)
			return;
		
		assert(commandlist.render_encoder != nullptr); // We must be inside renderpass at this point!
		
		PipelineHash pipeline_hash = commandlist.pipeline_hash;
		const PipelineState* pso = commandlist.active_pso;
		auto internal_state = to_internal(pso);
		
		if (internal_state->render_pipeline.get() == nullptr)
		{
			// Just in time PSO:
			MTL::RenderPipelineState* pipeline = nullptr;
			MTL::DepthStencilState* depth_stencil_state = nullptr;
			auto it = pipelines_global.find(pipeline_hash);
			if (it != pipelines_global.end())
			{
				// Exists in global just in time PSO map:
				pipeline = it->second.pipeline.get();
				depth_stencil_state = it->second.depth_stencil_state.get();
			}
			else
			{
				// Doesn't yet exist in global just in time PSO map, but maybe exists on this thread temporary PSO array:
				for (auto& x : commandlist.pipelines_worker)
				{
					if (pipeline_hash == x.first)
					{
						pipeline = x.second.pipeline.get();
						depth_stencil_state = x.second.depth_stencil_state.get();
						break;
					}
				}
			}
			if (pipeline == nullptr)
			{
				RenderPassInfo renderpass_info = GetRenderPassInfo(cmd);
				PipelineState newPSO;
				bool success = CreatePipelineState(&pso->desc, &newPSO, &renderpass_info);
				assert(success);
				assert(newPSO.IsValid());

				auto internal_new = to_internal(&newPSO);
				assert(internal_new->render_pipeline.get() != nullptr);
				assert(internal_new->depth_stencil_state.get() != nullptr);
				JustInTimePSO just_in_time_pso;
				just_in_time_pso.pipeline = internal_new->render_pipeline;
				just_in_time_pso.depth_stencil_state = internal_new->depth_stencil_state;
				commandlist.pipelines_worker.push_back(std::make_pair(pipeline_hash, std::move(just_in_time_pso)));
				pipeline = internal_new->render_pipeline.get();
				depth_stencil_state = internal_new->depth_stencil_state.get();
			}
			assert(pipeline != nullptr);
			assert(depth_stencil_state != nullptr);
			commandlist.render_encoder->setRenderPipelineState(pipeline);
			commandlist.render_encoder->setDepthStencilState(depth_stencil_state);
		}
		else
		{
			// Precompiled PSO:
			commandlist.render_encoder->setRenderPipelineState(internal_state->render_pipeline.get());
			commandlist.render_encoder->setDepthStencilState(internal_state->depth_stencil_state.get());
		}
		
		const RasterizerState& rs = commandlist.active_pso->desc.rs == nullptr ? RasterizerState() : *commandlist.active_pso->desc.rs;
		MTL::CullMode cull_mode = {};
		switch (rs.cull_mode)
		{
			case CullMode::BACK:
				cull_mode = MTL::CullModeBack;
				break;
			case CullMode::FRONT:
				cull_mode = MTL::CullModeFront;
				break;
			case CullMode::NONE:
				cull_mode = MTL::CullModeNone;
				break;
		}
		commandlist.render_encoder->setCullMode(cull_mode);
		commandlist.render_encoder->setDepthBias((float)rs.depth_bias, rs.slope_scaled_depth_bias, rs.depth_bias_clamp);
		commandlist.render_encoder->setDepthClipMode(rs.depth_clip_enable ? MTL::DepthClipModeClip : MTL::DepthClipModeClamp);
		MTL::TriangleFillMode fill_mode = {};
		switch (rs.fill_mode)
		{
			case FillMode::SOLID:
				fill_mode = MTL::TriangleFillModeFill;
				break;
			case FillMode::WIREFRAME:
				fill_mode = MTL::TriangleFillModeLines;
				break;
		}
		commandlist.render_encoder->setTriangleFillMode(fill_mode);
		commandlist.render_encoder->setFrontFacingWinding(rs.front_counter_clockwise ? MTL::WindingCounterClockwise : MTL::WindingClockwise);
		
		switch (pso->desc.pt)
		{
			case PrimitiveTopology::TRIANGLELIST:
				commandlist.primitive_type = MTL::PrimitiveTypeTriangle;
				break;
			case PrimitiveTopology::TRIANGLESTRIP:
				commandlist.primitive_type = MTL::PrimitiveTypeTriangleStrip;
				break;
			case PrimitiveTopology::LINELIST:
				commandlist.primitive_type = MTL::PrimitiveTypeLine;
				break;
			case PrimitiveTopology::LINESTRIP:
				commandlist.primitive_type = MTL::PrimitiveTypeLineStrip;
				break;
			case PrimitiveTopology::POINTLIST:
				commandlist.primitive_type = MTL::PrimitiveTypePoint;
				break;
			case PrimitiveTopology::PATCHLIST:
				commandlist.primitive_type = MTL::PrimitiveTypeTriangle;
				break;
			default:
				break;
		}
		
		commandlist.dirty_pso = false;
	}
	void GraphicsDevice_Metal::predraw(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.active_cs = nullptr;
		assert(commandlist.render_encoder != nullptr);
		pso_validate(cmd);
		binder_flush(cmd);
		
		if (commandlist.dirty_scissor && commandlist.scissor_count > 0)
		{
			commandlist.dirty_scissor = false;
			MTL::ScissorRect scissors[arraysize(commandlist.scissors)];
			std::memcpy(&scissors, commandlist.scissors, sizeof(commandlist.scissors));
			for (uint32_t i = 0; i < commandlist.scissor_count; ++i)
			{
				MTL::ScissorRect& scissor = scissors[i];
				scissor.x = clamp(scissor.x, NS::UInteger(0), NS::UInteger(commandlist.render_width));
				scissor.y = clamp(scissor.y, NS::UInteger(0), NS::UInteger(commandlist.render_height));
				scissor.width = clamp(scissor.width, NS::UInteger(0), NS::UInteger(commandlist.render_width) - scissor.x);
				scissor.height = clamp(scissor.height, NS::UInteger(0), NS::UInteger(commandlist.render_height) - scissor.y);
			}
			commandlist.render_encoder->setScissorRects(scissors, commandlist.scissor_count);
		}
		if (commandlist.dirty_viewport && commandlist.viewport_count > 0)
		{
			commandlist.dirty_viewport = false;
			commandlist.render_encoder->setViewports(commandlist.viewports, commandlist.viewport_count);
		}
	}
	void GraphicsDevice_Metal::predispatch(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.autorelease_start();
		if (commandlist.compute_encoder == nullptr)
		{
			commandlist.compute_encoder = commandlist.commandbuffer->computeCommandEncoder();
		}
		commandlist.active_pso = nullptr;
		commandlist.dirty_vb = true;
		commandlist.dirty_root = true;
		commandlist.dirty_sampler = true;
		commandlist.dirty_resource = true;
		commandlist.dirty_scissor = true;
		commandlist.dirty_viewport = true;
		commandlist.dirty_cs = true;
		
		if (commandlist.dirty_cs && commandlist.active_cs != nullptr)
		{
			commandlist.dirty_cs = false;
			auto internal_state = to_internal(commandlist.active_cs);
			commandlist.compute_encoder->setComputePipelineState(internal_state->compute_pipeline.get());
		}
		
		binder_flush(cmd);
		
		if (!commandlist.barriers.empty())
		{
			commandlist.compute_encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, MTL4::VisibilityOptionNone);
			commandlist.barriers.clear();
		}
	}
	void GraphicsDevice_Metal::precopy(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.autorelease_start();
		if (commandlist.compute_encoder == nullptr)
		{
			commandlist.compute_encoder = commandlist.commandbuffer->computeCommandEncoder();
		}
		
		if (!commandlist.barriers.empty())
		{
			commandlist.compute_encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, MTL4::VisibilityOptionNone);
			commandlist.barriers.clear();
		}
	}

	GraphicsDevice_Metal::GraphicsDevice_Metal(ValidationMode validationMode_, GPUPreference preference)
	{
		wi::Timer timer;
		device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
		
		if (device.get() == nullptr)
		{
			wilog_messagebox("Metal graphics device creation failed, exiting!");
			wi::platform::Exit();
		}
		if (!device->supportsFamily(MTL::GPUFamilyMetal4))
		{
			wilog_messagebox("Metal 4 graphics is not supported by your system, exiting!");
			wi::platform::Exit();
		}
		
		capabilities |= GraphicsDeviceCapability::SAMPLER_MINMAX;
		capabilities |= GraphicsDeviceCapability::ALIASING_GENERIC;
		capabilities |= GraphicsDeviceCapability::DEPTH_BOUNDS_TEST;
		capabilities |= GraphicsDeviceCapability::UAV_LOAD_FORMAT_COMMON;
		capabilities |= GraphicsDeviceCapability::UAV_LOAD_FORMAT_R11G11B10_FLOAT;
		capabilities |= GraphicsDeviceCapability::SPARSE_BUFFER;
		capabilities |= GraphicsDeviceCapability::SPARSE_TEXTURE2D;
		capabilities |= GraphicsDeviceCapability::SPARSE_TEXTURE3D;
		capabilities |= GraphicsDeviceCapability::SPARSE_NULL_MAPPING;
		capabilities |= GraphicsDeviceCapability::DEPTH_RESOLVE_MIN_MAX;
		capabilities |= GraphicsDeviceCapability::STENCIL_RESOLVE_MIN_MAX;
		capabilities |= GraphicsDeviceCapability::COPY_BETWEEN_DIFFERENT_IMAGE_ASPECTS_NOT_SUPPORTED;
		
		if (device->hasUnifiedMemory())
		{
			capabilities |= GraphicsDeviceCapability::CACHE_COHERENT_UMA;
		}
		if (device->supportsRaytracing())
		{
			capabilities |= GraphicsDeviceCapability::RAYTRACING;
			TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(MTL::IndirectAccelerationStructureInstanceDescriptor);
		}
		
		TIMESTAMP_FREQUENCY = device->queryTimestampFrequency();
		
		uploadqueue = NS::TransferPtr(device->newMTL4CommandQueue());
		allocationhandler = wi::allocator::make_shared_single<AllocationHandler>();
		
		argument_table_desc = NS::TransferPtr(MTL4::ArgumentTableDescriptor::alloc()->init());
		argument_table_desc->setInitializeBindings(false);
		argument_table_desc->setSupportAttributeStrides(true);
		argument_table_desc->setMaxBufferBindCount(31);
		argument_table_desc->setMaxTextureBindCount(0);
		argument_table_desc->setMaxSamplerStateBindCount(0);
		
		descriptor_heap_res = NS::TransferPtr(device->newBuffer(bindless_resource_capacity * sizeof(IRDescriptorTableEntry), MTL::ResourceStorageModeShared | MTL::ResourceHazardTrackingModeUntracked));
		descriptor_heap_res->setLabel(NS::TransferPtr(NS::String::alloc()->init("descriptor_heap_res", NS::UTF8StringEncoding)).get());
		
		const uint64_t real_bindless_sampler_capacity = std::min(bindless_sampler_capacity, (uint64_t)device->maxArgumentBufferSamplerCount());
		descriptor_heap_sam = NS::TransferPtr(device->newBuffer(real_bindless_sampler_capacity * sizeof(IRDescriptorTableEntry), MTL::ResourceStorageModeShared | MTL::ResourceHazardTrackingModeUntracked));
		descriptor_heap_sam->setLabel(NS::TransferPtr(NS::String::alloc()->init("descriptor_heap_sam", NS::UTF8StringEncoding)).get());
		
		descriptor_heap_res_data = (IRDescriptorTableEntry*)descriptor_heap_res->contents();
		descriptor_heap_sam_data = (IRDescriptorTableEntry*)descriptor_heap_sam->contents();
		
		allocationhandler->free_bindless_res.reserve(bindless_resource_capacity);
		allocationhandler->free_bindless_sam.reserve(real_bindless_sampler_capacity);
		for (int i = 0; i < real_bindless_sampler_capacity; ++i)
		{
			allocationhandler->free_bindless_sam.push_back((int)real_bindless_sampler_capacity - i - 1);
		}
		for (int i = 0; i < bindless_resource_capacity; ++i)
		{
			allocationhandler->free_bindless_res.push_back((int)bindless_resource_capacity - i - 1);
		}
		
		NS::SharedPtr<MTL::ResidencySetDescriptor> residency_set_descriptor = NS::TransferPtr(MTL::ResidencySetDescriptor::alloc()->init());
		residency_set_descriptor->setInitialCapacity(bindless_resource_capacity + real_bindless_sampler_capacity);
		NS::Error* error = nullptr;
		allocationhandler->residency_set = NS::TransferPtr(device->newResidencySet(residency_set_descriptor.get(), &error));
		if (error != nullptr)
		{
			NS::String* errDesc = error->localizedDescription();
			wilog_assert(0, "%s", errDesc->utf8String());
			error->release();
		}
		uploadqueue->addResidencySet(allocationhandler->residency_set.get());
		allocationhandler->make_resident(descriptor_heap_res.get());
		allocationhandler->make_resident(descriptor_heap_sam.get());
		
#ifdef USE_TEXTURE_VIEW_POOL
		NS::SharedPtr<MTL::ResourceViewPoolDescriptor> view_pool_desc = NS::TransferPtr(MTL::ResourceViewPoolDescriptor::alloc()->init());
		view_pool_desc->setResourceViewCount(bindless_resource_capacity);
		texture_view_pool = NS::TransferPtr(device->newTextureViewPool(view_pool_desc.get(), &error));
		if (error != nullptr)
		{
			NS::String* errDesc = error->localizedDescription();
			wilog_assert(0, "%s", errDesc->utf8String());
			error->release();
		}
#endif // USE_TEXTURE_VIEW_POOL
		
		// Static samplers workaround:
		NS::SharedPtr<MTL::SamplerDescriptor> sampler_descriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
		sampler_descriptor->setLodMinClamp(0);
		sampler_descriptor->setLodMaxClamp(FLT_MAX);
		sampler_descriptor->setBorderColor(MTL::SamplerBorderColorTransparentBlack);
		sampler_descriptor->setMaxAnisotropy(1);
		sampler_descriptor->setReductionMode(MTL::SamplerReductionModeWeightedAverage);
		sampler_descriptor->setSupportArgumentBuffers(true);
		
		sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMipFilter(MTL::SamplerMipFilterLinear);
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		static_samplers[0] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeRepeat);
		static_samplers[1] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		static_samplers[2] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		
		sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterNearest);
		sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterNearest);
		sampler_descriptor->setMipFilter(MTL::SamplerMipFilterNearest);
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		static_samplers[3] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeRepeat);
		static_samplers[4] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		static_samplers[5] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		
		sampler_descriptor->setMaxAnisotropy(16);
		sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMipFilter(MTL::SamplerMipFilterLinear);
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		static_samplers[6] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeRepeat);
		static_samplers[7] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeMirrorRepeat);
		static_samplers[8] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		
		sampler_descriptor->setMaxAnisotropy(1);
		sampler_descriptor->setLodMaxClamp(0);
		sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
		sampler_descriptor->setMipFilter(MTL::SamplerMipFilterNearest);
		sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
		sampler_descriptor->setCompareFunction(MTL::CompareFunctionGreaterEqual);
		static_samplers[9] = NS::TransferPtr(device->newSamplerState(sampler_descriptor.get()));
		
		for (uint32_t i = 0; i < arraysize(static_samplers); ++i)
		{
			IRDescriptorTableSetSampler(&static_sampler_descriptors.samplers[i], static_samplers[i].get(), 0);
		}
		
		queues[QUEUE_GRAPHICS].queue = NS::TransferPtr(device->newMTL4CommandQueue());
		queues[QUEUE_GRAPHICS].queue->addResidencySet(allocationhandler->residency_set.get());
		queues[QUEUE_COMPUTE].queue = NS::TransferPtr(device->newMTL4CommandQueue());
		queues[QUEUE_COMPUTE].queue->addResidencySet(allocationhandler->residency_set.get());
		queues[QUEUE_COPY].queue = NS::TransferPtr(device->newMTL4CommandQueue());
		queues[QUEUE_COPY].queue->addResidencySet(allocationhandler->residency_set.get());
		
		for (uint32_t q = 0; q < QUEUE_COUNT; ++q)
		{
			for (uint32_t i = 0; i < BUFFERCOUNT; ++i)
			{
				if (queues[q].queue.get() == nullptr)
					continue;
				frame_fence[i][q] = NS::TransferPtr(device->newSharedEvent());
				frame_fence[i][q]->setSignaledValue(0);
			}
		}
		
		if (CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			// This creates a dummy BLAS that will be a stand-in for unused TLAS instances:
			NS::SharedPtr<MTL4::PrimitiveAccelerationStructureDescriptor> emptydesc = NS::TransferPtr(MTL4::PrimitiveAccelerationStructureDescriptor::alloc()->init());
			NS::SharedPtr<MTL4::AccelerationStructureBoundingBoxGeometryDescriptor> geo = NS::TransferPtr(MTL4::AccelerationStructureBoundingBoxGeometryDescriptor::alloc()->init());
			MTL::AxisAlignedBoundingBox box = {};
			dummyblasbuffer = NS::TransferPtr(device->newBuffer(sizeof(box), MTL::ResourceStorageModeShared));
			std::memcpy(dummyblasbuffer->contents(), &box, sizeof(box));
			geo->setBoundingBoxBuffer({dummyblasbuffer->gpuAddress(), dummyblasbuffer->length()});
			geo->setBoundingBoxStride(sizeof(box));
			geo->setBoundingBoxCount(1);
			MTL4::AccelerationStructureBoundingBoxGeometryDescriptor* geos[] = { geo.get() };
			NS::SharedPtr<NS::Array> array = NS::TransferPtr(NS::Array::array((NS::Object**)geos, arraysize(geos))->retain());
			emptydesc->setGeometryDescriptors(array.get());
			MTL::AccelerationStructureSizes size = device->accelerationStructureSizes(emptydesc.get());
			dummyblas = NS::TransferPtr(device->newAccelerationStructure(size.accelerationStructureSize));
			dummyblas->setLabel(NS::TransferPtr(NS::String::alloc()->init("dummyBLAS", NS::UTF8StringEncoding)).get());
			dummyblas_resourceid = dummyblas->gpuResourceID();
			NS::SharedPtr<MTL::Buffer> scratch = NS::TransferPtr(device->newBuffer(size.buildScratchBufferSize, MTL::ResourceStorageModePrivate));
			NS::SharedPtr<MTL4::CommandBuffer> commandbuffer = NS::TransferPtr(device->newCommandBuffer());
			NS::SharedPtr<MTL4::CommandAllocator> commandallocator = NS::TransferPtr(device->newCommandAllocator());
			commandbuffer->beginCommandBuffer(commandallocator.get());
			allocationhandler->residency_set->addAllocation(dummyblasbuffer.get());
			allocationhandler->residency_set->addAllocation(dummyblas.get());
			allocationhandler->residency_set->addAllocation(scratch.get());
			allocationhandler->residency_set->commit();
			MTL4::ComputeCommandEncoder* encoder = commandbuffer->computeCommandEncoder();
			MTL4::BufferRange scratch_range = {};
			scratch_range.bufferAddress = scratch->gpuAddress();
			scratch_range.length = scratch->length();
			encoder->buildAccelerationStructure(dummyblas.get(), emptydesc.get(), scratch_range);
			encoder->endEncoding();
			commandbuffer->endCommandBuffer();
			MTL4::CommandBuffer* cmds[] = {commandbuffer.get()};
			uploadqueue->commit(cmds, arraysize(cmds));
		}
		
		wilog("Created GraphicsDevice_Metal (%d ms)", (int)std::round(timer.elapsed()));
	}
	GraphicsDevice_Metal::~GraphicsDevice_Metal()
	{
		WaitForGPU();
	}

	bool GraphicsDevice_Metal::CreateSwapChain(const SwapChainDesc* desc, wi::platform::window_type window, SwapChain* swapchain) const
	{
		auto internal_state = swapchain->IsValid() ? wi::allocator::shared_ptr<SwapChain_Metal>(swapchain->internal_state) : wi::allocator::make_shared<SwapChain_Metal>();
		internal_state->allocationhandler = allocationhandler;
		swapchain->internal_state = internal_state;
		swapchain->desc = *desc;
		
		swapchain->desc.buffer_count = clamp(desc->buffer_count, 2u, 3u);
		
		if (internal_state->layer == nullptr)
		{
			internal_state->layer = CA::MetalLayer::layer();
			internal_state->layer->setDevice(device.get());
			internal_state->layer->setMaximumDrawableCount(swapchain->desc.buffer_count);
			internal_state->layer->setFramebufferOnly(false); // GetBackBuffer() srv
			wi::apple::SetMetalLayerToWindow(window, internal_state->layer);
		}
		
		CGSize size = {(CGFloat)desc->width, (CGFloat)desc->height};
		internal_state->layer->setDrawableSize(size);
		internal_state->layer->setDisplaySyncEnabled(desc->vsync);
		internal_state->layer->setPixelFormat(_ConvertPixelFormat(desc->format));

		return internal_state->layer != nullptr;
	}
	bool GraphicsDevice_Metal::CreateBuffer2(const GPUBufferDesc* desc, const std::function<void(void*)>& init_callback, GPUBuffer* buffer, const GPUResource* alias, uint64_t alias_offset) const
	{
		auto internal_state = wi::allocator::make_shared<Buffer_Metal>();
		internal_state->allocationhandler = allocationhandler;
		buffer->internal_state = internal_state;
		buffer->type = GPUResource::Type::BUFFER;
		buffer->mapped_data = nullptr;
		buffer->mapped_size = 0;
		buffer->desc = *desc;
		
		const bool sparse = has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE);
		const bool aliasing_storage = has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_BUFFER) || has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_TEXTURE_NON_RT_DS) || has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_TEXTURE_RT_DS);
		
		MTL::ResourceOptions resource_options = {};
		if (desc->usage == Usage::DEFAULT)
		{
			if (aliasing_storage)
			{
				// potentially used for sparse or other private requirement resource
				resource_options |= MTL::ResourceStorageModePrivate;
			}
			else if (CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA))
			{
				resource_options |= MTL::ResourceStorageModeShared;
			}
			else
			{
				resource_options |= MTL::ResourceStorageModePrivate;
			}
		}
		else if (desc->usage == Usage::UPLOAD)
		{
			resource_options |= MTL::ResourceStorageModeShared;
			resource_options |= MTL::ResourceCPUCacheModeWriteCombined;
		}
		else if (desc->usage == Usage::READBACK)
		{
			resource_options |= MTL::ResourceStorageModeShared;
		}
		resource_options |= MTL::ResourceHazardTrackingModeUntracked;
		
		if (aliasing_storage)
		{
			// This is an aliasing storage:
			MTL::SizeAndAlign sizealign = device->heapBufferSizeAndAlign(desc->size, resource_options);
			NS::SharedPtr<MTL::HeapDescriptor> heap_desc = NS::TransferPtr(MTL::HeapDescriptor::alloc()->init());
			heap_desc->setResourceOptions(resource_options);
			heap_desc->setSize(sizealign.size);
			heap_desc->setType(MTL::HeapTypePlacement);
			heap_desc->setMaxCompatiblePlacementSparsePageSize(MTL::SparsePageSize64);
			NS::SharedPtr<MTL::Heap> heap = NS::TransferPtr(device->newHeap(heap_desc.get()));
			internal_state->buffer = NS::TransferPtr(heap->newBuffer(desc->size, resource_options, 0));
			internal_state->buffer->makeAliasable();
		}
		else if (alias != nullptr)
		{
			// This is an aliasing view:
			if (alias->IsBuffer())
			{
				auto alias_internal = to_internal<GPUBuffer>(alias);
				internal_state->buffer = NS::TransferPtr(alias_internal->buffer->heap()->newBuffer(desc->size, resource_options, alias_internal->buffer->heapOffset() + alias_offset));
			}
			else if (alias->IsTexture())
			{
				auto alias_internal = to_internal<Texture>(alias);
				internal_state->buffer = NS::TransferPtr(alias_internal->texture->heap()->newBuffer(desc->size, resource_options, alias_internal->texture->heapOffset() + alias_offset));
			}
		}
		else if (sparse)
		{
			// This is a placement sparse buffer:
			internal_state->buffer = NS::TransferPtr(device->newBuffer(desc->size, resource_options, MTL::SparsePageSize64));
		}
		else
		{
			// This is a standalone buffer:
			internal_state->buffer = NS::TransferPtr(device->newBuffer(desc->size, resource_options));
		}
		
		allocationhandler->make_resident(internal_state->buffer.get());
		internal_state->gpu_address = internal_state->buffer->gpuAddress();
		if ((resource_options & MTL::ResourceStorageModePrivate) == 0)
		{
			buffer->mapped_data = internal_state->buffer->contents();
			buffer->mapped_size = internal_state->buffer->allocatedSize();
		}
		
		if (init_callback)
		{
			if (buffer->mapped_data == nullptr)
			{
				NS::SharedPtr<MTL::Buffer> uploadbuffer = NS::TransferPtr(device->newBuffer(desc->size, MTL::ResourceStorageModeShared | MTL::ResourceOptionCPUCacheModeWriteCombined));
				init_callback(uploadbuffer->contents());
				NS::SharedPtr<NS::AutoreleasePool> autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init()); // scoped drain!
				NS::SharedPtr<MTL4::CommandBuffer> commandbuffer = NS::TransferPtr(device->newCommandBuffer());
				NS::SharedPtr<MTL4::CommandAllocator> commandallocator = NS::TransferPtr(device->newCommandAllocator());
				commandbuffer->beginCommandBuffer(commandallocator.get());
				allocationhandler->destroylocker.lock();
				allocationhandler->residency_set->addAllocation(uploadbuffer.get());
				allocationhandler->residency_set->commit();
				allocationhandler->destroylocker.unlock();
				MTL4::ComputeCommandEncoder* encoder = commandbuffer->computeCommandEncoder();
				encoder->copyFromBuffer(uploadbuffer.get(), 0, internal_state->buffer.get(), 0, desc->size);
				encoder->endEncoding();
				commandbuffer->endCommandBuffer();
				MTL4::CommandBuffer* cmds[] = {commandbuffer.get()};
				uploadqueue->commit(cmds, arraysize(cmds));
				NS::SharedPtr<MTL::SharedEvent> event = NS::TransferPtr(device->newSharedEvent());
				event->setSignaledValue(0);
				uploadqueue->signalEvent(event.get(), 1);
				event->waitUntilSignaledValue(1, ~0ull);
				allocationhandler->destroylocker.lock();
				allocationhandler->residency_set->removeAllocation(uploadbuffer.get());
				allocationhandler->destroylocker.unlock();
			}
			else
			{
				init_callback(buffer->mapped_data);
			}
		}
		
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
		
		return internal_state->buffer.get() != nullptr;
	}
	bool GraphicsDevice_Metal::CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture, const GPUResource* alias, uint64_t alias_offset) const
	{
		auto internal_state = wi::allocator::make_shared<Texture_Metal>();
		internal_state->allocationhandler = allocationhandler;
		texture->internal_state = internal_state;
		texture->type = GPUResource::Type::TEXTURE;
		texture->mapped_data = nullptr;
		texture->mapped_size = 0;
		texture->mapped_subresources = nullptr;
		texture->mapped_subresource_count = 0;
		texture->sparse_properties = nullptr;
		texture->desc = *desc;
		
		wilog_assert(!IsFormatBlockCompressed(desc->format) || device->supportsBCTextureCompression(), "Block compressed textures are not supported by this device!");
		
		const bool sparse = has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE);
		const bool aliasing_storage = has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_BUFFER) || has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_TEXTURE_NON_RT_DS) || has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_TEXTURE_RT_DS);
		
		if (texture->desc.mip_levels == 0)
		{
			texture->desc.mip_levels = GetMipCount(texture->desc.width, texture->desc.height, texture->desc.depth);
		}
		
		NS::SharedPtr<MTL::TextureDescriptor> descriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
		descriptor->setWidth(desc->width);
		descriptor->setHeight(desc->height);
		descriptor->setDepth(desc->depth);
		descriptor->setArrayLength(desc->array_size);
		descriptor->setMipmapLevelCount(desc->mip_levels);
		descriptor->setPixelFormat(_ConvertPixelFormat(desc->format));
		
		uint32_t sample_count = desc->sample_count;
		while (sample_count > 1 && !device->supportsTextureSampleCount(sample_count))
		{
			sample_count /= 2;
		}
		descriptor->setSampleCount(sample_count);
		texture->desc.sample_count = sample_count;
		
		switch (desc->type)
		{
			case TextureDesc::Type::TEXTURE_1D:
				descriptor->setTextureType(desc->array_size > 1 ? MTL::TextureType1DArray : MTL::TextureType1D);
				break;
			case TextureDesc::Type::TEXTURE_2D:
				if(desc->sample_count > 1)
				{
					descriptor->setTextureType(desc->array_size > 1 ? MTL::TextureType2DMultisampleArray : MTL::TextureType2DMultisample);
				}
				else
				{
					descriptor->setTextureType(desc->array_size > 1 ? MTL::TextureType2DArray : MTL::TextureType2D);
				}
				break;
			case TextureDesc::Type::TEXTURE_3D:
				descriptor->setTextureType(MTL::TextureType3D);
				break;
			default:
				break;
		}
		if (has_flag(desc->misc_flags, ResourceMiscFlag::TEXTURECUBE))
		{
			descriptor->setTextureType(desc->array_size > 6 ? MTL::TextureTypeCubeArray : MTL::TextureTypeCube);
			descriptor->setArrayLength(desc->array_size / 6);
		}
		
		MTL::ResourceOptions resource_options = {};
		if (has_flag(desc->misc_flags, ResourceMiscFlag::TRANSIENT_ATTACHMENT))
		{
			resource_options |= MTL::ResourceStorageModeMemoryless;
			descriptor->setStorageMode(MTL::StorageModeMemoryless);
		}
		if (desc->usage == Usage::DEFAULT && !CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA))
		{
			// Discrete GPU path:
			resource_options |= MTL::ResourceStorageModePrivate;
			descriptor->setStorageMode(MTL::StorageModePrivate);
		}
		else if (
				 sparse ||
				 has_flag(desc->bind_flags, BindFlag::RENDER_TARGET) ||
				 has_flag(desc->bind_flags, BindFlag::DEPTH_STENCIL) ||
				 has_flag(desc->bind_flags, BindFlag::UNORDERED_ACCESS)
				 )
		{
			// optimized storage for render efficiency even on UMA GPU:
			resource_options |= MTL::ResourceStorageModePrivate;
			descriptor->setStorageMode(MTL::StorageModePrivate);
		}
		else if (desc->usage == Usage::UPLOAD)
		{
			resource_options |= MTL::ResourceStorageModeShared;
			resource_options |= MTL::ResourceOptionCPUCacheModeWriteCombined;
			descriptor->setStorageMode(MTL::StorageModeShared);
			descriptor->setTextureType(MTL::TextureTypeTextureBuffer);
		}
		else if (desc->usage == Usage::READBACK)
		{
			resource_options |= MTL::ResourceStorageModeShared;
			descriptor->setStorageMode(MTL::StorageModeShared);
			descriptor->setTextureType(MTL::TextureTypeTextureBuffer);
		}
		else
		{
			// CPU accessible or UMA GPU zero-copy optimized:
			resource_options |= MTL::ResourceStorageModeShared;
			descriptor->setStorageMode(MTL::StorageModeShared);
		}
		resource_options |= MTL::ResourceHazardTrackingModeUntracked;
		descriptor->setResourceOptions(resource_options);
		
		MTL::TextureUsage usage = {};
		if (has_flag(desc->bind_flags, BindFlag::RENDER_TARGET) || has_flag(desc->bind_flags, BindFlag::DEPTH_STENCIL))
		{
			usage |= MTL::TextureUsageRenderTarget;
		}
		if (has_flag(desc->bind_flags, BindFlag::UNORDERED_ACCESS))
		{
			usage |= MTL::TextureUsageShaderWrite;
			switch (descriptor->pixelFormat())
			{
				case MTL::PixelFormatR32Uint:
				case MTL::PixelFormatR32Sint:
				case MTL::PixelFormatRG32Uint:
					usage |= MTL::TextureUsageShaderAtomic;
					break;
				default:
					break;
			}
		}
		if (has_flag(desc->bind_flags, BindFlag::SHADER_RESOURCE))
		{
			usage |= MTL::TextureUsageShaderRead;
		}
		descriptor->setUsage(usage);
		
		descriptor->setAllowGPUOptimizedContents(true);
		
		if (sparse)
		{
			constexpr MTL::SparsePageSize sparse_page_size = MTL::SparsePageSize64;
			descriptor->setPlacementSparsePageSize(sparse_page_size);
			texture->sparse_page_size = (uint32_t)device->sparseTileSizeInBytes(sparse_page_size);
			texture->sparse_properties = &internal_state->sparse_properties;
			const MTL::Size sparse_size = device->sparseTileSize(descriptor->textureType(), descriptor->pixelFormat(), descriptor->sampleCount(), sparse_page_size);
			internal_state->sparse_properties.tile_width = (uint32_t)sparse_size.width;
			internal_state->sparse_properties.tile_height = (uint32_t)sparse_size.height;
			internal_state->sparse_properties.tile_depth = (uint32_t)sparse_size.depth;
			const MTL::SizeAndAlign sizealign = device->heapTextureSizeAndAlign(descriptor.get());
			internal_state->sparse_properties.total_tile_count = uint32_t(sizealign.size / (uint64_t)texture->sparse_page_size);
		}
		
		if (!has_flag(desc->bind_flags, BindFlag::UNORDERED_ACCESS) && !has_flag(desc->bind_flags, BindFlag::RENDER_TARGET))
		{
			MTL::TextureSwizzleChannels swizzle = MTL::TextureSwizzleChannels::Default();
			swizzle.red = _ConvertComponentSwizzle(desc->swizzle.r);
			swizzle.green = _ConvertComponentSwizzle(desc->swizzle.g);
			swizzle.blue = _ConvertComponentSwizzle(desc->swizzle.b);
			swizzle.alpha = _ConvertComponentSwizzle(desc->swizzle.a);
			descriptor->setSwizzle(swizzle);
		}
		
		if (aliasing_storage)
		{
			// This is an aliasing storage:
			MTL::SizeAndAlign sizealign = device->heapTextureSizeAndAlign(descriptor.get());
			NS::SharedPtr<MTL::HeapDescriptor> heap_desc = NS::TransferPtr(MTL::HeapDescriptor::alloc()->init());
			heap_desc->setResourceOptions(resource_options);
			heap_desc->setSize(sizealign.size);
			heap_desc->setType(MTL::HeapTypePlacement);
			heap_desc->setMaxCompatiblePlacementSparsePageSize(MTL::SparsePageSize64);
			NS::SharedPtr<MTL::Heap> heap = NS::TransferPtr(device->newHeap(heap_desc.get()));
			internal_state->texture = NS::TransferPtr(heap->newTexture(descriptor.get(), 0));
			internal_state->texture->makeAliasable();
		}
		else if (alias != nullptr)
		{
			// This is an aliasing view:
			if (alias->IsBuffer())
			{
				auto alias_internal = to_internal<GPUBuffer>(alias);
				internal_state->texture = NS::TransferPtr(alias_internal->buffer->heap()->newTexture(descriptor.get(), alias_internal->buffer->heapOffset() + alias_offset));
			}
			else if (alias->IsTexture())
			{
				auto alias_internal = to_internal<Texture>(alias);
				internal_state->texture = NS::TransferPtr(alias_internal->texture->heap()->newTexture(descriptor.get(), alias_internal->texture->heapOffset() + alias_offset));
			}
		}
		else
		{
			if (desc->usage == Usage::READBACK || desc->usage == Usage::UPLOAD)
			{
				// Note: we are creating a buffer instead of linear image because linear image cannot have mips
				//	With a buffer, we can tightly pack mips linearly into a buffer so it won't have that limitation
				const size_t buffersize = ComputeTextureMemorySizeInBytes(*desc);
				internal_state->buffer = NS::TransferPtr(device->newBuffer(buffersize, resource_options));
				allocationhandler->make_resident(internal_state->buffer.get());
				
				texture->mapped_data = internal_state->buffer->contents();
				texture->mapped_size = buffersize;
				
				CreateTextureSubresourceDatas(*desc, texture->mapped_data, internal_state->mapped_subresources);
				texture->mapped_subresources = internal_state->mapped_subresources.data();
				texture->mapped_subresource_count = internal_state->mapped_subresources.size();
				assert(texture->mapped_subresources != nullptr);
			}
			else
			{
				internal_state->texture = NS::TransferPtr(device->newTexture(descriptor.get()));
			}
		}
		
		if (internal_state->texture.get() != nullptr)
		{
			allocationhandler->make_resident(internal_state->texture.get());
		}
		
		assert(internal_state->texture->isSparse() == sparse);
		
		if (initial_data != nullptr)
		{
			NS::SharedPtr<MTL4::CommandAllocator> commandallocator;
			NS::SharedPtr<MTL4::CommandBuffer> commandbuffer;
			MTL4::ComputeCommandEncoder* encoder = nullptr;
			NS::SharedPtr<MTL::Buffer> uploadbuffer;
			NS::SharedPtr<NS::AutoreleasePool> autorelease_pool; // scoped drain!
			uint8_t* upload_data = nullptr;
			if (internal_state->buffer.get() != nullptr)
			{
				// readback or upload, linear memory:
				upload_data = (uint8_t*)internal_state->buffer->contents();
			}
			else if (descriptor->storageMode() == MTL::StorageModePrivate)
			{
				autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
				uploadbuffer = NS::TransferPtr(device->newBuffer(internal_state->texture->allocatedSize(), MTL::ResourceStorageModeShared | MTL::ResourceOptionCPUCacheModeWriteCombined));
				upload_data = (uint8_t*)uploadbuffer->contents();
				commandallocator = NS::TransferPtr(device->newCommandAllocator());
				commandbuffer = NS::TransferPtr(device->newCommandBuffer());
				commandbuffer->beginCommandBuffer(commandallocator.get());
				allocationhandler->destroylocker.lock();
				allocationhandler->residency_set->addAllocation(uploadbuffer.get());
				allocationhandler->residency_set->commit();
				allocationhandler->destroylocker.unlock();
				encoder = commandbuffer->computeCommandEncoder();
			}
			
			const uint32_t data_stride = GetFormatStride(desc->format);
			uint32_t initDataIdx = 0;
			uint64_t src_offset = 0;
			for (uint32_t slice = 0; slice < desc->array_size; ++slice)
			{
				uint32_t width = desc->width;
				uint32_t height = desc->height;
				uint32_t depth = desc->depth;
				for (uint32_t mip = 0; mip < desc->mip_levels; ++mip)
				{
					const SubresourceData& subresourceData = initial_data[initDataIdx++];
					const uint32_t block_size = GetFormatBlockSize(desc->format);
					const uint32_t num_blocks_x = std::max(1u, width / block_size);
					const uint32_t num_blocks_y = std::max(1u, height / block_size);
					const uint64_t datasize = data_stride * num_blocks_x * num_blocks_y * depth;
					MTL::Region region = {};
					region.size.width = width;
					region.size.height = height;
					region.size.depth = depth;
					if (internal_state->buffer.get() != nullptr)
					{
						// readback or upload, linear memory:
						std::memcpy(upload_data + src_offset, subresourceData.data_ptr, datasize);
					}
					else if (descriptor->storageMode() == MTL::StorageModePrivate)
					{
						std::memcpy(upload_data + src_offset, subresourceData.data_ptr, datasize);
						MTL::Origin origin = {};
						MTL::Size size = {};
						size.width = width;
						size.height = height;
						size.depth = depth;
						encoder->copyFromBuffer(uploadbuffer.get(), src_offset, subresourceData.row_pitch, subresourceData.slice_pitch, size, internal_state->texture.get(), slice, mip, origin);
						width = std::max(1u, width / 2);
						height = std::max(1u, height / 2);
					}
					else
					{
						internal_state->texture->replaceRegion(region, mip, slice, subresourceData.data_ptr, subresourceData.row_pitch, datasize);
						width = std::max(block_size, width / 2);
						height = std::max(block_size, height / 2);
					}
					depth = std::max(1u, depth / 2);
					src_offset += datasize;
				}
			}
			
			if (commandbuffer.get() != nullptr)
			{
				encoder->endEncoding();
				commandbuffer->endCommandBuffer();
				MTL4::CommandBuffer* cmds[] = {commandbuffer.get()};
				uploadqueue->commit(cmds, arraysize(cmds));
				NS::SharedPtr<MTL::SharedEvent> event = NS::TransferPtr(device->newSharedEvent());
				event->setSignaledValue(0);
				uploadqueue->signalEvent(event.get(), 1);
				event->waitUntilSignaledValue(1, ~0ull);
				allocationhandler->destroylocker.lock();
				allocationhandler->residency_set->removeAllocation(uploadbuffer.get());
				allocationhandler->destroylocker.unlock();
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
		
		return internal_state->texture.get() != nullptr || internal_state->buffer.get() != nullptr;
	}
	bool GraphicsDevice_Metal::CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader) const
	{
		auto internal_state = wi::allocator::make_shared<Shader_Metal>();
		internal_state->allocationhandler = allocationhandler;
		shader->internal_state = internal_state;
		
		// The needs_draw_param or numthreads was gathered by offline shader reflection system in wiShaderCompiler.cpp and attached to the end of shadercode data:
		uint32_t reflection_append[3] = {};
		if (
			stage == ShaderStage::VS ||
			stage == ShaderStage::CS ||
			stage == ShaderStage::MS ||
			stage == ShaderStage::AS
			)
		{
			shadercode_size -= sizeof(reflection_append);
			std::memcpy(reflection_append, (uint8_t*)shadercode + shadercode_size, sizeof(reflection_append));
			if (stage == ShaderStage::VS)
			{
				internal_state->needs_draw_params = reflection_append[0] != 0;
			}
			else if (
				stage == ShaderStage::CS ||
				stage == ShaderStage::MS ||
				stage == ShaderStage::AS
				)
			{
				internal_state->numthreads = { reflection_append[0], reflection_append[1], reflection_append[2] };
			}
		}
		
		dispatch_data_t bytecodeData = dispatch_data_create(shadercode, shadercode_size, dispatch_get_main_queue(), nullptr);
		NS::Error* error = nullptr;
		internal_state->library = NS::TransferPtr(device->newLibrary(bytecodeData, &error));
		if(error != nullptr)
		{
			NS::String* errDesc = error->localizedDescription();
			wilog_assert(0, "%s", errDesc->utf8String());
			error->release();
		}
		assert(internal_state->library.get() != nullptr);
		
		NS::SharedPtr<NS::String> entry = NS::TransferPtr(NS::String::alloc()->init("main", NS::UTF8StringEncoding));
		NS::SharedPtr<MTL::FunctionConstantValues> constants = NS::TransferPtr(MTL::FunctionConstantValues::alloc()->init());
		
		if (stage == ShaderStage::HS || stage == ShaderStage::DS || stage == ShaderStage::GS)
			return false; // TODO
		//bool tessellationEnabled = false;
		//int vertex_shader_output_size_fc = 1024;
		//if (stage == ShaderStage::HS || stage == ShaderStage::DS || stage == ShaderStage::GS)
		//{
		//	constants->setConstantValue(&tessellationEnabled, MTL::DataTypeBool, (NS::UInteger)0);
		//	constants->setConstantValue(&vertex_shader_output_size_fc, MTL::DataTypeInt, (NS::UInteger)1);
		//}
		
		error = nullptr;
		internal_state->function = NS::TransferPtr(internal_state->library->newFunction(entry.get(), constants.get(), &error));
		if(error != nullptr)
		{
			NS::String* errDesc = error->localizedDescription();
			wilog_error("%s", errDesc->utf8String());
			assert(0);
			error->release();
		}
		assert(internal_state->function.get() != nullptr);
		
		if (stage == ShaderStage::CS)
		{
			error = nullptr;
			internal_state->compute_pipeline = NS::TransferPtr(device->newComputePipelineState(internal_state->function.get(), &error));
			if (error != nullptr)
			{
				NS::String* errDesc = error->localizedDescription();
				wilog_error("%s", errDesc->utf8String());
				assert(0);
				error->release();
			}
			
			return internal_state->compute_pipeline.get() != nullptr;
		}

		return internal_state->function.get() != nullptr;
	}
	bool GraphicsDevice_Metal::CreateSampler(const SamplerDesc* desc, Sampler* sampler) const
	{
		auto internal_state = wi::allocator::make_shared<Sampler_Metal>();
		internal_state->allocationhandler = allocationhandler;
		sampler->internal_state = internal_state;
		sampler->desc = *desc;
		
		NS::SharedPtr<MTL::SamplerDescriptor> descriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
		descriptor->setSupportArgumentBuffers(true);
		MTL::SamplerMipFilter mip_filter = {};
		switch (desc->filter)
		{
			case Filter::MIN_MAG_MIP_POINT:
			case Filter::MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::MIN_LINEAR_MAG_MIP_POINT:
			case Filter::MIN_MAG_LINEAR_MIP_POINT:
			case Filter::COMPARISON_MIN_MAG_MIP_POINT:
			case Filter::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			case Filter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
			case Filter::MINIMUM_MIN_MAG_MIP_POINT:
			case Filter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
			case Filter::MAXIMUM_MIN_MAG_MIP_POINT:
			case Filter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
				mip_filter = MTL::SamplerMipFilterNearest;
				break;
			default:
				mip_filter = MTL::SamplerMipFilterLinear;
				break;
		}
		descriptor->setMipFilter(mip_filter);
		MTL::SamplerMinMagFilter min_filter = {};
		switch (desc->filter)
		{
			case Filter::MIN_MAG_MIP_POINT:
			case Filter::MIN_MAG_POINT_MIP_LINEAR:
			case Filter::MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::MIN_POINT_MAG_MIP_LINEAR:
			case Filter::COMPARISON_MIN_MAG_MIP_POINT:
			case Filter::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			case Filter::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
			case Filter::MINIMUM_MIN_MAG_MIP_POINT:
			case Filter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
			case Filter::MAXIMUM_MIN_MAG_MIP_POINT:
			case Filter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
				min_filter = MTL::SamplerMinMagFilterNearest;
				break;
			default:
				min_filter = MTL::SamplerMinMagFilterLinear;
				break;
		}
		descriptor->setMinFilter(min_filter);
		MTL::SamplerMinMagFilter mag_filter = {};
		switch (desc->filter)
		{
			case Filter::MIN_MAG_MIP_POINT:
			case Filter::MIN_MAG_POINT_MIP_LINEAR:
			case Filter::MIN_LINEAR_MAG_MIP_POINT:
			case Filter::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter::COMPARISON_MIN_MAG_MIP_POINT:
			case Filter::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			case Filter::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			case Filter::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter::MINIMUM_MIN_MAG_MIP_POINT:
			case Filter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter::MAXIMUM_MIN_MAG_MIP_POINT:
			case Filter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
				mag_filter = MTL::SamplerMinMagFilterNearest;
				break;
			default:
				mag_filter = MTL::SamplerMinMagFilterLinear;
				break;
		}
		descriptor->setMagFilter(mag_filter);
		MTL::SamplerReductionMode reduction_mode = {};
		switch (desc->filter)
		{
			case Filter::MINIMUM_MIN_MAG_MIP_POINT:
			case Filter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
			case Filter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
			case Filter::MINIMUM_MIN_MAG_MIP_LINEAR:
			case Filter::MINIMUM_ANISOTROPIC:
				reduction_mode = MTL::SamplerReductionModeMinimum;
				break;
			case Filter::MAXIMUM_MIN_MAG_MIP_POINT:
			case Filter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
			case Filter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
			case Filter::MAXIMUM_MIN_MAG_MIP_LINEAR:
			case Filter::MAXIMUM_ANISOTROPIC:
				reduction_mode = MTL::SamplerReductionModeMaximum;
				break;
			default:
				reduction_mode = MTL::SamplerReductionModeWeightedAverage;
				break;
		}
		descriptor->setReductionMode(reduction_mode);
		descriptor->setCompareFunction(_ConvertCompareFunction(desc->comparison_func));
		descriptor->setLodMinClamp(desc->min_lod);
		descriptor->setLodMaxClamp(desc->max_lod);
		descriptor->setMaxAnisotropy(clamp(desc->max_anisotropy, 1u, 16u));
		descriptor->setLodBias(desc->mip_lod_bias);
		MTL::SamplerBorderColor border_color = {};
		switch (desc->border_color)
		{
			case SamplerBorderColor::TRANSPARENT_BLACK:
				border_color = MTL::SamplerBorderColorTransparentBlack;
				break;
			case SamplerBorderColor::OPAQUE_BLACK:
				border_color = MTL::SamplerBorderColorOpaqueBlack;
				break;
			case SamplerBorderColor::OPAQUE_WHITE:
				border_color = MTL::SamplerBorderColorOpaqueWhite;
				break;
		}
		descriptor->setBorderColor(border_color);
		descriptor->setSAddressMode(_ConvertAddressMode(desc->address_u));
		descriptor->setTAddressMode(_ConvertAddressMode(desc->address_v));
		descriptor->setRAddressMode(_ConvertAddressMode(desc->address_w));
		internal_state->sampler = NS::TransferPtr(device->newSamplerState(descriptor.get()));
		internal_state->index = allocationhandler->allocate_sampler_index();
		internal_state->entry = create_entry(internal_state->sampler.get());
		std::memcpy(descriptor_heap_sam_data + internal_state->index, &internal_state->entry, sizeof(internal_state->entry));

		return internal_state->sampler.get() != nullptr;
	}
	bool GraphicsDevice_Metal::CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const
	{
		auto internal_state = wi::allocator::make_shared<QueryHeap_Metal>();
		internal_state->allocationhandler = allocationhandler;
		queryheap->internal_state = internal_state;
		queryheap->desc = *desc;
		
		switch (desc->type)
		{
			case GpuQueryType::OCCLUSION:
			case GpuQueryType::OCCLUSION_BINARY:
				internal_state->buffer = NS::TransferPtr(device->newBuffer(desc->query_count * sizeof(uint64_t), MTL::ResourceStorageModePrivate));
				break;
				
			case GpuQueryType::TIMESTAMP:
			{
				NS::SharedPtr<MTL4::CounterHeapDescriptor> descriptor = NS::TransferPtr(MTL4::CounterHeapDescriptor::alloc()->init());
				descriptor->setType(MTL4::CounterHeapTypeTimestamp);
				descriptor->setCount(desc->query_count);
				NS::Error* error = nullptr;
				internal_state->counter_heap = NS::TransferPtr(device->newCounterHeap(descriptor.get(), &error));
				if (error != nullptr)
				{
					NS::String* errDesc = error->localizedDescription();
					wilog_assert(0, "%s", errDesc->utf8String());
					error->release();
				}
				internal_state->buffer = NS::TransferPtr(device->newBuffer(desc->query_count * sizeof(uint64_t), MTL::ResourceStorageModeShared));
			}
			break;
				
			default:
				break;
		}
		
		return internal_state->buffer.get() != nullptr;
	}
	bool GraphicsDevice_Metal::CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso, const RenderPassInfo* renderpass_info) const
	{
		auto internal_state = wi::allocator::make_shared<PipelineState_Metal>();
		internal_state->allocationhandler = allocationhandler;
		pso->internal_state = internal_state;
		pso->desc = *desc;
		
		internal_state->descriptor = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
		
		if (desc->vs != nullptr)
		{
			auto shader_internal = to_internal(desc->vs);
			internal_state->descriptor->setVertexFunction(shader_internal->function.get());
			internal_state->needs_draw_params = shader_internal->needs_draw_params;
		}
		if (desc->ps != nullptr)
		{
			auto shader_internal = to_internal(desc->ps);
			internal_state->descriptor->setFragmentFunction(shader_internal->function.get());
		}
		if (desc->ds != nullptr)
		{
			return false; // TODO
		}
		if (desc->hs != nullptr)
		{
			return false; // TODO
		}
		if (desc->gs != nullptr)
		{
			return false; // TODO
		}
		if (desc->as != nullptr)
		{
			return false; // TODO
		}
		if (desc->ms != nullptr)
		{
			return false; // TODO
		}
		
		NS::SharedPtr<MTL::VertexDescriptor> vertex_descriptor;
		if (desc->il != nullptr)
		{
			vertex_descriptor = NS::TransferPtr(MTL::VertexDescriptor::alloc()->init());
			// WARNING: the whole input slot strides are precalculated here at PSO creation based on formats. This doesn't support user supplied dynamic stride at draw time!
			uint64_t input_slot_strides[32] = {};
			for (size_t i = 0; i < desc->il->elements.size(); ++i)
			{
				const InputLayout::Element& element = desc->il->elements[i];
				input_slot_strides[element.input_slot] += GetFormatStride(element.format);
			}
			uint64_t offset = 0;
			for (size_t i = 0; i < desc->il->elements.size(); ++i)
			{
				const InputLayout::Element& element = desc->il->elements[i];
				const uint64_t element_stride = GetFormatStride(element.format);
				const uint64_t input_slot_stride = input_slot_strides[element.input_slot];
				MTL::VertexBufferLayoutDescriptor* layout = vertex_descriptor->layouts()->object(kIRVertexBufferBindPoint + i);
				layout->setStride(input_slot_stride);
				layout->setStepFunction(element.input_slot_class == InputClassification::PER_VERTEX_DATA ? MTL::VertexStepFunctionPerVertex : MTL::VertexStepFunctionPerInstance);
				layout->setStepRate(1);
				MTL::VertexAttributeDescriptor* attribute = vertex_descriptor->attributes()->object(kIRStageInAttributeStartIndex + i);
				attribute->setFormat(_ConvertVertexFormat(element.format));
				attribute->setOffset(element.aligned_byte_offset == InputLayout::APPEND_ALIGNED_ELEMENT ? offset : element.aligned_byte_offset);
				attribute->setBufferIndex(kIRVertexBufferBindPoint + i);
				if (element.aligned_byte_offset != InputLayout::APPEND_ALIGNED_ELEMENT)
				{
					offset = element.aligned_byte_offset;
				}
				offset += element_stride;
			}
			internal_state->descriptor->setVertexDescriptor(vertex_descriptor.get());
		}
		
		switch (desc->pt)
		{
			case PrimitiveTopology::TRIANGLELIST:
			case PrimitiveTopology::TRIANGLESTRIP:
			case PrimitiveTopology::PATCHLIST:
				internal_state->descriptor->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassTriangle);
				break;
			case PrimitiveTopology::LINELIST:
			case PrimitiveTopology::LINESTRIP:
				internal_state->descriptor->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassLine);
				break;
			case PrimitiveTopology::POINTLIST:
				internal_state->descriptor->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassPoint);
				break;
			default:
				break;
		}
		
		if (renderpass_info != nullptr)
		{
			// When renderpass_info is provided, it will be a completely precompiled pipeline state only useable by that renderpass type:
			NS::SharedPtr<MTL::RenderPipelineColorAttachmentDescriptor> attachments[8] = {
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
			};
			const BlendState& bs = desc->bs == nullptr ? BlendState() : *desc->bs;
			internal_state->descriptor->setAlphaToCoverageEnabled(bs.alpha_to_coverage_enable);
			for (uint32_t i = 0; i < renderpass_info->rt_count; ++i)
			{
				MTL::RenderPipelineColorAttachmentDescriptor& attachment = *attachments[i].get();
				attachment.setPixelFormat(_ConvertPixelFormat(renderpass_info->rt_formats[i]));
				
				const BlendState::RenderTargetBlendState& bs_rt = bs.render_target[i];
				MTL::ColorWriteMask color_write_mask = {};
				if (has_flag(bs_rt.render_target_write_mask, ColorWrite::ENABLE_RED))
				{
					color_write_mask |= MTL::ColorWriteMaskRed;
				}
				if (has_flag(bs_rt.render_target_write_mask, ColorWrite::ENABLE_GREEN))
				{
					color_write_mask |= MTL::ColorWriteMaskGreen;
				}
				if (has_flag(bs_rt.render_target_write_mask, ColorWrite::ENABLE_BLUE))
				{
					color_write_mask |= MTL::ColorWriteMaskBlue;
				}
				if (has_flag(bs_rt.render_target_write_mask, ColorWrite::ENABLE_ALPHA))
				{
					color_write_mask |= MTL::ColorWriteMaskAlpha;
				}
				attachment.setWriteMask(color_write_mask);
				attachment.setBlendingEnabled(bs_rt.blend_enable);
				attachment.setRgbBlendOperation(_ConvertBlendOp(bs_rt.blend_op));
				attachment.setAlphaBlendOperation(_ConvertBlendOp(bs_rt.blend_op_alpha));
				attachment.setSourceRGBBlendFactor(_ConvertBlendFactor(bs_rt.src_blend));
				attachment.setSourceAlphaBlendFactor(_ConvertBlendFactor(bs_rt.src_blend_alpha));
				attachment.setDestinationRGBBlendFactor(_ConvertBlendFactor(bs_rt.dest_blend));
				attachment.setDestinationAlphaBlendFactor(_ConvertBlendFactor(bs_rt.dest_blend_alpha));
				internal_state->descriptor->colorAttachments()->setObject(&attachment, i);
			}
			internal_state->descriptor->setDepthAttachmentPixelFormat(_ConvertPixelFormat(renderpass_info->ds_format));
			if (IsFormatStencilSupport(renderpass_info->ds_format))
			{
				internal_state->descriptor->setStencilAttachmentPixelFormat(_ConvertPixelFormat(renderpass_info->ds_format));
			}
			
			uint32_t sample_count = renderpass_info->sample_count;
			while (sample_count > 1 && !device->supportsTextureSampleCount(sample_count))
			{
				sample_count /= 2;
			}
			internal_state->descriptor->setSampleCount(sample_count);
			
			const DepthStencilState& dss = desc->dss == nullptr ? DepthStencilState() : *desc->dss;
			NS::SharedPtr<MTL::DepthStencilDescriptor> depth_stencil_desc = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
			if (dss.depth_enable && renderpass_info->ds_format != Format::UNKNOWN)
			{
				depth_stencil_desc->setDepthCompareFunction(_ConvertCompareFunction(dss.depth_func));
				depth_stencil_desc->setDepthWriteEnabled(dss.depth_write_mask == DepthWriteMask::ALL);
			}
			NS::SharedPtr<MTL::StencilDescriptor> stencil_front;
			NS::SharedPtr<MTL::StencilDescriptor> stencil_back;
			if (dss.stencil_enable && IsFormatStencilSupport(renderpass_info->ds_format))
			{
				stencil_front = NS::TransferPtr(MTL::StencilDescriptor::alloc()->init());
				stencil_back = NS::TransferPtr(MTL::StencilDescriptor::alloc()->init());
				stencil_front->setReadMask(dss.stencil_read_mask);
				stencil_front->setWriteMask(dss.stencil_write_mask);
				stencil_front->setStencilCompareFunction(_ConvertCompareFunction(dss.front_face.stencil_func));
				stencil_front->setStencilFailureOperation(_ConvertStencilOperation(dss.front_face.stencil_fail_op));
				stencil_front->setDepthFailureOperation(_ConvertStencilOperation(dss.front_face.stencil_depth_fail_op));
				stencil_front->setDepthStencilPassOperation(_ConvertStencilOperation(dss.front_face.stencil_pass_op));
				stencil_back->setReadMask(dss.stencil_read_mask);
				stencil_back->setWriteMask(dss.stencil_write_mask);
				stencil_back->setStencilCompareFunction(_ConvertCompareFunction(dss.back_face.stencil_func));
				stencil_back->setStencilFailureOperation(_ConvertStencilOperation(dss.back_face.stencil_fail_op));
				stencil_back->setDepthFailureOperation(_ConvertStencilOperation(dss.back_face.stencil_depth_fail_op));
				stencil_back->setDepthStencilPassOperation(_ConvertStencilOperation(dss.back_face.stencil_pass_op));
				depth_stencil_desc->setFrontFaceStencil(stencil_front.get());
				depth_stencil_desc->setBackFaceStencil(stencil_back.get());
			}
			internal_state->depth_stencil_state = NS::TransferPtr(device->newDepthStencilState(depth_stencil_desc.get()));
			
			
			NS::Error* error = nullptr;
			internal_state->render_pipeline = NS::TransferPtr(device->newRenderPipelineState(internal_state->descriptor.get(), &error));
			if (error != nullptr)
			{
				NS::String* errDesc = error->localizedDescription();
				wilog_assert(0, "%s", errDesc->utf8String());
				error->release();
			}
			
			return internal_state->render_pipeline.get() != nullptr;
		}
		
		// If we get here, this pipeline state is not complete, but it will be reuseable by different render passes (and compiled just in time at runtime)
		return true;
	}

	static NS::SharedPtr<MTL4::AccelerationStructureDescriptor> mtl_acceleration_structure_descriptor(const RaytracingAccelerationStructureDesc* desc)
	{
		NS::SharedPtr<MTL4::AccelerationStructureDescriptor> descriptor;
		
		NS::SharedPtr<NS::Array> object_array;
		wi::vector<NS::SharedPtr<MTL4::AccelerationStructureGeometryDescriptor>> geometry_descs;
		wi::vector<MTL4::AccelerationStructureGeometryDescriptor*> geometry_descs_raw;
		
		if (desc->type == RaytracingAccelerationStructureDesc::Type::TOPLEVEL)
		{
			NS::SharedPtr<MTL4::InstanceAccelerationStructureDescriptor> instance_descriptor = NS::TransferPtr(MTL4::InstanceAccelerationStructureDescriptor::alloc()->init());
			instance_descriptor->setInstanceCount(desc->top_level.count);
			auto buffer_internal = to_internal(&desc->top_level.instance_buffer);
			instance_descriptor->setInstanceDescriptorBuffer({buffer_internal->gpu_address + desc->top_level.offset, buffer_internal->buffer->length()});
			instance_descriptor->setInstanceDescriptorStride(sizeof(MTL::IndirectAccelerationStructureInstanceDescriptor));
			instance_descriptor->setInstanceDescriptorType(MTL::AccelerationStructureInstanceDescriptorTypeIndirect);
			instance_descriptor->setInstanceTransformationMatrixLayout(MTL::MatrixLayoutRowMajor);
			descriptor = instance_descriptor;
		}
		else
		{
			auto primitive_descriptor = NS::TransferPtr(MTL4::PrimitiveAccelerationStructureDescriptor::alloc()->init());
			geometry_descs.reserve(desc->bottom_level.geometries.size());
			
			for (auto& x : desc->bottom_level.geometries)
			{
				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES)
				{
					NS::SharedPtr<MTL4::AccelerationStructureTriangleGeometryDescriptor> geo = NS::TransferPtr(MTL4::AccelerationStructureTriangleGeometryDescriptor::alloc()->init());
					geometry_descs.push_back(geo);
					geo->setAllowDuplicateIntersectionFunctionInvocation((x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_NO_DUPLICATE_ANYHIT_INVOCATION) == 0);
					geo->setOpaque(x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE);
					auto ib_internal = to_internal(&x.triangles.index_buffer);
					auto vb_internal = to_internal(&x.triangles.vertex_buffer);
					geo->setIndexType(x.triangles.index_format == IndexBufferFormat::UINT32 ? MTL::IndexTypeUInt32 : MTL::IndexTypeUInt16);
					const uint64_t index_byteoffset = x.triangles.index_offset * GetIndexStride(x.triangles.index_format);
					geo->setIndexBuffer({ib_internal->gpu_address + index_byteoffset, ib_internal->buffer->length()});
					geo->setVertexFormat(_ConvertAttributeFormat(x.triangles.vertex_format));
					geo->setVertexBuffer({vb_internal->gpu_address + x.triangles.vertex_byte_offset, vb_internal->buffer->length()});
					geo->setVertexStride(x.triangles.vertex_stride);
					geo->setTriangleCount(x.triangles.index_count / 3);
					if (x.triangles.transform_3x4_buffer.IsValid())
					{
						auto transform_internal = to_internal(&x.triangles.transform_3x4_buffer);
						geo->setTransformationMatrixBuffer({transform_internal->gpu_address + x.triangles.transform_3x4_buffer_offset, transform_internal->buffer->length()});
						geo->setTransformationMatrixLayout(MTL::MatrixLayoutRowMajor);
					}
				}
				else
				{
					NS::SharedPtr<MTL4::AccelerationStructureBoundingBoxGeometryDescriptor> geo = NS::TransferPtr(MTL4::AccelerationStructureBoundingBoxGeometryDescriptor::alloc()->init());
					geometry_descs.push_back(geo);
					geo->setOpaque(x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE);
					geo->setBoundingBoxCount(x.aabbs.count);
					auto buffer_internal = to_internal(&x.aabbs.aabb_buffer);
					geo->setBoundingBoxBuffer({buffer_internal->gpu_address + x.aabbs.offset, buffer_internal->buffer->length()});
					geo->setBoundingBoxStride(x.aabbs.stride);
				}
			}
			
			geometry_descs_raw.reserve(geometry_descs.size());
			for (auto& x : geometry_descs)
			{
				geometry_descs_raw.push_back(x.get());
			}
			object_array = NS::TransferPtr(NS::Array::array((NS::Object**)geometry_descs_raw.data(), geometry_descs_raw.size())->retain());
			primitive_descriptor->setGeometryDescriptors(object_array.get());
			descriptor = primitive_descriptor;
		}
		
		MTL::AccelerationStructureUsage usage = MTL::AccelerationStructureUsageNone;
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
		{
			usage |= MTL::AccelerationStructureUsageRefit;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
		{
			usage |= MTL::AccelerationStructureUsagePreferFastBuild;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
		{
			usage |= MTL::AccelerationStructureUsagePreferFastIntersection;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
		{
			usage |= MTL::AccelerationStructureUsageMinimizeMemory;
		}
		descriptor->setUsage(usage);
		
		return descriptor;
	}
	bool GraphicsDevice_Metal::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const
	{
		auto internal_state = wi::allocator::make_shared<BVH_Metal>();
		internal_state->allocationhandler = allocationhandler;
		bvh->internal_state = internal_state;
		bvh->type = GPUResource::Type::RAYTRACING_ACCELERATION_STRUCTURE;
		bvh->desc = *desc;
		bvh->size = 0;
		
		NS::SharedPtr<MTL4::AccelerationStructureDescriptor> descriptor = mtl_acceleration_structure_descriptor(desc);
		
		const MTL::AccelerationStructureSizes size = device->accelerationStructureSizes(descriptor.get());
		internal_state->acceleration_structure = NS::TransferPtr(device->newAccelerationStructure(size.accelerationStructureSize));
		internal_state->scratch = NS::TransferPtr(device->newBuffer(std::max(size.buildScratchBufferSize, size.refitScratchBufferSize), MTL::ResourceStorageModePrivate));

		allocationhandler->make_resident(internal_state->acceleration_structure.get());
		allocationhandler->make_resident(internal_state->scratch.get());
		
		internal_state->resourceid = internal_state->acceleration_structure->gpuResourceID();
		
		if (desc->type == RaytracingAccelerationStructureDesc::Type::TOPLEVEL)
		{
			internal_state->tlas_header_instancecontributions = NS::TransferPtr(device->newBuffer(sizeof(IRRaytracingAccelerationStructureGPUHeader) + sizeof(uint32_t) * desc->top_level.count, MTL::ResourceStorageModeShared));
			allocationhandler->make_resident(internal_state->tlas_header_instancecontributions.get());
			wi::vector<uint32_t> instance_contributions(desc->top_level.count);
			std::fill(instance_contributions.begin(), instance_contributions.end(), 0);
			IRRaytracingAccelerationStructureGPUHeader* header = (IRRaytracingAccelerationStructureGPUHeader*)internal_state->tlas_header_instancecontributions->contents();
			uint8_t* instancecontributions_gpudata = (uint8_t*)header + sizeof(IRRaytracingAccelerationStructureGPUHeader);
			MTL::GPUAddress header_gpuaddress = internal_state->tlas_header_instancecontributions->gpuAddress();
			MTL::GPUAddress instancecontributions_gpuaddress = header_gpuaddress + sizeof(IRRaytracingAccelerationStructureGPUHeader);
			IRRaytracingSetAccelerationStructure((uint8_t*)header, internal_state->resourceid, instancecontributions_gpudata, instancecontributions_gpuaddress, instance_contributions.data(), desc->top_level.count);
			IRDescriptorTableSetAccelerationStructure(&internal_state->tlas_entry, header_gpuaddress);
			internal_state->tlas_descriptor_index = allocationhandler->allocate_resource_index();
			std::memcpy(descriptor_heap_res_data + internal_state->tlas_descriptor_index, &internal_state->tlas_entry, sizeof(internal_state->tlas_entry));
		}
		
		return internal_state->acceleration_structure.get() != nullptr;
	}
	bool GraphicsDevice_Metal::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const
	{
		//auto internal_state = wi::allocator::make_shared<RTPipelineState_Metal>();
		//internal_state->allocationhandler = allocationhandler;
		//rtpso->internal_state = internal_state;
		//rtpso->desc = *desc;

		return false;
	}
	bool GraphicsDevice_Metal::CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const
	{
		//auto internal_state = wi::allocator::make_shared<VideoDecoder_Metal>();
		//internal_state->allocationhandler = allocationhandler;
		//video_decoder->internal_state = internal_state;
		//video_decoder->desc = *desc;

		return false;
	}

	int GraphicsDevice_Metal::CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change, const ImageAspect* aspect, const Swizzle* swizzle, float min_lod_clamp) const
	{
		auto internal_state = to_internal(texture);

		Format format = texture->GetDesc().format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}
		
		const MTL::PixelFormat pixelformat = _ConvertPixelFormat(format);
		assert(pixelformat != MTL::PixelFormatInvalid);
		
		sliceCount = std::min(sliceCount, texture->desc.array_size - firstSlice);
		mipCount = std::min(mipCount, texture->desc.mip_levels - firstMip);
		
		MTL::TextureType texture_type = internal_state->texture->textureType();
		if (type != SubresourceType::SRV &&
			(texture_type == MTL::TextureTypeCube || texture_type == MTL::TextureTypeCubeArray)
			)
		{
			texture_type = MTL::TextureType2DArray;
		}
		
		NS::SharedPtr<MTL::TextureViewDescriptor> view_desc;
		
		switch (type) {
			case SubresourceType::SRV:
			case SubresourceType::UAV:
				view_desc = NS::TransferPtr(MTL::TextureViewDescriptor::alloc()->init());
				view_desc->setLevelRange({firstMip, mipCount});
				view_desc->setSliceRange({firstSlice, sliceCount});
				view_desc->setPixelFormat(pixelformat);
				view_desc->setTextureType(texture_type);
				break;
			default:
				break;
		};
		
		switch (type) {
			case SubresourceType::SRV:
				{
					MTL::TextureSwizzleChannels mtlswizzle = MTL::TextureSwizzleChannels::Default();
					Swizzle requested_swizzle = texture->desc.swizzle;
					if (swizzle != nullptr)
					{
						requested_swizzle = *swizzle;
					}
					mtlswizzle.red = _ConvertComponentSwizzle(requested_swizzle.r);
					mtlswizzle.green = _ConvertComponentSwizzle(requested_swizzle.g);
					mtlswizzle.blue = _ConvertComponentSwizzle(requested_swizzle.b);
					mtlswizzle.alpha = _ConvertComponentSwizzle(requested_swizzle.a);
					view_desc->setSwizzle(mtlswizzle);
					if (!internal_state->srv.IsValid())
					{
						auto& subresource = internal_state->srv;
						subresource.index = allocationhandler->allocate_resource_index();
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = texture_view_pool->setTextureView(internal_state->texture.get(), view_desc.get(), subresource.index);
						subresource.entry = create_entry(resourceID, min_lod_clamp);
#else
						subresource.view = NS::TransferPtr(internal_state->texture->newTextureView(view_desc.get()));
						subresource.entry = create_entry(subresource.view.get());
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_srv.emplace_back();
						subresource.index = allocationhandler->allocate_resource_index();
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = texture_view_pool->setTextureView(internal_state->texture.get(), view_desc.get(), subresource.index);
						subresource.entry = create_entry(resourceID, min_lod_clamp);
#else
						subresource.view = NS::TransferPtr(internal_state->texture->newTextureView(view_desc.get()));
						subresource.entry = create_entry(subresource.view.get());
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return (int)internal_state->subresources_srv.size() - 1;
					}
				}
				break;
				
			case SubresourceType::UAV:
				{
					if (!internal_state->uav.IsValid())
					{
						auto& subresource = internal_state->uav;
						subresource.index = allocationhandler->allocate_resource_index();
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = texture_view_pool->setTextureView(internal_state->texture.get(), view_desc.get(), subresource.index);
						subresource.entry = create_entry(resourceID, min_lod_clamp);
#else
						subresource.view = NS::TransferPtr(internal_state->texture->newTextureView(view_desc.get()));
						subresource.entry = create_entry(subresource.view.get());
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_uav.emplace_back();
						subresource.index = allocationhandler->allocate_resource_index();
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = texture_view_pool->setTextureView(internal_state->texture.get(), view_desc.get(), subresource.index);
						subresource.entry = create_entry(resourceID, min_lod_clamp);
#else
						subresource.view = NS::TransferPtr(internal_state->texture->newTextureView(view_desc.get()));
						subresource.entry = create_entry(subresource.view.get());
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return (int)internal_state->subresources_uav.size() - 1;
					}
				}
				break;
				
			case SubresourceType::RTV:
				{
					if (!internal_state->rtv.IsValid())
					{
						auto& subresource = internal_state->rtv;
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_rtv.emplace_back();
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return (int)internal_state->subresources_rtv.size() - 1;
					}
				}
				break;
				
			case SubresourceType::DSV:
				{
					if (!internal_state->dsv.IsValid())
					{
						auto& subresource = internal_state->dsv;
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_dsv.emplace_back();
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return (int)internal_state->subresources_dsv.size() - 1;
					}
				}
				break;
				
			default:
				break;
		}

		return -1;
	}
	int GraphicsDevice_Metal::CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size, const Format* format_change, const uint32_t* structuredbuffer_stride_change) const
	{
		auto internal_state = to_internal(buffer);
		NS::SharedPtr<MTL::TextureDescriptor> texture_descriptor;
		
		size = std::min(size, buffer->desc.size - offset);
		
		Format format = buffer->GetDesc().format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}
		
		const bool structured = has_flag(buffer->desc.misc_flags, ResourceMiscFlag::BUFFER_STRUCTURED) || (structuredbuffer_stride_change != nullptr);
		
		const MTL::PixelFormat pixelformat = _ConvertPixelFormat(format);
		if (pixelformat != MTL::PixelFormatInvalid)
		{
			texture_descriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
			texture_descriptor->setPixelFormat(pixelformat);
			texture_descriptor->setTextureType(MTL::TextureTypeTextureBuffer);
			texture_descriptor->setResourceOptions(internal_state->buffer->resourceOptions());
			texture_descriptor->setWidth(size / GetFormatStride(format));
			MTL::TextureUsage texture_usage = MTL::TextureUsageShaderRead | MTL::TextureUsagePixelFormatView;
			if (type == SubresourceType::UAV)
			{
				texture_usage |= MTL::TextureUsageShaderWrite;
				switch (texture_descriptor->pixelFormat())
				{
					case MTL::PixelFormatR32Uint:
					case MTL::PixelFormatR32Sint:
					case MTL::PixelFormatRG32Uint:
						texture_usage |= MTL::TextureUsageShaderAtomic;
						break;
					default:
						break;
				}
			}
			texture_descriptor->setUsage(texture_usage);
		}
		
		switch (type) {
			case SubresourceType::SRV:
				{
					if (!internal_state->srv.IsValid())
					{
						auto& subresource = internal_state->srv;
						subresource.index = allocationhandler->allocate_resource_index();
						subresource.offset = offset;
						subresource.size = size;
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = {};
						if (texture_descriptor.get() != nullptr)
						{
							resourceID = texture_view_pool->setTextureViewFromBuffer(internal_state->buffer.get(), texture_descriptor.get(), offset, size, subresource.index);
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, resourceID, format, structured);
#else
						if (texture_descriptor.get() != nullptr)
						{
							subresource.view = NS::TransferPtr(internal_state->buffer->newTexture(texture_descriptor.get(), offset, size));
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, subresource.view.get(), format, structured);
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_srv.emplace_back();
						subresource.index = allocationhandler->allocate_resource_index();
						subresource.offset = offset;
						subresource.size = size;
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = {};
						if (texture_descriptor.get() != nullptr)
						{
							resourceID = texture_view_pool->setTextureViewFromBuffer(internal_state->buffer.get(), texture_descriptor.get(), offset, size, subresource.index);
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, resourceID, format, structured);
#else
						if (texture_descriptor.get() != nullptr)
						{
							subresource.view = NS::TransferPtr(internal_state->buffer->newTexture(texture_descriptor.get(), offset, size));
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, subresource.view.get(), format, structured);
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						return (int)internal_state->subresources_srv.size() - 1;
					}
				}
				break;
			case SubresourceType::UAV:
				{
					if (!internal_state->uav.IsValid())
					{
						auto& subresource = internal_state->uav;
						subresource.index = allocationhandler->allocate_resource_index();
						subresource.offset = offset;
						subresource.size = size;
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = {};
						if (texture_descriptor.get() != nullptr)
						{
							resourceID = texture_view_pool->setTextureViewFromBuffer(internal_state->buffer.get(), texture_descriptor.get(), offset, size, subresource.index);
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, resourceID, format, structured);
#else
						if (texture_descriptor.get() != nullptr)
						{
							subresource.view = NS::TransferPtr(internal_state->buffer->newTexture(texture_descriptor.get(), offset, size));
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, subresource.view.get(), format, structured);
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_uav.emplace_back();
						subresource.index = allocationhandler->allocate_resource_index();
						subresource.offset = offset;
						subresource.size = size;
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = {};
						if (texture_descriptor.get() != nullptr)
						{
							resourceID = texture_view_pool->setTextureViewFromBuffer(internal_state->buffer.get(), texture_descriptor.get(), offset, size, subresource.index);
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, resourceID, format, structured);
#else
						if (texture_descriptor.get() != nullptr)
						{
							subresource.view = NS::TransferPtr(internal_state->buffer->newTexture(texture_descriptor.get(), offset, size));
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, subresource.view.get(), format, structured);
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						return (int)internal_state->subresources_uav.size() - 1;
					}
				}
				break;
			default:
				break;
		}
		
		return -1;
	}

	void GraphicsDevice_Metal::DeleteSubresources(GPUResource* resource)
	{
		std::scoped_lock lck(allocationhandler->destroylocker);
		if (resource->IsTexture())
		{
			auto internal_state = to_internal<Texture>(resource);
			internal_state->destroy_subresources();
		}
		else if (resource->IsBuffer())
		{
			auto internal_state = to_internal<GPUBuffer>(resource);
			internal_state->destroy_subresources();
		}
	}

	int GraphicsDevice_Metal::GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource) const
	{
		if (resource == nullptr || !resource->IsValid())
			return -1;

		if (resource->IsTexture())
		{
			auto internal_state = to_internal<Texture>(resource);
			switch (type)
			{
				case SubresourceType::SRV:
					return subresource < 0 ? internal_state->srv.index : internal_state->subresources_srv[subresource].index;
				case SubresourceType::UAV:
					return subresource < 0 ? internal_state->uav.index : internal_state->subresources_uav[subresource].index;
				default:
					break;
			}
		}
		else if (resource->IsBuffer())
		{
			auto internal_state = to_internal<GPUBuffer>(resource);
			switch (type)
			{
				case SubresourceType::SRV:
					return subresource < 0 ? internal_state->srv.index : internal_state->subresources_srv[subresource].index;
				case SubresourceType::UAV:
					return subresource < 0 ? internal_state->uav.index : internal_state->subresources_uav[subresource].index;
				default:
					break;
			}
		}
		else if (resource->IsAccelerationStructure())
		{
			auto internal_state = to_internal<RaytracingAccelerationStructure>(resource);
			return internal_state->tlas_descriptor_index;
		}
		return -1;
	}
	int GraphicsDevice_Metal::GetDescriptorIndex(const Sampler* sampler) const
	{
		if (sampler == nullptr || !sampler->IsValid())
			return -1;
		
		auto internal_state = to_internal(sampler);
		return internal_state->index;
	}

	void GraphicsDevice_Metal::WriteShadingRateValue(ShadingRate rate, void* dest) const
	{
	}
	void GraphicsDevice_Metal::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const
	{
		MTL::IndirectAccelerationStructureInstanceDescriptor descriptor = {};
		descriptor.options = MTL::AccelerationStructureInstanceOptionNone;
		if (instance == nullptr)
		{
			descriptor.accelerationStructureID = dummyblas_resourceid;
		}
		else
		{
			if (instance->flags & RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE)
			{
				descriptor.options |= MTL::AccelerationStructureInstanceOptionDisableTriangleCulling;
			}
			if (instance->flags & RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE)
			{
				descriptor.options |= MTL::AccelerationStructureInstanceOptionTriangleFrontFacingWindingCounterClockwise;
			}
			if (instance->flags & RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_FORCE_OPAQUE)
			{
				descriptor.options |= MTL::AccelerationStructureInstanceOptionOpaque;
			}
			if (instance->flags & RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_FORCE_NON_OPAQUE)
			{
				descriptor.options |= MTL::AccelerationStructureInstanceOptionNonOpaque;
			}
			descriptor.mask = instance->instance_mask;
			descriptor.intersectionFunctionTableOffset = 0;
			auto blas_internal = to_internal<RaytracingAccelerationStructure>(instance->bottom_level);
			descriptor.accelerationStructureID = blas_internal->resourceid;
#if 1
			// The top level descriptor can specify row major layout now like directx:
			std::memcpy(&descriptor.transformationMatrix, instance->transform, sizeof(descriptor.transformationMatrix));
#else
			// Conversion from row major to column major:
			descriptor.transformationMatrix.columns[0][0] = instance->transform[0][0];
			descriptor.transformationMatrix.columns[0][1] = instance->transform[1][0];
			descriptor.transformationMatrix.columns[0][2] = instance->transform[2][0];
			descriptor.transformationMatrix.columns[1][0] = instance->transform[0][1];
			descriptor.transformationMatrix.columns[1][1] = instance->transform[1][1];
			descriptor.transformationMatrix.columns[1][2] = instance->transform[2][1];
			descriptor.transformationMatrix.columns[2][0] = instance->transform[0][2];
			descriptor.transformationMatrix.columns[2][1] = instance->transform[1][2];
			descriptor.transformationMatrix.columns[2][2] = instance->transform[2][2];
			descriptor.transformationMatrix.columns[3][0] = instance->transform[0][3];
			descriptor.transformationMatrix.columns[3][1] = instance->transform[1][3];
			descriptor.transformationMatrix.columns[3][2] = instance->transform[2][3];
#endif
		}
		std::memcpy(dest, &descriptor, sizeof(descriptor)); // force memcpy into potentially write combined cache memory
	}
	void GraphicsDevice_Metal::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const
	{
	}

	void GraphicsDevice_Metal::SetName(GPUResource* pResource, const char* name) const
	{
		if (pResource == nullptr || !pResource->IsValid())
			return;
		NS::SharedPtr<NS::String> str = NS::TransferPtr(NS::String::alloc()->init(name, NS::UTF8StringEncoding));
		if (pResource->IsTexture())
		{
			auto internal_state = to_internal<Texture>(pResource);
			internal_state->texture->setLabel(str.get());
		}
		else if (pResource->IsBuffer())
		{
			auto internal_state = to_internal<GPUBuffer>(pResource);
			internal_state->buffer->setLabel(str.get());
		}
		else if (pResource->IsAccelerationStructure())
		{
			auto internal_state = to_internal<RaytracingAccelerationStructure>(pResource);
			internal_state->acceleration_structure->setLabel(str.get());
		}
	}
	void GraphicsDevice_Metal::SetName(Shader* shader, const char* name) const
	{
		if (shader == nullptr || !shader->IsValid())
			return;
		NS::SharedPtr<NS::String> str = NS::TransferPtr(NS::String::alloc()->init(name, NS::UTF8StringEncoding));
		auto internal_state = to_internal(shader);
		internal_state->library->setLabel(str.get());
	}

	CommandList GraphicsDevice_Metal::BeginCommandList(QUEUE_TYPE queue)
	{
		cmd_locker.lock();
		uint32_t cmd_current = cmd_count++;
		if (cmd_current >= commandlists.size())
		{
			commandlists.push_back(std::make_unique<CommandList_Metal>());
			auto& commandlist = commandlists.back();
			commandlist->commandbuffer = NS::TransferPtr(device->newCommandBuffer());
			for (auto& x : commandlist->commandallocators)
			{
				x = NS::TransferPtr(device->newCommandAllocator());
			}
			NS::Error* error = nullptr;
			commandlist->argument_table = NS::TransferPtr(device->newArgumentTable(argument_table_desc.get(), &error));
			if (error != nullptr)
			{
				NS::String* errDesc = error->localizedDescription();
				wilog_assert(0, "%s", errDesc->utf8String());
				error->release();
			}
		}
		CommandList cmd;
		cmd.internal_state = commandlists[cmd_current].get();
		cmd_locker.unlock();

		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.reset(GetBufferIndex());
		commandlist.queue = queue;
		commandlist.id = cmd_current;
		commandlist.commandallocators[GetBufferIndex()]->reset();
		commandlist.commandbuffer->beginCommandBuffer(commandlist.commandallocators[GetBufferIndex()].get());
		commandlist.argument_table->setAddress(descriptor_heap_res->gpuAddress(), kIRDescriptorHeapBindPoint);
		commandlist.argument_table->setAddress(descriptor_heap_sam->gpuAddress(), kIRSamplerHeapBindPoint);
		
		return cmd;
	}
	void GraphicsDevice_Metal::SubmitCommandLists()
{
		allocationhandler->destroylocker.lock();
		allocationhandler->residency_set->commit();
		allocationhandler->destroylocker.unlock();
		
		uint32_t cmd_last = cmd_count;
		cmd_count = 0;
		
		// Presents wait:
		for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
		{
			auto& commandlist = *commandlists[cmd].get();
			for (auto& x : commandlist.presents)
			{
				CommandQueue& queue = queues[commandlist.queue];
				queue.queue->wait(x.get());
			}
		}
		
		// Submit work and resolve dependencies:
		for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
		{
			auto& commandlist = *commandlists[cmd].get();
			if (!commandlist.barriers.empty())
			{
				if (commandlist.compute_encoder == nullptr)
				{
					commandlist.compute_encoder = commandlist.commandbuffer->computeCommandEncoder();
				}
				commandlist.compute_encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, MTL4::VisibilityOptionNone);
				commandlist.barriers.clear();
			}
			if (commandlist.compute_encoder != nullptr)
			{
				commandlist.compute_encoder->endEncoding();
				commandlist.compute_encoder = nullptr;
			}
			commandlist.commandbuffer->endCommandBuffer();
			
			CommandQueue& queue = queues[commandlist.queue];
			const bool dependency = !commandlist.signals.empty() || !commandlist.waits.empty();
			if (dependency)
			{
				queue.submit();
			}
			
			queue.submit_cmds.push_back(commandlist.commandbuffer.get());
			
			if (dependency)
			{
				for (auto& semaphore : commandlist.waits)
				{
					queue.wait(semaphore);
					// recycle semaphore only in signals
				}
				commandlist.waits.clear();
				
				queue.submit();
				
				for (auto& semaphore : commandlist.signals)
				{
					queue.signal(semaphore);
					free_semaphore(semaphore);
				}
				commandlist.signals.clear();
			}
			
			// Worker pipelines are merged in to global:
			for (auto& x : commandlist.pipelines_worker)
			{
				if (pipelines_global.count(x.first) == 0)
				{
					pipelines_global[x.first] = std::move(x.second);
				}
				else
				{
					allocationhandler->destroylocker.lock();
					allocationhandler->destroyer_render_pipelines.push_back(std::make_pair(x.second.pipeline, allocationhandler->framecount));
					allocationhandler->destroyer_depth_stencil_states.push_back(std::make_pair(x.second.depth_stencil_state, allocationhandler->framecount));
					allocationhandler->destroylocker.unlock();
				}
			}
			commandlist.pipelines_worker.clear();
		}
		
		// Mark the completion of queues for this frame:
		frame_fence_values[GetBufferIndex()]++;
		for (int q = 0; q < QUEUE_COUNT; ++q)
		{
			CommandQueue& queue = queues[q];
			if (queue.queue.get() == nullptr)
				continue;
			
			queue.submit();
			
			queue.queue->signalEvent(frame_fence[GetBufferIndex()][q].get(), frame_fence_values[GetBufferIndex()]);
		}
		
		// Presents submit:
		for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
		{
			auto& commandlist = *commandlists[cmd].get();
			for (auto& x : commandlist.presents)
			{
				CommandQueue& queue = queues[commandlist.queue];
				queue.queue->signalDrawable(x.get());
				x->present();
			}
		}
		
		// Sync up every queue to every other queue at the end of the frame:
		//	Note: it disables overlapping queues into the next frame
		for (int queue1 = 0; queue1 < QUEUE_COUNT; ++queue1)
		{
			if (queues[queue1].queue.get() == nullptr)
				continue;
			for (int queue2 = 0; queue2 < QUEUE_COUNT; ++queue2)
			{
				if (queue1 == queue2)
					continue;
				if (queues[queue2].queue.get() == nullptr)
					continue;
				MTL::SharedEvent* fence = frame_fence[GetBufferIndex()][queue2].get();
				queues[queue1].queue->wait(fence, frame_fence_values[GetBufferIndex()]);
			}
		}
		
		// From here, we begin a new frame, this affects GetBufferIndex()!
		FRAMECOUNT++;
		
		// Initiate stalling CPU when GPU is not yet finished with next frame:
		const uint32_t bufferindex = GetBufferIndex();
		for (int queue = 0; queue < QUEUE_COUNT; ++queue)
		{
			if (queues[queue].queue.get() == nullptr)
				continue;
			MTL::SharedEvent* fence = frame_fence[bufferindex][queue].get();
			if (fence->signaledValue() < frame_fence_values[GetBufferIndex()])
			{
				fence->waitUntilSignaledValue(frame_fence_values[GetBufferIndex()], ~0ull);
			}
		}
		
		allocationhandler->Update(FRAMECOUNT, BUFFERCOUNT);
	}

	void GraphicsDevice_Metal::WaitForGPU() const
	{
		NS::SharedPtr<MTL::SharedEvent> event = NS::TransferPtr(device->newSharedEvent());
		for (auto& queue : queues)
		{
			if (queue.queue.get() == nullptr)
				continue;
			event->setSignaledValue(0);
			queue.queue->signalEvent(event.get(), 1);
			event->waitUntilSignaledValue(1, ~0ull);
		}
	}
	void GraphicsDevice_Metal::ClearPipelineStateCache()
	{
		std::scoped_lock locker(allocationhandler->destroylocker);
		uint64_t framecount = allocationhandler->framecount;
		for (auto& x : pipelines_global)
		{
			allocationhandler->destroyer_render_pipelines.push_back(std::make_pair(std::move(x.second.pipeline), framecount));
			allocationhandler->destroyer_depth_stencil_states.push_back(std::make_pair(std::move(x.second.depth_stencil_state), framecount));
		}
		pipelines_global.clear();
	}

	Texture GraphicsDevice_Metal::GetBackBuffer(const SwapChain* swapchain) const
	{
		auto swapchain_internal = to_internal(swapchain);
		Texture result;
		result.type = GPUResource::Type::TEXTURE;
		auto texture_internal = wi::allocator::make_shared<Texture_Metal>();
		texture_internal->texture = swapchain_internal->current_texture;
		texture_internal->srv.index = allocationhandler->allocate_resource_index();
		texture_internal->srv.entry = create_entry(texture_internal->texture.get());
		std::memcpy(descriptor_heap_res_data + texture_internal->srv.index, &texture_internal->srv.entry, sizeof(texture_internal->srv.entry));
		result.internal_state = texture_internal;
		result.desc.width = (uint32_t)texture_internal->texture->width();
		result.desc.height = (uint32_t)texture_internal->texture->height();
		result.desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::RENDER_TARGET;
		result.desc.format = swapchain->desc.format;
		return result;
	}
	ColorSpace GraphicsDevice_Metal::GetSwapChainColorSpace(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);
		return internal_state->colorSpace;
	}
	bool GraphicsDevice_Metal::IsSwapChainSupportsHDR(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);
		// TODO
		return false;
	}

	void GraphicsDevice_Metal::SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count)
	{
		MTL4::CommandQueue* commandqueue = queues[queue].queue.get();
		for (uint32_t i = 0; i < command_count; ++i)
		{
			const SparseUpdateCommand& command = commands[i];
			auto tilepool_internal = to_internal<GPUBuffer>(command.tile_pool);
			MTL::Heap* heap = tilepool_internal->buffer->heap();
			assert(heap != nullptr);
			
			if (command.sparse_resource->IsTexture())
			{
				auto sparse_internal = to_internal<Texture>(command.sparse_resource);
				wi::vector<MTL4::UpdateSparseTextureMappingOperation> operations;
				operations.reserve(command.num_resource_regions);
				for (uint32_t j = 0; j < command.num_resource_regions; ++j)
				{
					const SparseResourceCoordinate& coordinate = command.coordinates[j];
					const SparseRegionSize& size = command.sizes[j];
					auto& op = operations.emplace_back();
					op.mode = command.range_flags[j] == TileRangeFlags::Null ? MTL::SparseTextureMappingModeUnmap : MTL::SparseTextureMappingModeMap;
					op.heapOffset = command.range_start_offsets[j];
					op.textureLevel = coordinate.mip;
					op.textureSlice = coordinate.slice;
					op.textureRegion.origin.x = coordinate.x;
					op.textureRegion.origin.y = coordinate.y;
					op.textureRegion.origin.z = coordinate.z;
					op.textureRegion.size.width = size.width;
					op.textureRegion.size.height = size.height;
					op.textureRegion.size.depth = size.depth;
				}
				commandqueue->updateTextureMappings(sparse_internal->texture.get(), heap, operations.data(), operations.size());
			}
			else if (command.sparse_resource->IsBuffer())
			{
				auto sparse_internal = to_internal<GPUBuffer>(command.sparse_resource);
				wi::vector<MTL4::UpdateSparseBufferMappingOperation> operations;
				operations.reserve(command.num_resource_regions);
				for (uint32_t j = 0; j < command.num_resource_regions; ++j)
				{
					const SparseResourceCoordinate& coordinate = command.coordinates[j];
					const SparseRegionSize& size = command.sizes[j];
					MTL4::UpdateSparseBufferMappingOperation op = {
						command.range_flags[j] == TileRangeFlags::Null ? MTL::SparseTextureMappingModeUnmap : MTL::SparseTextureMappingModeMap,
						{coordinate.x, size.width},
						command.range_start_offsets[j]
					};
					operations.push_back(op);
				}
				commandqueue->updateBufferMappings(sparse_internal->buffer.get(), heap, operations.data(), operations.size());
			}
		}
		
#if 0
		NS::SharedPtr<MTL::SharedEvent> event = NS::TransferPtr(device->newSharedEvent());
		event->setSignaledValue(0);
		commandqueue->signalEvent(event.get(), 1);
		event->waitUntilSignaledValue(1, ~0ull);
#endif
	}

	void GraphicsDevice_Metal::WaitCommandList(CommandList cmd, CommandList wait_for)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		CommandList_Metal& commandlist_wait_for = GetCommandList(wait_for);
		assert(commandlist_wait_for.id < commandlist.id); // can't wait for future command list!
		Semaphore sema = new_semaphore();
		commandlist.waits.push_back(sema);
		commandlist_wait_for.signals.push_back(sema);
	}
	void GraphicsDevice_Metal::RenderPassBegin(const SwapChain* swapchain, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.compute_encoder != nullptr)
		{
			commandlist.compute_encoder->endEncoding();
			commandlist.compute_encoder = nullptr;
		}
		commandlist.autorelease_start();
		
		auto internal_state = to_internal(swapchain);
		
		CA::MetalDrawable* drawable = internal_state->layer->nextDrawable();
		while (drawable == nullptr)
		{
			drawable = internal_state->layer->nextDrawable();
		}
		internal_state->current_drawable = NS::TransferPtr(drawable->retain());
		internal_state->current_texture = NS::TransferPtr(drawable->texture()->retain());
		
		commandlist.presents.push_back(internal_state->current_drawable);
		
		NS::SharedPtr<MTL4::RenderPassDescriptor> descriptor = NS::TransferPtr(MTL4::RenderPassDescriptor::alloc()->init());
		NS::SharedPtr<MTL::RenderPassColorAttachmentDescriptor> color_attachment_descriptor = NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()->init());
		
		CGSize size = internal_state->layer->drawableSize();
		descriptor->setRenderTargetWidth(size.width);
		descriptor->setRenderTargetHeight(size.height);
		descriptor->setDefaultRasterSampleCount(1);
		
		color_attachment_descriptor->setTexture(internal_state->current_drawable->texture());
		color_attachment_descriptor->setClearColor(MTL::ClearColor::Make(swapchain->desc.clear_color[0], swapchain->desc.clear_color[1], swapchain->desc.clear_color[2], swapchain->desc.clear_color[3]));
		color_attachment_descriptor->setLoadAction(MTL::LoadActionClear);
		color_attachment_descriptor->setStoreAction(MTL::StoreActionStore);
		descriptor->colorAttachments()->setObject(color_attachment_descriptor.get(), 0);
		
		commandlist.render_encoder = commandlist.commandbuffer->renderCommandEncoder(descriptor.get());
		commandlist.dirty_vb = true;
		commandlist.dirty_root = true;
		commandlist.dirty_sampler = true;
		commandlist.dirty_resource = true;
		commandlist.dirty_scissor = true;
		commandlist.dirty_viewport = true;
		commandlist.dirty_pso = true;
		
		commandlist.render_width = size.width;
		commandlist.render_height = size.height;
		
		commandlist.renderpass_info = RenderPassInfo::from(swapchain->desc);
	}
	void GraphicsDevice_Metal::RenderPassBegin(const RenderPassImage* images, uint32_t image_count, const GPUQueryHeap* occlusionqueries, CommandList cmd, RenderPassFlags flags)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.compute_encoder != nullptr)
		{
			commandlist.compute_encoder->endEncoding();
			commandlist.compute_encoder = nullptr;
		}
		commandlist.autorelease_start();
		
		NS::SharedPtr<MTL4::RenderPassDescriptor> descriptor = NS::TransferPtr(MTL4::RenderPassDescriptor::alloc()->init());
		NS::SharedPtr<MTL::RenderPassColorAttachmentDescriptor> color_attachment_descriptors[8];
		NS::SharedPtr<MTL::RenderPassDepthAttachmentDescriptor> depth_attachment_descriptor;
		NS::SharedPtr<MTL::RenderPassStencilAttachmentDescriptor> stencil_attachment_descriptor;
		
		if (image_count > 0)
		{
			descriptor->setRenderTargetWidth(images[0].texture->desc.width);
			descriptor->setRenderTargetHeight(images[0].texture->desc.height);
			descriptor->setRenderTargetArrayLength(images[0].texture->desc.array_size);
			descriptor->setDefaultRasterSampleCount(images[0].texture->desc.sample_count);
			commandlist.render_width = images[0].texture->desc.width;
			commandlist.render_height = images[0].texture->desc.height;
		}
		else
		{
			descriptor->setDefaultRasterSampleCount(1);
		}
		
		uint32_t color_attachment_index = 0;
		uint32_t resolve_index = 0;
		for (uint32_t i = 0; i < image_count; ++i)
		{
			const RenderPassImage& image = images[i];
			auto internal_state = to_internal(image.texture);
			
			MTL::LoadAction load_action;
			switch (image.loadop) {
				case RenderPassImage::LoadOp::DONTCARE:
					load_action = MTL::LoadActionDontCare;
					break;
				case RenderPassImage::LoadOp::LOAD:
					load_action = MTL::LoadActionLoad;
					break;
				case RenderPassImage::LoadOp::CLEAR:
					load_action = MTL::LoadActionClear;
					break;
				default:
					break;
			}
			MTL::StoreAction store_action;
			switch (image.storeop) {
			  case RenderPassImage::StoreOp::DONTCARE:
				  store_action = MTL::StoreActionDontCare;
				  break;
			  case RenderPassImage::StoreOp::STORE:
				  store_action = MTL::StoreActionStore;
				  break;
			  default:
				  break;
			}
			
			switch (image.type) {
				case RenderPassImage::Type::RENDERTARGET:
				{
					Texture_Metal::Subresource& subresource = image.subresource < 0 ? internal_state->rtv : internal_state->subresources_rtv[image.subresource];
					auto& color_attachment_descriptor = color_attachment_descriptors[color_attachment_index];
					color_attachment_descriptor = NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()->init());
					color_attachment_descriptor->init();
					color_attachment_descriptor->setTexture(internal_state->texture.get());
					color_attachment_descriptor->setLevel(subresource.firstMip);
					color_attachment_descriptor->setSlice(subresource.firstSlice);
					color_attachment_descriptor->setClearColor(MTL::ClearColor::Make(image.texture->desc.clear.color[0], image.texture->desc.clear.color[1], image.texture->desc.clear.color[2], image.texture->desc.clear.color[3]));
					color_attachment_descriptor->setLoadAction(load_action);
					color_attachment_descriptor->setStoreAction(store_action);
					color_attachment_descriptor->setResolveTexture(nullptr);
					color_attachment_descriptor->setResolveLevel(0);
					color_attachment_descriptor->setResolveSlice(0);
					descriptor->colorAttachments()->setObject(color_attachment_descriptor.get(), color_attachment_index);
					color_attachment_index++;
					break;
				}
				case RenderPassImage::Type::DEPTH_STENCIL:
				{
					Texture_Metal::Subresource& subresource = image.subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[image.subresource];
					depth_attachment_descriptor = NS::TransferPtr(MTL::RenderPassDepthAttachmentDescriptor::alloc()->init());
					depth_attachment_descriptor->setTexture(internal_state->texture.get());
					depth_attachment_descriptor->setLevel(subresource.firstMip);
					depth_attachment_descriptor->setSlice(subresource.firstSlice);
					depth_attachment_descriptor->setClearDepth(image.texture->desc.clear.depth_stencil.depth);
					depth_attachment_descriptor->setLoadAction(load_action);
					depth_attachment_descriptor->setStoreAction(store_action);
					depth_attachment_descriptor->setResolveTexture(nullptr);
					descriptor->setDepthAttachment(depth_attachment_descriptor.get());
					if (IsFormatStencilSupport(image.texture->desc.format))
					{
						stencil_attachment_descriptor = NS::TransferPtr(MTL::RenderPassStencilAttachmentDescriptor::alloc()->init());
						stencil_attachment_descriptor->setTexture(internal_state->texture.get());
						stencil_attachment_descriptor->setClearStencil(image.texture->desc.clear.depth_stencil.stencil);
						stencil_attachment_descriptor->setLoadAction(load_action);
						stencil_attachment_descriptor->setStoreAction(store_action);
						descriptor->setStencilAttachment(stencil_attachment_descriptor.get());
					}
					break;
				}
				case RenderPassImage::Type::RESOLVE:
				{
					Texture_Metal::Subresource& subresource = image.subresource < 0 ? internal_state->srv : internal_state->subresources_srv[image.subresource];
					auto& color_attachment_descriptor = color_attachment_descriptors[resolve_index];
					color_attachment_descriptor->setResolveTexture(internal_state->texture.get());
					color_attachment_descriptor->setResolveLevel(subresource.firstMip);
					color_attachment_descriptor->setResolveSlice(subresource.firstSlice);
					color_attachment_descriptor->setStoreAction(color_attachment_descriptor->storeAction() == MTL::StoreActionStore ? MTL::StoreActionStoreAndMultisampleResolve : MTL::StoreActionMultisampleResolve);
					descriptor->colorAttachments()->setObject(color_attachment_descriptor.get(), resolve_index);
					resolve_index++;
					break;
				}
				case RenderPassImage::Type::RESOLVE_DEPTH:
				{
					Texture_Metal::Subresource& subresource = image.subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[image.subresource];
					depth_attachment_descriptor->setResolveTexture(internal_state->texture.get());
					depth_attachment_descriptor->setResolveLevel(subresource.firstMip);
					depth_attachment_descriptor->setResolveSlice(subresource.firstSlice);
					depth_attachment_descriptor->setStoreAction(depth_attachment_descriptor->storeAction() == MTL::StoreActionStore ? MTL::StoreActionStoreAndMultisampleResolve : MTL::StoreActionMultisampleResolve);
					descriptor->setDepthAttachment(depth_attachment_descriptor.get());
					break;
				}
				default:
					assert(0);
					break;
			}
		}
		
		if (occlusionqueries != nullptr && occlusionqueries->IsValid())
		{
			auto occlusionquery_internal = to_internal(occlusionqueries);
			descriptor->setVisibilityResultBuffer(occlusionquery_internal->buffer.get());
			descriptor->setVisibilityResultType(MTL::VisibilityResultTypeAccumulate);
		}
		
		MTL4::RenderEncoderOptions options = MTL4::RenderEncoderOptionNone;
		if (has_flag(flags, RenderPassFlags::SUSPENDING))
		{
			options |= MTL4::RenderEncoderOptionSuspending;
		}
		if (has_flag(flags, RenderPassFlags::RESUMING))
		{
			options |= MTL4::RenderEncoderOptionResuming;
		}
		commandlist.render_encoder = commandlist.commandbuffer->renderCommandEncoder(descriptor.get(), options);
		if (occlusionqueries != nullptr && occlusionqueries->IsValid())
		{
			commandlist.render_encoder->setVisibilityResultMode(MTL::VisibilityResultModeDisabled, 0);
		}
		commandlist.dirty_vb = true;
		commandlist.dirty_root = true;
		commandlist.dirty_sampler = true;
		commandlist.dirty_resource = true;
		commandlist.dirty_scissor = true;
		commandlist.dirty_viewport = true;
		commandlist.dirty_pso = true;
		
		commandlist.renderpass_info = RenderPassInfo::from(images, image_count);
	}
	void GraphicsDevice_Metal::RenderPassEnd(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(commandlist.render_encoder != nullptr);
		
		commandlist.render_encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, MTL4::VisibilityOptionNone);
		commandlist.render_encoder->endEncoding();
		commandlist.dirty_pso = true;
		
		commandlist.render_width = 0;
		commandlist.render_height = 0;

		commandlist.renderpass_info = {};
		commandlist.render_encoder = nullptr;
		
		commandlist.autorelease_end();
	}
	void GraphicsDevice_Metal::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd)
	{
		assert(rects != nullptr);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		for (uint32_t i = 0; i < numRects; ++i)
		{
			commandlist.scissors[i].x = uint32_t(rects[i].left);
			commandlist.scissors[i].y = uint32_t(rects[i].top);
			commandlist.scissors[i].width = uint32_t(rects[i].right - rects[i].left);
			commandlist.scissors[i].height = uint32_t(rects[i].bottom - rects[i].top);
		}
		commandlist.scissor_count = numRects;
		commandlist.dirty_scissor = true;
	}
	void GraphicsDevice_Metal::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		assert(pViewports != nullptr);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		for (uint32_t i = 0; i < NumViewports; ++i)
		{
			commandlist.viewports[i].originX = pViewports[i].top_left_x;
			commandlist.viewports[i].originY = pViewports[i].top_left_y;
			commandlist.viewports[i].width = pViewports[i].width;
			commandlist.viewports[i].height = pViewports[i].height;
			commandlist.viewports[i].znear = pViewports[i].min_depth;
			commandlist.viewports[i].zfar = pViewports[i].max_depth;
		}
		commandlist.viewport_count = NumViewports;
		commandlist.dirty_viewport = true;
	}
	void GraphicsDevice_Metal::BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_SRV_COUNT);
		if (commandlist.binding_table.SRV[slot].internal_state == resource->internal_state && commandlist.binding_table.SRV_index[slot] == subresource)
			return;
		commandlist.dirty_root = true;
		commandlist.dirty_resource = true;
		commandlist.binding_table.SRV[slot] = *resource;
		commandlist.binding_table.SRV_index[slot] = subresource;
	}
	void GraphicsDevice_Metal::BindResources(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindResource(resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_Metal::BindUAV(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_UAV_COUNT);
		if (commandlist.binding_table.UAV[slot].internal_state == resource->internal_state && commandlist.binding_table.UAV_index[slot] == subresource)
			return;
		commandlist.dirty_root = true;
		commandlist.dirty_resource = true;
		commandlist.binding_table.UAV[slot] = *resource;
		commandlist.binding_table.UAV_index[slot] = subresource;
	}
	void GraphicsDevice_Metal::BindUAVs(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindUAV(resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_Metal::BindSampler(const Sampler* sampler, uint32_t slot, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_SAMPLER_COUNT);
		if (commandlist.binding_table.SAM[slot].internal_state == sampler->internal_state)
			return;
		commandlist.dirty_root = true;
		commandlist.dirty_sampler = true;
		commandlist.binding_table.SAM[slot] = *sampler;
	}
	void GraphicsDevice_Metal::BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_CBV_COUNT);
		if (commandlist.binding_table.CBV[slot].internal_state == buffer->internal_state && commandlist.binding_table.CBV_offset[slot] == offset)
			return;
		commandlist.dirty_root = true;
		if (slot >= arraysize(RootLayout::root_cbvs))
		{
			commandlist.dirty_resource = true;
		}
		commandlist.binding_table.CBV[slot] = *buffer;
		commandlist.binding_table.CBV_offset[slot] = offset;
	}
	void GraphicsDevice_Metal::BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint64_t* offsets, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.dirty_vb = true;
		for (uint32_t i = 0; i < count; ++i)
		{
			auto& vb = commandlist.vertex_buffers[slot + i];
			if (vertexBuffers[i] == nullptr || !vertexBuffers[i]->IsValid())
				continue;
			auto internal_state = to_internal(vertexBuffers[i]);
			const uint64_t offset = offsets == nullptr ? 0 : offsets[i];
			vb.gpu_address = internal_state->gpu_address + offset;
			vb.stride = strides[i];
		}
	}
	void GraphicsDevice_Metal::BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd)
	{
		if (indexBuffer == nullptr || !indexBuffer->IsValid())
			return;
		auto internal_state = to_internal(indexBuffer);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.index_buffer.bufferAddress = internal_state->gpu_address + offset;
		commandlist.index_buffer.length = internal_state->buffer->length();
		commandlist.index_type = format == IndexBufferFormat::UINT32 ? MTL::IndexTypeUInt32 : MTL::IndexTypeUInt16;
	}
	void GraphicsDevice_Metal::BindStencilRef(uint32_t value, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.render_encoder->setStencilReferenceValue(value);
	}
	void GraphicsDevice_Metal::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.render_encoder->setBlendColor(r, g, b, a);
	}
	void GraphicsDevice_Metal::BindShadingRate(ShadingRate rate, CommandList cmd)
	{
	}
	void GraphicsDevice_Metal::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(pso);
		
		if (internal_state->render_pipeline.get() == nullptr)
		{
			// Just in time pso:
			PipelineHash pipeline_hash;
			pipeline_hash.pso = pso;
			pipeline_hash.renderpass_hash = commandlist.renderpass_info.get_hash();
			if (commandlist.pipeline_hash != pipeline_hash)
			{
				commandlist.dirty_pso = true;
				commandlist.pipeline_hash = pipeline_hash;
			}
		}
		else
		{
			// Precompiled pso:
			if (commandlist.active_pso != pso)
			{
				commandlist.active_pso = pso;
				commandlist.dirty_pso = true;
				commandlist.pipeline_hash = {};
			}
		}
		commandlist.active_pso = pso;
		commandlist.drawargs_required = internal_state->needs_draw_params;
	}
	void GraphicsDevice_Metal::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.dirty_cs = commandlist.active_cs != cs;
		commandlist.active_cs = cs;
	}
	void GraphicsDevice_Metal::BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.render_encoder->setDepthTestBounds(min_bounds, max_bounds);
	}

	void GraphicsDevice_Metal::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		// Draw args binding is a requirement of metal shader converter to match DirectX behaviour of vertexID and instanceID:
		if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			IRRuntimeDrawArgument da = {
				.vertexCountPerInstance = vertexCount,
				.instanceCount = 1,
				.startVertexLocation = startVertexLocation,
				.startInstanceLocation = 0
			};
			IRRuntimeDrawParams dp = { .draw = da };
			auto alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			alloc = AllocateGPU(sizeof(uint16_t), cmd);
			std::memcpy(alloc.data, &kIRNonIndexedDraw, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		commandlist.render_encoder->drawPrimitives(commandlist.primitive_type, startVertexLocation, vertexCount);
	}
	void GraphicsDevice_Metal::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		// Draw args binding is a requirement of metal shader converter to match DirectX behaviour of vertexID and instanceID:
		if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			IRRuntimeDrawIndexedArgument da = {
				.indexCountPerInstance = indexCount,
				.instanceCount = 1,
				.startIndexLocation = startIndexLocation,
				.baseVertexLocation = baseVertexLocation,
				.startInstanceLocation = 0
			};
			IRRuntimeDrawParams dp = { .drawIndexed = da };
			auto alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			alloc = AllocateGPU(sizeof(uint16_t), cmd);
			uint16_t irindextype = IRMetalIndexToIRIndex(commandlist.index_type);
			std::memcpy(alloc.data, &irindextype, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		const uint64_t index_stride = commandlist.index_type == MTL::IndexTypeUInt32 ? sizeof(uint32_t) : sizeof(uint16_t);
		const uint64_t indexBufferOffset = startIndexLocation * index_stride;
		commandlist.render_encoder->drawIndexedPrimitives(commandlist.primitive_type, indexCount, commandlist.index_type, commandlist.index_buffer.bufferAddress + indexBufferOffset, commandlist.index_buffer.length);
	}
	void GraphicsDevice_Metal::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		// Draw args binding is a requirement of metal shader converter to match DirectX behaviour of vertexID and instanceID:
		if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			IRRuntimeDrawArgument da = {
				.vertexCountPerInstance = vertexCount,
				.instanceCount = instanceCount,
				.startVertexLocation = startVertexLocation,
				.startInstanceLocation = startInstanceLocation
			};
			IRRuntimeDrawParams dp = { .draw = da };
			auto alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			alloc = AllocateGPU(sizeof(uint16_t), cmd);
			std::memcpy(alloc.data, &kIRNonIndexedDraw, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		commandlist.render_encoder->drawPrimitives(commandlist.primitive_type, startVertexLocation, vertexCount, instanceCount, startInstanceLocation);
	}
	void GraphicsDevice_Metal::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		// Draw args binding is a requirement of metal shader converter to match DirectX behaviour of vertexID and instanceID:
		if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			IRRuntimeDrawIndexedArgument da = {
				.indexCountPerInstance = indexCount,
				.instanceCount = instanceCount,
				.startIndexLocation = startIndexLocation,
				.baseVertexLocation = baseVertexLocation,
				.startInstanceLocation = startInstanceLocation
			};
			IRRuntimeDrawParams dp = { .drawIndexed = da };
			auto alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			alloc = AllocateGPU(sizeof(uint16_t), cmd);
			uint16_t irindextype = IRMetalIndexToIRIndex(commandlist.index_type);
			std::memcpy(alloc.data, &irindextype, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		const uint64_t index_stride = commandlist.index_type == MTL::IndexTypeUInt32 ? sizeof(uint32_t) : sizeof(uint16_t);
		const uint64_t indexBufferOffset = startIndexLocation * index_stride;
		commandlist.render_encoder->drawIndexedPrimitives(commandlist.primitive_type, indexCount, commandlist.index_type, commandlist.index_buffer.bufferAddress + indexBufferOffset, commandlist.index_buffer.length, instanceCount, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_Metal::DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		auto internal_state = to_internal(args);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		// Draw args binding is a requirement of metal shader converter to match DirectX behaviour of vertexID and instanceID:
		if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			commandlist.argument_table->setAddress(internal_state->gpu_address + args_offset, kIRArgumentBufferDrawArgumentsBindPoint);
			auto alloc = AllocateGPU(sizeof(uint16_t), cmd);
			std::memcpy(alloc.data, &kIRNonIndexedDraw, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		commandlist.render_encoder->drawPrimitives(commandlist.primitive_type, internal_state->gpu_address + args_offset);
	}
	void GraphicsDevice_Metal::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		auto internal_state = to_internal(args);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		// Draw args binding is a requirement of metal shader converter to match DirectX behaviour of vertexID and instanceID:
		if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			commandlist.argument_table->setAddress(internal_state->gpu_address + args_offset, kIRArgumentBufferDrawArgumentsBindPoint);
			auto alloc = AllocateGPU(sizeof(uint16_t), cmd);
			uint16_t irindextype = IRMetalIndexToIRIndex(commandlist.index_type);
			std::memcpy(alloc.data, &irindextype, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		commandlist.render_encoder->drawIndexedPrimitives(commandlist.primitive_type, commandlist.index_type, commandlist.index_buffer.bufferAddress, commandlist.index_buffer.length, internal_state->gpu_address + args_offset);
	}
	void GraphicsDevice_Metal::DrawInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Metal& commandlist = GetCommandList(cmd);
	}
	void GraphicsDevice_Metal::DrawIndexedInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Metal& commandlist = GetCommandList(cmd);
	}
	void GraphicsDevice_Metal::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predispatch(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto cs_internal = to_internal(commandlist.active_cs);
		commandlist.compute_encoder->dispatchThreadgroups({threadGroupCountX, threadGroupCountY, threadGroupCountZ}, cs_internal->numthreads);
		commandlist.autorelease_end();
	}
	void GraphicsDevice_Metal::DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto cs_internal = to_internal(commandlist.active_cs);
		auto internal_state = to_internal(args);
		commandlist.compute_encoder->dispatchThreadgroups(internal_state->gpu_address + args_offset, cs_internal->numthreads);
		commandlist.autorelease_end();
	}
	void GraphicsDevice_Metal::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
	}
	void GraphicsDevice_Metal::DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Metal& commandlist = GetCommandList(cmd);
	}
	void GraphicsDevice_Metal::DispatchMeshIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Metal& commandlist = GetCommandList(cmd);
	}
	void GraphicsDevice_Metal::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		precopy(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (pDst->IsTexture() && pSrc->IsTexture())
		{
			const Texture& srctex = *(Texture*)pSrc;
			const Texture& dsttex = *(Texture*)pDst;
			auto src_internal = to_internal<Texture>(pSrc);
			auto dst_internal = to_internal<Texture>(pDst);
			if (src_internal->texture.get() != nullptr && dst_internal->texture.get() != nullptr)
			{
				// Normal texture->texture copy
				commandlist.compute_encoder->copyFromTexture(src_internal->texture.get(), dst_internal->texture.get());
			}
			else if(src_internal->texture.get() != nullptr && dst_internal->buffer.get() != nullptr)
			{
				// Texture->linear copy:
				const uint64_t data_begin = (uint64_t)dsttex.mapped_subresources[0].data_ptr;
				uint32_t subresource_index = 0;
				for (uint32_t slice = 0; slice < srctex.desc.array_size; ++slice)
				{
					for (uint32_t mip = 0; mip < srctex.desc.mip_levels; ++mip)
					{
						const uint32_t mip_width = std::max(1u, srctex.desc.width >> mip);
						const uint32_t mip_height = std::max(1u, srctex.desc.height >> mip);
						const uint32_t mip_depth = std::max(1u, srctex.desc.depth >> mip);
						const SubresourceData& subresource_data = dsttex.mapped_subresources[subresource_index++];
						uint64_t data_offset = uint64_t((uint64_t)subresource_data.data_ptr - data_begin);
						commandlist.compute_encoder->copyFromTexture(src_internal->texture.get(), slice, mip, {0,0,0}, {mip_width, mip_height, mip_depth}, dst_internal->buffer.get(), data_offset, subresource_data.row_pitch, subresource_data.slice_pitch * mip_depth);
					}
				}
			}
			else if(src_internal->texture.get() != nullptr && dst_internal->buffer.get() != nullptr)
			{
				// Linear->texture copy:
				const uint64_t data_begin = (uint64_t)srctex.mapped_subresources[0].data_ptr;
				uint32_t subresource_index = 0;
				for (uint32_t slice = 0; slice < dsttex.desc.array_size; ++slice)
				{
					for (uint32_t mip = 0; mip < dsttex.desc.mip_levels; ++mip)
					{
						const uint32_t mip_width = std::max(1u, dsttex.desc.width >> mip);
						const uint32_t mip_height = std::max(1u, dsttex.desc.height >> mip);
						const uint32_t mip_depth = std::max(1u, dsttex.desc.depth >> mip);
						const SubresourceData& subresource_data = srctex.mapped_subresources[subresource_index++];
						uint64_t data_offset = uint64_t((uint64_t)subresource_data.data_ptr - data_begin);
						commandlist.compute_encoder->copyFromBuffer(src_internal->buffer.get(), data_offset, subresource_data.row_pitch, subresource_data.slice_pitch * mip_depth, {mip_width,mip_height,mip_depth}, dst_internal->texture.get(), slice, mip, {0,0,0});
					}
				}
			}
		}
		else if (pDst->IsBuffer() && pSrc->IsBuffer())
		{
			auto src_internal = to_internal<GPUBuffer>(pSrc);
			auto dst_internal = to_internal<GPUBuffer>(pDst);
			commandlist.compute_encoder->copyFromBuffer(src_internal->buffer.get(), 0, dst_internal->buffer.get(), 0, dst_internal->buffer->length());
		}
		commandlist.autorelease_end();
	}
	void GraphicsDevice_Metal::CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd)
	{
		precopy(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);
		commandlist.compute_encoder->copyFromBuffer(internal_state_src->buffer.get(), src_offset, internal_state_dst->buffer.get(), dst_offset, size);
		commandlist.autorelease_end();
	}
	void GraphicsDevice_Metal::CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox, ImageAspect dst_aspect, ImageAspect src_aspect)
	{
		precopy(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		const uint32_t srcWidth = std::max(1u, src->desc.width >> srcMip);
		const uint32_t srcHeight = std::max(1u, src->desc.height >> srcMip);
		const uint32_t srcDepth = std::max(1u, src->desc.depth >> srcMip);
		const uint32_t dstWidth = std::max(1u, dst->desc.width >> dstMip);
		const uint32_t dstHeight = std::max(1u, dst->desc.height >> dstMip);
		const uint32_t dstDepth = std::max(1u, dst->desc.depth >> dstMip);
		auto src_internal = to_internal(src);
		auto dst_internal = to_internal(dst);
		MTL::Origin srcOrigin = {};
		MTL::Size srcSize = {};
		MTL::Origin dstOrigin = { dstX, dstY, dstZ };
		if (srcbox == nullptr)
		{
			srcSize.width = srcWidth;
			srcSize.height = srcHeight;
			srcSize.depth = srcDepth;
		}
		else
		{
			srcOrigin.x = srcbox->left;
			srcOrigin.y = srcbox->top;
			srcOrigin.z = srcbox->front;
			srcSize.width = srcbox->right - srcbox->left;
			srcSize.height = srcbox->bottom - srcbox->top;
			srcSize.depth = srcbox->back - srcbox->front;
		}
		if (dst->desc.format != src->desc.format)
		{
			// Hack:
			//	Metal cannot do format reinterpret texture->texture copy, so instead of rewriting my block compression shaders,
			//	I implement texture->buffer->texture copy as a workaround
			const size_t buffer_size = ComputeTextureMipMemorySizeInBytes(src->desc, srcMip);
			const size_t row_pitch = ComputeTextureMipRowPitch(src->desc, srcMip);
			static NS::SharedPtr<MTL::Buffer> reinterpret_buffer[QUEUE_COUNT]; // one per queue for correct gpu multi-queue hazard safety
			static std::mutex locker;
			std::scoped_lock lck(locker);
			if (reinterpret_buffer[commandlist.queue].get() == nullptr || reinterpret_buffer[commandlist.queue]->length() < buffer_size)
			{
				if (reinterpret_buffer[commandlist.queue].get() != nullptr)
				{
					std::scoped_lock lck2(allocationhandler->destroylocker);
					allocationhandler->destroyer_resources.push_back(std::make_pair(reinterpret_buffer[commandlist.queue], allocationhandler->framecount));
				}
				reinterpret_buffer[commandlist.queue] = NS::TransferPtr(device->newBuffer(buffer_size, MTL::ResourceStorageModePrivate));
				reinterpret_buffer[commandlist.queue]->setLabel(NS::TransferPtr(NS::String::alloc()->init("reinterpret_buffer", NS::UTF8StringEncoding)).get());
				allocationhandler->make_resident(reinterpret_buffer[commandlist.queue].get());
			}
			commandlist.compute_encoder->copyFromTexture(src_internal->texture.get(), srcSlice, srcMip, srcOrigin, srcSize, reinterpret_buffer[commandlist.queue].get(), 0, row_pitch, buffer_size);
			commandlist.compute_encoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageBlit, MTL4::VisibilityOptionNone);
			if (!IsFormatBlockCompressed(src->desc.format) && IsFormatBlockCompressed(dst->desc.format))
			{
				// raw -> block compressed copy
				const uint32_t block_size = GetFormatBlockSize(dst->desc.format);
				srcSize.width = std::min((uint32_t)srcSize.width * block_size, dstWidth);
				srcSize.height = std::min((uint32_t)srcSize.height * block_size, dstHeight);
			}
			if (IsFormatBlockCompressed(src->desc.format) && !IsFormatBlockCompressed(dst->desc.format))
			{
				// block compressed -> raw copy
				const uint32_t block_size = GetFormatBlockSize(dst->desc.format);
				srcSize.width = std::max(1u, (uint32_t)srcSize.width / block_size);
				srcSize.height = std::max(1u, (uint32_t)srcSize.height / block_size);
			}
			commandlist.compute_encoder->copyFromBuffer(reinterpret_buffer[commandlist.queue].get(), 0, row_pitch, buffer_size, srcSize, dst_internal->texture.get(), dstSlice, dstMip, dstOrigin);
		}
		else
		{
			commandlist.compute_encoder->copyFromTexture(src_internal->texture.get(), srcSlice, srcMip, srcOrigin, srcSize, dst_internal->texture.get(), dstSlice, dstMip, dstOrigin);
		}
		commandlist.autorelease_end();
	}
	void GraphicsDevice_Metal::QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		switch (heap->desc.type)
		{
			case GpuQueryType::OCCLUSION:
				assert(commandlist.render_encoder != nullptr);
				commandlist.render_encoder->setVisibilityResultMode(MTL::VisibilityResultModeCounting, index * sizeof(uint64_t));
				break;
			case GpuQueryType::OCCLUSION_BINARY:
				assert(commandlist.render_encoder != nullptr);
				commandlist.render_encoder->setVisibilityResultMode(MTL::VisibilityResultModeBoolean, index * sizeof(uint64_t));
				break;
			case GpuQueryType::TIMESTAMP:
				break;
			default:
				break;
		}
	}
	void GraphicsDevice_Metal::QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);
		switch (heap->desc.type)
		{
			case GpuQueryType::OCCLUSION:
			case GpuQueryType::OCCLUSION_BINARY:
				assert(commandlist.render_encoder != nullptr);
				commandlist.render_encoder->setVisibilityResultMode(MTL::VisibilityResultModeDisabled, index * sizeof(uint64_t));
				break;
			case GpuQueryType::TIMESTAMP:
				if (commandlist.render_encoder != nullptr)
				{
					// Note: fMTL::RenderStageFragment timestamp is unreliable if no fragment was rendered. But we can't determine here in a non-intrusive way whether a fragment was rendered or not in the current render pass for this timestamp
					commandlist.render_encoder->writeTimestamp(MTL4::TimestampGranularityPrecise, MTL::RenderStageFragment, internal_state->counter_heap.get(), index);
				}
				else
				{
#if 0
					precopy(cmd);
					commandlist.compute_encoder->writeTimestamp(MTL4::TimestampGranularityPrecise, internal_state->counter_heap.get(), index);
					commandlist.autorelease_end();
#else
					commandlist.commandbuffer->writeTimestampIntoHeap(internal_state->counter_heap.get(), index);
#endif
				}
				break;
			default:
				break;
		}
	}
	void GraphicsDevice_Metal::QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd)
	{
		if (count == 0)
			return;
		CommandList_Metal& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);
		auto dst_internal = to_internal(dest);
		
		switch (heap->desc.type)
		{
			case GpuQueryType::OCCLUSION:
			case GpuQueryType::OCCLUSION_BINARY:
				precopy(cmd);
				commandlist.compute_encoder->copyFromBuffer(internal_state->buffer.get(), index * sizeof(uint64_t), dst_internal->buffer.get(), dest_offset, count * sizeof(uint64_t));
				commandlist.autorelease_end();
				break;
			case GpuQueryType::TIMESTAMP:
				commandlist.commandbuffer->resolveCounterHeap(internal_state->counter_heap.get(), {index, count}, {dst_internal->gpu_address, count * sizeof(uint64_t)}, nullptr, nullptr);
				break;
			default:
				break;
		}
	}
	void GraphicsDevice_Metal::QueryReset(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);
		
		switch (heap->desc.type)
		{
			case GpuQueryType::OCCLUSION:
			case GpuQueryType::OCCLUSION_BINARY:
				precopy(cmd);
				commandlist.compute_encoder->fillBuffer(internal_state->buffer.get(), {index * sizeof(uint64_t), count * sizeof(uint64_t)}, 0);
				break;
			case GpuQueryType::TIMESTAMP:
				break;
			default:
				break;
		}
		commandlist.autorelease_end();
	}
	void GraphicsDevice_Metal::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];
			commandlist.barriers.push_back(barrier);
		}
	}
	void GraphicsDevice_Metal::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		predispatch(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto dst_internal = to_internal(dst);
		
		// descriptor is recreated because buffer references might have changed since creation:
		NS::SharedPtr<MTL4::AccelerationStructureDescriptor> descriptor = mtl_acceleration_structure_descriptor(&dst->desc);
		
		if (src != nullptr && (dst->desc.flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE))
		{
			auto src_internal = to_internal(src);
			commandlist.compute_encoder->refitAccelerationStructure(src_internal->acceleration_structure.get(), descriptor.get(), dst_internal->acceleration_structure.get(), {dst_internal->scratch->gpuAddress(), dst_internal->scratch->length()});
		}
		else
		{
			commandlist.compute_encoder->buildAccelerationStructure(dst_internal->acceleration_structure.get(), descriptor.get(), {dst_internal->scratch->gpuAddress(), dst_internal->scratch->length()});
		}
		
		if (dst->desc.type == RaytracingAccelerationStructureDesc::Type::TOPLEVEL)
		{
			// Note: this is workaround for Metal API issue: FB21571936
			//	This reuses the compute encoder for all blas and tlas builds and then closes autorelease
			commandlist.autorelease_end();
		}
	}
	void GraphicsDevice_Metal::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
	}
	void GraphicsDevice_Metal::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		predispatch(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
	}
	void GraphicsDevice_Metal::PushConstants(const void* data, uint32_t size, CommandList cmd, uint32_t offset)
	{
		assert(offset + size < sizeof(RootLayout::constants));
		CommandList_Metal& commandlist = GetCommandList(cmd);
		std::memcpy((uint8_t*)commandlist.root.constants + offset, data, size);
		commandlist.dirty_root = true;
	}
	void GraphicsDevice_Metal::PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability::PREDICATION))
		{
			CommandList_Metal& commandlist = GetCommandList(cmd);
			auto internal_state = to_internal(buffer);
		}
	}
	void GraphicsDevice_Metal::PredicationEnd(CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability::PREDICATION))
		{
			CommandList_Metal& commandlist = GetCommandList(cmd);
		}
	}
	void GraphicsDevice_Metal::ClearUAV(const GPUResource* resource, uint32_t value, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (resource->IsTexture())
		{
			auto internal_state = to_internal<Texture>(resource);
		}
		else if (resource->IsBuffer())
		{
			precopy(cmd);
			auto internal_state = to_internal<GPUBuffer>(resource);
			assert(value == uint8_t(value)); // The fillBuffer only works with uint8_t
			commandlist.compute_encoder->fillBuffer(internal_state->buffer.get(), {0, internal_state->buffer->length()}, (uint8_t)value);
		}
		commandlist.autorelease_end();
	}
	void GraphicsDevice_Metal::VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd)
	{
	}

	void GraphicsDevice_Metal::EventBegin(const char* name, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		NS::SharedPtr<NS::String> str = NS::TransferPtr(NS::String::alloc()->init(name, NS::UTF8StringEncoding));
		if (commandlist.render_encoder != nullptr)
		{
			commandlist.render_encoder->pushDebugGroup(str.get());
		}
		else if (commandlist.compute_encoder != nullptr)
		{
			commandlist.compute_encoder->pushDebugGroup(str.get());
		}
		else
		{
			commandlist.commandbuffer->pushDebugGroup(str.get());
		}
	}
	void GraphicsDevice_Metal::EventEnd(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.render_encoder != nullptr)
		{
			commandlist.render_encoder->popDebugGroup();
		}
		else if (commandlist.compute_encoder != nullptr)
		{
			commandlist.compute_encoder->popDebugGroup();
		}
		else
		{
			commandlist.commandbuffer->popDebugGroup();
		}
	}
	void GraphicsDevice_Metal::SetMarker(const char* name, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		NS::SharedPtr<NS::String> str = NS::TransferPtr(NS::String::alloc()->init(name, NS::UTF8StringEncoding));
		if (commandlist.render_encoder != nullptr)
		{
			commandlist.render_encoder->setLabel(str.get());
		}
		else if (commandlist.compute_encoder != nullptr)
		{
			commandlist.compute_encoder->setLabel(str.get());
		}
		else
		{
			commandlist.commandbuffer->setLabel(str.get());
		}
	}
}

#endif // __APPLE__
