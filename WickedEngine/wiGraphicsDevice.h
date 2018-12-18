#ifndef _GRAPHICSDEVICE_H_
#define _GRAPHICSDEVICE_H_

#include "CommonInclude.h"
#include "wiThreadSafeManager.h"
#include "wiEnums.h"
#include "wiGraphicsDescriptors.h"
#include "wiGraphicsResource.h"

namespace wiGraphicsTypes
{

	class GraphicsDevice
	{
	protected:
		uint64_t FRAMECOUNT = 0;
		bool VSYNC = true;
		int SCREENWIDTH = 0;
		int SCREENHEIGHT = 0;
		bool FULLSCREEN = false;
		bool RESOLUTIONCHANGED = false;
		FORMAT BACKBUFFER_FORMAT = FORMAT_R10G10B10A2_UNORM;
		static const UINT BACKBUFFER_COUNT = 2;
		bool TESSELLATION = false;
		bool MULTITHREADED_RENDERING = false;
		bool CONSERVATIVE_RASTERIZATION = false;
		bool RASTERIZER_ORDERED_VIEWS = false;
		bool UNORDEREDACCESSTEXTURE_LOAD_EXT = false;
	public:

		virtual HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer) = 0;
		virtual HRESULT CreateTexture1D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture1D **ppTexture1D) = 0;
		virtual HRESULT CreateTexture2D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D) = 0;
		virtual HRESULT CreateTexture3D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture3D **ppTexture3D) = 0;
		virtual HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements, const ShaderByteCode* shaderCode, VertexLayout *pInputLayout) = 0;
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


		virtual void DestroyResource(GPUResource* pResource) = 0;
		virtual void DestroyBuffer(GPUBuffer *pBuffer) = 0;
		virtual void DestroyTexture1D(Texture1D *pTexture1D) = 0;
		virtual void DestroyTexture2D(Texture2D *pTexture2D) = 0;
		virtual void DestroyTexture3D(Texture3D *pTexture3D) = 0;
		virtual void DestroyInputLayout(VertexLayout *pInputLayout) = 0;
		virtual void DestroyVertexShader(VertexShader *pVertexShader) = 0;
		virtual void DestroyPixelShader(PixelShader *pPixelShader) = 0;
		virtual void DestroyGeometryShader(GeometryShader *pGeometryShader) = 0;
		virtual void DestroyHullShader(HullShader *pHullShader) = 0;
		virtual void DestroyDomainShader(DomainShader *pDomainShader) = 0;
		virtual void DestroyComputeShader(ComputeShader *pComputeShader) = 0;
		virtual void DestroyBlendState(BlendState *pBlendState) = 0;
		virtual void DestroyDepthStencilState(DepthStencilState *pDepthStencilState) = 0;
		virtual void DestroyRasterizerState(RasterizerState *pRasterizerState) = 0;
		virtual void DestroySamplerState(Sampler *pSamplerState) = 0;
		virtual void DestroyQuery(GPUQuery *pQuery) = 0;
		virtual void DestroyGraphicsPSO(GraphicsPSO* pso) = 0;
		virtual void DestroyComputePSO(ComputePSO* pso) = 0;

		virtual void SetName(GPUResource* pResource, const std::string& name) = 0;

		virtual void PresentBegin() = 0;
		virtual void PresentEnd() = 0;

		virtual void CreateCommandLists() = 0;
		virtual void ExecuteCommandLists() = 0;
		virtual void FinishCommandList(GRAPHICSTHREAD thread) = 0;

		virtual void WaitForGPU() = 0;

		inline bool GetVSyncEnabled() const { return VSYNC; }
		inline void SetVSyncEnabled(bool value) { VSYNC = value; }
		inline uint64_t GetFrameCount() const { return FRAMECOUNT; }

		inline int GetScreenWidth() const { return SCREENWIDTH; }
		inline int GetScreenHeight() const { return SCREENHEIGHT; }
		inline bool ResolutionChanged() const { return RESOLUTIONCHANGED; }


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
		bool CheckCapability(GRAPHICSDEVICE_CAPABILITY capability) const;

		uint32_t GetFormatStride(FORMAT value) const;
		bool IsFormatUnorm(FORMAT value) const;

		inline XMMATRIX GetScreenProjection() const
		{
			return XMMatrixOrthographicOffCenterLH(0, (float)GetScreenWidth(), (float)GetScreenHeight(), 0, -1, 1);
		}
		inline FORMAT GetBackBufferFormat() const { return BACKBUFFER_FORMAT; }
		inline static UINT GetBackBufferCount() { return BACKBUFFER_COUNT; }


		///////////////Thread-sensitive////////////////////////

		virtual void BindScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID) = 0;
		virtual void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID) = 0;
		virtual void BindRenderTargets(UINT NumViews, Texture2D* const *ppRenderTargets, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void ClearRenderTarget(Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindResource(SHADERSTAGE stage, GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindResources(SHADERSTAGE stage, GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) = 0;
		virtual void BindUAV(SHADERSTAGE stage, GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) = 0;
		virtual void BindUAVs(SHADERSTAGE stage, GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) = 0;
		virtual void UnbindResources(int slot, int num, GRAPHICSTHREAD threadID) = 0;
		virtual void UnbindUAVs(int slot, int num, GRAPHICSTHREAD threadID) = 0;
		virtual void BindSampler(SHADERSTAGE stage, Sampler* sampler, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindConstantBuffer(SHADERSTAGE stage, GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) = 0;
		virtual void BindVertexBuffers(GPUBuffer *const* vertexBuffers, int slot, int count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID) = 0;
		virtual void BindIndexBuffer(GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID) = 0;
		virtual void BindStencilRef(UINT value, GRAPHICSTHREAD threadID) = 0;
		virtual void BindBlendFactor(XMFLOAT4 value, GRAPHICSTHREAD threadID) = 0;
		virtual void BindGraphicsPSO(GraphicsPSO* pso, GRAPHICSTHREAD threadID) = 0;
		virtual void BindComputePSO(ComputePSO* pso, GRAPHICSTHREAD threadID) = 0;
		virtual void Draw(int vertexCount, UINT startVertexLocation, GRAPHICSTHREAD threadID) = 0;
		virtual void DrawIndexed(int indexCount, UINT startIndexLocation, UINT baseVertexLocation, GRAPHICSTHREAD threadID) = 0;
		virtual void DrawInstanced(int vertexCount, int instanceCount, UINT startVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID) = 0;
		virtual void DrawIndexedInstanced(int indexCount, int instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID) = 0;
		virtual void DrawInstancedIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) = 0;
		virtual void DrawIndexedInstancedIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) = 0;
		virtual void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID) = 0;
		virtual void DispatchIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) = 0;
		virtual void CopyTexture2D(Texture2D* pDst, Texture2D* pSrc, GRAPHICSTHREAD threadID) = 0;
		virtual void CopyTexture2D_Region(Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID) = 0;
		virtual void MSAAResolve(Texture2D* pDst, Texture2D* pSrc, GRAPHICSTHREAD threadID) = 0;
		virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize = -1) = 0;
		virtual void* AllocateFromRingBuffer(GPURingBuffer* buffer, size_t dataSize, UINT& offsetIntoBuffer, GRAPHICSTHREAD threadID) = 0;
		virtual void InvalidateBufferAccess(GPUBuffer* buffer, GRAPHICSTHREAD threadID) = 0;
		virtual bool DownloadResource(GPUResource* resourceToDownload, GPUResource* resourceDest, void* dataDest, GRAPHICSTHREAD threadID) = 0;
		virtual void QueryBegin(GPUQuery *query, GRAPHICSTHREAD threadID) = 0;
		virtual void QueryEnd(GPUQuery *query, GRAPHICSTHREAD threadID) = 0;
		virtual bool QueryRead(GPUQuery *query, GRAPHICSTHREAD threadID) = 0;
		virtual void UAVBarrier(GPUResource *const* uavs, UINT NumBarriers, GRAPHICSTHREAD threadID) = 0;
		virtual void TransitionBarrier(GPUResource *const* resources, UINT NumBarriers, RESOURCE_STATES stateBefore, RESOURCE_STATES stateAfter, GRAPHICSTHREAD threadID) = 0;
		
		virtual void EventBegin(const std::string& name, GRAPHICSTHREAD threadID) = 0;
		virtual void EventEnd(GRAPHICSTHREAD threadID) = 0;
		virtual void SetMarker(const std::string& name, GRAPHICSTHREAD threadID) = 0;
	};

}

#endif // _GRAPHICSDEVICE_H_
