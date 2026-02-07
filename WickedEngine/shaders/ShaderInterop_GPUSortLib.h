#ifndef WI_SHADERINTEROP_GPUSORTLIB_H
#define WI_SHADERINTEROP_GPUSORTLIB_H

#include "ShaderInterop.h"

struct SortConstants
{
	int3 job_params;
	uint padding;
};

struct SortCount
{
	uint count;
};
CONSTANTBUFFER(counterBuffer, SortCount, CBSLOT_GPUSORTLIB);

#endif // WI_SHADERINTEROP_GPUSORTLIB_H
