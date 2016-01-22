#ifndef DEPTHCONVERT
#define DEPTHCONVERT
#include "globals.hlsli"

inline float getLinearDepth(float4 c)
{
	float z_b = c.x;
    float z_n = 2.0 * z_b - 1.0;
    float lin = 2.0 * g_xCamera_ZNearP * g_xCamera_ZFarP / (g_xCamera_ZFarP + g_xCamera_ZNearP - z_n * (g_xCamera_ZFarP - g_xCamera_ZNearP));
	return lin;
}

#endif