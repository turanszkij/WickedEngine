#include "wiGPUSortLib.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "shaders/ShaderInterop_GPUSortLib.h"
#include "wiEventHandler.h"
#include "wiTimer.h"
#include "wiBacklog.h"

using namespace wi::graphics;

namespace wi::gpusortlib
{
	static GPUBuffer indirectBuffer;
	static Shader kickoffSortCS;
	static Shader sortCS;
	static Shader sortInnerCS;
	static Shader sortStepCS;


	void LoadShaders()
	{
		wi::renderer::LoadShader(ShaderStage::CS, kickoffSortCS, "gpusortlib_kickoffSortCS.cso");
		wi::renderer::LoadShader(ShaderStage::CS, sortCS, "gpusortlib_sortCS.cso");
		wi::renderer::LoadShader(ShaderStage::CS, sortInnerCS, "gpusortlib_sortInnerCS.cso");
		wi::renderer::LoadShader(ShaderStage::CS, sortStepCS, "gpusortlib_sortStepCS.cso");

	}

	void Initialize()
	{
		wi::Timer timer;

		GPUBufferDesc bd;
		bd.usage = Usage::DEFAULT;
		bd.bind_flags = BindFlag::UNORDERED_ACCESS;
		bd.misc_flags = ResourceMiscFlag::INDIRECT_ARGS | ResourceMiscFlag::BUFFER_RAW;
		bd.size = sizeof(IndirectDispatchArgs);
		wi::graphics::GetDevice()->CreateBuffer(&bd, nullptr, &indirectBuffer);

		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		wilog("wi::gpusortlib Initialized (%d ms)", (int)std::round(timer.elapsed()));
	}


	void Sort(
		uint32_t maxCount, 
		const GPUBuffer& comparisonBuffer_read, 
		const GPUBuffer& counterBuffer_read, 
		uint32_t counterReadOffset, 
		const GPUBuffer& indexBuffer_write,
		CommandList cmd)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		device->EventBegin("GPUSortLib", cmd);


		SortConstants sort;
		sort.counterReadOffset = counterReadOffset;

		// initialize sorting arguments:
		{
			device->BindComputeShader(&kickoffSortCS, cmd);

			const GPUResource* res[] = {
				&counterBuffer_read,
			};
			device->BindResources(res, 0, arraysize(res), cmd);

			const GPUResource* uavs[] = {
				&indirectBuffer,
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Buffer(&indirectBuffer, ResourceState::INDIRECT_ARGUMENT, ResourceState::UNORDERED_ACCESS)
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->PushConstants(&sort, sizeof(sort), cmd);
			device->Dispatch(1, 1, 1, cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
					GPUBarrier::Buffer(&indirectBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::INDIRECT_ARGUMENT)
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

		}


		const GPUResource* uavs[] = {
			&indexBuffer_write,
		};
		device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

		const GPUResource* resources[] = {
			&counterBuffer_read,
			&comparisonBuffer_read,
		};
		device->BindResources(resources, 0, arraysize(resources), cmd);

		// initial sorting:
		bool bDone = true;
		{
			// calculate how many threads we'll require:
			//   we'll sort 512 elements per CU (threadgroupsize 256)
			//     maybe need to optimize this or make it changeable during init
			//     TGS=256 is a good intermediate value

			unsigned int numThreadGroups = ((maxCount - 1) >> 9) + 1;

			//assert(numThreadGroups <= 1024);

			if (numThreadGroups > 1)
			{
				bDone = false;
			}

			// sort all buffers of size 512 (and presort bigger ones)
			device->BindComputeShader(&sortCS, cmd);
			device->PushConstants(&sort, sizeof(sort), cmd);
			device->DispatchIndirect(&indirectBuffer, 0, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		int presorted = 512;
		while (!bDone)
		{
			// Incremental sorting:

			bDone = true;
			device->BindComputeShader(&sortStepCS, cmd);

			// prepare thread group description data
			uint32_t numThreadGroups = 0;

			if (maxCount > (uint32_t)presorted)
			{
				if (maxCount > (uint32_t)presorted * 2)
					bDone = false;

				uint32_t pow2 = presorted;
				while (pow2 < maxCount)
					pow2 *= 2;
				numThreadGroups = pow2 >> 9;
			}

			uint32_t nMergeSize = presorted * 2;
			for (uint32_t nMergeSubSize = nMergeSize >> 1; nMergeSubSize > 256; nMergeSubSize = nMergeSubSize >> 1)
			{
				SortConstants sort;
				sort.job_params.x = nMergeSubSize;
				if (nMergeSubSize == nMergeSize >> 1)
				{
					sort.job_params.y = (2 * nMergeSubSize - 1);
					sort.job_params.z = -1;
				}
				else
				{
					sort.job_params.y = nMergeSubSize;
					sort.job_params.z = 1;
				}
				sort.counterReadOffset = counterReadOffset;

				device->PushConstants(&sort, sizeof(sort), cmd);
				device->Dispatch(numThreadGroups, 1, 1, cmd);

				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->BindComputeShader(&sortInnerCS, cmd);
			device->PushConstants(&sort, sizeof(sort), cmd);
			device->Dispatch(numThreadGroups, 1, 1, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

			presorted *= 2;
		}



		device->EventEnd(cmd);
	}

}
