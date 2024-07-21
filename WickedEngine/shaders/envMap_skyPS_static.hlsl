#define OBJECTSHADER_USE_NORMAL
#define OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
#include "objectHF.hlsli"
#include "skyHF.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
	float3 normal = normalize(input.nor);
	float3 color = GetStaticSkyColor(normal);
	color = saturateMediump(color);
	return float4(color, 1);
}
