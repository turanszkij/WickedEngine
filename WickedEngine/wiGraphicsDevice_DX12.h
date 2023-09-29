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
#include "wiSpinLock.h"

#ifdef PLATFORM_XBOX
#include "wiGraphicsDevice_DX12_XBOX.h"
#else
#include "Utility/dx12/d3d12.h"
#include "Utility/dx12/d3d12video.h"
#include <dxgi1_6.h>
#define PPV_ARGS(x) IID_PPV_ARGS(&x)
#endif // PLATFORM_XBOX

#include <wrl/client.h> // ComPtr

#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED
#define __ID3D12Device1_INTERFACE_DEFINED__
#include "Utility/D3D12MemAlloc.h"

#include <deque>
#include <atomic>
#include <mutex>

namespace wi::graphics
{
	class GraphicsDevice_DX12 final : public GraphicsDevice
	{
	protected:
#ifndef PLATFORM_XBOX
		Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
#endif // PLATFORM_XBOX
		Microsoft::WRL::ComPtr<ID3D12Device5> device;
		Microsoft::WRL::ComPtr<ID3D12VideoDevice> video_device;

#ifdef PLATFORM_WINDOWS_DESKTOP
		Microsoft::WRL::ComPtr<ID3D12Fence> deviceRemovedFence;
		HANDLE deviceRemovedWaitHandle = {};
#endif // PLATFORM_WINDOWS_DESKTOP
		std::mutex onDeviceRemovedMutex;
		bool deviceRemoved = false;

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawInstancedIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawIndexedInstancedIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchMeshIndirectCommandSignature;

		bool tearingSupported = false;
		bool additionalShadingRatesSupported = false;
		bool casting_fully_typed_formats = false;

		uint32_t rtv_descriptor_size = 0;
		uint32_t dsv_descriptor_size = 0;
		uint32_t resource_descriptor_size = 0;
		uint32_t sampler_descriptor_size = 0;
		D3D12_RESOURCE_HEAP_TIER resource_heap_tier = {};

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
			wi::vector<ID3D12CommandList*> submit_cmds;
		} queues[QUEUE_COUNT];

#ifdef PLATFORM_XBOX
		std::mutex queue_locker;
#endif // PLATFORM_XBOX

		struct CopyAllocator
		{
			GraphicsDevice_DX12* device = nullptr;
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue; // create separate copy queue to reduce interference with main QUEUE_COPY
			std::mutex locker;

			struct CopyCMD
			{
				Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
				Microsoft::WRL::ComPtr<ID3D12Fence> fence;
				uint64_t fenceValueSignaled = 0;
				GPUBuffer uploadbuffer;
				inline bool IsValid() const { return commandList != nullptr; }
				inline bool IsCompleted() const { return fence->GetCompletedValue() >= fenceValueSignaled; }
			};
			wi::vector<CopyCMD> freelist;

			void init(GraphicsDevice_DX12* device);
			CopyCMD allocate(uint64_t staging_size);
			void submit(CopyCMD cmd);
		};
		mutable CopyAllocator copyAllocator;

		Microsoft::WRL::ComPtr<ID3D12Fence> frame_fence[BUFFERCOUNT][QUEUE_COUNT];

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

		struct CommandList_DX12
		{
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[BUFFERCOUNT][QUEUE_COUNT];
			Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[QUEUE_COUNT];
			using graphics_command_list_version = ID3D12GraphicsCommandList6;
			uint32_t buffer_index = 0;

			QUEUE_TYPE queue = {};
			uint32_t id = 0;
			wi::vector<CommandList> waits;
			std::atomic_bool waited_on{ false };

			DescriptorBinder binder;
			GPULinearAllocator frame_allocators[BUFFERCOUNT];

			wi::vector<D3D12_RESOURCE_BARRIER> frame_barriers;
			struct Discard
			{
				ID3D12Resource* resource = nullptr;
				D3D12_DISCARD_REGION region = {};
			};
			wi::vector<Discard> discards;
			D3D_PRIMITIVE_TOPOLOGY prev_pt = {};
			wi::vector<std::pair<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>>> pipelines_worker;
			size_t prev_pipeline_hash = {};
			const PipelineState* active_pso = {};
			const Shader* active_cs = {};
			const RaytracingPipelineState* active_rt = {};
			const ID3D12RootSignature* active_rootsig_graphics = {};
			const ID3D12RootSignature* active_rootsig_compute = {};
			ShadingRate prev_shadingrate = {};
			wi::vector<const SwapChain*> swapchains;
			bool dirty_pso = {};
			wi::vector<D3D12_RAYTRACING_GEOMETRY_DESC> accelerationstructure_build_geometries;
			RenderPassInfo renderpass_info;
			wi::vector<D3D12_RESOURCE_BARRIER> renderpass_barriers_begin;
			wi::vector<D3D12_RESOURCE_BARRIER> renderpass_barriers_begin_after_discards;
			wi::vector<D3D12_RESOURCE_BARRIER> renderpass_barriers_end;
			ID3D12Resource* shading_rate_image = nullptr;
			ID3D12Resource* resolve_src[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
			ID3D12Resource* resolve_dst[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
			DXGI_FORMAT resolve_formats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
			ID3D12Resource* resolve_src_ds = nullptr;
			ID3D12Resource* resolve_dst_ds = nullptr;
			DXGI_FORMAT resolve_ds_format = {};
			wi::vector<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS> resolve_subresources[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
			wi::vector<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS> resolve_subresources_dsv = {};
			wi::vector<D3D12_RESOURCE_BARRIER> resolve_src_barriers;

			void reset(uint32_t bufferindex)
			{
				buffer_index = bufferindex;
				waits.clear();
				binder.reset();
				frame_allocators[buffer_index].reset();
				prev_pt = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
				prev_pipeline_hash = 0;
				active_pso = nullptr;
				active_cs = nullptr;
				active_rt = nullptr;
				active_rootsig_graphics = nullptr;
				active_rootsig_compute = nullptr;
				prev_shadingrate = ShadingRate::RATE_INVALID;
				dirty_pso = false;
				swapchains.clear();
				renderpass_info = {};
				renderpass_barriers_begin.clear();
				renderpass_barriers_end.clear();
				for (size_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				{
					resolve_src[i] = {};
					resolve_dst[i] = {};
					resolve_subresources[i].clear();
				}
				resolve_subresources_dsv.clear();
				resolve_src_barriers.clear();
				resolve_src_ds = nullptr;
				resolve_dst_ds = nullptr;
				shading_rate_image = nullptr;
			}

			inline ID3D12CommandAllocator* GetCommandAllocator()
			{
				return commandAllocators[buffer_index][queue].Get();
			}
			inline ID3D12CommandList* GetCommandList()
			{
				return commandLists[queue].Get();
			}
			inline ID3D12GraphicsCommandList* GetGraphicsCommandList()
			{
				assert(queue != QUEUE_VIDEO_DECODE);
				return (ID3D12GraphicsCommandList*)commandLists[queue].Get();
			}
			inline graphics_command_list_version* GetGraphicsCommandListLatest()
			{
				assert(queue != QUEUE_VIDEO_DECODE && queue != QUEUE_COPY);
				return (graphics_command_list_version*)commandLists[queue].Get();
			}
			inline ID3D12VideoDecodeCommandList* GetVideoDecodeCommandList()
			{
				assert(queue == QUEUE_VIDEO_DECODE);
				return (ID3D12VideoDecodeCommandList*)commandLists[queue].Get();
			}
		};
		wi::vector<std::unique_ptr<CommandList_DX12>> commandlists;
		uint32_t cmd_count = 0;
		wi::SpinLock cmd_locker;

		constexpr CommandList_DX12& GetCommandList(CommandList cmd) const
		{
			assert(cmd.IsValid());
			return *(CommandList_DX12*)cmd.internal_state;
		}

		wi::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelines_global;

		void pso_validate(CommandList cmd);

		void predraw(CommandList cmd);
		void predispatch(CommandList cmd);

	public:
		GraphicsDevice_DX12(ValidationMode validationMode = ValidationMode::Disabled, GPUPreference preference = GPUPreference::Discrete);
		~GraphicsDevice_DX12() override;

		bool CreateSwapChain(const SwapChainDesc* desc, wi::platform::window_type window, SwapChain* swapchain) const override;
		bool CreateBuffer2(const GPUBufferDesc * desc, const std::function<void(void*)>& init_callback, GPUBuffer* buffer) const override;
		bool CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture) const override;
		bool CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader) const override;
		bool CreateSampler(const SamplerDesc* desc, Sampler* sampler) const override;
		bool CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const override;
		bool CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso, const RenderPassInfo* renderpass_info = nullptr) const override;
		bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const override;
		bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const override;
		bool CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const override;

		int CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change = nullptr, const ImageAspect* aspect = nullptr, const Swizzle* swizzle = nullptr) const override;
		int CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size = ~0, const Format* format_change = nullptr, const uint32_t* structuredbuffer_stride_change = nullptr) const override;

		int GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource = -1) const override;
		int GetDescriptorIndex(const Sampler* sampler) const override;

		void WriteShadingRateValue(ShadingRate rate, void* dest) const override;
		void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const override;
		void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const override;

		void SetName(GPUResource* pResource, const char* name) const override;

		CommandList BeginCommandList(QUEUE_TYPE queue = QUEUE_GRAPHICS) override;
		void SubmitCommandLists() override;
		void OnDeviceRemoved();

		void WaitForGPU() const override;
		void ClearPipelineStateCache() override;
		size_t GetActivePipelineCount() const override { return pipelines_global.size(); }

		ShaderFormat GetShaderFormat() const override
		{
#ifdef PLATFORM_XBOX
			return ShaderFormat::HLSL6_XS;
#else
			return ShaderFormat::HLSL6;
#endif // PLATFORM_XBOX
		}

		Texture GetBackBuffer(const SwapChain* swapchain) const override;

		ColorSpace GetSwapChainColorSpace(const SwapChain* swapchain) const override;
		bool IsSwapChainSupportsHDR(const SwapChain* swapchain) const override;

		uint64_t GetMinOffsetAlignment(const GPUBufferDesc* desc) const override
		{
			uint64_t alignment = 1u;
			if (has_flag(desc->bind_flags, BindFlag::CONSTANT_BUFFER))
			{
				alignment = std::max(alignment, (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
			}
			if (has_flag(desc->misc_flags, ResourceMiscFlag::BUFFER_RAW))
			{
				alignment = std::max(alignment, (uint64_t)D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT);
			}
			if (has_flag(desc->misc_flags, ResourceMiscFlag::BUFFER_STRUCTURED))
			{
				alignment = std::max(alignment, (uint64_t)desc->stride);
			}
			if (desc->format != Format::UNKNOWN || has_flag(desc->misc_flags, ResourceMiscFlag::TYPED_FORMAT_CASTING))
			{
				alignment = std::max(alignment, 16ull);
			}
			return alignment;
		}

		MemoryUsage GetMemoryUsage() const override
		{
			MemoryUsage retval;
			D3D12MA::Budget budget;
			allocationhandler->allocator->GetBudget(&budget, nullptr);
			retval.budget = budget.BudgetBytes;
			retval.usage = budget.UsageBytes;
			return retval;
		}

		uint32_t GetMaxViewportCount() const override { return D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; };

		void SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count) override;

		///////////////Thread-sensitive////////////////////////

		void WaitCommandList(CommandList cmd, CommandList wait_for) override;
		void RenderPassBegin(const SwapChain* swapchain, CommandList cmd) override;
		void RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags = RenderPassFlags::NONE) override;
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
		void DrawInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd) override;
		void DrawIndexedInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd) override;
		void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) override;
		void DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) override;
		void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) override;
		void DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) override;
		void DispatchMeshIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd) override;
		void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) override;
		void CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd) override;
		void CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox, ImageAspect dst_aspect, ImageAspect src_aspect) override;
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
		void ClearUAV(const GPUResource* resource, uint32_t value, CommandList cmd) override;
		void VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd) override;

		void EventBegin(const char* name, CommandList cmd) override;
		void EventEnd(CommandList cmd) override;
		void SetMarker(const char* name, CommandList cmd) override;

		RenderPassInfo GetRenderPassInfo(CommandList cmd) override
		{
			return GetCommandList(cmd).renderpass_info;
		}

		GPULinearAllocator& GetFrameAllocator(CommandList cmd) override
		{
			return GetCommandList(cmd).frame_allocators[GetBufferIndex()];
		}

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
			Microsoft::WRL::ComPtr<D3D12MA::Allocator> allocator;
			Microsoft::WRL::ComPtr<ID3D12Device> device;
			uint64_t framecount = 0;
			std::mutex destroylocker;

			Microsoft::WRL::ComPtr<D3D12MA::Pool> uma_pool;

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
					HRESULT hr = device->device->CreateDescriptorHeap(&desc, PPV_ARGS(heaps.back()));
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

			std::deque<std::pair<Microsoft::WRL::ComPtr<D3D12MA::Allocation>, uint64_t>> destroyer_allocations;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint64_t>> destroyer_resources;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12QueryHeap>, uint64_t>> destroyer_queryheaps;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12PipelineState>, uint64_t>> destroyer_pipelines;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12RootSignature>, uint64_t>> destroyer_rootSignatures;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12StateObject>, uint64_t>> destroyer_stateobjects;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12VideoDecoderHeap>, uint64_t>> destroyer_video_decoder_heaps;
			std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12VideoDecoder>, uint64_t>> destroyer_video_decoders;
			std::deque<std::pair<int, uint64_t>> destroyer_bindless_res;
			std::deque<std::pair<int, uint64_t>> destroyer_bindless_sam;

			~AllocationHandler()
			{
				Update(~0, 0); // destroy all remaining
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
						destroyer_allocations.pop_front();
						// comptr auto delete
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
				while (!destroyer_video_decoder_heaps.empty())
				{
					if (destroyer_video_decoder_heaps.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						destroyer_video_decoder_heaps.pop_front();
						// comptr auto delete
					}
					else
					{
						break;
					}
				}
				while (!destroyer_video_decoders.empty())
				{
					if (destroyer_video_decoders.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						destroyer_video_decoders.pop_front();
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
