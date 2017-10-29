#ifndef _EMITTEDPARTICLE_HF_
#define _EMITTEDPARTICLE_HF_

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float4 pos2D			: TEXCOORD0;
	float2 tex				: TEXCOORD1;
	nointerpolation float2 opaSiz : TEXCOORD2;
};

#endif // _EMITTEDPARTICLE_HF_
