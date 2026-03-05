#ifndef WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
#define WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

struct GaussianSplat
{
	float3 position;
	uint2 color;						// half4
	uint2 cov3D_M11_M12_M13_radius;		// half4
	uint2 cov3D_M22_M23_M33;			// half3
};
struct ShaderGaussianSplatModel
{
	ShaderTransform transform;
	ShaderTransform transform_inverse;
	float4x4 modelViewMatrices[6]; // 6 for cubemap rendering
	int sphericalHarmonicsDegree;
	int splatStride;
	int descriptor_splatBuffer;
	int descriptor_shBuffer;
};

static const uint GAUSSIAN_COMPUTE_THREADSIZE = 64;

#endif // WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
