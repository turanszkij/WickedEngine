#pragma once

#if __has_include("vulkan/vulkan.h")
#define WICKEDENGINE_BUILD_VULKAN
#endif // HAS VULKAN

#ifdef WICKEDENGINE_BUILD_VULKAN
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiPlatform.h"
#include "wiSpinLock.h"
#include "wiContainers.h"
#include "wiGraphicsDevice_SharedInternals.h"

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // _WIN32

#include <vulkan/vulkan.h>

#include "Utility/vk_mem_alloc.h"

#include <vector>
#include <unordered_map>
#include <deque>
#include <atomic>
#include <mutex>
#include <algorithm>

namespace wiGraphics
{
	struct FrameResources;
	struct DescriptorTableFrameAllocator;

	struct QueueFamilyIndices {
		int graphicsFamily = -1;
		int presentFamily = -1;
		int copyFamily = -1;

		bool isComplete() {
			return graphicsFamily >= 0 && presentFamily >= 0 && copyFamily >= 0;
		}
	};

	class GraphicsDevice_Vulkan : public GraphicsDevice
	{
		friend struct DescriptorTableFrameAllocator;
	private:

		VkInstance instance;
		VkDebugReportCallbackEXT callback;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device;
		QueueFamilyIndices queueIndices;
		VkQueue graphicsQueue;
		VkQueue presentQueue;

		VkPhysicalDeviceProperties physicalDeviceProperties;

		VkQueue copyQueue;
		VkCommandPool copyCommandPool;
		VkCommandBuffer copyCommandBuffer;
		VkFence copyFence;
		std::mutex copyQueueLock;

		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;

		VkSwapchainKHR swapChain;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImage> swapChainImages;

		VkRenderPass defaultRenderPass;
		VkPipelineLayout defaultPipelineLayout_Graphics;
		VkPipelineLayout defaultPipelineLayout_Compute;
		VkDescriptorSetLayout defaultDescriptorSetlayouts[SHADERSTAGE_COUNT];
		uint32_t descriptorCount;

		VkBuffer		nullBuffer;
		VmaAllocation	nullBufferAllocation;
		VkBufferView	nullBufferView;
		VkImage			nullImage;
		VmaAllocation	nullImageAllocation;
		VkImageView		nullImageView;
		VkSampler		nullSampler;

		uint64_t timestamp_frequency = 0;
		VkQueryPool querypool_timestamp;
		VkQueryPool querypool_occlusion;
		static const size_t timestamp_query_count = 1024;
		static const size_t occlusion_query_count = 1024;
		bool initial_querypool_reset = false;
		std::vector<uint32_t> timestamps_to_reset;
		std::vector<uint32_t> occlusions_to_reset;


		struct FrameResources
		{
			VkFence frameFence;
			VkCommandPool commandPools[COMMANDLIST_COUNT] = {};
			VkCommandBuffer commandBuffers[COMMANDLIST_COUNT] = {};
			VkImageView swapChainImageView;
			VkFramebuffer swapChainFramebuffer;

			VkCommandPool transitionCommandPool;
			VkCommandBuffer transitionCommandBuffer;
			std::vector<VkImageMemoryBarrier> loadedimagetransitions;

			struct DescriptorTableFrameAllocator
			{
				GraphicsDevice_Vulkan* device;

				static const int writeCapacity =
					1 +									// null		CBV
					3 +									// null		SRV (texture + typedbuffer + untypedbuffer)
					3 +									// null		UAV (texture + typedbuffer + untypedbuffer)
					1 +									// null		SAMPLER
					GPU_RESOURCE_HEAP_CBV_COUNT +		// valid	CBV
					GPU_RESOURCE_HEAP_SRV_COUNT * 3 +	// valid	SRV (texture + typedbuffer + untypedbuffer)
					GPU_RESOURCE_HEAP_UAV_COUNT * 3 +	// valid	UAV (texture + typedbuffer + untypedbuffer)
					GPU_SAMPLER_HEAP_COUNT;				// valid	SAMPLER
				VkWriteDescriptorSet descriptorWrites[writeCapacity];
				VkDescriptorBufferInfo bufferInfos[writeCapacity];
				VkDescriptorImageInfo imageInfos[writeCapacity];
				VkBufferView texelBufferViews[writeCapacity];

				static const int max_null_resource_capacity = std::max(GPU_RESOURCE_HEAP_CBV_COUNT, std::max(GPU_RESOURCE_HEAP_SRV_COUNT, GPU_RESOURCE_HEAP_UAV_COUNT));
				VkDescriptorBufferInfo null_bufferInfos[max_null_resource_capacity];
				VkDescriptorImageInfo null_imageInfos[max_null_resource_capacity];
				VkBufferView null_texelBufferViews[max_null_resource_capacity];
				VkDescriptorImageInfo null_samplerInfos[GPU_SAMPLER_HEAP_COUNT];

				struct DescriptorHeap
				{
					VkDescriptorPool descriptorPool;
					std::vector<VkDescriptorSet> descriptorSet_GPU;
					uint32_t ringOffset;
				};
				std::vector<DescriptorHeap> heaps[SHADERSTAGE_COUNT];
				size_t currentheap[SHADERSTAGE_COUNT] = {};

				struct Table
				{
					const GPUBuffer* CBV[GPU_RESOURCE_HEAP_CBV_COUNT];
					const GPUResource* SRV[GPU_RESOURCE_HEAP_SRV_COUNT];
					int SRV_index[GPU_RESOURCE_HEAP_SRV_COUNT];
					const GPUResource* UAV[GPU_RESOURCE_HEAP_UAV_COUNT];
					int UAV_index[GPU_RESOURCE_HEAP_UAV_COUNT];
					const Sampler* SAM[GPU_SAMPLER_HEAP_COUNT];

					bool dirty;

					void reset()
					{
						memset(CBV, 0, sizeof(CBV));
						memset(SRV, 0, sizeof(SRV));
						memset(SRV_index, -1, sizeof(SRV_index));
						memset(UAV, 0, sizeof(UAV));
						memset(UAV_index, -1, sizeof(UAV_index));
						memset(SAM, 0, sizeof(SAM));
						dirty = true;
					}

				} tables[SHADERSTAGE_COUNT];

				void init(GraphicsDevice_Vulkan* device);
				~DescriptorTableFrameAllocator();

				void reset();
				void validate(CommandList cmd);
				void create_heaps_on_demand(SHADERSTAGE stage);
			};
			DescriptorTableFrameAllocator descriptors[COMMANDLIST_COUNT];


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

		struct DynamicResourceState
		{
			GPUAllocation allocation;
			bool binding[SHADERSTAGE_COUNT] = {};
		};
		std::unordered_map<const GPUBuffer*, DynamicResourceState> dynamic_constantbuffers[COMMANDLIST_COUNT];

		struct UploadBuffer
		{
			GraphicsDevice_Vulkan*	device = nullptr;
			VkBuffer				resource = VK_NULL_HANDLE;
			VmaAllocation			allocation;
			uint8_t*				dataBegin = nullptr;
			uint8_t*				dataCur = nullptr;
			uint8_t*				dataEnd = nullptr;
			wiSpinLock				lock;

			void init(GraphicsDevice_Vulkan* device, const QueueFamilyIndices& queueIndices, size_t size);
			~UploadBuffer();

			uint8_t* allocate(size_t dataSize, size_t alignment);
			void clear();
			uint64_t calculateOffset(uint8_t* address);
		};
		UploadBuffer bufferUploader;
		UploadBuffer textureUploader;

		std::unordered_map<size_t, VkPipeline> pipelines_global;
		std::vector<std::pair<size_t, VkPipeline>> pipelines_worker[COMMANDLIST_COUNT];
		size_t prev_pipeline_hash[COMMANDLIST_COUNT] = {};
		const RenderPass* active_renderpass[COMMANDLIST_COUNT] = {};

		std::atomic<uint8_t> commandlist_count{ 0 };
		wiContainers::ThreadSafeRingBuffer<CommandList, COMMANDLIST_COUNT> free_commandlists;
		wiContainers::ThreadSafeRingBuffer<CommandList, COMMANDLIST_COUNT> active_commandlists;


	public:
		GraphicsDevice_Vulkan(wiPlatform::window_type window, bool fullscreen = false, bool debuglayer = false);
		virtual ~GraphicsDevice_Vulkan();

		bool CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer) override;
		bool CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture) override;
		bool CreateInputLayout(const InputLayoutDesc *pInputElementDescs, uint32_t NumElements, const Shader* shader, InputLayout *pInputLayout) override;
		bool CreateShader(SHADERSTAGE stafe, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader) override;
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
			VmaAllocator allocator = VK_NULL_HANDLE;
			VkDevice device = VK_NULL_HANDLE;
			VkInstance instance;
			uint64_t framecount = 0;
			std::mutex destroylocker;
			std::deque<std::pair<std::pair<VkImage, VmaAllocation>, uint64_t>> destroyer_images;
			std::deque<std::pair<VkImageView, uint64_t>> destroyer_imageviews;
			std::deque<std::pair<std::pair<VkBuffer, VmaAllocation>, uint64_t>> destroyer_buffers;
			std::deque<std::pair<VkBufferView, uint64_t>> destroyer_bufferviews;
			std::deque<std::pair<VkSampler, uint64_t>> destroyer_samplers;
			std::deque<std::pair<VkShaderModule, uint64_t>> destroyer_shadermodules;
			std::deque<std::pair<VkPipeline, uint64_t>> destroyer_pipelines;
			std::deque<std::pair<VkRenderPass, uint64_t>> destroyer_renderpasses;
			std::deque<std::pair<VkFramebuffer, uint64_t>> destroyer_framebuffers;
			std::deque<std::pair<uint32_t, uint64_t>> destroyer_queries_occlusion;
			std::deque<std::pair<uint32_t, uint64_t>> destroyer_queries_timestamp;

			wiContainers::ThreadSafeRingBuffer<uint32_t, timestamp_query_count> free_timestampqueries;
			wiContainers::ThreadSafeRingBuffer<uint32_t, occlusion_query_count> free_occlusionqueries;

			~AllocationHandler()
			{
				Update(~0, 0); // destroy all remaining
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
				destroylocker.unlock();
			}
		};
		std::shared_ptr<AllocationHandler> allocationhandler;
	};
}

#endif // WICKEDENGINE_BUILD_VULKAN
