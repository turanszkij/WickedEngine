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

	float2 ScreenCoord, ScreenCoordPrev;
	ScreenCoord.x = PSIn.pos2D.x / PSIn.pos2D.w / 2.0f + 0.5f;
	ScreenCoord.y = -PSIn.pos2D.y / PSIn.pos2D.w / 2.0f + 0.5f;
	ScreenCoordPrev.x = PSIn.pos2DPrev.x / PSIn.pos2DPrev.w / 2.0f + 0.5f;
	ScreenCoordPrev.y = -PSIn.pos2DPrev.y / PSIn.pos2DPrev.w / 2.0f + 0.5f;
	float2 vel = ScreenCoord - ScreenCoordPrev;

	PS_OUT Out = (PS_OUT)0;
	float4 col = xTexture.Sample(xSampler,PSIn.tex);
	clip( col.a < 0.1 ? -1:1 );
	Out.col = float4(col.rgb,1);
	Out.nor = float4(PSIn.nor,0);
	Out.vel = float4(vel,0,0);
	//Out.spe = float4(PSIn.col,0.1f);
	return Out;
}