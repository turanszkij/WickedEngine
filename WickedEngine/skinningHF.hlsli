#include "globals.hlsli"

#define SKINNING_ON

typedef row_major matrix<float,4,4> bonetype;

struct Bone
{
	bonetype pose, prev;
};

STRUCTUREDBUFFER(boneBuffer, Bone, SBSLOT_BONE);

inline void Skinning(inout float4 pos, inout float4 posPrev, inout float4 inNor, in float4 inBon, in float4 inWei)
{
	[branch]
	if(any(inWei))
	{
		bonetype sump = 0, sumpPrev = 0;
		float sumw = 0.0f;
		inWei = normalize(inWei);
		for (uint i = 0; i < 4; i++)
		{
			sumw += inWei[i];
			sump += boneBuffer[(uint)inBon[i]].pose * inWei[i];
			sumpPrev += boneBuffer[(uint)inBon[i]].prev * inWei[i];
		}
		sump /= sumw;
		sumpPrev /= sumw;
		pos = mul(pos, sump);
		posPrev = mul(posPrev, sumpPrev);
		inNor.xyz = mul(inNor.xyz, (float3x3)sump);
	}
}
