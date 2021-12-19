#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"

#ifdef _WIN32
#define WICKEDENGINE_BUILD_DX12
#endif // _WIN32

#ifdef WICKEDENGINE_BUILD_DX12
#include "wiGraphicsDevice.h"
#include "wiUnorderedMap.h"
#include "wiVector.h"

#include <dxgi1_6.h>
#include <wrl/client.h> // ComPtr

#include "Utility/dx12/d3d12.h"
#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED
#include "Utility/D3D12MemAlloc.h"

#include <deque>
#include <atomic>
#include <mutex>

namespace wi::graphics
{
	class GraphicsDevice_DX12 final : public GraphicsDevice
	{
	protected:
		Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter;
		Microsoft::WRL::ComPtr<ID3D12Device5> device;

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawInstancedIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawIndexedInstancedIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchMeshIndirectCommandSignature;

		bool tearingSupported = false;
		bool additionalShadingRatesSupported = false;

		uint32_t rtv_descriptor_size = 0;
		uint32_t dsv_descriptor_size = 0;
		uint32_t resource_descriptor_size = 0;
		uint32_t sampler_descriptor_size = 0;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> nulldescriptorheap_cbv_srv_uav;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> nulldescriptorheap_sampler;
		D3D12_CPU_DESCRIPTOR_HANDLE nullCBV = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSRV = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullUAV = {};
		D3D12_CPU_DESCRIPTOR_HANDLE nullSAM = {};

		struct CommandQueue
		{
			D3D12_COMMAND_QUEUE_DESC desc = {};
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
			Microsoft::WRL::ComPtr<ID3D12Fence> fence;
			ID3D12CommandList* submit_cmds[COMMANDLIST_COUNT] = {};
			uint32_t submit_count = 0;
		} queues[QUEUE_COUNT];

		struct CopyAllocator
		{
			GraphicsDevice_DX12* device = nullptr;
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
			std::mutex locker;

			struct CopyCMD
			{
				Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
				Microsoft::WRL::ComPtr<ID3D12Fence> fence;
				GPUBuffer uploadbuffer;
			};
			wi::vector<CopyCMD> freelist;

			void init(GraphicsDevice_DX12* device);
			void destroy();
			CopyCMD allocate(uint64_t staging_size);
			void submit(CopyCMD cmd);
		};
		mutable CopyAllocator copyAllocator;

		struct FrameResources
		{
			Microsoft::WRL::ComPtr<ID3D12Fence> fence[QUEUE_COUNT];
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[COMMANDLIST_COUNT][QUEUE_COUNT];
		};
		FrameResources frames[BUFFERCOUNT];
		FrameResources& GetFrameResources() { return frames[GetBufferIndex()]; }

		struct CommandListMetadata
		{
			QUEUE_TYPE queue = {};
			wi::vector<CommandList> waits;
		} cmd_meta[COMMANDLIST_COUNT];

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandLists[COMMANDLIST_COUNT][QUEUE_COUNT];
		inline ID3D12GraphicsCommandList6* GetCommandList(CommandList::index_type cmd)
		{
			return (ID3D12GraphicsCommandList6*)commandLists[cmd][cmd_meta[cmd].queue].Get();
		}

		struct DescriptorBinder
		{
			DescriptorBindingTable table;
			GraphicsDevice_DX12* device = nullptr;

			const void* optimizer_graphics = nullptr;
			uint64_t dirty_graphics = 0ull; // 1 dirty bit flag per root parameter
			const void* optimizer_compute = nullptr;
			uint64_t dirty_compute = 0ull; // 1 dirty bit flag per root parameter

			void init(GraphicsDevice_DX12* device);
			void reset();
			void flush(bool graphics, CommandList cmd);
		};
		DescriptorBinder binders[COMMANDLIST_COUNT];

		wi::vector<D3D12_RESOURCE_BARRIER> frame_barriers[COMMANDLIST_COUNT];

		D3D_PRIMITIVE_TOPOLOGY prev_pt[COMMANDLIST_COUNT] = {};

		mutable wi::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootsignature_cache;
		mutable std::mutex rootsignature_cache_mutex;

		wi::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelines_global;
		wi::vector<std::pair<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>>> pipelines_worker[COMMANDLIST_COUNT];
		size_t prev_pipeline_hash[COMMANDLIST_COUNT] = {};
		const PipelineState* active_pso[COMMANDLIST_COUNT] = {};
		const Shader* active_cs[COMMANDLIST_COUNT] = {};
		const RaytracingPipelineState* active_rt[COMMANDLIST_COUNT] = {};
		const ID3D12RootSignature* active_rootsig_graphics[COMMANDLIST_COUNT] = {};
		const ID3D12RootSignature* active_rootsig_compute[COMMANDLIST_COUNT] = {};
		const RenderPass* active_renderpass[COMMANDLIST_COUNT] = {};
		ShadingRate prev_shadingrate[COMMANDLIST_COUNT] = {};
		wi::vector<const SwapChain*> swapchains[COMMANDLIST_COUNT];
		Microsoft::WRL::ComPtr<ID3D12Resource> active_backbuffer[COMMANDLIST_COUNT];

		bool dirty_pso[COMMANDLIST_COUNT] = {};
		void pso_validate(CommandList cmd);

		void predraw(CommandList cmd);
		void predispatch(CommandList cmd);

		std::atomic<CommandList::index_type> cmd_count{ 0 };

	public:
		GraphicsDevice_DX12(bool debuglayer = false, bool gpuvalidation = false);
		virtual ~GraphicsDevice_DX12();

		bool CreateSwapChain(const SwapChainDesc* pDesc, wi::platform::window_type window, SwapChain* swapChain) const override;
		bool CreateBuffer(const GPUBufferDesc *pDesc, const void* pInitialData, GPUBuffer *pBuffer) const override;
		bool CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture) const override;
		bool CreateShader(ShaderStage stage, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader) const override;
		bool CreateSampler(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) const override;
		bool CreateQueryHeap(const GPUQueryHeapDesc* pDesc, GPUQueryHeap* pQueryHeap) const override;
		bool CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso) const override;
		bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) const override;
		bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh) const override;
		bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso) const override;
		
		int CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) const override;
		int CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size = ~0) const override;

		int GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource = -1) const override;
		int GetDescriptorIndex(const Sampler* sampler) const override;

		void WriteShadingRateValue(ShadingRate rate, void* dest) const override;
		void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const override;
		void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const override;

		void SetName(GPUResource* pResource, const char* name) override;

		CommandList BeginCommandList(QUEUE_TYPE queue = QUEUE_GRAPHICS) override;
		void SubmitCommandLists() override;

		void WaitForGPU() const override;
		void ClearPipelineStateCache() override;
		size_t GetActivePipelineCount() const override { return pipelines_global.size(); }

		ShaderFormat GetShaderFormat() const override { return ShaderFormat::HLSL6; }

		Texture GetBackBuffer(const SwapChain* swapchain) const override;

		ColorSpace GetSwapChainColorSpace(const SwapChain* swapchain) const override;
		bool IsSwapChainSupportsHDR(const SwapChain* swapchain) const override;

		///////////////Thread-sensitive////////////////////////

		void WaitCommandList(CommandList cmd, CommandList wait_for) override;
		void RenderPassBegin(const SwapChain* swapchain, CommandList cmd) override;
		void RenderPassBegin(const RenderPass* renderpass, CommandList cmd) override;
		void RenderPassEnd(CommandList cmd) override;
		void BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) override;
		void BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd) override;
		void BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) override;
		void BindResources(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd) override;
		void BindUAV(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) override;
		void BindUAVs(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd) override;
		void BindSampler(const Sampler* sampler, uint32_t slot, CommandList cmd) override;
		void BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset = 0ull) override;
		void BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint64_t* offsets, CommandList cmd) override;
		void BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd) override;
		void BindStencilRef(uint32_t value, CommandList cmd) override;
		void BindBlendFactor(float r, float g, float b, float a, CommandList cmd) override;
		void BindShadingRate(ShadingRate rate, CommandList cmd) override;
		void BindPipelineState(const PipelineState* pso, CommandList cmd) override;
		void BindComputeShader(const Shader* cs, CommandList cmd) override;
		void BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd) override;
		void Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) override;
		void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd) override;
		void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) override;
		void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) override;
		void DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) override;
		void DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) override;
		void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) override;
		void DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) override;
		void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) override;
		void DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) override;
		void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) override;
		void CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd) override;
		void QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd) override;
		void QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd) override;
		void QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd) override;
		void QueryReset(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd) override {}
		void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) override;
		void BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src = nullptr) override;
		void BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd) override;
		void DispatchRays(const DispatchRaysDesc* desc, CommandList cmd) override;
		void PushConstants(const void* data, uint32_t size, CommandList cmd, uint32_t offset = 0) override;
		void PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd) override;
		void PredicationEnd(CommandList cmd) override;

		void EventBegin(const char* name, CommandList cmd) override;
		void EventEnd(CommandList cmd) override;
		void SetMarker(const char* name, CommandList cmd) override;

		const RenderPass* GetCurrentRenderPass(CommandList cmd) const override;


		struct DescriptorHeapGPU
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

			void SignalGPU(ID3D12CommandQueue* queue)
			{
				// Descriptor heaps' progress is recorded by the GPU:
				fenceValue = allocationOffset.load();
				HRESULT hr = queue->Signal(fence.Get(), fenceValue);
				assert(SUCCEEDED(hr));
				cached_completedValue = fence->GetCompletedValue();
			}
		};
		DescriptorHeapGPU descriptorheap_res;
		DescriptorHeapGPU descriptorheap_sam;

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
				wi::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> heaps;
				uint32_t descriptor_size = 0;
				wi::vector<D3D12_CPU_DESCRIPTOR_HANDLE> freelist;

				void init(GraphicsDevice_DX12* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptorsPerBlock)
				{
					this->device = device;
					desc.Type = type;
					desc.NumDescriptors = numDescriptorsPerBlock;
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

			wi::vector<int> free_bindless_res;
			wi::vector<int> free_bindless_sam;

			std::deque<std::pair<D3D12MA::Allocation*, uint64_t>> destroyer_allocations;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint64_t>> destroyer_resources;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12QueryHeap>, uint64_t>> destroyer_queryheaps;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12PipelineState>, uint64_t>> destroyer_pipelines;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12RootSignature>, uint64_t>> destroyer_rootSignatures;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12StateObject>, uint64_t>> destroyer_stateobjects;
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
