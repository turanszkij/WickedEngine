#define OBJECTSHADER_USE_NORMAL
#define OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
#include "objectHF.hlsli"
#include "lightingHF.hlsli"
#include "skyHF.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
	float3 normal = normalize(input.nor);
	float4 color = float4(GetDynamicSkyColor(normal), 1);
	color = clamp(color, 0, 65000);
	return float4(color.rgb, 1);
}
