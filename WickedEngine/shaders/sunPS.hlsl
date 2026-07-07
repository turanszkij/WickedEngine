#include "objectHF.hlsli"
#include "globals.hlsli"
#include "skyHF.hlsli"

float4 main(float4 pos : SV_Position) : SV_Target
{
	const float3 V = normalize(-GetCamera().screen_to_view(pos));

	return float4(saturateMediump(GetDynamicSkyColor(V, true, true)), 1);
}
