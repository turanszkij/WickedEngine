#ifndef _GRAPHICSDEVICE_VULKAN_H_
#define _GRAPHICSDEVICE_VULKAN_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiWindowRegistration.h"
#include "wiSpinLock.h"

#ifdef WICKEDENGINE_BUILD_VULKAN
#include "wiGraphicsDevice_SharedInternals.h"


#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // WIN32

#ifdef WICKEDENGINE_BUILD_VULKAN
#include <vulkan/vulkan.h>
#endif // WICKEDENGINE_BUILD_VULKAN

#include <vector>
#include <unordered_map>

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
		wiSpinLock copyQueueLock;

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


		struct RenderPassManager
		{
			bool dirty = true;

			VkImageView attachments[9] = {};
			uint32_t attachmentCount = 0;
			VkExtent2D attachmentsExtents = {};
			uint32_t attachmentLayers = 0;
			VkClearValue clearColor[9] = {};

			struct RenderPassAndFramebuffer
			{
				VkRenderPass renderPass = VK_NULL_HANDLE;
				VkFramebuffer frameBuffer = VK_NULL_HANDLE;
			};
			// RTFormats hash <-> renderpass+framebuffer
			std::unordered_map<uint64_t, RenderPassAndFramebuffer> renderPassCollection;
			uint64_t activeRTHash = 0;
			const GraphicsPSODesc* pDesc = nullptr;

			VkRenderPass overrideRenderPass = VK_NULL_HANDLE;
			VkFramebuffer overrideFramebuffer = VK_NULL_HANDLE;

			struct ClearRequest
			{
				VkImageView attachment = VK_NULL_HANDLE;
				VkClearValue clearValue = {};
				uint32_t clearFlags = 0;
			};
			std::vector<ClearRequest> clearRequests;

			void reset();
			void disable(VkCommandBuffer commandBuffer);
			void validate(VkDevice device, VkCommandBuffer commandBuffer);
		};
		RenderPassManager renderPass[GRAPHICSTHREAD_COUNT];


		struct FrameResources
		{
			VkFence frameFence;
			VkCommandPool commandPools[GRAPHICSTHREAD_COUNT];
			VkCommandBuffer commandBuffers[GRAPHICSTHREAD_COUNT];
			VkImageView swapChainImageView;
			VkFramebuffer swapChainFramebuffer;

			struct DescriptorTableFrameAllocator
			{
				GraphicsDevice_Vulkan* device;
				VkDescriptorPool descriptorPool;
				VkDescriptorSet descriptorSet_CPU[SHADERSTAGE_COUNT];
				std::vector<VkDescriptorSet> descriptorSet_GPU[SHADERSTAGE_COUNT];
				UINT ringOffset[SHADERSTAGE_COUNT];
				bool dirty[SHADERSTAGE_COUNT];

				// default descriptor table contents:
				VkDescriptorBufferInfo bufferInfo[GPU_RESOURCE_HEAP_SRV_COUNT] = {};
				VkDescriptorImageInfo imageInfo[GPU_RESOURCE_HEAP_SRV_COUNT] = {};
				VkBufferView bufferViews[GPU_RESOURCE_HEAP_SRV_COUNT] = {};
				VkDescriptorImageInfo samplerInfo[GPU_SAMPLER_HEAP_COUNT] = {};
				std::vector<VkWriteDescriptorSet> initWrites[SHADERSTAGE_COUNT];

				// descriptor table rename guards:
				std::vector<wiCPUHandle> boundDescriptors[SHADERSTAGE_COUNT];

				DescriptorTableFrameAllocator(GraphicsDevice_Vulkan* device, UINT maxRenameCount);
				~DescriptorTableFrameAllocator();

				void reset();
				void update(SHADERSTAGE stage, UINT slot, VkBuffer descriptor, VkCommandBuffer commandList);
				void validate(VkCommandBuffer commandList);
			};
			DescriptorTableFrameAllocator*		ResourceDescriptorsGPU[GRAPHICSTHREAD_COUNT];


			struct ResourceFrameAllocator
			{
				VkDevice				device;
				GPUBuffer				buffer;
				uint8_t*				dataBegin;
				uint8_t*				dataCur;
				uint8_t*				dataEnd;

				ResourceFrameAllocator(VkPhysicalDevice physicalDevice, VkDevice device, size_t size);
				~ResourceFrameAllocator();

				uint8_t* allocate(size_t dataSize, size_t alignment);
				void clear();
				uint64_t calculateOffset(uint8_t* address);
			};
			ResourceFrameAllocator* resourceBuffer[GRAPHICSTHREAD_COUNT];
		};
		FrameResources frames[BACKBUFFER_COUNT];
		FrameResources& GetFrameResources() { return frames[GetFrameCount() % BACKBUFFER_COUNT]; }
		VkCommandBuffer GetDirectCommandList(GRAPHICSTHREAD threadID);


		struct UploadBuffer
		{
			VkDevice				device;
			VkBuffer				resource;
			VkDeviceMemory			resourceMemory;
			uint8_t*				dataBegin;
			uint8_t*				dataCur;
			uint8_t*				dataEnd;
			wiSpinLock				lock;

			UploadBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const QueueFamilyIndices& queueIndices, size_t size);
			~UploadBuffer();

			uint8_t* allocate(size_t dataSize, size_t alignment);
			void clear();
			uint64_t calculateOffset(uint8_t* address);
		};
		UploadBuffer* bufferUploader;
		UploadBuffer* textureUploader;


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
		HRESULT CreateGraphicsPSO(const GraphicsPSODesc* pDesc, GraphicsPSO* pso) override;
		HRESULT CreateComputePSO(const ComputePSODesc* pDesc, ComputePSO* pso) override;


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
		void DestroyGraphicsPSO(GraphicsPSO* pso) override;
		void DestroyComputePSO(ComputePSO* pso) override;

		void SetName(GPUResource* pResource, const std::string& name) override;

		void PresentBegin() override;
		void PresentEnd() override;

		void CreateCommandLists() override;
		void ExecuteCommandLists() override;
		void FinishCommandList(GRAPHICSTHREAD thread) override;

		void WaitForGPU() override;

		void SetResolution(int width, int height) override;

		Texture2D GetBackBuffer() override;

		///////////////Thread-sensitive////////////////////////

		void BindScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID) override;
		void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID) override;
		void BindRenderTargets(UINT NumViews, const Texture2D* const *ppRenderTargets, const Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		void ClearRenderTarget(const Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		void ClearDepthStencil(const Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		void BindResource(SHADERSTAGE stage, const GPUResource* resource, UINT slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		void BindResources(SHADERSTAGE stage, const GPUResource *const* resources, UINT slot, UINT count, GRAPHICSTHREAD threadID) override;
		void BindUAV(SHADERSTAGE stage, const GPUResource* resource, UINT slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		void BindUAVs(SHADERSTAGE stage, const GPUResource *const* resources, UINT slot, UINT count, GRAPHICSTHREAD threadID) override;
		void UnbindResources(UINT slot, UINT num, GRAPHICSTHREAD threadID) override;
		void UnbindUAVs(UINT slot, UINT num, GRAPHICSTHREAD threadID) override;
		void BindSampler(SHADERSTAGE stage, const Sampler* sampler, UINT slot, GRAPHICSTHREAD threadID) override;
		void BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, UINT slot, GRAPHICSTHREAD threadID) override;
		void BindVertexBuffers(const GPUBuffer *const* vertexBuffers, UINT slot, UINT count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID) override;
		void BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID) override;
		void BindStencilRef(UINT value, GRAPHICSTHREAD threadID) override;
		void BindBlendFactor(float r, float g, float b, float a, GRAPHICSTHREAD threadID) override;
		void BindGraphicsPSO(const GraphicsPSO* pso, GRAPHICSTHREAD threadID) override;
		void BindComputePSO(const ComputePSO* pso, GRAPHICSTHREAD threadID) override;
		void Draw(UINT vertexCount, UINT startVertexLocation, GRAPHICSTHREAD threadID) override;
		void DrawIndexed(UINT indexCount, UINT startIndexLocation, UINT baseVertexLocation, GRAPHICSTHREAD threadID) override;
		void DrawInstanced(UINT vertexCount, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID) override;
		void DrawIndexedInstanced(UINT indexCount, UINT instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID) override;
		void DrawInstancedIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) override;
		void DrawIndexedInstancedIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) override;
		void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID) override;
		void DispatchIndirect(const GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) override;
		void CopyTexture2D(const Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID) override;
		void CopyTexture2D_Region(const Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, const Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID) override;
		void MSAAResolve(const Texture2D* pDst, const Texture2D* pSrc, GRAPHICSTHREAD threadID) override;
		void UpdateBuffer(const GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize = -1) override;
		bool DownloadResource(const GPUResource* resourceToDownload, const GPUResource* resourceDest, void* dataDest, GRAPHICSTHREAD threadID) override;
		void QueryBegin(const GPUQuery *query, GRAPHICSTHREAD threadID) override;
		void QueryEnd(const GPUQuery *query, GRAPHICSTHREAD threadID) override;
		bool QueryRead(const GPUQuery* query, GPUQueryResult* result) override;
		void UAVBarrier(const GPUResource *const* uavs, UINT NumBarriers, GRAPHICSTHREAD threadID) override;
		void TransitionBarrier(const GPUResource *const* resources, UINT NumBarriers, RESOURCE_STATES stateBefore, RESOURCE_STATES stateAfter, GRAPHICSTHREAD threadID) override;

		GPUAllocation AllocateGPU(size_t dataSize, GRAPHICSTHREAD threadID) override;

		void EventBegin(const std::string& name, GRAPHICSTHREAD threadID) override;
		void EventEnd(GRAPHICSTHREAD threadID) override;
		void SetMarker(const std::string& name, GRAPHICSTHREAD threadID) override;

	};
}

#endif // WICKEDENGINE_BUILD_VULKAN

#endif // _GRAPHICSDEVICE_VULKAN_H_
