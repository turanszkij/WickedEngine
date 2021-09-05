#pragma once
#include "CommonInclude.h"
#include "wiGraphics.h"
#include "wiPlatform.h"

#include <cstring>
#include <algorithm>

namespace wiGraphics
{
	typedef uint8_t CommandList;
	static const CommandList COMMANDLIST_COUNT = 32;
	static const CommandList INVALID_COMMANDLIST = COMMANDLIST_COUNT;

	// Descriptor binding counts:
	//	It's OK increase these limits if not enough
	static const uint32_t DESCRIPTORBINDER_CBV_COUNT = 15;
	static const uint32_t DESCRIPTORBINDER_SRV_COUNT = 64;
	static const uint32_t DESCRIPTORBINDER_UAV_COUNT = 16;
	static const uint32_t DESCRIPTORBINDER_SAMPLER_COUNT = 16;

	inline size_t Align(size_t uLocation, size_t uAlign)
	{
		if ((0 == uAlign) || (uAlign & (uAlign - 1)))
		{
			assert(0);
		}
		return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
	}

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
		size_t ALLOCATION_MIN_ALIGNMENT = 0;

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
		constexpr uint32_t GetBufferIndex() const { return GetFrameCount() % BUFFERCOUNT; }

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
		virtual void BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) = 0;
		virtual void BindResources(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd) = 0;
		virtual void BindUAV(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) = 0;
		virtual void BindUAVs(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd) = 0;
		virtual void BindSampler(const Sampler* sampler, uint32_t slot, CommandList cmd) = 0;
		virtual void BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset = 0ull) = 0;
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
		virtual void CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd) = 0;
		virtual void QueryBegin(const GPUQueryHeap *heap, uint32_t index, CommandList cmd) = 0;
		virtual void QueryEnd(const GPUQueryHeap *heap, uint32_t index, CommandList cmd) = 0;
		virtual void QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd) {}
		virtual void QueryReset(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd) {}
		virtual void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) = 0;
		virtual void BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src = nullptr) {}
		virtual void BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd) {}
		virtual void DispatchRays(const DispatchRaysDesc* desc, CommandList cmd) {}
		virtual void PushConstants(const void* data, uint32_t size, CommandList cmd) {}
		
		virtual void EventBegin(const char* name, CommandList cmd) = 0;
		virtual void EventEnd(CommandList cmd) = 0;
		virtual void SetMarker(const char* name, CommandList cmd) = 0;




		// Some useful helpers:

		struct GPULinearAllocator
		{
			GPUBuffer buffer;
			size_t offset = 0;
			uint64_t frame_index = 0;
		} frame_allocators[BUFFERCOUNT][COMMANDLIST_COUNT];

		struct GPUAllocation
		{
			void* data = nullptr;				// application can write to this. Reads might be not supported or slow. The offset is already applied
			GPUBuffer buffer;					// application can bind it to the GPU
			uint32_t offset = 0;				// allocation's offset from the GPUbuffer's beginning

			// Returns true if the allocation was successful
			inline bool IsValid() const { return data != nullptr && buffer.IsValid(); }
		};
		// Allocates temporary memory that the CPU can write and GPU can read. 
		//	It is only alive for one frame and automatically invalidated after that.
		GPUAllocation AllocateGPU(size_t dataSize, CommandList cmd)
		{
			GPUAllocation allocation;
			if (dataSize == 0)
				return allocation;

			GPULinearAllocator& allocator = frame_allocators[GetBufferIndex()][cmd];
			if (FRAMECOUNT != allocator.frame_index)
			{
				allocator.frame_index = FRAMECOUNT;
				allocator.offset = 0;
			}

			size_t free_space = (size_t)allocator.buffer.desc.ByteWidth - allocator.offset;
			if (dataSize > free_space)
			{
				GPUBufferDesc desc;
				desc.Usage = USAGE_UPLOAD;
				desc.ByteWidth = (uint32_t)Align((allocator.buffer.desc.ByteWidth + dataSize) * 2, ALLOCATION_MIN_ALIGNMENT);
				desc.BindFlags = BIND_CONSTANT_BUFFER | BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
				desc.MiscFlags = RESOURCE_MISC_BUFFER_RAW;
				CreateBuffer(&desc, nullptr, &allocator.buffer);
				SetName(&allocator.buffer, "frame_allocator");
				allocator.offset = 0;
			}

			allocation.buffer = allocator.buffer;
			allocation.offset = (uint32_t)allocator.offset;
			allocation.data = (void*)((size_t)allocator.buffer.mapped_data + allocator.offset);

			allocator.offset += Align(dataSize, ALLOCATION_MIN_ALIGNMENT);

			assert(allocation.IsValid());
			return allocation;
		}

		// Updates a USAGE_DEFAULT buffer data
		//	Since it uses a GPU Copy operation, appropriate synchronization is expected
		//	And it cannot be used inside a RenderPass
		void UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, size_t size = ~0, size_t offset = 0)
		{
			if (buffer == nullptr || data == nullptr)
				return;
			size = std::min((size_t)buffer->desc.ByteWidth, size);
			if (size == 0)
				return;
			GPUAllocation allocation = AllocateGPU(size, cmd);
			std::memcpy(allocation.data, data, size);
			CopyBuffer(buffer, offset, &allocation.buffer, allocation.offset, size, cmd);
		}

		// Helper util to bind a constant buffer with data for a specific command list:
		//	This will be done on the CPU to an UPLOAD buffer, so this can be used inside a RenderPass
		//	But this will be only visible on the command list it was bound to
		template<typename T>
		void BindDynamicConstantBuffer(const T& data, uint32_t slot, wiGraphics::CommandList cmd)
		{
			GPUAllocation allocation = AllocateGPU(sizeof(T), cmd);
			std::memcpy(allocation.data, &data, sizeof(T));
			BindConstantBuffer(&allocation.buffer, slot, cmd, allocation.offset);
		}
	};

}
