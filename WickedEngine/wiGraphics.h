#pragma once
#include "CommonInclude.h"

#include <memory>
#include <vector>
#include <string>

namespace wiGraphics
{
	struct Shader;
	struct BlendState;
	struct RasterizerState;
	struct DepthStencilState;
	struct InputLayout;
	struct GPUResource;
	struct GPUBuffer;
	struct Texture;
	struct RootSignature;

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
		SHADERSTAGE_COUNT,
	};
	enum PRIMITIVETOPOLOGY
	{
		UNDEFINED,
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
	enum COLOR_WRITE_ENABLE
	{
		COLOR_WRITE_DISABLE = 0,
		COLOR_WRITE_ENABLE_RED = 1,
		COLOR_WRITE_ENABLE_GREEN = 2,
		COLOR_WRITE_ENABLE_BLUE = 4,
		COLOR_WRITE_ENABLE_ALPHA = 8,
		COLOR_WRITE_ENABLE_ALL = (((COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN) | COLOR_WRITE_ENABLE_BLUE) | COLOR_WRITE_ENABLE_ALPHA)
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
		USAGE_DEFAULT,
		USAGE_IMMUTABLE,
		USAGE_DYNAMIC,
		USAGE_STAGING,
	};
	enum TEXTURE_ADDRESS_MODE
	{
		TEXTURE_ADDRESS_WRAP,
		TEXTURE_ADDRESS_MIRROR,
		TEXTURE_ADDRESS_CLAMP,
		TEXTURE_ADDRESS_BORDER,
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
		FORMAT_R32G8X24_TYPELESS,		// depth + stencil (alias)
		FORMAT_D32_FLOAT_S8X24_UINT,	// depth + stencil

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
		FORMAT_R32_TYPELESS,			// depth (alias)
		FORMAT_D32_FLOAT,				// depth
		FORMAT_R32_FLOAT,
		FORMAT_R32_UINT,
		FORMAT_R32_SINT, 
		FORMAT_R24G8_TYPELESS,			// depth + stencil (alias)
		FORMAT_D24_UNORM_S8_UINT,		// depth + stencil

		FORMAT_R8G8_UNORM,
		FORMAT_R8G8_UINT,
		FORMAT_R8G8_SNORM,
		FORMAT_R8G8_SINT,
		FORMAT_R16_TYPELESS,			// depth (alias)
		FORMAT_R16_FLOAT,
		FORMAT_D16_UNORM,				// depth
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
		GPU_QUERY_TYPE_INVALID,				// do not use! Indicates if query was not created.
		GPU_QUERY_TYPE_EVENT,				// has the GPU reached this point?
		GPU_QUERY_TYPE_OCCLUSION,			// how many samples passed depthstencil test?
		GPU_QUERY_TYPE_OCCLUSION_PREDICATE, // are there any samples that passed depthstencil test
		GPU_QUERY_TYPE_TIMESTAMP,			// retrieve time point of gpu execution
		GPU_QUERY_TYPE_TIMESTAMP_DISJOINT,	// timestamp frequency information
	};
	enum INDEXBUFFER_FORMAT
	{
		INDEXFORMAT_16BIT,
		INDEXFORMAT_32BIT,
	};
	enum SUBRESOURCE_TYPE
	{
		SRV,
		UAV,
		RTV,
		DSV,
	};
	enum IMAGE_LAYOUT
	{
		IMAGE_LAYOUT_UNDEFINED,					// discard contents
		IMAGE_LAYOUT_GENERAL,					// supports everything
		IMAGE_LAYOUT_RENDERTARGET,				// render target, write enabled
		IMAGE_LAYOUT_DEPTHSTENCIL,				// depth stencil, write enabled
		IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,		// depth stencil, read only
		IMAGE_LAYOUT_SHADER_RESOURCE,			// shader resource, read only
		IMAGE_LAYOUT_UNORDERED_ACCESS,			// shader resource, write enabled
		IMAGE_LAYOUT_COPY_SRC,					// copy from
		IMAGE_LAYOUT_COPY_DST,					// copy to
		IMAGE_LAYOUT_SHADING_RATE_SOURCE,		// shading rate control per tile
	};
	enum BUFFER_STATE
	{
		BUFFER_STATE_GENERAL,					// supports everything
		BUFFER_STATE_VERTEX_BUFFER,				// vertex buffer, read only
		BUFFER_STATE_INDEX_BUFFER,				// index buffer, read only
		BUFFER_STATE_CONSTANT_BUFFER,			// constant buffer, read only
		BUFFER_STATE_INDIRECT_ARGUMENT,			// argument buffer to DrawIndirect() or DispatchIndirect()
		BUFFER_STATE_SHADER_RESOURCE,			// shader resource, read only
		BUFFER_STATE_UNORDERED_ACCESS,			// shader resource, write enabled
		BUFFER_STATE_COPY_SRC,					// copy from
		BUFFER_STATE_COPY_DST,					// copy to
		BUFFER_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
	};
	enum SHADING_RATE
	{
		SHADING_RATE_1X1,
		SHADING_RATE_1X2,
		SHADING_RATE_2X1,
		SHADING_RATE_2X2,
		SHADING_RATE_2X4,
		SHADING_RATE_4X2,
		SHADING_RATE_4X4
	};
	enum GRAPHICSDEVICE_CAPABILITY
	{
		GRAPHICSDEVICE_CAPABILITY_TESSELLATION,
		GRAPHICSDEVICE_CAPABILITY_CONSERVATIVE_RASTERIZATION,
		GRAPHICSDEVICE_CAPABILITY_RASTERIZER_ORDERED_VIEWS,
		GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_COMMON, // eg: R16G16B16A16_FLOAT, R8G8B8A8_UNORM and more common ones
		GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT,
		GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS,
		GRAPHICSDEVICE_CAPABILITY_RAYTRACING,
		GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE,
		GRAPHICSDEVICE_CAPABILITY_DESCRIPTOR_MANAGEMENT,
		GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING,
		GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2,
		GRAPHICSDEVICE_CAPABILITY_MESH_SHADER,
		GRAPHICSDEVICE_CAPABILITY_COUNT,
	};

	// Flags ////////////////////////////////////////////
	enum BIND_FLAG
	{
		BIND_VERTEX_BUFFER = 1 << 0,
		BIND_INDEX_BUFFER = 1 << 1,
		BIND_CONSTANT_BUFFER = 1 << 2,
		BIND_SHADER_RESOURCE = 1 << 3,
		BIND_STREAM_OUTPUT = 1 << 4,
		BIND_RENDER_TARGET = 1 << 5,
		BIND_DEPTH_STENCIL = 1 << 6,
		BIND_UNORDERED_ACCESS = 1 << 7,
	};
	enum CPU_ACCESS
	{
		CPU_ACCESS_WRITE = 1 << 0,
		CPU_ACCESS_READ = 1 << 1,
	};
	enum RESOURCE_MISC_FLAG
	{
		RESOURCE_MISC_SHARED = 1 << 0,
		RESOURCE_MISC_TEXTURECUBE = 1 << 1,
		RESOURCE_MISC_INDIRECT_ARGS = 1 << 2,
		RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS = 1 << 3,
		RESOURCE_MISC_BUFFER_STRUCTURED = 1 << 4,
		RESOURCE_MISC_TILED = 1 << 5,
		RESOURCE_MISC_RAY_TRACING = 1 << 0,
	};

	// Descriptor structs:

	struct Viewport
	{
		float TopLeftX = 0.0f;
		float TopLeftY = 0.0f;
		float Width = 0.0f;
		float Height = 0.0f;
		float MinDepth = 0.0f;
		float MaxDepth = 1.0f;
	};
	struct InputLayoutDesc
	{
		static const uint32_t APPEND_ALIGNED_ELEMENT = 0xffffffff; // automatically figure out AlignedByteOffset depending on Format

		std::string SemanticName;
		uint32_t SemanticIndex = 0;
		FORMAT Format = FORMAT_UNKNOWN;
		uint32_t InputSlot = 0;
		uint32_t AlignedByteOffset = APPEND_ALIGNED_ELEMENT;
		INPUT_CLASSIFICATION InputSlotClass = INPUT_CLASSIFICATION::INPUT_PER_VERTEX_DATA;
		uint32_t InstanceDataStepRate = 0;
	};
	union ClearValue
	{
		float color[4];
		struct ClearDepthStencil
		{
			float depth;
			uint32_t stencil;
		} depthstencil;
	};
	struct TextureDesc
	{
		enum TEXTURE_TYPE
		{
			TEXTURE_1D,
			TEXTURE_2D,
			TEXTURE_3D,
		} type = TEXTURE_2D;
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t Depth = 0;
		uint32_t ArraySize = 1;
		uint32_t MipLevels = 1;
		FORMAT Format = FORMAT_UNKNOWN;
		uint32_t SampleCount = 1;
		USAGE Usage = USAGE_DEFAULT;
		uint32_t BindFlags = 0;
		uint32_t CPUAccessFlags = 0;
		uint32_t MiscFlags = 0;
		ClearValue clear = {};
		IMAGE_LAYOUT layout = IMAGE_LAYOUT_GENERAL;
	};
	struct SamplerDesc
	{
		FILTER Filter = FILTER_MIN_MAG_MIP_POINT;
		TEXTURE_ADDRESS_MODE AddressU = TEXTURE_ADDRESS_CLAMP;
		TEXTURE_ADDRESS_MODE AddressV = TEXTURE_ADDRESS_CLAMP;
		TEXTURE_ADDRESS_MODE AddressW = TEXTURE_ADDRESS_CLAMP;
		float MipLODBias = 0.0f;
		uint32_t MaxAnisotropy = 0;
		COMPARISON_FUNC ComparisonFunc = COMPARISON_NEVER;
		float BorderColor[4] = { 0.0f,0.0f,0.0f,0.0f };
		float MinLOD = 0.0f;
		float MaxLOD = FLT_MAX;
	};
	struct RasterizerStateDesc
	{
		FILL_MODE FillMode = FILL_SOLID;
		CULL_MODE CullMode = CULL_NONE;
		bool FrontCounterClockwise = false;
		int32_t DepthBias = 0;
		float DepthBiasClamp = 0.0f;
		float SlopeScaledDepthBias = 0.0f;
		bool DepthClipEnable = false;
		bool MultisampleEnable = false;
		bool AntialiasedLineEnable = false;
		bool ConservativeRasterizationEnable = false;
		uint32_t ForcedSampleCount = 0;
	};
	struct DepthStencilOpDesc
	{
		STENCIL_OP StencilFailOp = STENCIL_OP_KEEP;
		STENCIL_OP StencilDepthFailOp = STENCIL_OP_KEEP;
		STENCIL_OP StencilPassOp = STENCIL_OP_KEEP;
		COMPARISON_FUNC StencilFunc = COMPARISON_NEVER;
	};
	struct DepthStencilStateDesc
	{
		bool DepthEnable = false;
		DEPTH_WRITE_MASK DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
		COMPARISON_FUNC DepthFunc = COMPARISON_NEVER;
		bool StencilEnable = false;
		uint8_t StencilReadMask = 0xff;
		uint8_t StencilWriteMask = 0xff;
		DepthStencilOpDesc FrontFace;
		DepthStencilOpDesc BackFace;
	};
	struct RenderTargetBlendStateDesc
	{
		bool BlendEnable = false;
		BLEND SrcBlend = BLEND_SRC_ALPHA;
		BLEND DestBlend = BLEND_INV_SRC_ALPHA;
		BLEND_OP BlendOp = BLEND_OP_ADD;
		BLEND SrcBlendAlpha = BLEND_ONE;
		BLEND DestBlendAlpha = BLEND_ONE;
		BLEND_OP BlendOpAlpha = BLEND_OP_ADD;
		uint8_t RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	};
	struct BlendStateDesc
	{
		bool AlphaToCoverageEnable = false;
		bool IndependentBlendEnable = false;
		RenderTargetBlendStateDesc RenderTarget[8];
	};
	struct GPUBufferDesc
	{
		uint32_t ByteWidth = 0;
		USAGE Usage = USAGE_DEFAULT;
		uint32_t BindFlags = 0;
		uint32_t CPUAccessFlags = 0;
		uint32_t MiscFlags = 0;
		uint32_t StructureByteStride = 0; // needed for typed and structured buffer types!
		FORMAT Format = FORMAT_UNKNOWN; // only needed for typed buffer!
	};
	struct GPUQueryDesc
	{
		GPU_QUERY_TYPE Type = GPU_QUERY_TYPE_INVALID;
	};
	struct GPUQueryResult
	{
		uint64_t	result_passed_sample_count = 0;
		uint64_t	result_timestamp = 0;
		uint64_t	result_timestamp_frequency = 0;
	};
	struct PipelineStateDesc
	{
		const RootSignature* rootSignature = nullptr;
		const Shader*				vs = nullptr;
		const Shader*				ps = nullptr;
		const Shader*				hs = nullptr;
		const Shader*				ds = nullptr;
		const Shader*				gs = nullptr;
		const Shader*				ms = nullptr;
		const Shader*				as = nullptr;
		const BlendState*			bs = nullptr;
		const RasterizerState*		rs = nullptr;
		const DepthStencilState*	dss = nullptr;
		const InputLayout*			il = nullptr;
		PRIMITIVETOPOLOGY			pt = TRIANGLELIST;
		uint32_t					sampleMask = 0xFFFFFFFF;
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
			IMAGE_LAYOUT layout_before;
			IMAGE_LAYOUT layout_after;
		};
		struct Buffer
		{
			const GPUBuffer* buffer;
			BUFFER_STATE state_before;
			BUFFER_STATE state_after;
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
		static GPUBarrier Image(const Texture* texture, IMAGE_LAYOUT before, IMAGE_LAYOUT after)
		{
			GPUBarrier barrier;
			barrier.type = IMAGE_BARRIER;
			barrier.image.texture = texture;
			barrier.image.layout_before = before;
			barrier.image.layout_after = after;
			return barrier;
		}
		static GPUBarrier Buffer(const GPUBuffer* buffer, BUFFER_STATE before, BUFFER_STATE after)
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
		IMAGE_LAYOUT initial_layout = IMAGE_LAYOUT_GENERAL;		// layout before the render pass
		IMAGE_LAYOUT final_layout = IMAGE_LAYOUT_GENERAL;		// layout after the render pass
		IMAGE_LAYOUT subpass_layout = IMAGE_LAYOUT_RENDERTARGET;// layout within the render pass

		static RenderPassAttachment RenderTarget(
			const Texture* resource = nullptr,
			LOAD_OPERATION load_op = LOADOP_LOAD,
			STORE_OPERATION store_op = STOREOP_STORE,
			IMAGE_LAYOUT initial_layout = IMAGE_LAYOUT_GENERAL,
			IMAGE_LAYOUT subpass_layout = IMAGE_LAYOUT_RENDERTARGET,
			IMAGE_LAYOUT final_layout = IMAGE_LAYOUT_GENERAL
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
			IMAGE_LAYOUT initial_layout = IMAGE_LAYOUT_DEPTHSTENCIL,
			IMAGE_LAYOUT subpass_layout = IMAGE_LAYOUT_DEPTHSTENCIL,
			IMAGE_LAYOUT final_layout = IMAGE_LAYOUT_DEPTHSTENCIL
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
			IMAGE_LAYOUT initial_layout = IMAGE_LAYOUT_GENERAL,
			IMAGE_LAYOUT final_layout = IMAGE_LAYOUT_GENERAL
		)
		{
			RenderPassAttachment attachment;
			attachment.type = RESOLVE;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
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
		uint32_t _flags = FLAG_EMPTY;
		std::vector<RenderPassAttachment> attachments;
	};
	struct IndirectDrawArgsInstanced
	{
		uint32_t VertexCountPerInstance = 0;
		uint32_t InstanceCount = 0;
		uint32_t StartVertexLocation = 0;
		uint32_t StartInstanceLocation = 0;
	};
	struct IndirectDrawArgsIndexedInstanced
	{
		uint32_t IndexCountPerInstance = 0;
		uint32_t InstanceCount = 0;
		uint32_t StartIndexLocation = 0;
		int32_t BaseVertexLocation = 0;
		uint32_t StartInstanceLocation = 0;
	};
	struct IndirectDispatchArgs
	{
		uint32_t ThreadGroupCountX = 0;
		uint32_t ThreadGroupCountY = 0;
		uint32_t ThreadGroupCountZ = 0;
	};
	struct SubresourceData
	{
		const void *pSysMem = nullptr;
		uint32_t SysMemPitch = 0;
		uint32_t SysMemSlicePitch = 0;
	};
	struct Rect
	{
		int32_t left = 0;
		int32_t top = 0;
		int32_t right = 0;
		int32_t bottom = 0;
	};
	struct Mapping
	{
		enum FLAGS
		{
			FLAG_EMPTY = 0,
			FLAG_READ = 1 << 0,
			FLAG_WRITE = 1 << 1,
		};
		uint32_t _flags = FLAG_EMPTY;
		size_t offset = 0;
		size_t size = 0;
		uint32_t rowpitch = 0;	// output
		void* data = nullptr;	// output
	};


	// Resources:

	struct GraphicsDeviceChild
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }
	};

	struct Shader : public GraphicsDeviceChild
	{
		SHADERSTAGE stage = SHADERSTAGE_COUNT;
		std::vector<uint8_t> code;
		const RootSignature* rootSignature = nullptr;
	};

	struct Sampler : public GraphicsDeviceChild
	{
		SamplerDesc desc;

		const SamplerDesc& GetDesc() const { return desc; }
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
	};

	struct GPUBuffer : public GPUResource
	{
		GPUBufferDesc desc;

		const GPUBufferDesc& GetDesc() const { return desc; }
	};

	struct InputLayout : public GraphicsDeviceChild
	{
		std::vector<InputLayoutDesc> desc;
	};

	struct BlendState : public GraphicsDeviceChild
	{
		BlendStateDesc desc;

		const BlendStateDesc& GetDesc() const { return desc; }
	};

	struct DepthStencilState : public GraphicsDeviceChild
	{
		DepthStencilStateDesc desc;

		const DepthStencilStateDesc& GetDesc() const { return desc; }
	};

	struct RasterizerState : public GraphicsDeviceChild
	{
		RasterizerStateDesc desc;

		const RasterizerStateDesc& GetDesc() const { return desc; }
	};

	struct Texture : public GPUResource
	{
		TextureDesc	desc;

		const TextureDesc& GetDesc() const { return desc; }
	};

	struct GPUQuery : public GraphicsDeviceChild
	{
		GPUQueryDesc desc;

		const GPUQueryDesc& GetDesc() const { return desc; }
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
		uint32_t _flags = FLAG_EMPTY;

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
				uint32_t _flags = FLAG_EMPTY;

				enum TYPE
				{
					TRIANGLES,
					PROCEDURAL_AABBS,
				} type = TRIANGLES;

				struct Triangles
				{
					GPUBuffer vertexBuffer;
					GPUBuffer indexBuffer;
					uint32_t indexCount = 0;
					uint32_t indexOffset = 0;
					uint32_t vertexCount = 0;
					uint32_t vertexByteOffset = 0;
					uint32_t vertexStride = 0;
					INDEXBUFFER_FORMAT indexFormat = INDEXFORMAT_32BIT;
					FORMAT vertexFormat = FORMAT_R32G32B32_FLOAT;
					GPUBuffer transform3x4Buffer;
					uint32_t transform3x4BufferOffset = 0;
				} triangles;
				struct Procedural_AABBs
				{
					GPUBuffer aabbBuffer;
					uint32_t offset = 0;
					uint32_t count = 0;
					uint32_t stride = 0;
				} aabbs;

			};
			std::vector<Geometry> geometries;
		} bottomlevel;

		struct TopLevel
		{
			struct Instance
			{
				XMFLOAT3X4 transform;
				uint32_t InstanceID : 24;
				uint32_t InstanceMask : 8;
				uint32_t InstanceContributionToHitGroupIndex : 24;
				uint32_t Flags : 8;
				GPUResource bottomlevel;
			};
			GPUBuffer instanceBuffer;
			uint32_t offset = 0;
			uint32_t count = 0;
		} toplevel;
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
		uint32_t closesthit_shader = ~0;
		uint32_t anyhit_shader = ~0;
		uint32_t intersection_shader = ~0;
	};
	struct RaytracingPipelineStateDesc
	{
		const RootSignature* rootSignature = nullptr;
		std::vector<ShaderLibrary> shaderlibraries;
		std::vector<ShaderHitGroup> hitgroups;
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
		ShaderTable raygeneration;
		ShaderTable miss;
		ShaderTable hitgroup;
		ShaderTable callable;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Depth = 1;
	};

	enum BINDPOINT
	{
		GRAPHICS,
		COMPUTE,
		RAYTRACING,
	};
	enum RESOURCEBINDING
	{
		ROOT_CONSTANTBUFFER,
		ROOT_RAWBUFFER,
		ROOT_STRUCTUREDBUFFER,
		ROOT_RWRAWBUFFER,
		ROOT_RWSTRUCTUREDBUFFER,

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
	struct ResourceRange
	{
		RESOURCEBINDING binding = CONSTANTBUFFER;
		uint32_t slot = 0;
		uint32_t count = 1;
	};
	struct SamplerRange
	{
		uint32_t slot = 0;
		uint32_t count = 1;
	};
	struct StaticSampler
	{
		Sampler sampler;
		uint32_t slot = 0;
	};
	struct DescriptorTable : public GraphicsDeviceChild
	{
		SHADERSTAGE stage = SHADERSTAGE_COUNT;
		std::vector<ResourceRange> resources;
		std::vector<SamplerRange> samplers;
		std::vector<StaticSampler> staticsamplers;
	};
	struct RootConstantRange
	{
		SHADERSTAGE stage = SHADERSTAGE_COUNT;
		uint32_t slot = 0;
		uint32_t size = 0;
		uint32_t offset = 0;
	};
	struct RootSignature : public GraphicsDeviceChild
	{
		enum FLAGS
		{
			FLAG_EMPTY = 0,
			FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT = 1 << 0,
		};
		uint32_t _flags = FLAG_EMPTY;
		std::vector<DescriptorTable> tables;
		std::vector<RootConstantRange> rootconstants;
	};

}
