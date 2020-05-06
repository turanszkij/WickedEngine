#ifndef WI_EMITTEDPARTICLE_HF
#define WI_EMITTEDPARTICLE_HF

struct VertextoPixel
{
	float4 pos						: SV_POSITION;
	float4 pos2D					: TEXCOORD0;
	float4 tex						: TEXCOORD1;
	nointerpolation float size		: TEXCOORD2;
	nointerpolation uint color		: TEXCOORD3;
	float3 P : WORLDPOSITION;
	nointerpolation float frameBlend : FRAMEBLEND;
	float2 unrotated_uv : UNROTATED_UV;
};

#endif // WI_EMITTEDPARTICLE_HF
