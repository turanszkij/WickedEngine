#pragma once
#include "CommonInclude.h"
#include "wiThreadSafeManager.h"
#include "wiEnums.h"

#include <d3d11_2.h>
#include <DXGI1_2.h>

#include "ConstantBufferMapping.h"


typedef ID3D11RenderTargetView*		RenderTargetView;
typedef ID3D11ShaderResourceView*	ShaderResourceView;
typedef ID3D11Texture2D*			APITexture2D;
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

class Texture
{
public:
	ShaderResourceView	shaderResourceView;
	RenderTargetView	renderTargetView;
	DepthStencilView	depthStencilView;

	Texture() :shaderResourceView(nullptr), renderTargetView(nullptr), depthStencilView(nullptr) {}
	virtual ~Texture()
	{
		SAFE_RELEASE(shaderResourceView);
		SAFE_RELEASE(renderTargetView);
		SAFE_RELEASE(depthStencilView);
	}
};
class Texture2D : public Texture
{
public:
	APITexture2D		texture2D;
	Texture2DDesc		desc;

	Texture2D() :Texture(), texture2D(nullptr) {}
	virtual ~Texture2D()
	{
		SAFE_RELEASE(texture2D);
	}
};
class TextureCube : public Texture2D
{
public:
	TextureCube():Texture2D() {}
	virtual ~TextureCube(){}
};

class GraphicsDevice : public wiThreadSafeManager
{
protected:
	bool oddFrame;
	bool VSYNC;
	int SCREENWIDTH, SCREENHEIGHT;
public:
	GraphicsDevice() :oddFrame(false), VSYNC(true), SCREENWIDTH(0), SCREENHEIGHT(0) {}

	virtual HRESULT CreateBuffer(const BufferDesc *pDesc, const SubresourceData* pInitialData, BufferResource *ppBuffer) = 0;
	virtual HRESULT CreateTexture1D() = 0;
	virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D) = 0;
	virtual HRESULT CreateTexture3D() = 0;
	virtual HRESULT CreateTextureCube(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, TextureCube **ppTextureCube) = 0;
	virtual HRESULT CreateShaderResourceView(Texture2D* pTexture) = 0;
	virtual HRESULT CreateRenderTargetView(Texture2D* pTexture) = 0;
	virtual HRESULT CreateDepthStencilView(Texture2D* pTexture) = 0;
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
	virtual HRESULT CreateBlendState(const BlendDesc *pBlendStateDesc, BlendState *ppBlendState) = 0;
	virtual HRESULT CreateDepthStencilState(const DepthStencilDesc *pDepthStencilDesc, DepthStencilState *ppDepthStencilState) = 0;
	virtual HRESULT CreateRasterizerState(const RasterizerDesc *pRasterizerDesc, RasterizerState *ppRasterizerState) = 0;
	virtual HRESULT CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *ppSamplerState) = 0;

	virtual void PresentBegin() = 0;
	virtual void PresentEnd() = 0;

	virtual void ExecuteDeferredContexts() = 0;
	virtual void FinishCommandList(GRAPHICSTHREAD thread) = 0;

	virtual bool GetMultithreadingSupport() = 0;

	bool GetVSyncEnabled() { return VSYNC; }
	void SetVSyncEnabled(bool value) { VSYNC = value; }

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
	virtual void GenerateMips(Texture* texture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void CopyResource(APIResource pDstResource, const APIResource pSrcResource, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void CopyTexture2D(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	virtual void UpdateBuffer(BufferResource& buffer, const void* data, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, int dataSize = -1) = 0;

	virtual HRESULT CreateTextureFromFile(const wstring& fileName, Texture2D **ppTexture, bool mipMaps = true, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;

};



