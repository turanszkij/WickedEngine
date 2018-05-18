#ifndef _SHADERINTEROP_GPUSORTLIB_H_
#define _SHADERINTEROP_GPUSORTLIB_H_

#include "ShaderInterop.h"

// todo: remove!!
#include "ShaderInterop_EmittedParticle.h"

CBUFFER(SortConstants, CBSLOT_OTHER_GPUSORTLIB)
{
	uint counterReadOffset;
	int3 job_params;
};


#endif // _SHADERINTEROP_GPUSORTLIB_H_
