#ifndef _GRAPHICSDEVICE_VULKAN_H_
#define _GRAPHICSDEVICE_VULKAN_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiWindowRegistration.h"


#ifdef WICKEDENGINE_BUILD_VULKAN
#include "Include_Vulkan.h"

namespace wiGraphicsTypes
{

	class GraphicsDevice_Vulkan : public GraphicsDevice
	{
	private:
		VkSurfaceKHR surface;
		bool prepared;
		bool use_staging_buffer;
		bool separate_present_queue;

		bool VK_KHR_incremental_present_enabled;

		bool VK_GOOGLE_display_timing_enabled;
		bool syncd_with_actual_presents;
		uint64_t refresh_duration;
		uint64_t refresh_duration_multiplier;
		uint64_t target_IPD;  // image present duration (inverse of frame rate)
		uint64_t prev_desired_present_time;
		uint32_t next_present_id;
		uint32_t last_early_id;  // 0 if no early images
		uint32_t last_late_id;   // 0 if no late images

		VkInstance inst;
		VkPhysicalDevice gpu;
		VkDevice device;
		VkQueue graphics_queue;
		VkQueue present_queue;
		uint32_t graphics_queue_family_index;
		uint32_t present_queue_family_index;
		VkSemaphore image_acquired_semaphores[BACKBUFFER_COUNT];
		VkSemaphore draw_complete_semaphores[BACKBUFFER_COUNT];
		VkSemaphore image_ownership_semaphores[BACKBUFFER_COUNT];
		VkPhysicalDeviceProperties gpu_props;
		VkQueueFamilyProperties *queue_props;
		VkPhysicalDeviceMemoryProperties memory_properties;

		uint32_t enabled_extension_count;
		uint32_t enabled_layer_count;
		char *extension_names[64];
		char *enabled_layers[64];

		int width, height;
		VkFormat format;
		VkColorSpaceKHR color_space;

		typedef struct {
			VkImage image;
			VkCommandBuffer cmd;
			VkCommandBuffer graphics_to_present_cmd;
			VkImageView view;
			VkBuffer uniform_buffer;
			VkDeviceMemory uniform_memory;
			VkFramebuffer framebuffer;
			VkDescriptorSet descriptor_set;
		} SwapchainImageResources;

		PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
		PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
		PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
		PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
		PFN_vkQueuePresentKHR fpQueuePresentKHR;
		PFN_vkGetRefreshCycleDurationGOOGLE fpGetRefreshCycleDurationGOOGLE;
		PFN_vkGetPastPresentationTimingGOOGLE fpGetPastPresentationTimingGOOGLE;
		uint32_t swapchainImageCount;
		VkSwapchainKHR swapchain;
		SwapchainImageResources *swapchain_image_resources;
		VkPresentModeKHR presentMode;
		VkFence fences[BACKBUFFER_COUNT];
		int frame_index;

	public:
		GraphicsDevice_Vulkan(wiWindowRegistration::window_type window, bool fullscreen = false);

		~GraphicsDevice_Vulkan();

		virtual HRESULT CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *ppBuffer) override;
		virtual HRESULT CreateTexture1D(const Texture1DDesc* pDesc, const SubresourceData *pInitialData, Texture1D **ppTexture1D) override;
		virtual HRESULT CreateTexture2D(const Texture2DDesc* pDesc, const SubresourceData *pInitialData, Texture2D **ppTexture2D) override;
		virtual HRESULT CreateTexture3D(const Texture3DDesc* pDesc, const SubresourceData *pInitialData, Texture3D **ppTexture3D) override;
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
		virtual void BindRenderTargetsUAVs(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GPUResource* const *ppUAVs, int slotUAV, int countUAV,
			GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
		virtual void BindRenderTargets(UINT NumViews, Texture* const *ppRenderTargets, Texture2D* depthStencilTexture, GRAPHICSTHREAD threadID, int arrayIndex = -1) override;
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
		virtual void BindPrimitiveTopology(PRIMITIVETOPOLOGY type, GRAPHICSTHREAD threadID) override;
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
