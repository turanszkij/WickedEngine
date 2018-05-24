#ifndef DEPTHCONVERT
#define DEPTHCONVERT
#include "globals.hlsli"

inline float getLinearDepth(float c)
{
	float z_b = c;
    float z_n = 2.0 * z_b - 1.0;
	//float lin = 2.0 * g_xFrame_MainCamera_ZNearP * g_xFrame_MainCamera_ZFarP / (g_xFrame_MainCamera_ZFarP + g_xFrame_MainCamera_ZNearP - z_n * (g_xFrame_MainCamera_ZFarP - g_xFrame_MainCamera_ZNearP));
	float lin = 2.0 * g_xFrame_MainCamera_ZFarP * g_xFrame_MainCamera_ZNearP / (g_xFrame_MainCamera_ZNearP + g_xFrame_MainCamera_ZFarP - z_n * (g_xFrame_MainCamera_ZNearP - g_xFrame_MainCamera_ZFarP));
	return lin;
}

#endif