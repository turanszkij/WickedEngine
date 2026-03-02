#ifndef WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
#define WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
#include "ShaderInterop.h"

struct GaussianSplat
{
	float3 position;
	float radius;
	uint2 color;
	float3 cov3D_M11_M12_M13;
	float3 cov3D_M22_M23_M33;
};
struct GaussianSplatPush
{
	int sphericalHarmonicsDegree;
	int splatStride;
};

#endif // WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
