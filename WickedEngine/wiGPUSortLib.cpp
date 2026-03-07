#include "wiGPUSortLib.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiEventHandler.h"
#include "wiTimer.h"
#include "wiBacklog.h"

#define FFX_CPP
#include "shaders/ffx-parallelsort/FFX_ParallelSort.h"

using namespace wi::graphics;

namespace wi::gpusortlib
{
	static Shader radix_countCS;
	static Shader radix_reduceCS;
	static Shader radix_scanCS;
	static Shader radix_scanaddCS;
	static Shader radix_scatterCS;
	static Shader radix_scatter_payloadCS;
	static Shader radix_indirectCS;

	GPUBuffer             m_TmpKeyBuffer;     // 32 bit destination key buffer (when not doing in place writes)
	GPUBuffer             m_TmpPayloadBuffer; // 32 bit destination payload buffer (when not doing in place writes)

	// Resources         for parallel sort algorithm
	GPUBuffer             m_FPSScratchBuffer;             // Sort scratch buffer
	GPUBuffer             m_FPSReducedScratchBuffer;      // Sort reduced scratch buffer
	// Resources for indirect execution of algorithm
	GPUBuffer m_IndirectCountBuffer;
	GPUBuffer             m_IndirectConstantBuffer;       // Buffer to hold radix sort constant buffer data for indirect dispatch
	GPUBuffer             m_IndirectCountScatterArgs;     // Buffer to hold dispatch arguments used for Count/Scatter parts of the algorithm
	GPUBuffer             m_IndirectReduceScanArgs;       // Buffer to hold dispatch arguments used for Reduce/Scan parts of the algorithm


	void LoadShaders()
	{
		wi::renderer::LoadShader(ShaderStage::CS, radix_countCS, "radix_sortCS.cso", ShaderModel::SM_6_0, {"FPS_COUNT"}, "FPS_Count");
		wi::renderer::LoadShader(ShaderStage::CS, radix_reduceCS, "radix_sortCS.cso", ShaderModel::SM_6_0, {"FPS_COUNT_REDUCE"}, "FPS_CountReduce");
		wi::renderer::LoadShader(ShaderStage::CS, radix_scanCS, "radix_sortCS.cso", ShaderModel::SM_6_0, {"FPS_SCAN"}, "FPS_Scan");
		wi::renderer::LoadShader(ShaderStage::CS, radix_scanaddCS, "radix_sortCS.cso", ShaderModel::SM_6_0, {"FPS_SCAN_ADD"}, "FPS_ScanAdd");
		wi::renderer::LoadShader(ShaderStage::CS, radix_scatterCS, "radix_sortCS.cso", ShaderModel::SM_6_0, {"FPS_SCATTER"}, "FPS_Scatter");
		wi::renderer::LoadShader(ShaderStage::CS, radix_scatter_payloadCS, "radix_sortCS.cso", ShaderModel::SM_6_0, {"FPS_SCATTER", "kRS_ValueCopy"}, "FPS_Scatter");
		wi::renderer::LoadShader(ShaderStage::CS, radix_indirectCS, "radix_sortCS.cso", ShaderModel::SM_6_0, {"FPS_INDIRECT"}, "FPS_SetupIndirectParameters");
	}

	void Initialize()
	{
		wi::Timer timer;

		GraphicsDevice* device = GetDevice();

		GPUBufferDesc bd;

		bd.stride = sizeof(FFX_ParallelSortCB);
		bd.size = bd.stride;
		bd.bind_flags = BindFlag::CONSTANT_BUFFER | BindFlag::UNORDERED_ACCESS;
		bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
		device->CreateBuffer(&bd, nullptr, &m_IndirectConstantBuffer);
		device->SetName(&m_IndirectConstantBuffer, "gpusortlib::m_IndirectConstantBuffer");

		bd.stride = sizeof(uint32_t);
		bd.size = bd.stride * 3;
		bd.bind_flags = BindFlag::UNORDERED_ACCESS;
		bd.misc_flags = ResourceMiscFlag::INDIRECT_ARGS | ResourceMiscFlag::BUFFER_STRUCTURED;
		device->CreateBuffer(&bd, nullptr, &m_IndirectCountScatterArgs);
		device->SetName(&m_IndirectCountScatterArgs, "gpusortlib::m_IndirectCountScatterArgs");
		device->CreateBuffer(&bd, nullptr, &m_IndirectReduceScanArgs);
		device->SetName(&m_IndirectReduceScanArgs, "gpusortlib::m_IndirectReduceScanArgs");
		bd.size = bd.stride;
		device->CreateBuffer(&bd, nullptr, &m_IndirectCountBuffer);
		device->SetName(&m_IndirectCountBuffer, "gpusortlib::m_IndirectCountBuffer");

		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		wilog("wi::gpusortlib Initialized (%d ms)", (int)std::round(timer.elapsed()));
	}


	void Sort(
		uint32_t maxCount, 
		const GPUBuffer& comparisonBuffer_read, 
		const GPUBuffer& counterBuffer_read,
		uint counterReadOffset,
		const GPUBuffer& indexBuffer_write,
		CommandList cmd)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		uint32_t scratchBufferSize;
		uint32_t reducedScratchBufferSize;
		FFX_ParallelSort_CalculateScratchResourceSize(maxCount, scratchBufferSize, reducedScratchBufferSize);

		if (m_FPSScratchBuffer.desc.size < scratchBufferSize)
		{
			GPUBufferDesc bd;
			bd.stride = sizeof(uint32_t);
			bd.size = scratchBufferSize;
			bd.bind_flags = BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			device->CreateBuffer(&bd, nullptr, &m_FPSScratchBuffer);
			device->SetName(&m_FPSScratchBuffer, "gpusortlib::m_FPSScratchBuffer");
		}
		if (m_FPSReducedScratchBuffer.desc.size < reducedScratchBufferSize)
		{
			GPUBufferDesc bd;
			bd.stride = sizeof(uint32_t);
			bd.size = reducedScratchBufferSize;
			bd.bind_flags = BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			device->CreateBuffer(&bd, nullptr, &m_FPSReducedScratchBuffer);
			device->SetName(&m_FPSReducedScratchBuffer, "gpusortlib::m_FPSReducedScratchBuffer");
		}
		if (m_TmpKeyBuffer.desc.size < comparisonBuffer_read.desc.size)
		{
			GPUBufferDesc bd;
			bd.stride = sizeof(uint32_t);
			bd.size = comparisonBuffer_read.desc.size;
			bd.bind_flags = BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			device->CreateBuffer(&bd, nullptr, &m_TmpKeyBuffer);
			device->SetName(&m_TmpKeyBuffer, "gpusortlib::m_TmpKeyBuffer");
		}
		if (m_TmpPayloadBuffer.desc.size < indexBuffer_write.desc.size)
		{
			GPUBufferDesc bd;
			bd.stride = sizeof(uint32_t);
			bd.size = indexBuffer_write.desc.size;
			bd.bind_flags = BindFlag::UNORDERED_ACCESS;
			bd.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			device->CreateBuffer(&bd, nullptr, &m_TmpPayloadBuffer);
			device->SetName(&m_TmpPayloadBuffer, "gpusortlib::m_TmpPayloadBuffer");
		}

		device->EventBegin("GPUSortLib", cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&counterBuffer_read,ResourceState::SHADER_RESOURCE,ResourceState::COPY_SRC),
				GPUBarrier::Buffer(&m_IndirectCountBuffer,ResourceState::UNORDERED_ACCESS,ResourceState::COPY_DST),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}
		device->CopyBuffer(&m_IndirectCountBuffer, 0, &counterBuffer_read, counterReadOffset, sizeof(uint32_t), cmd);
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&counterBuffer_read,ResourceState::COPY_SRC,ResourceState::SHADER_RESOURCE),
				GPUBarrier::Buffer(&m_IndirectCountBuffer,ResourceState::COPY_DST,ResourceState::UNORDERED_ACCESS),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		static constexpr uint32_t m_MaxNumThreadgroups = 800; // Use a generic thread group size when not on AMD hardware (taken from experiments to determine best performance threshold)

		struct SetupIndirectCB
		{
			uint32_t NumKeysIndex;
			uint32_t MaxThreadGroups;
		};
		SetupIndirectCB IndirectSetupCB;
		IndirectSetupCB.NumKeysIndex = 0;
		IndirectSetupCB.MaxThreadGroups = m_MaxNumThreadgroups;
		device->BindDynamicConstantBuffer(IndirectSetupCB, CBSLOT_GPUSORTLIB, cmd);

		device->BindUAV(&m_IndirectCountBuffer, 9, cmd);
		device->BindUAV(&m_IndirectConstantBuffer, 10, cmd);
		device->BindUAV(&m_IndirectCountScatterArgs, 11, cmd);
		device->BindUAV(&m_IndirectReduceScanArgs, 12, cmd);

		device->BindComputeShader(&radix_indirectCS, cmd);
		device->Dispatch(1, 1, 1, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Buffer(&m_IndirectConstantBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::CONSTANT_BUFFER),
				GPUBarrier::Buffer(&m_IndirectCountScatterArgs, ResourceState::UNORDERED_ACCESS, ResourceState::INDIRECT_ARGUMENT),
				GPUBarrier::Buffer(&m_IndirectReduceScanArgs, ResourceState::UNORDERED_ACCESS, ResourceState::INDIRECT_ARGUMENT),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		GPUBuffer m_DstKeyBuffers[2] = { comparisonBuffer_read, m_TmpKeyBuffer };
		GPUBuffer m_DstPayloadBuffers[2] = { indexBuffer_write, m_TmpPayloadBuffer };

		// Setup resource/UAV pairs to use during sort
		const GPUBuffer& KeySrcInfo = m_DstKeyBuffers[0];
		const GPUBuffer& PayloadSrcInfo = m_DstPayloadBuffers[0];
		const GPUBuffer& KeyTmpInfo = m_DstKeyBuffers[1];
		const GPUBuffer& PayloadTmpInfo = m_DstPayloadBuffers[1];
		const GPUBuffer& ScratchBufferInfo = m_FPSScratchBuffer;
		const GPUBuffer& ReducedScratchBufferInfo = m_FPSReducedScratchBuffer;

		// Buffers to ping-pong between when writing out sorted values
		const GPUBuffer* ReadBufferInfo(&KeySrcInfo), * WriteBufferInfo(&KeyTmpInfo);
		const GPUBuffer* ReadPayloadBufferInfo(&PayloadSrcInfo), * WritePayloadBufferInfo(&PayloadTmpInfo);
		static bool bHasPayload = true;

		// Setup barriers for the run
		GPUBarrier barriers[3];

		// Perform Radix Sort (currently only support 32-bit key/payload sorting
		for (uint32_t Shift = 0; Shift < 32u; Shift += FFX_PARALLELSORT_SORT_BITS_PER_PASS)
		{
			// Copy the data into the constant buffer
			const GPUBuffer& constantBuffer = m_IndirectConstantBuffer;

			device->BindConstantBuffer(&constantBuffer, CBSLOT_OTHER_GPUSORTLIB, cmd);
			device->BindUAV(ReadBufferInfo, 1, cmd);
			device->BindUAV(&ScratchBufferInfo, 3, cmd);

			// Sort Count
			{
				device->BindComputeShader(&radix_countCS, cmd);
				device->PushConstants(&Shift, sizeof(Shift), cmd);
				device->DispatchIndirect(&m_IndirectCountScatterArgs, 0, cmd);
			}

			// UAV barrier on the sum table
			device->Barrier(GPUBarrier::Memory(&ScratchBufferInfo), cmd);

			device->BindUAV(&ReducedScratchBufferInfo, 3, cmd);

			// Sort Reduce
			{
				device->BindComputeShader(&radix_reduceCS, cmd);
				device->PushConstants(&Shift, sizeof(Shift), cmd);
				device->DispatchIndirect(&m_IndirectReduceScanArgs, 0, cmd);

				// UAV barrier on the reduced sum table
				device->Barrier(GPUBarrier::Memory(&ReducedScratchBufferInfo), cmd);
			}

			// Sort Scan
			{
				// First do scan prefix of reduced values
				device->BindUAV(&ReducedScratchBufferInfo, 6, cmd);
				device->BindUAV(&ReducedScratchBufferInfo, 7, cmd);

				device->BindComputeShader(&radix_scanCS, cmd);
				device->PushConstants(&Shift, sizeof(Shift), cmd);
				device->Dispatch(1, 1, 1, cmd);

				// UAV barrier on the reduced sum table
				device->Barrier(GPUBarrier::Memory(&ReducedScratchBufferInfo), cmd);

				// Next do scan prefix on the histogram with partial sums that we just did
				device->BindUAV(&ScratchBufferInfo, 6, cmd);
				device->BindUAV(&ScratchBufferInfo, 7, cmd);
				device->BindUAV(&ReducedScratchBufferInfo, 8, cmd);

				device->BindComputeShader(&radix_scanaddCS, cmd);
				device->PushConstants(&Shift, sizeof(Shift), cmd);
				device->DispatchIndirect(&m_IndirectReduceScanArgs, 0, cmd);
			}

			// UAV barrier on the sum table
			device->Barrier(GPUBarrier::Memory(&ScratchBufferInfo), cmd);

			if (bHasPayload)
			{
				device->BindUAV(ReadPayloadBufferInfo, 1, cmd);
				device->BindUAV(WritePayloadBufferInfo, 5, cmd);
			}

			device->BindUAV(WriteBufferInfo, 4, cmd);

			// Sort Scatter
			{
				device->BindComputeShader(bHasPayload ? &radix_scatter_payloadCS : &radix_scatterCS, cmd);
				device->PushConstants(&Shift, sizeof(Shift), cmd);
				device->DispatchIndirect(&m_IndirectCountScatterArgs, 0, cmd);
			}

			// Finish doing everything and barrier for the next pass
			int numBarriers = 0;
			barriers[numBarriers++] = GPUBarrier::Memory(WriteBufferInfo);
			if (bHasPayload)
				barriers[numBarriers++] = GPUBarrier::Memory(WritePayloadBufferInfo);
			device->Barrier(barriers, numBarriers, cmd);

			// Swap read/write sources
			std::swap(ReadBufferInfo, WriteBufferInfo);
			if (bHasPayload)
				std::swap(ReadPayloadBufferInfo, WritePayloadBufferInfo);
		}

		//barriers[0] = GPUBarrier::Buffer(&m_IndirectCountScatterArgs, ResourceState::INDIRECT_ARGUMENT, ResourceState::UNORDERED_ACCESS);
		//barriers[1] = GPUBarrier::Buffer(&m_IndirectReduceScanArgs, ResourceState::INDIRECT_ARGUMENT, ResourceState::UNORDERED_ACCESS);
		//barriers[2] = GPUBarrier::Buffer(&m_IndirectConstantBuffer, ResourceState::CONSTANT_BUFFER, ResourceState::UNORDERED_ACCESS);
		//device->Barrier(barriers, arraysize(barriers), cmd);

		device->EventEnd(cmd);
	}

}
