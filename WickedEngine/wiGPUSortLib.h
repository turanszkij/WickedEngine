#ifndef _WI_GPU_SORTLIB_H_
#define _WI_GPU_SORTLIB_H_

#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

class wiGPUSortLib
{
private:
	static wiGraphicsTypes::GPUBuffer* indirectBuffer;
	static wiGraphicsTypes::GPUBuffer* sortCB;
	static wiGraphicsTypes::ComputeShader* kickoffSortCS;
	static wiGraphicsTypes::ComputeShader* sortCS;
	static wiGraphicsTypes::ComputeShader* sortInnerCS;
	static wiGraphicsTypes::ComputeShader* sortStepCS;
	static wiGraphicsTypes::ComputePSO CPSO_kickoffSort;
	static wiGraphicsTypes::ComputePSO CPSO_sort;
	static wiGraphicsTypes::ComputePSO CPSO_sortInner;
	static wiGraphicsTypes::ComputePSO CPSO_sortStep;

public:

	// Perform bitonic sort on a GPU dataset
	//	maxCount				-	Maximum size of the dataset. GPU count can be smaller (see: counterBuffer_read param)
	//	comparisonBuffer_read	-	Buffer containing values to compare by (Read Only)
	//	counterBuffer_read		-	Buffer containing count of values to sort (Read Only)
	//	counterReadOffset		-	Byte offset into the counter buffer to read the count value (Read Only)
	//	indexBuffer_write		-	The index list which to sort. Contains index values which can index the sortBase_read buffer. This will be modified (Read + Write)
	static void Sort(UINT maxCount, wiGraphicsTypes::GPUBuffer* comparisonBuffer_read, wiGraphicsTypes::GPUBuffer* counterBuffer_read, UINT counterReadOffset, wiGraphicsTypes::GPUBuffer* indexBuffer_write, GRAPHICSTHREAD threadID);

	static void Initialize();
	static void LoadShaders();
	static void CleanUpStatic();
};

#endif // _WI_GPU_SORTLIB_H_
