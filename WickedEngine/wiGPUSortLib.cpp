#include "wiGPUSortLib.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "ShaderInterop_GPUSortLib.h"
#include "wiEvent.h"

using namespace wiGraphics;

namespace wiGPUSortLib
{
	static GPUBuffer indirectBuffer;
	static GPUBuffer sortCB;
	static Shader kickoffSortCS;
	static Shader sortCS;
	static Shader sortInnerCS;
	static Shader sortStepCS;


	void LoadShaders()
	{
		std::string path = wiRenderer::GetShaderPath();

		wiRenderer::LoadShader(CS, kickoffSortCS, "gpusortlib_kickoffSortCS.cso");
		wiRenderer::LoadShader(CS, sortCS, "gpusortlib_sortCS.cso");
		wiRenderer::LoadShader(CS, sortInnerCS, "gpusortlib_sortInnerCS.cso");
		wiRenderer::LoadShader(CS, sortStepCS, "gpusortlib_sortStepCS.cso");

	}

	void Initialize()
	{
		GPUBufferDesc bd;

		bd.Usage = USAGE_DYNAMIC;
		bd.CPUAccessFlags = CPU_ACCESS_WRITE;
		bd.BindFlags = BIND_CONSTANT_BUFFER;
		bd.MiscFlags = 0;
		bd.ByteWidth = sizeof(SortConstants);
		wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &sortCB);


		bd.Usage = USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
		bd.BindFlags = BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_INDIRECT_ARGS | RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		bd.ByteWidth = sizeof(IndirectDispatchArgs);
		wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &indirectBuffer);

		static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();
	}


	void Sort(
		uint32_t maxCount, 
		const GPUBuffer& comparisonBuffer_read, 
		const GPUBuffer& counterBuffer_read, 
		uint32_t counterReadOffset, 
		const GPUBuffer& indexBuffer_write,
		CommandList cmd)
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		device->EventBegin("GPUSortLib", cmd);


		SortConstants sc;
		sc.counterReadOffset = counterReadOffset;
		device->UpdateBuffer(&sortCB, &sc, cmd);
		device->BindConstantBuffer(CS, &sortCB, CB_GETBINDSLOT(SortConstants), cmd);

		device->UnbindUAVs(0, 8, cmd);

		// initialize sorting arguments:
		{
			device->BindComputeShader(&kickoffSortCS, cmd);

			const GPUResource* res[] = {
				&counterBuffer_read,
			};
			device->BindResources(CS, res, 0, arraysize(res), cmd);

			const GPUResource* uavs[] = {
				&indirectBuffer,
			};
			device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

			device->Dispatch(1, 1, 1, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

			device->UnbindUAVs(0, arraysize(uavs), cmd);

			{
				GPUBarrier barrier;
				barrier.type = GPUBarrier::BUFFER_BARRIER;
				barrier.buffer.buffer = &indirectBuffer;
				barrier.buffer.state_before = BUFFER_STATE_UNORDERED_ACCESS;
				barrier.buffer.state_after = BUFFER_STATE_INDIRECT_ARGUMENT;
				device->Barrier(&barrier, 1, cmd);
			}
		}


		const GPUResource* uavs[] = {
			&indexBuffer_write,
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		const GPUResource* resources[] = {
			&counterBuffer_read,
			&comparisonBuffer_read,
		};
		device->BindResources(CS, resources, 0, arraysize(resources), cmd);

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
				SortConstants sc;
				sc.job_params.x = nMergeSubSize;
				if (nMergeSubSize == nMergeSize >> 1)
				{
					sc.job_params.y = (2 * nMergeSubSize - 1);
					sc.job_params.z = -1;
				}
				else
				{
					sc.job_params.y = nMergeSubSize;
					sc.job_params.z = 1;
				}
				sc.counterReadOffset = counterReadOffset;

				device->UpdateBuffer(&sortCB, &sc, cmd);
				device->BindConstantBuffer(CS, &sortCB, CB_GETBINDSLOT(SortConstants), cmd);

				device->Dispatch(numThreadGroups, 1, 1, cmd);

				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->BindComputeShader(&sortInnerCS, cmd);
			device->Dispatch(numThreadGroups, 1, 1, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

			presorted *= 2;
		}

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->UnbindResources(0, arraysize(resources), cmd);


		device->EventEnd(cmd);
	}

}
