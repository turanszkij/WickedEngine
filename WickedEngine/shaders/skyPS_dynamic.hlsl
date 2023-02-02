#include "objectHF.hlsli"
#include "skyHF.hlsli"

float4 main(float4 pos : SV_POSITION, float2 clipspace : TEXCOORD) : SV_TARGET
{
	float4 color = 0;
	
	// When realistic sky 'High Quality' is enabled, let's disable skybox color since we render high quality sky in SkyAtmosphere
	[branch]
	if ((GetFrame().options & OPTION_BIT_REALISTIC_SKY_HIGH_QUALITY) == 0)
	{
		float4 unprojected = mul(GetCamera().inverse_view_projection, float4(clipspace, 0.0f, 1.0f));
		unprojected.xyz /= unprojected.w;

		const float3 V = normalize(unprojected.xyz - GetCamera().position);
		
		color = float4(GetDynamicSkyColor(V), 1);
		color = clamp(color, 0, 65000);
	}
	
	return color;
}
