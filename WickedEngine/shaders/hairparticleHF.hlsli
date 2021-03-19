#ifndef WI_HAIRPARTICLE_HF
#define WI_HAIRPARTICLE_HF

struct VertexToPixel
{
	float4 pos : SV_POSITION;
	float3 pos3D : POSITION3D;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float  fade : DITHERFADE;
	float4 pos2DPrev : SCREENPOSITIONPREV;
	float3 color : COLOR;
};

#endif // WI_HAIRPARTICLE_HF
