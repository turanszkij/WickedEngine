#ifndef _GRAPHICSDEVICE_VULKAN_H_
#define _GRAPHICSDEVICE_VULKAN_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiWindowRegistration.h"

#ifdef WICKEDENGINE_BUILD_VULKAN
#include "Include_Vulkan.h"
#include "wiGraphicsDevice_SharedInternals.h"

#include <vector>
#include <unordered_map>

namespace wiGraphicsTypes
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
			bool active = false;
			bool dirty = true;

			VkImageView attachments[9] = {};
			uint32_t attachmentCount = 0;
			VkExtent2D attachmentsExtents = {};
			uint32_t attachmentLayers = 0;
			VkClearValue clearColor[9] = {};

			VkPipeline pso = VK_NULL_HANDLE;
			VkRenderPass renderPass = VK_NULL_HANDLE;
			VkFramebuffer fbo = VK_NULL_HANDLE;

			std::unordered_map<VkPipeline, VkFramebuffer> renderPassFrameBuffers;

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
				std::vector<wiHandle> boundDescriptors[SHADERSTAGE_COUNT];

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
				VkBuffer				resource;
				VkDeviceMemory			resourceMemory;
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


		struct UploadBuffer : wiThreadSafeManager
		{
			VkDevice				device;
			VkBuffer				resource;
			VkDeviceMemory			resourceMemory;
			uint8_t*				dataBegin;
			uint8_t*				dataCur;
			uint8_t*				dataEnd;

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

		~GraphicsDevice_Vulkan();

		virtual HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer) override;
		virtual HRESULT CreateTexture1D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture1D **ppTexture1D) override;
		virtual HRESULT CreateTexture2D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D) override;
		virtual HRESULT CreateTexture3D(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture3D **ppTexture3D) override;
		virtual HRESULT CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, UINT NumElements,
			const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, VertexLayout *pInputLayout) override;
		virtual HRESULT CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, VertexShader *pVertexShader) override;
		virtual HRESULT CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, PixelShader *pPixelShader) override;
		virtual HRESULT CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, GeometryShader *pGeometryShader) override;
		virtual HRESULT CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, HullShader *pHullShader) override;
		virtual HRESULT CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, DomainShader *pDomainShader) override;
		virtual HRESULT CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ComputeShader *pComputeShader) override;
		virtual HRESULT CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState) override;
		virtual HRESULT CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState) override;
		virtual HRESULT CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState) override;
		virtual HRESULT CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) override;
		virtual HRESULT CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery) override;
		virtual HRESULT CreateGraphicsPSO(const GraphicsPSODesc* pDesc, GraphicsPSO* pso) override;
		virtual HRESULT CreateComputePSO(const ComputePSODesc* pDesc, ComputePSO* pso) override;

		virtual void PresentBegin() override;
		virtual void PresentEnd() override;

		virtual void ExecuteDeferredContexts() override;
		virtual void FinishCommandList(GRAPHICSTHREAD thread) override;

		virtual void SetResolution(int width, int height) override;

		virtual Texture2D GetBackBuffer() override;

		///////////////Thread-sensitive////////////////////////

		virtual void BindViewports(UINT NumViewports, const ViewPort *pViewports, GRAPHICSTHREAD threadID) override;
		virtual void BindRenderTargetsUAVs(UINT NumViews, Texture2D* const *ppRenderTargets, Texture2D* depthStencilTexture, GPUResource* const *ppUAVs, int slotUAV, int countUAV,
			GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindRenderTargets(UINT NumViews, Texture2D* const *ppRenderTargets, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void ClearRenderTarget(Texture* pTexture, const FLOAT ColorRGBA[4], GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void ClearDepthStencil(Texture2D* pTexture, UINT ClearFlags, FLOAT Depth, UINT8 Stencil, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResource(SHADERSTAGE stage, GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindResources(SHADERSTAGE stage, GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		void BindUnorderedAccessResource(SHADERSTAGE stage, GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1);
		void BindUnorderedAccessResources(SHADERSTAGE stage, GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID);
		virtual void BindUnorderedAccessResourceCS(GPUResource* resource, int slot, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindUnorderedAccessResourcesCS(GPUResource *const* resources, int slot, int count, GRAPHICSTHREAD threadID) override;
		virtual void UnBindResources(int slot, int num, GRAPHICSTHREAD threadID) override;
		virtual void UnBindUnorderedAccessResources(int slot, int num, GRAPHICSTHREAD threadID) override;
		virtual void BindSampler(SHADERSTAGE stage, Sampler* sampler, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindConstantBuffer(SHADERSTAGE stage, GPUBuffer* buffer, int slot, GRAPHICSTHREAD threadID) override;
		virtual void BindVertexBuffers(GPUBuffer* const *vertexBuffers, int slot, int count, const UINT* strides, const UINT* offsets, GRAPHICSTHREAD threadID) override;
		virtual void BindIndexBuffer(GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, UINT offset, GRAPHICSTHREAD threadID) override;
		virtual void BindStencilRef(UINT value, GRAPHICSTHREAD threadID) override;
		virtual void BindBlendFactor(XMFLOAT4 value, GRAPHICSTHREAD threadID) override;
		virtual void BindGraphicsPSO(GraphicsPSO* pso, GRAPHICSTHREAD threadID) override;
		virtual void BindComputePSO(ComputePSO* pso, GRAPHICSTHREAD threadID) override;
		virtual void Draw(int vertexCount, UINT startVertexLocation, GRAPHICSTHREAD threadID) override;
		virtual void DrawIndexed(int indexCount, UINT startIndexLocation, UINT baseVertexLocation, GRAPHICSTHREAD threadID) override;
		virtual void DrawInstanced(int vertexCount, int instanceCount, UINT startVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID) override;
		virtual void DrawIndexedInstanced(int indexCount, int instanceCount, UINT startIndexLocation, UINT baseVertexLocation, UINT startInstanceLocation, GRAPHICSTHREAD threadID) override;
		virtual void DrawInstancedIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) override;
		virtual void DrawIndexedInstancedIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) override;
		virtual void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ, GRAPHICSTHREAD threadID) override;
		virtual void DispatchIndirect(GPUBuffer* args, UINT args_offset, GRAPHICSTHREAD threadID) override;
		virtual void GenerateMips(Texture* texture, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void CopyTexture2D(Texture2D* pDst, Texture2D* pSrc, GRAPHICSTHREAD threadID) override;
		virtual void CopyTexture2D_Region(Texture2D* pDst, UINT dstMip, UINT dstX, UINT dstY, Texture2D* pSrc, UINT srcMip, GRAPHICSTHREAD threadID) override;
		virtual void MSAAResolve(Texture2D* pDst, Texture2D* pSrc, GRAPHICSTHREAD threadID) override;
		virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, GRAPHICSTHREAD threadID, int dataSize = -1) override;
		virtual void* AllocateFromRingBuffer(GPURingBuffer* buffer, size_t dataSize, UINT& offsetIntoBuffer, GRAPHICSTHREAD threadID) override;
		virtual void InvalidateBufferAccess(GPUBuffer* buffer, GRAPHICSTHREAD threadID) override;
		virtual bool DownloadBuffer(GPUBuffer* bufferToDownload, GPUBuffer* bufferDest, void* dataDest, GRAPHICSTHREAD threadID) override;
		virtual void SetScissorRects(UINT numRects, const Rect* rects, GRAPHICSTHREAD threadID) override;
		virtual void QueryBegin(GPUQuery *query, GRAPHICSTHREAD threadID) override;
		virtual void QueryEnd(GPUQuery *query, GRAPHICSTHREAD threadID) override;
		virtual bool QueryRead(GPUQuery *query, GRAPHICSTHREAD threadID) override;
		virtual void UAVBarrier(GPUResource *const* uavs, UINT NumBarriers, GRAPHICSTHREAD threadID) override;
		virtual void TransitionBarrier(GPUResource *const* resources, UINT NumBarriers, RESOURCE_STATES stateBefore, RESOURCE_STATES stateAfter, GRAPHICSTHREAD threadID) override;

		virtual void WaitForGPU();

		virtual HRESULT CreateTextureFromFile(const std::string& fileName, Texture2D **ppTexture, bool mipMaps, GRAPHICSTHREAD threadID) override;
		virtual HRESULT SaveTexturePNG(const std::string& fileName, Texture2D *pTexture, GRAPHICSTHREAD threadID) override;
		virtual HRESULT SaveTextureDDS(const std::string& fileName, Texture *pTexture, GRAPHICSTHREAD threadID) override;

		virtual void EventBegin(const std::string& name, GRAPHICSTHREAD threadID) override;
		virtual void EventEnd(GRAPHICSTHREAD threadID) override;
		virtual void SetMarker(const std::string& name, GRAPHICSTHREAD threadID) override;

	};
}

#endif // WICKEDENGINE_BUILD_VULKAN

#endif // _GRAPHICSDEVICE_VULKAN_H_
