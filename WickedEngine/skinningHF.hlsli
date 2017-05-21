#include "globals.hlsli"

#define SKINNING_ON

struct Bone
{
	float4x4 pose, prev;
};

STRUCTUREDBUFFER(boneBuffer, Bone, SBSLOT_BONE);

inline void Skinning(inout float4 pos, inout float4 posPrev, inout float4 nor, in float4 inBon, in float4 inWei)
{
	float4 p = 0, pp = 0;
	float3 n = 0;
	float4x4 m, mp;
	float3x3 m3;

	[unroll]
	for (uint i = 0; i < 4; i++)
	{
		m = boneBuffer[(uint)inBon[i]].pose;
		mp = boneBuffer[(uint)inBon[i]].prev;
		m3 = (float3x3)m;

		p += mul(float4(pos.xyz, 1), m)*inWei[i];
		pp += mul(float4(posPrev.xyz, 1), mp)*inWei[i];
		n += mul(nor.xyz, m3)*inWei[i];
	}

	bool w = any(inWei);
	pos.xyz = w ? p.xyz : pos.xyz;
	posPrev.xyz = w ? pp.xyz : posPrev.xyz;
	nor.xyz = w ? n : nor.xyz;
}
