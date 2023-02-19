#define OBJECTSHADER_USE_NORMAL
#define OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
#define DISABLE_SOFT_SHADOWMAP
#define TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
#include "objectHF.hlsli"
#include "lightingHF.hlsli"
#include "skyHF.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
	float3 normal = normalize(input.nor);
		
	bool highQuality = GetFrame().options & OPTION_BIT_REALISTIC_SKY_HIGH_QUALITY;
	bool receiveShadow = GetFrame().options & OPTION_BIT_REALISTIC_SKY_RECEIVE_SHADOW;

	float4 color = float4(GetDynamicSkyColor(input.pos.xy, normal, true, false, false, highQuality, false, receiveShadow), 1);

	// Apply height fog on sky
	if (GetFrame().options & OPTION_BIT_HEIGHT_FOG)
	{
		// Layer fog on top of color
		float4 fog = GetFog(FLT_MAX, GetCamera().position, normal);
		color.rgb = (1.0 - fog.a) * color.rgb + fog.rgb;
	}
	
	color = clamp(color, 0, 65000);
	return float4(color.rgb, 1);
}
