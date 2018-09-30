#ifndef _HAIRPARTICLE_HF_
#define _HAIRPARTICLE_HF_

struct VertexToPixel
{
	float4 pos : SV_POSITION;
	float3 pos3D : POSITION3D;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float  fade : DITHERFADE;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
	float3 color : COLOR;
};

#endif // _HAIRPARTICLE_HF_
