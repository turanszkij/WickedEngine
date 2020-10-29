#include "objectHF.hlsli"
#include "skyHF.hlsli"

GBUFFEROutputType main(float4 pos : SV_POSITION, float2 clipspace : TEXCOORD)
{
	float4 unprojected = mul(g_xCamera_InvVP, float4(clipspace, 0.0f, 1.0f));
	unprojected.xyz /= unprojected.w;

	const float3 V = normalize(unprojected.xyz - g_xCamera_CamPos);

	float4 color = float4(GetDynamicSkyColor(V), 1);

	float4 pos2DPrev = mul(g_xFrame_MainCamera_PrevVP, float4(unprojected.xyz, 1));
	float2 velocity = ((pos2DPrev.xy / pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (clipspace - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);

	GBUFFEROutputType Out = (GBUFFEROutputType)0;
	Out.g0 = color;
	Out.g1 = float4(0, 0, velocity);
	return Out;
}
