#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiWindowRegistration.h"
#include "wiContainers.h"

#include <d3d11_3.h>
#include <DXGI1_3.h>

#include <atomic>

namespace wiGraphics
{

	class GraphicsDevice_DX11 : public GraphicsDevice
	{
	private:
		ID3D11Device*				device = nullptr;
		D3D_DRIVER_TYPE				driverType;
		D3D_FEATURE_LEVEL			featureLevel;
		IDXGISwapChain1*			swapChain = nullptr;
		ID3D11RenderTargetView*		renderTargetView = nullptr;
		ID3D11Texture2D*			backBuffer = nullptr;
		ID3D11DeviceContext*		immediateContext = nullptr;
		ID3D11DeviceContext*		deviceContexts[COMMANDLIST_COUNT] = {};
		ID3D11CommandList*			commandLists[COMMANDLIST_COUNT] = {};
		ID3DUserDefinedAnnotation*	userDefinedAnnotations[COMMANDLIST_COUNT] = {};

		uint32_t	stencilRef[COMMANDLIST_COUNT];
		XMFLOAT4	blendFactor[COMMANDLIST_COUNT];

		ID3D11VertexShader* prev_vs[COMMANDLIST_COUNT] = {};
		ID3D11PixelShader* prev_ps[COMMANDLIST_COUNT] = {};
		ID3D11HullShader* prev_hs[COMMANDLIST_COUNT] = {};
		ID3D11DomainShader* prev_ds[COMMANDLIST_COUNT] = {};
		ID3D11GeometryShader* prev_gs[COMMANDLIST_COUNT] = {};
		ID3D11ComputeShader* prev_cs[COMMANDLIST_COUNT] = {};
		XMFLOAT4 prev_blendfactor[COMMANDLIST_COUNT] = {};
		uint32_t prev_samplemask[COMMANDLIST_COUNT] = {};
		ID3D11BlendState* prev_bs[COMMANDLIST_COUNT] = {};
		ID3D11RasterizerState* prev_rs[COMMANDLIST_COUNT] = {};
		uint32_t prev_stencilRef[COMMANDLIST_COUNT] = {};
		ID3D11DepthStencilState* prev_dss[COMMANDLIST_COUNT] = {};
		ID3D11InputLayout* prev_il[COMMANDLIST_COUNT] = {};
		PRIMITIVETOPOLOGY prev_pt[COMMANDLIST_COUNT] = {};

		ID3D11UnorderedAccessView* raster_uavs[COMMANDLIST_COUNT][8] = {};
		uint8_t raster_uavs_slot[COMMANDLIST_COUNT] = {};
		uint8_t raster_uavs_count[COMMANDLIST_COUNT] = {};
		void validate_raster_uavs(CommandList cmd);

		struct GPUAllocator
		{
			GPUBuffer buffer;
			size_t byteOffset = 0;
			uint64_t residentFrame = 0;
			bool dirty = false;
		} frame_allocators[COMMANDLIST_COUNT];
		void commit_allocations(CommandList cmd);

		void CreateBackBufferResources();

		std::atomic<uint8_t> commandlist_count{ 0 };
		wiContainers::ThreadSafeRingBuffer<CommandList, COMMANDLIST_COUNT> free_commandlists;
		wiContainers::ThreadSafeRingBuffer<CommandList, COMMANDLIST_COUNT> active_commandlists;

	public:
		GraphicsDevice_DX11(wiWindowRegistration::window_type window, bool fullscreen = false, bool debuglayer = false);
		virtual ~GraphicsDevice_DX11();

		HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer) override;
		HRESULT CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture) override;
		HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, uint32_t NumElements, const ShaderByteCode* shaderCode, VertexLayout *pInputLayout) override;
		HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, VertexShader *pVertexShader) override;
		HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, PixelShader *pPixelShader) override;
		HRESULT CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, GeometryShader *pGeometryShader) override;
		HRESULT CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, HullShader *pHullShader) override;
		HRESULT CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, DomainShader *pDomainShader) override;
		HRESULT CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ComputeShader *pComputeShader) override;
		HRESULT CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState) override;
		HRESULT CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState) override;
		HRESULT CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState) override;
		HRESULT CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) override;
		HRESULT CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery) override;
		HRESULT CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso) override;
		HRESULT CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) override;

		int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) override;

		void DestroyResource(GPUResource* pResource) override;
		void DestroyBuffer(GPUBuffer *pBuffer) override;
		void DestroyTexture(Texture *pTexture) override;
		void DestroyInputLayout(VertexLayout *pInputLayout) override;
		void DestroyVertexShader(VertexShader *pVertexShader) override;
		void DestroyPixelShader(PixelShader *pPixelShader) override;
		void DestroyGeometryShader(GeometryShader *pGeometryShader) override;
		void DestroyHullShader(HullShader *pHullShader) override;
		void DestroyDomainShader(DomainShader *pDomainShader) override;
		void DestroyComputeShader(ComputeShader *pComputeShader) override;
		void DestroyBlendState(BlendState *pBlendState) override;
		void DestroyDepthStencilState(DepthStencilState *pDepthStencilState) override;
		void DestroyRasterizerState(RasterizerState *pRasterizerState) override;
		void DestroySamplerState(Sampler *pSamplerState) override;
		void DestroyQuery(GPUQuery *pQuery) override;
		void DestroyPipelineState(PipelineState* pso) override;
		void DestroyRenderPass(RenderPass* renderpass) override;

		bool DownloadResource(const GPUResource* resourceToDownload, const GPUResource* resourceDest, void* dataDest) override;

		void SetName(GPUResource* pResource, const std::string& name) override;

		void PresentBegin(CommandList cmd) override;
		void PresentEnd(CommandList cmd) override;

		void WaitForGPU() override;

		virtual CommandList BeginCommandList() override;

		void SetResolution(int width, int height) override;

		Texture GetBackBuffer() override;

		///////////////Thread-sensitive////////////////////////

		void RenderPassBegin(const RenderPass* renderpass, CommandList cmd) override;
		void RenderPassEnd(CommandList cmd) override;
		void BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) override;
		void BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd) override;
		void BindResource(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) override;
		void BindResources(SHADERSTAGE stage, const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd) override;
		void BindUAV(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) override;
		void BindUAVs(SHADERSTAGE stage, const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd) override;
		void UnbindResources(uint32_t slot, uint32_t num, CommandList cmd) override;
		void UnbindUAVs(uint32_t slot, uint32_t num, CommandList cmd) override;
		void BindSampler(SHADERSTAGE stage, const Sampler* sampler, uint32_t slot, CommandList cmd) override;
		void BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, uint32_t slot, CommandList cmd) override;
		void BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets, CommandList cmd) override;
		void BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, uint32_t offset, CommandList cmd) override;
		void BindStencilRef(uint32_t value, CommandList cmd) override;
		void BindBlendFactor(float r, float g, float b, float a, CommandList cmd) override;
		void BindPipelineState(const PipelineState* pso, CommandList cmd) override;
		void BindComputeShader(const ComputeShader* cs, CommandList cmd) override;
		void Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) override;
		void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, CommandList cmd) override;
		void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) override;
		void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) override;
		void DrawInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd) override;
		void DrawIndexedInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd) override;
		void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) override;
		void DispatchIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd) override;
		void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) override;
		void CopyTexture2D_Region(const Texture* pDst, uint32_t dstMip, uint32_t dstX, uint32_t dstY, const Texture* pSrc, uint32_t srcMip, CommandList cmd) override;
		void MSAAResolve(const Texture* pDst, const Texture* pSrc, CommandList cmd) override;
		void UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize = -1) override;
		void QueryBegin(const GPUQuery *query, CommandList cmd) override;
		void QueryEnd(const GPUQuery *query, CommandList cmd) override;
		bool QueryRead(const GPUQuery* query, GPUQueryResult* result) override;
		void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) override {}

		GPUAllocation AllocateGPU(size_t dataSize, CommandList cmd) override;

		void EventBegin(const std::string& name, CommandList cmd) override;
		void EventEnd(CommandList cmd) override;
		void SetMarker(const std::string& name, CommandList cmd) override;
	};

}
