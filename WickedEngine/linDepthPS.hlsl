#include "postProcessHF.hlsli"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	return getLinearDepth(xTexture.SampleLevel(sampler_point_clamp,PSIn.tex,0).r) * g_xFrame_MainCamera_ZFarP_Recip; // store in range 0-1 for reduced precision
}