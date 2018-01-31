#ifndef _FOG_HF_
#define _FOG_HF_
#include "globals.hlsli"

inline float GetFog(float dist)
{
	return saturate((dist - g_xWorld_Fog.x) / (g_xWorld_Fog.y - g_xWorld_Fog.x));
}

#endif // _FOGHF_
