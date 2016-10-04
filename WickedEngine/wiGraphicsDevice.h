#ifndef _GRAPHICSDEVICE_H_
#define _GRAPHICSDEVICE_H_

#include "wiThreadSafeManager.h"
#include "wiEnums.h"
#include "wiGraphicsDescriptors.h"
#include "wiGraphicsResource.h"

namespace wiGraphicsTypes
{

	class GraphicsDevice : public wiThreadSafeManager
	{
	protected:
		long FRAMECOUNT;
		bool VSYNC;
		int SCREENWIDTH, SCREENHEIGHT;
		bool FULLSCREEN;
	public:
		GraphicsDevice() :FRAMECOUNT(0), VSYNC(true), SCREENWIDTH(0), SCREENHEIGHT(0), FULLSCREEN(false) {}

		virtual HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer) = 0;
		virtual HRESULT CreateTexture1D() = 0;
		virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D) = 0;
		virtual HRESULT CreateTexture3D() = 0;
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

		bool GetVSyncEnabled() { return VSYNC; }
		void SetVSyncEnabled(bool value) { VSYNC = value; }
		long GetFrameCount() { return FRAMECOUNT; }

		int GetScreenWidth() { return SCREENWIDTH; }
		int GetScreenHeight() { return SCREENHEIGHT; }

		virtual void SetScreenWidth(int value) = 0;
		virtual void SetScreenHeight(int value) = 0;

		virtual Texture2D GetBackBuffer() = 0;

		enum GRAPHICSDEVICE_CAPABILITY
		{
			GRAPHICSDEVICE_CAPABILITY_TESSELLATION,
			GRAPHICSDEVICE_CAPABILITY_MULTITHREADED_RENDERING,
			GRAPHICSDEVICE_CAPABILITY_COUNT,
		};
		virtual bool CheckCapability(GRAPHICSDEVICE_CAPABILITY capability) = 0;

		XMMATRIX GetScreenProjection()
		{
			return XMMatrixOrthographicOffCenterLH(0, (float)GetScreenWidth(), (float)GetScreenHeight(), 0, -1, 1);
		}

		///////////////Thread-sensitive////////////////////////

		virtual void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindRenderTargets(UINT NumViews, Texture2D* const *ppRenderTargetViews, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, UINT arrayIndex = 0) = 0;
		virtual void ClearRenderTarget(Texture2D* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, UINT arrayIndex = 0) = 0;
		virtual void ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, UINT arrayIndex = 0) = 0;
		virtual void BindResourcePS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindResourceVS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindResourceGS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindResourceDS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindResourceHS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindResourceCS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindUnorderedAccessResourceCS(const GPUUnorderedResource* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void UnBindResources(int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void UnBindUnorderedAccessResources(int slot, int num, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerPS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerVS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerGS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerHS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerDS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindSamplerCS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferPS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferVS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferGS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferDS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferHS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindConstantBufferCS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindVertexBuffer(const GPUBuffer* vertexBuffer, int slot, UINT stride, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindIndexBuffer(const GPUBuffer* indexBuffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindVertexLayout(const VertexLayout* layout, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindBlendState(const BlendState* state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindBlendStateEx(const BlendState* state, const XMFLOAT4& blendFactor = XMFLOAT4(1, 1, 1, 1), UINT sampleMask = 0xffffffff, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindDepthStencilState(const DepthStencilState* state, UINT stencilRef, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindRasterizerState(const RasterizerState* state, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindStreamOutTarget(const GPUBuffer* buffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindPS(const PixelShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindVS(const VertexShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindGS(const GeometryShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindHS(const HullShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindDS(const DomainShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void BindCS(const ComputeShader* shader, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void Draw(int vertexCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void DrawIndexed(int indexCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void DrawInstanced(int vertexCount, int instanceCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void DrawIndexedInstanced(int indexCount, int instanceCount, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void GenerateMips(Texture* texture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void CopyTexture2D(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE, int dataSize = -1) = 0;
		virtual GPUBuffer* DownloadBuffer(GPUBuffer* buffer, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void Map(GPUBuffer* resource, UINT subResource, MAP mapType, UINT mapFlags, MappedSubresource* mappedResource, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void Unmap(GPUBuffer* resource, UINT subResource = 0, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void SetScissorRects(UINT numRects, const Rect* rects = nullptr, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;

		virtual HRESULT CreateTextureFromFile(const string& fileName, Texture2D **ppTexture, bool mipMaps = true, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual HRESULT SaveTexturePNG(const string& fileName, Texture2D *pTexture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual HRESULT SaveTextureDDS(const string& fileName, Texture *pTexture, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;

		virtual void EventBegin(const wchar_t* name, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void EventEnd(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
		virtual void SetMarker(const wchar_t* name, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) = 0;
	};

}

#endif // _GRAPHICSDEVICE_H_
