#pragma once
#include "CommonInclude.h"
#include "wiGraphics.h"
#include "wiPlatform.h"

#include <cassert>
#include <cstring>
#include <algorithm>

namespace wi::graphics
{
	// CommandList can be used to record graphics commands from a CPU thread
	//	Use GraphicsDevice::BeginCommandList() to start a command list
	//	Use GraphicsDevice::SubmitCommandLists() to give all started command lists to the GPU for execution
	//	CommandList recording is not thread safe
	struct CommandList
	{
		using index_type = uint8_t;
		index_type index = 0xFF;
		constexpr operator index_type() const { return index; }
	};
	static constexpr CommandList::index_type COMMANDLIST_COUNT = 32;	// If you increase command list count, more memory will be statically allocated for per-command list resources
	static constexpr CommandList INVALID_COMMANDLIST;					// CommandList is invalid if it's just declared, but not started

	// Descriptor binding counts:
	//	It's OK increase these limits if not enough
	//	But it's better to refactor shaders to use bindless descriptors if they require more resources
	static constexpr uint32_t DESCRIPTORBINDER_CBV_COUNT = 14;
	static constexpr uint32_t DESCRIPTORBINDER_SRV_COUNT = 16;
	static constexpr uint32_t DESCRIPTORBINDER_UAV_COUNT = 16;
	static constexpr uint32_t DESCRIPTORBINDER_SAMPLER_COUNT = 8;
	struct DescriptorBindingTable
	{
		GPUBuffer CBV[DESCRIPTORBINDER_CBV_COUNT];
		uint64_t CBV_offset[DESCRIPTORBINDER_CBV_COUNT] = {};
		GPUResource SRV[DESCRIPTORBINDER_SRV_COUNT];
		int SRV_index[DESCRIPTORBINDER_SRV_COUNT] = {};
		GPUResource UAV[DESCRIPTORBINDER_UAV_COUNT];
		int UAV_index[DESCRIPTORBINDER_UAV_COUNT] = {};
		Sampler SAM[DESCRIPTORBINDER_SAMPLER_COUNT];
	};

	constexpr uint32_t AlignTo(uint32_t value, uint32_t alignment)
	{
		return ((value + alignment - 1) / alignment) * alignment;
	}
	constexpr uint64_t AlignTo(uint64_t value, uint64_t alignment)
	{
		return ((value + alignment - 1) / alignment) * alignment;
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
		GraphicsDeviceCapability capabilities = GraphicsDeviceCapability::NONE;
		size_t SHADER_IDENTIFIER_SIZE = 0;
		size_t TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = 0;
		uint32_t VARIABLE_RATE_SHADING_TILE_SIZE = 0;
		uint64_t TIMESTAMP_FREQUENCY = 0;
		uint64_t ALLOCATION_MIN_ALIGNMENT = 0;

	public:
		virtual ~GraphicsDevice() = default;

		// Create a SwapChain. If the SwapChain is to be recreated, the window handle can be nullptr.
		virtual bool CreateSwapChain(const SwapChainDesc* pDesc, wi::platform::window_type window, SwapChain* swapChain) const = 0;
		virtual bool CreateBuffer(const GPUBufferDesc *pDesc, const void* pInitialData, GPUBuffer *pBuffer) const = 0;
		virtual bool CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture) const = 0;
		virtual bool CreateShader(ShaderStage stage, const void *pShaderBytecode, size_t BytecodeLength, Shader *pShader) const = 0;
		virtual bool CreateSampler(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState) const = 0;
		virtual bool CreateQueryHeap(const GPUQueryHeapDesc *pDesc, GPUQueryHeap *pQueryHeap) const = 0;
		virtual bool CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso) const = 0;
		virtual bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) const = 0;
		virtual bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh) const { return false; }
		virtual bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso) const { return false; }
		
		virtual int CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) const = 0;
		virtual int CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size = ~0) const = 0;

		virtual int GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource = -1) const = 0;
		virtual int GetDescriptorIndex(const Sampler* sampler) const = 0;

		virtual void WriteShadingRateValue(ShadingRate rate, void* dest) const {};
		virtual void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const {}
		virtual void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const {}

		// Set a debug name for the GPUResource, which will be visible in graphics debuggers
		virtual void SetName(GPUResource* pResource, const char* name) = 0;

		// Begin a new command list for GPU command recording.
		//	This will be valid until SubmitCommandLists() is called.
		virtual CommandList BeginCommandList(QUEUE_TYPE queue = QUEUE_GRAPHICS) = 0;
		// Submit all command list that were used with BeginCommandList before this call.
		//	This will make every command list to be in "available" state and restarts them
		virtual void SubmitCommandLists() = 0;

		// The CPU will wait until all submitted GPU work is finished execution
		virtual void WaitForGPU() const = 0;

		// The current PipelineState cache will be cleared. It is useful to clear this when reloading shaders, to avoid accumulating unused pipeline states
		virtual void ClearPipelineStateCache() = 0;

		// Returns the number of active pipelines. Active pipelines are the pipelines that were compiled internally for a set of render target formats
		//	One PipelineState object can be compiled internally for multiple render target or depth-stencil formats, or sample counts
		virtual size_t GetActivePipelineCount() const = 0;

		// Returns the number of elapsed frames (submits)
		//	It is incremented when calling SubmitCommandLists()
		constexpr uint64_t GetFrameCount() const { return FRAMECOUNT; }

		// Check whether the graphics device supports a feature or not
		constexpr bool CheckCapability(GraphicsDeviceCapability capability) const { return has_flag(capabilities, capability); }

		// Returns the buffer count, which is the array size of buffered resources used by both the CPU and GPU
		static constexpr uint32_t GetBufferCount() { return BUFFERCOUNT; }
		// Returns the current buffer index, which is in range [0, GetBufferCount() - 1]
		constexpr uint32_t GetBufferIndex() const { return GetFrameCount() % BUFFERCOUNT; }

		// Returns whether the graphics debug layer is enabled. It can be enabled when creating the device.
		constexpr bool IsDebugDevice() const { return DEBUGDEVICE; }

		constexpr size_t GetShaderIdentifierSize() const { return SHADER_IDENTIFIER_SIZE; }
		constexpr size_t GetTopLevelAccelerationStructureInstanceSize() const { return TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE; }
		constexpr uint32_t GetVariableRateShadingTileSize() const { return VARIABLE_RATE_SHADING_TILE_SIZE; }
		constexpr uint64_t GetTimestampFrequency() const { return TIMESTAMP_FREQUENCY; }

		// Get the shader binary format that the underlying graphics API consumes
		virtual ShaderFormat GetShaderFormat() const = 0;

		// Get a Texture resource that represents the current back buffer of the SwapChain
		virtual Texture GetBackBuffer(const SwapChain* swapchain) const = 0;
		// Returns the current color space of the swapchain output
		virtual ColorSpace GetSwapChainColorSpace(const SwapChain* swapchain) const = 0;
		// Returns true if the swapchain could support HDR output regardless of current format
		//	Returns false if the swapchain couldn't support HDR output
		virtual bool IsSwapChainSupportsHDR(const SwapChain* swapchain) const = 0;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Command List functions are below:
		//	- These are used to record rendering commands to a CommandList
		//	- To get a CommandList that can be recorded into, call BeginCommandList()
		//	- These commands are not immediately executed, but they begin executing on the GPU after calling SubmitCommandLists()
		//	- These are not thread safe, only a single thread should use a single CommandList at one time

		virtual void WaitCommandList(CommandList cmd, CommandList wait_for) = 0;
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
		virtual void BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint64_t* offsets, CommandList cmd) = 0;
		virtual void BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd) = 0;
		virtual void BindStencilRef(uint32_t value, CommandList cmd) = 0;
		virtual void BindBlendFactor(float r, float g, float b, float a, CommandList cmd) = 0;
		virtual void BindShadingRate(ShadingRate rate, CommandList cmd) {}
		virtual void BindPipelineState(const PipelineState* pso, CommandList cmd) = 0;
		virtual void BindComputeShader(const Shader* cs, CommandList cmd) = 0;
		virtual void BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd) = 0;
		virtual void Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) = 0;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd) = 0;
		virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) = 0;
		virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) = 0;
		virtual void DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) = 0;
		virtual void DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) = 0;
		virtual void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) = 0;
		virtual void DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) = 0;
		virtual void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) {}
		virtual void DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) {}
		virtual void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) = 0;
		virtual void CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd) = 0;
		virtual void QueryBegin(const GPUQueryHeap *heap, uint32_t index, CommandList cmd) = 0;
		virtual void QueryEnd(const GPUQueryHeap *heap, uint32_t index, CommandList cmd) = 0;
		virtual void QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd) = 0;
		virtual void QueryReset(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd) {}
		virtual void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) = 0;
		virtual void BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src = nullptr) {}
		virtual void BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd) {}
		virtual void DispatchRays(const DispatchRaysDesc* desc, CommandList cmd) {}
		virtual void PushConstants(const void* data, uint32_t size, CommandList cmd, uint32_t offset = 0) = 0;
		virtual void PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd) {}
		virtual void PredicationEnd(CommandList cmd) {}

		virtual void EventBegin(const char* name, CommandList cmd) = 0;
		virtual void EventEnd(CommandList cmd) = 0;
		virtual void SetMarker(const char* name, CommandList cmd) = 0;

		virtual const RenderPass* GetCurrentRenderPass(CommandList cmd) const = 0;


		// Some useful helpers:

		struct GPULinearAllocator
		{
			GPUBuffer buffer;
			uint64_t offset = 0;
			uint64_t frame_index = 0;
		} frame_allocators[BUFFERCOUNT][COMMANDLIST_COUNT];

		struct GPUAllocation
		{
			void* data = nullptr;	// application can write to this. Reads might be not supported or slow. The offset is already applied
			GPUBuffer buffer;		// application can bind it to the GPU
			uint64_t offset = 0;	// allocation's offset from the GPUbuffer's beginning

			// Returns true if the allocation was successful
			inline bool IsValid() const { return data != nullptr && buffer.IsValid(); }
		};

		// Allocates temporary memory that the CPU can write and GPU can read. 
		//	It is only alive for one frame and automatically invalidated after that.
		GPUAllocation AllocateGPU(uint64_t dataSize, CommandList cmd)
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

			const uint64_t free_space = allocator.buffer.desc.size - allocator.offset;
			if (dataSize > free_space)
			{
				GPUBufferDesc desc;
				desc.usage = Usage::UPLOAD;
				desc.size = AlignTo((allocator.buffer.desc.size + dataSize) * 2, ALLOCATION_MIN_ALIGNMENT);
				desc.bind_flags = BindFlag::CONSTANT_BUFFER | BindFlag::VERTEX_BUFFER | BindFlag::INDEX_BUFFER | BindFlag::SHADER_RESOURCE;
				desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				CreateBuffer(&desc, nullptr, &allocator.buffer);
				SetName(&allocator.buffer, "frame_allocator");
				allocator.offset = 0;
			}

			allocation.buffer = allocator.buffer;
			allocation.offset = allocator.offset;
			allocation.data = (void*)((size_t)allocator.buffer.mapped_data + allocator.offset);

			allocator.offset += AlignTo(dataSize, ALLOCATION_MIN_ALIGNMENT);

			assert(allocation.IsValid());
			return allocation;
		}

		// Updates a Usage::DEFAULT buffer data
		//	Since it uses a GPU Copy operation, appropriate synchronization is expected
		//	And it cannot be used inside a RenderPass
		void UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, uint64_t size = ~0, uint64_t offset = 0)
		{
			if (buffer == nullptr || data == nullptr)
				return;
			size = std::min(buffer->desc.size, size);
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
		void BindDynamicConstantBuffer(const T& data, uint32_t slot, CommandList cmd)
		{
			GPUAllocation allocation = AllocateGPU(sizeof(T), cmd);
			std::memcpy(allocation.data, &data, sizeof(T));
			BindConstantBuffer(&allocation.buffer, slot, cmd, allocation.offset);
		}
	};


	// This is a helper to get access to a global device instance
	//	- The engine uses this, but it is not necessary to use a single global device object
	//	- This is not a lifetime managing object, just a way to globally expose a reference to an object by pointer
	inline GraphicsDevice*& GetDevice()
	{
		static GraphicsDevice* device = nullptr;
		return device;
	}

}
