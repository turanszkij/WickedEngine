#define DISABLE_SOFT_SHADOWMAP
#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
#include "objectHF.hlsli"
#include "skyHF.hlsli"
#include "fogHF.hlsli"

float4 main(float4 pos : SV_POSITION, float2 clipspace : TEXCOORD) : SV_TARGET
{	
	float4 unprojected = mul(GetCamera().inverse_view_projection, float4(clipspace, 0.0f, 1.0f));
	unprojected.xyz /= unprojected.w;

	const float3 V = normalize(unprojected.xyz - GetCamera().position);
	
	bool highQuality = GetFrame().options & OPTION_BIT_REALISTIC_SKY_HIGH_QUALITY;
	bool perPixelNoise = GetFrame().options & OPTION_BIT_TEMPORALAA_ENABLED;
	bool receiveShadow = GetFrame().options & OPTION_BIT_REALISTIC_SKY_RECEIVE_SHADOW;

	// Calculate dynamic sky
	float4 color = float4(GetDynamicSkyColor(pos.xy, V, true, false, false, highQuality, perPixelNoise, receiveShadow), 1);

	color = saturateMediump(color);
	return color;
}
