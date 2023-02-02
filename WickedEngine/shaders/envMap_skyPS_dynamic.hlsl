#define OBJECTSHADER_USE_NORMAL
#define OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
#include "objectHF.hlsli"
#include "lightingHF.hlsli"
#include "skyHF.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
	float4 color = 0;
	
	// When realistic sky 'High Quality' is enabled, let's disable skybox color since we render high quality sky in SkyAtmosphere
	[branch]
	if ((GetFrame().options & OPTION_BIT_REALISTIC_SKY_HIGH_QUALITY) == 0)
	{
		float3 normal = normalize(input.nor);
		color = float4(GetDynamicSkyColor(normal), 1);
		color = clamp(color, 0, 65000);
	}
	
	return float4(color.rgb, 1);
}
