#pragma once
#include "CommonInclude.h"
#include "wiGraphics.h"
#include "wiPlatform.h"

namespace wiGraphics
{
	typedef uint8_t CommandList;
	static const CommandList COMMANDLIST_COUNT = 32;
	static const CommandList INVALID_COMMANDLIST = COMMANDLIST_COUNT;

	enum QUEUE_TYPE
	{
		QUEUE_GRAPHICS,
		QUEUE_COMPUTE,

		QUEUE_COUNT,
	};

	class GraphicsDevice
	{
	protected:
		static const uint32_t BUFFERCOUNT = 2;
		uint64_t FRAMECOUNT = 0;
		bool DEBUGDEVICE = false;
		uint32_t capabilities = 0;
		size_t SHADER_IDENTIFIER_SIZE = 0;
		size_t TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = 0;
		uint32_t VARIABLE_RATE_SHADING_TILE_SIZE = 0;
		uint64_t TIMESTAMP_FREQUENCY = 0;

	public:
		virtual ~GraphicsDevice() = default;

		virtual bool CreateSwapChain(const SwapChainDesc* pDesc, wiPlatform::window_type window, SwapChain* swapChain) const = 0;
		virtual bool CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer) const = 0;
		virtual bool CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture) const = 0;
		virtual bool CreateShader(SHADERSTAGE stage, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader) const = 0;
		virtual bool CreateSampler(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) const = 0;
		virtual bool CreateQueryHeap(const GPUQueryHeapDesc *pDesc, GPUQueryHeap *pQueryHeap) const = 0;
		virtual bool CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso) const = 0;
		virtual bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) const = 0;
		virtual bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh) const { return false; }
		virtual bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso) const { return false; }
		
		virtual int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) const = 0;
		virtual int CreateSubresource(GPUBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size = ~0) const = 0;

		virtual int GetDescriptorIndex(const GPUResource* resource, SUBRESOURCE_TYPE type, int subresource = -1) const { return -1; };
		virtual int GetDescriptorIndex(const Sampler* sampler) const { return -1; };

		virtual void WriteShadingRateValue(SHADING_RATE rate, void* dest) const {};
		virtual void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const {}
		virtual void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const {}
		
		virtual void Map(const GPUResource* resource, Mapping* mapping) const = 0;
		virtual void Unmap(const GPUResource* resource) const = 0;
		virtual void QueryRead(const GPUQueryHeap* heap, uint32_t index, uint32_t count, uint64_t* results) const = 0;

		virtual void SetCommonSampler(const StaticSampler* sam) = 0;

		virtual void SetName(GPUResource* pResource, const char* name) = 0;

		// Begin a new command list for GPU command recording.
		//	This will be valid until SubmitCommandLists() is called.
		virtual CommandList BeginCommandList(QUEUE_TYPE queue = QUEUE_GRAPHICS) = 0;
		// Submit all command list that were used with BeginCommandList before this call.
		//	This will make every command list to be in "available" state and restarts them
		virtual void SubmitCommandLists() = 0;

		virtual void WaitForGPU() const = 0;
		virtual void ClearPipelineStateCache() {};

		constexpr uint64_t GetFrameCount() const { return FRAMECOUNT; }

		inline bool CheckCapability(GRAPHICSDEVICE_CAPABILITY capability) const { return capabilities & capability; }

		uint32_t GetFormatStride(FORMAT value) const;
		bool IsFormatUnorm(FORMAT value) const;
		bool IsFormatBlockCompressed(FORMAT value) const;
		bool IsFormatStencilSupport(FORMAT value) const;

		static constexpr uint32_t GetBufferCount() { return BUFFERCOUNT; }

		constexpr bool IsDebugDevice() const { return DEBUGDEVICE; }

		constexpr size_t GetShaderIdentifierSize() const { return SHADER_IDENTIFIER_SIZE; }
		constexpr size_t GetTopLevelAccelerationStructureInstanceSize() const { return TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE; }
		constexpr uint32_t GetVariableRateShadingTileSize() const { return VARIABLE_RATE_SHADING_TILE_SIZE; }
		constexpr uint64_t GetTimestampFrequency() const { return TIMESTAMP_FREQUENCY; }

		virtual SHADERFORMAT GetShaderFormat() const { return SHADERFORMAT_NONE; }

		virtual Texture GetBackBuffer(const SwapChain* swapchain) const = 0;

		///////////////Thread-sensitive////////////////////////

		virtual void WaitCommandList(CommandList cmd, CommandList wait_for) {}
		virtual void RenderPassBegin(const SwapChain* swapchain, CommandList cmd) = 0;
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
		virtual void BindShadingRate(SHADING_RATE rate, CommandList cmd) {}
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
		virtual void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) {}
		virtual void DispatchMeshIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd) {}
		virtual void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) = 0;
		virtual void UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize = -1) = 0;
		virtual void QueryBegin(const GPUQueryHeap *heap, uint32_t index, CommandList cmd) = 0;
		virtual void QueryEnd(const GPUQueryHeap *heap, uint32_t index, CommandList cmd) = 0;
		virtual void QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd) {}
		virtual void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) = 0;
		virtual void BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src = nullptr) {}
		virtual void BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd) {}
		virtual void DispatchRays(const DispatchRaysDesc* desc, CommandList cmd) {}
		virtual void PushConstants(const void* data, uint32_t size, CommandList cmd) {}

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
