#ifndef _SHADERINTEROP_FFTGENERATOR_H_
#define _SHADERINTEROP_FFTGENERATOR_H_
#include "ShaderInterop.h"

CBUFFER(FFTGeneratorCB, CBSLOT_OTHER_FFTGENERATOR)
{
	uint thread_count;
	uint ostride;
	uint istride;
	uint pstride;

	float phase_base;
};

#endif // _SHADERINTEROP_FFTGENERATOR_H_
