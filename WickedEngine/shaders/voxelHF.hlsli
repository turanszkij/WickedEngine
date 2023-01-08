#ifndef WI_VOXEL_HF
#define WI_VOXEL_HF
#include "ColorSpaceUtility.hlsli"

static const float __hdrRange = 10.0f;		// HDR to SDR packing scale
static const uint MAX_VOXEL_RGB = 511;		// 9 bits for RGB
static const uint MAX_VOXEL_ALPHA = 31;		// 5 bits for alpha (alpha is needed for atomic average)
static const float DARK_PACKING_POW = 8;	// improves precision for dark colors

uint PackVoxelColor(in float4 color)
{
	color.rgb /= __hdrRange;
	color = saturate(color);
	color.rgb = pow(color.rgb, 1.0 / DARK_PACKING_POW);
	uint retVal = 0;
	retVal |= (uint)(color.r * MAX_VOXEL_RGB) << 0u;
	retVal |= (uint)(color.g * MAX_VOXEL_RGB) << 9u;
	retVal |= (uint)(color.b * MAX_VOXEL_RGB) << 18u;
	retVal |= (uint)(color.a * MAX_VOXEL_ALPHA) << 27u;
	return retVal;
}

float4 UnpackVoxelColor(in uint colorMask)
{
	float4 retVal;
	retVal.r = (float)((colorMask >> 0u) & MAX_VOXEL_RGB) / MAX_VOXEL_RGB;
	retVal.g = (float)((colorMask >> 9u) & MAX_VOXEL_RGB) / MAX_VOXEL_RGB;
	retVal.b = (float)((colorMask >> 18u) & MAX_VOXEL_RGB) / MAX_VOXEL_RGB;
	retVal.a = (float)((colorMask >> 27u) & MAX_VOXEL_ALPHA) / MAX_VOXEL_ALPHA;
	retVal = saturate(retVal);
	retVal.rgb = pow(retVal.rgb, DARK_PACKING_POW);
	retVal.rgb *= __hdrRange;
	return retVal;
}

uint PackVoxelChannel(float value)
{
	return uint(value * 1024);
}
float UnpackVoxelChannel(uint value)
{
	return float(value) / 1024.0f;
}

#endif // WI_VOXEL_HF
