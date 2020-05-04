#pragma once
#include "CommonInclude.h"
#include "wiGraphics.h"

#include <memory>

namespace wiGraphics
{
	typedef uint8_t CommandList;
	static const CommandList COMMANDLIST_COUNT = 16;

	class GraphicsDevice
	{
	protected:
		uint64_t FRAMECOUNT = 0;
		bool VSYNC = true;
		int RESOLUTIONWIDTH = 0;
		int RESOLUTIONHEIGHT = 0;
		bool DEBUGDEVICE = false;
		bool FULLSCREEN = false;
		bool RESOLUTIONCHANGED = false;
		FORMAT BACKBUFFER_FORMAT = FORMAT_R10G10B10A2_UNORM;
		static const uint32_t BACKBUFFER_COUNT = 2;
		bool TESSELLATION = false;
		bool CONSERVATIVE_RASTERIZATION = false;
		bool RASTERIZER_ORDERED_VIEWS = false;
		bool UAV_LOAD_FORMAT_COMMON = false;
		bool UAV_LOAD_FORMAT_R11G11B10_FLOAT = false;
		bool RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS = false;

	public:
		virtual bool CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer) = 0;
		virtual bool CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture) = 0;
		virtual bool CreateInputLayout(const InputLayoutDesc *pInputElementDescs, uint32_t NumElements, const Shader* shader, InputLayout *pInputLayout) = 0;
		virtual bool CreateShader(SHADERSTAGE stage, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader) = 0;
		virtual bool CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState) = 0;
		virtual bool CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState) = 0;
		virtual bool CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState) = 0;
		virtual bool CreateSampler(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) = 0;
		virtual bool CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery) = 0;
		virtual bool CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso) = 0;
		virtual bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) = 0;

		virtual int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) = 0;

		virtual bool DownloadResource(const GPUResource* resourceToDownload, const GPUResource* resourceDest, void* dataDest) = 0;

		virtual void SetName(GPUResource* pResource, const char* name) = 0;

		virtual void PresentBegin(CommandList cmd) = 0;
		virtual void PresentEnd(CommandList cmd) = 0;

		virtual CommandList BeginCommandList() = 0;

		virtual void WaitForGPU() = 0;
		virtual void ClearPipelineStateCache() {};

		inline bool GetVSyncEnabled() const { return VSYNC; }
		inline void SetVSyncEnabled(bool value) { VSYNC = value; }
		inline uint64_t GetFrameCount() const { return FRAMECOUNT; }

		// Returns native resolution width of back buffer in pixels:
		inline int GetResolutionWidth() const { return RESOLUTIONWIDTH; }
		// Returns native resolution height of back buffer in pixels:
		inline int GetResolutionHeight() const { return RESOLUTIONHEIGHT; }
		// Has the resolution changed since the last frame?
		inline bool ResolutionChanged() const { return RESOLUTIONCHANGED; }

		// Returns the width of the screen with DPI scaling applied (subpixel size):
		float GetScreenWidth() const;
		// Returns the height of the screen with DPI scaling applied (subpixel size):
		float GetScreenHeight() const;


		virtual void SetResolution(int width, int height) = 0;

		virtual Texture GetBackBuffer() = 0;

		enum GRAPHICSDEVICE_CAPABILITY
		{
			GRAPHICSDEVICE_CAPABILITY_TESSELLATION,
			GRAPHICSDEVICE_CAPABILITY_CONSERVATIVE_RASTERIZATION,
			GRAPHICSDEVICE_CAPABILITY_RASTERIZER_ORDERED_VIEWS,
			GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_COMMON, // eg: R16G16B16A16_FLOAT, R8G8B8A8_UNORM and more common ones
			GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT,
			GRAPHICSDEVICE_CAPABILITY_RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS,
			GRAPHICSDEVICE_CAPABILITY_COUNT,
		};
		bool CheckCapability(GRAPHICSDEVICE_CAPABILITY capability) const;

		uint32_t GetFormatStride(FORMAT value) const;
		bool IsFormatUnorm(FORMAT value) const;
		bool IsFormatBlockCompressed(FORMAT value) const;
		bool IsFormatStencilSupport(FORMAT value) const;

		inline XMMATRIX GetScreenProjection() const
		{
			return XMMatrixOrthographicOffCenterLH(0, (float)GetScreenWidth(), (float)GetScreenHeight(), 0, -1, 1);
		}
		inline FORMAT GetBackBufferFormat() const { return BACKBUFFER_FORMAT; }
		inline static uint32_t GetBackBufferCount() { return BACKBUFFER_COUNT; }

		inline bool IsDebugDevice() const { return DEBUGDEVICE; }


		///////////////Thread-sensitive////////////////////////

		virtual void RenderPassBegin(const RenderPass* renderpass, CommandList cmd) = 0;
		virtual void RenderPassEnd(CommandList cmd) = 0;
		virtual void BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) = 0;
		virtual void BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd) = 0;
		virtual void BindResource(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) = 0;
		virtual void BindResources(SHADERSTAGE stage, const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd) = 0;
		virtual void BindUAV(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) = 0;
		virtual void BindUAVs(SHADERSTAGE stage, const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd) = 0;
		virtual void UnbindResources(uint32_t slot, uint32_t num, CommandList cmd) = 0;
		virtual void UnbindUAVs(uint32_t slot, uint32_t num, CommandList cmd) = 0;
		virtual void BindSampler(SHADERSTAGE stage, const Sampler* sampler, uint32_t slot, CommandList cmd) = 0;
		virtual void BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, uint32_t slot, CommandList cmd) = 0;
		virtual void BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets, CommandList cmd) = 0;
		virtual void BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, uint32_t offset, CommandList cmd) = 0;
		virtual void BindStencilRef(uint32_t value, CommandList cmd) = 0;
		virtual void BindBlendFactor(float r, float g, float b, float a, CommandList cmd) = 0;
		virtual void BindPipelineState(const PipelineState* pso, CommandList cmd) = 0;
		virtual void BindComputeShader(const Shader* cs, CommandList cmd) = 0;
		virtual void Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) = 0;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, CommandList cmd) = 0;
		virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) = 0;
		virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) = 0;
		virtual void DrawInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd) = 0;
		virtual void DrawIndexedInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd) = 0;
		virtual void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) = 0;
		virtual void DispatchIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd) = 0;
		virtual void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) = 0;
		virtual void CopyTexture2D_Region(const Texture* pDst, uint32_t dstMip, uint32_t dstX, uint32_t dstY, const Texture* pSrc, uint32_t srcMip, CommandList cmd) = 0;
		virtual void MSAAResolve(const Texture* pDst, const Texture* pSrc, CommandList cmd) = 0;
		virtual void UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize = -1) = 0;
		virtual void QueryBegin(const GPUQuery *query, CommandList cmd) = 0;
		virtual void QueryEnd(const GPUQuery *query, CommandList cmd) = 0;
		virtual bool QueryRead(const GPUQuery *query, GPUQueryResult* result) = 0;
		virtual void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) = 0;

		struct GPUAllocation
		{
			void* data = nullptr;				// application can write to this. Reads might be not supported or slow. The offset is already applied
			const GPUBuffer* buffer = nullptr;	// application can bind it to the GPU
			uint32_t offset = 0;					// allocation's offset from the GPUbuffer's beginning

			// Returns true if the allocation was successful
			inline bool IsValid() const { return data != nullptr && buffer != nullptr; }
		};
		// Allocates temporary memory that the CPU can write and GPU can read. 
		//	It is only alive for one frame and automatically invalidated after that.
		//	The CPU pointer gets invalidated as soon as there is a Draw() or Dispatch() event on the same thread
		//	This allocation can be used to provide temporary vertex buffer, index buffer or raw buffer data to shaders
		virtual GPUAllocation AllocateGPU(size_t dataSize, CommandList cmd) = 0;
		
		virtual void EventBegin(const char* name, CommandList cmd) = 0;
		virtual void EventEnd(CommandList cmd) = 0;
		virtual void SetMarker(const char* name, CommandList cmd) = 0;
	};

}
