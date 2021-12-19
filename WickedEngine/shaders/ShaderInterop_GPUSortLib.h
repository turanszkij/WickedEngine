#ifndef WI_SHADERINTEROP_GPUSORTLIB_H
#define WI_SHADERINTEROP_GPUSORTLIB_H

#include "ShaderInterop.h"

struct SortConstants
{
	int3 job_params;
	uint counterReadOffset;
};
PUSHCONSTANT(sort, SortConstants);

#endif // WI_SHADERINTEROP_GPUSORTLIB_H
