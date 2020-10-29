#include "objectHF.hlsli"
#include "globals.hlsli"
#include "skyHF.hlsli"

float4 main(float4 pos : SV_POSITION, float2 clipspace : TEXCOORD) : SV_TARGET
{
	float4 unprojected = mul(g_xCamera_InvVP, float4(clipspace, 0.0f, 1.0f));
	unprojected.xyz /= unprojected.w;

	const float3 origin = g_xCamera_CamPos;
	const float3 direction = normalize(unprojected.xyz - origin);

	return float4(GetDynamicSkyColor(direction, true, true, true), 1);
}
