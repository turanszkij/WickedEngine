#pragma once
#include "CommonInclude.h"
#include "wiThreadSafeManager.h"
#include "wiEnums.h"

#include <d3d11_2.h>
#include <DXGI1_2.h>

#include "ConstantBufferMapping.h"


namespace wiGraphicsTypes
{
	enum PRIMITIVETOPOLOGY
	{
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
		RESOURCE_MISC_GENERATE_MIPS = 0x1L,
		RESOURCE_MISC_SHARED = 0x2L,
		RESOURCE_MISC_TEXTURECUBE = 0x4L,
		RESOURCE_MISC_BUFFER_STRUCTURED = 0x40L,
		RESOURCE_MISC_TILED = 0x40000L,
	};

#define	APPEND_ALIGNED_ELEMENT			( 0xffffffff )
#define FLOAT32_MAX						( 3.402823466e+38f )
#define DEFAULT_STENCIL_READ_MASK		( 0xff )
#define SO_NO_RASTERIZED_STREAM			( 0xffffffff )

	// Structs /////////////////////////////////////////////

	struct ViewPort
	{
		FLOAT TopLeftX;
		FLOAT TopLeftY;
		FLOAT Width;
		FLOAT Height;
		FLOAT MinDepth;
		FLOAT MaxDepth;
	};
	struct VertexLayoutDesc
	{
		LPCSTR SemanticName;
		UINT SemanticIndex;
		FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
	};
	struct StreamOutDeclaration
	{
		UINT Stream;
		LPCSTR SemanticName;
		UINT SemanticIndex;
		BYTE StartComponent;
		BYTE ComponentCount;
		BYTE OutputSlot;
	};
	struct SampleDesc
	{
		UINT Count;
		UINT Quality;
	};
	struct Texture2DDesc
	{
		UINT Width;
		UINT Height;
		UINT MipLevels;
		UINT ArraySize;
		FORMAT Format;
		SampleDesc SampleDesc;
		USAGE Usage;
		UINT BindFlags;
		UINT CPUAccessFlags;
		UINT MiscFlags;
	};
	struct SamplerDesc
	{
		FILTER Filter;
		TEXTURE_ADDRESS_MODE AddressU;
		TEXTURE_ADDRESS_MODE AddressV;
		TEXTURE_ADDRESS_MODE AddressW;
		FLOAT MipLODBias;
		UINT MaxAnisotropy;
		COMPARISON_FUNC ComparisonFunc;
		FLOAT BorderColor[4];
		FLOAT MinLOD;
		FLOAT MaxLOD;
	};
	struct RasterizerStateDesc
	{
		FILL_MODE FillMode;
		CULL_MODE CullMode;
		BOOL FrontCounterClockwise;
		INT DepthBias;
		FLOAT DepthBiasClamp;
		FLOAT SlopeScaledDepthBias;
		BOOL DepthClipEnable;
		BOOL ScissorEnable;
		BOOL MultisampleEnable;
		BOOL AntialiasedLineEnable;
	};
	struct DepthStencilOpDesc
	{
		STENCIL_OP StencilFailOp;
		STENCIL_OP StencilDepthFailOp;
		STENCIL_OP StencilPassOp;
		COMPARISON_FUNC StencilFunc;
	};
	struct DepthStencilStateDesc
	{
		BOOL DepthEnable;
		DEPTH_WRITE_MASK DepthWriteMask;
		COMPARISON_FUNC DepthFunc;
		BOOL StencilEnable;
		UINT8 StencilReadMask;
		UINT8 StencilWriteMask;
		DepthStencilOpDesc FrontFace;
		DepthStencilOpDesc BackFace;
	};
	struct RenderTargetBlendStateDesc
	{
		BOOL BlendEnable;
		BLEND SrcBlend;
		BLEND DestBlend;
		BLEND_OP BlendOp;
		BLEND SrcBlendAlpha;
		BLEND DestBlendAlpha;
		BLEND_OP BlendOpAlpha;
		UINT8 RenderTargetWriteMask;
	};
	struct BlendStateDesc
	{
		BOOL AlphaToCoverageEnable;
		BOOL IndependentBlendEnable;
		RenderTargetBlendStateDesc RenderTarget[8];
	};
	struct GPUBufferDesc
	{
		UINT ByteWidth;
		USAGE Usage;
		UINT BindFlags;
		UINT CPUAccessFlags;
		UINT MiscFlags;
		UINT StructureByteStride;
	};
	struct SubresourceData
	{
		const void *pSysMem;
		UINT SysMemPitch;
		UINT SysMemSlicePitch;
	};




	class GraphicsDevice;
	class GraphicsDevice_DX11;

	class VertexShader
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11VertexShader*		resource_DX11;
	public:
		VertexShader() :resource_DX11(nullptr) {}
		~VertexShader()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
	};

	class PixelShader
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11PixelShader*		resource_DX11;
	public:
		PixelShader() :resource_DX11(nullptr) {}
		~PixelShader()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
	};

	class GeometryShader
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11GeometryShader*	resource_DX11;
	public:
		GeometryShader() :resource_DX11(nullptr) {}
		~GeometryShader()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
	};

	class HullShader
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11HullShader*		resource_DX11;
	public:
		HullShader() :resource_DX11(nullptr) {}
		~HullShader()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
	};

	class DomainShader
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11DomainShader*		resource_DX11;
	public:
		DomainShader() :resource_DX11(nullptr) {}
		~DomainShader()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
	};

	class ComputeShader
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11ComputeShader*	resource_DX11;
	public:
		ComputeShader() :resource_DX11(nullptr) {}
		~ComputeShader()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
	};

	class Sampler
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11SamplerState*	resource_DX11;
		SamplerDesc desc;
	public:
		Sampler() :resource_DX11(nullptr) {}
		~Sampler()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
		SamplerDesc GetDesc() { return desc; }
	};

	class GPUBuffer
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11Buffer* resource_DX11[2]; // double buffer
		GPUBufferDesc desc;
		int activeBuffer;
	public:
		GPUBuffer() 
		{
			activeBuffer = 0;
			SAFE_INIT(resource_DX11[0]);
			SAFE_INIT(resource_DX11[1]);
		}
		~GPUBuffer()
		{
			SAFE_RELEASE(resource_DX11[0]);
			SAFE_RELEASE(resource_DX11[1]);
		}

		bool IsValid() { return resource_DX11[activeBuffer] != nullptr; }
		GPUBufferDesc GetDesc() { return desc; }
	};

	class VertexLayout
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11InputLayout*	resource_DX11;
	public:
		VertexLayout() :resource_DX11(nullptr) {}
		~VertexLayout()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
	};

	class BlendState
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11BlendState*	resource_DX11;
		BlendStateDesc desc;
	public:
		BlendState() :resource_DX11(nullptr) {}
		~BlendState()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
		BlendStateDesc GetDesc() { return desc; }
	};

	class DepthStencilState
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11DepthStencilState*	resource_DX11;
		DepthStencilStateDesc desc;
	public:
		DepthStencilState() :resource_DX11(nullptr) {}
		~DepthStencilState()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
		DepthStencilStateDesc GetDesc() { return desc; }
	};

	class RasterizerState
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11RasterizerState*	resource_DX11;
		RasterizerStateDesc desc;
	public:
		RasterizerState() :resource_DX11(nullptr) {}
		~RasterizerState()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
		RasterizerStateDesc GetDesc() { return desc; }
	};

	class ClassLinkage
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11ClassLinkage*	resource_DX11;
	public:
		ClassLinkage() :resource_DX11(nullptr) {}
		~ClassLinkage()
		{
			SAFE_RELEASE(resource_DX11);
		}

		bool IsValid() { return resource_DX11 != nullptr; }
	};

	struct VertexShaderInfo {
		VertexShader* vertexShader;
		VertexLayout* vertexLayout;

		VertexShaderInfo() {
			vertexShader = nullptr;
			vertexLayout = nullptr;
		}
		~VertexShaderInfo()
		{
			SAFE_DELETE(vertexShader);
			SAFE_DELETE(vertexLayout);
		}
	};

	class Texture
	{
		friend class GraphicsDevice_DX11;
	private: 
		ID3D11ShaderResourceView*	shaderResourceView_DX11;
		ID3D11RenderTargetView*		renderTargetView_DX11;
		ID3D11DepthStencilView*		depthStencilView_DX11;
	public:

		Texture() :shaderResourceView_DX11(nullptr), renderTargetView_DX11(nullptr), depthStencilView_DX11(nullptr) {}
		virtual ~Texture()
		{
			SAFE_RELEASE(shaderResourceView_DX11);
			SAFE_RELEASE(renderTargetView_DX11);
			SAFE_RELEASE(depthStencilView_DX11);
		}
	};
	class Texture2D : public Texture
	{
		friend class GraphicsDevice_DX11;
	private:
		ID3D11Texture2D*			texture2D_DX11;
		Texture2DDesc				desc;
	public:
		Texture2D() :Texture(), texture2D_DX11(nullptr) {}
		virtual ~Texture2D()
		{
			SAFE_RELEASE(texture2D_DX11);
		}

		Texture2DDesc GetDesc() { return desc; }
	};
	class TextureCube : public Texture2D
	{
		friend class GraphicsDevice_DX11;
	public:
		TextureCube() :Texture2D() {}
		virtual ~TextureCube() {}
	};

	class GraphicsDevice : public wiThreadSafeManager
	{
	protected:
		long FRAMECOUNT;
		bool VSYNC;
		int SCREENWIDTH, SCREENHEIGHT;
	public:
		GraphicsDevice() :FRAMECOUNT(0), VSYNC(true), SCREENWIDTH(0), SCREENHEIGHT(0) {}

		virtual HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer) = 0;
		virtual HRESULT CreateTexture1D() = 0;
		virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D) = 0;
		virtual HRESULT CreateTexture3D() = 0;
		virtual HRESULT CreateTextureCube(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, TextureCube **ppTextureCube) = 0;
		virtual HRESULT CreateShaderResourceView(Texture2D* pTexture) = 0;
		virtual HRESULT CreateRenderTargetView(Texture2D* pTexture) = 0;
		virtual HRESULT CreateDepthStencilView(Texture2D* pTexture) = 0;
		virtual HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
			const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout) = 0;
		virtual HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, VertexShader *pVertexShader) = 0;
		virtual HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, PixelShader *pPixelShader) = 0;
		virtual HRESULT CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, GeometryShader *pGeometryShader) = 0;
		virtual HRESULT CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode, SIZE_T BytecodeLength, const StreamOutDeclaration *pSODeclaration,
			UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ClassLinkage* pClassLinkage, GeometryShader *pGeometryShader) = 0;
		virtual HRESULT CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, HullShader *pHullShader) = 0;
		virtual HRESULT CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, DomainShader *pDomainShader) = 0;
		virtual HRESULT CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, ComputeShader *pComputeShader) = 0;
		virtual HRESULT CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState) = 0;
		virtual HRESULT CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState) = 0;
		virtual HRESULT CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState) = 0;
		virtual HRESULT CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) = 0;

		virtual void PresentBegin() = 0;
		virtual void PresentEnd() = 0;

		virtual void ExecuteDeferredContexts() = 0;
		virtual void FinishCommandList(GRAPHICSTHREAD thread) = 0;

		virtual bool GetMultithreadingSupport() = 0;

		bool GetVSyncEnabled() { return VSYNC; }
		void SetVSyncEnabled(bool value) { VSYNC = value; }
		long GetFrameCount() { return FRAMECOUNT; }

		int GetScreenWidth() { return SCREENWIDTH; }
		int GetScreenHeight() { return SCREENHEIGHT; }

		virtual void SetScreenWidth(int value) = 0;
		virtual void SetScreenHeight(int value) = 0;

		///////////////Thread-sensitive////////////////////////

		virtual void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindRenderTargets(UINT NumViews, Texture2D* const *ppRenderTargetViews, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void ClearRenderTarget(Texture2D* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindTexturePS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindTextureVS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindTextureGS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindTextureDS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindTextureHS(const Texture* texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void UnbindTextures(int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerPS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerVS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerGS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerHS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerDS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferPS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferVS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferGS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferDS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferHS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindVertexBuffer(const GPUBuffer* vertexBuffer, int slot, UINT stride, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindIndexBuffer(const GPUBuffer* indexBuffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindVertexLayout(const VertexLayout* layout, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindBlendState(const BlendState* state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindBlendStateEx(const BlendState* state, const XMFLOAT4& blendFactor = XMFLOAT4(1, 1, 1, 1), UINT sampleMask = 0xffffffff,
			GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindDepthStencilState(const DepthStencilState* state, UINT stencilRef, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindRasterizerState(const RasterizerState* state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindStreamOutTarget(const GPUBuffer* buffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindPS(const PixelShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindVS(const VertexShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindGS(const GeometryShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindHS(const HullShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindDS(const DomainShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void Draw(int vertexCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void DrawIndexed(int indexCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void DrawIndexedInstanced(int indexCount, int instanceCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void GenerateMips(Texture* texture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void CopyTexture2D(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, int dataSize = -1) = 0;

		virtual HRESULT CreateTextureFromFile(const wstring& fileName, Texture2D **ppTexture, bool mipMaps = true, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;

	};

}

