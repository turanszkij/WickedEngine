#include "objectHF.hlsli"

float4 main(float4 pos : SV_POSITION, float2 tex : TEXCOORD, uint RTIndex : SV_RenderTargetArrayIndex) : SV_TARGET
{
	return DEGAMMA(xBaseColorMap.Sample(sampler_linear_clamp, tex));
}