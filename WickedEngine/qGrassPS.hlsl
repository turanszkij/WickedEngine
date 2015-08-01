#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"
Texture2D xTexture:register(t0);
SamplerState xSampler:register(s0);

PS_OUT main(QGS_OUT PSIn)
{
#ifdef GRASS_FADE_DITHER
	clip(dither(PSIn.pos.xy) - PSIn.fade);
#endif

	PS_OUT Out = (PS_OUT)0;
	float4 col = xTexture.Sample(xSampler,PSIn.tex);
	clip( col.a < 0.1 ? -1:1 );
	Out.col = float4(col.rgb,1);
	Out.nor = float4(PSIn.nor,0);
	//Out.spe = float4(PSIn.col,0.1f);
	return Out;
}