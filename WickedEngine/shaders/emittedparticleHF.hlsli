#ifndef WI_EMITTEDPARTICLE_HF
#define WI_EMITTEDPARTICLE_HF

struct VertextoPixel
{
	float4 pos						: SV_POSITION;
	float4 tex						: TEXCOORD0;
	nointerpolation float size		: TEXCOORD1;
	nointerpolation uint color		: TEXCOORD2;
	float3 P : WORLDPOSITION;
	nointerpolation float frameBlend : FRAMEBLEND;
	float2 unrotated_uv : UNROTATED_UV;
};

#endif // WI_EMITTEDPARTICLE_HF
