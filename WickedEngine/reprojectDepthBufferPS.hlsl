#include "postProcessHF.hlsli"
#include "reconstructPositionHF.hlsli"

float main(VertexToPixelPostProcess PSIn) : SV_DEPTH
{
	float prevDepth = texture_0.Sample(sampler_point_clamp, PSIn.tex).r;

	float3 P = getPositionEx(PSIn.tex, prevDepth, g_xFrame_MainCamera_PrevInvVP);

	float4 reprojectedP = mul(float4(P,1), g_xCamera_VP);

	return reprojectedP.z / reprojectedP.w;
}