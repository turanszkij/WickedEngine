#pragma once
#ifdef __APPLE__
#include "CommonInclude.h"
#include "wiPlatform.h"
#include "wiAllocator.h"
#include "wiGraphicsDevice.h"
#include "wiUnorderedMap.h"

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <Foundation/Foundation.hpp>

#define IR_RUNTIME_METALCPP
#include <metal_irconverter_runtime/metal_irconverter_runtime.h>

#include <mutex>
#include <deque>

namespace wi::graphics
{
	class GraphicsDevice_Metal final : public GraphicsDevice
	{
	public:
		struct RootLayout
		{
			uint32_t constants[22];
			MTL::GPUAddress root_cbvs[3];
			MTL::GPUAddress resource_table_ptr;
			MTL::GPUAddress sampler_table_ptr;
		};
		struct ResourceTable
		{
			IRDescriptorTableEntry cbvs[arraysize(DescriptorBindingTable::CBV) - arraysize(RootLayout::root_cbvs)];
			IRDescriptorTableEntry srvs[arraysize(DescriptorBindingTable::SRV)];
			IRDescriptorTableEntry uavs[arraysize(DescriptorBindingTable::UAV)];
		};
		NS::SharedPtr<MTL::SamplerState> static_samplers[10];
		struct StaticSamplerDescriptors
		{
			IRDescriptorTableEntry samplers[arraysize(static_samplers)]; // workaround for static sampler, they are not supported by Metal Shader Converter
		} static_sampler_descriptors;
		struct SamplerTable
		{
			IRDescriptorTableEntry samplers[arraysize(DescriptorBindingTable::SAM)];
			StaticSamplerDescriptors static_samplers;
		};
		
	private:
		NS::SharedPtr<MTL::Device> device;
		NS::SharedPtr<MTL::CommandQueue> commandqueue;
		
		struct FrameResources
		{
			NS::SharedPtr<MTL::SharedEvent> event;
			uint64_t requiredValue = 0;
		} frame_resources[BUFFERCOUNT];
		
		FrameResources& GetFrameResources() { return frame_resources[GetBufferIndex()]; }
		
		struct JustInTimePSO
		{
			NS::SharedPtr<MTL::RenderPipelineState> pipeline;
			NS::SharedPtr<MTL::DepthStencilState> depth_stencil_state;
		};
		
		struct CommandList_Metal
		{
			MTL::CommandBuffer* commandbuffer = nullptr;
			GPULinearAllocator frame_allocators[BUFFERCOUNT];
			RenderPassInfo renderpass_info;
			uint32_t id = 0;
			QUEUE_TYPE queue = QUEUE_COUNT;
			const PipelineState* active_pso = nullptr;
			bool dirty_pso = false;
			bool dirty_cs = false;
			const Shader* active_cs = nullptr;
			wi::vector<MTK::View*> presents;
			MTL::RenderCommandEncoder* render_encoder = nullptr;
			MTL::ComputeCommandEncoder* compute_encoder = nullptr;
			MTL::BlitCommandEncoder* blit_encoder = nullptr;
			MTL::PrimitiveType primitive_type = MTL::PrimitiveTypeTriangle;
			NS::SharedPtr<MTL::Buffer> index_buffer;
			MTL::IndexType index_type = MTL::IndexTypeUInt32;
			uint64_t index_buffer_offset = 0;
			wi::vector<std::pair<PipelineHash, JustInTimePSO>> pipelines_worker;
			PipelineHash pipeline_hash;
			DescriptorBindingTable binding_table;
			bool dirty_root = false;
			bool dirty_resource = false;
			bool dirty_sampler = false;
			RootLayout root = {};
			uint32_t render_width = 0;
			uint32_t render_height = 0;
			bool dirty_scissor = false;
			uint32_t scissor_count = 0;
			MTL::ScissorRect scissors[16] = {};
			bool dirty_viewport = false;
			uint32_t viewport_count = 0;
			MTL::Viewport viewports[16] = {};
			
			struct VertexBufferBinding
			{
				GPUBuffer buffer;
				uint64_t offset = 0;
				uint32_t stride = 0;
			};
			VertexBufferBinding vertex_buffers[32];
			bool dirty_vb = false;
			
			wi::vector<GPUBarrier> barriers;

			void reset(uint32_t bufferindex)
			{
				commandbuffer = nullptr;
				frame_allocators[bufferindex].reset();
				renderpass_info = {};
				id = 0;
				queue = QUEUE_COUNT;
				active_pso = nullptr;
				active_cs = nullptr;
				dirty_pso = false;
				dirty_cs = false;
				presents.clear();
				render_encoder = nullptr;
				compute_encoder = nullptr;
				blit_encoder = nullptr;
				primitive_type = MTL::PrimitiveTypeTriangle;
				index_buffer.reset();
				index_buffer_offset = 0;
				index_type = MTL::IndexTypeUInt32;
				pipelines_worker.clear();
				pipeline_hash = {};
				binding_table = {};
				dirty_root = true;
				dirty_resource = true;
				dirty_sampler = true;
				root = {};
				for (auto& x : vertex_buffers)
				{
					x = {};
				}
				dirty_vb = false;
				render_width = 0;
				render_height = 0;
				dirty_viewport = false;
				dirty_scissor = false;
				scissor_count = 0;
				viewport_count = 0;
				for (auto& x : scissors)
				{
					x = {};
				}
				for (auto& x : viewports)
				{
					x = {};
				}
				barriers.clear();
			}
		};
		wi::vector<std::unique_ptr<CommandList_Metal>> commandlists;
		uint32_t cmd_count = 0;
		wi::SpinLock cmd_locker;
		
		wi::unordered_map<PipelineHash, JustInTimePSO> pipelines_global;
		
		NS::SharedPtr<MTL::Buffer> descriptor_heap_res;
		NS::SharedPtr<MTL::Buffer> descriptor_heap_sam;
		
		void binder_flush(CommandList cmd);

		constexpr CommandList_Metal& GetCommandList(CommandList cmd) const
		{
			assert(cmd.IsValid());
			return *(CommandList_Metal*)cmd.internal_state;
		}
		
		void pso_validate(CommandList cmd);
		void predraw(CommandList cmd);
		void predispatch(CommandList cmd);
		void precopy(CommandList cmd);

	public:
		GraphicsDevice_Metal(ValidationMode validationMode = ValidationMode::Disabled, GPUPreference preference = GPUPreference::Discrete);
		~GraphicsDevice_Metal() override;

		bool CreateSwapChain(const SwapChainDesc* desc, wi::platform::window_type window, SwapChain* swapchain) const override;
		bool CreateBuffer2(const GPUBufferDesc* desc, const std::function<void(void*)>& init_callback, GPUBuffer* buffer, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const override;
		bool CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const override;
		bool CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader) const override;
		bool CreateSampler(const SamplerDesc* desc, Sampler* sampler) const override;
		bool CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const override;
		bool CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso, const RenderPassInfo* renderpass_info = nullptr) const override;
		bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const override;
		bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const override;
		bool CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const override;

		int CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change = nullptr, const ImageAspect* aspect = nullptr, const Swizzle* swizzle = nullptr, float min_lod_clamp = 0) const override;
		int CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size = ~0, const Format* format_change = nullptr, const uint32_t* structuredbuffer_stride_change = nullptr) const override;

		void DeleteSubresources(GPUResource* resource) override;

		int GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource = -1) const override;
		int GetDescriptorIndex(const Sampler* sampler) const override;

		void WriteShadingRateValue(ShadingRate rate, void* dest) const override;
		void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const override;
		void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const override;

		void SetName(GPUResource* pResource, const char* name) const override;
		void SetName(Shader* shader, const char* name) const override;

		CommandList BeginCommandList(QUEUE_TYPE queue = QUEUE_GRAPHICS) override;
		void SubmitCommandLists() override;

		void WaitForGPU() const override;
		void ClearPipelineStateCache() override;
		size_t GetActivePipelineCount() const override { return 0; }

		ShaderFormat GetShaderFormat() const override { return ShaderFormat::METAL; }

		Texture GetBackBuffer(const SwapChain* swapchain) const override;

		ColorSpace GetSwapChainColorSpace(const SwapChain* swapchain) const override;
		bool IsSwapChainSupportsHDR(const SwapChain* swapchain) const override;

		uint32_t GetMinOffsetAlignment(const GPUBufferDesc* desc) const override
		{
			uint32_t alignment = 256u;
			if (has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_BUFFER) || has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_TEXTURE_NON_RT_DS) || has_flag(desc->misc_flags, ResourceMiscFlag::ALIASING_TEXTURE_RT_DS))
			{
				alignment = std::max(alignment, uint32_t(64 * 1024)); // 64KB safety to match DX12
			}
			return alignment;
		}

		MemoryUsage GetMemoryUsage() const override
		{
			MemoryUsage retval;
			return retval;
		}

		uint32_t GetMaxViewportCount() const override { return 16; };

		void SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count) override;

		const char* GetTag() const override { return "[Metal]"; }

		///////////////Thread-sensitive////////////////////////

		void WaitCommandList(CommandList cmd, CommandList wait_for) override;
		void RenderPassBegin(const SwapChain* swapchain, CommandList cmd) override;
		void RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags = RenderPassFlags::NONE) override { RenderPassBegin(images, image_count, nullptr, cmd, flags); };
		void RenderPassBegin(const RenderPassImage* images, uint32_t image_count, const GPUQueryHeap* occlusionqueries, CommandList cmd, RenderPassFlags flags = RenderPassFlags::NONE) override;
		void RenderPassEnd(CommandList cmd) override;
		void BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) override;
		void BindViewports(uint32_t NumViewports, const Viewport *pViewports, CommandList cmd) override;
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
		void QueryReset(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd) override;
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

		struct AllocationHandler
		{
			std::mutex destroylocker;
			uint64_t framecount = 0;
			std::deque<std::pair<NS::SharedPtr<MTL::Resource>, uint64_t>> destroyer_resources;
			std::deque<std::pair<NS::SharedPtr<MTL::SamplerState>, uint64_t>> destroyer_samplers;
			std::deque<std::pair<NS::SharedPtr<MTL::Library>, uint64_t>> destroyer_libraries;
			std::deque<std::pair<NS::SharedPtr<MTL::Function>, uint64_t>> destroyer_functions;
			std::deque<std::pair<NS::SharedPtr<MTL::RenderPipelineState>, uint64_t>> destroyer_render_pipelines;
			std::deque<std::pair<NS::SharedPtr<MTL::ComputePipelineState>, uint64_t>> destroyer_compute_pipelines;
			std::deque<std::pair<NS::SharedPtr<MTL::DepthStencilState>, uint64_t>> destroyer_depth_stencil_states;
			std::deque<std::pair<NS::SharedPtr<MTL::CounterSampleBuffer>, uint64_t>> destroyer_counters;
			std::deque<std::pair<int, uint64_t>> destroyer_bindless_res;
			std::deque<std::pair<int, uint64_t>> destroyer_bindless_sam;
			wi::vector<int> free_bindless_res;
			wi::vector<int> free_bindless_sam;
			IRDescriptorTableEntry* descriptor_heap_res_data = nullptr;
			IRDescriptorTableEntry* descriptor_heap_sam_data = nullptr;

			void Update(uint64_t FRAMECOUNT, uint32_t BUFFERCOUNT)
			{
				std::scoped_lock lck(destroylocker);
				framecount = FRAMECOUNT;
				while (!destroyer_resources.empty() && destroyer_resources.front().second + BUFFERCOUNT < FRAMECOUNT)
				{
					remove_resident(destroyer_resources.front().first.get());
					destroyer_resources.pop_front();
					// SharedPtr auto delete
				}
				while (!destroyer_samplers.empty() && destroyer_samplers.front().second + BUFFERCOUNT < FRAMECOUNT)
				{
					destroyer_samplers.pop_front();
					// SharedPtr auto delete
				}
				while (!destroyer_libraries.empty() && destroyer_libraries.front().second + BUFFERCOUNT < FRAMECOUNT)
				{
					destroyer_libraries.pop_front();
					// SharedPtr auto delete
				}
				while (!destroyer_functions.empty() && destroyer_functions.front().second + BUFFERCOUNT < FRAMECOUNT)
				{
					destroyer_functions.pop_front();
					// SharedPtr auto delete
				}
				while (!destroyer_render_pipelines.empty() && destroyer_render_pipelines.front().second + BUFFERCOUNT < FRAMECOUNT)
				{
					destroyer_render_pipelines.pop_front();
					// SharedPtr auto delete
				}
				while (!destroyer_compute_pipelines.empty() && destroyer_compute_pipelines.front().second + BUFFERCOUNT < FRAMECOUNT)
				{
					destroyer_compute_pipelines.pop_front();
					// SharedPtr auto delete
				}
				while (!destroyer_depth_stencil_states.empty() && destroyer_depth_stencil_states.front().second + BUFFERCOUNT < FRAMECOUNT)
				{
					destroyer_depth_stencil_states.pop_front();
					// SharedPtr auto delete
				}
				while (!destroyer_counters.empty() && destroyer_counters.front().second + BUFFERCOUNT < FRAMECOUNT)
				{
					destroyer_counters.pop_front();
					// SharedPtr auto delete
				}
				while (!destroyer_bindless_res.empty() && destroyer_bindless_res.front().second + BUFFERCOUNT < FRAMECOUNT)
				{
					free_bindless_res.push_back(destroyer_bindless_res.front().first);
					destroyer_bindless_res.pop_front();
				}
				while (!destroyer_bindless_sam.empty() && destroyer_bindless_sam.front().second + BUFFERCOUNT < FRAMECOUNT)
				{
					free_bindless_sam.push_back(destroyer_bindless_sam.front().first);
					destroyer_bindless_sam.pop_front();
				}
			}
			
			int allocate_bindless(const IRDescriptorTableEntry& entry)
			{
				std::scoped_lock lck(destroylocker);
				if (free_bindless_res.empty())
					return -1;
				int index = free_bindless_res.back();
				free_bindless_res.pop_back();
				std::memcpy(descriptor_heap_res_data + index, &entry, sizeof(entry));
				return index;
			}
			int allocate_bindless_sampler(const IRDescriptorTableEntry& entry)
			{
				std::scoped_lock lck(destroylocker);
				if (free_bindless_sam.empty())
					return -1;
				int index = free_bindless_sam.back();
				free_bindless_sam.pop_back();
				std::memcpy(descriptor_heap_sam_data + index, &entry, sizeof(entry));
				return index;
			}
			
			NS::SharedPtr<MTL::ResidencySet> residency_set;
			void make_resident(const MTL::Allocation* allocation)
			{
				if (allocation == nullptr)
					return;
				std::scoped_lock locker(destroylocker);
				residency_set->addAllocation(allocation);
			}
			void remove_resident(const MTL::Allocation* allocation)
			{
				if (allocation == nullptr)
					return;
				residency_set->removeAllocation(allocation);
			}
		};
		wi::allocator::shared_ptr<AllocationHandler> allocationhandler;
	};
}
#endif // __APPLE__
