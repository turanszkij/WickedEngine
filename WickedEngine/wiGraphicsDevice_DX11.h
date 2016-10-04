#ifndef _GRAPHICSDEVICE_DX11_H_
#define _GRAPHICSDEVICE_DX11_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiWindowRegistration.h"

interface ID3D11Device;
interface IDXGISwapChain;
interface IDXGISwapChain1;
interface ID3D11DeviceContext;
interface ID3D11CommandList;
interface ID3DUserDefinedAnnotation;
enum D3D_DRIVER_TYPE;
enum D3D_FEATURE_LEVEL;

namespace wiGraphicsTypes
{

	class GraphicsDevice_DX11 : public GraphicsDevice
	{
	private:
		ID3D11Device*				device;
		D3D_DRIVER_TYPE				driverType;
		D3D_FEATURE_LEVEL			featureLevel;
#ifndef WINSTORE_SUPPORT
		IDXGISwapChain*				swapChain;
#else
		IDXGISwapChain1*			swapChain;
#endif
		ID3D11RenderTargetView*		renderTargetView;
		ID3D11Texture2D*			backBuffer;
		ViewPort					viewPort;
		ID3D11DeviceContext*		deviceContexts[GRAPHICSTHREAD_COUNT];
		ID3D11CommandList*			commandLists[GRAPHICSTHREAD_COUNT];
		bool						DX11, DEFERREDCONTEXT_SUPPORT;
		ID3DUserDefinedAnnotation*	userDefinedAnnotations[GRAPHICSTHREAD_COUNT];

	public:
		GraphicsDevice_DX11(wiWindowRegistration::window_type window, bool fullscreen = false);

		~GraphicsDevice_DX11();

		virtual HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer) override;
		virtual HRESULT CreateTexture1D() override;
		virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D) override;
		virtual HRESULT CreateTexture3D() override;
		virtual HRESULT CreateShaderResourceView(Texture2D* pTexture) override;
		virtual HRESULT CreateRenderTargetView(Texture2D* pTexture) override;
		virtual HRESULT CreateDepthStencilView(Texture2D* pTexture) override;
		virtual HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
			const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout) override;
		virtual HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, VertexShader *pVertexShader) override;
		virtual HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, PixelShader *pPixelShader) override;
		virtual HRESULT CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, GeometryShader *pGeometryShader) override;
		virtual HRESULT CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode, SIZE_T BytecodeLength, const StreamOutDeclaration *pSODeclaration,
			UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ClassLinkage* pClassLinkage, GeometryShader *pGeometryShader) override;
		virtual HRESULT CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, HullShader *pHullShader) override;
		virtual HRESULT CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, DomainShader *pDomainShader) override;
		virtual HRESULT CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ClassLinkage* pClassLinkage, ComputeShader *pComputeShader) override;
		virtual HRESULT CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState) override;
		virtual HRESULT CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState) override;
		virtual HRESULT CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState) override;
		virtual HRESULT CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) override;

		virtual void PresentBegin() override;
		virtual void PresentEnd() override;

		virtual void ExecuteDeferredContexts() override;
		virtual void FinishCommandList(GRAPHICSTHREAD thread) override;

		virtual void SetScreenWidth(int value) override;
		virtual void SetScreenHeight(int value) override;

		virtual Texture2D GetBackBuffer() override;

		virtual bool CheckCapability(GRAPHICSDEVICE_CAPABILITY capability) override;

		///////////////Thread-sensitive////////////////////////

		virtual void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindRenderTargets(UINT NumViews, Texture2D* const *ppRenderTargetViews, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, UINT arrayIndex = 0) override;
		virtual void ClearRenderTarget(Texture2D* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, UINT arrayIndex = 0) override;
		virtual void ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, UINT arrayIndex = 0) override;
		virtual void BindResourcePS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindResourceVS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindResourceGS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindResourceDS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindResourceHS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindResourceCS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindUnorderedAccessResourceCS(const GPUUnorderedResource* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void UnBindResources(int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void UnBindUnorderedAccessResources(int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindSamplerPS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindSamplerVS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindSamplerGS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindSamplerHS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindSamplerDS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindSamplerCS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindConstantBufferPS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindConstantBufferVS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindConstantBufferGS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindConstantBufferDS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindConstantBufferHS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindConstantBufferCS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindVertexBuffer(const GPUBuffer* vertexBuffer, int slot, UINT stride, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindIndexBuffer(const GPUBuffer* indexBuffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindVertexLayout(const VertexLayout* layout, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindBlendState(const BlendState* state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindBlendStateEx(const BlendState* state, const XMFLOAT4& blendFactor = XMFLOAT4(1, 1, 1, 1), UINT sampleMask = 0xffffffff, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindDepthStencilState(const DepthStencilState* state, UINT stencilRef, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindRasterizerState(const RasterizerState* state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindStreamOutTarget(const GPUBuffer* buffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindPS(const PixelShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindVS(const VertexShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindGS(const GeometryShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindHS(const HullShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindDS(const DomainShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void BindCS(const ComputeShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void Draw(int vertexCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void DrawIndexed(int indexCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void DrawInstanced(int vertexCount, int instanceCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void DrawIndexedInstanced(int indexCount, int instanceCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void GenerateMips(Texture* texture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void CopyTexture2D(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, int dataSize = -1) override;
		virtual GPUBuffer* DownloadBuffer(GPUBuffer* buffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void Map(GPUBuffer* resource, UINT subResource, MAP mapType, UINT mapFlags, MappedSubresource* mappedResource, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void Unmap(GPUBuffer* resource, UINT subResource, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void SetScissorRects(UINT numRects, const Rect* rects = nullptr, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;

		virtual HRESULT CreateTextureFromFile(const string& fileName, Texture2D **ppTexture, bool mipMaps = true, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual HRESULT SaveTexturePNG(const string& fileName, Texture2D *pTexture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual HRESULT SaveTextureDDS(const string& fileName, Texture *pTexture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;

		virtual void EventBegin(const wchar_t* name, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void EventEnd(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
		virtual void SetMarker(const wchar_t* name, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
	};

}

#endif // _GRAPHICSDEVICE_DX11_H_
