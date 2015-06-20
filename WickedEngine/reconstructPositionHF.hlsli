#ifndef RECONSTRUCTPOSITION_HF
#define RECONSTRUCTPOSITION_HF

inline float3 getPosition(float2 texCoord, float z)
{
	float x = texCoord.x * 2.0f - 1.0f;
	float y = (1.0 - texCoord.y) * 2.0f - 1.0f;
	float4 position_s = float4(x, y, z, 1.0f);
	float4 position_v = mul(position_s, matProjInv);
	return position_v.xyz / position_v.w;
}
inline float4 getPositionScreen(float2 texCoord, float z)
{
	float x = texCoord.x * 2.0f - 1.0f;
	float y = (1.0 - texCoord.y) * 2.0f - 1.0f;
	float4 position_s = float4(x, y, z, 1.0f);
	return mul(position_s, matProjInv);
}

#endif