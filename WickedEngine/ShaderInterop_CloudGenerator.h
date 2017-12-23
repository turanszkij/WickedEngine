#ifndef _SHADERINTEROP_CLOUDGEN_H_
#define _SHADERINTEROP_CLOUDGEN_H_
#include "ShaderInterop.h"

#define CLOUDGENERATOR_BLOCKSIZE 8

CBUFFER(CloudGeneratorCB, CBSLOT_OTHER_CLOUDGENERATOR)
{
	float2 xNoiseTexDim;
	float xRandomness;
	uint xRefinementCount;
};


#endif // _SHADERINTEROP_CLOUDGEN_H_
