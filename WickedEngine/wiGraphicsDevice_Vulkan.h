#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiWindowRegistration.h"
#include "wiSpinLock.h"
#include "wiContainers.h"

#ifdef WICKEDENGINE_BUILD_VULKAN
#include "wiGraphicsDevice_SharedInternals.h"

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // WIN32

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
		VmaAllocator allocator;

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
		VkBufferView	nullBufferView;
		VkImage			nullImage;
		VkImageView		nullImageView;
		VkSampler		nullSampler;


		struct FrameResources
		{
			VkFence frameFence;
			VkCommandPool commandPools[COMMANDLIST_COUNT];
			VkCommandBuffer commandBuffers[COMMANDLIST_COUNT];
			VkImageView swapChainImageView;
			VkFramebuffer swapChainFramebuffer;

			VkCommandPool transitionCommandPool;
			VkCommandBuffer transitionCommandBuffer;
			std::vector<VkImageMemoryBarrier> loadedimagetransitions;

			struct DescriptorTableFrameAllocator
			{
				GraphicsDevice_Vulkan* device;
				VkDescriptorPool descriptorPool;

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

				struct Table
				{
					const GPUBuffer* CBV[GPU_RESOURCE_HEAP_CBV_COUNT];
					const GPUResource* SRV[GPU_RESOURCE_HEAP_SRV_COUNT];
					int SRV_index[GPU_RESOURCE_HEAP_SRV_COUNT];
					const GPUResource* UAV[GPU_RESOURCE_HEAP_UAV_COUNT];
					int UAV_index[GPU_RESOURCE_HEAP_UAV_COUNT];
					const Sampler* SAM[GPU_SAMPLER_HEAP_COUNT];

					std::vector<VkDescriptorSet> descriptorSet_GPU;
					UINT ringOffset;
					bool dirty;

					void reset()
					{
						memset(CBV, 0, sizeof(CBV));
						memset(SRV, 0, sizeof(SRV));
						memset(SRV_index, -1, sizeof(SRV_index));
						memset(UAV, 0, sizeof(UAV));
						memset(UAV_index, -1, sizeof(UAV_index));
						memset(SAM, 0, sizeof(SAM));
						ringOffset = 0;
						dirty = true;
					}

				} tables[SHADERSTAGE_COUNT];

				DescriptorTableFrameAllocator(GraphicsDevice_Vulkan* device, UINT maxRenameCount);
				~DescriptorTableFrameAllocator();

				void reset();
				void validate(CommandList cmd);
			};
			DescriptorTableFrameAllocator*		descriptors[COMMANDLIST_COUNT];


			struct ResourceFrameAllocator
			{
				GraphicsDevice_Vulkan*	device = nullptr;
				GPUBuffer				buffer;
				VmaAllocation			allocation;
				uint8_t*				dataBegin = nullptr;
				uint8_t*				dataCur = nullptr;
				uint8_t*				dataEnd = nullptr;

				ResourceFrameAllocator(GraphicsDevice_Vulkan* device, size_t size);
				~ResourceFrameAllocator();

				uint8_t* allocate(size_t dataSize, size_t alignment);
				void clear();
				uint64_t calculateOffset(uint8_t* address);
			};
			ResourceFrameAllocator* resourceBuffer[COMMANDLIST_COUNT];

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

			UploadBuffer(GraphicsDevice_Vulkan* device, const QueueFamilyIndices& queueIndices, size_t size);
			~UploadBuffer();

			uint8_t* allocate(size_t dataSize, size_t alignment);
			void clear();
			uint64_t calculateOffset(uint8_t* address);
		};
		UploadBuffer* bufferUploader;
		UploadBuffer* textureUploader;

		std::unordered_map<size_t, VkPipeline> pipelines_global;
		std::vector<std::pair<size_t, VkPipeline>> pipelines_worker[COMMANDLIST_COUNT];
		size_t prev_pipeline_hash[COMMANDLIST_COUNT] = {};
		const RenderPass* active_renderpass[COMMANDLIST_COUNT] = {};

		std::unordered_map<wiCPUHandle, VmaAllocation> vma_allocations;

		std::atomic<uint8_t> commandlist_count{ 0 };
		wiContainers::ThreadSafeRingBuffer<CommandList, COMMANDLIST_COUNT> free_commandlists;
		wiContainers::ThreadSafeRingBuffer<CommandList, COMMANDLIST_COUNT> active_commandlists;

		struct DestroyItem
		{
			enum TYPE
			{
				IMAGE,
				IMAGEVIEW,
				BUFFER,
				BUFFERVIEW,
				SAMPLER,
				PIPELINE,
				RENDERPASS,
				FRAMEBUFFER,
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

	public:
		GraphicsDevice_Vulkan(wiWindowRegistration::window_type window, bool fullscreen = false, bool debuglayer = false);
		virtual ~GraphicsDevice_Vulkan();

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

#endif // WICKEDENGINE_BUILD_VULKAN
