#ifndef _SHADERINTEROP_GPUSORTLIB_H_
#define _SHADERINTEROP_GPUSORTLIB_H_

#include "ShaderInterop.h"

CBUFFER(SortConstants, CBSLOT_OTHER_GPUSORTLIB)
{
	int3 job_params;
	uint counterReadOffset;
};

#define __ReadSortElementCount__ counterBuffer.Load(counterReadOffset);


#endif // _SHADERINTEROP_GPUSORTLIB_H_
