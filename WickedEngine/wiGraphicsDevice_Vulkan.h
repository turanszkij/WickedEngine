#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"

#ifndef PLATFORM_UWP
#define WICKEDENGINE_BUILD_VULKAN
#endif // PLATFORM_UWP

#ifdef WICKEDENGINE_BUILD_VULKAN
#include "wiGraphicsDevice.h"
#include "wiUnorderedMap.h"
#include "wiVector.h"
#include "wiSpinLock.h"

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // _WIN32

#define VK_NO_PROTOTYPES
#include "Utility/vulkan/vulkan.h"
#include "Utility/volk.h"
#include "Utility/vk_mem_alloc.h"

#include <deque>
#include <atomic>
#include <mutex>
#include <algorithm>

namespace wi::graphics
{
	class GraphicsDevice_Vulkan final : public GraphicsDevice
	{
		friend struct CommandQueue;
	protected:
		bool debugUtils = false;
		VkInstance instance = VK_NULL_HANDLE;
	    VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		wi::vector<VkQueueFamilyProperties> queueFamilies;
		uint32_t graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
		uint32_t computeFamily = VK_QUEUE_FAMILY_IGNORED;
		uint32_t copyFamily = VK_QUEUE_FAMILY_IGNORED;
		wi::vector<uint32_t> families;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue computeQueue = VK_NULL_HANDLE;
		VkQueue copyQueue = VK_NULL_HANDLE;

		VkPhysicalDeviceProperties2 properties2 = {};
		VkPhysicalDeviceVulkan11Properties properties_1_1 = {};
		VkPhysicalDeviceVulkan12Properties properties_1_2 = {};
		VkPhysicalDeviceSamplerFilterMinmaxProperties sampler_minmax_properties = {};
		VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties = {};
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracing_properties = {};
		VkPhysicalDeviceFragmentShadingRatePropertiesKHR fragment_shading_rate_properties = {};
		VkPhysicalDeviceMeshShaderPropertiesNV mesh_shader_properties = {};
		VkPhysicalDeviceMemoryProperties2 memory_properties_2 = {};
		VkPhysicalDeviceDepthStencilResolveProperties depth_stencil_resolve_properties = {};

		VkPhysicalDeviceFeatures2 features2 = {};
		VkPhysicalDeviceVulkan11Features features_1_1 = {};
		VkPhysicalDeviceVulkan12Features features_1_2 = {};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {};
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_features = {};
		VkPhysicalDeviceRayQueryFeaturesKHR raytracing_query_features = {};
		VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragment_shading_rate_features = {};
		VkPhysicalDeviceMeshShaderFeaturesNV mesh_shader_features = {};
		VkPhysicalDeviceConditionalRenderingFeaturesEXT conditional_rendering_features = {};
		VkPhysicalDeviceDepthClipEnableFeaturesEXT depth_clip_enable_features = {};

		wi::vector<VkDynamicState> pso_dynamicStates;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};

		VkBuffer		nullBuffer = VK_NULL_HANDLE;
		VmaAllocation	nullBufferAllocation = VK_NULL_HANDLE;
		VkBufferView	nullBufferView = VK_NULL_HANDLE;
		VkSampler		nullSampler = VK_NULL_HANDLE;
		VmaAllocation	nullImageAllocation1D = VK_NULL_HANDLE;
		VmaAllocation	nullImageAllocation2D = VK_NULL_HANDLE;
		VmaAllocation	nullImageAllocation3D = VK_NULL_HANDLE;
		VkImage			nullImage1D = VK_NULL_HANDLE;
		VkImage			nullImage2D = VK_NULL_HANDLE;
		VkImage			nullImage3D = VK_NULL_HANDLE;
		VkImageView		nullImageView1D = VK_NULL_HANDLE;
		VkImageView		nullImageView1DArray = VK_NULL_HANDLE;
		VkImageView		nullImageView2D = VK_NULL_HANDLE;
		VkImageView		nullImageView2DArray = VK_NULL_HANDLE;
		VkImageView		nullImageViewCube = VK_NULL_HANDLE;
		VkImageView		nullImageViewCubeArray = VK_NULL_HANDLE;
		VkImageView		nullImageView3D = VK_NULL_HANDLE;

		struct CommandQueue
		{
			VkQueue queue = VK_NULL_HANDLE;
			VkSemaphore semaphore = VK_NULL_HANDLE;
			wi::vector<SwapChain> swapchain_updates;
			wi::vector<VkSwapchainKHR> submit_swapchains;
			wi::vector<uint32_t> submit_swapChainImageIndices;
			wi::vector<VkPipelineStageFlags> submit_waitStages;
			wi::vector<VkSemaphore> submit_waitSemaphores;
			wi::vector<uint64_t> submit_waitValues;
			wi::vector<VkSemaphore> submit_signalSemaphores;
			wi::vector<uint64_t> submit_signalValues;
			wi::vector<VkCommandBuffer> submit_cmds;

			bool sparse_binding_supported = false;
			std::mutex sparse_mutex;
			VkSemaphore sparse_semaphore = VK_NULL_HANDLE;
			uint64_t sparse_semaphore_value = 0ull;
			wi::vector<VkBindSparseInfo> sparse_infos;
			struct DataPerBind
			{
				wi::vector<VkSparseBufferMemoryBindInfo> buffer_bind_infos;
				wi::vector<VkSparseImageOpaqueMemoryBindInfo> image_opaque_bind_infos;
				wi::vector<VkSparseImageMemoryBindInfo> image_bind_infos;
				wi::vector<VkSparseMemoryBind> memory_binds;
				wi::vector<VkSparseImageMemoryBind> image_memory_binds;
			};
			wi::vector<DataPerBind> sparse_binds;

			void submit(GraphicsDevice_Vulkan* device, VkFence fence);

		} queues[QUEUE_COUNT];

		struct CopyAllocator
		{
			GraphicsDevice_Vulkan* device = nullptr;
			VkSemaphore semaphore = VK_NULL_HANDLE;
			uint64_t fenceValue = 0;
			std::mutex locker;

			struct CopyCMD
			{
				VkCommandPool commandPool = VK_NULL_HANDLE;
				VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
				uint64_t target = 0;
				GPUBuffer uploadbuffer;
			};
			wi::vector<CopyCMD> freelist; // available
			wi::vector<CopyCMD> worklist; // in progress
			wi::vector<VkCommandBuffer> submit_cmds; // for next submit
			uint64_t submit_wait = 0; // last submit wait value

			void init(GraphicsDevice_Vulkan* device);
			void destroy();
			CopyCMD allocate(uint64_t staging_size);
			void submit(CopyCMD cmd);
			uint64_t flush();
		};
		mutable CopyAllocator copyAllocator;

		mutable std::mutex initLocker;
		mutable bool submit_inits = false;

		struct FrameResources
		{
			VkFence fence[QUEUE_COUNT] = {};

			VkCommandPool initCommandPool = VK_NULL_HANDLE;
			VkCommandBuffer initCommandBuffer = VK_NULL_HANDLE;
		};
		FrameResources frames[BUFFERCOUNT];
		const FrameResources& GetFrameResources() const { return frames[GetBufferIndex()]; }
		FrameResources& GetFrameResources() { return frames[GetBufferIndex()]; }

		struct DescriptorBinder
		{
			DescriptorBindingTable table;
			GraphicsDevice_Vulkan* device;

			wi::vector<VkWriteDescriptorSet> descriptorWrites;
			wi::vector<VkDescriptorBufferInfo> bufferInfos;
			wi::vector<VkDescriptorImageInfo> imageInfos;
			wi::vector<VkBufferView> texelBufferViews;
			wi::vector<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructureViews;

			uint32_t uniform_buffer_dynamic_offsets[DESCRIPTORBINDER_CBV_COUNT] = {};

			VkDescriptorSet descriptorSet_graphics = VK_NULL_HANDLE;
			VkDescriptorSet descriptorSet_compute = VK_NULL_HANDLE;

			enum DIRTY_FLAGS
			{
				DIRTY_NONE = 0,
				DIRTY_DESCRIPTOR = 1 << 1,
				DIRTY_OFFSET = 1 << 2,

				DIRTY_ALL = ~0,
			};
			uint32_t dirty = DIRTY_NONE;

			void init(GraphicsDevice_Vulkan* device);
			void reset();
			void flush(bool graphics, CommandList cmd);
		};

		struct DescriptorBinderPool
		{
			GraphicsDevice_Vulkan* device;
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			uint32_t poolSize = 256;

			void init(GraphicsDevice_Vulkan* device);
			void destroy();
			void reset();
		};

		struct CommandList_Vulkan
		{
			VkCommandPool commandPools[BUFFERCOUNT][QUEUE_COUNT] = {};
			VkCommandBuffer commandBuffers[BUFFERCOUNT][QUEUE_COUNT] = {};
			uint32_t buffer_index = 0;

			QUEUE_TYPE queue = {};
			uint32_t id = 0;
			wi::vector<CommandList> waits;

			DescriptorBinder binder;
			DescriptorBinderPool binder_pools[BUFFERCOUNT];
			GPULinearAllocator frame_allocators[BUFFERCOUNT];

			wi::vector<std::pair<size_t, VkPipeline>> pipelines_worker;
			size_t prev_pipeline_hash = {};
			const PipelineState* active_pso = {};
			const Shader* active_cs = {};
			const RaytracingPipelineState* active_rt = {};
			const RenderPass* active_renderpass = {};
			ShadingRate prev_shadingrate = {};
			wi::vector<SwapChain> prev_swapchains;
			uint32_t vb_strides[8] = {};
			size_t vb_hash = {};
			bool dirty_pso = {};
			wi::vector<VkMemoryBarrier> frame_memoryBarriers;
			wi::vector<VkImageMemoryBarrier> frame_imageBarriers;
			wi::vector<VkBufferMemoryBarrier> frame_bufferBarriers;
			wi::vector<VkAccelerationStructureGeometryKHR> accelerationstructure_build_geometries;
			wi::vector<VkAccelerationStructureBuildRangeInfoKHR> accelerationstructure_build_ranges;

			void reset(uint32_t bufferindex)
			{
				buffer_index = bufferindex;
				waits.clear();
				binder_pools[buffer_index].reset();
				binder.reset();
				frame_allocators[buffer_index].reset();
				prev_pipeline_hash = 0;
				active_pso = nullptr;
				active_cs = nullptr;
				active_rt = nullptr;
				active_renderpass = nullptr;
				dirty_pso = false;
				prev_shadingrate = ShadingRate::RATE_INVALID;
				vb_hash = 0;
				for (int i = 0; i < arraysize(vb_strides); ++i)
				{
					vb_strides[i] = 0;
				}
				prev_swapchains.clear();
			}

			inline VkCommandPool GetCommandPool() const
			{
				return commandPools[buffer_index][queue];
			}
			inline VkCommandBuffer GetCommandBuffer() const
			{
				return commandBuffers[buffer_index][queue];
			}
		};
		wi::vector<std::unique_ptr<CommandList_Vulkan>> commandlists;
		uint32_t cmd_count = 0;
		wi::SpinLock cmd_locker;

		constexpr CommandList_Vulkan& GetCommandList(CommandList cmd) const
		{
			assert(cmd.IsValid());
			return *(CommandList_Vulkan*)cmd.internal_state;
		}

		struct PSOLayout
		{
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			wi::vector<VkDescriptorSet> bindlessSets;
			uint32_t bindlessFirstSet = 0;
		};
		mutable wi::unordered_map<size_t, PSOLayout> pso_layout_cache;
		mutable std::mutex pso_layout_cache_mutex;

		VkPipelineCache pipelineCache = VK_NULL_HANDLE;
		wi::unordered_map<size_t, VkPipeline> pipelines_global;

		void pso_validate(CommandList cmd);

		void predraw(CommandList cmd);
		void predispatch(CommandList cmd);

		static constexpr uint32_t immutable_sampler_slot_begin = 100;
		wi::vector<VkSampler> immutable_samplers;

	public:
		GraphicsDevice_Vulkan(wi::platform::window_type window, ValidationMode validationMode = ValidationMode::Disabled);
		~GraphicsDevice_Vulkan() override;

		bool CreateSwapChain(const SwapChainDesc* desc, wi::platform::window_type window, SwapChain* swapchain) const override;
		bool CreateBuffer(const GPUBufferDesc* desc, const void* initial_data, GPUBuffer* buffer) const override;
		bool CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture) const override;
		bool CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader) const override;
		bool CreateSampler(const SamplerDesc* desc, Sampler* sampler) const override;
		bool CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const override;
		bool CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso) const override;
		bool CreateRenderPass(const RenderPassDesc* desc, RenderPass* renderpass) const override;
		bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const override;
		bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const override;
		
		int CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change = nullptr) const override;
		int CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size = ~0, const Format* format_change = nullptr) const override;

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

		ShaderFormat GetShaderFormat() const override { return ShaderFormat::SPIRV; }

		Texture GetBackBuffer(const SwapChain* swapchain) const override;

		ColorSpace GetSwapChainColorSpace(const SwapChain* swapchain) const override;
		bool IsSwapChainSupportsHDR(const SwapChain* swapchain) const override;

		uint64_t GetMinOffsetAlignment(const GPUBufferDesc* desc) const override
		{
			uint64_t alignment = 1u;
			if (has_flag(desc->bind_flags, BindFlag::CONSTANT_BUFFER))
			{
				alignment = std::max(alignment, properties2.properties.limits.minUniformBufferOffsetAlignment);
			}
			if (desc->format == Format::UNKNOWN)
			{
				alignment = std::max(alignment, properties2.properties.limits.minStorageBufferOffsetAlignment);
			}
			else
			{
				alignment = std::max(alignment, properties2.properties.limits.minTexelBufferOffsetAlignment);
			}
			return alignment;
		}

		MemoryUsage GetMemoryUsage() const override
		{
			MemoryUsage retval;
			VmaBudget budgets[VK_MAX_MEMORY_HEAPS] = {};
			vmaGetHeapBudgets(allocationhandler->allocator, budgets);
			for (uint32_t i = 0; i < memory_properties_2.memoryProperties.memoryHeapCount; ++i)
			{
				if (memory_properties_2.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				{
					retval.budget += budgets[i].budget;
					retval.usage += budgets[i].usage;
				}
			}
			return retval;
		}

		uint32_t GetMaxViewportCount() const override { return properties2.properties.limits.maxViewports; };

		void SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count) override;

		///////////////Thread-sensitive////////////////////////

		void WaitCommandList(CommandList cmd, CommandList wait_for) override;
		void RenderPassBegin(const SwapChain* swapchain, CommandList cmd) override;
		void RenderPassBegin(const RenderPass* renderpass, CommandList cmd) override;
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
		void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) override;
		void CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd) override;
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

		void EventBegin(const char* name, CommandList cmd) override;
		void EventEnd(CommandList cmd) override;
		void SetMarker(const char* name, CommandList cmd) override;

		GPULinearAllocator& GetFrameAllocator(CommandList cmd) override
		{
			return GetCommandList(cmd).frame_allocators[GetBufferIndex()];
		}

		struct AllocationHandler
		{
			VmaAllocator allocator = VK_NULL_HANDLE;
			VkDevice device = VK_NULL_HANDLE;
			VkInstance instance;
			uint64_t framecount = 0;
			std::mutex destroylocker;

			struct BindlessDescriptorHeap
			{
				VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
				VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
				VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
				wi::vector<int> freelist;
				std::mutex locker;

				void init(VkDevice device, VkDescriptorType type, uint32_t descriptorCount)
				{
					descriptorCount = std::min(descriptorCount, 100000u);

					VkDescriptorPoolSize poolSize = {};
					poolSize.type = type;
					poolSize.descriptorCount = descriptorCount;

					VkDescriptorPoolCreateInfo poolInfo = {};
					poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
					poolInfo.poolSizeCount = 1;
					poolInfo.pPoolSizes = &poolSize;
					poolInfo.maxSets = 1;
					poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

					VkResult res = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
					assert(res == VK_SUCCESS);

					VkDescriptorSetLayoutBinding binding = {};
					binding.descriptorType = type;
					binding.binding = 0;
					binding.descriptorCount = descriptorCount;
					binding.stageFlags = VK_SHADER_STAGE_ALL;
					binding.pImmutableSamplers = nullptr;

					VkDescriptorSetLayoutCreateInfo layoutInfo = {};
					layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					layoutInfo.bindingCount = 1;
					layoutInfo.pBindings = &binding;
					layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

					VkDescriptorBindingFlags bindingFlags =
						VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
						VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
						VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
					//| VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
					VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {};
					bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
					bindingFlagsInfo.bindingCount = 1;
					bindingFlagsInfo.pBindingFlags = &bindingFlags;
					layoutInfo.pNext = &bindingFlagsInfo;

					res = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
					assert(res == VK_SUCCESS);

					VkDescriptorSetAllocateInfo allocInfo = {};
					allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					allocInfo.descriptorPool = descriptorPool;
					allocInfo.descriptorSetCount = 1;
					allocInfo.pSetLayouts = &descriptorSetLayout;
					res = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
					assert(res == VK_SUCCESS);

					for (int i = 0; i < (int)descriptorCount; ++i)
					{
						freelist.push_back((int)descriptorCount - i - 1);
					}
				}
				void destroy(VkDevice device)
				{
					if (descriptorSetLayout != VK_NULL_HANDLE)
					{
						vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
						descriptorSetLayout = VK_NULL_HANDLE;
					}

					if (descriptorPool != VK_NULL_HANDLE)
					{
						vkDestroyDescriptorPool(device, descriptorPool, nullptr);
						descriptorPool = VK_NULL_HANDLE;
					}
				}

				int allocate()
				{
					locker.lock();
					if (!freelist.empty())
					{
						int index = freelist.back();
						freelist.pop_back();
						locker.unlock();
						return index;
					}
					locker.unlock();
					return -1;
				}
				void free(int index)
				{
					if (index >= 0)
					{
						locker.lock();
						freelist.push_back(index);
						locker.unlock();
					}
				}
			};
			BindlessDescriptorHeap bindlessSampledImages;
			BindlessDescriptorHeap bindlessUniformTexelBuffers;
			BindlessDescriptorHeap bindlessStorageBuffers;
			BindlessDescriptorHeap bindlessStorageImages;
			BindlessDescriptorHeap bindlessStorageTexelBuffers;
			BindlessDescriptorHeap bindlessSamplers;
			BindlessDescriptorHeap bindlessAccelerationStructures;

			std::deque<std::pair<VmaAllocation, uint64_t>> destroyer_allocations;
			std::deque<std::pair<std::pair<VkImage, VmaAllocation>, uint64_t>> destroyer_images;
			std::deque<std::pair<VkImageView, uint64_t>> destroyer_imageviews;
			std::deque<std::pair<std::pair<VkBuffer, VmaAllocation>, uint64_t>> destroyer_buffers;
			std::deque<std::pair<VkBufferView, uint64_t>> destroyer_bufferviews;
			std::deque<std::pair<VkAccelerationStructureKHR, uint64_t>> destroyer_bvhs;
			std::deque<std::pair<VkSampler, uint64_t>> destroyer_samplers;
			std::deque<std::pair<VkDescriptorPool, uint64_t>> destroyer_descriptorPools;
			std::deque<std::pair<VkDescriptorSetLayout, uint64_t>> destroyer_descriptorSetLayouts;
			std::deque<std::pair<VkDescriptorUpdateTemplate, uint64_t>> destroyer_descriptorUpdateTemplates;
			std::deque<std::pair<VkShaderModule, uint64_t>> destroyer_shadermodules;
			std::deque<std::pair<VkPipelineLayout, uint64_t>> destroyer_pipelineLayouts;
			std::deque<std::pair<VkPipeline, uint64_t>> destroyer_pipelines;
			std::deque<std::pair<VkRenderPass, uint64_t>> destroyer_renderpasses;
			std::deque<std::pair<VkFramebuffer, uint64_t>> destroyer_framebuffers;
			std::deque<std::pair<VkQueryPool, uint64_t>> destroyer_querypools;
			std::deque<std::pair<VkSwapchainKHR, uint64_t>> destroyer_swapchains;
			std::deque<std::pair<VkSurfaceKHR, uint64_t>> destroyer_surfaces;
			std::deque<std::pair<VkSemaphore, uint64_t>> destroyer_semaphores;
			std::deque<std::pair<int, uint64_t>> destroyer_bindlessSampledImages;
			std::deque<std::pair<int, uint64_t>> destroyer_bindlessUniformTexelBuffers;
			std::deque<std::pair<int, uint64_t>> destroyer_bindlessStorageBuffers;
			std::deque<std::pair<int, uint64_t>> destroyer_bindlessStorageImages;
			std::deque<std::pair<int, uint64_t>> destroyer_bindlessStorageTexelBuffers;
			std::deque<std::pair<int, uint64_t>> destroyer_bindlessSamplers;
			std::deque<std::pair<int, uint64_t>> destroyer_bindlessAccelerationStructures;

			~AllocationHandler()
			{
				bindlessSampledImages.destroy(device);
				bindlessUniformTexelBuffers.destroy(device);
				bindlessStorageBuffers.destroy(device);
				bindlessStorageImages.destroy(device);
				bindlessStorageTexelBuffers.destroy(device);
				bindlessSamplers.destroy(device);
				bindlessAccelerationStructures.destroy(device);
				Update(~0, 0); // destroy all remaining
				vmaDestroyAllocator(allocator);
				vkDestroyDevice(device, nullptr);
				vkDestroyInstance(instance, nullptr);
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
						vmaFreeMemory(allocator, item.first);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_images.empty())
				{
					if (destroyer_images.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_images.front();
						destroyer_images.pop_front();
						vmaDestroyImage(allocator, item.first.first, item.first.second);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_imageviews.empty())
				{
					if (destroyer_imageviews.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_imageviews.front();
						destroyer_imageviews.pop_front();
						vkDestroyImageView(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_buffers.empty())
				{
					if (destroyer_buffers.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_buffers.front();
						destroyer_buffers.pop_front();
						vmaDestroyBuffer(allocator, item.first.first, item.first.second);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bufferviews.empty())
				{
					if (destroyer_bufferviews.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_bufferviews.front();
						destroyer_bufferviews.pop_front();
						vkDestroyBufferView(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bvhs.empty())
				{
					if (destroyer_bvhs.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_bvhs.front();
						destroyer_bvhs.pop_front();
						vkDestroyAccelerationStructureKHR(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_samplers.empty())
				{
					if (destroyer_samplers.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_samplers.front();
						destroyer_samplers.pop_front();
						vkDestroySampler(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_descriptorPools.empty())
				{
					if (destroyer_descriptorPools.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_descriptorPools.front();
						destroyer_descriptorPools.pop_front();
						vkDestroyDescriptorPool(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_descriptorSetLayouts.empty())
				{
					if (destroyer_descriptorSetLayouts.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_descriptorSetLayouts.front();
						destroyer_descriptorSetLayouts.pop_front();
						vkDestroyDescriptorSetLayout(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_descriptorUpdateTemplates.empty())
				{
					if (destroyer_descriptorUpdateTemplates.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_descriptorUpdateTemplates.front();
						destroyer_descriptorUpdateTemplates.pop_front();
						vkDestroyDescriptorUpdateTemplate(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_shadermodules.empty())
				{
					if (destroyer_shadermodules.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_shadermodules.front();
						destroyer_shadermodules.pop_front();
						vkDestroyShaderModule(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_pipelineLayouts.empty())
				{
					if (destroyer_pipelineLayouts.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_pipelineLayouts.front();
						destroyer_pipelineLayouts.pop_front();
						vkDestroyPipelineLayout(device, item.first, nullptr);
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
						auto item = destroyer_pipelines.front();
						destroyer_pipelines.pop_front();
						vkDestroyPipeline(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_renderpasses.empty())
				{
					if (destroyer_renderpasses.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_renderpasses.front();
						destroyer_renderpasses.pop_front();
						vkDestroyRenderPass(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_framebuffers.empty())
				{
					if (destroyer_framebuffers.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_framebuffers.front();
						destroyer_framebuffers.pop_front();
						vkDestroyFramebuffer(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_querypools.empty())
				{
					if (destroyer_querypools.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_querypools.front();
						destroyer_querypools.pop_front();
						vkDestroyQueryPool(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_swapchains.empty())
				{
					if (destroyer_swapchains.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_swapchains.front();
						destroyer_swapchains.pop_front();
						vkDestroySwapchainKHR(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_surfaces.empty())
				{
					if (destroyer_surfaces.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_surfaces.front();
						destroyer_surfaces.pop_front();
						vkDestroySurfaceKHR(instance, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_semaphores.empty())
				{
					if (destroyer_semaphores.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						auto item = destroyer_semaphores.front();
						destroyer_semaphores.pop_front();
						vkDestroySemaphore(device, item.first, nullptr);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bindlessSampledImages.empty())
				{
					if (destroyer_bindlessSampledImages.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						int index = destroyer_bindlessSampledImages.front().first;
						destroyer_bindlessSampledImages.pop_front();
						bindlessSampledImages.free(index);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bindlessUniformTexelBuffers.empty())
				{
					if (destroyer_bindlessUniformTexelBuffers.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						int index = destroyer_bindlessUniformTexelBuffers.front().first;
						destroyer_bindlessUniformTexelBuffers.pop_front();
						bindlessUniformTexelBuffers.free(index);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bindlessStorageBuffers.empty())
				{
					if (destroyer_bindlessStorageBuffers.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						int index = destroyer_bindlessStorageBuffers.front().first;
						destroyer_bindlessStorageBuffers.pop_front();
						bindlessStorageBuffers.free(index);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bindlessStorageImages.empty())
				{
					if (destroyer_bindlessStorageImages.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						int index = destroyer_bindlessStorageImages.front().first;
						destroyer_bindlessStorageImages.pop_front();
						bindlessStorageImages.free(index);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bindlessStorageTexelBuffers.empty())
				{
					if (destroyer_bindlessStorageTexelBuffers.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						int index = destroyer_bindlessStorageTexelBuffers.front().first;
						destroyer_bindlessStorageTexelBuffers.pop_front();
						bindlessStorageTexelBuffers.free(index);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bindlessSamplers.empty())
				{
					if (destroyer_bindlessSamplers.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						int index = destroyer_bindlessSamplers.front().first;
						destroyer_bindlessSamplers.pop_front();
						bindlessSamplers.free(index);
					}
					else
					{
						break;
					}
				}
				while (!destroyer_bindlessAccelerationStructures.empty())
				{
					if (destroyer_bindlessAccelerationStructures.front().second + BUFFERCOUNT < FRAMECOUNT)
					{
						int index = destroyer_bindlessAccelerationStructures.front().first;
						destroyer_bindlessAccelerationStructures.pop_front();
						bindlessAccelerationStructures.free(index);
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

#endif // WICKEDENGINE_BUILD_VULKAN
