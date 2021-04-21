#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"

#ifdef _WIN32
#define WICKEDENGINE_BUILD_DX12
#endif // _WIN32

#ifdef WICKEDENGINE_BUILD_DX12
#include "wiGraphicsDevice.h"
#include "wiGraphicsDevice_SharedInternals.h"

#include <dxgi1_6.h>
#include <wrl/client.h> // ComPtr

#include "Utility/dx12/d3d12.h"
#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED
#include "Utility/D3D12MemAlloc.h"

#include <unordered_map>
#include <deque>
#include <atomic>
#include <mutex>


namespace wiGraphics
{
	class GraphicsDevice_DX12 : public GraphicsDevice
	{
	protected:
		Microsoft::WRL::ComPtr<ID3D12Device5> device;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter;
		Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> graphicsQueue;

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawInstancedIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawIndexedInstancedIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchMeshIndirectCommandSignature;

		D3D12_FEATURE_DATA_D3D12_OPTIONS features_0;
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 features_5;
		D3D12_FEATURE_DATA_D3D12_OPTIONS6 features_6;
		D3D12_FEATURE_DATA_D3D12_OPTIONS7 features_7;

		uint32_t rtv_descriptor_size = 0;
		uint32_t dsv_descriptor_size = 0;
		uint32_t resource_descriptor_size = 0;
		uint32_t sampler_descriptor_size = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE nullCBV = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSAM = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSRV_buffer = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSRV_texture1d = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSRV_texture1darray = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSRV_texture2d = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSRV_texture2darray = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSRV_texturecube = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSRV_texturecubearray = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSRV_texture3d = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSRV_accelerationstructure = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullUAV_buffer = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullUAV_texture1d = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullUAV_texture1darray = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullUAV_texture2d = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullUAV_texture2darray = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullUAV_texture3d = {};

		std::vector<D3D12_STATIC_SAMPLER_DESC> common_samplers;

		struct CopyAllocator
		{
			Microsoft::WRL::ComPtr<ID3D12Device5> device;
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
			Microsoft::WRL::ComPtr<ID3D12Fence> fence;
			uint64_t fenceValue = 0;
			std::mutex locker;

			struct CopyCMD
			{
				Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
				uint64_t target = 0;
				GPUBuffer uploadbuffer;
			};
			std::vector<CopyCMD> freelist;
			std::vector<CopyCMD> worklist;
			uint64_t submit_wait = 0;

			void Create(Microsoft::WRL::ComPtr<ID3D12Device5> device)
			{
				this->device = device;

				D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
				copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
				copyQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
				copyQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
				copyQueueDesc.NodeMask = 0;
				HRESULT hr = device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&queue));
				assert(SUCCEEDED(hr));

				hr = device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&fence));
				assert(SUCCEEDED(hr));
			}
			void Destroy()
			{
				uint64_t value = ++fenceValue;
				HRESULT hr = queue->Signal(fence.Get(), value);
				assert(SUCCEEDED(hr));
				hr = fence->SetEventOnCompletion(value, nullptr);
				assert(SUCCEEDED(hr));
			}

			CopyCMD allocate(uint32_t staging_size = 0)
			{
				locker.lock();

				// create a new command list if there are no free ones:
				if (freelist.empty())
				{
					CopyCMD cmd;

					HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&cmd.commandAllocator));
					assert(SUCCEEDED(hr));
					hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, cmd.commandAllocator.Get(), nullptr, IID_PPV_ARGS(&cmd.commandList));
					assert(SUCCEEDED(hr));

					hr = static_cast<ID3D12GraphicsCommandList*>(cmd.commandList.Get())->Close();
					assert(SUCCEEDED(hr));

					freelist.push_back(cmd);
				}

				CopyCMD cmd = freelist.back();
				if (cmd.uploadbuffer.desc.ByteWidth < staging_size)
				{
					// Try to search for a staging buffer that is good:
					for (size_t i = 0; i < freelist.size(); ++i)
					{
						if (freelist[i].uploadbuffer.desc.ByteWidth >= staging_size)
						{
							cmd = freelist[i];
							std::swap(freelist[i], freelist.back());
							break;
						}
					}
				}
				freelist.pop_back();
				locker.unlock();

				// begin command list in valid state:
				HRESULT hr = cmd.commandAllocator->Reset();
				assert(SUCCEEDED(hr));
				hr = static_cast<ID3D12GraphicsCommandList*>(cmd.commandList.Get())->Reset(cmd.commandAllocator.Get(), nullptr);
				assert(SUCCEEDED(hr));

				return cmd;
			}
			void submit(CopyCMD cmd)
			{
				static_cast<ID3D12GraphicsCommandList*>(cmd.commandList.Get())->Close();
				ID3D12CommandList* commandlists[] = {
					cmd.commandList.Get()
				};
				queue->ExecuteCommandLists(1, commandlists);

				locker.lock();
				cmd.target = ++fenceValue;
				worklist.push_back(cmd);
				submit_wait = std::max(submit_wait, cmd.target);
				locker.unlock();
			}
			uint64_t flush()
			{
				locker.lock();

				queue->Signal(fence.Get(), submit_wait);

				// free up the finished command lists:
				uint64_t completed_fence_value = fence->GetCompletedValue();
				for (size_t i = 0; i < worklist.size(); ++i)
				{
					if (worklist[i].target <= completed_fence_value)
					{
						freelist.push_back(worklist[i]);
						worklist[i] = worklist.back();
						worklist.pop_back();
						i--;
					}
				}

				uint64_t value = submit_wait;
				submit_wait = 0;
				locker.unlock();
				return value;
			}
		};
		mutable CopyAllocator copyAllocator;

		struct FrameResources
		{
			Microsoft::WRL::ComPtr<ID3D12Fence> fence;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[COMMANDLIST_COUNT];
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandLists[COMMANDLIST_COUNT];

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
		FrameResources frames[BUFFERCOUNT];
		FrameResources& GetFrameResources() { return frames[GetFrameCount() % BUFFERCOUNT]; }
		inline ID3D12GraphicsCommandList6* GetCommandList(CommandList cmd) { return (ID3D12GraphicsCommandList6*)GetFrameResources().commandLists[cmd].Get(); }

		struct DescriptorBinder
		{
			GraphicsDevice_DX12* device = nullptr;
			uint32_t ringOffset_res = 0;
			uint32_t ringOffset_sam = 0;
			bool dirty_res = false;
			bool dirty_sam = false;

			const GPUBuffer* CBV[GPU_RESOURCE_HEAP_CBV_COUNT];
			const GPUResource* SRV[GPU_RESOURCE_HEAP_SRV_COUNT];
			int SRV_index[GPU_RESOURCE_HEAP_SRV_COUNT];
			const GPUResource* UAV[GPU_RESOURCE_HEAP_UAV_COUNT];
			int UAV_index[GPU_RESOURCE_HEAP_UAV_COUNT];
			const Sampler* SAM[GPU_SAMPLER_HEAP_COUNT];

			uint32_t dirty_root_cbvs_gfx = 0; // bitmask
			uint32_t dirty_root_cbvs_compute = 0; // bitmask

			struct DescriptorHandles
			{
				D3D12_GPU_DESCRIPTOR_HANDLE sampler_handle = {};
				D3D12_GPU_DESCRIPTOR_HANDLE resource_handle = {};
			};

			void init(GraphicsDevice_DX12* device);

			void reset();
			void validate(bool graphics, CommandList cmd);
		};
		DescriptorBinder descriptors[COMMANDLIST_COUNT];

		std::vector<D3D12_RESOURCE_BARRIER> frame_barriers[COMMANDLIST_COUNT];

		PRIMITIVETOPOLOGY prev_pt[COMMANDLIST_COUNT] = {};

		mutable std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootsignature_cache;
		mutable std::mutex rootsignature_cache_mutex;

		std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelines_global;
		std::vector<std::pair<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>>> pipelines_worker[COMMANDLIST_COUNT];
		size_t prev_pipeline_hash[COMMANDLIST_COUNT] = {};
		const PipelineState* active_pso[COMMANDLIST_COUNT] = {};
		const Shader* active_cs[COMMANDLIST_COUNT] = {};
		const RaytracingPipelineState* active_rt[COMMANDLIST_COUNT] = {};
		const ID3D12RootSignature* active_rootsig_graphics[COMMANDLIST_COUNT] = {};
		const ID3D12RootSignature* active_rootsig_compute[COMMANDLIST_COUNT] = {};
		const RenderPass* active_renderpass[COMMANDLIST_COUNT] = {};
		SHADING_RATE prev_shadingrate[COMMANDLIST_COUNT] = {};
		std::vector<const SwapChain*> swapchains[COMMANDLIST_COUNT];
		Microsoft::WRL::ComPtr<ID3D12Resource> active_backbuffer[COMMANDLIST_COUNT];

		struct DeferredPushConstantData
		{
			uint8_t data[128];
			uint32_t size;
		};
		DeferredPushConstantData pushconstants[COMMANDLIST_COUNT] = {};

		bool dirty_pso[COMMANDLIST_COUNT] = {};
		void pso_validate(CommandList cmd);

		void query_flush(CommandList cmd);
		void barrier_flush(CommandList cmd);
		void predraw(CommandList cmd);
		void predispatch(CommandList cmd);

		struct QueryResolver
		{
			const GPUQueryHeap* heap = nullptr;
			uint32_t index = 0;
			uint32_t count = 0;
		};
		std::vector<QueryResolver> query_resolves[COMMANDLIST_COUNT];

		std::atomic<CommandList> cmd_count{ 0 };

	public:
		GraphicsDevice_DX12(bool debuglayer = false, bool gpuvalidation = false);
		virtual ~GraphicsDevice_DX12();

		bool CreateSwapChain(const SwapChainDesc* pDesc, wiPlatform::window_type window, SwapChain* swapChain) const override;
		bool CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer) const override;
		bool CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture) const override;
		bool CreateShader(SHADERSTAGE stage, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader) const override;
		bool CreateSampler(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) const override;
		bool CreateQueryHeap(const GPUQueryHeapDesc* pDesc, GPUQueryHeap* pQueryHeap) const override;
		bool CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso) const override;
		bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) const override;
		bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh) const override;
		bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso) const override;
		
		int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) const override;
		int CreateSubresource(GPUBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size = ~0) const override;

		int GetDescriptorIndex(const GPUResource* resource, SUBRESOURCE_TYPE type, int subresource = -1) const override;
		int GetDescriptorIndex(const Sampler* sampler) const override;

		void WriteShadingRateValue(SHADING_RATE rate, void* dest) const override;
		void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const override;
		void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const override;
		
		void Map(const GPUResource* resource, Mapping* mapping) const override;
		void Unmap(const GPUResource* resource) const override;
		void QueryRead(const GPUQueryHeap* heap, uint32_t index, uint32_t count, uint64_t* results) const override;

		void SetCommonSampler(const StaticSampler* sam) override;

		void SetName(GPUResource* pResource, const char* name) override;

		CommandList BeginCommandList() override;
		void SubmitCommandLists() override;

		void WaitForGPU() const override;
		void ClearPipelineStateCache() override;

		SHADERFORMAT GetShaderFormat() const override { return SHADERFORMAT_HLSL6; }

		Texture GetBackBuffer(const SwapChain* swapchain) const override;

		///////////////Thread-sensitive////////////////////////

		void RenderPassBegin(const SwapChain* swapchain, CommandList cmd) override;
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
		void BindShadingRate(SHADING_RATE rate, CommandList cmd) override;
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
		void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) override;
		void DispatchMeshIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd) override;
		void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) override;
		void UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize = -1) override;
		void QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd) override;
		void QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd) override;
		void QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd) override;
		void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) override;
		void BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src = nullptr) override;
		void BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd) override;
		void DispatchRays(const DispatchRaysDesc* desc, CommandList cmd) override;
		void PushConstants(const void* data, uint32_t size, CommandList cmd) override;

		GPUAllocation AllocateGPU(size_t dataSize, CommandList cmd) override;

		void EventBegin(const char* name, CommandList cmd) override;
		void EventEnd(CommandList cmd) override;
		void SetMarker(const char* name, CommandList cmd) override;



		struct DescriptorHeap
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_GPU;
			D3D12_CPU_DESCRIPTOR_HANDLE start_cpu = {};
			D3D12_GPU_DESCRIPTOR_HANDLE start_gpu = {};

			// CPU status:
			std::atomic<uint64_t> allocationOffset{ 0 };

			// GPU status:
			Microsoft::WRL::ComPtr<ID3D12Fence> fence;
			uint64_t fenceValue = 0;
			uint64_t cached_completedValue = 0;
		};
		DescriptorHeap descriptorheap_res;
		DescriptorHeap descriptorheap_sam;

		struct AllocationHandler
		{
			D3D12MA::Allocator* allocator = nullptr;
			Microsoft::WRL::ComPtr<ID3D12Device> device;
			uint64_t framecount = 0;
			std::mutex destroylocker;

			struct DescriptorAllocator
			{
				GraphicsDevice_DX12* device = nullptr;
				std::mutex locker;
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> heaps;
				uint32_t descriptor_size = 0;
				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> freelist;

				void init(GraphicsDevice_DX12* device, D3D12_DESCRIPTOR_HEAP_TYPE type)
				{
					this->device = device;
					desc.Type = type;
					desc.NumDescriptors = 1024;
					descriptor_size = device->device->GetDescriptorHandleIncrementSize(type);
				}
				void block_allocate()
				{
					heaps.emplace_back();
					HRESULT hr = device->device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heaps.back()));
					assert(SUCCEEDED(hr));
					D3D12_CPU_DESCRIPTOR_HANDLE heap_start = heaps.back()->GetCPUDescriptorHandleForHeapStart();
					for (UINT i = 0; i < desc.NumDescriptors; ++i)
					{
						D3D12_CPU_DESCRIPTOR_HANDLE handle = heap_start;
						handle.ptr += i * descriptor_size;
						freelist.push_back(handle);
					}
				}
				D3D12_CPU_DESCRIPTOR_HANDLE allocate()
				{
					locker.lock();
					if (freelist.empty())
					{
						block_allocate();
					}
					assert(!freelist.empty());
					D3D12_CPU_DESCRIPTOR_HANDLE handle = freelist.back();
					freelist.pop_back();
					locker.unlock();
					return handle;
				}
				void free(D3D12_CPU_DESCRIPTOR_HANDLE index)
				{
					locker.lock();
					freelist.push_back(index);
					locker.unlock();
				}
			};
			DescriptorAllocator descriptors_res;
			DescriptorAllocator descriptors_sam;
			DescriptorAllocator descriptors_rtv;
			DescriptorAllocator descriptors_dsv;

			std::vector<int> free_bindless_res;
			std::vector<int> free_bindless_sam;

			std::deque<std::pair<D3D12MA::Allocation*, uint64_t>> destroyer_allocations;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint64_t>> destroyer_resources;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12QueryHeap>, uint64_t>> destroyer_queryheaps;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12PipelineState>, uint64_t>> destroyer_pipelines;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12RootSignature>, uint64_t>> destroyer_rootSignatures;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12StateObject>, uint64_t>> destroyer_stateobjects;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>, uint64_t>> destroyer_descriptorHeaps;
			std::deque<std::pair<int, uint64_t>> destroyer_bindless_res;
			std::deque<std::pair<int, uint64_t>> destroyer_bindless_sam;

			~AllocationHandler()
			{
				Update(~0, 0); // destroy all remaining
				if (allocator) allocator->Release();
			}

			// Deferred destroy of resources that the GPU is already finished with:
			void Update(uint64_t FRAMECOUNT, uint32_t BUFFERCOUNT)
			{
				destroylocker.lock();
				framecount = FRAMECOUNT;
				while (!destroyer_allocations.empty())
				{
					if (destroyer_allocations.front().second + BUFFERCOUNT < FRAMECOUNT)
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
					if (destroyer_resources.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						destroyer_resources.pop_front();
						// comptr auto delete
					}
					else
					{
						break;
					}
				}
				while (!destroyer_queryheaps.empty())
				{
					if (destroyer_queryheaps.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						destroyer_queryheaps.pop_front();
						// comptr auto delete
					}
					else
					{
						break;
					}
				}
				while (!destroyer_pipelines.empty())
				{
					if (destroyer_pipelines.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						destroyer_pipelines.pop_front();
						// comptr auto delete
					}
					else
					{
						break;
					}
				}
				while (!destroyer_rootSignatures.empty())
				{
					if (destroyer_rootSignatures.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						destroyer_rootSignatures.pop_front();
						// comptr auto delete
					}
					else
					{
						break;
					}
				}
				while (!destroyer_stateobjects.empty())
				{
					if (destroyer_stateobjects.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						destroyer_stateobjects.pop_front();
						// comptr auto delete
					}
					else
					{
						break;
					}
				}
				while (!destroyer_descriptorHeaps.empty())
				{
					if (destroyer_descriptorHeaps.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						destroyer_descriptorHeaps.pop_front();
						// comptr auto delete
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bindless_res.empty())
				{
					if (destroyer_bindless_res.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						int index = destroyer_bindless_res.front().first;
						destroyer_bindless_res.pop_front();
						free_bindless_res.push_back(index);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bindless_sam.empty())
				{
					if (destroyer_bindless_sam.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						int index = destroyer_bindless_sam.front().first;
						destroyer_bindless_sam.pop_front();
						free_bindless_sam.push_back(index);
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
