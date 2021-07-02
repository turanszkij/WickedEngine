#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input_edgeMap, uint4, TEXSLOT_ONDEMAND0); // DXGI_FORMAT_R32G32B32A32_UINT

RWTEXTURE2D(input_output_normals, unorm float4, 0); // DXGI_FORMAT_R8G8B8A8_UNORM


// resolution of Gbuffer normals texture
uint2 GetResolution()
{
	return xPPResolution;
}

// 1.0f / resolution of Gbuffer normals texture
float2 GetResolutionRcp()
{
	return xPPResolution_rcp;
}

// it should return pow(2.0f, float(passcount - current_pass - 1));
//	where passcount is the number of passes: (int)ceil(log2((float)max(resolutionWidth, resolutionHeight)));
//	and current_pass is the zero-based pass number
float GetJumpFloodRadius()
{
	return xPPParams0.x;
}

// inverse of camera's view-projection matrix (column-major layout is assumed)
float4x4 GetCameraInverseViewProjection()
{
	return g_xCamera_InvVP;
}

float LoadDepth(uint2 pixel)
{
	return texture_depth[pixel];
}
float LoadLinearDepth(uint2 pixel)
{
	return texture_lineardepth[pixel];
}
float4 LoadNormal(uint2 pixel)
{
	float4 normal = input_output_normals[pixel];
	normal.xyz = normalize(normal.xyz * 2 - 1);
	return normal;
}
void StoreNormal(uint2 pixel, float4 normal)
{
	normal.xyz = normal.xyz * 0.5 + 0.5;
	input_output_normals[pixel] = normal;
}
uint4 LoadEdgeMap(uint2 pixel)
{
	return input_edgeMap[pixel];
}

#define CHAMFERNORMALS_TWISTNORMAL_IMPLEMENTATION
#include "chamfernormals_util.h"
