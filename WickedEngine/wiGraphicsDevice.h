#ifndef _GRAPHICSDEVICE_H_
#define _GRAPHICSDEVICE_H_

#include "CommonInclude.h"
#include "wiThreadSafeManager.h"
#include "wiEnums.h"
#include "wiGraphicsDescriptors.h"
#include "wiGraphicsResource.h"

namespace wiGraphicsTypes
{

	class GraphicsDevice : public wiThreadSafeManager
	{
	protected:
		uint64_t FRAMECOUNT;
		bool VSYNC;
		int SCREENWIDTH, SCREENHEIGHT;
		bool FULLSCREEN;
		bool RESOLUTIONCHANGED;
		static FORMAT BACKBUFFER_FORMAT;
		bool TESSELLATION, MULTITHREADED_RENDERING, CONSERVATIVE_RASTERIZATION, RASTERIZER_ORDERED_VIEWS, UNORDEREDACCESSTEXTURE_LOAD_EXT;
	public:
		GraphicsDevice() 
			:FRAMECOUNT(0), VSYNC(true), SCREENWIDTH(0), SCREENHEIGHT(0), FULLSCREEN(false), RESOLUTIONCHANGED(false),
			TESSELLATION(false), MULTITHREADED_RENDERING(false), CONSERVATIVE_RASTERIZATION(false),RASTERIZER_ORDERED_VIEWS(false), UNORDEREDACCESSTEXTURE_LOAD_EXT(false)
		{}

		virtual HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer) = 0;
		virtual HRESULT CreateTexture1D(const Texture1DDesc* pDesc, const SubresourceData *pInitialData, Texture1D **ppTexture1D) = 0;
		virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D) = 0;
		virtual HRESULT CreateTexture3D(const Texture3DDesc* pDesc, const SubresourceData *pInitialData, Texture3D **ppTexture3D) = 0;
		virtual HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
			const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout) = 0;
		virtual HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, VertexShader *pVertexShader) = 0;
		virtual HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, PixelShader *pPixelShader) = 0;
		virtual HRESULT CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, GeometryShader *pGeometryShader) = 0;
		virtual HRESULT CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, HullShader *pHullShader) = 0;
		virtual HRESULT CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, DomainShader *pDomainShader) = 0;
		virtual HRESULT CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ComputeShader *pComputeShader) = 0;
		virtual HRESULT CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState) = 0;
		virtual HRESULT CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState) = 0;
		virtual HRESULT CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState) = 0;
		virtual HRESULT CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) = 0;
		virtual HRESULT CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery) = 0;
		virtual HRESULT CreateGraphicsPSO(const GraphicsPSODesc* pDesc, GraphicsPSO* pso) = 0;
		virtual HRESULT CreateComputePSO(const ComputePSODesc* pDesc, ComputePSO* pso) = 0;

		virtual void PresentBegin() = 0;
		virtual void PresentEnd() = 0;

		virtual void ExecuteDeferredContexts() = 0;
		virtual void FinishCommandList(GRAPHICSTHREAD thread) = 0;

		bool GetVSyncEnabled() { return VSYNC; }
		void SetVSyncEnabled(bool value) { VSYNC = value; }
		uint64_t GetFrameCount() { return FRAMECOUNT; }

		int GetScreenWidth() { return SCREENWIDTH; }
		int GetScreenHeight() { return SCREENHEIGHT; }
		bool ResolutionChanged() { return RESOLUTIONCHANGED; }


		virtual void SetResolution(int width, int height) = 0;
		

		virtual Texture2D GetBackBuffer() = 0;

		enum GRAPHICSDEVICE_CAPABILITY
		{
			GRAPHICSDEVICE_CAPABILITY_TESSELLATION,
			GRAPHICSDEVICE_CAPABILITY_MULTITHREADED_RENDERING,
			GRAPHICSDEVICE_CAPABILITY_CONSERVATIVE_RASTERIZATION,
			GRAPHICSDEVICE_CAPABILITY_RASTERIZER_ORDERED_VIEWS,
			GRAPHICSDEVICE_CAPABILITY_UNORDEREDACCESSTEXTURE_LOAD_FORMAT_EXT,
			GRAPHICSDEVICE_CAPABILITY_COUNT,
		};
		bool CheckCapability(GRAPHICSDEVICE_CAPABILITY capability);

		XMMATRIX GetScreenProjection()
		{
			return XMMatrixOrthographicOffCenterLH(0, (float)GetScreenWidth(), (float)GetScreenHeight(), 0, -1, 1);
		}
		static FORMAT GetBackBufferFormat();


		///////////////Thread-sensitive////////////////////////

		virtual void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID) = 0;
		virtual void BindRenderTargetsUAVs(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GPUResource* const *ppUAVs, int slotUAV, int countUAV,
			GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindRenderTargets(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void ClearRenderTarget(Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindResourcePS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindResourceVS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindResourceGS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindResourceDS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindResourceHS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindResourceCS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindResourcesPS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) = 0;
		virtual void BindResourcesVS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) = 0;
		virtual void BindResourcesGS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) = 0;
		virtual void BindResourcesDS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) = 0;
		virtual void BindResourcesHS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) = 0;
		virtual void BindResourcesCS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) = 0;
		virtual void BindUnorderedAccessResourceCS(const GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindUnorderedAccessResourcesCS(const GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) = 0;
		virtual void UnBindResources(int slot, int num, GRAPHICSTHREAD threadID) = 0;
		virtual void UnBindUnorderedAccessResources(int slot, int num, GRAPHICSTHREAD threadID) = 0;
		virtual void BindSamplerPS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindSamplerVS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindSamplerGS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindSamplerHS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindSamplerDS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindSamplerCS(const Sampler* sampler, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindConstantBufferPS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindConstantBufferVS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindConstantBufferGS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindConstantBufferDS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindConstantBufferHS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindConstantBufferCS(const GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindVertexBuffers(const GPUBuffer *const* vertexBuffers, int slot, int count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID) = 0;
		virtual void BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID) = 0;
		virtual void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID) = 0;
		virtual void BindStencilRef(UINT value, GRAPHICSTHREAD threadID) = 0;
		virtual void BindBlendFactor(XMFLOAT4 value, GRAPHICSTHREAD threadID) = 0;
		virtual void BindGraphicsPSO(const GraphicsPSO* pso, GRAPHICSTHREAD threadID) = 0;
		virtual void BindComputePSO(const ComputePSO* pso, GRAPHICSTHREAD threadID) = 0;
		virtual void Draw(int vertexCount, UINT startVertexLocation, GRAPHICSTHREAD threadID) = 0;
		virtual void DrawIndexed(int indexCount, UINT startIndexLocation, UINT baseVertexLocation, GRAPHICSTHREAD threadID) = 0;
		virtual void DrawInstanced(int vertexCount, int instanceCount, UINT startVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID) = 0;
		virtual void DrawIndexedInstanced(int indexCount, int instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID) = 0;
		virtual void DrawInstancedIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) = 0;
		virtual void DrawIndexedInstancedIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) = 0;
		virtual void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID) = 0;
		virtual void DispatchIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) = 0;
		virtual void GenerateMips(Texture* texture, GRAPHICSTHREAD threadID) = 0;
		virtual void CopyTexture2D(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID) = 0;
		virtual void CopyTexture2D_Region(Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, const Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID) = 0;
		virtual void MSAAResolve(Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID) = 0;
		virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize = -1) = 0;
		virtual void* AllocateFromRingBuffer(GPURingBuffer* buffer, size_t dataSize, UINT& offsetIntoBuffer, GRAPHICSTHREAD threadID) = 0;
		virtual void InvalidateBufferAccess(GPUBuffer* buffer, GRAPHICSTHREAD threadID) = 0;
		virtual bool DownloadBuffer(GPUBuffer* bufferToDownload, GPUBuffer* bufferDest, void* dataDest, GRAPHICSTHREAD threadID) = 0;
		virtual void SetScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID) = 0;
		virtual void QueryBegin(GPUQuery *query, GRAPHICSTHREAD threadID) = 0;
		virtual void QueryEnd(GPUQuery *query, GRAPHICSTHREAD threadID) = 0;
		virtual bool QueryRead(GPUQuery *query, GRAPHICSTHREAD threadID) = 0;

		virtual HRESULT CreateTextureFromFile(const std::string& fileName, Texture2D **ppTexture, bool mipMaps, GRAPHICSTHREAD threadID) = 0;
		virtual HRESULT SaveTexturePNG(const std::string& fileName, Texture2D *pTexture, GRAPHICSTHREAD threadID) = 0;
		virtual HRESULT SaveTextureDDS(const std::string& fileName, Texture *pTexture, GRAPHICSTHREAD threadID) = 0;

		virtual void EventBegin(const std::string& name, GRAPHICSTHREAD threadID) = 0;
		virtual void EventEnd(GRAPHICSTHREAD threadID) = 0;
		virtual void SetMarker(const std::string& name, GRAPHICSTHREAD threadID) = 0;
	};

}

#endif // _GRAPHICSDEVICE_H_
