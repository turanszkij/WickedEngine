#ifndef _FOG_HF_
#define _FOG_HF_
#include "globals.hlsli"

inline float GetFog(float dist)
{
	return saturate((dist - g_xFrame_Fog.x) / (g_xFrame_Fog.y - g_xFrame_Fog.x));
}

#endif // _FOGHF_
