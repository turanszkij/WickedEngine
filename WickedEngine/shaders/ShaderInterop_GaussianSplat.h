#ifndef WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
#define WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

struct GaussianSplat
{
	float3 position;
	float radius;
	uint2 color;
	float3 cov3D_M11_M12_M13;
	float3 cov3D_M22_M23_M33;
};
struct GaussianSplatCB
{
	ShaderTransform transform;
	ShaderTransform transform_inverse;
	int sphericalHarmonicsDegree;
	int splatStride;
	int padding0;
	int padding1;
};
CONSTANTBUFFER(cb, GaussianSplatCB, CBSLOT_GAUSSIANSPLAT);

#endif // WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
