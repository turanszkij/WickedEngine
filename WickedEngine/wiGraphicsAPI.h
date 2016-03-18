#pragma once
#include "CommonInclude.h"
#include "wiThreadSafeManager.h"
#include "wiEnums.h"

#include <d3d11_2.h>
#include <DXGI1_2.h>

#include "ConstantBufferMapping.h"


#ifndef WINSTORE_SUPPORT
typedef IDXGISwapChain*				SwapChain;
#else
typedef IDXGISwapChain1*			SwapChain;
#endif
typedef IUnknown*					APIInterface;
typedef ID3D11DeviceContext*		DeviceContext;
//typedef	ID3D11Device*				GraphicsDevice;
typedef ID3D11CommandList*			CommandList;
typedef ID3D11RenderTargetView*		RenderTargetView;
typedef ID3D11ShaderResourceView*	TextureView;
typedef ID3D11Texture2D*			Texture2D;
typedef ID3D11SamplerState*			Sampler;
typedef ID3D11Resource*				APIResource;
typedef ID3D11Buffer*				BufferResource;
typedef ID3D11VertexShader*			VertexShader;
typedef ID3D11PixelShader*			PixelShader;
typedef ID3D11GeometryShader*		GeometryShader;
typedef ID3D11HullShader*			HullShader;
typedef ID3D11DomainShader*			DomainShader;
typedef ID3D11ComputeShader*		ComputeShader;
typedef ID3D11InputLayout*			VertexLayout;
typedef ID3D11BlendState*			BlendState;
typedef ID3D11DepthStencilState*	DepthStencilState;
typedef ID3D11DepthStencilView*		DepthStencilView;
typedef ID3D11RasterizerState*		RasterizerState;
typedef ID3D11ClassLinkage*			ClassLinkage;



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
	COMPARISON_NEVER				= D3D11_COMPARISON_NEVER,
	COMPARISON_LESS					= D3D11_COMPARISON_LESS,
	COMPARISON_EQUAL				= D3D11_COMPARISON_EQUAL,
	COMPARISON_LESS_EQUAL			= D3D11_COMPARISON_LESS_EQUAL,
	COMPARISON_GREATER				= D3D11_COMPARISON_GREATER,
	COMPARISON_NOT_EQUAL			= D3D11_COMPARISON_NOT_EQUAL,
	COMPARISON_GREATER_EQUAL		= D3D11_COMPARISON_GREATER_EQUAL,
	COMPARISON_ALWAYS				= D3D11_COMPARISON_ALWAYS,
};
enum DEPTH_WRITE_MASK
{
	DEPTH_WRITE_MASK_ZERO			= D3D11_DEPTH_WRITE_MASK_ZERO,
	DEPTH_WRITE_MASK_ALL			= D3D11_DEPTH_WRITE_MASK_ALL,
};
enum STENCIL_OP
{
	STENCIL_OP_KEEP					= D3D11_STENCIL_OP_KEEP,
	STENCIL_OP_ZERO					= D3D11_STENCIL_OP_ZERO,
	STENCIL_OP_REPLACE				= D3D11_STENCIL_OP_REPLACE,
	STENCIL_OP_INCR_SAT				= D3D11_STENCIL_OP_INCR_SAT,
	STENCIL_OP_DECR_SAT				= D3D11_STENCIL_OP_DECR_SAT,
	STENCIL_OP_INVERT				= D3D11_STENCIL_OP_INVERT,
	STENCIL_OP_INCR					= D3D11_STENCIL_OP_INCR,
	STENCIL_OP_DECR					= D3D11_STENCIL_OP_DECR,
};
enum BLEND
{
	BLEND_ZERO						= D3D11_BLEND_ZERO,
	BLEND_ONE						= D3D11_BLEND_ONE,
	BLEND_SRC_COLOR					= D3D11_BLEND_SRC_COLOR,
	BLEND_INV_SRC_COLOR				= D3D11_BLEND_INV_SRC_COLOR,
	BLEND_SRC_ALPHA					= D3D11_BLEND_SRC_ALPHA,
	BLEND_INV_SRC_ALPHA				= D3D11_BLEND_INV_SRC_ALPHA,
	BLEND_DEST_ALPHA				= D3D11_BLEND_DEST_ALPHA,
	BLEND_INV_DEST_ALPHA			= D3D11_BLEND_INV_DEST_ALPHA,
	BLEND_DEST_COLOR				= D3D11_BLEND_DEST_COLOR,
	BLEND_INV_DEST_COLOR			= D3D11_BLEND_INV_DEST_COLOR,
	BLEND_SRC_ALPHA_SAT				= D3D11_BLEND_SRC_ALPHA_SAT,
	BLEND_BLEND_FACTOR				= D3D11_BLEND_BLEND_FACTOR,
	BLEND_INV_BLEND_FACTOR			= D3D11_BLEND_INV_BLEND_FACTOR,
	BLEND_SRC1_COLOR				= D3D11_BLEND_SRC1_COLOR,
	BLEND_INV_SRC1_COLOR			= D3D11_BLEND_INV_SRC1_COLOR,
	BLEND_SRC1_ALPHA				= D3D11_BLEND_SRC1_ALPHA,
	BLEND_INV_SRC1_ALPHA			= D3D11_BLEND_INV_SRC1_ALPHA,
};
enum BLEND_OP
{
	BLEND_OP_ADD					= D3D11_BLEND_OP_ADD,
	BLEND_OP_SUBTRACT				= D3D11_BLEND_OP_SUBTRACT,
	BLEND_OP_REV_SUBTRACT			= D3D11_BLEND_OP_REV_SUBTRACT,
	BLEND_OP_MIN					= D3D11_BLEND_OP_MIN,
	BLEND_OP_MAX					= D3D11_BLEND_OP_MAX,
};
enum COLORWRITE
{
	COLOR_WRITE_ENABLE_RED			= D3D11_COLOR_WRITE_ENABLE_RED,
	COLOR_WRITE_ENABLE_GREEN		= D3D11_COLOR_WRITE_ENABLE_GREEN,
	COLOR_WRITE_ENABLE_BLUE			= D3D11_COLOR_WRITE_ENABLE_BLUE,
	COLOR_WRITE_ENABLE_ALPHA		= D3D11_COLOR_WRITE_ENABLE_ALPHA,
	COLOR_WRITE_ENABLE_ALL			= (((COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN) | COLOR_WRITE_ENABLE_BLUE) | COLOR_WRITE_ENABLE_ALPHA),
};
enum FILL_MODE
{
	FILL_WIREFRAME					= D3D11_FILL_WIREFRAME,
	FILL_SOLID						= D3D11_FILL_SOLID,
};
enum CULL_MODE
{
	CULL_NONE						= D3D11_CULL_NONE,
	CULL_FRONT						= D3D11_CULL_FRONT,
	CULL_BACK						= D3D11_CULL_BACK,
};
enum INPUT_CLASSIFICATION
{
	INPUT_PER_VERTEX_DATA			= D3D11_INPUT_PER_VERTEX_DATA,
	INPUT_PER_INSTANCE_DATA			= D3D11_INPUT_PER_INSTANCE_DATA,
};
enum BIND_FLAG
{
	BIND_VERTEX_BUFFER				= D3D11_BIND_VERTEX_BUFFER,
	BIND_INDEX_BUFFER				= D3D11_BIND_INDEX_BUFFER,
	BIND_CONSTANT_BUFFER			= D3D11_BIND_CONSTANT_BUFFER,
	BIND_SHADER_RESOURCE			= D3D11_BIND_SHADER_RESOURCE,
	BIND_STREAM_OUTPUT				= D3D11_BIND_STREAM_OUTPUT,
	BIND_RENDER_TARGET				= D3D11_BIND_RENDER_TARGET,
	BIND_DEPTH_STENCIL				= D3D11_BIND_DEPTH_STENCIL,
	BIND_UNORDERED_ACCESS			= D3D11_BIND_UNORDERED_ACCESS,
	BIND_DECODER					= D3D11_BIND_DECODER,
	BIND_VIDEO_ENCODER				= D3D11_BIND_VIDEO_ENCODER,
};
enum USAGE
{
	USAGE_DEFAULT					= D3D11_USAGE_DEFAULT,
	USAGE_IMMUTABLE					= D3D11_USAGE_IMMUTABLE,
	USAGE_DYNAMIC					= D3D11_USAGE_DYNAMIC,
	USAGE_STAGING					= D3D11_USAGE_STAGING,
};
enum CPU_ACCESS
{
	CPU_ACCESS_WRITE				= D3D11_CPU_ACCESS_WRITE,
	CPU_ACCESS_READ					= D3D11_CPU_ACCESS_READ,
};
enum CLEAR_FLAG
{
	CLEAR_DEPTH						= D3D11_CLEAR_DEPTH,
	CLEAR_STENCIL					= D3D11_CLEAR_STENCIL,
};
enum TEXTURE_ADDRESS_MODE
{
	TEXTURE_ADDRESS_WRAP			= D3D11_TEXTURE_ADDRESS_WRAP,
	TEXTURE_ADDRESS_MIRROR			= D3D11_TEXTURE_ADDRESS_MIRROR,
	TEXTURE_ADDRESS_CLAMP			= D3D11_TEXTURE_ADDRESS_CLAMP,
	TEXTURE_ADDRESS_BORDER			= D3D11_TEXTURE_ADDRESS_BORDER,
	TEXTURE_ADDRESS_MIRROR_ONCE		= D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
};
enum RESOURCE_DIMENSION
{
	RESOURCE_DIMENSION_UNKNOWN			= D3D_SRV_DIMENSION_UNKNOWN,
	RESOURCE_DIMENSION_BUFFER			= D3D_SRV_DIMENSION_BUFFER,
	RESOURCE_DIMENSION_TEXTURE1D		= D3D_SRV_DIMENSION_TEXTURE1D,
	RESOURCE_DIMENSION_TEXTURE1DARRAY	= D3D_SRV_DIMENSION_TEXTURE1DARRAY,
	RESOURCE_DIMENSION_TEXTURE2D		= D3D_SRV_DIMENSION_TEXTURE2D,
	RESOURCE_DIMENSION_TEXTURE2DARRAY	= D3D_SRV_DIMENSION_TEXTURE2DARRAY,
	RESOURCE_DIMENSION_TEXTURE2DMS		= D3D_SRV_DIMENSION_TEXTURE2DMS,
	RESOURCE_DIMENSION_TEXTURE2DMSARRAY = D3D_SRV_DIMENSION_TEXTURE2DMSARRAY,
	RESOURCE_DIMENSION_TEXTURE3D		= D3D_SRV_DIMENSION_TEXTURE3D,
	RESOURCE_DIMENSION_TEXTURECUBE		= D3D_SRV_DIMENSION_TEXTURECUBE,
	RESOURCE_DIMENSION_TEXTURECUBEARRAY = D3D_SRV_DIMENSION_TEXTURECUBEARRAY,
	RESOURCE_DIMENSION_BUFFEREX			= D3D_SRV_DIMENSION_BUFFEREX
};
enum FILTER
{
	FILTER_MIN_MAG_MIP_POINT							= D3D11_FILTER_MIN_MAG_MIP_POINT,
	FILTER_MIN_MAG_POINT_MIP_LINEAR						= D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
	FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT				= D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
	FILTER_MIN_POINT_MAG_MIP_LINEAR						= D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
	FILTER_MIN_LINEAR_MAG_MIP_POINT						= D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
	FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR				= D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
	FILTER_MIN_MAG_LINEAR_MIP_POINT						= D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
	FILTER_MIN_MAG_MIP_LINEAR							= D3D11_FILTER_MIN_MAG_MIP_LINEAR,
	FILTER_ANISOTROPIC									= D3D11_FILTER_ANISOTROPIC,
	FILTER_COMPARISON_MIN_MAG_MIP_POINT					= D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
	FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR			= D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
	FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT	= D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
	FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR			= D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
	FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT			= D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
	FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
	FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT			= D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
	FILTER_COMPARISON_MIN_MAG_MIP_LINEAR				= D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
	FILTER_COMPARISON_ANISOTROPIC						= D3D11_FILTER_COMPARISON_ANISOTROPIC,
	FILTER_MINIMUM_MIN_MAG_MIP_POINT					= D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT,
	FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR				= D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
	FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT		= D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
	FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR				= D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
	FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT				= D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
	FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR		= D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
	FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT				= D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
	FILTER_MINIMUM_MIN_MAG_MIP_LINEAR					= D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR,
	FILTER_MINIMUM_ANISOTROPIC							= D3D11_FILTER_MINIMUM_ANISOTROPIC,
	FILTER_MAXIMUM_MIN_MAG_MIP_POINT					= D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT,
	FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR				= D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
	FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT		= D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
	FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR				= D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
	FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT				= D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
	FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR		= D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
	FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT				= D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
	FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR					= D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR,
	FILTER_MAXIMUM_ANISOTROPIC							= D3D11_FILTER_MAXIMUM_ANISOTROPIC,
};
enum RESOURCE_MISC_FLAG
{
	RESOURCE_MISC_GENERATE_MIPS							= D3D11_RESOURCE_MISC_GENERATE_MIPS,
	RESOURCE_MISC_SHARED								= D3D11_RESOURCE_MISC_SHARED,
	RESOURCE_MISC_TEXTURECUBE							= D3D11_RESOURCE_MISC_TEXTURECUBE,
	RESOURCE_MISC_DRAWINDIRECT_ARGS						= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS,
	RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS				= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS,
	RESOURCE_MISC_BUFFER_STRUCTURED						= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
	RESOURCE_MISC_RESOURCE_CLAMP						= D3D11_RESOURCE_MISC_RESOURCE_CLAMP,
	RESOURCE_MISC_SHARED_KEYEDMUTEX						= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX,
	RESOURCE_MISC_GDI_COMPATIBLE						= D3D11_RESOURCE_MISC_GDI_COMPATIBLE,
	RESOURCE_MISC_SHARED_NTHANDLE						= D3D11_RESOURCE_MISC_SHARED_NTHANDLE,
	RESOURCE_MISC_RESTRICTED_CONTENT					= D3D11_RESOURCE_MISC_RESTRICTED_CONTENT,
	RESOURCE_MISC_RESTRICT_SHARED_RESOURCE				= D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE,
	RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER		= D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER,
	RESOURCE_MISC_GUARDED								= D3D11_RESOURCE_MISC_GUARDED,
	RESOURCE_MISC_TILE_POOL								= D3D11_RESOURCE_MISC_TILE_POOL,
	RESOURCE_MISC_TILED									= D3D11_RESOURCE_MISC_TILED,
};

#define	APPEND_ALIGNED_ELEMENT			D3D11_APPEND_ALIGNED_ELEMENT
#define FLOAT32_MAX						D3D11_FLOAT32_MAX
#define DEFAULT_STENCIL_READ_MASK		D3D11_DEFAULT_STENCIL_READ_MASK
#define SO_NO_RASTERIZED_STREAM			D3D11_SO_NO_RASTERIZED_STREAM

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
	DXGI_FORMAT Format;
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
struct Texture2DDesc
{
	UINT Width;
	UINT Height;
	UINT MipLevels;
	UINT ArraySize;
	DXGI_FORMAT Format;
	DXGI_SAMPLE_DESC SampleDesc;
	USAGE Usage;
	UINT BindFlags;
	UINT CPUAccessFlags;
	UINT MiscFlags;
};
struct ShaderResourceViewDesc
{
	DXGI_FORMAT Format;
	RESOURCE_DIMENSION ViewDimension;
	UINT mipLevels;
};
struct RenderTargetViewDesc
{
	DXGI_FORMAT Format;
	RESOURCE_DIMENSION ViewDimension;
	UINT ArraySize;
};
struct DepthStencilViewDesc
{
	DXGI_FORMAT Format;
	RESOURCE_DIMENSION ViewDimension;
	UINT Flags;
	UINT ArraySize;
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
struct RasterizerDesc
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
struct DepthStencilDesc
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
struct RenderTargetBlendDesc
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
struct BlendDesc
{
	BOOL AlphaToCoverageEnable;
	BOOL IndependentBlendEnable;
	RenderTargetBlendDesc RenderTarget[8];
};
struct BufferDesc
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

struct VertexShaderInfo {
	VertexShader vertexShader;
	VertexLayout vertexLayout;

	VertexShaderInfo() {
		vertexShader = nullptr;
		vertexLayout = VertexLayout();
	}
};

class GraphicsDevice : public wiThreadSafeManager
{
public:
	virtual HRESULT CreateBuffer(const BufferDesc *pDesc, const SubresourceData* pInitialData, BufferResource *ppBuffer) = 0;
	virtual HRESULT CreateTexture1D() = 0;
	virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D *ppTexture2D) = 0;
	virtual HRESULT CreateTexture3D() = 0;
	virtual HRESULT CreateShaderResourceView(APIResource pResource, const ShaderResourceViewDesc* pDesc, TextureView *ppSRView) = 0;
	virtual HRESULT CreateUnorderedAccessView() = 0;
	virtual HRESULT CreateRenderTargetView(APIResource pResource, const RenderTargetViewDesc* pDesc, RenderTargetView *ppRTView) = 0;
	virtual HRESULT CreateDepthStencilView(APIResource pResource, const DepthStencilViewDesc* pDesc, DepthStencilView *ppDepthStencilView) = 0;
	virtual HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements, 
		const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *ppInputLayout) = 0;
	virtual HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, VertexShader *ppVertexShader) = 0;
	virtual HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, PixelShader *ppPixelShader) = 0;
	virtual HRESULT CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, GeometryShader *ppGeometryShader) = 0;
	virtual HRESULT CreateGeometryShaderWithStreamOutput( const void *pShaderBytecode, SIZE_T BytecodeLength, const StreamOutDeclaration *pSODeclaration, 
		UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ClassLinkage pClassLinkage, GeometryShader *ppGeometryShader) = 0;
	virtual HRESULT CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, HullShader *ppHullShader) = 0;
	virtual HRESULT CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, DomainShader *ppDomainShader) = 0;
	virtual HRESULT CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage pClassLinkage, ComputeShader *ppComputeShader) = 0;
	virtual HRESULT CreateClassLinkage() = 0;
	virtual HRESULT CreateBlendState(const BlendDesc *pBlendStateDesc, BlendState *ppBlendState) = 0;
	virtual HRESULT CreateDepthStencilState(const DepthStencilDesc *pDepthStencilDesc, DepthStencilState *ppDepthStencilState) = 0;
	virtual HRESULT CreateRasterizerState(const RasterizerDesc *pRasterizerDesc, RasterizerState *ppRasterizerState) = 0;
	virtual HRESULT CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *ppSamplerState) = 0;
	virtual HRESULT CreateQuery() = 0;
	virtual HRESULT CreatePredicate() = 0;
	virtual HRESULT CreateDeferredContext(UINT ContextFlags, DeviceContext *ppDeferredContext) = 0;
	virtual HRESULT OpenSharedResource() = 0;
	virtual HRESULT CheckFormatSupport() = 0;
	virtual HRESULT CheckMultiSampleQualityLevels() = 0;
	virtual HRESULT CheckCounterInfo() = 0;
	virtual HRESULT CheckCounter() = 0;
	virtual HRESULT CheckFeatureSupport() = 0;
	virtual HRESULT GetPrivateData() = 0;
	virtual HRESULT SetPrivateData() = 0;
	virtual HRESULT SetPrivateDataInterface() = 0;
	virtual UINT GetCreationFlags() = 0;
	virtual HRESULT GetDeviceRemovedReason() = 0;
	virtual DeviceContext GetImmediateContext() = 0;

	virtual void PresentBegin() = 0;
	virtual void PresentEnd() = 0;

	virtual void ExecuteDeferredContexts() = 0;
	virtual void FinishCommandList(GRAPHICSTHREAD thread) = 0;

	virtual bool GetMultithreadingSupport() = 0;
	virtual DeviceContext GetDeferredContext(GRAPHICSTHREAD thread) = 0;

	virtual bool GetVSyncEnabled() = 0;
	virtual void SetVSyncEnabled(bool value) = 0;

	///////////////Thread-sensitive////////////////////////

	virtual void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindRenderTargets(UINT NumViews, RenderTargetView const *ppRenderTargetViews, DepthStencilView pDepthStencilView, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void ClearRenderTarget(RenderTargetView pRenderTargetView, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void ClearDepthStencil(DepthStencilView pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindTexturePS(TextureView texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindTexturesPS(TextureView textures[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindTextureVS(TextureView texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindTexturesVS(TextureView textures[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindTextureGS(TextureView texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindTexturesGS(TextureView textures[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindTextureDS(TextureView texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindTexturesDS(TextureView textures[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindTextureHS(TextureView texture, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindTexturesHS(TextureView textures[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void UnbindTextures(int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindSamplerPS(Sampler sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindSamplersPS(Sampler samplers[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindSamplerVS(Sampler sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindSamplersVS(Sampler samplers[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindSamplerGS(Sampler sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindSamplersGS(Sampler samplers[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindSamplerHS(Sampler sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindSamplersHS(Sampler samplers[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindSamplerDS(Sampler sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindSamplersDS(Sampler samplers[], int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindConstantBufferPS(BufferResource buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindConstantBufferVS(BufferResource buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindConstantBufferGS(BufferResource buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindConstantBufferDS(BufferResource buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindConstantBufferHS(BufferResource buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindVertexBuffer(BufferResource vertexBuffer, int slot, UINT stride, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindIndexBuffer(BufferResource indexBuffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindVertexLayout(VertexLayout layout, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindBlendState(BlendState state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindBlendStateEx(BlendState state, const XMFLOAT4& blendFactor = XMFLOAT4(1, 1, 1, 1), UINT sampleMask = 0xffffffff,
		GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindDepthStencilState(DepthStencilState state, UINT stencilRef, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindRasterizerState(RasterizerState state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindStreamOutTarget(BufferResource buffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindPS(PixelShader shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindVS(VertexShader shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindGS(GeometryShader shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindHS(HullShader shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void BindDS(DomainShader shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void Draw(int vertexCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void DrawIndexed(int indexCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void DrawIndexedInstanced(int indexCount, int instanceCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void GenerateMips(TextureView texture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void CopyResource(APIResource pDstResource, APIResource pSrcResource, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void UpdateBuffer(BufferResource& buffer, const void* data, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, int dataSize = -1) = 0;

	virtual HRESULT CreateTextureFromFile(const wstring& fileName, TextureView *ppShaderResourceView, bool mipMaps = true, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;

};



