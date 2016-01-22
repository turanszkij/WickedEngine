#ifndef _FOGHF_
#define _FOGHF_
#include "globals.hlsli"

inline float getFog(float depth, float3 pos3D){
	return pow( saturate((depth - g_xWorld_Fog.x) / (g_xWorld_Fog.y - g_xWorld_Fog.x)),clamp(pos3D.y*g_xWorld_Fog.z,1,10000) );
}
inline float getFog(float depth){
	return saturate((depth - g_xWorld_Fog.x) / (g_xWorld_Fog.y - g_xWorld_Fog.x));
}
inline float3 applyFog(float3 color, float fog){
	return lerp(color, g_xWorld_Horizon,saturate(fog));
}

#endif // _FOGHF_
