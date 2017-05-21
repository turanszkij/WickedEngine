#ifndef _GRAPHICSDEVICE_DX11_H_
#define _GRAPHICSDEVICE_DX11_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiWindowRegistration.h"

struct ID3D11Device;
struct IDXGISwapChain1;
struct ID3D11DeviceContext;
struct ID3D11CommandList;
struct ID3DUserDefinedAnnotation;
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
		IDXGISwapChain1*			swapChain;
		ID3D11RenderTargetView*		renderTargetView;
		ID3D11Texture2D*			backBuffer;
		ViewPort					viewPort;
		ID3D11DeviceContext*		deviceContexts[GRAPHICSTHREAD_COUNT];
		ID3D11CommandList*			commandLists[GRAPHICSTHREAD_COUNT];
		ID3DUserDefinedAnnotation*	userDefinedAnnotations[GRAPHICSTHREAD_COUNT];

	public:
		GraphicsDevice_DX11(wiWindowRegistration::window_type window, bool fullscreen = false);

		~GraphicsDevice_DX11();

		virtual HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer) override;
		virtual HRESULT CreateTexture1D(const Texture1DDesc* pDesc, const SubresourceData *pInitialData, Texture1D **ppTexture1D) override;
		virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D) override;
		virtual HRESULT CreateTexture3D(const Texture3DDesc* pDesc, const SubresourceData *pInitialData, Texture3D **ppTexture3D) override;
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
		virtual HRESULT CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery) override;

		virtual void PresentBegin() override;
		virtual void PresentEnd() override;

		virtual void ExecuteDeferredContexts() override;
		virtual void FinishCommandList(GRAPHICSTHREAD thread) override;

		virtual void SetResolution(int width, int height) override;

		virtual Texture2D GetBackBuffer() override;

		///////////////Thread-sensitive////////////////////////

		virtual void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID) override;
		virtual void BindRenderTargetsUAVs(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GPUUnorderedResource* const *ppUAVs, int slotUAV, int countUAV,
			GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindRenderTargets(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void ClearRenderTarget(Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourcePS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourceVS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourceGS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourceDS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourceHS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourceCS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResourcesPS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindResourcesVS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindResourcesGS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindResourcesDS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindResourcesHS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindResourcesCS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void BindUnorderedAccessResourceCS(const GPUUnorderedResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindUnorderedAccessResourcesCS(const GPUUnorderedResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void UnBindResources(int slot, int num, GRAPHICSTHREAD threadID) override;
		virtual void UnBindUnorderedAccessResources(int slot, int num, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerPS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerVS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerGS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerHS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerDS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindSamplerCS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferPS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferVS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferGS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferDS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferHS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBufferCS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindVertexBuffers(const GPUBuffer* const *vertexBuffers, int slot, int count, const UINT* strides, GRAPHICSTHREAD threadID) override;
		virtual void BindIndexBuffer(const GPUBuffer* indexBuffer, GRAPHICSTHREAD threadID) override;
		virtual void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID) override;
		virtual void BindVertexLayout(const VertexLayout* layout, GRAPHICSTHREAD threadID) override;
		virtual void BindBlendState(const BlendState* state, GRAPHICSTHREAD threadID) override;
		virtual void BindBlendStateEx(const BlendState* state, const XMFLOAT4& blendFactor, UINT sampleMask, GRAPHICSTHREAD threadID) override;
		virtual void BindDepthStencilState(const DepthStencilState* state, UINT stencilRef, GRAPHICSTHREAD threadID) override;
		virtual void BindRasterizerState(const RasterizerState* state, GRAPHICSTHREAD threadID) override;
		virtual void BindStreamOutTargets(GPUBuffer* const *buffers, UINT count, GRAPHICSTHREAD threadID) override;
		virtual void BindPS(const PixelShader* shader, GRAPHICSTHREAD threadID) override;
		virtual void BindVS(const VertexShader* shader, GRAPHICSTHREAD threadID) override;
		virtual void BindGS(const GeometryShader* shader, GRAPHICSTHREAD threadID) override;
		virtual void BindHS(const HullShader* shader, GRAPHICSTHREAD threadID) override;
		virtual void BindDS(const DomainShader* shader, GRAPHICSTHREAD threadID) override;
		virtual void BindCS(const ComputeShader* shader, GRAPHICSTHREAD threadID) override;
		virtual void Draw(int vertexCount, GRAPHICSTHREAD threadID) override;
		virtual void DrawIndexed(int indexCount, GRAPHICSTHREAD threadID) override;
		virtual void DrawInstanced(int vertexCount, int instanceCount, GRAPHICSTHREAD threadID) override;
		virtual void DrawIndexedInstanced(int indexCount, int instanceCount, GRAPHICSTHREAD threadID) override;
		virtual void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID) override;
		virtual void GenerateMips(Texture* texture, GRAPHICSTHREAD threadID) override;
		virtual void CopyTexture2D(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID) override;
		virtual void CopyTexture2D_Region(Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, const Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID) override;
		virtual void MSAAResolve(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID) override;
		virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize = -1) override;
		virtual GPUBuffer* DownloadBuffer(GPUBuffer* buffer, GRAPHICSTHREAD threadID) override;
		virtual void Map(GPUBuffer* resource, UINT subResource, MAP mapType, UINT mapFlags, MappedSubresource* mappedResource, GRAPHICSTHREAD threadID) override;
		virtual void Unmap(GPUBuffer* resource, UINT subResource, GRAPHICSTHREAD threadID) override;
		virtual void SetScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID) override;
		virtual void QueryBegin(GPUQuery *query, GRAPHICSTHREAD threadID) override;
		virtual void QueryEnd(GPUQuery *query, GRAPHICSTHREAD threadID) override;
		virtual bool QueryRead(GPUQuery *query, GRAPHICSTHREAD threadID) override;

		virtual HRESULT CreateTextureFromFile(const std::string& fileName, Texture2D **ppTexture, bool mipMaps, GRAPHICSTHREAD threadID) override;
		virtual HRESULT SaveTexturePNG(const std::string& fileName, Texture2D *pTexture, GRAPHICSTHREAD threadID) override;
		virtual HRESULT SaveTextureDDS(const std::string& fileName, Texture *pTexture, GRAPHICSTHREAD threadID) override;

		virtual void EventBegin(const std::string& name, GRAPHICSTHREAD threadID) override;
		virtual void EventEnd(GRAPHICSTHREAD threadID) override;
		virtual void SetMarker(const std::string& name, GRAPHICSTHREAD threadID) override;

	private:
		HRESULT CreateShaderResourceView(Texture1D* pTexture);
		HRESULT CreateShaderResourceView(Texture2D* pTexture);
		HRESULT CreateShaderResourceView(Texture3D* pTexture);
		HRESULT CreateRenderTargetView(Texture2D* pTexture);
		HRESULT CreateRenderTargetView(Texture3D* pTexture);
		HRESULT CreateDepthStencilView(Texture2D* pTexture);
	};

}

#endif // _GRAPHICSDEVICE_DX11_H_
