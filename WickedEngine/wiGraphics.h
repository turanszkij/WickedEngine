#pragma once
#include "CommonInclude.h"

#include <memory>
#include <vector>
#include <string>
#include <type_traits>

namespace wiGraphics
{
	struct Shader;
	struct GPUResource;
	struct GPUBuffer;
	struct Texture;

	enum SHADERSTAGE
	{
		MS,
		AS,
		VS,
		HS,
		DS,
		GS,
		PS,
		CS,
		LIB,
		SHADERSTAGE_COUNT,
	};
	enum SHADERFORMAT
	{
		SHADERFORMAT_NONE,
		SHADERFORMAT_HLSL5,
		SHADERFORMAT_HLSL6,
		SHADERFORMAT_SPIRV,
	};
	enum SHADERMODEL
	{
		SHADERMODEL_5_0,
		SHADERMODEL_6_0,
		SHADERMODEL_6_1,
		SHADERMODEL_6_2,
		SHADERMODEL_6_3,
		SHADERMODEL_6_4,
		SHADERMODEL_6_5,
		SHADERMODEL_6_6,
		SHADERMODEL_6_7,
	};
	enum PRIMITIVETOPOLOGY
	{
		UNDEFINED_TOPOLOGY,
		TRIANGLELIST,
		TRIANGLESTRIP,
		POINTLIST,
		LINELIST,
		LINESTRIP,
		PATCHLIST,
	};
	enum COMPARISON_FUNC
	{
		COMPARISON_NEVER,
		COMPARISON_LESS,
		COMPARISON_EQUAL,
		COMPARISON_LESS_EQUAL,
		COMPARISON_GREATER,
		COMPARISON_NOT_EQUAL,
		COMPARISON_GREATER_EQUAL,
		COMPARISON_ALWAYS,
	};
	enum DEPTH_WRITE_MASK
	{
		DEPTH_WRITE_MASK_ZERO,
		DEPTH_WRITE_MASK_ALL,
	};
	enum STENCIL_OP
	{
		STENCIL_OP_KEEP,
		STENCIL_OP_ZERO,
		STENCIL_OP_REPLACE,
		STENCIL_OP_INCR_SAT,
		STENCIL_OP_DECR_SAT,
		STENCIL_OP_INVERT,
		STENCIL_OP_INCR,
		STENCIL_OP_DECR,
	};
	enum BLEND
	{
		BLEND_ZERO,
		BLEND_ONE,
		BLEND_SRC_COLOR,
		BLEND_INV_SRC_COLOR,
		BLEND_SRC_ALPHA,
		BLEND_INV_SRC_ALPHA,
		BLEND_DEST_ALPHA,
		BLEND_INV_DEST_ALPHA,
		BLEND_DEST_COLOR,
		BLEND_INV_DEST_COLOR,
		BLEND_SRC_ALPHA_SAT,
		BLEND_BLEND_FACTOR,
		BLEND_INV_BLEND_FACTOR,
		BLEND_SRC1_COLOR,
		BLEND_INV_SRC1_COLOR,
		BLEND_SRC1_ALPHA,
		BLEND_INV_SRC1_ALPHA,
	}; 
	enum BLEND_OP
	{
		BLEND_OP_ADD,
		BLEND_OP_SUBTRACT,
		BLEND_OP_REV_SUBTRACT,
		BLEND_OP_MIN,
		BLEND_OP_MAX,
	};
	enum FILL_MODE
	{
		FILL_WIREFRAME,
		FILL_SOLID,
	};
	enum CULL_MODE
	{
		CULL_NONE,
		CULL_FRONT,
		CULL_BACK,
	};
	enum INPUT_CLASSIFICATION
	{
		INPUT_PER_VERTEX_DATA,
		INPUT_PER_INSTANCE_DATA,
	};
	enum USAGE
	{
		USAGE_DEFAULT,	// CPU no access, GPU read/write
		USAGE_UPLOAD,	// CPU write, GPU read
		USAGE_READBACK,	// CPU read, GPU write
	};
	enum TEXTURE_ADDRESS_MODE
	{
		TEXTURE_ADDRESS_WRAP,
		TEXTURE_ADDRESS_MIRROR,
		TEXTURE_ADDRESS_CLAMP,
		TEXTURE_ADDRESS_BORDER,
		TEXTURE_ADDRESS_MIRROR_ONCE,
	};
	enum FILTER
	{
		FILTER_MIN_MAG_MIP_POINT,
		FILTER_MIN_MAG_POINT_MIP_LINEAR,
		FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
		FILTER_MIN_POINT_MAG_MIP_LINEAR,
		FILTER_MIN_LINEAR_MAG_MIP_POINT,
		FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		FILTER_MIN_MAG_LINEAR_MIP_POINT,
		FILTER_MIN_MAG_MIP_LINEAR,
		FILTER_ANISOTROPIC,
		FILTER_COMPARISON_MIN_MAG_MIP_POINT,
		FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
		FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
		FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
		FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
		FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
		FILTER_COMPARISON_ANISOTROPIC,
		FILTER_MINIMUM_MIN_MAG_MIP_POINT,
		FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
		FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
		FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
		FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
		FILTER_MINIMUM_MIN_MAG_MIP_LINEAR,
		FILTER_MINIMUM_ANISOTROPIC,
		FILTER_MAXIMUM_MIN_MAG_MIP_POINT,
		FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
		FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
		FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
		FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
		FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR,
		FILTER_MAXIMUM_ANISOTROPIC,
	};
	enum SAMPLER_BORDER_COLOR
	{
		SAMPLER_BORDER_COLOR_TRANSPARENT_BLACK,
		SAMPLER_BORDER_COLOR_OPAQUE_BLACK,
		SAMPLER_BORDER_COLOR_OPAQUE_WHITE,
	};
	enum FORMAT
	{
		FORMAT_UNKNOWN,

		FORMAT_R32G32B32A32_FLOAT,
		FORMAT_R32G32B32A32_UINT,
		FORMAT_R32G32B32A32_SINT,

		FORMAT_R32G32B32_FLOAT,
		FORMAT_R32G32B32_UINT,
		FORMAT_R32G32B32_SINT,

		FORMAT_R16G16B16A16_FLOAT,
		FORMAT_R16G16B16A16_UNORM,
		FORMAT_R16G16B16A16_UINT,
		FORMAT_R16G16B16A16_SNORM,
		FORMAT_R16G16B16A16_SINT,

		FORMAT_R32G32_FLOAT,
		FORMAT_R32G32_UINT,
		FORMAT_R32G32_SINT,
		FORMAT_R32G8X24_TYPELESS,		// depth (32-bit) + stencil (8-bit) + shader resource (32-bit)
		FORMAT_D32_FLOAT_S8X24_UINT,	// depth (32-bit) + stencil (8-bit)

		FORMAT_R10G10B10A2_UNORM,
		FORMAT_R10G10B10A2_UINT,
		FORMAT_R11G11B10_FLOAT,
		FORMAT_R8G8B8A8_UNORM,
		FORMAT_R8G8B8A8_UNORM_SRGB,
		FORMAT_R8G8B8A8_UINT,
		FORMAT_R8G8B8A8_SNORM,
		FORMAT_R8G8B8A8_SINT, 
		FORMAT_B8G8R8A8_UNORM,
		FORMAT_B8G8R8A8_UNORM_SRGB,
		FORMAT_R16G16_FLOAT,
		FORMAT_R16G16_UNORM,
		FORMAT_R16G16_UINT,
		FORMAT_R16G16_SNORM,
		FORMAT_R16G16_SINT,
		FORMAT_R32_TYPELESS,			// depth (32-bit) + shader resource (32-bit)
		FORMAT_D32_FLOAT,				// depth (32-bit)
		FORMAT_R32_FLOAT,
		FORMAT_R32_UINT,
		FORMAT_R32_SINT, 
		FORMAT_R24G8_TYPELESS,			// depth (24-bit) + stencil (8-bit) + shader resource (24-bit)
		FORMAT_D24_UNORM_S8_UINT,		// depth (24-bit) + stencil (8-bit)

		FORMAT_R8G8_UNORM,
		FORMAT_R8G8_UINT,
		FORMAT_R8G8_SNORM,
		FORMAT_R8G8_SINT,
		FORMAT_R16_TYPELESS,			// depth (16-bit) + shader resource (16-bit)
		FORMAT_R16_FLOAT,
		FORMAT_D16_UNORM,				// depth (16-bit)
		FORMAT_R16_UNORM,
		FORMAT_R16_UINT,
		FORMAT_R16_SNORM,
		FORMAT_R16_SINT,

		FORMAT_R8_UNORM,
		FORMAT_R8_UINT,
		FORMAT_R8_SNORM,
		FORMAT_R8_SINT,

		FORMAT_BC1_UNORM,
		FORMAT_BC1_UNORM_SRGB,
		FORMAT_BC2_UNORM,
		FORMAT_BC2_UNORM_SRGB,
		FORMAT_BC3_UNORM,
		FORMAT_BC3_UNORM_SRGB,
		FORMAT_BC4_UNORM,
		FORMAT_BC4_SNORM,
		FORMAT_BC5_UNORM,
		FORMAT_BC5_SNORM,
		FORMAT_BC6H_UF16,
		FORMAT_BC6H_SF16,
		FORMAT_BC7_UNORM,
		FORMAT_BC7_UNORM_SRGB
	};
	enum GPU_QUERY_TYPE
	{
		GPU_QUERY_TYPE_TIMESTAMP,			// retrieve time point of gpu execution
		GPU_QUERY_TYPE_OCCLUSION,			// how many samples passed depth test?
		GPU_QUERY_TYPE_OCCLUSION_BINARY,	// depth test passed or not?
	};
	enum INDEXBUFFER_FORMAT
	{
		INDEXFORMAT_16BIT,
		INDEXFORMAT_32BIT,
	};
	enum SUBRESOURCE_TYPE
	{
		SRV, // shader resource view
		UAV, // unordered access view
		RTV, // render target view
		DSV, // depth stencil view
	};

	enum SHADING_RATE
	{
		SHADING_RATE_1X1,
		SHADING_RATE_1X2,
		SHADING_RATE_2X1,
		SHADING_RATE_2X2,
		SHADING_RATE_2X4,
		SHADING_RATE_4X2,
		SHADING_RATE_4X4,

		SHADING_RATE_INVALID
	};

	enum PREDICATION_OP
	{
		PREDICATION_OP_EQUAL_ZERO,
		PREDICATION_OP_NOT_EQUAL_ZERO,
	};

	// Flags ////////////////////////////////////////////

	// Enable enum flags:
	//	https://www.justsoftwaresolutions.co.uk/cplusplus/using-enum-classes-as-bitfields.html
	template<typename E>
	struct enable_bitmask_operators {
		static constexpr bool enable = false;
	};
	template<typename E>
	typename std::enable_if<enable_bitmask_operators<E>::enable, E>::type operator|(E lhs, E rhs)
	{
		typedef typename std::underlying_type<E>::type underlying;
		return static_cast<E>(
			static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
	}
	template<typename E>
	typename std::enable_if<enable_bitmask_operators<E>::enable, E&>::type operator|=(E& lhs, E rhs)
	{
		typedef typename std::underlying_type<E>::type underlying;
		lhs = static_cast<E>(
			static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
		return lhs;
	}

	enum COLOR_WRITE_ENABLE
	{
		COLOR_WRITE_DISABLE = 0,
		COLOR_WRITE_ENABLE_RED = 1 << 0,
		COLOR_WRITE_ENABLE_GREEN = 1 << 1,
		COLOR_WRITE_ENABLE_BLUE = 1 << 2,
		COLOR_WRITE_ENABLE_ALPHA = 1 << 3,
		COLOR_WRITE_ENABLE_ALL = ~0,
	};
	template<>
	struct enable_bitmask_operators<COLOR_WRITE_ENABLE> {
		static const bool enable = true;
	};

	enum BIND_FLAG
	{
		BIND_NONE = 0,
		BIND_VERTEX_BUFFER = 1 << 0,
		BIND_INDEX_BUFFER = 1 << 1,
		BIND_CONSTANT_BUFFER = 1 << 2,
		BIND_SHADER_RESOURCE = 1 << 3,
		BIND_RENDER_TARGET = 1 << 4,
		BIND_DEPTH_STENCIL = 1 << 5,
		BIND_UNORDERED_ACCESS = 1 << 6,
		BIND_SHADING_RATE = 1 << 7,
	};
	template<>
	struct enable_bitmask_operators<BIND_FLAG> {
		static const bool enable = true;
	};

	enum RESOURCE_MISC_FLAG
	{
		RESOURCE_MISC_NONE = 0,
		RESOURCE_MISC_TEXTURECUBE = 1 << 0,
		RESOURCE_MISC_INDIRECT_ARGS = 1 << 1,
		RESOURCE_MISC_BUFFER_RAW = 1 << 2,
		RESOURCE_MISC_BUFFER_STRUCTURED = 1 << 3,
		RESOURCE_MISC_RAY_TRACING = 1 << 4,
		RESOURCE_MISC_PREDICATION = 1 << 5,
	};
	template<>
	struct enable_bitmask_operators<RESOURCE_MISC_FLAG> {
		static const bool enable = true;
	};

	enum GRAPHICSDEVICE_CAPABILITY
	{
		GRAPHICSDEVICE_CAPABILITY_TESSELLATION = 1 << 0,
		GRAPHICSDEVICE_CAPABILITY_CONSERVATIVE_RASTERIZATION = 1 << 1,
		GRAPHICSDEVICE_CAPABILITY_RASTERIZER_ORDERED_VIEWS = 1 << 2,
		GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_COMMON = 1 << 3, // eg: R16G16B16A16_FLOAT, R8G8B8A8_UNORM and more common ones
		GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT = 1 << 4,
		GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS = 1 << 5,
		GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING = 1 << 6,
		GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2 = 1 << 7,
		GRAPHICSDEVICE_CAPABILITY_MESH_SHADER = 1 << 8,
		GRAPHICSDEVICE_CAPABILITY_RAYTRACING = 1 << 9,
		GRAPHICSDEVICE_CAPABILITY_PREDICATION = 1 << 10,
		GRAPHICSDEVICE_CAPABILITY_SAMPLER_MINMAX = 1 << 11,
	};
	enum RESOURCE_STATE
	{
		// Common resource states:
		RESOURCE_STATE_UNDEFINED = 0,						// invalid state
		RESOURCE_STATE_SHADER_RESOURCE = 1 << 0,			// shader resource, read only
		RESOURCE_STATE_SHADER_RESOURCE_COMPUTE = 1 << 1,	// shader resource, read only, non-pixel shader
		RESOURCE_STATE_UNORDERED_ACCESS = 1 << 2,			// shader resource, write enabled
		RESOURCE_STATE_COPY_SRC = 1 << 3,					// copy from
		RESOURCE_STATE_COPY_DST = 1 << 4,					// copy to

		// Texture specific resource states:
		RESOURCE_STATE_RENDERTARGET = 1 << 5,				// render target, write enabled
		RESOURCE_STATE_DEPTHSTENCIL = 1 << 6,				// depth stencil, write enabled
		RESOURCE_STATE_DEPTHSTENCIL_READONLY = 1 << 7,		// depth stencil, read only
		RESOURCE_STATE_SHADING_RATE_SOURCE = 1 << 8,		// shading rate control per tile

		// GPUBuffer specific resource states:
		RESOURCE_STATE_VERTEX_BUFFER = 1 << 9,				// vertex buffer, read only
		RESOURCE_STATE_INDEX_BUFFER = 1 << 10,				// index buffer, read only
		RESOURCE_STATE_CONSTANT_BUFFER = 1 << 11,			// constant buffer, read only
		RESOURCE_STATE_INDIRECT_ARGUMENT = 1 << 12,			// argument buffer to DrawIndirect() or DispatchIndirect()
		RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE = 1 << 13, // acceleration structure storage or scratch
		RESOURCE_STATE_PREDICATION = 1 << 14				// storage for predication comparison value
	};
	template<>
	struct enable_bitmask_operators<RESOURCE_STATE> {
		static const bool enable = true;
	};

	enum COLOR_SPACE
	{
		COLOR_SPACE_SRGB,			// SDR color space (8 or 10 bits per channel)
		COLOR_SPACE_HDR10_ST2084,	// HDR10 color space (10 bits per channel)
		COLOR_SPACE_HDR_LINEAR,		// HDR color space (16 bits per channel)
	};

	// Descriptor structs:

	struct Viewport
	{
		float top_left_x = 0.0f;
		float top_left_y = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
		float min_depth = 0.0f;
		float max_depth = 1.0f;
	};
	struct InputLayout
	{
		static const uint32_t APPEND_ALIGNED_ELEMENT = 0xffffffff; // automatically figure out AlignedByteOffset depending on Format

		struct Element
		{
			std::string semantic_name;
			uint32_t semantic_index = 0;
			FORMAT format = FORMAT_UNKNOWN;
			uint32_t input_slot = 0;
			uint32_t aligned_byte_offset = APPEND_ALIGNED_ELEMENT;
			INPUT_CLASSIFICATION input_slot_class = INPUT_CLASSIFICATION::INPUT_PER_VERTEX_DATA;
		};
		std::vector<Element> elements;
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
		enum TEXTURE_TYPE
		{
			TEXTURE_1D,
			TEXTURE_2D,
			TEXTURE_3D,
		} type = TEXTURE_2D;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
		uint32_t array_size = 1;
		uint32_t mip_levels = 1;
		FORMAT format = FORMAT_UNKNOWN;
		uint32_t sample_count = 1;
		USAGE usage = USAGE_DEFAULT;
		BIND_FLAG bind_flags = BIND_NONE;
		RESOURCE_MISC_FLAG misc_flags = RESOURCE_MISC_NONE;
		ClearValue clear = {};
		RESOURCE_STATE layout = RESOURCE_STATE_SHADER_RESOURCE;
	};
	struct SamplerDesc
	{
		FILTER filter = FILTER_MIN_MAG_MIP_POINT;
		TEXTURE_ADDRESS_MODE address_u = TEXTURE_ADDRESS_CLAMP;
		TEXTURE_ADDRESS_MODE address_v = TEXTURE_ADDRESS_CLAMP;
		TEXTURE_ADDRESS_MODE address_w = TEXTURE_ADDRESS_CLAMP;
		float mip_lod_bias = 0.0f;
		uint32_t max_anisotropy = 0;
		COMPARISON_FUNC comparison_func = COMPARISON_NEVER;
		SAMPLER_BORDER_COLOR border_color = SAMPLER_BORDER_COLOR_TRANSPARENT_BLACK;
		float min_lod = 0.0f;
		float max_lod = FLT_MAX;
	};
	struct RasterizerState
	{
		FILL_MODE fill_mode = FILL_SOLID;
		CULL_MODE cull_mode = CULL_NONE;
		bool front_counter_clockwise = false;
		int32_t depth_bias = 0;
		float depth_bias_clamp = 0.0f;
		float slope_scaled_depth_bias = 0.0f;
		bool depth_clip_enable = false;
		bool multisample_enable = false;
		bool antialiased_line_enable = false;
		bool conservative_rasterization_enable = false;
		uint32_t forced_sample_count = 0;
	};
	struct DepthStencilState
	{
		bool depth_enable = false;
		DEPTH_WRITE_MASK depth_write_mask = DEPTH_WRITE_MASK_ZERO;
		COMPARISON_FUNC depth_func = COMPARISON_NEVER;
		bool stencil_enable = false;
		uint8_t stencil_read_mask = 0xff;
		uint8_t stencil_write_mask = 0xff;

		struct DepthStencilOp
		{
			STENCIL_OP stencil_fail_op = STENCIL_OP_KEEP;
			STENCIL_OP stencil_depth_fail_op = STENCIL_OP_KEEP;
			STENCIL_OP stencil_pass_op = STENCIL_OP_KEEP;
			COMPARISON_FUNC stencil_func = COMPARISON_NEVER;
		};
		DepthStencilOp front_face;
		DepthStencilOp back_face;
	};
	struct BlendState
	{
		bool alpha_to_coverage_enable = false;
		bool independent_blend_enable = false;

		struct RenderTargetBlendState
		{
			bool blend_enable = false;
			BLEND src_blend = BLEND_SRC_ALPHA;
			BLEND dest_blend = BLEND_INV_SRC_ALPHA;
			BLEND_OP blend_op = BLEND_OP_ADD;
			BLEND src_blend_alpha = BLEND_ONE;
			BLEND dest_blend_alpha = BLEND_ONE;
			BLEND_OP blend_op_alpha = BLEND_OP_ADD;
			COLOR_WRITE_ENABLE render_target_write_mask = COLOR_WRITE_ENABLE_ALL;
		};
		RenderTargetBlendState render_target[8];
	};
	struct GPUBufferDesc
	{
		uint64_t size = 0;
		USAGE usage = USAGE_DEFAULT;
		BIND_FLAG bind_flags = BIND_NONE;
		RESOURCE_MISC_FLAG misc_flags = RESOURCE_MISC_NONE;
		uint32_t stride = 0; // needed for typed and structured buffer types!
		FORMAT format = FORMAT_UNKNOWN; // only needed for typed buffer!
	};
	struct GPUQueryHeapDesc
	{
		GPU_QUERY_TYPE type = GPU_QUERY_TYPE_TIMESTAMP;
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
		PRIMITIVETOPOLOGY		pt = TRIANGLELIST;
		uint32_t                patch_control_points = 3;
		uint32_t				sample_mask = 0xFFFFFFFF;
	};
	struct GPUBarrier
	{
		enum TYPE
		{
			MEMORY_BARRIER,		// UAV accesses
			IMAGE_BARRIER,		// image layout transition
			BUFFER_BARRIER,		// buffer state transition
		} type = MEMORY_BARRIER;

		struct Memory
		{
			const GPUResource* resource;
		};
		struct Image
		{
			const Texture* texture;
			RESOURCE_STATE layout_before;
			RESOURCE_STATE layout_after;
			int mip;
			int slice;
		};
		struct Buffer
		{
			const GPUBuffer* buffer;
			RESOURCE_STATE state_before;
			RESOURCE_STATE state_after;
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
			barrier.type = MEMORY_BARRIER;
			barrier.memory.resource = resource;
			return barrier;
		}
		static GPUBarrier Image(const Texture* texture, RESOURCE_STATE before, RESOURCE_STATE after,
			int mip = -1, int slice = -1)
		{
			GPUBarrier barrier;
			barrier.type = IMAGE_BARRIER;
			barrier.image.texture = texture;
			barrier.image.layout_before = before;
			barrier.image.layout_after = after;
			barrier.image.mip = mip;
			barrier.image.slice = slice;
			return barrier;
		}
		static GPUBarrier Buffer(const GPUBuffer* buffer, RESOURCE_STATE before, RESOURCE_STATE after)
		{
			GPUBarrier barrier;
			barrier.type = BUFFER_BARRIER;
			barrier.buffer.buffer = buffer;
			barrier.buffer.state_before = before;
			barrier.buffer.state_after = after;
			return barrier;
		}
	};
	struct RenderPassAttachment
	{
		enum TYPE
		{
			RENDERTARGET,
			DEPTH_STENCIL,
			RESOLVE,
			SHADING_RATE_SOURCE
		} type = RENDERTARGET;
		enum LOAD_OPERATION
		{
			LOADOP_LOAD,
			LOADOP_CLEAR,
			LOADOP_DONTCARE,
		} loadop = LOADOP_LOAD;
		const Texture* texture = nullptr;
		int subresource = -1;
		enum STORE_OPERATION
		{
			STOREOP_STORE,
			STOREOP_DONTCARE,
		} storeop = STOREOP_STORE;
		RESOURCE_STATE initial_layout = RESOURCE_STATE_UNDEFINED;	// layout before the render pass
		RESOURCE_STATE subpass_layout = RESOURCE_STATE_UNDEFINED;	// layout within the render pass
		RESOURCE_STATE final_layout = RESOURCE_STATE_UNDEFINED;		// layout after the render pass

		static RenderPassAttachment RenderTarget(
			const Texture* resource = nullptr,
			LOAD_OPERATION load_op = LOADOP_LOAD,
			STORE_OPERATION store_op = STOREOP_STORE,
			RESOURCE_STATE initial_layout = RESOURCE_STATE_SHADER_RESOURCE,
			RESOURCE_STATE subpass_layout = RESOURCE_STATE_RENDERTARGET,
			RESOURCE_STATE final_layout = RESOURCE_STATE_SHADER_RESOURCE
		)
		{
			RenderPassAttachment attachment;
			attachment.type = RENDERTARGET;
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
			LOAD_OPERATION load_op = LOADOP_LOAD,
			STORE_OPERATION store_op = STOREOP_STORE,
			RESOURCE_STATE initial_layout = RESOURCE_STATE_DEPTHSTENCIL,
			RESOURCE_STATE subpass_layout = RESOURCE_STATE_DEPTHSTENCIL,
			RESOURCE_STATE final_layout = RESOURCE_STATE_DEPTHSTENCIL
		)
		{
			RenderPassAttachment attachment;
			attachment.type = DEPTH_STENCIL;
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
			RESOURCE_STATE initial_layout = RESOURCE_STATE_SHADER_RESOURCE,
			RESOURCE_STATE final_layout = RESOURCE_STATE_SHADER_RESOURCE
		)
		{
			RenderPassAttachment attachment;
			attachment.type = RESOLVE;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
			attachment.final_layout = final_layout;
			return attachment;
		}

		static RenderPassAttachment ShadingRateSource(
			const Texture* resource = nullptr,
			RESOURCE_STATE initial_layout = RESOURCE_STATE_SHADING_RATE_SOURCE,
			RESOURCE_STATE final_layout = RESOURCE_STATE_SHADING_RATE_SOURCE
		)
		{
			RenderPassAttachment attachment;
			attachment.type = SHADING_RATE_SOURCE;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
			attachment.subpass_layout = RESOURCE_STATE_SHADING_RATE_SOURCE;
			attachment.final_layout = final_layout;
			return attachment;
		}
	};
	struct RenderPassDesc
	{
		enum FLAGS
		{
			FLAG_EMPTY = 0,
			FLAG_ALLOW_UAV_WRITES = 1 << 0,
		};
		uint32_t flags = FLAG_EMPTY;
		std::vector<RenderPassAttachment> attachments;
	};
	struct SwapChainDesc
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t buffer_count = 2;
		FORMAT format = FORMAT_R10G10B10A2_UNORM;
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
	struct StaticSampler
	{
		Sampler sampler;
		uint32_t slot = 0;
	};

	struct Shader : public GraphicsDeviceChild
	{
		SHADERSTAGE stage = SHADERSTAGE_COUNT;
		std::vector<StaticSampler> auto_samplers; // ability to set static samplers without explicit root signature
	};

	struct GPUResource : public GraphicsDeviceChild
	{
		enum class GPU_RESOURCE_TYPE
		{
			BUFFER,
			TEXTURE,
			RAYTRACING_ACCELERATION_STRUCTURE,
			UNKNOWN_TYPE,
		} type = GPU_RESOURCE_TYPE::UNKNOWN_TYPE;
		inline bool IsTexture() const { return type == GPU_RESOURCE_TYPE::TEXTURE; }
		inline bool IsBuffer() const { return type == GPU_RESOURCE_TYPE::BUFFER; }
		inline bool IsAccelerationStructure() const { return type == GPU_RESOURCE_TYPE::RAYTRACING_ACCELERATION_STRUCTURE; }

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
		enum FLAGS
		{
			FLAG_EMPTY = 0,
			FLAG_ALLOW_UPDATE = 1 << 0,
			FLAG_ALLOW_COMPACTION = 1 << 1,
			FLAG_PREFER_FAST_TRACE = 1 << 2,
			FLAG_PREFER_FAST_BUILD = 1 << 3,
			FLAG_MINIMIZE_MEMORY = 1 << 4,
		};
		uint32_t flags = FLAG_EMPTY;

		enum TYPE
		{
			BOTTOMLEVEL,
			TOPLEVEL,
		} type = BOTTOMLEVEL;

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

				enum TYPE
				{
					TRIANGLES,
					PROCEDURAL_AABBS,
				} type = TRIANGLES;

				struct Triangles
				{
					GPUBuffer vertex_buffer;
					GPUBuffer index_buffer;
					uint32_t index_count = 0;
					uint32_t index_offset = 0;
					uint32_t vertex_count = 0;
					uint32_t vertex_byte_offset = 0;
					uint32_t vertex_stride = 0;
					INDEXBUFFER_FORMAT index_format = INDEXFORMAT_32BIT;
					FORMAT vertex_format = FORMAT_R32G32B32_FLOAT;
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
			std::vector<Geometry> geometries;
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
				XMFLOAT3X4 transform;
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
		enum TYPE
		{
			RAYGENERATION,
			MISS,
			CLOSESTHIT,
			ANYHIT,
			INTERSECTION,
		} type = RAYGENERATION;
		const Shader* shader = nullptr;
		std::string function_name;
	};
	struct ShaderHitGroup
	{
		enum TYPE
		{
			GENERAL, // raygen or miss
			TRIANGLES,
			PROCEDURAL,
		} type = TRIANGLES;
		std::string name;
		uint32_t general_shader = ~0;
		uint32_t closest_hit_shader = ~0;
		uint32_t any_hit_shader = ~0;
		uint32_t intersection_shader = ~0;
	};
	struct RaytracingPipelineStateDesc
	{
		std::vector<ShaderLibrary> shader_libraries;
		std::vector<ShaderHitGroup> hit_groups;
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


	constexpr bool IsFormatUnorm(FORMAT format)
	{
		switch (format)
		{
		case FORMAT_R16G16B16A16_UNORM:
		case FORMAT_R10G10B10A2_UNORM:
		case FORMAT_R8G8B8A8_UNORM:
		case FORMAT_R8G8B8A8_UNORM_SRGB:
		case FORMAT_B8G8R8A8_UNORM:
		case FORMAT_B8G8R8A8_UNORM_SRGB:
		case FORMAT_R16G16_UNORM:
		case FORMAT_D24_UNORM_S8_UINT:
		case FORMAT_R8G8_UNORM:
		case FORMAT_D16_UNORM:
		case FORMAT_R16_UNORM:
		case FORMAT_R8_UNORM:
			return true;
		}

		return false;
	}
	constexpr bool IsFormatBlockCompressed(FORMAT format)
	{
		switch (format)
		{
		case FORMAT_BC1_UNORM:
		case FORMAT_BC1_UNORM_SRGB:
		case FORMAT_BC2_UNORM:
		case FORMAT_BC2_UNORM_SRGB:
		case FORMAT_BC3_UNORM:
		case FORMAT_BC3_UNORM_SRGB:
		case FORMAT_BC4_UNORM:
		case FORMAT_BC4_SNORM:
		case FORMAT_BC5_UNORM:
		case FORMAT_BC5_SNORM:
		case FORMAT_BC6H_UF16:
		case FORMAT_BC6H_SF16:
		case FORMAT_BC7_UNORM:
		case FORMAT_BC7_UNORM_SRGB:
			return true;
		}

		return false;
	}
	constexpr bool IsFormatStencilSupport(FORMAT format)
	{
		switch (format)
		{
		case FORMAT_R32G8X24_TYPELESS:
		case FORMAT_D32_FLOAT_S8X24_UINT:
		case FORMAT_R24G8_TYPELESS:
		case FORMAT_D24_UNORM_S8_UINT:
			return true;
		}

		return false;
	}
	constexpr uint32_t GetFormatBlockSize(FORMAT format)
	{
		if(IsFormatBlockCompressed(format))
		{
			return 4u;
		}
		return 1u;
	}
	constexpr uint32_t GetFormatStride(FORMAT format)
	{
		switch (format)
		{
		case FORMAT_BC1_UNORM:
		case FORMAT_BC1_UNORM_SRGB:
		case FORMAT_BC4_SNORM:
		case FORMAT_BC4_UNORM:
			return 8u;

		case FORMAT_R32G32B32A32_FLOAT:
		case FORMAT_R32G32B32A32_UINT:
		case FORMAT_R32G32B32A32_SINT:
		case FORMAT_BC2_UNORM:
		case FORMAT_BC2_UNORM_SRGB:
		case FORMAT_BC3_UNORM:
		case FORMAT_BC3_UNORM_SRGB:
		case FORMAT_BC5_SNORM:
		case FORMAT_BC5_UNORM:
		case FORMAT_BC6H_UF16:
		case FORMAT_BC6H_SF16:
		case FORMAT_BC7_UNORM:
		case FORMAT_BC7_UNORM_SRGB:
			return 16u;

		case FORMAT_R32G32B32_FLOAT:
		case FORMAT_R32G32B32_UINT:
		case FORMAT_R32G32B32_SINT:
			return 12u;

		case FORMAT_R16G16B16A16_FLOAT:
		case FORMAT_R16G16B16A16_UNORM:
		case FORMAT_R16G16B16A16_UINT:
		case FORMAT_R16G16B16A16_SNORM:
		case FORMAT_R16G16B16A16_SINT:
			return 8u;

		case FORMAT_R32G32_FLOAT:
		case FORMAT_R32G32_UINT:
		case FORMAT_R32G32_SINT:
		case FORMAT_R32G8X24_TYPELESS:
		case FORMAT_D32_FLOAT_S8X24_UINT:
			return 8u;

		case FORMAT_R10G10B10A2_UNORM:
		case FORMAT_R10G10B10A2_UINT:
		case FORMAT_R11G11B10_FLOAT:
		case FORMAT_R8G8B8A8_UNORM:
		case FORMAT_R8G8B8A8_UNORM_SRGB:
		case FORMAT_R8G8B8A8_UINT:
		case FORMAT_R8G8B8A8_SNORM:
		case FORMAT_R8G8B8A8_SINT:
		case FORMAT_B8G8R8A8_UNORM:
		case FORMAT_B8G8R8A8_UNORM_SRGB:
		case FORMAT_R16G16_FLOAT:
		case FORMAT_R16G16_UNORM:
		case FORMAT_R16G16_UINT:
		case FORMAT_R16G16_SNORM:
		case FORMAT_R16G16_SINT:
		case FORMAT_R32_TYPELESS:
		case FORMAT_D32_FLOAT:
		case FORMAT_R32_FLOAT:
		case FORMAT_R32_UINT:
		case FORMAT_R32_SINT:
		case FORMAT_R24G8_TYPELESS:
		case FORMAT_D24_UNORM_S8_UINT:
			return 4u;

		case FORMAT_R8G8_UNORM:
		case FORMAT_R8G8_UINT:
		case FORMAT_R8G8_SNORM:
		case FORMAT_R8G8_SINT:
		case FORMAT_R16_TYPELESS:
		case FORMAT_R16_FLOAT:
		case FORMAT_D16_UNORM:
		case FORMAT_R16_UNORM:
		case FORMAT_R16_UINT:
		case FORMAT_R16_SNORM:
		case FORMAT_R16_SINT:
			return 2u;

		case FORMAT_R8_UNORM:
		case FORMAT_R8_UINT:
		case FORMAT_R8_SNORM:
		case FORMAT_R8_SINT:
			return 1u;


		default:
			assert(0); // didn't catch format!
			break;
		}

		return 16u;
	}

}
