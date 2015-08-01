#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"

PS_OUT main(GS_OUT PSIn)
{
#ifdef GRASS_FADE_DITHER
	clip(dither(PSIn.pos.xy) - PSIn.fade);
#endif
	
	PS_OUT Out = (PS_OUT)0;
	Out.col = float4(PSIn.col,1);
	Out.nor = float4(PSIn.nor,0);
	//Out.spe = float4(PSIn.col,0.1f);
	//Out.mat = float4(50,0,0,0);
	return Out;
}