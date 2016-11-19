#ifndef RECONSTRUCTPOSITION_HF
#define RECONSTRUCTPOSITION_HF
#include "globals.hlsli"

inline float3 getPositionEx(in float2 texCoord, in float z, in float4x4 InvVP)
{
	float x = texCoord.x * 2.0f - 1.0f;
	float y = (1.0 - texCoord.y) * 2.0f - 1.0f;
	float4 position_s = float4(x, y, z, 1.0f);
	float4 position_v = mul(position_s, InvVP);
	return position_v.xyz / position_v.w;
}
inline float3 getPosition(in float2 texCoord, in float z)
{
	return getPositionEx(texCoord, z, g_xFrame_MainCamera_InvVP);
}

inline float4 getPositionScreenEx(in float2 texCoord, in float z)
{
	float x = texCoord.x * 2.0f - 1.0f;
	float y = (1.0 - texCoord.y) * 2.0f - 1.0f;
	float4 position_s = float4(x, y, z, 1.0f);
	return mul(position_s, g_xFrame_MainCamera_InvVP);
}

#endif