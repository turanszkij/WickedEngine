#ifndef WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
#define WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

struct GaussianSplat
{
	uint2 position_radius;				// unorm16x3 | half
	uint2 color;						// half4
	uint2 cov3D_M11_M12_M13;			// half3
	uint2 cov3D_M22_M23_M33;			// half3
};
struct ShaderGaussianSplatModel
{
	ShaderTransform transform;
	ShaderTransform transform_inverse;
	float3 aabb_min;					// for position remapping from unorm16x3 to float3
	float maxScale;						// for culling radius modification
	float3 aabb_max;					// for position remapping from unorm16x3 to float3
	float padding0;
	float4x4 modelViewMatrices[6];		// 6 for cubemap rendering
	int sphericalHarmonicsDegree;		// 0, 1, 2 or 3
	int splatStride;					// element count of shBuffer per splat
	int descriptor_splatBuffer;			// pointer to StructuredBuffer<GaussianSplat> (one element per splat)
	int descriptor_shBuffer;			// pointer to Buffer<half> (splatStride element count per splat)
};

static const uint GAUSSIAN_COMPUTE_THREADSIZE = 64;

#endif // WI_SHADERINTEROP_GAUSSIAN_SPLAT_H
