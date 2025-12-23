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

	struct Buffer_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::Buffer> buffer;
		MTL::GPUAddress gpu_address = {};
		
		struct Subresource
		{
			NS::SharedPtr<MTL::Buffer> buffer;
			NS::SharedPtr<MTL::Texture> texture_buffer_view;
			uint64_t offset = 0;
			uint64_t size = 0;
			int index = -1;
		};
		Subresource srv;
		Subresource uav;
		wi::vector<Subresource> subresources_srv;
		wi::vector<Subresource> subresources_uav;
		
		void destroy_subresources()
		{
			uint64_t framecount = allocationhandler->framecount;
			
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(srv.buffer), framecount));
			if (srv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(srv.index, framecount));
			
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(uav.buffer), framecount));
			if (uav.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(uav.index, framecount));
			
			for (auto& x : subresources_srv)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(buffer), framecount));
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_srv.clear();
			for (auto& x : subresources_uav)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(buffer), framecount));
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
		
		struct Subresource
		{
			NS::SharedPtr<MTL::Texture> texture;
			int index = -1;
		};
		Subresource srv;
		Subresource uav;
		Subresource rtv;
		Subresource dsv;
		wi::vector<Subresource> subresources_srv;
		wi::vector<Subresource> subresources_uav;
		wi::vector<Subresource> subresources_rtv;
		wi::vector<Subresource> subresources_dsv;
		
		void destroy_subresources()
		{
			uint64_t framecount = allocationhandler->framecount;
			
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(srv.texture), framecount));
			if (srv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(srv.index, framecount));
			
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(uav.texture), framecount));
			if (uav.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(uav.index, framecount));
			
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(rtv.texture), framecount));
			if (rtv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(rtv.index, framecount));
			
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(dsv.texture), framecount));
			if (dsv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(dsv.index, framecount));
			
			for (auto& x : subresources_srv)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(x.texture), framecount));
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_srv.clear();
			for (auto& x : subresources_uav)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(x.texture), framecount));
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_uav.clear();
			for (auto& x : subresources_rtv)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(x.texture), framecount));
			}
			subresources_rtv.clear();
			for (auto& x : subresources_dsv)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(x.texture), framecount));
			}
			subresources_dsv.clear();
		}

		~Texture_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(texture), framecount));
			destroy_subresources();
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Sampler_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		int index = -1;
		NS::SharedPtr<MTL::SamplerState> sampler;

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

		~QueryHeap_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(buffer), framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Shader_Metal
	{
		wi::allocator::shared_ptr<GraphicsDevice_Metal::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::Library> library;
		NS::SharedPtr<MTL::Function> function;
		NS::SharedPtr<MTL::ComputePipelineState> compute_pipeline;

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

		~BVH_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
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
		MTK::View* view = nullptr;

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
			commandlist.dirty_root = false;
			GPUAllocation root_allocation = AllocateGPU(sizeof(RootLayout), cmd);
			auto root_allocation_internal = to_internal(&root_allocation.buffer);
			
			// root CBVs:
			for (uint32_t i = 0; i < arraysize(commandlist.root.root_cbv); ++i)
			{
				if (!commandlist.binding_table.CBV[i].IsValid())
					continue;
				auto internal_state = to_internal(&commandlist.binding_table.CBV[i]);
				commandlist.root.root_cbv[i] = internal_state->gpu_address + commandlist.binding_table.CBV_offset[i];
			}
			
			if (commandlist.dirty_resource)
			{
				commandlist.dirty_resource = false;
				ResourceTable resource_table = {};
				GPUAllocation resource_table_allocation = AllocateGPU(sizeof(ResourceTable), cmd);
				auto resource_table_allocation_internal = to_internal(&resource_table_allocation.buffer);
				commandlist.root.resource_table_ptr = resource_table_allocation_internal->gpu_address + resource_table_allocation.offset;
				
				uint32_t resource_table_offset = 0;
				for (uint32_t i = arraysize(commandlist.root.root_cbv); i < arraysize(DescriptorBindingTable::CBV); ++i)
				{
					if (commandlist.binding_table.CBV[i].IsValid())
					{
						auto internal_state = to_internal(&commandlist.binding_table.CBV[i]);
						const uint64_t offset = commandlist.binding_table.CBV_offset[i];
						const MTL::GPUAddress gpu_address = internal_state->gpu_address + offset;
						const uint64_t metadata = 0;
						IRDescriptorTableSetBuffer(&resource_table.descriptors[resource_table_offset], gpu_address, metadata);
					}
					resource_table_offset++;
				}
				for (uint32_t i = 0; i < arraysize(commandlist.binding_table.SRV); ++i)
				{
					if (commandlist.binding_table.SRV[i].IsValid())
					{
						if (commandlist.binding_table.SRV[i].IsBuffer())
						{
							auto internal_state = to_internal<GPUBuffer>(&commandlist.binding_table.SRV[i]);
							const int subresource_index = commandlist.binding_table.SRV_index[i];
							const auto& subresource = subresource_index < 0 ? internal_state->srv : internal_state->subresources_srv[subresource_index];
							IRBufferView buffer_view = {};
							buffer_view.buffer = subresource.buffer.get();
							buffer_view.bufferOffset = subresource.offset;
							buffer_view.bufferSize = subresource.size;
							IRDescriptorTableSetBufferView(&resource_table.descriptors[resource_table_offset], &buffer_view);
						}
						else if (commandlist.binding_table.SRV[i].IsTexture())
						{
							auto internal_state = to_internal<Texture>(&commandlist.binding_table.SRV[i]);
							const int subresource_index = commandlist.binding_table.SRV_index[i];
							const auto& subresource = subresource_index < 0 ? internal_state->srv : internal_state->subresources_srv[subresource_index];
							const uint64_t metadata = 0;
							IRDescriptorTableSetTexture(&resource_table.descriptors[resource_table_offset], subresource.texture.get(), 0, metadata);
						}
						else if (commandlist.binding_table.SRV[i].IsAccelerationStructure())
						{
							// TODO
						}
					}
					resource_table_offset++;
				}
				for (uint32_t i = 0; i < arraysize(commandlist.binding_table.UAV); ++i)
				{
					if (commandlist.binding_table.UAV[i].IsValid())
					{
						if (commandlist.binding_table.UAV[i].IsBuffer())
						{
							auto internal_state = to_internal<GPUBuffer>(&commandlist.binding_table.UAV[i]);
							const int subresource_index = commandlist.binding_table.UAV_index[i];
							const auto& subresource = subresource_index < 0 ? internal_state->uav : internal_state->subresources_uav[subresource_index];
							IRBufferView buffer_view = {};
							buffer_view.buffer = subresource.buffer.get();
							buffer_view.bufferOffset = subresource.offset;
							buffer_view.bufferSize = subresource.size;
							IRDescriptorTableSetBufferView(&resource_table.descriptors[resource_table_offset], &buffer_view);
						}
						else if (commandlist.binding_table.UAV[i].IsTexture())
						{
							auto internal_state = to_internal<Texture>(&commandlist.binding_table.UAV[i]);
							const int subresource_index = commandlist.binding_table.UAV_index[i];
							const auto& subresource = subresource_index < 0 ? internal_state->uav : internal_state->subresources_uav[subresource_index];
							const uint64_t metadata = 0;
							IRDescriptorTableSetTexture(&resource_table.descriptors[resource_table_offset], subresource.texture.get(), 0, metadata);
						}
					}
					resource_table_offset++;
				}
				
				std::memcpy(resource_table_allocation.data, &resource_table, sizeof(resource_table));
			}
			if (commandlist.dirty_sampler)
			{
				commandlist.dirty_sampler = false;
				SamplerTable sampler_table = {};
				GPUAllocation sampler_table_allocation = AllocateGPU(sizeof(SamplerTable), cmd);
				auto sampler_table_allocation_internal = to_internal(&sampler_table_allocation.buffer);
				commandlist.root.sampler_table_ptr = sampler_table_allocation_internal->gpu_address + sampler_table_allocation.offset;
				
				for (uint32_t i = 0; i < arraysize(commandlist.binding_table.SAM); ++i)
				{
					if (!commandlist.binding_table.SAM[i].IsValid())
						continue;
					auto internal_state = to_internal(&commandlist.binding_table.SAM[i]);
					IRDescriptorTableSetSampler(&sampler_table.descriptors[i], internal_state->sampler.get(), 0);
				}
				
				std::memcpy(sampler_table_allocation.data, &sampler_table, sizeof(sampler_table));
			}
			
			// Root data CPU -> GPU:
			std::memcpy(root_allocation.data, &commandlist.root, sizeof(commandlist.root));
			
			if (commandlist.render_encoder != nullptr)
			{
				if (commandlist.active_pso->desc.vs != nullptr)
				{
					commandlist.render_encoder->setVertexBuffer(root_allocation_internal->buffer.get(), root_allocation.offset, kIRArgumentBufferBindPoint);
				}
				if (commandlist.active_pso->desc.ps != nullptr)
				{
					commandlist.render_encoder->setFragmentBuffer(root_allocation_internal->buffer.get(), root_allocation.offset, kIRArgumentBufferBindPoint);
				}
			}
			else if (commandlist.compute_encoder != nullptr)
			{
				commandlist.compute_encoder->setBuffer(root_allocation_internal->buffer.get(), root_allocation.offset, kIRArgumentBufferBindPoint);
			}
		}
		if (commandlist.dirty_vb && commandlist.active_pso != nullptr && commandlist.active_pso->desc.il != nullptr)
		{
			commandlist.dirty_vb = false;
			const InputLayout& il = *commandlist.active_pso->desc.il;
			for (size_t i = 0; i < il.elements.size(); ++i)
			{
				auto& element = il.elements[i];
				auto& vb = commandlist.vertex_buffers[element.input_slot];
				if (!vb.buffer.IsValid())
					continue;
				auto buffer_internal = to_internal(&vb.buffer);
				commandlist.render_encoder->setVertexBuffer(buffer_internal->buffer.get(), vb.offset, kIRVertexBufferBindPoint + i);
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
			auto it = pipelines_global.find(pipeline_hash);
			if (it != pipelines_global.end())
			{
				// Exists in global just in time PSO map:
				pipeline = it->second.get();
			}
			else
			{
				// Doesn't yet exist in global just in time PSO map, but maybe exists on this thread temporary PSO array:
				for (auto& x : commandlist.pipelines_worker)
				{
					if (pipeline_hash == x.first)
					{
						pipeline = x.second.get();
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
				commandlist.pipelines_worker.push_back(std::make_pair(pipeline_hash, internal_new->render_pipeline));
				pipeline = internal_new->render_pipeline.get();
			}
			assert(pipeline != nullptr);
			commandlist.render_encoder->setRenderPipelineState(pipeline);
		}
		else
		{
			// Precompiled PSO:
			commandlist.render_encoder->setRenderPipelineState(internal_state->render_pipeline.get());
		}
		
		if (commandlist.active_pso->desc.vs != nullptr)
		{
			commandlist.render_encoder->setVertexBuffer(descriptor_heap_res.get(), 0, kIRDescriptorHeapBindPoint);
			commandlist.render_encoder->setVertexBuffer(descriptor_heap_sam.get(), 0, kIRSamplerHeapBindPoint);
		}
		if (commandlist.active_pso->desc.ps != nullptr)
		{
			commandlist.render_encoder->setFragmentBuffer(descriptor_heap_res.get(), 0, kIRDescriptorHeapBindPoint);
			commandlist.render_encoder->setFragmentBuffer(descriptor_heap_sam.get(), 0, kIRSamplerHeapBindPoint);
		}
		
		commandlist.render_encoder->setDepthStencilState(internal_state->depth_stencil_state.get());
		
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
		
		if (commandlist.dirty_scissor)
		{
			commandlist.dirty_scissor = false;
			commandlist.render_encoder->setScissorRects(commandlist.scissors, commandlist.scissor_count);
		}
		if (commandlist.dirty_viewport)
		{
			commandlist.dirty_viewport = false;
			commandlist.render_encoder->setViewports(commandlist.viewports, commandlist.viewport_count);
		}
	}
	void GraphicsDevice_Metal::predispatch(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.active_pso = nullptr;
		if (commandlist.render_encoder != nullptr)
		{
			commandlist.render_encoder->endEncoding();
			commandlist.render_encoder = nullptr;
		}
		if (commandlist.blit_encoder != nullptr)
		{
			commandlist.blit_encoder->endEncoding();
			commandlist.blit_encoder = nullptr;
		}
		if (commandlist.compute_encoder == nullptr)
		{
			commandlist.compute_encoder = commandlist.commandbuffer->computeCommandEncoder();
			commandlist.compute_encoder->setBuffer(descriptor_heap_res.get(), 0, kIRDescriptorHeapBindPoint);
			commandlist.compute_encoder->setBuffer(descriptor_heap_sam.get(), 0, kIRSamplerHeapBindPoint);
		}
		
		auto internal_state = to_internal(commandlist.active_cs);
		commandlist.compute_encoder->setComputePipelineState(internal_state->compute_pipeline.get());
		
		binder_flush(cmd);
	}
	void GraphicsDevice_Metal::precopy(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.render_encoder != nullptr)
		{
			commandlist.render_encoder->endEncoding();
			commandlist.render_encoder = nullptr;
		}
		if (commandlist.compute_encoder != nullptr)
		{
			commandlist.compute_encoder->endEncoding();
			commandlist.compute_encoder = nullptr;
		}
		if (commandlist.blit_encoder == nullptr)
		{
			commandlist.blit_encoder = commandlist.commandbuffer->blitCommandEncoder();
		}
	}

	GraphicsDevice_Metal::GraphicsDevice_Metal(ValidationMode validationMode_, GPUPreference preference)
	{
		wi::Timer timer;
		device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
		commandqueue = NS::TransferPtr(device->newCommandQueue());
		allocationhandler = wi::allocator::make_shared_single<AllocationHandler>();
		
		if (device->hasUnifiedMemory())
		{
			capabilities |= GraphicsDeviceCapability::CACHE_COHERENT_UMA;
		}
		
		descriptor_heap_res = NS::TransferPtr(device->newBuffer(bindless_resource_capacity * sizeof(IRDescriptorTableEntry), MTL::ResourceStorageModeShared | MTL::ResourceHazardTrackingModeUntracked));
		descriptor_heap_res->setLabel(NS::String::string("descriptor_heap_res", NS::UTF8StringEncoding));
		
		const uint64_t real_bindless_sampler_capacity = std::min(bindless_sampler_capacity, (uint64_t)device->maxArgumentBufferSamplerCount());
		descriptor_heap_sam = NS::TransferPtr(device->newBuffer(real_bindless_sampler_capacity * sizeof(IRDescriptorTableEntry), MTL::ResourceStorageModeShared | MTL::ResourceHazardTrackingModeUntracked));
		descriptor_heap_sam->setLabel(NS::String::string("descriptor_heap_sam", NS::UTF8StringEncoding));
		
		allocationhandler->descriptor_heap_res_data = (IRDescriptorTableEntry*)descriptor_heap_res->contents();
		allocationhandler->descriptor_heap_sam_data = (IRDescriptorTableEntry*)descriptor_heap_sam->contents();
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
		if(error != nullptr)
		{
			NS::String* errDesc = error->localizedDescription();
			wilog_error("%s", errDesc->utf8String());
			assert(0);
			error->release();
		}
		allocationhandler->make_resident(descriptor_heap_res.get());
		allocationhandler->make_resident(descriptor_heap_sam.get());
		
		for (auto& frame : frame_resources)
		{
			frame.event = NS::TransferPtr(device->newSharedEvent());
			frame.event->setSignaledValue(1);
		}
		
		wilog("Created GraphicsDevice_Metal (%d ms)", (int)std::round(timer.elapsed()));
	}
	GraphicsDevice_Metal::~GraphicsDevice_Metal()
	{
	}

	bool GraphicsDevice_Metal::CreateSwapChain(const SwapChainDesc* desc, wi::platform::window_type window, SwapChain* swapchain) const
	{
		auto internal_state = swapchain->IsValid() ? wi::allocator::shared_ptr<SwapChain_Metal>(swapchain->internal_state) : wi::allocator::make_shared<SwapChain_Metal>();
		internal_state->allocationhandler = allocationhandler;
		swapchain->internal_state = internal_state;
		swapchain->desc = *desc;
		
		if(internal_state->view == nullptr)
		{
			CGRect frame = (CGRect){ {0.0f, 0.0f}, {float(desc->width), float(desc->height)} };
			internal_state->view = (MTK::View*)window;
			internal_state->view->init(frame, device.get());
			internal_state->view->setColorPixelFormat(_ConvertPixelFormat(desc->format));
			internal_state->view->setClearColor(MTL::ClearColor::Make(desc->clear_color[0], desc->clear_color[1], desc->clear_color[2], desc->clear_color[3]));
		}

		return true;
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
		
		MTL::ResourceOptions options = {};
		if (desc->usage == Usage::DEFAULT)
		{
			if (CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA))
			{
				options |= MTL::ResourceStorageModeShared;
			}
			else
			{
				options |= MTL::ResourceStorageModePrivate;
			}
		}
		else if (desc->usage == Usage::UPLOAD)
		{
			options |= MTL::ResourceStorageModeShared;
			options |= MTL::ResourceCPUCacheModeWriteCombined;
		}
		else if (desc->usage == Usage::READBACK)
		{
			options |= MTL::ResourceStorageModeShared;
		}
		options |= MTL::ResourceHazardTrackingModeUntracked;
		
		if (has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_BUFFER))
		{
			// This is an aliasing storage:
			NS::SharedPtr<MTL::HeapDescriptor> heap_desc = NS::TransferPtr(MTL::HeapDescriptor::alloc()->init());
			heap_desc->setResourceOptions(options);
			heap_desc->setSize(desc->size);
			heap_desc->setType(MTL::HeapTypePlacement);
			NS::SharedPtr<MTL::Heap> heap = NS::TransferPtr(device->newHeap(heap_desc.get()));
			internal_state->buffer = NS::TransferPtr(heap->newBuffer(desc->size, options, 0));
			internal_state->buffer->makeAliasable();
		}
		else if (alias != nullptr)
		{
			// This is an aliasing view:
			if (alias->IsBuffer())
			{
				auto alias_internal = to_internal<GPUBuffer>(alias);
				internal_state->buffer = NS::TransferPtr(alias_internal->buffer->heap()->newBuffer(desc->size, options, alias_internal->buffer->heapOffset() + alias_offset));
			}
			else if (alias->IsTexture())
			{
				auto alias_internal = to_internal<Texture>(alias);
				internal_state->buffer = NS::TransferPtr(alias_internal->texture->heap()->newBuffer(desc->size, options, alias_internal->texture->heapOffset() + alias_offset));
			}
		}
		else
		{
			// This is a standalone buffer:
			internal_state->buffer = NS::TransferPtr(device->newBuffer(desc->size, options));
		}
		
		allocationhandler->make_resident(internal_state->buffer.get());
		internal_state->gpu_address = internal_state->buffer->gpuAddress();
		if ((options & MTL::ResourceStorageModePrivate) == 0)
		{
			buffer->mapped_data = internal_state->buffer->contents();
			buffer->mapped_size = internal_state->buffer->allocatedSize();
		}
		
		if (init_callback)
		{
			if (buffer->mapped_data == nullptr)
			{
				wilog_assert(0, "Buffer upload for discrete GPU is not implemented yet!");
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
		
		MTL::ResourceOptions resource_options = MTL::ResourceHazardTrackingModeUntracked;
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
		else
		{
			resource_options |= MTL::ResourceStorageModeShared;
			descriptor->setStorageMode(MTL::StorageModeShared);
		}
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
		
		internal_state->texture = NS::TransferPtr(device->newTexture(descriptor.get()));
		allocationhandler->make_resident(internal_state->texture.get());
		
		if (initial_data != nullptr)
		{
			const uint32_t data_stride = GetFormatStride(desc->format);
			uint32_t initDataIdx = 0;
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
					if (descriptor->storageMode() == MTL::StorageModePrivate)
					{
						wilog_assert(0, "Texture upload in StorageModePrivate for discrete GPU is not implemented yet!");
					}
					else
					{
						internal_state->texture->replaceRegion(region, mip, slice, subresourceData.data_ptr, subresourceData.row_pitch, datasize);
					}
					width = std::max(block_size, width / 2);
					height = std::max(block_size, height / 2);
					depth = std::max(1u, depth / 2);
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
		
		return internal_state->texture.get() != nullptr;
	}
	bool GraphicsDevice_Metal::CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader) const
	{
		auto internal_state = wi::allocator::make_shared<Shader_Metal>();
		internal_state->allocationhandler = allocationhandler;
		shader->internal_state = internal_state;
		
		dispatch_data_t bytecodeData = dispatch_data_create(shadercode, shadercode_size, dispatch_get_main_queue(), nullptr);
		NS::Error* error = nullptr;
		internal_state->library = NS::TransferPtr(device->newLibrary(bytecodeData, &error));
		if(error != nullptr)
		{
			NS::String* errDesc = error->localizedDescription();
			wilog_error("%s", errDesc->utf8String());
			assert(0);
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
			if(error != nullptr)
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
		internal_state->sampler = NS::TransferPtr(device->newSamplerState(descriptor.get()));
		internal_state->index = allocationhandler->allocate_bindless(internal_state->sampler.get());

		return internal_state->sampler.get() != nullptr;
	}
	bool GraphicsDevice_Metal::CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const
	{
		auto internal_state = wi::allocator::make_shared<QueryHeap_Metal>();
		internal_state->allocationhandler = allocationhandler;
		queryheap->internal_state = internal_state;
		queryheap->desc = *desc;
		
		internal_state->buffer = NS::TransferPtr(device->newBuffer(desc->query_count * sizeof(uint64_t), MTL::ResourceStorageModePrivate));

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
		
		const DepthStencilState& dss = desc->dss == nullptr ? DepthStencilState() : *desc->dss;
		NS::SharedPtr<MTL::DepthStencilDescriptor> depth_stencil_desc = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
		if (dss.depth_enable)
		{
			depth_stencil_desc->setDepthCompareFunction(_ConvertCompareFunction(dss.depth_func));
			depth_stencil_desc->setDepthWriteEnabled(dss.depth_write_mask == DepthWriteMask::ALL);
		}
		NS::SharedPtr<MTL::StencilDescriptor> stencil_front;
		NS::SharedPtr<MTL::StencilDescriptor> stencil_back;
		if (dss.stencil_enable)
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
		
		NS::SharedPtr<MTL::VertexDescriptor> vertex_descriptor;
		if (desc->il != nullptr)
		{
			vertex_descriptor = NS::TransferPtr(MTL::VertexDescriptor::alloc()->init());
			uint64_t offset = 0;
			for (size_t i = 0; i < desc->il->elements.size(); ++i)
			{
				const InputLayout::Element& element = desc->il->elements[i];
				MTL::VertexBufferLayoutDescriptor* layout = vertex_descriptor->layouts()->object(kIRVertexBufferBindPoint + i);
				const uint64_t stride = GetFormatStride(element.format);
				layout->setStride(stride);
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
				offset += stride;
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
			
			NS::Error* error = nullptr;
			internal_state->render_pipeline = NS::TransferPtr(device->newRenderPipelineState(internal_state->descriptor.get(), &error));
			if(error != nullptr)
			{
				NS::String* errDesc = error->localizedDescription();
				wilog_error("%s", errDesc->utf8String());
				assert(0);
				error->release();
			}
			
			return internal_state->render_pipeline.get() != nullptr;
		}
		
		// If we get here, this pipeline state is not complete, but it will be reuseable by different render passes (and compiled just in time at runtime)
		return true;
	}
	bool GraphicsDevice_Metal::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const
	{
		//auto internal_state = wi::allocator::make_shared<BVH_Metal>();
		//internal_state->allocationhandler = allocationhandler;
		//bvh->internal_state = internal_state;
		//bvh->type = GPUResource::Type::RAYTRACING_ACCELERATION_STRUCTURE;
		//bvh->desc = *desc;
		//bvh->size = 0;

		return false;
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
		
		sliceCount = std::min(sliceCount, texture->desc.array_size - firstSlice);
		mipCount = std::min(mipCount, texture->desc.mip_levels - firstMip);
		
		switch (type) {
			case SubresourceType::SRV:
				{
					if(internal_state->srv.texture.get() == nullptr)
					{
						auto& subresource = internal_state->srv;
						subresource.texture = NS::TransferPtr(internal_state->texture->newTextureView(_ConvertPixelFormat(format), internal_state->texture->textureType(), {firstMip, mipCount}, {firstSlice, sliceCount}));
						subresource.index = allocationhandler->allocate_bindless(subresource.texture.get(), min_lod_clamp);
						allocationhandler->make_resident(subresource.texture.get());
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_srv.emplace_back();
						subresource.texture = NS::TransferPtr(internal_state->texture->newTextureView(_ConvertPixelFormat(format), internal_state->texture->textureType(), {firstMip, mipCount}, {firstSlice, sliceCount}));
						subresource.index = allocationhandler->allocate_bindless(subresource.texture.get(), min_lod_clamp);
						allocationhandler->make_resident(subresource.texture.get());
						return (int)internal_state->subresources_srv.size() - 1;
					}
				}
				break;
				
			case SubresourceType::UAV:
				{
					if(internal_state->uav.texture.get() == nullptr)
					{
						auto& subresource = internal_state->uav;
						subresource.texture = NS::TransferPtr(internal_state->texture->newTextureView(_ConvertPixelFormat(format), internal_state->texture->textureType(), {firstMip, mipCount}, {firstSlice, sliceCount}));
						subresource.index = allocationhandler->allocate_bindless(subresource.texture.get(), min_lod_clamp);
						allocationhandler->make_resident(subresource.texture.get());
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_uav.emplace_back();
						subresource.texture = NS::TransferPtr(internal_state->texture->newTextureView(_ConvertPixelFormat(format), internal_state->texture->textureType(), {firstMip, mipCount}, {firstSlice, sliceCount}));
						subresource.index = allocationhandler->allocate_bindless(subresource.texture.get(), min_lod_clamp);
						allocationhandler->make_resident(subresource.texture.get());
						return (int)internal_state->subresources_uav.size() - 1;
					}
				}
				break;
				
			case SubresourceType::RTV:
				{
					if(internal_state->rtv.texture.get() == nullptr)
					{
						auto& subresource = internal_state->rtv;
						subresource.texture = NS::TransferPtr(internal_state->texture->newTextureView(_ConvertPixelFormat(format), internal_state->texture->textureType(), {firstMip, mipCount}, {firstSlice, sliceCount}));
						subresource.index = allocationhandler->allocate_bindless(subresource.texture.get(), min_lod_clamp);
						allocationhandler->make_resident(subresource.texture.get());
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_rtv.emplace_back();
						subresource.texture = NS::TransferPtr(internal_state->texture->newTextureView(_ConvertPixelFormat(format), internal_state->texture->textureType(), {firstMip, mipCount}, {firstSlice, sliceCount}));
						subresource.index = allocationhandler->allocate_bindless(subresource.texture.get(), min_lod_clamp);
						allocationhandler->make_resident(subresource.texture.get());
						return (int)internal_state->subresources_rtv.size() - 1;
					}
				}
				break;
				
			case SubresourceType::DSV:
				{
					if(internal_state->dsv.texture.get() == nullptr)
					{
						auto& subresource = internal_state->dsv;
						subresource.texture = NS::TransferPtr(internal_state->texture->newTextureView(_ConvertPixelFormat(format), internal_state->texture->textureType(), {firstMip, mipCount}, {firstSlice, sliceCount}));
						subresource.index = allocationhandler->allocate_bindless(subresource.texture.get(), min_lod_clamp);
						allocationhandler->make_resident(subresource.texture.get());
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_dsv.emplace_back();
						subresource.texture = NS::TransferPtr(internal_state->texture->newTextureView(_ConvertPixelFormat(format), internal_state->texture->textureType(), {firstMip, mipCount}, {firstSlice, sliceCount}));
						subresource.index = allocationhandler->allocate_bindless(subresource.texture.get(), min_lod_clamp);
						allocationhandler->make_resident(subresource.texture.get());
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
		
		Format format = buffer->GetDesc().format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}
		
		size = std::min(size, buffer->desc.size - offset);
		
		switch (type) {
			case SubresourceType::SRV:
				{
					if(internal_state->srv.buffer.get() == nullptr)
					{
						auto& subresource = internal_state->srv;
						subresource.buffer = internal_state->buffer;
						subresource.offset = offset;
						subresource.size = size;
						if (format != Format::UNKNOWN)
						{
							NS::SharedPtr<MTL::TextureDescriptor> view_descriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
							view_descriptor->textureBufferDescriptor(_ConvertPixelFormat(format), size / GetFormatStride(format), subresource.buffer->resourceOptions(), MTL::TextureUsageShaderRead | MTL::TextureUsagePixelFormatView);
							subresource.texture_buffer_view = NS::TransferPtr(device->newTexture(view_descriptor.get()));
						}
						subresource.index = allocationhandler->allocate_bindless(subresource.buffer.get(), size, offset, subresource.texture_buffer_view.get(), format);
						allocationhandler->make_resident(subresource.buffer.get());
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_srv.emplace_back();
						subresource.buffer = internal_state->buffer;
						subresource.offset = offset;
						subresource.size = size;
						if (format != Format::UNKNOWN)
						{
							NS::SharedPtr<MTL::TextureDescriptor> view_descriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
							view_descriptor->textureBufferDescriptor(_ConvertPixelFormat(format), size / GetFormatStride(format), subresource.buffer->resourceOptions(), MTL::TextureUsageShaderRead | MTL::TextureUsagePixelFormatView);
							subresource.texture_buffer_view = NS::TransferPtr(device->newTexture(view_descriptor.get()));
						}
						subresource.index = allocationhandler->allocate_bindless(subresource.buffer.get(), size, offset, subresource.texture_buffer_view.get(), format);
						allocationhandler->make_resident(subresource.buffer.get());
						return (int)internal_state->subresources_srv.size() - 1;
					}
				}
				break;
			case SubresourceType::UAV:
				{
					if(internal_state->uav.buffer.get() == nullptr)
					{
						auto& subresource = internal_state->uav;
						subresource.buffer = internal_state->buffer;
						subresource.offset = offset;
						subresource.size = size;
						if (format != Format::UNKNOWN)
						{
							NS::SharedPtr<MTL::TextureDescriptor> view_descriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
							view_descriptor->textureBufferDescriptor(_ConvertPixelFormat(format), size / GetFormatStride(format), subresource.buffer->resourceOptions(), MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsagePixelFormatView);
							subresource.texture_buffer_view = NS::TransferPtr(device->newTexture(view_descriptor.get()));
						}
						subresource.index = allocationhandler->allocate_bindless(subresource.buffer.get(), size, offset);
						allocationhandler->make_resident(subresource.buffer.get());
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_uav.emplace_back();
						subresource.buffer = internal_state->buffer;
						subresource.offset = offset;
						subresource.size = size;
						if (format != Format::UNKNOWN)
						{
							NS::SharedPtr<MTL::TextureDescriptor> view_descriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
							view_descriptor->textureBufferDescriptor(_ConvertPixelFormat(format), size / GetFormatStride(format), subresource.buffer->resourceOptions(), MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsagePixelFormatView);
							subresource.texture_buffer_view = NS::TransferPtr(device->newTexture(view_descriptor.get()));
						}
						subresource.index = allocationhandler->allocate_bindless(subresource.buffer.get(), size, offset);
						allocationhandler->make_resident(subresource.buffer.get());
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
		if (resource->IsTexture())
		{
			auto internal_state = to_internal((Texture*)resource);
			internal_state->allocationhandler->destroylocker.lock();
			internal_state->destroy_subresources();
			internal_state->allocationhandler->destroylocker.unlock();
		}
		else if (resource->IsBuffer())
		{
			auto internal_state = to_internal((GPUBuffer*)resource);
			internal_state->allocationhandler->destroylocker.lock();
			internal_state->destroy_subresources();
			internal_state->allocationhandler->destroylocker.unlock();
		}
	}

	int GraphicsDevice_Metal::GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource) const
	{
		if (resource == nullptr || !resource->IsValid())
			return -1;

		if (resource->IsTexture())
		{
			auto internal_state = to_internal<Texture>(resource);
			switch (type) {
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
			switch (type) {
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
	}
	void GraphicsDevice_Metal::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const
	{
	}

	void GraphicsDevice_Metal::SetName(GPUResource* pResource, const char* name) const
	{
		if (pResource == nullptr || !pResource->IsValid())
			return;
		if (pResource->IsTexture())
		{
			auto internal_state = to_internal<Texture>(pResource);
			internal_state->texture->setLabel(NS::String::string(name, NS::UTF8StringEncoding));
		}
		else if (pResource->IsBuffer())
		{
			auto internal_state = to_internal<GPUBuffer>(pResource);
			internal_state->buffer->setLabel(NS::String::string(name, NS::UTF8StringEncoding));
		}
		else if (pResource->IsAccelerationStructure())
		{
			auto internal_state = to_internal<RaytracingAccelerationStructure>(pResource);
			// TODO
		}
	}
	void GraphicsDevice_Metal::SetName(Shader* shader, const char* name) const
	{
		if (shader == nullptr || !shader->IsValid())
			return;
		auto internal_state = to_internal(shader);
		internal_state->library->setLabel(NS::String::string(name, NS::UTF8StringEncoding));
	}

	CommandList GraphicsDevice_Metal::BeginCommandList(QUEUE_TYPE queue)
	{
		cmd_locker.lock();
		uint32_t cmd_current = cmd_count++;
		if (cmd_current >= commandlists.size())
		{
			commandlists.push_back(std::make_unique<CommandList_Metal>());
		}
		CommandList cmd;
		cmd.internal_state = commandlists[cmd_current].get();
		cmd_locker.unlock();

		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.reset(GetBufferIndex());
		commandlist.queue = queue;
		commandlist.id = cmd_current;
		commandlist.commandbuffer = commandqueue->commandBufferWithUnretainedReferences();
		commandlist.commandbuffer->useResidencySet(allocationhandler->residency_set.get());
		
		return cmd;
	}
	void GraphicsDevice_Metal::SubmitCommandLists()
	{
		allocationhandler->residency_set->commit();
		
		uint32_t cmd_last = cmd_count;
		cmd_count = 0;
		for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
		{
			auto& commandlist = *commandlists[cmd].get();
			if (commandlist.render_encoder != nullptr)
			{
				commandlist.render_encoder->endEncoding();
			}
			if (commandlist.compute_encoder != nullptr)
			{
				commandlist.compute_encoder->endEncoding();
			}
			if (commandlist.blit_encoder != nullptr)
			{
				commandlist.blit_encoder->endEncoding();
			}
			for (MTK::View* view : commandlist.presents)
			{
				commandlist.commandbuffer->presentDrawable(view->currentDrawable());
			}
			commandlist.commandbuffer->commit();
			
			if (cmd == cmd_last - 1)
			{
				commandlist.commandbuffer->encodeSignalEvent(GetFrameResources().event.get(), 1);
			}
			
			for (auto& x : commandlist.pipelines_worker)
			{
				if (pipelines_global.count(x.first) == 0)
				{
				   pipelines_global[x.first] = std::move(x.second);
				}
				else
				{
				   allocationhandler->destroylocker.lock();
				   allocationhandler->destroyer_render_pipelines.push_back(std::make_pair(x.second, FRAMECOUNT));
				   allocationhandler->destroylocker.unlock();
				}
			}
			commandlist.pipelines_worker.clear();
		}
		
		// From here, we begin a new frame, this affects GetBufferIndex()!
		FRAMECOUNT++;
		
		// The new frame event must be completed when we start using it from CPU:
		GetFrameResources().event->waitUntilSignaledValue(1, ~0ull);
		GetFrameResources().event->setSignaledValue(0);

		allocationhandler->Update(FRAMECOUNT, BUFFERCOUNT);
	}

	void GraphicsDevice_Metal::WaitForGPU() const
	{
		MTL::CommandBuffer* cmd = commandqueue->commandBufferWithUnretainedReferences();
		cmd->commit();
		cmd->waitUntilCompleted();
	}
	void GraphicsDevice_Metal::ClearPipelineStateCache()
	{
		std::scoped_lock locker(allocationhandler->destroylocker);
		uint64_t framecount = allocationhandler->framecount;
		for (auto& x : pipelines_global)
		{
			allocationhandler->destroyer_render_pipelines.push_back(std::make_pair(std::move(x.second), framecount));
		}
		pipelines_global.clear();
	}

	Texture GraphicsDevice_Metal::GetBackBuffer(const SwapChain* swapchain) const
	{
		auto swapchain_internal = to_internal(swapchain);
		Texture result;
		// TODO
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
		// TODO
	}

	void GraphicsDevice_Metal::WaitCommandList(CommandList cmd, CommandList wait_for)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		CommandList_Metal& commandlist_wait_for = GetCommandList(wait_for);
		assert(commandlist_wait_for.id < commandlist.id); // can't wait for future command list!
	}
	void GraphicsDevice_Metal::RenderPassBegin(const SwapChain* swapchain, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(commandlist.render_encoder == nullptr);
		if (commandlist.compute_encoder != nullptr)
		{
			commandlist.compute_encoder->endEncoding();
			commandlist.compute_encoder = nullptr;
		}
		if (commandlist.blit_encoder != nullptr)
		{
			commandlist.blit_encoder->endEncoding();
			commandlist.blit_encoder = nullptr;
		}
		
		auto internal_state = to_internal(swapchain);
		
		commandlist.presents.push_back(internal_state->view);
		
		MTL::RenderPassDescriptor* renderpass_descriptor = internal_state->view->currentRenderPassDescriptor();
		commandlist.render_encoder = commandlist.commandbuffer->renderCommandEncoder(renderpass_descriptor);

		commandlist.renderpass_info = RenderPassInfo::from(swapchain->desc);
	}
	void GraphicsDevice_Metal::RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(commandlist.render_encoder == nullptr);
		if (commandlist.compute_encoder != nullptr)
		{
			commandlist.compute_encoder->endEncoding();
			commandlist.compute_encoder = nullptr;
		}
		if (commandlist.blit_encoder != nullptr)
		{
			commandlist.blit_encoder->endEncoding();
			commandlist.blit_encoder = nullptr;
		}
		
		static thread_local NS::SharedPtr<MTL::RenderPassDescriptor> descriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc());
		static thread_local NS::SharedPtr<MTL::RenderPassColorAttachmentDescriptor> color_attachment_descriptors[8] = {
			NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()),
			NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()),
			NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()),
			NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()),
			NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()),
			NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()),
			NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()),
			NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()),
		};
		static thread_local NS::SharedPtr<MTL::RenderPassDepthAttachmentDescriptor> depth_attachment_descriptor = NS::TransferPtr(MTL::RenderPassDepthAttachmentDescriptor::alloc());
		static thread_local NS::SharedPtr<MTL::RenderPassStencilAttachmentDescriptor> stencil_attachment_descriptor = NS::TransferPtr(MTL::RenderPassStencilAttachmentDescriptor::alloc());
		
		descriptor->init();
		descriptor->setDefaultRasterSampleCount(1);
		
		if (image_count > 0)
		{
			descriptor->setRenderTargetWidth(images[0].texture->desc.width);
			descriptor->setRenderTargetHeight(images[0].texture->desc.height);
			descriptor->setRenderTargetArrayLength(images[0].texture->desc.array_size);
		}
		
		uint32_t color_attachment_index = 0;
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
					MTL::RenderPassColorAttachmentDescriptor* color_attachment_descriptor = color_attachment_descriptors[i].get();
					color_attachment_descriptor->init();
					color_attachment_descriptor->setTexture(internal_state->texture.get());
					color_attachment_descriptor->setClearColor(MTL::ClearColor::Make(image.texture->desc.clear.color[0], image.texture->desc.clear.color[1], image.texture->desc.clear.color[2], image.texture->desc.clear.color[3]));
					color_attachment_descriptor->setLoadAction(load_action);
					color_attachment_descriptor->setStoreAction(store_action);
					descriptor->colorAttachments()->setObject(color_attachment_descriptor, color_attachment_index);
					color_attachment_index++;
					break;
				}
				case RenderPassImage::Type::DEPTH_STENCIL:
				{
					depth_attachment_descriptor->init();
					depth_attachment_descriptor->setTexture(internal_state->texture.get());
					depth_attachment_descriptor->setClearDepth(image.texture->desc.clear.depth_stencil.depth);
					depth_attachment_descriptor->setLoadAction(load_action);
					depth_attachment_descriptor->setStoreAction(store_action);
					descriptor->setDepthAttachment(depth_attachment_descriptor.get());
					if (IsFormatStencilSupport(image.texture->desc.format))
					{
						stencil_attachment_descriptor->init();
						stencil_attachment_descriptor->setTexture(internal_state->texture.get());
						stencil_attachment_descriptor->setClearStencil(image.texture->desc.clear.depth_stencil.stencil);
						stencil_attachment_descriptor->setLoadAction(load_action);
						stencil_attachment_descriptor->setStoreAction(store_action);
						descriptor->setStencilAttachment(stencil_attachment_descriptor.get());
					}
					break;
				}
				case RenderPassImage::Type::RESOLVE:
					break;
				case RenderPassImage::Type::RESOLVE_DEPTH:
					break;
				default:
					assert(0);
					break;
			}
		}
		
		commandlist.render_encoder = commandlist.commandbuffer->renderCommandEncoder(descriptor.get());

		commandlist.renderpass_info = RenderPassInfo::from(images, image_count);
	}
	void GraphicsDevice_Metal::RenderPassEnd(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(commandlist.render_encoder != nullptr);
		
		commandlist.render_encoder->endEncoding();

		commandlist.renderpass_info = {};
		commandlist.render_encoder = nullptr;
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
		if (slot >= arraysize(RootLayout::root_cbv))
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
			vb.buffer = *vertexBuffers[i];
			vb.offset = offsets == nullptr ? 0 : offsets[i];
			vb.stride = strides[i];
		}
	}
	void GraphicsDevice_Metal::BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd)
	{
		if (indexBuffer == nullptr)
			return;
		auto internal_state = to_internal(indexBuffer);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.index_buffer = internal_state->buffer;
		commandlist.index_buffer_offset = offset;
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
		
		if(internal_state->render_pipeline.get() == nullptr)
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
	}
	void GraphicsDevice_Metal::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.active_cs = cs;
	}
	void GraphicsDevice_Metal::BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.render_encoder->setDepthTestBounds(min_bounds, max_bounds);
	}
	void GraphicsDevice_Metal::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		IRRuntimeDrawPrimitives(commandlist.render_encoder, commandlist.primitive_type, startVertexLocation, vertexCount);
	}
	void GraphicsDevice_Metal::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		const uint64_t index_stride = commandlist.index_type == MTL::IndexTypeUInt32 ? sizeof(uint32_t) : sizeof(uint16_t);
		const uint64_t indexBufferOffset = commandlist.index_buffer_offset + startIndexLocation * index_stride;
		IRRuntimeDrawIndexedPrimitives(commandlist.render_encoder, commandlist.primitive_type, indexCount, commandlist.index_type, commandlist.index_buffer.get(), indexBufferOffset);
	}
	void GraphicsDevice_Metal::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		IRRuntimeDrawPrimitives(commandlist.render_encoder, commandlist.primitive_type, startVertexLocation, vertexCount, instanceCount, startInstanceLocation);
	}
	void GraphicsDevice_Metal::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		const uint64_t index_stride = commandlist.index_type == MTL::IndexTypeUInt32 ? sizeof(uint32_t) : sizeof(uint16_t);
		const uint64_t indexBufferOffset = commandlist.index_buffer_offset + startIndexLocation * index_stride;
		IRRuntimeDrawIndexedPrimitives(commandlist.render_encoder, commandlist.primitive_type, indexCount, commandlist.index_type, commandlist.index_buffer.get(), indexBufferOffset, instanceCount, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_Metal::DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		IRRuntimeDrawPrimitives(commandlist.render_encoder, commandlist.primitive_type, internal_state->buffer.get(), args_offset);
	}
	void GraphicsDevice_Metal::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		IRRuntimeDrawIndexedPrimitives(commandlist.render_encoder, commandlist.primitive_type, commandlist.index_type, commandlist.index_buffer.get(), commandlist.index_buffer_offset, internal_state->buffer.get(), args_offset);
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
		commandlist.compute_encoder->dispatchThreadgroups({threadGroupCountX, threadGroupCountY, threadGroupCountZ}, {1, 1, 1});
	}
	void GraphicsDevice_Metal::DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		auto internal_state = to_internal(args);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.compute_encoder->dispatchThreadgroups(internal_state->buffer.get(), args_offset, {1, 1, 1});
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
		if (pDst->IsTexture())
		{
			auto src_internal = to_internal<Texture>(pSrc);
			auto dst_internal = to_internal<Texture>(pDst);
			commandlist.blit_encoder->copyFromTexture(src_internal->texture.get(), dst_internal->texture.get());
		}
		else if (pDst->IsBuffer())
		{
			auto src_internal = to_internal<GPUBuffer>(pSrc);
			auto dst_internal = to_internal<GPUBuffer>(pDst);
			commandlist.blit_encoder->copyFromBuffer(src_internal->buffer.get(), 0, dst_internal->buffer.get(), 0, dst_internal->buffer->length());
		}
	}
	void GraphicsDevice_Metal::CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd)
	{
		precopy(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);
		commandlist.blit_encoder->copyFromBuffer(internal_state_src->buffer.get(), src_offset, internal_state_dst->buffer.get(), dst_offset, size);
	}
	void GraphicsDevice_Metal::CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox, ImageAspect dst_aspect, ImageAspect src_aspect)
	{
		precopy(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto src_internal = to_internal(src);
		auto dst_internal = to_internal(dst);
		MTL::Origin srcOrigin = {};
		MTL::Size srcSize = {};
		MTL::Origin dstOrigin = { dstX, dstY, dstZ };
		if (srcbox != nullptr)
		{
			srcOrigin.x = srcbox->left;
			srcOrigin.y = srcbox->top;
			srcOrigin.z = srcbox->front;
			srcSize.width = srcbox->right - srcbox->left;
			srcSize.height = srcbox->bottom - srcbox->top;
			srcSize.depth = srcbox->back - srcbox->front;
		}
		commandlist.blit_encoder->copyFromTexture(src_internal->texture.get(), srcSlice, srcMip, srcOrigin, srcSize, dst_internal->texture.get(), dstSlice, dstMip, dstOrigin);
	}
	void GraphicsDevice_Metal::QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

	}
	void GraphicsDevice_Metal::QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

	}
	void GraphicsDevice_Metal::QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);
		auto dst_internal = to_internal(dest);

	}
	void GraphicsDevice_Metal::QueryReset(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);

	}
	void GraphicsDevice_Metal::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);

	}
	void GraphicsDevice_Metal::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto dst_internal = to_internal(dst);

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
			commandlist.blit_encoder->fillBuffer(internal_state->buffer.get(), {0, internal_state->buffer->length()}, (uint8_t)value);
		}
	}
	void GraphicsDevice_Metal::VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd)
	{
	}

	void GraphicsDevice_Metal::EventBegin(const char* name, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.render_encoder != nullptr)
		{
			commandlist.render_encoder->pushDebugGroup(NS::String::string(name, NS::UTF8StringEncoding));
		}
		else if (commandlist.compute_encoder != nullptr)
		{
			commandlist.compute_encoder->pushDebugGroup(NS::String::string(name, NS::UTF8StringEncoding));
		}
		else if (commandlist.blit_encoder != nullptr)
		{
			commandlist.blit_encoder->pushDebugGroup(NS::String::string(name, NS::UTF8StringEncoding));
		}
		else
		{
			commandlist.commandbuffer->pushDebugGroup(NS::String::string(name, NS::UTF8StringEncoding));
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
		else if (commandlist.blit_encoder != nullptr)
		{
			commandlist.blit_encoder->popDebugGroup();
		}
		else
		{
			commandlist.commandbuffer->popDebugGroup();
		}
	}
	void GraphicsDevice_Metal::SetMarker(const char* name, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.render_encoder != nullptr)
		{
			commandlist.render_encoder->setLabel(NS::String::string(name, NS::UTF8StringEncoding));
		}
		else if (commandlist.compute_encoder != nullptr)
		{
			commandlist.compute_encoder->setLabel(NS::String::string(name, NS::UTF8StringEncoding));
		}
		else if (commandlist.blit_encoder != nullptr)
		{
			commandlist.blit_encoder->setLabel(NS::String::string(name, NS::UTF8StringEncoding));
		}
		else
		{
			commandlist.commandbuffer->setLabel(NS::String::string(name, NS::UTF8StringEncoding));
		}
	}
}

#endif // __APPLE__
