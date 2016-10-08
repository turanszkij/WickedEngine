#ifndef DEPTHCONVERT
#define DEPTHCONVERT
#include "globals.hlsli"

inline float getLinearDepth(float4 c)
{
	float z_b = c.x;
    float z_n = 2.0 * z_b - 1.0;
    float lin = 2.0 * g_xFrame_MainCamera_ZNearP * g_xFrame_MainCamera_ZFarP / (g_xFrame_MainCamera_ZFarP + g_xFrame_MainCamera_ZNearP - z_n * (g_xFrame_MainCamera_ZFarP - g_xFrame_MainCamera_ZNearP));
	return lin;
}

#endif