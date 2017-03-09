#ifndef _WIND_HF_
#define _WIND_HF_

#include "globals.hlsli"

inline void affectWind(inout float3 pos, in float value, in uint randVertex){
	float3 wind = sin(g_xFrame_Time +(pos.x+pos.y+pos.z))*g_xFrame_WindDirection.xyz*0.1*value;
	pos+=wind;
}

#endif // _WIND_HF_
