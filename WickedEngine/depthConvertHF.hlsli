#ifndef DEPTHCONVERT
#define DEPTHCONVERT
//#include "ViewProp.h"
#include "viewProp.hlsli"
inline float getLinearDepth(float4 c)
{
	//const float zNear = 0.1f, zFar=400.0f;
	float z_b = c.x;
    float z_n = 2.0 * z_b - 1.0;
    float lin = 2.0 * zNearP * zFarP / (zFarP + zNearP - z_n * (zFarP - zNearP));
	return lin;
}

#endif