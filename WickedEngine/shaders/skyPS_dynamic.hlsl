#define DISABLE_SOFT_SHADOWMAP
#include "skyHF.hlsli"
#include "fogHF.hlsli"

struct PushData
{
	uint clouds_enabled;
};
PUSHCONSTANT(push, PushData);

float4 main(float4 pos : SV_Position) : SV_Target
{
	const float3 V = normalize(GetCamera().screen_to_farplane(pos) - GetCamera().position);
	
	bool highQuality = GetFrame().options & OPTION_BIT_REALISTIC_SKY_HIGH_QUALITY;
	bool perPixelNoise = GetFrame().options & OPTION_BIT_TEMPORALAA_ENABLED;
	bool receiveShadow = GetFrame().options & OPTION_BIT_REALISTIC_SKY_RECEIVE_SHADOW;

	// Calculate dynamic sky
	float4 color = float4(GetDynamicSkyColor(pos.xy, V, true, false, false, highQuality, perPixelNoise, receiveShadow, push.clouds_enabled), 1);

	color = saturateMediump(color);
	return color;
}
