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

	enum class ValidationMode
	{
		Disabled,	// No validation is enabled
		Enabled,	// CPU command validation
		GPU,		// CPU and GPU-based validation
		Verbose		// Print all warnings, errors and info messages
	};

	enum class AdapterType
	{
		Other,
		IntegratedGpu,
		DiscreteGpu,
		VirtualGpu,
		Cpu,
	};

	enum class GPUPreference
	{
		Discrete,
		Integrated,
	};

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
		NONE,		// Not used
		HLSL5,		// DXBC
		HLSL6,		// DXIL
		SPIRV,		// SPIR-V
		HLSL6_XS,	// XBOX Series Native
		PS5,		// Playstation 5
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
		D32_FLOAT_S8X24_UINT,	// depth (32-bit) + stencil (8-bit) | SRV: R32_FLOAT (default or depth aspect), R8_UINT (stencil aspect)

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
		D32_FLOAT,				// depth (32-bit) | SRV: R32_FLOAT
		R32_FLOAT,
		R32_UINT,
		R32_SINT, 
		D24_UNORM_S8_UINT,		// depth (24-bit) + stencil (8-bit) | SRV: R24_INTERNAL (default or depth aspect), R8_UINT (stencil aspect)
		R9G9B9E5_SHAREDEXP,

		R8G8_UNORM,
		R8G8_UINT,
		R8G8_SNORM,
		R8G8_SINT,
		R16_FLOAT,
		D16_UNORM,				// depth (16-bit) | SRV: R16_UNORM
		R16_UNORM,
		R16_UINT,
		R16_SNORM,
		R16_SINT,

		R8_UNORM,
		R8_UINT,
		R8_SNORM,
		R8_SINT,

		// Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:

		BC1_UNORM,			// Three color channels (5 bits:6 bits:5 bits), with 0 or 1 bit(s) of alpha
		BC1_UNORM_SRGB,		// Three color channels (5 bits:6 bits:5 bits), with 0 or 1 bit(s) of alpha
		BC2_UNORM,			// Three color channels (5 bits:6 bits:5 bits), with 4 bits of alpha
		BC2_UNORM_SRGB,		// Three color channels (5 bits:6 bits:5 bits), with 4 bits of alpha
		BC3_UNORM,			// Three color channels (5 bits:6 bits:5 bits) with 8 bits of alpha
		BC3_UNORM_SRGB,		// Three color channels (5 bits:6 bits:5 bits) with 8 bits of alpha
		BC4_UNORM,			// One color channel (8 bits)
		BC4_SNORM,			// One color channel (8 bits)
		BC5_UNORM,			// Two color channels (8 bits:8 bits)
		BC5_SNORM,			// Two color channels (8 bits:8 bits)
		BC6H_UF16,			// Three color channels (16 bits:16 bits:16 bits) in "half" floating point
		BC6H_SF16,			// Three color channels (16 bits:16 bits:16 bits) in "half" floating point
		BC7_UNORM,			// Three color channels (4 to 7 bits per channel) with 0 to 8 bits of alpha
		BC7_UNORM_SRGB,		// Three color channels (4 to 7 bits per channel) with 0 to 8 bits of alpha

		NV12,				// video YUV420; SRV Luminance aspect: R8_UNORM, SRV Chrominance aspect: R8G8_UNORM
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

	enum class ImageAspect
	{
		COLOR,
		DEPTH,
		STENCIL,
		LUMINANCE,
		CHROMINANCE,
	};

	enum class VideoFrameType
	{
		Intra,
		Predictive,
	};

	enum class VideoProfile
	{
		H264,	// AVC
	};

	enum class ComponentSwizzle
	{
		R,
		G,
		B,
		A,
		ZERO,
		ONE,
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
		TRANSIENT_ATTACHMENT = 1 << 6,	// hint: used in renderpass, without needing to write content to memory (VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
		SPARSE = 1 << 7,	// sparse resource without backing memory allocation
		SPARSE_TILE_POOL_BUFFER = 1 << 8,				// buffer only, makes it suitable for containing tile memory for sparse buffers
		SPARSE_TILE_POOL_TEXTURE_NON_RT_DS = 1 << 9,	// buffer only, makes it suitable for containing tile memory for sparse textures that are non render targets nor depth stencils
		SPARSE_TILE_POOL_TEXTURE_RT_DS = 1 << 10,		// buffer only, makes it suitable for containing tile memory for sparse textures that are either render targets or depth stencils
		SPARSE_TILE_POOL = SPARSE_TILE_POOL_BUFFER | SPARSE_TILE_POOL_TEXTURE_NON_RT_DS | SPARSE_TILE_POOL_TEXTURE_RT_DS, // buffer only, makes it suitable for containing tile memory for all kinds of sparse resources. Requires GraphicsDeviceCapability::GENERIC_SPARSE_TILE_POOL to be supported
		TYPED_FORMAT_CASTING = 1 << 11,	// enable casting formats between same type and different modifiers: eg. UNORM -> SRGB
		TYPELESS_FORMAT_CASTING = 1 << 12,	// enable casting formats to other formats that have the same bit-width and channel layout: eg. R32_FLOAT -> R32_UINT
		VIDEO_DECODE = 1 << 13,	// resource is usabe in video decoding operations
		NO_DEFAULT_DESCRIPTORS = 1 << 14, // skips creation of default descriptors for resources
		TEXTURE_COMPATIBLE_COMPRESSION = 1 << 15, // optimization that can enable sampling from compressed textures
		SHARED = 1 << 16, // shared texture
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
		SPARSE_BUFFER = 1 << 13,
		SPARSE_TEXTURE2D = 1 << 14,
		SPARSE_TEXTURE3D = 1 << 15,
		SPARSE_NULL_MAPPING = 1 << 16,
		GENERIC_SPARSE_TILE_POOL = 1 << 17, // allows using ResourceMiscFlag::SPARSE_TILE_POOL (non resource type specific version)
		DEPTH_RESOLVE_MIN_MAX = 1 << 18,
		STENCIL_RESOLVE_MIN_MAX = 1 << 19,
		CACHE_COHERENT_UMA = 1 << 20,	// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_architecture
		VIDEO_DECODE_H264 = 1 << 21,
		R9G9B9E5_SHAREDEXP_RENDERABLE = 1 << 22, // indicates supporting R9G9B9E5_SHAREDEXP format for rendering to
		COPY_BETWEEN_DIFFERENT_IMAGE_ASPECTS_NOT_SUPPORTED = 1 << 23, // indicates that CopyTexture src and dst ImageAspect must match
	};

	enum class ResourceState
	{
		// Common resource states:
		UNDEFINED = 0,						// invalid state (don't preserve contents)
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
		INDIRECT_ARGUMENT = 1 << 12,		// argument buffer to DrawIndirect() or DispatchIndirect()
		RAYTRACING_ACCELERATION_STRUCTURE = 1 << 13, // acceleration structure storage or scratch
		PREDICATION = 1 << 14,				// storage for predication comparison value

		// Other:
		VIDEO_DECODE_SRC = 1 << 15,			// video decode operation source (bitstream buffer or DPB texture)
		VIDEO_DECODE_DST = 1 << 16,			// video decode operation destination DPB texture
	};

	enum class ColorSpace
	{
		SRGB,			// SDR color space (8 or 10 bits per channel)
		HDR10_ST2084,	// HDR10 color space (10 bits per channel)
		HDR_LINEAR,		// HDR color space (16 bits per channel)
	};

	enum class RenderPassFlags
	{
		NONE = 0,
		ALLOW_UAV_WRITES = 1 << 0,
		SUSPENDING = 1 << 1,
		RESUMING = 1 << 2,
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

	struct Swizzle
	{
		ComponentSwizzle r = ComponentSwizzle::R;
		ComponentSwizzle g = ComponentSwizzle::G;
		ComponentSwizzle b = ComponentSwizzle::B;
		ComponentSwizzle a = ComponentSwizzle::A;
	};

	struct TextureDesc
	{
		enum class Type
		{
			TEXTURE_1D,
			TEXTURE_2D,
			TEXTURE_3D,
		} type = Type::TEXTURE_2D;
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
		uint32_t array_size = 1;
		uint32_t mip_levels = 1;
		Format format = Format::UNKNOWN;
		uint32_t sample_count = 1;
		Usage usage = Usage::DEFAULT;
		BindFlag bind_flags = BindFlag::NONE;
		ResourceMiscFlag misc_flags = ResourceMiscFlag::NONE;
		ClearValue clear = {};
		ResourceState layout = ResourceState::SHADER_RESOURCE;
		Swizzle swizzle;
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
		uint32_t stride = 0; // only needed for structured buffer types!
		Format format = Format::UNKNOWN; // only needed for typed buffer!
		uint64_t alignment = 0; // needed for tile pools
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
			const ImageAspect* aspect;
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
			int mip = -1, int slice = -1, const ImageAspect* aspect = nullptr)
		{
			GPUBarrier barrier;
			barrier.type = Type::IMAGE;
			barrier.image.texture = texture;
			barrier.image.layout_before = before;
			barrier.image.layout_after = after;
			barrier.image.mip = mip;
			barrier.image.slice = slice;
			barrier.image.aspect = aspect;
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

	struct SubresourceData
	{
		const void* data_ptr = nullptr;	// pointer to the beginning of the subresource data (pointer to beginning of resource + subresource offset)
		uint32_t row_pitch = 0;			// bytes between two rows of a texture (2D and 3D textures)
		uint32_t slice_pitch = 0;		// bytes between two depth slices of a texture (3D textures only)
	};

	struct Rect
	{
		int32_t left = 0;
		int32_t top = 0;
		int32_t right = 0;
		int32_t bottom = 0;
	};

	struct Box
	{
		uint32_t left = 0;
		uint32_t top = 0;
		uint32_t front = 0;
		uint32_t right = 0;
		uint32_t bottom = 0;
		uint32_t back = 0;
	};

	struct SparseTextureProperties
	{
		uint32_t tile_width = 0;				// width of 1 tile in texels
		uint32_t tile_height = 0;				// height of 1 tile in texels
		uint32_t tile_depth = 0;				// depth of 1 tile in texels
		uint32_t total_tile_count = 0;			// number of tiles for entire resource
		uint32_t packed_mip_start = 0;			// first mip of packed mipmap levels, these cannot be individually mapped and they cannot use a box mapping
		uint32_t packed_mip_count = 0;			// number of packed mipmap levels, these cannot be individually mapped and they cannot use a box mapping
		uint32_t packed_mip_tile_offset = 0;	// offset of the tiles for packed mip data relative to the entire resource
		uint32_t packed_mip_tile_count = 0;		// how many tiles are required for the packed mipmaps
	};

	struct VideoDesc
	{
		uint32_t width = 0;					// must meet the codec specific alignment requirements
		uint32_t height = 0;				// must meet the codec specific alignment requirements
		uint32_t bit_rate = 0;				// can be 0, it means that decoding will be prepared for worst case
		Format format = Format::NV12;
		VideoProfile profile = VideoProfile::H264;
		const void* pps_datas = nullptr;	// array of picture parameter set structures. The structure type depends on video codec
		size_t pps_count = 0;				// number of picture parameter set structures in the pps_datas array
		const void* sps_datas = nullptr;	// array of sequence parameter set structures. The structure type depends on video codec
		size_t sps_count = 0;				// number of sequence parameter set structures in the sps_datas array
		uint32_t num_dpb_slots = 0;			// The number of decode picture buffer slots. Usually it is required to be at least number_of_reference_frames + 1
	};


	// Resources:

	struct GraphicsDeviceChild
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state != nullptr; }

		virtual ~GraphicsDeviceChild() = default;
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
		constexpr bool IsTexture() const { return type == Type::TEXTURE; }
		constexpr bool IsBuffer() const { return type == Type::BUFFER; }
		constexpr bool IsAccelerationStructure() const { return type == Type::RAYTRACING_ACCELERATION_STRUCTURE; }

		// These are only valid if the resource was created with CPU access (USAGE::UPLOAD or USAGE::READBACK)
		void* mapped_data = nullptr;	// for buffers, it is a pointer to the buffer data; for textures, it is a pointer to texture data with linear tiling;
		size_t mapped_size = 0;			// for buffers, it is the full buffer size; for textures it is the full texture size including all subresources;

		size_t sparse_page_size = 0ull;	// specifies the required alignment of backing allocation for sparse tile pool
	};

	struct GPUBuffer : public GPUResource
	{
		GPUBufferDesc desc;

		constexpr const GPUBufferDesc& GetDesc() const { return desc; }
	};

	struct Texture : public GPUResource
	{
		TextureDesc	desc;

		// These are only valid if the texture was created with CPU access (USAGE::UPLOAD or USAGE::READBACK)
		const SubresourceData* mapped_subresources = nullptr;	// an array of subresource mappings in the following memory layout: slice0|mip0, slice0|mip1, slice0|mip2, ... sliceN|mipN
		size_t mapped_subresource_count = 0;					// the array size of mapped_subresources (number of slices * number of miplevels)

		// These are only valid if texture was created with ResourceMiscFlag::SPARSE flag:
		const SparseTextureProperties* sparse_properties = nullptr;

#if defined(_WIN32)
		void* shared_handle = nullptr; /* HANDLE */
#else
		int shared_handle = 0;
#endif

		constexpr const TextureDesc& GetDesc() const { return desc; }
	};

	struct VideoDecoder : public GraphicsDeviceChild
	{
		VideoDesc desc;
		constexpr const VideoDesc& GetDesc() const { return desc; }
	};

	struct VideoDecodeOperation
	{
		enum Flags
		{
			FLAG_EMPTY = 0,
			FLAG_SESSION_RESET = 1 << 0, // first usage of decoder needs reset
		};
		uint32_t flags = FLAG_EMPTY;
		const GPUBuffer* stream = nullptr;
		uint64_t stream_offset = 0; // must be aligned with GraphicsDevice::GetVideoDecodeBitstreamAlignment()
		uint64_t stream_size = 0;
		VideoFrameType frame_type = VideoFrameType::Intra;
		uint32_t reference_priority = 0; // nal_ref_idc from nal unit header
		int decoded_frame_index = 0; // frame index in order of decoding
		const void* slice_header = nullptr; // slice header for current frame
		const void* pps = nullptr; // picture parameter set for current slice header
		const void* sps = nullptr; // sequence parameter set for current picture parameter set
		int poc[2] = {}; // PictureOrderCount Top and Bottom fields
		uint32_t current_dpb = 0; // DPB slot for current output picture
		uint8_t dpb_reference_count = 0; // number of references in dpb_reference_slots array
		const uint8_t* dpb_reference_slots = nullptr; // dpb slot indices that are used as reference pictures
		const int* dpb_poc = nullptr; // for each DPB reference slot, indicate the PictureOrderCount
		const int* dpb_framenum = nullptr; // for each DPB reference slot, indicate the framenum value
		const Texture* DPB = nullptr; // DPB texture with arraysize = num_references + 1
	};

	struct RenderPassImage
	{
		enum class Type
		{
			RENDERTARGET,
			DEPTH_STENCIL,
			RESOLVE, // resolve render target (color)
			RESOLVE_DEPTH,
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
		ResourceState layout_before = ResourceState::UNDEFINED;	// layout before the render pass
		ResourceState layout = ResourceState::UNDEFINED;	// layout within the render pass
		ResourceState layout_after = ResourceState::UNDEFINED;	// layout after the render pass
		enum class DepthResolveMode
		{
			Min,
			Max,
		} depth_resolve_mode = DepthResolveMode::Min;

		static RenderPassImage RenderTarget(
			const Texture* resource,
			LoadOp load_op = LoadOp::LOAD,
			StoreOp store_op = StoreOp::STORE,
			ResourceState layout_before = ResourceState::SHADER_RESOURCE,
			ResourceState layout_after = ResourceState::SHADER_RESOURCE,
			int subresource_RTV = -1
		)
		{
			RenderPassImage image;
			image.type = Type::RENDERTARGET;
			image.texture = resource;
			image.loadop = load_op;
			image.storeop = store_op;
			image.layout_before = layout_before;
			image.layout = ResourceState::RENDERTARGET;
			image.layout_after = layout_after;
			image.subresource = subresource_RTV;
			return image;
		}

		static RenderPassImage DepthStencil(
			const Texture* resource,
			LoadOp load_op = LoadOp::LOAD,
			StoreOp store_op = StoreOp::STORE,
			ResourceState layout_before = ResourceState::DEPTHSTENCIL,
			ResourceState layout = ResourceState::DEPTHSTENCIL,
			ResourceState layout_after = ResourceState::DEPTHSTENCIL,
			int subresource_DSV = -1
		)
		{
			RenderPassImage image;
			image.type = Type::DEPTH_STENCIL;
			image.texture = resource;
			image.loadop = load_op;
			image.storeop = store_op;
			image.layout_before = layout_before;
			image.layout = layout;
			image.layout_after = layout_after;
			image.subresource = subresource_DSV;
			return image;
		}

		static RenderPassImage Resolve(
			const Texture* resource,
			ResourceState layout_before = ResourceState::SHADER_RESOURCE,
			ResourceState layout_after = ResourceState::SHADER_RESOURCE,
			int subresource_SRV = -1
		)
		{
			RenderPassImage image;
			image.type = Type::RESOLVE;
			image.texture = resource;
			image.layout_before = layout_before;
			image.layout = ResourceState::COPY_DST;
			image.layout_after = layout_after;
			image.subresource = subresource_SRV;
			return image;
		}

		static RenderPassImage ResolveDepth(
			const Texture* resource,
			DepthResolveMode depth_resolve_mode = DepthResolveMode::Min,
			ResourceState layout_before = ResourceState::SHADER_RESOURCE,
			ResourceState layout_after = ResourceState::SHADER_RESOURCE,
			int subresource_SRV = -1
		)
		{
			RenderPassImage image;
			image.type = Type::RESOLVE_DEPTH;
			image.texture = resource;
			image.layout_before = layout_before;
			image.layout = ResourceState::COPY_DST;
			image.layout_after = layout_after;
			image.subresource = subresource_SRV;
			image.depth_resolve_mode = depth_resolve_mode;
			return image;
		}

		static RenderPassImage ShadingRateSource(
			const Texture* resource,
			ResourceState layout_before = ResourceState::SHADING_RATE_SOURCE,
			ResourceState layout_after = ResourceState::SHADING_RATE_SOURCE
		)
		{
			RenderPassImage image;
			image.type = Type::SHADING_RATE_SOURCE;
			image.texture = resource;
			image.layout_before = layout_before;
			image.layout = ResourceState::SHADING_RATE_SOURCE;
			image.layout_after = layout_after;
			return image;
		}
	};

	struct RenderPassInfo
	{
		Format rt_formats[8] = {};
		uint32_t rt_count = 0;
		Format ds_format = Format::UNKNOWN;
		uint32_t sample_count = 1;

		constexpr uint64_t get_hash() const
		{
			union Hasher
			{
				struct
				{
					uint64_t rt_format_0 : 6;
					uint64_t rt_format_1 : 6;
					uint64_t rt_format_2 : 6;
					uint64_t rt_format_3 : 6;
					uint64_t rt_format_4 : 6;
					uint64_t rt_format_5 : 6;
					uint64_t rt_format_6 : 6;
					uint64_t rt_format_7 : 6;
					uint64_t ds_format : 6;
					uint64_t sample_count : 3;
				} bits;
				uint64_t value;
			} hasher = {};
			static_assert(sizeof(Hasher) == sizeof(uint64_t));
			hasher.bits.rt_format_0 = (uint64_t)rt_formats[0];
			hasher.bits.rt_format_1 = (uint64_t)rt_formats[1];
			hasher.bits.rt_format_2 = (uint64_t)rt_formats[2];
			hasher.bits.rt_format_3 = (uint64_t)rt_formats[3];
			hasher.bits.rt_format_4 = (uint64_t)rt_formats[4];
			hasher.bits.rt_format_5 = (uint64_t)rt_formats[5];
			hasher.bits.rt_format_6 = (uint64_t)rt_formats[6];
			hasher.bits.rt_format_7 = (uint64_t)rt_formats[7];
			hasher.bits.ds_format = (uint64_t)ds_format;
			hasher.bits.sample_count = (uint64_t)sample_count;
			return hasher.value;
		}
		static constexpr RenderPassInfo from(const RenderPassImage* images, uint32_t image_count)
		{
			RenderPassInfo info;
			for (uint32_t i = 0; i < image_count; ++i)
			{
				const RenderPassImage& image = images[i];
				const TextureDesc& desc = image.texture->GetDesc();
				switch (image.type)
				{
				case RenderPassImage::Type::RENDERTARGET:
					info.rt_formats[info.rt_count++] = desc.format;
					info.sample_count = desc.sample_count;
					break;
				case RenderPassImage::Type::DEPTH_STENCIL:
					info.ds_format = desc.format;
					info.sample_count = desc.sample_count;
					break;
				default:
					break;
				}
			}
			return info;
		}
		static constexpr RenderPassInfo from(const SwapChainDesc& swapchain_desc)
		{
			RenderPassInfo info;
			info.rt_count = 1;
			info.rt_formats[0] = swapchain_desc.format;
			return info;
		}
	};

	struct GPUQueryHeap : public GraphicsDeviceChild
	{
		GPUQueryHeapDesc desc;

		constexpr const GPUQueryHeapDesc& GetDesc() const { return desc; }
	};

	struct PipelineState : public GraphicsDeviceChild
	{
		PipelineStateDesc desc;

		constexpr const PipelineStateDesc& GetDesc() const { return desc; }
	};

	struct SwapChain : public GraphicsDeviceChild
	{
		SwapChainDesc desc;

		constexpr const SwapChainDesc& GetDesc() const { return desc; }
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
					uint64_t index_offset = 0;
					uint32_t vertex_count = 0;
					uint64_t vertex_byte_offset = 0;
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
				const GPUResource* bottom_level = nullptr;
			};
			GPUBuffer instance_buffer;
			uint32_t offset = 0;
			uint32_t count = 0;
		} top_level;
	};
	struct RaytracingAccelerationStructure : public GPUResource
	{
		RaytracingAccelerationStructureDesc desc;

		size_t size = 0;

		constexpr const RaytracingAccelerationStructureDesc& GetDesc() const { return desc; }
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

		constexpr const RaytracingPipelineStateDesc& GetDesc() const { return desc; }
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

	struct SparseResourceCoordinate
	{
		uint32_t x = 0;		// tile offset of buffer or texture in width
		uint32_t y = 0;		// tile offset of texture in height
		uint32_t z = 0;		// tile offset of 3D texture in depth
		uint32_t mip = 0;	// mip level of texture resource
		uint32_t slice = 0;	// array slice of texture resource
	};
	struct SparseRegionSize
	{
		uint32_t width = 1;		// number of tiles to be mapped in X dimension (buffer or texture)
		uint32_t height = 1;	// number of tiles to be mapped in Y dimension (texture only)
		uint32_t depth = 1;		// number of tiles to be mapped in Z dimension (3D texture only)
	};
	enum class TileRangeFlags
	{
		None = 0,		// map page to tile memory
		Null = 1 << 0,	// set page to null
	};
	struct SparseUpdateCommand
	{
		const GPUResource* sparse_resource = nullptr;			// the resource to do sparse mapping for (this requires resource to be created with ResourceMisc::SPARSE)
		uint32_t num_resource_regions = 0;						// number of: coordinates, sizes
		const SparseResourceCoordinate* coordinates = nullptr;	// mapping coordinates within sparse resource (num_resource_regions array size)
		const SparseRegionSize* sizes = nullptr;				// mapping sizes within sparse resource (num_resource_regions array size)
		const GPUBuffer* tile_pool = nullptr;					// this buffer must have been created with ResourceMisc::TILE_POOL
		const TileRangeFlags* range_flags = nullptr;			// flags (num_ranges array size)
		const uint32_t* range_start_offsets = nullptr;			// offset within tile pool (in pages) (num_ranges array size)
		const uint32_t* range_tile_counts = nullptr;			// number of tiles to be mapped (num_ranges array size)
	};


	constexpr bool IsFormatSRGB(Format format)
	{
		switch (format)
		{
		case Format::R8G8B8A8_UNORM_SRGB:
		case Format::B8G8R8A8_UNORM_SRGB:
		case Format::BC1_UNORM_SRGB:
		case Format::BC2_UNORM_SRGB:
		case Format::BC3_UNORM_SRGB:
		case Format::BC7_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}
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
		case Format::BC1_UNORM:
		case Format::BC1_UNORM_SRGB:
		case Format::BC2_UNORM:
		case Format::BC2_UNORM_SRGB:
		case Format::BC3_UNORM:
		case Format::BC3_UNORM_SRGB:
		case Format::BC4_UNORM:
		case Format::BC5_UNORM:
		case Format::BC7_UNORM:
		case Format::BC7_UNORM_SRGB:
			return true;
		default:
			return false;
		}
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
		default:
			return false;
		}
	}
	constexpr bool IsFormatDepthSupport(Format format)
	{
		switch (format)
		{
		case Format::D16_UNORM:
		case Format::D32_FLOAT:
		case Format::D32_FLOAT_S8X24_UINT:
		case Format::D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}
	constexpr bool IsFormatStencilSupport(Format format)
	{
		switch (format)
		{
		case Format::D32_FLOAT_S8X24_UINT:
		case Format::D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
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
		case Format::D32_FLOAT:
		case Format::R32_FLOAT:
		case Format::R32_UINT:
		case Format::R32_SINT:
		case Format::D24_UNORM_S8_UINT:
		case Format::R9G9B9E5_SHAREDEXP:
			return 4u;

		case Format::R8G8_UNORM:
		case Format::R8G8_UINT:
		case Format::R8G8_SNORM:
		case Format::R8G8_SINT:
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
			return 16u;
		}
	}
	constexpr Format GetFormatNonSRGB(Format format)
	{
		switch (format)
		{
		case Format::R8G8B8A8_UNORM_SRGB:
			return Format::R8G8B8A8_UNORM;
		case Format::B8G8R8A8_UNORM_SRGB:
			return Format::B8G8R8A8_UNORM;
		case Format::BC1_UNORM_SRGB:
			return Format::BC1_UNORM;
		case Format::BC2_UNORM_SRGB:
			return Format::BC2_UNORM;
		case Format::BC3_UNORM_SRGB:
			return Format::BC3_UNORM;
		case Format::BC7_UNORM_SRGB:
			return Format::BC7_UNORM;
		default:
			return format;
		}
	}
	constexpr Format GetFormatSRGB(Format format)
	{
		switch (format)
		{
		case Format::R8G8B8A8_UNORM:
		case Format::R8G8B8A8_UNORM_SRGB:
			return Format::R8G8B8A8_UNORM_SRGB;
		case Format::B8G8R8A8_UNORM:
		case Format::B8G8R8A8_UNORM_SRGB:
			return Format::B8G8R8A8_UNORM_SRGB;
		case Format::BC1_UNORM:
		case Format::BC1_UNORM_SRGB:
			return Format::BC1_UNORM_SRGB;
		case Format::BC2_UNORM:
		case Format::BC2_UNORM_SRGB:
			return Format::BC2_UNORM_SRGB;
		case Format::BC3_UNORM:
		case Format::BC3_UNORM_SRGB:
			return Format::BC3_UNORM_SRGB;
		case Format::BC7_UNORM:
		case Format::BC7_UNORM_SRGB:
			return Format::BC7_UNORM_SRGB;
		default:
			return Format::UNKNOWN;
		}
	}
	constexpr const char* GetFormatString(Format format)
	{
		switch (format)
		{
		case wi::graphics::Format::UNKNOWN:
			return "UNKNOWN";
		case wi::graphics::Format::R32G32B32A32_FLOAT:
			return "R32G32B32A32_FLOAT";
		case wi::graphics::Format::R32G32B32A32_UINT:
			return "R32G32B32A32_UINT";
		case wi::graphics::Format::R32G32B32A32_SINT:
			return "R32G32B32A32_SINT";
		case wi::graphics::Format::R32G32B32_FLOAT:
			return "R32G32B32_FLOAT";
		case wi::graphics::Format::R32G32B32_UINT:
			return "R32G32B32_UINT";
		case wi::graphics::Format::R32G32B32_SINT:
			return "R32G32B32_SINT";
		case wi::graphics::Format::R16G16B16A16_FLOAT:
			return "R16G16B16A16_FLOAT";
		case wi::graphics::Format::R16G16B16A16_UNORM:
			return "R16G16B16A16_UNORM";
		case wi::graphics::Format::R16G16B16A16_UINT:
			return "R16G16B16A16_UINT";
		case wi::graphics::Format::R16G16B16A16_SNORM:
			return "R16G16B16A16_SNORM";
		case wi::graphics::Format::R16G16B16A16_SINT:
			return "R16G16B16A16_SINT";
		case wi::graphics::Format::R32G32_FLOAT:
			return "R32G32_FLOAT";
		case wi::graphics::Format::R32G32_UINT:
			return "R32G32_UINT";
		case wi::graphics::Format::R32G32_SINT:
			return "R32G32_SINT";
		case wi::graphics::Format::D32_FLOAT_S8X24_UINT:
			return "D32_FLOAT_S8X24_UINT";
		case wi::graphics::Format::R10G10B10A2_UNORM:
			return "R10G10B10A2_UNORM";
		case wi::graphics::Format::R10G10B10A2_UINT:
			return "R10G10B10A2_UINT";
		case wi::graphics::Format::R11G11B10_FLOAT:
			return "R11G11B10_FLOAT";
		case wi::graphics::Format::R8G8B8A8_UNORM:
			return "R8G8B8A8_UNORM";
		case wi::graphics::Format::R8G8B8A8_UNORM_SRGB:
			return "R8G8B8A8_UNORM_SRGB";
		case wi::graphics::Format::R8G8B8A8_UINT:
			return "R8G8B8A8_UINT";
		case wi::graphics::Format::R8G8B8A8_SNORM:
			return "R8G8B8A8_SNORM";
		case wi::graphics::Format::R8G8B8A8_SINT:
			return "R8G8B8A8_SINT";
		case wi::graphics::Format::B8G8R8A8_UNORM:
			return "B8G8R8A8_UNORM";
		case wi::graphics::Format::B8G8R8A8_UNORM_SRGB:
			return "B8G8R8A8_UNORM_SRGB";
		case wi::graphics::Format::R16G16_FLOAT:
			return "R16G16_FLOAT";
		case wi::graphics::Format::R16G16_UNORM:
			return "R16G16_UNORM";
		case wi::graphics::Format::R16G16_UINT:
			return "R16G16_UINT";
		case wi::graphics::Format::R16G16_SNORM:
			return "R16G16_SNORM";
		case wi::graphics::Format::R16G16_SINT:
			return "R16G16_SINT";
		case wi::graphics::Format::D32_FLOAT:
			return "D32_FLOAT";
		case wi::graphics::Format::R32_FLOAT:
			return "R32_FLOAT";
		case wi::graphics::Format::R32_UINT:
			return "R32_UINT";
		case wi::graphics::Format::R32_SINT:
			return "R32_SINT";
		case wi::graphics::Format::D24_UNORM_S8_UINT:
			return "D24_UNORM_S8_UINT";
		case wi::graphics::Format::R9G9B9E5_SHAREDEXP:
			return "R9G9B9E5_SHAREDEXP";
		case wi::graphics::Format::R8G8_UNORM:
			return "R8G8_UNORM";
		case wi::graphics::Format::R8G8_UINT:
			return "R8G8_UINT";
		case wi::graphics::Format::R8G8_SNORM:
			return "R8G8_SNORM";
		case wi::graphics::Format::R8G8_SINT:
			return "R8G8_SINT";
		case wi::graphics::Format::R16_FLOAT:
			return "R16_FLOAT";
		case wi::graphics::Format::D16_UNORM:
			return "D16_UNORM";
		case wi::graphics::Format::R16_UNORM:
			return "R16_UNORM";
		case wi::graphics::Format::R16_UINT:
			return "R16_UINT";
		case wi::graphics::Format::R16_SNORM:
			return "R16_SNORM";
		case wi::graphics::Format::R16_SINT:
			return "R16_SINT";
		case wi::graphics::Format::R8_UNORM:
			return "R8_UNORM";
		case wi::graphics::Format::R8_UINT:
			return "R8_UINT";
		case wi::graphics::Format::R8_SNORM:
			return "R8_SNORM";
		case wi::graphics::Format::R8_SINT:
			return "R8_SINT";
		case wi::graphics::Format::BC1_UNORM:
			return "BC1_UNORM";
		case wi::graphics::Format::BC1_UNORM_SRGB:
			return "BC1_UNORM_SRGB";
		case wi::graphics::Format::BC2_UNORM:
			return "BC2_UNORM";
		case wi::graphics::Format::BC2_UNORM_SRGB:
			return "BC2_UNORM_SRGB";
		case wi::graphics::Format::BC3_UNORM:
			return "BC3_UNORM";
		case wi::graphics::Format::BC3_UNORM_SRGB:
			return "BC3_UNORM_SRGB";
		case wi::graphics::Format::BC4_UNORM:
			return "BC4_UNORM";
		case wi::graphics::Format::BC4_SNORM:
			return "BC4_SNORM";
		case wi::graphics::Format::BC5_UNORM:
			return "BC5_UNORM";
		case wi::graphics::Format::BC5_SNORM:
			return "BC5_SNORM";
		case wi::graphics::Format::BC6H_UF16:
			return "BC6H_UF16";
		case wi::graphics::Format::BC6H_SF16:
			return "BC6H_SF16";
		case wi::graphics::Format::BC7_UNORM:
			return "BC7_UNORM";
		case wi::graphics::Format::BC7_UNORM_SRGB:
			return "BC7_UNORM_SRGB";
		case wi::graphics::Format::NV12:
			return "NV12";
		default:
			return "";
		}
	}
	constexpr IndexBufferFormat GetIndexBufferFormat(Format format)
	{
		switch (format)
		{
		default:
		case Format::R32_UINT:
			return IndexBufferFormat::UINT32;
		case Format::R16_UINT:
			return IndexBufferFormat::UINT16;
		}
	}
	constexpr IndexBufferFormat GetIndexBufferFormat(uint32_t vertex_count)
	{
		return vertex_count > 65536 ? IndexBufferFormat::UINT32 : IndexBufferFormat::UINT16;
	}
	constexpr Format GetIndexBufferFormatRaw(uint32_t vertex_count)
	{
		return vertex_count > 65536 ? Format::R32_UINT : Format::R16_UINT;
	}
	constexpr const char* GetIndexBufferFormatString(IndexBufferFormat format)
	{
		switch (format)
		{
		default:
		case IndexBufferFormat::UINT32:
			return "UINT32";
		case IndexBufferFormat::UINT16:
			return "UINT16";
		}
	}

	constexpr const char GetComponentSwizzleChar(ComponentSwizzle value)
	{
		switch (value)
		{
		default:
		case wi::graphics::ComponentSwizzle::R:
			return 'R';
		case wi::graphics::ComponentSwizzle::G:
			return 'G';
		case wi::graphics::ComponentSwizzle::B:
			return 'B';
		case wi::graphics::ComponentSwizzle::A:
			return 'A';
		case wi::graphics::ComponentSwizzle::ZERO:
			return '0';
		case wi::graphics::ComponentSwizzle::ONE:
			return '1';
		}
	}
	struct SwizzleString
	{
		char chars[5] = {};
		constexpr operator const char*() const { return chars; }
	};
	constexpr const SwizzleString GetSwizzleString(Swizzle swizzle)
	{
		SwizzleString ret;
		ret.chars[0] = GetComponentSwizzleChar(swizzle.r);
		ret.chars[1] = GetComponentSwizzleChar(swizzle.g);
		ret.chars[2] = GetComponentSwizzleChar(swizzle.b);
		ret.chars[3] = GetComponentSwizzleChar(swizzle.a);
		ret.chars[4] = 0;
		return ret;
	}
	constexpr Swizzle SwizzleFromString(const char* str)
	{
		Swizzle swizzle;
		if (str == nullptr)
			return swizzle;
		ComponentSwizzle* comp = (ComponentSwizzle*)&swizzle;
		for (int i = 0; i < 4; ++i)
		{
			switch (str[i])
			{
			case 'r':
			case 'R':
				*comp = ComponentSwizzle::R;
				break;
			case 'g':
			case 'G':
				*comp = ComponentSwizzle::G;
				break;
			case 'b':
			case 'B':
				*comp = ComponentSwizzle::B;
				break;
			case 'a':
			case 'A':
				*comp = ComponentSwizzle::A;
				break;
			case '0':
				*comp = ComponentSwizzle::ZERO;
				break;
			case '1':
				*comp = ComponentSwizzle::ONE;
				break;
			case 0:
				return swizzle;
			}
			comp++;
		}
		return swizzle;
	}

	template<typename T>
	constexpr T AlignTo(T value, T alignment)
	{
		return ((value + alignment - T(1)) / alignment) * alignment;
	}
	template<typename T>
	constexpr bool IsAligned(T value, T alignment)
	{
		return value == AlignTo(value, alignment);
	}

	// Get mipmap count for a given texture dimension.
	//	width, height, depth: dimensions of the texture
	//	min_dimension: constrain all dimensions to a specific resolution (optional, default: 1x1x1)
	//	required_alignment: make sure to only return so many levels so that dimensions remain aligned to a value (optional)
	constexpr uint32_t GetMipCount(uint32_t width, uint32_t height, uint32_t depth = 1u, uint32_t min_dimension = 1u, uint32_t required_alignment = 1u)
	{
		uint32_t mips = 1;
		while (width > min_dimension || height > min_dimension || depth > min_dimension)
		{
			width = std::max(min_dimension, width >> 1u);
			height = std::max(min_dimension, height >> 1u);
			depth = std::max(min_dimension, depth >> 1u);
			if (
				AlignTo(width, required_alignment) != width ||
				AlignTo(height, required_alignment) != height ||
				AlignTo(depth, required_alignment) != depth
				)
				break;
			mips++;
		}
		return mips;
	}

	// Compute the approximate texture memory usage
	//	Approximate because this doesn't reflect GPU specific texture memory requirements, like alignment and metadata
	constexpr size_t ComputeTextureMemorySizeInBytes(const TextureDesc& desc)
	{
		size_t size = 0;
		const uint32_t bytes_per_block = GetFormatStride(desc.format);
		const uint32_t pixels_per_block = GetFormatBlockSize(desc.format);
		const uint32_t num_blocks_x = desc.width / pixels_per_block;
		const uint32_t num_blocks_y = desc.height / pixels_per_block;
		const uint32_t mips = desc.mip_levels == 0 ? GetMipCount(desc.width, desc.height, desc.depth) : desc.mip_levels;
		for (uint32_t layer = 0; layer < desc.array_size; ++layer)
		{
			for (uint32_t mip = 0; mip < mips; ++mip)
			{
				const uint32_t width = std::max(1u, num_blocks_x >> mip);
				const uint32_t height = std::max(1u, num_blocks_y >> mip);
				const uint32_t depth = std::max(1u, desc.depth >> mip);
				size += width * height * depth * bytes_per_block;
			}
		}
		size *= desc.sample_count;
		return size;
	}


	// Deprecated, kept for back-compat:
	struct RenderPassAttachment
	{
		enum class Type
		{
			RENDERTARGET,
			DEPTH_STENCIL,
			RESOLVE, // resolve render target (color)
			RESOLVE_DEPTH,
			SHADING_RATE_SOURCE
		} type = Type::RENDERTARGET;
		enum class LoadOp
		{
			LOAD,
			CLEAR,
			DONTCARE,
		} loadop = LoadOp::LOAD;
		Texture texture;
		int subresource = -1;
		enum class StoreOp
		{
			STORE,
			DONTCARE,
		} storeop = StoreOp::STORE;
		ResourceState initial_layout = ResourceState::UNDEFINED;	// layout before the render pass
		ResourceState subpass_layout = ResourceState::UNDEFINED;	// layout within the render pass
		ResourceState final_layout = ResourceState::UNDEFINED;		// layout after the render pass
		enum class DepthResolveMode
		{
			Min,
			Max,
		} depth_resolve_mode = DepthResolveMode::Min;

		static RenderPassAttachment RenderTarget(
			const Texture& resource,
			LoadOp load_op = LoadOp::LOAD,
			StoreOp store_op = StoreOp::STORE,
			ResourceState initial_layout = ResourceState::SHADER_RESOURCE,
			ResourceState subpass_layout = ResourceState::RENDERTARGET,
			ResourceState final_layout = ResourceState::SHADER_RESOURCE,
			int subresource_RTV = -1
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
			attachment.subresource = subresource_RTV;
			return attachment;
		}

		static RenderPassAttachment DepthStencil(
			const Texture& resource,
			LoadOp load_op = LoadOp::LOAD,
			StoreOp store_op = StoreOp::STORE,
			ResourceState initial_layout = ResourceState::DEPTHSTENCIL,
			ResourceState subpass_layout = ResourceState::DEPTHSTENCIL,
			ResourceState final_layout = ResourceState::DEPTHSTENCIL,
			int subresource_DSV = -1
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
			attachment.subresource = subresource_DSV;
			return attachment;
		}

		static RenderPassAttachment Resolve(
			const Texture& resource,
			ResourceState initial_layout = ResourceState::SHADER_RESOURCE,
			ResourceState final_layout = ResourceState::SHADER_RESOURCE,
			int subresource_SRV = -1
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type::RESOLVE;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
			attachment.final_layout = final_layout;
			attachment.subresource = subresource_SRV;
			return attachment;
		}

		static RenderPassAttachment ResolveDepth(
			const Texture& resource,
			DepthResolveMode depth_resolve_mode = DepthResolveMode::Min,
			ResourceState initial_layout = ResourceState::SHADER_RESOURCE,
			ResourceState final_layout = ResourceState::SHADER_RESOURCE,
			int subresource_SRV = -1
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type::RESOLVE_DEPTH;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
			attachment.final_layout = final_layout;
			attachment.subresource = subresource_SRV;
			attachment.depth_resolve_mode = depth_resolve_mode;
			return attachment;
		}

		static RenderPassAttachment ShadingRateSource(
			const Texture& resource,
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

		constexpr operator RenderPassImage() const
		{
			RenderPassImage image;
			switch (type)
			{
			default:
			case Type::RENDERTARGET:
				image.type = RenderPassImage::Type::RENDERTARGET;
				break;
			case Type::DEPTH_STENCIL:
				image.type = RenderPassImage::Type::DEPTH_STENCIL;
				break;
			case Type::RESOLVE:
				image.type = RenderPassImage::Type::RESOLVE;
				break;
			case Type::RESOLVE_DEPTH:
				image.type = RenderPassImage::Type::RESOLVE_DEPTH;
				break;
			case Type::SHADING_RATE_SOURCE:
				image.type = RenderPassImage::Type::SHADING_RATE_SOURCE;
				break;
			}
			switch (depth_resolve_mode)
			{
			default:
			case DepthResolveMode::Min:
				image.depth_resolve_mode = RenderPassImage::DepthResolveMode::Min;
				break;
			case DepthResolveMode::Max:
				image.depth_resolve_mode = RenderPassImage::DepthResolveMode::Max;
				break;
			}
			switch (loadop)
			{
			case RenderPassAttachment::LoadOp::LOAD:
				image.loadop = RenderPassImage::LoadOp::LOAD;
				break;
			case RenderPassAttachment::LoadOp::CLEAR:
				image.loadop = RenderPassImage::LoadOp::CLEAR;
				break;
			case RenderPassAttachment::LoadOp::DONTCARE:
				image.loadop = RenderPassImage::LoadOp::DONTCARE;
				break;
			default:
				break;
			}
			switch (storeop)
			{
			case RenderPassAttachment::StoreOp::STORE:
				image.storeop = RenderPassImage::StoreOp::STORE;
				break;
			case RenderPassAttachment::StoreOp::DONTCARE:
				image.storeop = RenderPassImage::StoreOp::DONTCARE;
				break;
			default:
				break;
			}
			image.layout_before = initial_layout;
			image.layout = subpass_layout;
			image.layout_after = final_layout;
			image.texture = &texture;
			image.subresource = subresource;
			return image;
		}
	};
	// Deprecated, kept for back-compat:
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
	// Deprecated, kept for back-compat:
	struct RenderPass
	{
		bool valid = false;
		RenderPassDesc desc;

		constexpr const RenderPassDesc& GetDesc() const { return desc; }
		constexpr bool IsValid() const { return valid; }
	};
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
template<>
struct enable_bitmask_operators<wi::graphics::RenderPassFlags> {
	static const bool enable = true;
};
