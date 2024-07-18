#pragma once
#include "CommonInclude.h"
#include "wiGraphics.h"
#include "wiPlatform.h"

#include <cassert>
#include <cstring>
#include <algorithm>
#include <functional>

namespace wi::graphics
{
	// CommandList can be used to record graphics commands from a CPU thread
	//	Use GraphicsDevice::BeginCommandList() to start a command list
	//	Use GraphicsDevice::SubmitCommandLists() to give all started command lists to the GPU for execution
	//	CommandList recording is not thread safe
	struct CommandList
	{
		void* internal_state = nullptr;
		constexpr bool IsValid() const { return internal_state != nullptr; }
	};

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

	enum QUEUE_TYPE
	{
		QUEUE_GRAPHICS,
		QUEUE_COMPUTE,
		QUEUE_COPY,
		QUEUE_VIDEO_DECODE,

		QUEUE_COUNT,
	};

	class GraphicsDevice
	{
	protected:
		static constexpr uint32_t BUFFERCOUNT = 2;
		uint64_t FRAMECOUNT = 0;
		ValidationMode validationMode = ValidationMode::Disabled;
		GraphicsDeviceCapability capabilities = GraphicsDeviceCapability::NONE;
		size_t SHADER_IDENTIFIER_SIZE = 0;
		size_t TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = 0;
		uint32_t VARIABLE_RATE_SHADING_TILE_SIZE = 0;
		uint64_t TIMESTAMP_FREQUENCY = 0;
		uint64_t VIDEO_DECODE_BITSTREAM_ALIGNMENT = 1u;
		uint32_t vendorId = 0;
		uint32_t deviceId = 0;
		std::string adapterName;
		std::string driverDescription;
		AdapterType adapterType = AdapterType::Other;

	public:
		virtual ~GraphicsDevice() = default;

		// Create a SwapChain. If the SwapChain is to be recreated, the window handle can be nullptr.
		virtual bool CreateSwapChain(const SwapChainDesc* desc, wi::platform::window_type window, SwapChain* swapchain) const = 0;
		// Create a buffer with a callback to initialize its data. Note: don't read from callback's dest pointer, reads will be very slow! Use memcpy to write to it to make sure only writes happen!
		virtual bool CreateBuffer2(const GPUBufferDesc* desc, const std::function<void(void* dest)>& init_callback, GPUBuffer* buffer, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const = 0;
		virtual bool CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const = 0;
		virtual bool CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader) const = 0;
		virtual bool CreateSampler(const SamplerDesc* desc, Sampler* sampler) const = 0;
		virtual bool CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const = 0;
		// Creates a graphics pipeline state. If renderpass_info is specified, then it will be only compatible with that renderpass info, but it will be created immediately (it can also take longer to be created)
		virtual bool CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso, const RenderPassInfo* renderpass_info = nullptr) const = 0;
		virtual bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const { return false; }
		virtual bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const { return false; }
		virtual bool CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const { return false; };

		virtual int CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change = nullptr, const ImageAspect* aspect = nullptr, const Swizzle* swizzle = nullptr, float min_lod_clamp = 0) const = 0;
		virtual int CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size = ~0, const Format* format_change = nullptr, const uint32_t* structuredbuffer_stride_change = nullptr) const = 0;

		virtual void DeleteSubresources(GPUResource* resource) = 0;

		virtual int GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource = -1) const = 0;
		virtual int GetDescriptorIndex(const Sampler* sampler) const = 0;

		virtual void WriteShadingRateValue(ShadingRate rate, void* dest) const {};
		virtual void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const {}
		virtual void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const {}

		// Set a debug name which will be visible in graphics debuggers
		virtual void SetName(GPUResource* pResource, const char* name) const {}
		virtual void SetName(Shader* shader, const char* name) const {}

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
		constexpr uint32_t GetBufferIndex() const { return GetFrameCount() % GetBufferCount(); }

		// Returns whether the graphics debug layer is enabled. It can be enabled when creating the device.
		constexpr bool IsDebugDevice() const { return validationMode != ValidationMode::Disabled; }

		// Get GPU-specific metrics:
		constexpr size_t GetShaderIdentifierSize() const { return SHADER_IDENTIFIER_SIZE; }
		constexpr size_t GetTopLevelAccelerationStructureInstanceSize() const { return TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE; }
		constexpr uint32_t GetVariableRateShadingTileSize() const { return VARIABLE_RATE_SHADING_TILE_SIZE; }
		constexpr uint64_t GetTimestampFrequency() const { return TIMESTAMP_FREQUENCY; }
		constexpr uint64_t GetVideoDecodeBitstreamAlignment() const { return VIDEO_DECODE_BITSTREAM_ALIGNMENT; }

		// Get information about the graphics device manufacturer:
		constexpr uint32_t GetVendorId() const { return vendorId; }
		constexpr uint32_t GetDeviceId() const { return deviceId; }
		constexpr const std::string& GetAdapterName() const { return adapterName; }
		constexpr const std::string& GetDriverDescription() const { return driverDescription; }
		constexpr AdapterType GetAdapterType() const { return adapterType; }

		// Get the shader binary format that the underlying graphics API consumes
		virtual ShaderFormat GetShaderFormat() const = 0;

		// Get a Texture resource that represents the current back buffer of the SwapChain
		virtual Texture GetBackBuffer(const SwapChain* swapchain) const = 0;
		// Returns the current color space of the swapchain output
		virtual ColorSpace GetSwapChainColorSpace(const SwapChain* swapchain) const = 0;
		// Returns true if the swapchain could support HDR output regardless of current format
		//	Returns false if the swapchain couldn't support HDR output
		virtual bool IsSwapChainSupportsHDR(const SwapChain* swapchain) const = 0;

		// Returns the minimum required alignment for buffer offsets when creating subresources
		virtual uint64_t GetMinOffsetAlignment(const GPUBufferDesc* desc) const = 0;

		struct MemoryUsage
		{
			uint64_t budget = 0ull;		// total video memory available for use by the current application (in bytes)
			uint64_t usage = 0ull;		// used video memory by the current application (in bytes)
		};
		// Returns video memory statistics for the current application
		virtual MemoryUsage GetMemoryUsage() const = 0;

		// Returns the maximum amount of viewports that can be bound at once
		virtual uint32_t GetMaxViewportCount() const = 0;

		// Performs a batched mapping of sparse resource pages to a tile pool
		virtual void SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count) {};

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Command List functions are below:
		//	- These are used to record rendering commands to a CommandList
		//	- To get a CommandList that can be recorded into, call BeginCommandList()
		//	- These commands are not immediately executed, but they begin executing on the GPU after calling SubmitCommandLists()
		//	- These are not thread safe, only a single thread should use a single CommandList at one time

		// Tell the command list to wait for an other command list which was started before it
		//	The granularity of this is at least that the beginning of the command list will wait for the end of the other command list
		//	On some platform like PS5 this can be implemented by waiting exactly at the wait insertion point within the command lists which is more precise
		virtual void WaitCommandList(CommandList cmd, CommandList wait_for) = 0;
		// Tell the command list to wait for the specified queue to finish processing
		//	It is useful when you want to wait for a previous frame, or just don't know which command list to wait for
		virtual void WaitQueue(CommandList cmd, QUEUE_TYPE wait_for) = 0;
		virtual void RenderPassBegin(const SwapChain* swapchain, CommandList cmd) = 0;
		virtual void RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags = RenderPassFlags::NONE) = 0;
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
		virtual void DrawInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd) = 0;
		virtual void DrawIndexedInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd) = 0;
		virtual void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) = 0;
		virtual void DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) = 0;
		virtual void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) {}
		virtual void DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) {}
		virtual void DispatchMeshIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd) {}
		virtual void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) = 0;
		virtual void CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd) = 0;
		virtual void CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox = nullptr, ImageAspect dst_aspect = ImageAspect::COLOR, ImageAspect src_aspect = ImageAspect::COLOR) = 0;
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
		virtual void ClearUAV(const GPUResource* resource, uint32_t value, CommandList cmd) = 0;
		virtual void VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd) {}

		virtual void EventBegin(const char* name, CommandList cmd) = 0;
		virtual void EventEnd(CommandList cmd) = 0;
		virtual void SetMarker(const char* name, CommandList cmd) = 0;

		virtual RenderPassInfo GetRenderPassInfo(CommandList cmd) = 0;

		// Some useful helpers:

		bool CreateBuffer(const GPUBufferDesc* desc, const void* initial_data, GPUBuffer* buffer, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const
		{
			if (initial_data == nullptr)
			{
				return CreateBuffer2(desc, nullptr, buffer, alias, alias_offset);
			}
			return CreateBuffer2(desc, [&](void* dest) { std::memcpy(dest, initial_data, desc->size); }, buffer, alias, alias_offset);
		}

		void Barrier(const GPUBarrier& barrier, CommandList cmd)
		{
			Barrier(&barrier, 1, cmd);
		}

		struct GPULinearAllocator
		{
			GPUBuffer buffer;
			uint64_t offset = 0ull;
			uint64_t alignment = 0ull;
			void reset()
			{
				offset = 0ull;
			}
		};
		virtual GPULinearAllocator& GetFrameAllocator(CommandList cmd) = 0;

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

			GPULinearAllocator& allocator = GetFrameAllocator(cmd);

			const uint64_t free_space = allocator.buffer.desc.size - allocator.offset;
			if (dataSize > free_space)
			{
				GPUBufferDesc desc;
				desc.usage = Usage::UPLOAD;
				desc.bind_flags = BindFlag::CONSTANT_BUFFER | BindFlag::VERTEX_BUFFER | BindFlag::INDEX_BUFFER | BindFlag::SHADER_RESOURCE;
				desc.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				allocator.alignment = GetMinOffsetAlignment(&desc);
				desc.size = AlignTo((allocator.buffer.desc.size + dataSize) * 2, allocator.alignment);
				CreateBuffer(&desc, nullptr, &allocator.buffer);
				SetName(&allocator.buffer, "frame_allocator");
				allocator.offset = 0;
			}

			allocation.buffer = allocator.buffer;
			allocation.offset = allocator.offset;
			allocation.data = (void*)((size_t)allocator.buffer.mapped_data + allocator.offset);

			allocator.offset += AlignTo(dataSize, allocator.alignment);

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

		// Deprecated, kept for back-compat:
		bool CreateRenderPass(const RenderPassDesc* desc, RenderPass* renderpass) const
		{
			renderpass->valid = true;
			renderpass->desc = *desc;
			return true;
		}
		// Deprecated, kept for back-compat:
		void RenderPassBegin(const RenderPass* renderpass, CommandList cmd)
		{
			RenderPassFlags flags = {};
			if (has_flag(renderpass->desc.flags, RenderPassDesc::Flags::ALLOW_UAV_WRITES))
			{
				flags |= RenderPassFlags::ALLOW_UAV_WRITES;
			}
			RenderPassImage rp[32] = {};
			for (size_t i = 0; i < renderpass->desc.attachments.size(); ++i)
			{
				rp[i] = renderpass->desc.attachments[i];
			}
			RenderPassBegin(rp, (uint32_t)renderpass->desc.attachments.size(), cmd, flags);
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
