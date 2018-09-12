#ifndef _HAIRPARTICLE_HF_
#define _HAIRPARTICLE_HF_


CBUFFER(HairParticleCB, CBSLOT_OTHER_HAIRPARTICLE)
{
	float4x4 xWorld;
	float4 xColor;
	float LOD0;
	float LOD1;
	float LOD2;
	float __pad1;
}

struct VertexToPixel
{
	float4 pos : SV_POSITION;
	float3 pos3D : POSITION3D;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float  fade : DITHERFADE;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

#endif // _HAIRPARTICLE_HF_
