#pragma once

#if __has_include("d3d12.h")
#define WICKEDENGINE_BUILD_DX12
#endif // HAS VULKAN

#ifdef WICKEDENGINE_BUILD_DX12
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiPlatform.h"
#include "wiSpinLock.h"
#include "wiContainers.h"
#include "wiGraphicsDevice_SharedInternals.h"

#include "Utility/D3D12MemAlloc.h"

#include <dxgi1_4.h>
#include <d3d12.h>
#include <wrl/client.h> // ComPtr

#include <unordered_map>
#include <deque>
#include <atomic>
#include <mutex>


namespace wiGraphics
{
	class GraphicsDevice_DX12 : public GraphicsDevice
	{
	private:
		Microsoft::WRL::ComPtr<ID3D12Device> device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> directQueue;
		Microsoft::WRL::ComPtr<ID3D12Fence> frameFence;
		HANDLE frameFenceEvent;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> copyQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> copyAllocator;
		Microsoft::WRL::ComPtr<ID3D12CommandList> copyCommandList;
		Microsoft::WRL::ComPtr<ID3D12Fence> copyFence;
		HANDLE copyFenceEvent;
		UINT64 copyFenceValue;
		wiSpinLock copyQueueLock;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> graphicsRootSig;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSig;

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawInstancedIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawIndexedInstancedIndirectCommandSignature;

		Microsoft::WRL::ComPtr<ID3D12QueryHeap> querypool_timestamp;
		Microsoft::WRL::ComPtr<ID3D12QueryHeap> querypool_occlusion;
		static const size_t timestamp_query_count = 1024;
		static const size_t occlusion_query_count = 1024;
		Microsoft::WRL::ComPtr<ID3D12Resource> querypool_timestamp_readback;
		Microsoft::WRL::ComPtr<ID3D12Resource> querypool_occlusion_readback;
		D3D12MA::Allocation* allocation_querypool_timestamp_readback = nullptr;
		D3D12MA::Allocation* allocation_querypool_occlusion_readback = nullptr;

		struct DescriptorAllocator
		{
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
			size_t					heap_begin;
			uint32_t				itemCount;
			uint32_t				maxCount;
			uint32_t				itemSize;
			bool*					itemsAlive = nullptr;
			uint32_t				lastAlloc;
			wiSpinLock				lock;

			void init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t maxCount);
			~DescriptorAllocator();

			D3D12_CPU_DESCRIPTOR_HANDLE allocate();
			void clear();
			void free(D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle);
		};

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> null_resource_heap_CPU;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> null_sampler_heap_CPU;
		D3D12_CPU_DESCRIPTOR_HANDLE null_resource_heap_cpu_start = {};
		D3D12_CPU_DESCRIPTOR_HANDLE null_sampler_heap_cpu_start = {};
		uint32_t resource_descriptor_size = 0;
		uint32_t sampler_descriptor_size = 0;

		struct FrameResources
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
			D3D12_CPU_DESCRIPTOR_HANDLE		backBufferRTV = {};
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[COMMANDLIST_COUNT];
			Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[COMMANDLIST_COUNT];

			struct DescriptorTableFrameAllocator
			{
				GraphicsDevice_DX12*	device = nullptr;
				struct DescriptorHeap
				{
					D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
					Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_GPU;
					D3D12_CPU_DESCRIPTOR_HANDLE start_cpu = {};
					D3D12_GPU_DESCRIPTOR_HANDLE start_gpu = {};
					uint32_t ringOffset = 0;
				};
				std::vector<DescriptorHeap> heaps_resource;
				std::vector<DescriptorHeap> heaps_sampler;
				size_t currentheap_resource = 0;
				size_t currentheap_sampler = 0;
				bool heaps_bound = false;

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

				void init(GraphicsDevice_DX12* device);

				void reset();
				void validate(CommandList cmd);
				void create_or_bind_heaps_on_demand(CommandList cmd);
			};
			DescriptorTableFrameAllocator descriptors[COMMANDLIST_COUNT];

			struct ResourceFrameAllocator
			{
				GraphicsDevice_DX12*	device = nullptr;
				GPUBuffer				buffer;
				uint8_t*				dataBegin = nullptr;
				uint8_t*				dataCur = nullptr;
				uint8_t*				dataEnd = nullptr;

				void init(GraphicsDevice_DX12* device, size_t size);

				uint8_t* allocate(size_t dataSize, size_t alignment);
				void clear();
				uint64_t calculateOffset(uint8_t* address);
			};
			ResourceFrameAllocator resourceBuffer[COMMANDLIST_COUNT];
		};
		FrameResources frames[BACKBUFFER_COUNT];
		FrameResources& GetFrameResources() { return frames[GetFrameCount() % BACKBUFFER_COUNT]; }
		inline ID3D12GraphicsCommandList4* GetDirectCommandList(CommandList cmd) { return static_cast<ID3D12GraphicsCommandList4*>(GetFrameResources().commandLists[cmd].Get()); }

		struct DynamicResourceState
		{
			GPUAllocation allocation;
			bool binding[SHADERSTAGE_COUNT] = {};
		};
		std::unordered_map<const GPUBuffer*, DynamicResourceState> dynamic_constantbuffers[COMMANDLIST_COUNT];

		struct UploadBuffer
		{
			GraphicsDevice_DX12* device = nullptr;
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			D3D12MA::Allocation*	allocation = nullptr;
			uint8_t*				dataBegin = nullptr;
			uint8_t*				dataCur = nullptr;
			uint8_t*				dataEnd = nullptr;
			wiSpinLock				lock;

			void init(GraphicsDevice_DX12* device, size_t size);
			~UploadBuffer()
			{
				if (allocation) allocation->Release();
			}

			uint8_t* allocate(size_t dataSize, size_t alignment);
			void clear();
			uint64_t calculateOffset(uint8_t* address);
		};
		UploadBuffer bufferUploader;
		UploadBuffer textureUploader;

		Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;

		PRIMITIVETOPOLOGY prev_pt[COMMANDLIST_COUNT] = {};

		std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelines_global;
		std::vector<std::pair<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>>> pipelines_worker[COMMANDLIST_COUNT];
		size_t prev_pipeline_hash[COMMANDLIST_COUNT] = {};
		const RenderPass* active_renderpass[COMMANDLIST_COUNT] = {};

		std::atomic<uint8_t> commandlist_count{ 0 };
		wiContainers::ThreadSafeRingBuffer<CommandList, COMMANDLIST_COUNT> free_commandlists;
		wiContainers::ThreadSafeRingBuffer<CommandList, COMMANDLIST_COUNT> active_commandlists;

	public:
		GraphicsDevice_DX12(wiPlatform::window_type window, bool fullscreen = false, bool debuglayer = false);
		virtual ~GraphicsDevice_DX12();

		bool CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer) override;
		bool CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture) override;
		bool CreateInputLayout(const InputLayoutDesc *pInputElementDescs, uint32_t NumElements, const Shader* shader, InputLayout *pInputLayout) override;
		bool CreateShader(SHADERSTAGE stage, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader) override;
		bool CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState) override;
		bool CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState) override;
		bool CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState) override;
		bool CreateSampler(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) override;
		bool CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery) override;
		bool CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso) override;
		bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) override;

		int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) override;

		bool DownloadResource(const GPUResource* resourceToDownload, const GPUResource* resourceDest, void* dataDest) override;

		void SetName(GPUResource* pResource, const char* name) override;

		void PresentBegin(CommandList cmd) override;
		void PresentEnd(CommandList cmd) override;

		virtual CommandList BeginCommandList() override;

		void WaitForGPU() override;
		void ClearPipelineStateCache() override;

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
		void BindComputeShader(const Shader* cs, CommandList cmd) override;
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
		void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) override;

		GPUAllocation AllocateGPU(size_t dataSize, CommandList cmd) override;

		void EventBegin(const char* name, CommandList cmd) override;
		void EventEnd(CommandList cmd) override;
		void SetMarker(const char* name, CommandList cmd) override;


		struct AllocationHandler
		{
			D3D12MA::Allocator* allocator = nullptr;
			Microsoft::WRL::ComPtr<ID3D12Device> device;
			uint64_t framecount = 0;
			std::mutex destroylocker;
			std::deque<std::pair<D3D12MA::Allocation*, uint64_t>> destroyer_allocations;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint64_t>> destroyer_resources;
			std::deque<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, uint64_t>> destroyer_resourceviews;
			std::deque<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, uint64_t>> destroyer_rtvs;
			std::deque<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, uint64_t>> destroyer_dsvs;
			std::deque<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, uint64_t>> destroyer_samplers;
			std::deque<std::pair<uint32_t, uint64_t>> destroyer_queries_timestamp;
			std::deque<std::pair<uint32_t, uint64_t>> destroyer_queries_occlusion;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12PipelineState>, uint64_t>> destroyer_pipelines;

			DescriptorAllocator RTAllocator;
			DescriptorAllocator DSAllocator;
			DescriptorAllocator ResourceAllocator;
			DescriptorAllocator SamplerAllocator;
			wiContainers::ThreadSafeRingBuffer<uint32_t, timestamp_query_count> free_timestampqueries;
			wiContainers::ThreadSafeRingBuffer<uint32_t, occlusion_query_count> free_occlusionqueries;

			~AllocationHandler()
			{
				Update(~0, 0); // destroy all remaining
				if(allocator) allocator->Release();
			}

			// Deferred destroy of resources that the GPU is already finished with:
			void Update(uint64_t FRAMECOUNT, uint32_t BACKBUFFER_COUNT)
			{
				destroylocker.lock();
				framecount = FRAMECOUNT;
				while (!destroyer_allocations.empty())
				{
					if (destroyer_allocations.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
					{
						auto item = destroyer_allocations.front();
						destroyer_allocations.pop_front();
						item.first->Release();
					}
					else
					{
						break;
					}
				}
				while (!destroyer_resources.empty())
				{
					if (destroyer_resources.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
					{
						destroyer_resources.pop_front();
						// comptr auto delete
					}
					else
					{
						break;
					}
				}
				while (!destroyer_resourceviews.empty())
				{
					if (destroyer_resourceviews.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
					{
						auto item = destroyer_resourceviews.front();
						destroyer_resourceviews.pop_front();
						ResourceAllocator.free(item.first);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_rtvs.empty())
				{
					if (destroyer_rtvs.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
					{
						auto item = destroyer_rtvs.front();
						destroyer_rtvs.pop_front();
						RTAllocator.free(item.first);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_dsvs.empty())
				{
					if (destroyer_dsvs.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
					{
						auto item = destroyer_dsvs.front();
						destroyer_dsvs.pop_front();
						DSAllocator.free(item.first);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_samplers.empty())
				{
					if (destroyer_samplers.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
					{
						auto item = destroyer_samplers.front();
						destroyer_samplers.pop_front();
						SamplerAllocator.free(item.first);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_queries_occlusion.empty())
				{
					if (destroyer_queries_occlusion.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
					{
						auto item = destroyer_queries_occlusion.front();
						destroyer_queries_occlusion.pop_front();
						free_occlusionqueries.push_back(item.first);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_queries_timestamp.empty())
				{
					if (destroyer_queries_timestamp.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
					{
						auto item = destroyer_queries_timestamp.front();
						destroyer_queries_timestamp.pop_front();
						free_timestampqueries.push_back(item.first);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_pipelines.empty())
				{
					if (destroyer_pipelines.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
					{
						destroyer_pipelines.pop_front();
						// comptr auto delete
					}
					else
					{
						break;
					}
				}
				destroylocker.unlock();
			}
		};
		std::shared_ptr<AllocationHandler> allocationhandler;

	};

}

#endif // WICKEDENGINE_BUILD_DX12
