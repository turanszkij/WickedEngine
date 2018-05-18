#ifndef _SHADERINTEROP_GPUSORTLIB_H_
#define _SHADERINTEROP_GPUSORTLIB_H_

#include "ShaderInterop.h"

CBUFFER(SortConstants, CBSLOT_OTHER_GPUSORTLIB)
{
	uint counterReadOffset;
	int3 job_params;
};

#define __ReadSortElementCount__ counterBuffer.Load(counterReadOffset);


#endif // _SHADERINTEROP_GPUSORTLIB_H_
