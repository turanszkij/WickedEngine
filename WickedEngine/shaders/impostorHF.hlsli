#ifndef WI_IMPOSTOR_HF
#define WI_IMPOSTOR_HF

struct VSOut
{
	float4 pos						: SV_POSITION;
	float3 tex						: TEXCOORD;
	nointerpolation float dither	: DITHER;
	float3 pos3D					: WORLDPOSITION;
	uint instanceColor				: INSTANCECOLOR;
	float4 pos2DPrev				: SCREENPOSITIONPREV;
};

TEXTURE2DARRAY(impostorTex, float4, TEXSLOT_ONDEMAND0);

#endif // WI_IMPOSTOR_HF
