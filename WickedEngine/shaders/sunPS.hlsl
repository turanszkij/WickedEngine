#include "objectHF.hlsli"
#include "globals.hlsli"
#include "skyHF.hlsli"

float4 main(float4 pos : SV_POSITION, float2 clipspace : TEXCOORD) : SV_TARGET
{
	float4 unprojected = mul(GetCamera().inverse_view_projection, float4(clipspace, 0.0f, 1.0f));
	unprojected.xyz /= unprojected.w;

	const float3 origin = GetCamera().position;
	const float3 direction = normalize(unprojected.xyz - origin);

	return float4(saturateMediump(GetDynamicSkyColor(direction, true, true)), 1);
}
