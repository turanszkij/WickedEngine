#include "skinningDEF.h"
#include "globals.hlsli"

#ifdef USE_GPU_SKINNING
#define SKINNING_ON

typedef matrix<float,4,4> bonetype;

CBUFFER(BoneCB, CBSLOT_RENDERER_BONEBUFFER)
{
	bonetype pose[MAXBONECOUNT];
	bonetype prev[MAXBONECOUNT];
}

inline void Skinning(inout float4 pos, inout float4 posPrev, inout float4 inNor, inout float4 inBon, inout float4 inWei)
{
	[branch]if(inWei.x || inWei.y || inWei.z || inWei.w){
		bonetype sump=0, sumpPrev=0;
		float sumw = 0.0f;
		inWei=normalize(inWei);
		for(uint i=0;i<4;i++){
			sumw += inWei[i];
			sump += pose[(uint)inBon[i]] * inWei[i];
			sumpPrev += prev[(uint)inBon[i]] * inWei[i];
		}
		sump/=sumw;
		sumpPrev/=sumw;
		pos = mul(pos,sump);
		posPrev = mul(posPrev,sumpPrev);
		inNor.xyz = mul(inNor.xyz,(float3x3)sump);
	}
}
inline void Skinning(inout float4 pos, inout float4 inNor, inout float4 inBon, inout float4 inWei)
{
	[branch]if(inWei.x || inWei.y || inWei.z || inWei.w){
		bonetype sump=0;
		float sumw = 0.0f;
		inWei=normalize(inWei);
		for(uint i=0;i<4;i++){
			sumw += inWei[i];
			sump += pose[(uint)inBon[i]] * inWei[i];
		}
		sump/=sumw;
		pos = mul(pos,sump);
		inNor.xyz = mul(inNor.xyz,(float3x3)sump);
	}
}
inline void Skinning(inout float4 pos, inout float4 inBon, inout float4 inWei)
{
	[branch]if(inWei.x || inWei.y || inWei.z || inWei.w){
		bonetype sump=0;
		float sumw = 0.0f;
		inWei=normalize(inWei);
		for(uint i=0;i<4;i++){
			sumw += inWei[i];
			sump += pose[(uint)inBon[i]] * inWei[i];
		}
		sump/=sumw;
		pos = mul(pos,sump);
	}
}

#endif