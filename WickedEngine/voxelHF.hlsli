#ifndef _VOXEL_HF_
#define _VOXEL_HF_

// Encode specified color (range 0.0f-1.0f), so that each channel is
// stored in 8 bits of an unsigned integer.
uint EncodeColor(in float4 color)
{
	uint4 iColor = uint4(color*255.0f);
	uint colorMask = (iColor.a << 24u) | (iColor.r << 16u) | (iColor.g << 8u) | iColor.b;
	return colorMask;
}

// Decode specified mask into a float3 color (range 0.0f-1.0f).
float4 DecodeColor(in uint colorMask)
{
	float4 color;
	color.a = (colorMask >> 24u) & 0x000000ff;
	color.r = (colorMask >> 16u) & 0x000000ff;
	color.g = (colorMask >> 8u) & 0x000000ff;
	color.b = colorMask & 0x000000ff;
	color /= 255.0f;
	return color;
}

// Encode specified normal (normalized) into an unsigned integer. Each axis of
// the normal is encoded into 9 bits (1 for the sign/ 8 for the value).
uint EncodeNormal(in float3 normal)
{
	int3 iNormal = int3(normal*255.0f);
	uint3 iNormalSigns;
	iNormalSigns.x = (iNormal.x >> 5) & 0x04000000;
	iNormalSigns.y = (iNormal.y >> 14) & 0x00020000;
	iNormalSigns.z = (iNormal.z >> 23) & 0x00000100;
	iNormal = abs(iNormal);
	uint normalMask = iNormalSigns.x | (iNormal.x << 18) | iNormalSigns.y | (iNormal.y << 9) | iNormalSigns.z | iNormal.z;
	return normalMask;
}

// Decode specified mask into a float3 normal (normalized).
float3 DecodeNormal(in uint normalMask)
{
	int3 iNormal;
	iNormal.x = (normalMask >> 18) & 0x000000ff;
	iNormal.y = (normalMask >> 9) & 0x000000ff;
	iNormal.z = normalMask & 0x000000ff;
	int3 iNormalSigns;
	iNormalSigns.x = (normalMask >> 25) & 0x00000002;
	iNormalSigns.y = (normalMask >> 16) & 0x00000002;
	iNormalSigns.z = (normalMask >> 7) & 0x00000002;
	iNormalSigns = 1 - iNormalSigns;
	float3 normal = float3(iNormal) / 255.0f;
	normal *= iNormalSigns;
	return normal;
}

#endif