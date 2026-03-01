#ifndef WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
#define WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
#include "ShaderInterop.h"

struct GaussianSplat
{
	float3 position;
	float3 normal;
	float4 rotation;
	float3 scale;
	float3 f_dc;
	float opacity;
	float3 f_rest[15];
};

#endif // WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
