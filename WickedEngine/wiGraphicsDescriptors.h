#ifndef _GRAPHICS_DESCRIPTORS_H_
#define _GRAPHICS_DESCRIPTORS_H_

#include "CommonInclude.h"

namespace wiGraphicsTypes
{
	struct VertexShader;
	struct PixelShader;
	struct HullShader;
	struct DomainShader;
	struct GeometryShader;
	struct ComputeShader;
	struct BlendState;
	struct RasterizerState;
	struct DepthStencilState;
	struct VertexLayout;
	struct Texture;

	enum SHADERSTAGE
	{
		VS,
		HS,
		DS,
		GS,
		PS,
		CS,
		SHADERSTAGE_COUNT
	};
	enum PRIMITIVETOPOLOGY
	{
		UNDEFINED_TOPOLOGY,
		TRIANGLELIST,
		TRIANGLESTRIP,
		POINTLIST,
		LINELIST,
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
	enum FORMAT
	{
		FORMAT_UNKNOWN,
		FORMAT_R32G32B32A32_TYPELESS,
		FORMAT_R32G32B32A32_FLOAT,
		FORMAT_R32G32B32A32_UINT,
		FORMAT_R32G32B32A32_SINT,
		FORMAT_R32G32B32_TYPELESS,
		FORMAT_R32G32B32_FLOAT,
		FORMAT_R32G32B32_UINT,
		FORMAT_R32G32B32_SINT,
		FORMAT_R16G16B16A16_TYPELESS,
		FORMAT_R16G16B16A16_FLOAT,
		FORMAT_R16G16B16A16_UNORM,
		FORMAT_R16G16B16A16_UINT,
		FORMAT_R16G16B16A16_SNORM,
		FORMAT_R16G16B16A16_SINT,
		FORMAT_R32G32_TYPELESS,
		FORMAT_R32G32_FLOAT,
		FORMAT_R32G32_UINT,
		FORMAT_R32G32_SINT,
		FORMAT_R32G8X24_TYPELESS,
		FORMAT_D32_FLOAT_S8X24_UINT,
		FORMAT_R32_FLOAT_X8X24_TYPELESS,
		FORMAT_X32_TYPELESS_G8X24_UINT,
		FORMAT_R10G10B10A2_TYPELESS,
		FORMAT_R10G10B10A2_UNORM,
		FORMAT_R10G10B10A2_UINT,
		FORMAT_R11G11B10_FLOAT,
		FORMAT_R8G8B8A8_TYPELESS,
		FORMAT_R8G8B8A8_UNORM,
		FORMAT_R8G8B8A8_UNORM_SRGB,
		FORMAT_R8G8B8A8_UINT,
		FORMAT_R8G8B8A8_SNORM,
		FORMAT_R8G8B8A8_SINT,
		FORMAT_R16G16_TYPELESS,
		FORMAT_R16G16_FLOAT,
		FORMAT_R16G16_UNORM,
		FORMAT_R16G16_UINT,
		FORMAT_R16G16_SNORM,
		FORMAT_R16G16_SINT,
		FORMAT_R32_TYPELESS,
		FORMAT_D32_FLOAT,
		FORMAT_R32_FLOAT,
		FORMAT_R32_UINT,
		FORMAT_R32_SINT,
		FORMAT_R24G8_TYPELESS,
		FORMAT_D24_UNORM_S8_UINT,
		FORMAT_R24_UNORM_X8_TYPELESS,
		FORMAT_X24_TYPELESS_G8_UINT,
		FORMAT_R8G8_TYPELESS,
		FORMAT_R8G8_UNORM,
		FORMAT_R8G8_UINT,
		FORMAT_R8G8_SNORM,
		FORMAT_R8G8_SINT,
		FORMAT_R16_TYPELESS,
		FORMAT_R16_FLOAT,
		FORMAT_D16_UNORM,
		FORMAT_R16_UNORM,
		FORMAT_R16_UINT,
		FORMAT_R16_SNORM,
		FORMAT_R16_SINT,
		FORMAT_R8_TYPELESS,
		FORMAT_R8_UNORM,
		FORMAT_R8_UINT,
		FORMAT_R8_SNORM,
		FORMAT_R8_SINT,
		FORMAT_A8_UNORM,
		FORMAT_R1_UNORM,
		FORMAT_R9G9B9E5_SHAREDEXP,
		FORMAT_R8G8_B8G8_UNORM,
		FORMAT_G8R8_G8B8_UNORM,
		FORMAT_BC1_TYPELESS,
		FORMAT_BC1_UNORM,
		FORMAT_BC1_UNORM_SRGB,
		FORMAT_BC2_TYPELESS,
		FORMAT_BC2_UNORM,
		FORMAT_BC2_UNORM_SRGB,
		FORMAT_BC3_TYPELESS,
		FORMAT_BC3_UNORM,
		FORMAT_BC3_UNORM_SRGB,
		FORMAT_BC4_TYPELESS,
		FORMAT_BC4_UNORM,
		FORMAT_BC4_SNORM,
		FORMAT_BC5_TYPELESS,
		FORMAT_BC5_UNORM,
		FORMAT_BC5_SNORM,
		FORMAT_B5G6R5_UNORM,
		FORMAT_B5G5R5A1_UNORM,
		FORMAT_B8G8R8A8_UNORM,
		FORMAT_B8G8R8X8_UNORM,
		FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
		FORMAT_B8G8R8A8_TYPELESS,
		FORMAT_B8G8R8A8_UNORM_SRGB,
		FORMAT_B8G8R8X8_TYPELESS,
		FORMAT_B8G8R8X8_UNORM_SRGB,
		FORMAT_BC6H_TYPELESS,
		FORMAT_BC6H_UF16,
		FORMAT_BC6H_SF16,
		FORMAT_BC7_TYPELESS,
		FORMAT_BC7_UNORM,
		FORMAT_BC7_UNORM_SRGB,
		FORMAT_AYUV,
		FORMAT_Y410,
		FORMAT_Y416,
		FORMAT_NV12,
		FORMAT_P010,
		FORMAT_P016,
		FORMAT_420_OPAQUE,
		FORMAT_YUY2,
		FORMAT_Y210,
		FORMAT_Y216,
		FORMAT_NV11,
		FORMAT_AI44,
		FORMAT_IA44,
		FORMAT_P8,
		FORMAT_A8P8,
		FORMAT_B4G4R4A4_UNORM,
		FORMAT_FORCE_UINT = 0xffffffff,
	};
	//enum MAP
	//{
	//	MAP_READ,
	//	MAP_WRITE,
	//	MAP_READ_WRITE,
	//	MAP_WRITE_DISCARD,
	//	MAP_WRITE_NO_OVERWRITE
	//};
	enum GPU_QUERY_TYPE
	{
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

	// Flags ////////////////////////////////////////////
	enum CLEAR_FLAG
	{
		CLEAR_DEPTH = 0x1L,
		CLEAR_STENCIL = 0x2L,
	};
	enum BIND_FLAG
	{
		BIND_VERTEX_BUFFER = 0x1L,
		BIND_INDEX_BUFFER = 0x2L,
		BIND_CONSTANT_BUFFER = 0x4L,
		BIND_SHADER_RESOURCE = 0x8L,
		BIND_STREAM_OUTPUT = 0x10L,
		BIND_RENDER_TARGET = 0x20L,
		BIND_DEPTH_STENCIL = 0x40L,
		BIND_UNORDERED_ACCESS = 0x80L,
	};
	enum CPU_ACCESS
	{
		CPU_ACCESS_WRITE = 0x10000L,
		CPU_ACCESS_READ = 0x20000L,
	};
	enum RESOURCE_MISC_FLAG
	{
		//RESOURCE_MISC_GENERATE_MIPS = 0x1L,
		RESOURCE_MISC_SHARED = 0x2L,
		RESOURCE_MISC_TEXTURECUBE = 0x4L,
		RESOURCE_MISC_DRAWINDIRECT_ARGS = 0x10L,
		RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS = 0x20L,
		RESOURCE_MISC_BUFFER_STRUCTURED = 0x40L,
		RESOURCE_MISC_TILED = 0x40000L,
	};
	enum RESOURCE_STATES
	{
		RESOURCE_STATE_COMMON = 0,
		RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER = 0x1,
		RESOURCE_STATE_INDEX_BUFFER = 0x2,
		RESOURCE_STATE_RENDER_TARGET = 0x4,
		RESOURCE_STATE_UNORDERED_ACCESS = 0x8,
		RESOURCE_STATE_DEPTH_WRITE = 0x10,
		RESOURCE_STATE_DEPTH_READ = 0x20,
		RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE = 0x40,
		RESOURCE_STATE_PIXEL_SHADER_RESOURCE = 0x80,
		RESOURCE_STATE_STREAM_OUT = 0x100,
		RESOURCE_STATE_INDIRECT_ARGUMENT = 0x200,
		RESOURCE_STATE_COPY_DEST = 0x400,
		RESOURCE_STATE_COPY_SOURCE = 0x800,
		RESOURCE_STATE_RESOLVE_DEST = 0x1000,
		RESOURCE_STATE_RESOLVE_SOURCE = 0x2000,
		RESOURCE_STATE_GENERIC_READ = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
		RESOURCE_STATE_PRESENT = 0,
		RESOURCE_STATE_PREDICATION = 0x200,
		RESOURCE_STATE_VIDEO_DECODE_READ = 0x10000,
		RESOURCE_STATE_VIDEO_DECODE_WRITE = 0x20000,
		RESOURCE_STATE_VIDEO_PROCESS_READ = 0x40000,
		RESOURCE_STATE_VIDEO_PROCESS_WRITE = 0x80000
	};

#define	APPEND_ALIGNED_ELEMENT			( 0xffffffff )
#define FLOAT32_MAX						( 3.402823466e+38f )
#define DEFAULT_STENCIL_READ_MASK		( 0xff )
#define SO_NO_RASTERIZED_STREAM			( 0xffffffff )

	// Structs /////////////////////////////////////////////

	struct ViewPort
	{
		float TopLeftX;
		float TopLeftY;
		float Width;
		float Height;
		float MinDepth;
		float MaxDepth;

		ViewPort():
			TopLeftX(0.0f),
			TopLeftY(0.0f),
			Width(0.0f),
			Height(0.0f),
			MinDepth(0.0f),
			MaxDepth(1.0f)
		{}
	};
	struct VertexLayoutDesc
	{
		char* SemanticName;
		UINT SemanticIndex;
		FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
	};
	struct SampleDesc
	{
		UINT Count;
		UINT Quality;

		SampleDesc() :Count( 1 ), Quality( 0 ) {}
	};
	struct TextureDesc
	{
		UINT Width;
		UINT Height;
		UINT Depth;
		UINT ArraySize;
		UINT MipLevels;
		FORMAT Format;
		SampleDesc SampleDesc;
		USAGE Usage;
		UINT BindFlags;
		UINT CPUAccessFlags;
		UINT MiscFlags;

		TextureDesc() :
			Width(0),
			Height(0),
			Depth(0),
			ArraySize(1),
			MipLevels(1),
			Format(FORMAT_UNKNOWN),
			Usage(USAGE_DEFAULT),
			BindFlags(0),
			CPUAccessFlags(0),
			MiscFlags(0)
		{}
	};
	struct SamplerDesc
	{
		FILTER Filter;
		TEXTURE_ADDRESS_MODE AddressU;
		TEXTURE_ADDRESS_MODE AddressV;
		TEXTURE_ADDRESS_MODE AddressW;
		float MipLODBias;
		UINT MaxAnisotropy;
		COMPARISON_FUNC ComparisonFunc;
		float BorderColor[4];
		float MinLOD;
		float MaxLOD;

		SamplerDesc():
			Filter(FILTER_MIN_MAG_MIP_POINT),
			AddressU(TEXTURE_ADDRESS_CLAMP),
			AddressV(TEXTURE_ADDRESS_CLAMP),
			AddressW(TEXTURE_ADDRESS_CLAMP),
			MipLODBias(0.0f),
			MaxAnisotropy(0),
			ComparisonFunc(COMPARISON_NEVER),
			BorderColor{0.0f,0.0f,0.0f,0.0f},
			MinLOD(0.0f),
			MaxLOD(FLT_MAX)
		{}
	};
	struct RasterizerStateDesc
	{
		FILL_MODE FillMode;
		CULL_MODE CullMode;
		bool FrontCounterClockwise;
		INT DepthBias;
		float DepthBiasClamp;
		float SlopeScaledDepthBias;
		bool DepthClipEnable;
		bool MultisampleEnable;
		bool AntialiasedLineEnable;
		bool ConservativeRasterizationEnable;
		UINT ForcedSampleCount;

		RasterizerStateDesc() :
			FillMode(FILL_SOLID),
			CullMode(CULL_NONE),
			FrontCounterClockwise(false),
			DepthBias(0),
			DepthBiasClamp(0.0f),
			SlopeScaledDepthBias(0.0f),
			DepthClipEnable(false),
			MultisampleEnable(false),
			AntialiasedLineEnable(false),
			ConservativeRasterizationEnable(false),
			ForcedSampleCount(0)
		{}
	};
	struct DepthStencilOpDesc
	{
		STENCIL_OP StencilFailOp;
		STENCIL_OP StencilDepthFailOp;
		STENCIL_OP StencilPassOp;
		COMPARISON_FUNC StencilFunc;

		DepthStencilOpDesc():
			StencilFailOp(STENCIL_OP_KEEP),
			StencilDepthFailOp(STENCIL_OP_KEEP),
			StencilPassOp(STENCIL_OP_KEEP),
			StencilFunc(COMPARISON_NEVER)
		{}
	};
	struct DepthStencilStateDesc
	{
		bool DepthEnable;
		DEPTH_WRITE_MASK DepthWriteMask;
		COMPARISON_FUNC DepthFunc;
		bool StencilEnable;
		UINT8 StencilReadMask;
		UINT8 StencilWriteMask;
		DepthStencilOpDesc FrontFace;
		DepthStencilOpDesc BackFace;

		DepthStencilStateDesc():
			DepthEnable(false),
			DepthWriteMask(DEPTH_WRITE_MASK_ZERO),
			DepthFunc(COMPARISON_NEVER),
			StencilEnable(false),
			StencilReadMask(0xff),
			StencilWriteMask(0xff)
		{}
	};
	struct RenderTargetBlendStateDesc
	{
		bool BlendEnable;
		BLEND SrcBlend;
		BLEND DestBlend;
		BLEND_OP BlendOp;
		BLEND SrcBlendAlpha;
		BLEND DestBlendAlpha;
		BLEND_OP BlendOpAlpha;
		UINT8 RenderTargetWriteMask;

		RenderTargetBlendStateDesc():
			BlendEnable(false),
			SrcBlend(BLEND_SRC_ALPHA),
			DestBlend(BLEND_INV_SRC_ALPHA),
			BlendOp(BLEND_OP_ADD),
			SrcBlendAlpha(BLEND_ONE),
			DestBlendAlpha(BLEND_ONE),
			BlendOpAlpha(BLEND_OP_ADD),
			RenderTargetWriteMask(COLOR_WRITE_ENABLE_ALL)
		{}
	};
	struct BlendStateDesc
	{
		bool AlphaToCoverageEnable;
		bool IndependentBlendEnable;
		RenderTargetBlendStateDesc RenderTarget[8];

		BlendStateDesc():
			AlphaToCoverageEnable(false),
			IndependentBlendEnable(false)
		{}
	};
	struct GPUBufferDesc
	{
		UINT ByteWidth;
		USAGE Usage;
		UINT BindFlags;
		UINT CPUAccessFlags;
		UINT MiscFlags;
		UINT StructureByteStride; // needed for typed and structured buffer types!
		FORMAT Format; // only needed for typed buffer!

		GPUBufferDesc():
			ByteWidth(0),
			Usage(USAGE_DEFAULT),
			BindFlags(0),
			CPUAccessFlags(0),
			MiscFlags(0),
			StructureByteStride(0),
			Format(FORMAT_UNKNOWN)
		{}
	};
	struct GPUQueryDesc
	{
		GPU_QUERY_TYPE Type;
		UINT MiscFlags;
		// 0 for immediate access!
		UINT async_latency;

		GPUQueryDesc():
			Type(GPU_QUERY_TYPE_OCCLUSION_PREDICATE),
			MiscFlags(0),
			async_latency(0)
		{}
	};
	struct GraphicsPSODesc
	{
		VertexShader*			vs = nullptr;
		PixelShader*			ps = nullptr;
		HullShader*				hs = nullptr;
		DomainShader*			ds = nullptr;
		GeometryShader*			gs = nullptr;
		BlendState*				bs = nullptr;
		RasterizerState*		rs = nullptr;
		DepthStencilState*		dss = nullptr;
		VertexLayout*			il = nullptr;
		PRIMITIVETOPOLOGY		pt = TRIANGLELIST;
		UINT					numRTs = 0;
		FORMAT					RTFormats[8] = {};
		FORMAT					DSFormat = FORMAT_UNKNOWN;
		SampleDesc				sampleDesc; 
		UINT					sampleMask = 0xFFFFFFFF;
	};
	struct ComputePSODesc
	{
		ComputeShader*			cs = nullptr;
	};
	struct IndirectDrawArgsInstanced
	{
		UINT VertexCountPerInstance;
		UINT InstanceCount;
		UINT StartVertexLocation;
		UINT StartInstanceLocation;

		IndirectDrawArgsInstanced():
			VertexCountPerInstance(0),
			InstanceCount(0),
			StartVertexLocation(0),
			StartInstanceLocation(0)
		{}
	};
	struct IndirectDrawArgsIndexedInstanced
	{
		UINT IndexCountPerInstance;
		UINT InstanceCount;
		UINT StartIndexLocation;
		INT BaseVertexLocation;
		UINT StartInstanceLocation;

		IndirectDrawArgsIndexedInstanced() :
			IndexCountPerInstance(0),
			InstanceCount(0),
			StartIndexLocation(0),
			BaseVertexLocation(0),
			StartInstanceLocation(0)
		{}
	};
	struct IndirectDispatchArgs
	{
		UINT ThreadGroupCountX;
		UINT ThreadGroupCountY;
		UINT ThreadGroupCountZ;

		IndirectDispatchArgs():
			ThreadGroupCountX(0),
			ThreadGroupCountY(0),
			ThreadGroupCountZ(0)
		{}
	};
	struct SubresourceData
	{
		const void *pSysMem;
		UINT SysMemPitch;
		UINT SysMemSlicePitch;

		SubresourceData():
			pSysMem(nullptr),
			SysMemPitch(0),
			SysMemSlicePitch(0)
		{}
	};
	struct MappedSubresource
	{
		void *pData;
		UINT RowPitch;
		UINT DepthPitch;

		MappedSubresource():
			pData(nullptr),
			RowPitch(0),
			DepthPitch(0)
		{}
	};
	struct Rect
	{
		LONG left;
		LONG top;
		LONG right;
		LONG bottom;

		Rect():
			left(0),
			top(0),
			right(0),
			bottom(0)
		{}
	};

}

#endif // _GRAPHICS_DESCRIPTORS_H_
