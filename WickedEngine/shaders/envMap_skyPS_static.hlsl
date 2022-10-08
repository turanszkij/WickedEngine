#define OBJECTSHADER_USE_NORMAL
#define OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
#include "objectHF.hlsli"
#include "skyHF.hlsli"

TextureCube<float4> texture_sky : register(t0);

float4 main(PixelInput input) : SV_TARGET
{
	float3 normal = normalize(input.nor);
	float3 color = DEGAMMA_SKY(texture_sky.SampleLevel(sampler_linear_clamp, normal, 0).rgb);
	color = clamp(color, 0, 65000);
	return float4(color, 1);
}
