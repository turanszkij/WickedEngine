#ifndef _EMITTEDPARTICLE_HF_
#define _EMITTEDPARTICLE_HF_

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	nointerpolation float4 opaAddDarkSiz	: TEXCOORD1;
	float4 pp				: TEXCOORD2;
};

#endif // _EMITTEDPARTICLE_HF_
