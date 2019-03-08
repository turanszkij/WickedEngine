#include "wiGPUSortLib.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "ShaderInterop_GPUSortLib.h"

using namespace wiGraphics;

namespace wiGPUSortLib
{
	static GPUBuffer indirectBuffer;
	static GPUBuffer sortCB;
	static const ComputeShader* kickoffSortCS = nullptr;
	static const ComputeShader* sortCS = nullptr;
	static const ComputeShader* sortInnerCS = nullptr;
	static const ComputeShader* sortStepCS = nullptr;
	static ComputePSO CPSO_kickoffSort;
	static ComputePSO CPSO_sort;
	static ComputePSO CPSO_sortInner;
	static ComputePSO CPSO_sortStep;

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
		bd.MiscFlags = RESOURCE_MISC_DRAWINDIRECT_ARGS | RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		bd.ByteWidth = sizeof(IndirectDispatchArgs);
		wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &indirectBuffer);

	}

	void LoadShaders()
	{
		std::string path = wiRenderer::GetShaderPath();

		kickoffSortCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "gpusortlib_kickoffSortCS.cso", wiResourceManager::COMPUTESHADER));
		sortCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "gpusortlib_sortCS.cso", wiResourceManager::COMPUTESHADER));
		sortInnerCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "gpusortlib_sortInnerCS.cso", wiResourceManager::COMPUTESHADER));
		sortStepCS = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "gpusortlib_sortStepCS.cso", wiResourceManager::COMPUTESHADER));


		GraphicsDevice* device = wiRenderer::GetDevice();

		ComputePSODesc desc;

		desc.cs = kickoffSortCS;
		device->CreateComputePSO(&desc, &CPSO_kickoffSort);

		desc.cs = sortCS;
		device->CreateComputePSO(&desc, &CPSO_sort);

		desc.cs = sortInnerCS;
		device->CreateComputePSO(&desc, &CPSO_sortInner);

		desc.cs = sortStepCS;
		device->CreateComputePSO(&desc, &CPSO_sortStep);
	}

	void CleanUp()
	{
	}


	void Sort(
		UINT maxCount, 
		const GPUBuffer& comparisonBuffer_read, 
		const GPUBuffer& counterBuffer_read, 
		UINT counterReadOffset, 
		const GPUBuffer& indexBuffer_write,
		GRAPHICSTHREAD threadID)
	{
		static bool init = false;
		if (!init)
		{
			Initialize();
			init = true;
		}


		GraphicsDevice* device = wiRenderer::GetDevice();

		device->EventBegin("GPUSortLib", threadID);


		SortConstants sc;
		sc.counterReadOffset = counterReadOffset;
		device->UpdateBuffer(&sortCB, &sc, threadID);
		device->BindConstantBuffer(CS, &sortCB, CB_GETBINDSLOT(SortConstants), threadID);

		device->UnbindUAVs(0, 8, threadID);

		// initialize sorting arguments:
		{
			device->BindComputePSO(&CPSO_kickoffSort, threadID);

			const GPUResource* res[] = {
				&counterBuffer_read,
			};
			device->BindResources(CS, res, 0, ARRAYSIZE(res), threadID);

			const GPUResource* uavs[] = {
				&indirectBuffer,
			};
			device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);

			device->Dispatch(1, 1, 1, threadID);
			device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);

			device->UnbindUAVs(0, ARRAYSIZE(uavs), threadID);
		}


		const GPUResource* uavs[] = {
			&indexBuffer_write,
		};
		device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), threadID);

		const GPUResource* resources[] = {
			&counterBuffer_read,
			&comparisonBuffer_read,
		};
		device->BindResources(CS, resources, 0, ARRAYSIZE(resources), threadID);

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
			device->BindComputePSO(&CPSO_sort, threadID);
			device->DispatchIndirect(&indirectBuffer, 0, threadID);
			device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		}

		int presorted = 512;
		while (!bDone)
		{
			// Incremental sorting:

			bDone = true;
			device->BindComputePSO(&CPSO_sortStep, threadID);

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

				device->UpdateBuffer(&sortCB, &sc, threadID);
				device->BindConstantBuffer(CS, &sortCB, CB_GETBINDSLOT(SortConstants), threadID);

				device->Dispatch(numThreadGroups, 1, 1, threadID);
				device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
			}

			device->BindComputePSO(&CPSO_sortInner, threadID);
			device->Dispatch(numThreadGroups, 1, 1, threadID);
			device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);

			presorted *= 2;
		}

		device->UnbindUAVs(0, ARRAYSIZE(uavs), threadID);
		device->UnbindResources(0, ARRAYSIZE(resources), threadID);


		device->EventEnd(threadID);
	}

}
