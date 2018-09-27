#ifndef _IMPOSTOR_HF_
#define _IMPOSTOR_HF_

struct VSOut
{
	float4 pos : SV_POSITION;
	float3 tex : TEXCOORD;
	nointerpolation float dither : DITHER;
};

TEXTURE2DARRAY(impostorTex, float4, TEXSLOT_ONDEMAND0);

#endif // _IMPOSTOR_HF_
