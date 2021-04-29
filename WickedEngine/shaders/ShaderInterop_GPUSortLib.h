#ifndef WI_SHADERINTEROP_GPUSORTLIB_H
#define WI_SHADERINTEROP_GPUSORTLIB_H

#include "ShaderInterop.h"

CBUFFER(SortConstants, CBSLOT_OTHER_GPUSORTLIB)
{
	int3 job_params;
	uint counterReadOffset;
};

#endif // WI_SHADERINTEROP_GPUSORTLIB_H
