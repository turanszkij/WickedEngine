#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"

#if defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX)
#define WICKEDENGINE_BUILD_VULKAN
#endif // PLATFORM_WINDOWS_DESKTOP || PLATFORM_LINUX

#ifdef WICKEDENGINE_BUILD_VULKAN
#include "wiGraphicsDevice.h"
#include "wiUnorderedMap.h"
#include "wiVector.h"
#include "wiSpinLock.h"
#include "wiBacklog.h"
#include "wiHelper.h"

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // _WIN32

#define VK_NO_PROTOTYPES
#include "Utility/vulkan/vulkan.h"
#include "Utility/volk.h"
#include "Utility/vk_mem_alloc.h"
#include "Utility/vk_enum_string_helper.h"

#include <deque>
#include <atomic>
#include <mutex>
#include <algorithm>

#define vulkan_assert(cond, fname) { wilog_assert(cond, "Vulkan error: %s failed with %s (%s:%d)", fname, string_VkResult(res), relative_path(__FILE__), __LINE__); }
#define vulkan_check(call) [&]() { VkResult res = call; vulkan_assert((res >= VK_SUCCESS), extract_function_name(#call).c_str()); return res; }()

namespace wi::graphics
{
	struct PSOLayoutHash
	{
		VkPushConstantRange push = {};
		struct Item
		{
			bool used = true;
			VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
			VkDescriptorSetLayoutBinding binding = {};
		};
		wi::vector<Item> items;
		size_t hash = 0;

		inline bool operator==(const PSOLayoutHash& other) const
		{
			if (hash != other.hash)
				return false;
			if (items.size() != other.items.size())
				return false;
			for (size_t i = 0; i < items.size(); ++i)
			{
				const Item& a = items[i];
				const Item& b = other.items[i];
				if (
					a.used != b.used ||
					a.viewType != b.viewType ||
					a.binding.binding != b.binding.binding ||
					a.binding.descriptorCount != b.binding.descriptorCount ||
					a.binding.descriptorType != b.binding.descriptorType ||
					a.binding.stageFlags != b.binding.stageFlags ||
					a.binding.pImmutableSamplers != b.binding.pImmutableSamplers
					)
					return false;
			}
			return
				push.offset == other.push.offset &&
				push.size == other.push.size &&
				push.stageFlags == other.push.stageFlags;
		}
		inline void embed_hash()
		{
			hash = 0;
			for (auto& x : items)
			{
				wi::helper::hash_combine(hash, x.used);
				wi::helper::hash_combine(hash, x.viewType);
				wi::helper::hash_combine(hash, x.binding.binding);
				wi::helper::hash_combine(hash, x.binding.descriptorCount);
				wi::helper::hash_combine(hash, x.binding.descriptorType);
				wi::helper::hash_combine(hash, x.binding.stageFlags);
			}
			wi::helper::hash_combine(hash, push.offset);
			wi::helper::hash_combine(hash, push.size);
			wi::helper::hash_combine(hash, push.stageFlags);
		}
		constexpr size_t get_hash() const
		{
			return hash;
		}
	};
}
namespace std
{
	template <>
	struct hash<wi::graphics::PSOLayoutHash>
	{
		constexpr size_t operator()(const wi::graphics::PSOLayoutHash& hash) const
		{
			return hash.get_hash();
		}
	};
}

namespace wi::graphics
{
	class GraphicsDevice_Vulkan final : public GraphicsDevice
	{
		friend struct CommandQueue;
	protected:
		VkInstance instance = VK_NULL_HANDLE;
	    VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		wi::vector<VkQueueFamilyProperties2> queueFamilies;
		wi::vector<VkQueueFamilyVideoPropertiesKHR> queueFamiliesVideo;
		uint32_t graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
		uint32_t computeFamily = VK_QUEUE_FAMILY_IGNORED;
		uint32_t copyFamily = VK_QUEUE_FAMILY_IGNORED;
		uint32_t videoFamily = VK_QUEUE_FAMILY_IGNORED;
		uint32_t initFamily = VK_QUEUE_FAMILY_IGNORED;
		uint32_t sparseFamily = VK_QUEUE_FAMILY_IGNORED;
		wi::vector<uint32_t> families;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue computeQueue = VK_NULL_HANDLE;
		VkQueue copyQueue = VK_NULL_HANDLE;
		VkQueue videoQueue = VK_NULL_HANDLE;
		VkQueue initQueue = VK_NULL_HANDLE;
		VkQueue sparseQueue = VK_NULL_HANDLE;
		bool debugUtils = false;

		VkPhysicalDeviceProperties2 properties2 = {};
		VkPhysicalDeviceVulkan11Properties properties_1_1 = {};
		VkPhysicalDeviceVulkan12Properties properties_1_2 = {};
		VkPhysicalDeviceVulkan13Properties properties_1_3 = {};
		VkPhysicalDeviceSamplerFilterMinmaxProperties sampler_minmax_properties = {};
		VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties = {};
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracing_properties = {};
		VkPhysicalDeviceFragmentShadingRatePropertiesKHR fragment_shading_rate_properties = {};
		VkPhysicalDeviceMeshShaderPropertiesEXT mesh_shader_properties = {};
		VkPhysicalDeviceMemoryProperties2 memory_properties_2 = {};
		VkPhysicalDeviceDepthStencilResolveProperties depth_stencil_resolve_properties = {};
		VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservative_raster_properties = {};

		VkPhysicalDeviceFeatures2 features2 = {};
		VkPhysicalDeviceVulkan11Features features_1_1 = {};
		VkPhysicalDeviceVulkan12Features features_1_2 = {};
		VkPhysicalDeviceVulkan13Features features_1_3 = {};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {};
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_features = {};
		VkPhysicalDeviceRayQueryFeaturesKHR raytracing_query_features = {};
		VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragment_shading_rate_features = {};
		VkPhysicalDeviceConditionalRenderingFeaturesEXT conditional_rendering_features = {};
		VkPhysicalDeviceDepthClipEnableFeaturesEXT depth_clip_enable_features = {};
		VkPhysicalDeviceImageViewMinLodFeaturesEXT image_view_min_lod_features = {};
		VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features = {};

		VkVideoDecodeH264ProfileInfoKHR decode_h264_profile = {};
		VkVideoDecodeH264CapabilitiesKHR decode_h264_capabilities = {};
		struct VideoCapability
		{
			VkVideoProfileInfoKHR profile = {};
			VkVideoDecodeCapabilitiesKHR decode_capabilities = {};
			VkVideoCapabilitiesKHR video_capabilities = {};
		};
		VideoCapability video_capability_h264 = {};

		wi::vector<VkDynamicState> pso_dynamicStates;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
		VkPipelineDynamicStateCreateInfo dynamicStateInfo_MeshShader = {};

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
			VkSemaphore frame_semaphores[BUFFERCOUNT][QUEUE_COUNT] = {};
			wi::vector<SwapChain> swapchain_updates;
			wi::vector<VkSwapchainKHR> submit_swapchains;
			wi::vector<uint32_t> submit_swapChainImageIndices;
			wi::vector<VkSemaphoreSubmitInfo> submit_waitSemaphoreInfos;
			wi::vector<VkSemaphore> submit_signalSemaphores;
			wi::vector<VkSemaphoreSubmitInfo> submit_signalSemaphoreInfos;
			wi::vector<VkCommandBufferSubmitInfo> submit_cmds;

			bool sparse_binding_supported = false;
			std::shared_ptr<std::mutex> locker;

			void signal(VkSemaphore semaphore);
			void wait(VkSemaphore semaphore);
			void submit(GraphicsDevice_Vulkan* device, VkFence fence);

		} queues[QUEUE_COUNT];

		CommandQueue queue_init;
		CommandQueue queue_sparse;

		struct CopyAllocator
		{
			GraphicsDevice_Vulkan* device = nullptr;
			std::mutex locker;

			struct CopyCMD
			{
				VkCommandPool transferCommandPool = VK_NULL_HANDLE;
				VkCommandBuffer transferCommandBuffer = VK_NULL_HANDLE;
				VkCommandPool transitionCommandPool = VK_NULL_HANDLE;
				VkCommandBuffer transitionCommandBuffer = VK_NULL_HANDLE;
				VkFence fence = VK_NULL_HANDLE;
				VkSemaphore semaphore = VK_NULL_HANDLE;
				GPUBuffer uploadbuffer;
				constexpr bool IsValid() const { return transferCommandBuffer != VK_NULL_HANDLE; }
			};
			wi::vector<CopyCMD> freelist;

			void init(GraphicsDevice_Vulkan* device);
			void destroy();
			CopyCMD allocate(uint64_t staging_size);
			void submit(CopyCMD cmd);
		};
		mutable CopyAllocator copyAllocator;

		VkFence frame_fence[BUFFERCOUNT][QUEUE_COUNT] = {};

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

		wi::vector<VkSemaphore> semaphore_pool;
		std::mutex semaphore_pool_locker;
		VkSemaphore new_semaphore()
		{
			std::scoped_lock lck(semaphore_pool_locker);
			if (semaphore_pool.empty())
			{
				VkSemaphore& sema = semaphore_pool.emplace_back();
				VkSemaphoreCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				vulkan_check(vkCreateSemaphore(device, &info, nullptr, &sema));
				set_semaphore_name(sema, "DependencySemaphore");
			}
			VkSemaphore semaphore = semaphore_pool.back();
			semaphore_pool.pop_back();
			return semaphore;
		}
		void free_semaphore(VkSemaphore semaphore)
		{
			std::scoped_lock lck(semaphore_pool_locker);
			semaphore_pool.push_back(semaphore);
		}

		struct CommandList_Vulkan
		{
			VkCommandPool commandPools[BUFFERCOUNT][QUEUE_COUNT] = {};
			VkCommandBuffer commandBuffers[BUFFERCOUNT][QUEUE_COUNT] = {};
			uint32_t buffer_index = 0;

			QUEUE_TYPE queue = {};
			uint32_t id = 0;
			wi::vector<VkSemaphore> waits;
			wi::vector<VkSemaphore> signals;

			DescriptorBinder binder;
			DescriptorBinderPool binder_pools[BUFFERCOUNT];
			GPULinearAllocator frame_allocators[BUFFERCOUNT];

			wi::vector<std::pair<PipelineHash, VkPipeline>> pipelines_worker;
			PipelineHash prev_pipeline_hash = {};
			const PipelineState* active_pso = {};
			const Shader* active_cs = {};
			const RaytracingPipelineState* active_rt = {};
			ShadingRate prev_shadingrate = {};
			uint32_t prev_stencilref = 0;
			wi::vector<SwapChain> prev_swapchains;
			bool dirty_pso = {};
			wi::vector<VkMemoryBarrier2> frame_memoryBarriers;
			wi::vector<VkImageMemoryBarrier2> frame_imageBarriers;
			wi::vector<VkBufferMemoryBarrier2> frame_bufferBarriers;
			wi::vector<VkAccelerationStructureGeometryKHR> accelerationstructure_build_geometries;
			wi::vector<VkAccelerationStructureBuildRangeInfoKHR> accelerationstructure_build_ranges;
			RenderPassInfo renderpass_info;
			wi::vector<VkImageMemoryBarrier2> renderpass_barriers_begin;
			wi::vector<VkImageMemoryBarrier2> renderpass_barriers_end;

			void reset(uint32_t bufferindex)
			{
				buffer_index = bufferindex;
				waits.clear();
				signals.clear();
				binder_pools[buffer_index].reset();
				binder.reset();
				frame_allocators[buffer_index].reset();
				prev_pipeline_hash = {};
				active_pso = nullptr;
				active_cs = nullptr;
				active_rt = nullptr;
				dirty_pso = false;
				prev_shadingrate = ShadingRate::RATE_INVALID;
				prev_stencilref = 0;
				prev_swapchains.clear();
				renderpass_info = {};
				renderpass_barriers_begin.clear();
				renderpass_barriers_end.clear();
			}

			constexpr VkCommandPool GetCommandPool() const
			{
				return commandPools[buffer_index][queue];
			}
			constexpr VkCommandBuffer GetCommandBuffer() const
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
		mutable wi::unordered_map<PSOLayoutHash, PSOLayout> pso_layout_cache;
		mutable std::mutex pso_layout_cache_mutex;

		VkPipelineCache pipelineCache = VK_NULL_HANDLE;
		wi::unordered_map<PipelineHash, VkPipeline> pipelines_global;

		void pso_validate(CommandList cmd);

		void predraw(CommandList cmd);
		void predispatch(CommandList cmd);

		static constexpr uint32_t immutable_sampler_slot_begin = 100;
		wi::vector<VkSampler> immutable_samplers;

		void set_fence_name(VkFence fence, const char* name);
		void set_semaphore_name(VkSemaphore semaphore, const char* name);

	public:
		GraphicsDevice_Vulkan(wi::platform::window_type window, ValidationMode validationMode = ValidationMode::Disabled, GPUPreference preference = GPUPreference::Discrete);
		~GraphicsDevice_Vulkan() override;

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
			if (has_flag(desc->misc_flags, ResourceMiscFlag::BUFFER_RAW) || has_flag(desc->misc_flags, ResourceMiscFlag::BUFFER_STRUCTURED))
			{
				alignment = std::max(alignment, properties2.properties.limits.minStorageBufferOffsetAlignment);
			}
			if (desc->format != Format::UNKNOWN || has_flag(desc->misc_flags, ResourceMiscFlag::TYPED_FORMAT_CASTING))
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
		void RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags = RenderPassFlags::NONE) override;
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

		VkDevice GetDevice();
		VkImage GetTextureInternalResource(const Texture* texture);
		VkPhysicalDevice GetPhysicalDevice();
		VkInstance GetInstance();
		VkQueue GetGraphicsCommandQueue();
		uint32_t GetGraphicsFamilyIndex();

		struct AllocationHandler
		{
			VmaAllocator allocator = VK_NULL_HANDLE;
			VmaAllocator externalAllocator = VK_NULL_HANDLE;
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

				void init(GraphicsDevice_Vulkan* device, VkDescriptorType type, uint32_t descriptorCount)
				{
					descriptorCount = std::min(descriptorCount, 500000u);

					VkDescriptorPoolSize poolSize = {};
					poolSize.type = type;
					poolSize.descriptorCount = descriptorCount;

					VkDescriptorPoolCreateInfo poolInfo = {};
					poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
					poolInfo.poolSizeCount = 1;
					poolInfo.pPoolSizes = &poolSize;
					poolInfo.maxSets = 1;
					poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

					vulkan_check(vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool));

#if 0
					VkDebugUtilsObjectNameInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
					info.pObjectName = "BindlessDescriptorHeap";
					info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
					info.objectHandle = (uint64_t)descriptorPool;
					res = vkSetDebugUtilsObjectNameEXT(device->device, &info);
					vulkan_check(res);
#endif

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

					vulkan_check(vkCreateDescriptorSetLayout(device->device, &layoutInfo, nullptr, &descriptorSetLayout));

					VkDescriptorSetAllocateInfo allocInfo = {};
					allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					allocInfo.descriptorPool = descriptorPool;
					allocInfo.descriptorSetCount = 1;
					allocInfo.pSetLayouts = &descriptorSetLayout;
					vulkan_check(vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet));

					for (int i = 0; i < (int)descriptorCount; ++i)
					{
						freelist.push_back((int)descriptorCount - i - 1);
					}

					if (type != VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
					{
						// Descriptor safety feature:
						//	We init null descriptors for bindless index = 0 for access safety
						//	Because shader compiler sometimes incorrectly loads descriptor outside of safety branch
						//	Note: these are never freed, this is intentional
						int index = allocate();
						wilog_assert(index == 0, "Descriptor safety feature error: descriptor index must be 0!");
						VkWriteDescriptorSet write = {};
						write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						write.descriptorType = type;
						write.dstBinding = 0;
						write.dstArrayElement = index;
						write.descriptorCount = 1;
						write.dstSet = descriptorSet;

						VkDescriptorImageInfo image_info = {};
						VkDescriptorBufferInfo buffer_info = {};

						switch (type)
						{
						case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
							image_info.imageView = device->nullImageView2D;
							image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
							write.pImageInfo = &image_info;
							break;
						case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
						case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
							write.pTexelBufferView = &device->nullBufferView;
							break;
						case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
							buffer_info.buffer = device->nullBuffer;
							buffer_info.range = VK_WHOLE_SIZE;
							write.pBufferInfo = &buffer_info;
							break;
						case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
							image_info.imageView = device->nullImageView2D;
							image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
							write.pImageInfo = &image_info;
							break;
						case VK_DESCRIPTOR_TYPE_SAMPLER:
							image_info.sampler = device->nullSampler;
							write.pImageInfo = &image_info;
							break;
						default:
							wilog_assert(0, "Descriptor safety feature error: descriptor type not handled!");
							break;
						}
						vkUpdateDescriptorSets(device->device, 1, &write, 0, nullptr);
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
			std::deque<std::pair<VkQueryPool, uint64_t>> destroyer_querypools;
			std::deque<std::pair<VkSwapchainKHR, uint64_t>> destroyer_swapchains;
			std::deque<std::pair<VkSurfaceKHR, uint64_t>> destroyer_surfaces;
			std::deque<std::pair<VkSemaphore, uint64_t>> destroyer_semaphores;
			std::deque<std::pair<VkVideoSessionKHR, uint64_t>> destroyer_video_sessions;
			std::deque<std::pair<VkVideoSessionParametersKHR, uint64_t>> destroyer_video_session_parameters;
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
				Update(~0ull, 0); // destroy all remaining
				vmaDestroyAllocator(allocator);
				vmaDestroyAllocator(externalAllocator);
				vkDestroyDevice(device, nullptr);
				vkDestroyInstance(instance, nullptr);
			}

			// Deferred destroy of resources that the GPU is already finished with:
			void Update(uint64_t FRAMECOUNT, uint32_t BUFFERCOUNT)
			{
				const auto destroy = [&](auto&& queue, auto&& handler) {
					while (!queue.empty()) {
						if (queue.front().second + BUFFERCOUNT < FRAMECOUNT)
						{
							auto item = queue.front();
							queue.pop_front();
							handler(item.first);
						}
						else
						{
							break;
						}
					}
				};

				framecount = FRAMECOUNT;

				std::scoped_lock lck(destroylocker);

				destroy(destroyer_allocations, [&](auto& item) {
					vmaFreeMemory(allocator, item);
				});
				destroy(destroyer_images, [&](auto& item) {
					vmaDestroyImage(allocator, item.first, item.second);
				});
				destroy(destroyer_imageviews, [&](auto& item) {
					vkDestroyImageView(device, item, nullptr);
				});
				destroy(destroyer_buffers, [&](auto& item) {
					vmaDestroyBuffer(allocator, item.first, item.second);
				});
				destroy(destroyer_bufferviews, [&](auto& item) {
					vkDestroyBufferView(device, item, nullptr);
				});
				destroy(destroyer_bvhs, [&](auto& item) {
					vkDestroyAccelerationStructureKHR(device, item, nullptr);
				});
				destroy(destroyer_samplers, [&](auto& item) {
					vkDestroySampler(device, item, nullptr);
				});
				destroy(destroyer_descriptorPools, [&](auto& item) {
					vkDestroyDescriptorPool(device, item, nullptr);
				});
				destroy(destroyer_descriptorSetLayouts, [&](auto& item) {
					vkDestroyDescriptorSetLayout(device, item, nullptr);
				});
				destroy(destroyer_descriptorUpdateTemplates, [&](auto& item) {
					vkDestroyDescriptorUpdateTemplate(device, item, nullptr);
				});
				destroy(destroyer_shadermodules, [&](auto& item) {
					vkDestroyShaderModule(device, item, nullptr);
				});
				destroy(destroyer_pipelineLayouts, [&](auto& item) {
					vkDestroyPipelineLayout(device, item, nullptr);
				});
				destroy(destroyer_pipelines, [&](auto& item) {
					vkDestroyPipeline(device, item, nullptr);
				});
				destroy(destroyer_querypools, [&](auto& item) {
					vkDestroyQueryPool(device, item, nullptr);
				});
				destroy(destroyer_swapchains, [&](auto& item) {
					vkDestroySwapchainKHR(device, item, nullptr);
				});
				destroy(destroyer_surfaces, [&](auto& item) {
					vkDestroySurfaceKHR(instance, item, nullptr);
				});
				destroy(destroyer_semaphores, [&](auto& item) {
					vkDestroySemaphore(device, item, nullptr);
				});
				destroy(destroyer_video_sessions, [&](auto& item) {
					vkDestroyVideoSessionKHR(device, item, nullptr);
				});
				destroy(destroyer_video_session_parameters, [&](auto& item) {
					vkDestroyVideoSessionParametersKHR(device, item, nullptr);
				});
				destroy(destroyer_bindlessSampledImages, [&](auto& item) {
					bindlessSampledImages.free(item);
				});
				destroy(destroyer_bindlessUniformTexelBuffers, [&](auto& item) {
					bindlessUniformTexelBuffers.free(item);
				});
				destroy(destroyer_bindlessStorageBuffers, [&](auto& item) {
					bindlessStorageBuffers.free(item);
				});
				destroy(destroyer_bindlessStorageImages, [&](auto& item) {
					bindlessStorageImages.free(item);
				});
				destroy(destroyer_bindlessStorageTexelBuffers, [&](auto& item) {
					bindlessStorageTexelBuffers.free(item);
				});
				destroy(destroyer_bindlessSamplers, [&](auto& item) {
					bindlessSamplers.free(item);
				});
				destroy(destroyer_bindlessAccelerationStructures, [&](auto& item) {
					bindlessAccelerationStructures.free(item);
				});
			}
		};
		std::shared_ptr<AllocationHandler> allocationhandler;
	};
}

#endif // WICKEDENGINE_BUILD_VULKAN
