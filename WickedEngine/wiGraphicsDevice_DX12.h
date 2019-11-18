#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiWindowRegistration.h"
#include "wiSpinLock.h"
#include "wiContainers.h"
#include "wiGraphicsDevice_SharedInternals.h"

#include "Utility/D3D12MemAlloc.h"

#include <dxgi1_4.h>
#include <d3d12.h>

#include <unordered_map>
#include <deque>
#include <atomic>
#include <mutex>


namespace wiGraphics
{

	class GraphicsDevice_DX12 : public GraphicsDevice
	{
	private:
		ID3D12Device*				device = nullptr;
		ID3D12CommandQueue*			directQueue = nullptr;
		ID3D12Fence*				frameFence = nullptr;
		HANDLE						frameFenceEvent;
		D3D12MA::Allocator*			allocator = nullptr;

		ID3D12CommandQueue*			copyQueue = nullptr;
		ID3D12CommandAllocator*		copyAllocator = nullptr;
		ID3D12CommandList*			copyCommandList = nullptr;
		ID3D12Fence*				copyFence = nullptr;
		HANDLE						copyFenceEvent;
		UINT64						copyFenceValue;
		wiSpinLock					copyQueueLock;

		ID3D12RootSignature*		graphicsRootSig = nullptr;
		ID3D12RootSignature*		computeRootSig = nullptr;

		ID3D12CommandSignature*		dispatchIndirectCommandSignature = nullptr;
		ID3D12CommandSignature*		drawInstancedIndirectCommandSignature = nullptr;
		ID3D12CommandSignature*		drawIndexedInstancedIndirectCommandSignature = nullptr;

		ID3D12QueryHeap* querypool_timestamp = nullptr;
		ID3D12QueryHeap* querypool_occlusion = nullptr;
		static const size_t timestamp_query_count = 1024;
		static const size_t occlusion_query_count = 1024;
		wiContainers::ThreadSafeRingBuffer<UINT, timestamp_query_count> free_timestampqueries;
		wiContainers::ThreadSafeRingBuffer<UINT, occlusion_query_count> free_occlusionqueries;
		ID3D12Resource* querypool_timestamp_readback = nullptr;
		ID3D12Resource* querypool_occlusion_readback = nullptr;
		D3D12MA::Allocation* allocation_querypool_timestamp_readback = nullptr;
		D3D12MA::Allocation* allocation_querypool_occlusion_readback = nullptr;

		struct DescriptorAllocator
		{
			ID3D12DescriptorHeap*	heap = nullptr;
			size_t					heap_begin;
			uint32_t				itemCount;
			UINT					maxCount;
			UINT					itemSize;
			bool*					itemsAlive = nullptr;
			uint32_t				lastAlloc;
			wiSpinLock				lock;

			DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT maxCount);
			~DescriptorAllocator();

			size_t allocate();
			void clear();
			void free(wiCPUHandle descriptorHandle);
		};
		DescriptorAllocator*		RTAllocator = nullptr;
		DescriptorAllocator*		DSAllocator = nullptr;
		DescriptorAllocator*		ResourceAllocator = nullptr;
		DescriptorAllocator*		SamplerAllocator = nullptr;

		ID3D12DescriptorHeap*	null_resource_heap_CPU = nullptr;
		ID3D12DescriptorHeap*	null_sampler_heap_CPU = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE null_resource_heap_cpu_start = {};
		D3D12_CPU_DESCRIPTOR_HANDLE null_sampler_heap_cpu_start = {};
		UINT resource_descriptor_size = 0;
		UINT sampler_descriptor_size = 0;

		struct FrameResources
		{
			ID3D12Resource*					backBuffer = nullptr;
			D3D12_CPU_DESCRIPTOR_HANDLE		backBufferRTV = {};
			ID3D12CommandAllocator*			commandAllocators[COMMANDLIST_COUNT] = {};
			ID3D12CommandList*				commandLists[COMMANDLIST_COUNT] = {};

			struct DescriptorTableFrameAllocator
			{
				GraphicsDevice_DX12*	device = nullptr;
				ID3D12DescriptorHeap*	resource_heap_GPU = nullptr;
				ID3D12DescriptorHeap*	sampler_heap_GPU = nullptr;
				D3D12_CPU_DESCRIPTOR_HANDLE resource_heap_cpu_start = {};
				D3D12_GPU_DESCRIPTOR_HANDLE resource_heap_gpu_start = {};
				D3D12_CPU_DESCRIPTOR_HANDLE sampler_heap_cpu_start = {};
				D3D12_GPU_DESCRIPTOR_HANDLE sampler_heap_gpu_start = {};
				UINT ringOffset_resources = 0;
				UINT ringOffset_samplers = 0;

				struct Table
				{
					const GPUBuffer* CBV[GPU_RESOURCE_HEAP_CBV_COUNT];
					const GPUResource* SRV[GPU_RESOURCE_HEAP_SRV_COUNT];
					int SRV_index[GPU_RESOURCE_HEAP_SRV_COUNT];
					const GPUResource* UAV[GPU_RESOURCE_HEAP_UAV_COUNT];
					int UAV_index[GPU_RESOURCE_HEAP_UAV_COUNT];
					const Sampler* SAM[GPU_SAMPLER_HEAP_COUNT];

					bool dirty_resources;
					bool dirty_samplers;

					void reset()
					{
						memset(CBV, 0, sizeof(CBV));
						memset(SRV, 0, sizeof(SRV));
						memset(SRV_index, -1, sizeof(SRV_index));
						memset(UAV, 0, sizeof(UAV));
						memset(UAV_index, -1, sizeof(UAV_index));
						memset(SAM, 0, sizeof(SAM));
						dirty_resources = true;
						dirty_samplers = true;
					}

				} tables[SHADERSTAGE_COUNT];

				DescriptorTableFrameAllocator(GraphicsDevice_DX12* device, UINT maxRenameCount_resources, UINT maxRenameCount_samplers);
				~DescriptorTableFrameAllocator();

				void reset();
				void validate(CommandList cmd);
			};
			DescriptorTableFrameAllocator*		descriptors[COMMANDLIST_COUNT] = {};

			struct ResourceFrameAllocator
			{
				GraphicsDevice_DX12*	device = nullptr;
				GPUBuffer				buffer;
				D3D12MA::Allocation*	allocation = nullptr;
				uint8_t*				dataBegin = nullptr;
				uint8_t*				dataCur = nullptr;
				uint8_t*				dataEnd = nullptr;

				ResourceFrameAllocator(GraphicsDevice_DX12* device, size_t size);
				~ResourceFrameAllocator();

				uint8_t* allocate(size_t dataSize, size_t alignment);
				void clear();
				uint64_t calculateOffset(uint8_t* address);
			};
			ResourceFrameAllocator* resourceBuffer[COMMANDLIST_COUNT] = {};
		};
		FrameResources frames[BACKBUFFER_COUNT];
		FrameResources& GetFrameResources() { return frames[GetFrameCount() % BACKBUFFER_COUNT]; }
		inline ID3D12GraphicsCommandList4* GetDirectCommandList(CommandList cmd) { return static_cast<ID3D12GraphicsCommandList4*>(GetFrameResources().commandLists[cmd]); }

		struct DynamicResourceState
		{
			GPUAllocation allocation;
			bool binding[SHADERSTAGE_COUNT] = {};
		};
		std::unordered_map<const GPUBuffer*, DynamicResourceState> dynamic_constantbuffers[COMMANDLIST_COUNT];

		struct UploadBuffer
		{
			GraphicsDevice_DX12*	device = nullptr;
			ID3D12Resource*			resource = nullptr;
			D3D12MA::Allocation*	allocation = nullptr;
			uint8_t*				dataBegin = nullptr;
			uint8_t*				dataCur = nullptr;
			uint8_t*				dataEnd = nullptr;
			wiSpinLock				lock;

			UploadBuffer(GraphicsDevice_DX12* device, size_t size);
			~UploadBuffer();

			uint8_t* allocate(size_t dataSize, size_t alignment);
			void clear();
			uint64_t calculateOffset(uint8_t* address);
		};
		UploadBuffer* bufferUploader = nullptr;
		UploadBuffer* textureUploader = nullptr;

		IDXGISwapChain3*			swapChain = nullptr;

		PRIMITIVETOPOLOGY prev_pt[COMMANDLIST_COUNT] = {};

		std::unordered_map<size_t, ID3D12PipelineState*> pipelines_global;
		std::vector<std::pair<size_t, ID3D12PipelineState*>> pipelines_worker[COMMANDLIST_COUNT];
		size_t prev_pipeline_hash[COMMANDLIST_COUNT] = {};
		const RenderPass* active_renderpass[COMMANDLIST_COUNT] = {};

		std::atomic<uint8_t> commandlist_count{ 0 };
		wiContainers::ThreadSafeRingBuffer<CommandList, COMMANDLIST_COUNT> free_commandlists;
		wiContainers::ThreadSafeRingBuffer<CommandList, COMMANDLIST_COUNT> active_commandlists;

		struct DestroyItem
		{
			enum TYPE
			{
				RESOURCE,
				RESOURCEVIEW,
				RENDERTARGETVIEW,
				DEPTHSTENCILVIEW,
				SAMPLER,
				PIPELINE,
				QUERY_TIMESTAMP,
				QUERY_OCCLUSION,
			} type;
			uint64_t frame;
			wiCPUHandle handle;
		};
		std::deque<DestroyItem> destroyer;
		std::mutex destroylocker;
		inline void DeferredDestroy(const DestroyItem& item)
		{
			destroylocker.lock();
			destroyer.push_back(item);
			destroylocker.unlock();
		}
		std::unordered_map<wiCPUHandle, D3D12MA::Allocation*> mem_allocations;

	public:
		GraphicsDevice_DX12(wiWindowRegistration::window_type window, bool fullscreen = false, bool debuglayer = false);
		virtual ~GraphicsDevice_DX12();

		HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer) override;
		HRESULT CreateTexture1D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture1D *pTexture1D) override;
		HRESULT CreateTexture2D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture2D *pTexture2D) override;
		HRESULT CreateTexture3D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture3D *pTexture3D) override;
		HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements, const ShaderByteCode* shaderCode, VertexLayout *pInputLayout) override;
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

		int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, UINT firstSlice, UINT sliceCount, UINT firstMip, UINT mipCount) override;

		void DestroyResource(GPUResource* pResource) override;
		void DestroyBuffer(GPUBuffer *pBuffer) override;
		void DestroyTexture1D(Texture1D *pTexture1D) override;
		void DestroyTexture2D(Texture2D *pTexture2D) override;
		void DestroyTexture3D(Texture3D *pTexture3D) override;
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

		virtual CommandList BeginCommandList() override;

		void WaitForGPU() override;
		void ClearPipelineStateCache() override;

		void SetResolution(int width, int height) override;

		Texture2D GetBackBuffer() override;

		///////////////Thread-sensitive////////////////////////

		void RenderPassBegin(const RenderPass* renderpass, CommandList cmd) override;
		void RenderPassEnd(CommandList cmd) override;
		void BindScissorRects(UINT numRects, const Rect* rects, CommandList cmd) override;
		void BindViewports(UINT NumViewports, const ViewPort *pViewports, CommandList cmd) override;
		void BindResource(SHADERSTAGE stage, const GPUResource* resource, UINT slot, CommandList cmd, int subresource = -1) override;
		void BindResources(SHADERSTAGE stage, const GPUResource *const* resources, UINT slot, UINT count, CommandList cmd) override;
		void BindUAV(SHADERSTAGE stage, const GPUResource* resource, UINT slot, CommandList cmd, int subresource = -1) override;
		void BindUAVs(SHADERSTAGE stage, const GPUResource *const* resources, UINT slot, UINT count, CommandList cmd) override;
		void UnbindResources(UINT slot, UINT num, CommandList cmd) override;
		void UnbindUAVs(UINT slot, UINT num, CommandList cmd) override;
		void BindSampler(SHADERSTAGE stage, const Sampler* sampler, UINT slot, CommandList cmd) override;
		void BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, UINT slot, CommandList cmd) override;
		void BindVertexBuffers(const GPUBuffer *const* vertexBuffers, UINT slot, UINT count, const UINT* strides, const UINT* offsets, CommandList cmd) override;
		void BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, CommandList cmd) override;
		void BindStencilRef(UINT value, CommandList cmd) override;
		void BindBlendFactor(float r, float g, float b, float a, CommandList cmd) override;
		void BindPipelineState(const PipelineState* pso, CommandList cmd) override;
		void BindComputeShader(const ComputeShader* cs, CommandList cmd) override;
		void Draw(UINT vertexCount, UINT startVertexLocation, CommandList cmd) override;
		void DrawIndexed(UINT indexCount, UINT startIndexLocation, UINT baseVertexLocation, CommandList cmd) override;
		void DrawInstanced(UINT vertexCount, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation, CommandList cmd) override;
		void DrawIndexedInstanced(UINT indexCount, UINT instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, CommandList cmd) override;
		void DrawInstancedIndirect(const GPUBuffer* args, UINT args_offset, CommandList cmd) override;
		void DrawIndexedInstancedIndirect(const GPUBuffer* args, UINT args_offset, CommandList cmd) override;
		void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, CommandList cmd) override;
		void DispatchIndirect(const GPUBuffer* args, UINT args_offset, CommandList cmd) override;
		void CopyTexture2D(const Texture2D* pDst, const Texture2D* pSrc, CommandList cmd) override;
		void CopyTexture2D_Region(const Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, const Texture2D* pSrc, UINT srcMip, CommandList cmd) override;
		void MSAAResolve(const Texture2D* pDst, const Texture2D* pSrc, CommandList cmd) override;
		void UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize = -1) override;
		void QueryBegin(const GPUQuery *query, CommandList cmd) override;
		void QueryEnd(const GPUQuery *query, CommandList cmd) override;
		bool QueryRead(const GPUQuery* query, GPUQueryResult* result) override;
		void Barrier(const GPUBarrier* barriers, UINT numBarriers, CommandList cmd) override;

		GPUAllocation AllocateGPU(size_t dataSize, CommandList cmd) override;

		void EventBegin(const std::string& name, CommandList cmd) override;
		void EventEnd(CommandList cmd) override;
		void SetMarker(const std::string& name, CommandList cmd) override;

	};

}
