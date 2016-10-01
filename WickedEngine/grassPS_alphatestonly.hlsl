#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"

float4 main(GS_OUT PSIn) : SV_Target
{
#ifdef GRASS_FADE_DITHER
	clip(dither(PSIn.pos.xy) - PSIn.fade);
#endif

	return 0;
}