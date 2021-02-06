#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"

#ifndef PLATFORM_UWP
#define WICKEDENGINE_BUILD_VULKAN
#endif // PLATFORM_UWP

#ifdef WICKEDENGINE_BUILD_VULKAN
#include "wiGraphicsDevice.h"
#include "wiSpinLock.h"
#include "wiContainers.h"
#include "wiGraphicsDevice_SharedInternals.h"

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // _WIN32

#define VK_NO_PROTOTYPES
#include "Utility/vulkan/vulkan.h"
#include "Utility/volk.h"
#include "Utility/vk_mem_alloc.h"

#include <vector>
#include <unordered_map>
#include <deque>
#include <atomic>
#include <mutex>
#include <algorithm>

namespace wiGraphics
{
	class GraphicsDevice_Vulkan : public GraphicsDevice
	{
	protected:
		VkInstance instance = VK_NULL_HANDLE;
	    VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		int graphicsFamily = -1;
		int presentFamily = -1;
		int copyFamily = -1;
		int computeFamily = -1;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
		VkQueue computeQueue = VK_NULL_HANDLE;

		VkQueue copyQueue = VK_NULL_HANDLE;
		std::mutex copyQueueLock;
		bool copyQueueUse = false;
		VkSemaphore copySemaphore = VK_NULL_HANDLE;

		VkPhysicalDeviceProperties2 properties2 = {};
		VkPhysicalDeviceVulkan11Properties properties_1_1 = {};
		VkPhysicalDeviceVulkan12Properties properties_1_2 = {};
		VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties = {};
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracing_properties = {};
		VkPhysicalDeviceFragmentShadingRatePropertiesKHR fragment_shading_rate_properties = {};
		VkPhysicalDeviceMeshShaderPropertiesNV mesh_shader_properties = {};

		VkPhysicalDeviceFeatures2 features2 = {};
		VkPhysicalDeviceVulkan11Features features_1_1 = {};
		VkPhysicalDeviceVulkan12Features features_1_2 = {};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {};
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_features = {};
		VkPhysicalDeviceRayQueryFeaturesKHR raytracing_query_features = {};
		VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragment_shading_rate_features = {};
		VkPhysicalDeviceMeshShaderFeaturesNV mesh_shader_features = {};

		VkSurfaceCapabilitiesKHR swapchain_capabilities;
		std::vector<VkSurfaceFormatKHR> swapchain_formats;
		std::vector<VkPresentModeKHR> swapchain_presentModes;

		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		uint32_t swapChainImageIndex = 0;
		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;

		std::vector<VkDynamicState> pso_dynamicStates;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};

		VkRenderPass defaultRenderPass = VK_NULL_HANDLE;

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

		uint64_t timestamp_frequency = 0;

		void CreateBackBufferResources();

		struct FrameResources
		{
			VkFence frameFence = VK_NULL_HANDLE;
			VkCommandPool commandPools[COMMANDLIST_COUNT] = {};
			VkCommandBuffer commandBuffers[COMMANDLIST_COUNT] = {};

			VkCommandPool copyCommandPool = VK_NULL_HANDLE;
			VkCommandBuffer copyCommandBuffer = VK_NULL_HANDLE;

			VkCommandPool transitionCommandPool = VK_NULL_HANDLE;
			VkCommandBuffer transitionCommandBuffer = VK_NULL_HANDLE;
			std::vector<VkImageMemoryBarrier> loadedimagetransitions;

			VkSemaphore swapchainAcquireSemaphore = VK_NULL_HANDLE;
			VkSemaphore swapchainReleaseSemaphore = VK_NULL_HANDLE;

			struct DescriptorBinder
			{
				GraphicsDevice_Vulkan* device;
				VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
				uint32_t poolSize = 256;

				std::vector<VkWriteDescriptorSet> descriptorWrites;
				std::vector<VkDescriptorBufferInfo> bufferInfos;
				std::vector<VkDescriptorImageInfo> imageInfos;
				std::vector<VkBufferView> texelBufferViews;
				std::vector<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructureViews;
				bool dirty = false;

				const GPUBuffer* CBV[GPU_RESOURCE_HEAP_CBV_COUNT];
				const GPUResource* SRV[GPU_RESOURCE_HEAP_SRV_COUNT];
				int SRV_index[GPU_RESOURCE_HEAP_SRV_COUNT];
				const GPUResource* UAV[GPU_RESOURCE_HEAP_UAV_COUNT];
				int UAV_index[GPU_RESOURCE_HEAP_UAV_COUNT];
				const Sampler* SAM[GPU_SAMPLER_HEAP_COUNT];

				void init(GraphicsDevice_Vulkan* device);
				void destroy();

				void reset();
				void validate(bool graphics, CommandList cmd, bool raytracing = false);
				VkDescriptorSet commit(const DescriptorTable* table);
			};
			DescriptorBinder descriptors[COMMANDLIST_COUNT];


			struct ResourceFrameAllocator
			{
				GraphicsDevice_Vulkan*	device = nullptr;
				GPUBuffer				buffer;
				uint8_t*				dataBegin = nullptr;
				uint8_t*				dataCur = nullptr;
				uint8_t*				dataEnd = nullptr;

				void init(GraphicsDevice_Vulkan* device, size_t size);

				uint8_t* allocate(size_t dataSize, size_t alignment);
				void clear();
				uint64_t calculateOffset(uint8_t* address);
			};
			ResourceFrameAllocator resourceBuffer[COMMANDLIST_COUNT];
		};
		FrameResources frames[BACKBUFFER_COUNT];
		FrameResources& GetFrameResources() { return frames[GetFrameCount() % BACKBUFFER_COUNT]; }
		inline VkCommandBuffer GetDirectCommandList(CommandList cmd) { return GetFrameResources().commandBuffers[cmd]; }

		std::vector<VkMemoryBarrier> frame_memoryBarriers[COMMANDLIST_COUNT];
		std::vector<VkImageMemoryBarrier> frame_imageBarriers[COMMANDLIST_COUNT];
		std::vector<VkBufferMemoryBarrier> frame_bufferBarriers[COMMANDLIST_COUNT];

		struct PSOLayout
		{
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		};
		std::unordered_map<size_t, PSOLayout> pso_layout_cache;
		std::mutex pso_layout_cache_mutex;

		std::unordered_map<size_t, VkPipeline> pipelines_global;
		std::vector<std::pair<size_t, VkPipeline>> pipelines_worker[COMMANDLIST_COUNT];
		size_t prev_pipeline_hash[COMMANDLIST_COUNT] = {};
		const PipelineState* active_pso[COMMANDLIST_COUNT] = {};
		const Shader* active_cs[COMMANDLIST_COUNT] = {};
		const RaytracingPipelineState* active_rt[COMMANDLIST_COUNT] = {};
		const RenderPass* active_renderpass[COMMANDLIST_COUNT] = {};
		SHADING_RATE prev_shadingrate[COMMANDLIST_COUNT] = {};

		bool dirty_pso[COMMANDLIST_COUNT] = {};
		void pso_validate(CommandList cmd);

		void barrier_flush(CommandList cmd);
		void predraw(CommandList cmd);
		void predispatch(CommandList cmd);
		void preraytrace(CommandList cmd);

		std::atomic<CommandList> cmd_count{ 0 };

		std::vector<StaticSampler> common_samplers;

	public:
		GraphicsDevice_Vulkan(wiPlatform::window_type window, bool fullscreen = false, bool debuglayer = false);
		virtual ~GraphicsDevice_Vulkan();

		bool CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer) override;
		bool CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture) override;
		bool CreateShader(SHADERSTAGE stage, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader) override;
		bool CreateSampler(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) override;
		bool CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery) override;
		bool CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso) override;
		bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) override;
		bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh) override;
		bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso) override;
		bool CreateDescriptorTable(DescriptorTable* table) override;
		bool CreateRootSignature(RootSignature* rootsig) override;

		int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) override;
		int CreateSubresource(GPUBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size = ~0) override;

		void WriteShadingRateValue(SHADING_RATE rate, void* dest) override;
		void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) override;
		void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) override;
		void WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const GPUResource* resource, int subresource = -1, uint64_t offset = 0) override;
		void WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const Sampler* sampler) override;

		void Map(const GPUResource* resource, Mapping* mapping) override;
		void Unmap(const GPUResource* resource) override;
		bool QueryRead(const GPUQuery* query, GPUQueryResult* result) override;

		void SetCommonSampler(const StaticSampler* sam) override;

		void SetName(GPUResource* pResource, const char* name) override;

		void PresentBegin(CommandList cmd) override;
		void PresentEnd(CommandList cmd) override;

		CommandList BeginCommandList() override;
		void SubmitCommandLists() override;

		void WaitForGPU() override;
		void ClearPipelineStateCache() override;

		void SetResolution(int width, int height) override;

		Texture GetBackBuffer() override;

		void SetVSyncEnabled(bool value) override { VSYNC = value; CreateBackBufferResources(); };

		///////////////Thread-sensitive////////////////////////

		void RenderPassBegin(const RenderPass* renderpass, CommandList cmd) override;
		void RenderPassEnd(CommandList cmd) override;
		void BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) override;
		void BindViewports(uint32_t NumViewports, const Viewport *pViewports, CommandList cmd) override;
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
		void QueryBegin(const GPUQuery *query, CommandList cmd) override;
		void QueryEnd(const GPUQuery *query, CommandList cmd) override;
		void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) override;
		void BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src = nullptr) override;
		void BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd) override;
		void DispatchRays(const DispatchRaysDesc* desc, CommandList cmd) override;

		void BindDescriptorTable(BINDPOINT bindpoint, uint32_t space, const DescriptorTable* table, CommandList cmd) override;
		void BindRootDescriptor(BINDPOINT bindpoint, uint32_t index, const GPUBuffer* buffer, uint32_t offset, CommandList cmd) override;
		void BindRootConstants(BINDPOINT bindpoint, uint32_t index, const void* srcdata, CommandList cmd) override;

		GPUAllocation AllocateGPU(size_t dataSize, CommandList cmd) override;

		void EventBegin(const char* name, CommandList cmd) override;
		void EventEnd(CommandList cmd) override;
		void SetMarker(const char* name, CommandList cmd) override;


		struct AllocationHandler
		{
			VmaAllocator allocator = VK_NULL_HANDLE;
			VkDevice device = VK_NULL_HANDLE;
			VkInstance instance;
			uint64_t framecount = 0;
			std::mutex destroylocker;

			struct QueryAllocator
			{
				AllocationHandler* allocationhandler = nullptr;
				std::mutex locker;
				VkQueryPoolCreateInfo poolInfo = {};

				std::vector<VkQueryPool> blocks;

				struct Query
				{
					uint32_t block = ~0;
					uint32_t index = ~0;
				};
				std::vector<Query> freelist;

				void init(AllocationHandler* allocationhandler, VkQueryType type)
				{
					this->allocationhandler = allocationhandler;

					poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
					poolInfo.queryCount = 1024;
					poolInfo.queryType = type;
				}
				void destroy()
				{
					for (auto& x : blocks)
					{
						vkDestroyQueryPool(allocationhandler->device, x, nullptr);
					}
				}
				void block_allocate()
				{
					uint32_t block_index = (uint32_t)blocks.size();
					blocks.emplace_back();
					auto& block = blocks.back();
					VkResult res = vkCreateQueryPool(allocationhandler->device, &poolInfo, nullptr, &block);
					assert(res == VK_SUCCESS);
					vkResetQueryPool(allocationhandler->device, block, 0, poolInfo.queryCount);
					for (uint32_t i = 0; i < poolInfo.queryCount; ++i)
					{
						freelist.emplace_back();
						freelist.back().block = block_index;
						freelist.back().index = i;
					}
				}
				Query allocate()
				{
					locker.lock();
					if (freelist.empty())
					{
						block_allocate();
					}
					assert(!freelist.empty());
					auto query = freelist.back();
					freelist.pop_back();
					locker.unlock();
					return query;
				}
				void free(Query query)
				{
					locker.lock();
					freelist.push_back(query);
					locker.unlock();
				}
			};
			QueryAllocator queries_timestamp;
			QueryAllocator queries_occlusion;

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
			std::deque<std::pair<QueryAllocator::Query, uint64_t>> destroyer_queries_occlusion;
			std::deque<std::pair<QueryAllocator::Query, uint64_t>> destroyer_queries_timestamp;

			~AllocationHandler()
			{
				Update(~0, 0); // destroy all remaining
				queries_occlusion.destroy();
				queries_timestamp.destroy();
				vmaDestroyAllocator(allocator);
				vkDestroyDevice(device, nullptr);
				vkDestroyInstance(instance, nullptr);
			}

			// Deferred destroy of resources that the GPU is already finished with:
			void Update(uint64_t FRAMECOUNT, uint32_t BACKBUFFER_COUNT)
			{
				destroylocker.lock();
				framecount = FRAMECOUNT;
				while (!destroyer_images.empty())
				{
					if (destroyer_images.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_imageviews.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_buffers.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_bufferviews.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_bvhs.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_samplers.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_descriptorPools.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_descriptorSetLayouts.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_descriptorUpdateTemplates.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_shadermodules.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_pipelineLayouts.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_pipelines.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_renderpasses.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
					if (destroyer_framebuffers.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
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
				while (!destroyer_queries_occlusion.empty())
				{
					if (destroyer_queries_occlusion.front().second + BACKBUFFER_COUNT < FRAMECOUNT)
					{
						auto item = destroyer_queries_occlusion.front();
						destroyer_queries_occlusion.pop_front();
						queries_occlusion.free(item.first);
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
						queries_timestamp.free(item.first);
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
