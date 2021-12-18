#pragma once
#include "CommonInclude.h"
#include "wiVector.h"

#include <cassert>
#include <memory>
#include <string>
#include <limits>

namespace wi::graphics
{
	struct Shader;
	struct GPUResource;
	struct GPUBuffer;
	struct Texture;

	enum class ShaderStage
	{
		MS,		// Mesh Shader
		AS,		// Amplification Shader
		VS,		// Vertex Shader
		HS,		// Hull Shader
		DS,		// Domain Shader
		GS,		// Geometry Shader
		PS,		// Pixel Shader
		CS,		// Compute Shader
		LIB,	// Shader Library
		Count,
	};
	enum class ShaderFormat
	{
		NONE,	// Not used
		HLSL5,	// DXBC
		HLSL6,	// DXIL
		SPIRV,	// SPIR-V
	};
	enum class ShaderModel
	{
		SM_5_0,
		SM_6_0,
		SM_6_1,
		SM_6_2,
		SM_6_3,
		SM_6_4,
		SM_6_5,
		SM_6_6,
		SM_6_7,
	};
	enum class PrimitiveTopology
	{
		UNDEFINED,
		TRIANGLELIST,
		TRIANGLESTRIP,
		POINTLIST,
		LINELIST,
		LINESTRIP,
		PATCHLIST,
	};
	enum class ComparisonFunc
	{
		NEVER,
		LESS,
		EQUAL,
		LESS_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_EQUAL,
		ALWAYS,
	};
	enum class DepthWriteMask
	{
		ZERO,	// Disables depth write
		ALL,	// Enables depth write
	};
	enum class StencilOp
	{
		KEEP,
		ZERO,
		REPLACE,
		INCR_SAT,
		DECR_SAT,
		INVERT,
		INCR,
		DECR,
	};
	enum class Blend
	{
		ZERO,
		ONE,
		SRC_COLOR,
		INV_SRC_COLOR,
		SRC_ALPHA,
		INV_SRC_ALPHA,
		DEST_ALPHA,
		INV_DEST_ALPHA,
		DEST_COLOR,
		INV_DEST_COLOR,
		SRC_ALPHA_SAT,
		BLEND_FACTOR,
		INV_BLEND_FACTOR,
		SRC1_COLOR,
		INV_SRC1_COLOR,
		SRC1_ALPHA,
		INV_SRC1_ALPHA,
	}; 
	enum class BlendOp
	{
		ADD,
		SUBTRACT,
		REV_SUBTRACT,
		MIN,
		MAX,
	};
	enum class FillMode
	{
		WIREFRAME,
		SOLID,
	};
	enum class CullMode
	{
		NONE,
		FRONT,
		BACK,
	};
	enum class InputClassification
	{
		PER_VERTEX_DATA,
		PER_INSTANCE_DATA,
	};
	enum class Usage
	{
		DEFAULT,	// CPU no access, GPU read/write
		UPLOAD,	    // CPU write, GPU read
		READBACK,	// CPU read, GPU write
	};
	enum class TextureAddressMode
	{
		WRAP,
		MIRROR,
		CLAMP,
		BORDER,
		MIRROR_ONCE,
	};
	enum class Filter
	{
		MIN_MAG_MIP_POINT,
		MIN_MAG_POINT_MIP_LINEAR,
		MIN_POINT_MAG_LINEAR_MIP_POINT,
		MIN_POINT_MAG_MIP_LINEAR,
		MIN_LINEAR_MAG_MIP_POINT,
		MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MIN_MAG_LINEAR_MIP_POINT,
		MIN_MAG_MIP_LINEAR,
		ANISOTROPIC,
		COMPARISON_MIN_MAG_MIP_POINT,
		COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
		COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
		COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
		COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
		COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		COMPARISON_MIN_MAG_MIP_LINEAR,
		COMPARISON_ANISOTROPIC,
		MINIMUM_MIN_MAG_MIP_POINT,
		MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
		MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
		MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
		MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
		MINIMUM_MIN_MAG_MIP_LINEAR,
		MINIMUM_ANISOTROPIC,
		MAXIMUM_MIN_MAG_MIP_POINT,
		MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
		MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
		MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
		MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
		MAXIMUM_MIN_MAG_MIP_LINEAR,
		MAXIMUM_ANISOTROPIC,
	};
	enum class SamplerBorderColor
	{
		TRANSPARENT_BLACK,
		OPAQUE_BLACK,
		OPAQUE_WHITE,
	};
	enum class Format
	{
		UNKNOWN,

		R32G32B32A32_FLOAT,
		R32G32B32A32_UINT,
		R32G32B32A32_SINT,

		R32G32B32_FLOAT,
		R32G32B32_UINT,
		R32G32B32_SINT,

		R16G16B16A16_FLOAT,
		R16G16B16A16_UNORM,
		R16G16B16A16_UINT,
		R16G16B16A16_SNORM,
		R16G16B16A16_SINT,

		R32G32_FLOAT,
		R32G32_UINT,
		R32G32_SINT,
		R32G8X24_TYPELESS,		// depth (32-bit) + stencil (8-bit) + shader resource (32-bit)
		D32_FLOAT_S8X24_UINT,	// depth (32-bit) + stencil (8-bit)

		R10G10B10A2_UNORM,
		R10G10B10A2_UINT,
		R11G11B10_FLOAT,
		R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB,
		R8G8B8A8_UINT,
		R8G8B8A8_SNORM,
		R8G8B8A8_SINT, 
		B8G8R8A8_UNORM,
		B8G8R8A8_UNORM_SRGB,
		R16G16_FLOAT,
		R16G16_UNORM,
		R16G16_UINT,
		R16G16_SNORM,
		R16G16_SINT,
		R32_TYPELESS,			// depth (32-bit) + shader resource (32-bit)
		D32_FLOAT,				// depth (32-bit)
		R32_FLOAT,
		R32_UINT,
		R32_SINT, 
		R24G8_TYPELESS,			// depth (24-bit) + stencil (8-bit) + shader resource (24-bit)
		D24_UNORM_S8_UINT,		// depth (24-bit) + stencil (8-bit)

		R8G8_UNORM,
		R8G8_UINT,
		R8G8_SNORM,
		R8G8_SINT,
		R16_TYPELESS,			// depth (16-bit) + shader resource (16-bit)
		R16_FLOAT,
		D16_UNORM,				// depth (16-bit)
		R16_UNORM,
		R16_UINT,
		R16_SNORM,
		R16_SINT,

		R8_UNORM,
		R8_UINT,
		R8_SNORM,
		R8_SINT,

		BC1_UNORM,
		BC1_UNORM_SRGB,
		BC2_UNORM,
		BC2_UNORM_SRGB,
		BC3_UNORM,
		BC3_UNORM_SRGB,
		BC4_UNORM,
		BC4_SNORM,
		BC5_UNORM,
		BC5_SNORM,
		BC6H_UF16,
		BC6H_SF16,
		BC7_UNORM,
		BC7_UNORM_SRGB
	};
	enum class GpuQueryType
	{
		TIMESTAMP,			// retrieve time point of gpu execution
		OCCLUSION,			// how many samples passed depth test?
		OCCLUSION_BINARY,	// depth test passed or not?
	};
	enum class IndexBufferFormat
	{
		UINT16,
		UINT32,
	};
	enum class SubresourceType
	{
		SRV, // shader resource view
		UAV, // unordered access view
		RTV, // render target view
		DSV, // depth stencil view
	};

	enum class ShadingRate
	{
		RATE_1X1,	// Default/full shading rate
		RATE_1X2,
		RATE_2X1,
		RATE_2X2,
		RATE_2X4,
		RATE_4X2,
		RATE_4X4,

		RATE_INVALID
	};

	enum class PredicationOp
	{
		EQUAL_ZERO,
		NOT_EQUAL_ZERO,
	};

	// Flags ////////////////////////////////////////////

	enum class ColorWrite
	{
		DISABLE = 0,
		ENABLE_RED = 1 << 0,
		ENABLE_GREEN = 1 << 1,
		ENABLE_BLUE = 1 << 2,
		ENABLE_ALPHA = 1 << 3,
		ENABLE_ALL = ~0,
	};

	enum class BindFlag
	{
		NONE = 0,
		VERTEX_BUFFER = 1 << 0,
		INDEX_BUFFER = 1 << 1,
		CONSTANT_BUFFER = 1 << 2,
		SHADER_RESOURCE = 1 << 3,
		RENDER_TARGET = 1 << 4,
		DEPTH_STENCIL = 1 << 5,
		UNORDERED_ACCESS = 1 << 6,
		SHADING_RATE = 1 << 7,
	};

	enum class ResourceMiscFlag
	{
		NONE = 0,
		TEXTURECUBE = 1 << 0,
		INDIRECT_ARGS = 1 << 1,
		BUFFER_RAW = 1 << 2,
		BUFFER_STRUCTURED = 1 << 3,
		RAY_TRACING = 1 << 4,
		PREDICATION = 1 << 5,
	};

	enum class GraphicsDeviceCapability
	{
		NONE = 0,
		TESSELLATION = 1 << 0,
		CONSERVATIVE_RASTERIZATION = 1 << 1,
		RASTERIZER_ORDERED_VIEWS = 1 << 2,
		UAV_LOAD_FORMAT_COMMON = 1 << 3, // eg: R16G16B16A16_FLOAT, R8G8B8A8_UNORM and more common ones
		UAV_LOAD_FORMAT_R11G11B10_FLOAT = 1 << 4,
		RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS = 1 << 5,
		VARIABLE_RATE_SHADING = 1 << 6,
		VARIABLE_RATE_SHADING_TIER2 = 1 << 7,
		MESH_SHADER = 1 << 8,
		RAYTRACING = 1 << 9,
		PREDICATION = 1 << 10,
		SAMPLER_MINMAX = 1 << 11,
		DEPTH_BOUNDS_TEST = 1 << 12,
	};

	enum class ResourceState
	{
		// Common resource states:
		UNDEFINED = 0,						// invalid state
		SHADER_RESOURCE = 1 << 0,			// shader resource, read only
		SHADER_RESOURCE_COMPUTE = 1 << 1,	// shader resource, read only, non-pixel shader
		UNORDERED_ACCESS = 1 << 2,			// shader resource, write enabled
		COPY_SRC = 1 << 3,					// copy from
		COPY_DST = 1 << 4,					// copy to

		// Texture specific resource states:
		RENDERTARGET = 1 << 5,				// render target, write enabled
		DEPTHSTENCIL = 1 << 6,				// depth stencil, write enabled
		DEPTHSTENCIL_READONLY = 1 << 7,		// depth stencil, read only
		SHADING_RATE_SOURCE = 1 << 8,		// shading rate control per tile

		// GPUBuffer specific resource states:
		VERTEX_BUFFER = 1 << 9,				// vertex buffer, read only
		INDEX_BUFFER = 1 << 10,				// index buffer, read only
		CONSTANT_BUFFER = 1 << 11,			// constant buffer, read only
		INDIRECT_ARGUMENT = 1 << 12,			// argument buffer to DrawIndirect() or DispatchIndirect()
		RAYTRACING_ACCELERATION_STRUCTURE = 1 << 13, // acceleration structure storage or scratch
		PREDICATION = 1 << 14				// storage for predication comparison value
	};

	enum class ColorSpace
	{
		SRGB,			// SDR color space (8 or 10 bits per channel)
		HDR10_ST2084,	// HDR10 color space (10 bits per channel)
		HDR_LINEAR,		// HDR color space (16 bits per channel)
	};


	// Descriptor structs:

	struct Viewport
	{
		float top_left_x = 0;
		float top_left_y = 0;
		float width = 0;
		float height = 0;
		float min_depth = 0;
		float max_depth = 1;
	};

	struct InputLayout
	{
		static const uint32_t APPEND_ALIGNED_ELEMENT = ~0u; // automatically figure out AlignedByteOffset depending on Format

		struct Element
		{
			std::string semantic_name;
			uint32_t semantic_index = 0;
			Format format = Format::UNKNOWN;
			uint32_t input_slot = 0;
			uint32_t aligned_byte_offset = APPEND_ALIGNED_ELEMENT;
			InputClassification input_slot_class = InputClassification::PER_VERTEX_DATA;
		};
		wi::vector<Element> elements;
	};

	union ClearValue
	{
		float color[4];
		struct ClearDepthStencil
		{
			float depth;
			uint32_t stencil;
		} depth_stencil;
	};

	struct TextureDesc
	{
		enum class Type
		{
			TEXTURE_1D,
			TEXTURE_2D,
			TEXTURE_3D,
		} type = Type::TEXTURE_2D;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
		uint32_t array_size = 1;
		uint32_t mip_levels = 1;
		Format format = Format::UNKNOWN;
		uint32_t sample_count = 1;
		Usage usage = Usage::DEFAULT;
		BindFlag bind_flags = BindFlag::NONE;
		ResourceMiscFlag misc_flags = ResourceMiscFlag::NONE;
		ClearValue clear = {};
		ResourceState layout = ResourceState::SHADER_RESOURCE;
	};

	struct SamplerDesc
	{
		Filter filter = Filter::MIN_MAG_MIP_POINT;
		TextureAddressMode address_u = TextureAddressMode::CLAMP;
		TextureAddressMode address_v = TextureAddressMode::CLAMP;
		TextureAddressMode address_w = TextureAddressMode::CLAMP;
		float mip_lod_bias = 0;
		uint32_t max_anisotropy = 0;
		ComparisonFunc comparison_func = ComparisonFunc::NEVER;
		SamplerBorderColor border_color = SamplerBorderColor::TRANSPARENT_BLACK;
		float min_lod = 0;
		float max_lod = std::numeric_limits<float>::max();
	};

	struct RasterizerState
	{
		FillMode fill_mode = FillMode::SOLID;
		CullMode cull_mode = CullMode::NONE;
		bool front_counter_clockwise = false;
		int32_t depth_bias = 0;
		float depth_bias_clamp = 0;
		float slope_scaled_depth_bias = 0;
		bool depth_clip_enable = false;
		bool multisample_enable = false;
		bool antialiased_line_enable = false;
		bool conservative_rasterization_enable = false;
		uint32_t forced_sample_count = 0;
	};

	struct DepthStencilState
	{
		bool depth_enable = false;
		DepthWriteMask depth_write_mask = DepthWriteMask::ZERO;
		ComparisonFunc depth_func = ComparisonFunc::NEVER;
		bool stencil_enable = false;
		uint8_t stencil_read_mask = 0xff;
		uint8_t stencil_write_mask = 0xff;

		struct DepthStencilOp
		{
			StencilOp stencil_fail_op = StencilOp::KEEP;
			StencilOp stencil_depth_fail_op = StencilOp::KEEP;
			StencilOp stencil_pass_op = StencilOp::KEEP;
			ComparisonFunc stencil_func = ComparisonFunc::NEVER;
		};
		DepthStencilOp front_face;
		DepthStencilOp back_face;
		bool depth_bounds_test_enable = false;
	};

	struct BlendState
	{
		bool alpha_to_coverage_enable = false;
		bool independent_blend_enable = false;

		struct RenderTargetBlendState
		{
			bool blend_enable = false;
			Blend src_blend = Blend::SRC_ALPHA;
			Blend dest_blend = Blend::INV_SRC_ALPHA;
			BlendOp blend_op = BlendOp::ADD;
			Blend src_blend_alpha = Blend::ONE;
			Blend dest_blend_alpha = Blend::ONE;
			BlendOp blend_op_alpha = BlendOp::ADD;
			ColorWrite render_target_write_mask = ColorWrite::ENABLE_ALL;
		};
		RenderTargetBlendState render_target[8];
	};

	struct GPUBufferDesc
	{
		uint64_t size = 0;
		Usage usage = Usage::DEFAULT;
		BindFlag bind_flags = BindFlag::NONE;
		ResourceMiscFlag misc_flags = ResourceMiscFlag::NONE;
		uint32_t stride = 0; // needed for typed and structured buffer types!
		Format format = Format::UNKNOWN; // only needed for typed buffer!
	};

	struct GPUQueryHeapDesc
	{
		GpuQueryType type = GpuQueryType::TIMESTAMP;
		uint32_t query_count = 0;
	};

	struct PipelineStateDesc
	{
		const Shader*			vs = nullptr;
		const Shader*			ps = nullptr;
		const Shader*			hs = nullptr;
		const Shader*			ds = nullptr;
		const Shader*			gs = nullptr;
		const Shader*			ms = nullptr;
		const Shader*			as = nullptr;
		const BlendState*		bs = nullptr;
		const RasterizerState*	rs = nullptr;
		const DepthStencilState* dss = nullptr;
		const InputLayout*		il = nullptr;
		PrimitiveTopology		pt = PrimitiveTopology::TRIANGLELIST;
		uint32_t                patch_control_points = 3;
		uint32_t				sample_mask = 0xFFFFFFFF;
	};

	struct GPUBarrier
	{
		enum class Type
		{
			MEMORY,		// UAV accesses
			IMAGE,		// image layout transition
			BUFFER,		// buffer state transition
		} type = Type::MEMORY;

		struct Memory
		{
			const GPUResource* resource;
		};
		struct Image
		{
			const Texture* texture;
			ResourceState layout_before;
			ResourceState layout_after;
			int mip;
			int slice;
		};
		struct Buffer
		{
			const GPUBuffer* buffer;
			ResourceState state_before;
			ResourceState state_after;
		};
		union
		{
			Memory memory;
			Image image;
			Buffer buffer;
		};

		static GPUBarrier Memory(const GPUResource* resource = nullptr)
		{
			GPUBarrier barrier;
			barrier.type = Type::MEMORY;
			barrier.memory.resource = resource;
			return barrier;
		}
		static GPUBarrier Image(const Texture* texture, ResourceState before, ResourceState after,
			int mip = -1, int slice = -1)
		{
			GPUBarrier barrier;
			barrier.type = Type::IMAGE;
			barrier.image.texture = texture;
			barrier.image.layout_before = before;
			barrier.image.layout_after = after;
			barrier.image.mip = mip;
			barrier.image.slice = slice;
			return barrier;
		}
		static GPUBarrier Buffer(const GPUBuffer* buffer, ResourceState before, ResourceState after)
		{
			GPUBarrier barrier;
			barrier.type = Type::BUFFER;
			barrier.buffer.buffer = buffer;
			barrier.buffer.state_before = before;
			barrier.buffer.state_after = after;
			return barrier;
		}
	};

	struct RenderPassAttachment
	{
		enum class Type
		{
			RENDERTARGET,
			DEPTH_STENCIL,
			RESOLVE,
			SHADING_RATE_SOURCE
		} type = Type::RENDERTARGET;
		enum class LoadOp
		{
			LOAD,
			CLEAR,
			DONTCARE,
		} loadop = LoadOp::LOAD;
		const Texture* texture = nullptr;
		int subresource = -1;
		enum class StoreOp
		{
			STORE,
			DONTCARE,
		} storeop = StoreOp::STORE;
		ResourceState initial_layout = ResourceState::UNDEFINED;	// layout before the render pass
		ResourceState subpass_layout = ResourceState::UNDEFINED;	// layout within the render pass
		ResourceState final_layout = ResourceState::UNDEFINED;		// layout after the render pass

		static RenderPassAttachment RenderTarget(
			const Texture* resource = nullptr,
			LoadOp load_op = LoadOp::LOAD,
			StoreOp store_op = StoreOp::STORE,
			ResourceState initial_layout = ResourceState::SHADER_RESOURCE,
			ResourceState subpass_layout = ResourceState::RENDERTARGET,
			ResourceState final_layout = ResourceState::SHADER_RESOURCE
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type::RENDERTARGET;
			attachment.texture = resource;
			attachment.loadop = load_op;
			attachment.storeop = store_op;
			attachment.initial_layout = initial_layout;
			attachment.subpass_layout = subpass_layout;
			attachment.final_layout = final_layout;
			return attachment;
		}

		static RenderPassAttachment DepthStencil(
			const Texture* resource = nullptr,
			LoadOp load_op = LoadOp::LOAD,
			StoreOp store_op = StoreOp::STORE,
			ResourceState initial_layout = ResourceState::DEPTHSTENCIL,
			ResourceState subpass_layout = ResourceState::DEPTHSTENCIL,
			ResourceState final_layout = ResourceState::DEPTHSTENCIL
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type::DEPTH_STENCIL;
			attachment.texture = resource;
			attachment.loadop = load_op;
			attachment.storeop = store_op;
			attachment.initial_layout = initial_layout;
			attachment.subpass_layout = subpass_layout;
			attachment.final_layout = final_layout;
			return attachment;
		}

		static RenderPassAttachment Resolve(
			const Texture* resource = nullptr,
			ResourceState initial_layout = ResourceState::SHADER_RESOURCE,
			ResourceState final_layout = ResourceState::SHADER_RESOURCE
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type::RESOLVE;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
			attachment.final_layout = final_layout;
			return attachment;
		}

		static RenderPassAttachment ShadingRateSource(
			const Texture* resource = nullptr,
			ResourceState initial_layout = ResourceState::SHADING_RATE_SOURCE,
			ResourceState final_layout = ResourceState::SHADING_RATE_SOURCE
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type::SHADING_RATE_SOURCE;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
			attachment.subpass_layout = ResourceState::SHADING_RATE_SOURCE;
			attachment.final_layout = final_layout;
			return attachment;
		}
	};

	struct RenderPassDesc
	{
		enum class Flags
		{
			EMPTY = 0,
			ALLOW_UAV_WRITES = 1 << 0,
		};
		Flags flags = Flags::EMPTY;
		wi::vector<RenderPassAttachment> attachments;
	};

	struct SwapChainDesc
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t buffer_count = 2;
		Format format = Format::R10G10B10A2_UNORM;
		bool fullscreen = false;
		bool vsync = true;
		float clear_color[4] = { 0,0,0,1 };
		bool allow_hdr = true;
	};

	struct IndirectDrawArgsInstanced
	{
		uint32_t vertex_count_per_instance = 0;
		uint32_t instance_count = 0;
		uint32_t start_vertex_location = 0;
		uint32_t start_instance_location = 0;
	};

	struct IndirectDrawArgsIndexedInstanced
	{
		uint32_t index_count_per_instance = 0;
		uint32_t instance_count = 0;
		uint32_t start_index_location = 0;
		int32_t base_vertex_location = 0;
		uint32_t start_instance_location = 0;
	};

	struct IndirectDispatchArgs
	{
		uint32_t thread_group_count_x = 0;
		uint32_t thread_group_count_y = 0;
		uint32_t thread_group_count_z = 0;
	};

	struct SubresourceData
	{
		const void *data_ptr = nullptr;
		uint32_t row_pitch = 0;
		uint32_t slice_pitch = 0;
	};

	struct Rect
	{
		int32_t left = 0;
		int32_t top = 0;
		int32_t right = 0;
		int32_t bottom = 0;
	};


	// Resources:

	struct GraphicsDeviceChild
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }
	};

	struct Sampler : public GraphicsDeviceChild
	{
		SamplerDesc desc;

		const SamplerDesc& GetDesc() const { return desc; }
	};

	struct Shader : public GraphicsDeviceChild
	{
		ShaderStage stage = ShaderStage::Count;
	};

	struct GPUResource : public GraphicsDeviceChild
	{
		enum class Type
		{
			BUFFER,
			TEXTURE,
			RAYTRACING_ACCELERATION_STRUCTURE,
			UNKNOWN_TYPE,
		} type = Type::UNKNOWN_TYPE;
		inline bool IsTexture() const { return type == Type::TEXTURE; }
		inline bool IsBuffer() const { return type == Type::BUFFER; }
		inline bool IsAccelerationStructure() const { return type == Type::RAYTRACING_ACCELERATION_STRUCTURE; }

		void* mapped_data = nullptr;
		uint32_t mapped_rowpitch = 0;
	};

	struct GPUBuffer : public GPUResource
	{
		GPUBufferDesc desc;

		const GPUBufferDesc& GetDesc() const { return desc; }
	};

	struct Texture : public GPUResource
	{
		TextureDesc	desc;

		const TextureDesc& GetDesc() const { return desc; }
	};

	struct GPUQueryHeap : public GraphicsDeviceChild
	{
		GPUQueryHeapDesc desc;

		const GPUQueryHeapDesc& GetDesc() const { return desc; }
	};

	struct PipelineState : public GraphicsDeviceChild
	{
		size_t hash = 0;
		PipelineStateDesc desc;

		const PipelineStateDesc& GetDesc() const { return desc; }
	};

	struct RenderPass : public GraphicsDeviceChild
	{
		size_t hash = 0;
		RenderPassDesc desc;

		const RenderPassDesc& GetDesc() const { return desc; }
	};

	struct SwapChain : public GraphicsDeviceChild
	{
		SwapChainDesc desc;

		const SwapChainDesc& GetDesc() const { return desc; }
	};


	struct RaytracingAccelerationStructureDesc
	{
		enum Flags
		{
			FLAG_EMPTY = 0,
			FLAG_ALLOW_UPDATE = 1 << 0,
			FLAG_ALLOW_COMPACTION = 1 << 1,
			FLAG_PREFER_FAST_TRACE = 1 << 2,
			FLAG_PREFER_FAST_BUILD = 1 << 3,
			FLAG_MINIMIZE_MEMORY = 1 << 4,
		};
		uint32_t flags = FLAG_EMPTY;

		enum class Type
		{
			BOTTOMLEVEL,
			TOPLEVEL,
		} type = Type::BOTTOMLEVEL;

		struct BottomLevel
		{
			struct Geometry
			{
				enum FLAGS
				{
					FLAG_EMPTY = 0,
					FLAG_OPAQUE = 1 << 0,
					FLAG_NO_DUPLICATE_ANYHIT_INVOCATION = 1 << 1,
					FLAG_USE_TRANSFORM = 1 << 2,
				};
				uint32_t flags = FLAG_EMPTY;

				enum class Type
				{
					TRIANGLES,
					PROCEDURAL_AABBS,
				} type = Type::TRIANGLES;

				struct Triangles
				{
					GPUBuffer vertex_buffer;
					GPUBuffer index_buffer;
					uint32_t index_count = 0;
					uint32_t index_offset = 0;
					uint32_t vertex_count = 0;
					uint32_t vertex_byte_offset = 0;
					uint32_t vertex_stride = 0;
					IndexBufferFormat index_format = IndexBufferFormat::UINT32;
					Format vertex_format = Format::R32G32B32_FLOAT;
					GPUBuffer transform_3x4_buffer;
					uint32_t transform_3x4_buffer_offset = 0;
				} triangles;
				struct Procedural_AABBs
				{
					GPUBuffer aabb_buffer;
					uint32_t offset = 0;
					uint32_t count = 0;
					uint32_t stride = 0;
				} aabbs;

			};
			wi::vector<Geometry> geometries;
		} bottom_level;

		struct TopLevel
		{
			struct Instance
			{
				enum FLAGS
				{
					FLAG_EMPTY = 0,
					FLAG_TRIANGLE_CULL_DISABLE = 1 << 0,
					FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE = 1 << 1,
					FLAG_FORCE_OPAQUE = 1 << 2,
					FLAG_FORCE_NON_OPAQUE = 1 << 3,
				};
				float transform[3][4];
				uint32_t instance_id : 24;
				uint32_t instance_mask : 8;
				uint32_t instance_contribution_to_hit_group_index : 24;
				uint32_t flags : 8;
				GPUResource bottom_level;
			};
			GPUBuffer instance_buffer;
			uint32_t offset = 0;
			uint32_t count = 0;
		} top_level;
	};
	struct RaytracingAccelerationStructure : public GPUResource
	{
		RaytracingAccelerationStructureDesc desc;

		const RaytracingAccelerationStructureDesc& GetDesc() const { return desc; }
	};

	struct ShaderLibrary
	{
		enum class Type
		{
			RAYGENERATION,
			MISS,
			CLOSESTHIT,
			ANYHIT,
			INTERSECTION,
		} type = Type::RAYGENERATION;
		const Shader* shader = nullptr;
		std::string function_name;
	};
	struct ShaderHitGroup
	{
		enum class Type
		{
			GENERAL, // raygen or miss
			TRIANGLES,
			PROCEDURAL,
		} type = Type::TRIANGLES;
		std::string name;
		uint32_t general_shader = ~0u;
		uint32_t closest_hit_shader = ~0u;
		uint32_t any_hit_shader = ~0u;
		uint32_t intersection_shader = ~0u;
	};
	struct RaytracingPipelineStateDesc
	{
		wi::vector<ShaderLibrary> shader_libraries;
		wi::vector<ShaderHitGroup> hit_groups;
		uint32_t max_trace_recursion_depth = 1;
		uint32_t max_attribute_size_in_bytes = 0;
		uint32_t max_payload_size_in_bytes = 0;
	};
	struct RaytracingPipelineState : public GraphicsDeviceChild
	{
		RaytracingPipelineStateDesc desc;

		const RaytracingPipelineStateDesc& GetDesc() const { return desc; }
	};

	struct ShaderTable
	{
		const GPUBuffer* buffer = nullptr;
		uint64_t offset = 0;
		uint64_t size = 0;
		uint64_t stride = 0;
	};
	struct DispatchRaysDesc
	{
		ShaderTable ray_generation;
		ShaderTable miss;
		ShaderTable hit_group;
		ShaderTable callable;
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
	};


	constexpr bool IsFormatUnorm(Format format)
	{
		switch (format)
		{
		case Format::R16G16B16A16_UNORM:
		case Format::R10G10B10A2_UNORM:
		case Format::R8G8B8A8_UNORM:
		case Format::R8G8B8A8_UNORM_SRGB:
		case Format::B8G8R8A8_UNORM:
		case Format::B8G8R8A8_UNORM_SRGB:
		case Format::R16G16_UNORM:
		case Format::D24_UNORM_S8_UINT:
		case Format::R8G8_UNORM:
		case Format::D16_UNORM:
		case Format::R16_UNORM:
		case Format::R8_UNORM:
			return true;
		}

		return false;
	}
	constexpr bool IsFormatBlockCompressed(Format format)
	{
		switch (format)
		{
		case Format::BC1_UNORM:
		case Format::BC1_UNORM_SRGB:
		case Format::BC2_UNORM:
		case Format::BC2_UNORM_SRGB:
		case Format::BC3_UNORM:
		case Format::BC3_UNORM_SRGB:
		case Format::BC4_UNORM:
		case Format::BC4_SNORM:
		case Format::BC5_UNORM:
		case Format::BC5_SNORM:
		case Format::BC6H_UF16:
		case Format::BC6H_SF16:
		case Format::BC7_UNORM:
		case Format::BC7_UNORM_SRGB:
			return true;
		}

		return false;
	}
	constexpr bool IsFormatStencilSupport(Format format)
	{
		switch (format)
		{
		case Format::R32G8X24_TYPELESS:
		case Format::D32_FLOAT_S8X24_UINT:
		case Format::R24G8_TYPELESS:
		case Format::D24_UNORM_S8_UINT:
			return true;
		}

		return false;
	}
	constexpr uint32_t GetFormatBlockSize(Format format)
	{
		if(IsFormatBlockCompressed(format))
		{
			return 4u;
		}
		return 1u;
	}
	constexpr uint32_t GetFormatStride(Format format)
	{
		switch (format)
		{
		case Format::BC1_UNORM:
		case Format::BC1_UNORM_SRGB:
		case Format::BC4_SNORM:
		case Format::BC4_UNORM:
			return 8u;

		case Format::R32G32B32A32_FLOAT:
		case Format::R32G32B32A32_UINT:
		case Format::R32G32B32A32_SINT:
		case Format::BC2_UNORM:
		case Format::BC2_UNORM_SRGB:
		case Format::BC3_UNORM:
		case Format::BC3_UNORM_SRGB:
		case Format::BC5_SNORM:
		case Format::BC5_UNORM:
		case Format::BC6H_UF16:
		case Format::BC6H_SF16:
		case Format::BC7_UNORM:
		case Format::BC7_UNORM_SRGB:
			return 16u;

		case Format::R32G32B32_FLOAT:
		case Format::R32G32B32_UINT:
		case Format::R32G32B32_SINT:
			return 12u;

		case Format::R16G16B16A16_FLOAT:
		case Format::R16G16B16A16_UNORM:
		case Format::R16G16B16A16_UINT:
		case Format::R16G16B16A16_SNORM:
		case Format::R16G16B16A16_SINT:
			return 8u;

		case Format::R32G32_FLOAT:
		case Format::R32G32_UINT:
		case Format::R32G32_SINT:
		case Format::R32G8X24_TYPELESS:
		case Format::D32_FLOAT_S8X24_UINT:
			return 8u;

		case Format::R10G10B10A2_UNORM:
		case Format::R10G10B10A2_UINT:
		case Format::R11G11B10_FLOAT:
		case Format::R8G8B8A8_UNORM:
		case Format::R8G8B8A8_UNORM_SRGB:
		case Format::R8G8B8A8_UINT:
		case Format::R8G8B8A8_SNORM:
		case Format::R8G8B8A8_SINT:
		case Format::B8G8R8A8_UNORM:
		case Format::B8G8R8A8_UNORM_SRGB:
		case Format::R16G16_FLOAT:
		case Format::R16G16_UNORM:
		case Format::R16G16_UINT:
		case Format::R16G16_SNORM:
		case Format::R16G16_SINT:
		case Format::R32_TYPELESS:
		case Format::D32_FLOAT:
		case Format::R32_FLOAT:
		case Format::R32_UINT:
		case Format::R32_SINT:
		case Format::R24G8_TYPELESS:
		case Format::D24_UNORM_S8_UINT:
			return 4u;

		case Format::R8G8_UNORM:
		case Format::R8G8_UINT:
		case Format::R8G8_SNORM:
		case Format::R8G8_SINT:
		case Format::R16_TYPELESS:
		case Format::R16_FLOAT:
		case Format::D16_UNORM:
		case Format::R16_UNORM:
		case Format::R16_UINT:
		case Format::R16_SNORM:
		case Format::R16_SINT:
			return 2u;

		case Format::R8_UNORM:
		case Format::R8_UINT:
		case Format::R8_SNORM:
		case Format::R8_SINT:
			return 1u;


		default:
			assert(0); // didn't catch format!
			break;
		}

		return 16u;
	}

}

template<>
struct enable_bitmask_operators<wi::graphics::ColorWrite> {
	static const bool enable = true;
};
template<>
struct enable_bitmask_operators<wi::graphics::BindFlag> {
	static const bool enable = true;
};
template<>
struct enable_bitmask_operators<wi::graphics::ResourceMiscFlag> {
	static const bool enable = true;
};
template<>
struct enable_bitmask_operators<wi::graphics::GraphicsDeviceCapability> {
	static const bool enable = true;
};
template<>
struct enable_bitmask_operators<wi::graphics::ResourceState> {
	static const bool enable = true;
};
template<>
struct enable_bitmask_operators<wi::graphics::RenderPassDesc::Flags> {
	static const bool enable = true;
};
