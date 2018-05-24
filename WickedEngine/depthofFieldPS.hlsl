#include "postProcessHF.hlsli"
//#include "ViewProp.h"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,0);

	color += xTexture.SampleLevel(Sampler,PSIn.tex,0);

	float targetDepth = xPPParams0[2];

	float fragmentDepth = texture_lineardepth.SampleLevel(Sampler, PSIn.tex, 0).r * g_xFrame_MainCamera_ZFarP;
	float difference = abs(targetDepth - fragmentDepth);

	color = lerp(color,xMaskTex.SampleLevel(Sampler,PSIn.tex,0),abs(clamp(difference*0.008f,-1,1)));

	return color;
}
