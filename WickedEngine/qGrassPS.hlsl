#include "grassHF_GS.hlsli"
#include "grassHF_PS.hlsli"
#include "ditherHF.hlsli"

PS_OUT main(QGS_OUT PSIn)
{
#ifdef GRASS_FADE_DITHER
	clip(dither(PSIn.pos.xy) - PSIn.fade);
#endif

	float2 ScreenCoord = float2(1, -1) * PSIn.pos2D.xy / PSIn.pos2D.w / 2.0f + 0.5f;
	float2 ScreenCoordPrev = float2(1, -1) * PSIn.pos2DPrev.xy / PSIn.pos2DPrev.w / 2.0f + 0.5f;
	float2 vel = ScreenCoord - ScreenCoordPrev;

	PS_OUT Out = (PS_OUT)0;
	float4 col = texture_0.Sample(sampler_linear_clamp,PSIn.tex);
	ALPHATEST(col.a)
	col = DEGAMMA(col);
	Out.col = float4(col.rgb,1);
	Out.nor = float4(PSIn.nor,0);
	Out.vel = float4(vel,0,0);
	//Out.spe = float4(PSIn.col,0.1f);
	return Out;
}